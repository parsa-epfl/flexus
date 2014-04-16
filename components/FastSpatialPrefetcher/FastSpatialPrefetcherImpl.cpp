#include <components/FastSpatialPrefetcher/FastSpatialPrefetcher.hpp>

#include <memory>
#include <zlib.h>

#include <components/Common/Slices/TransactionTracker.hpp>

#include <components/Common/TraceTracker.hpp>

#include <components/SpatialPrefetcher/seq_map.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

#define FLEXUS_BEGIN_COMPONENT FastCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nSpatialPrefetcher {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(SpatialPrefetcher) {
  FLEXUS_COMPONENT_IMPL( SpatialPrefetcher );

  int32_t theNodeId;
  std::string theName;
  int64_t theBlockMask;
  MemoryMessage thePrefetchMessage;

  typedef flexus_boost_seq_map<uint32_t, MemoryMessage::MemoryMessageType> BufferType;
  typedef BufferType::iterator BufIter;
  BufferType theBuffer;

  Stat::StatCounter statPrefetches;
  Stat::StatCounter statCompleted;
  Stat::StatCounter statRedundant;
  Stat::StatCounter statRedundantInBuffer;
  Stat::StatCounter statRedundantInL1;
  Stat::StatCounter statRedundantInL2;
  Stat::StatCounter statRedundantUnknown;
  Stat::StatCounter statReadHits;
  Stat::StatCounter statWriteMisses;
  Stat::StatCounter statReplacedBlocks;
  Stat::StatCounter statInvalidatedBlocks;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(SpatialPrefetcher)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theNodeId(flexusIndex())
    , theName(std::string(boost::padded_string_cast < 2, '0' > (theNodeId) + "-pb"))
    , thePrefetchMessage(MemoryMessage::PrefetchReadAllocReq)
    , statPrefetches(theName + "-Prefetches")
    , statCompleted(theName + "-Prefetches:Completed")
    , statRedundant(theName + "-Prefetches:Redundant")
    , statRedundantInBuffer(theName + "-Prefetches:Redundant:PresentInBuffer")
    , statRedundantInL1(theName + "-Prefetches:Redundant:PresentInL1")
    , statRedundantInL2(theName + "-Prefetches:Redundant:PresentInL2")
    , statRedundantUnknown(theName + "-Prefetches:Redundant:PresentUnknown")
    , statReadHits(theName + "-ReadHits")
    , statWriteMisses(theName + "-WriteMissBlocks")
    , statReplacedBlocks(theName + "-ReplacedBlocks")
    , statInvalidatedBlocks(theName + "-InvalidatedBlocks")
  {}

  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    theBlockMask = ~(cfg.BlockSize - 1);
    if (cfg.UsageEnable || cfg.RepetEnable || cfg.PrefetchEnable) {
      theTraceTracker.initSGP( theNodeId, cfg.CacheLevel,
                               cfg.UsageEnable, cfg.RepetEnable, cfg.BufFetchEnable,
                               cfg.TimeRepetEnable, cfg.PrefetchEnable, cfg.ActiveEnable,
                               cfg.OrderEnable, cfg.StreamEnable,
                               cfg.BlockSize, cfg.SgpBlocks, cfg.RepetType, cfg.RepetFills,
                               cfg.SparseOpt, cfg.PhtSize, cfg.PhtAssoc, cfg.PcBits,
                               cfg.CptType, cfg.CptSize, cfg.CptAssoc, cfg.CptSparse,
                               cfg.FetchDist, cfg.StreamWindow, cfg.StreamDense, false /*sendStreams*/,
                               cfg.BufSize, cfg.StreamDescs, false /*delayed commits*/, cfg.CptFilter );
      if (cfg.BufferPrefetching) {
        DBG_Assert(cfg.PrefetchEnable);
        // nothing to initialize for a fully-associative buffer
      }
    }
    if (cfg.GhbEnable) {
      DBG_Assert(!cfg.PrefetchEnable);
      DBG_Assert(cfg.PrefetchLevel == eL1);
      theTraceTracker.initGHB(theNodeId, cfg.CacheLevel, cfg.BlockSize, cfg.GhbSize);
    }
  }

  void saveState(std::string const & aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.saveSGP(theNodeId, cfg.CacheLevel, aDirName);
    }
  }

  void loadState(std::string const & aDirName) {
    if (cfg.RepetEnable) {
      theTraceTracker.loadSGP(theNodeId, cfg.CacheLevel, aDirName);
    } else {
      DBG_(Dev, ( << "Warning: SGP state not loaded because Repet not enabled" ) );
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestInL1);
  void push( interface::RequestInL1 const &, MemoryMessage & aMessage) {
    uint32_t tagset = aMessage.address() & theBlockMask;
    DBG_( Iface, ( << "Request L1: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    FLEXUS_CHANNEL( RequestOutL1 ) << aMessage;

    // try prefetching
    if (cfg.PrefetchLevel == eL1) {
      if (cfg.PrefetchEnable) {
        while (theTraceTracker.prefetchReady(theNodeId, cfg.CacheLevel)) {
          thePrefetchMessage.address() = PhysicalMemoryAddress( theTraceTracker.getPrefetch(theNodeId, cfg.CacheLevel) );
          if (cfg.BufferPrefetching) {
            bool send = sendPrefetch();
            if (send) {
              thePrefetchMessage.type() = MemoryMessage::PrefetchReadNoAllocReq;
              FLEXUS_CHANNEL( RequestOutL1 ) << thePrefetchMessage;
              handlePrefetchReply();
            }
          } else {
            thePrefetchMessage.type() = MemoryMessage::PrefetchReadAllocReq;
            FLEXUS_CHANNEL( RequestOutL1 ) << thePrefetchMessage;
          }
        }
      }
      if (cfg.GhbEnable) {
        while (theTraceTracker.ghbPrefetchReady(theNodeId, cfg.CacheLevel)) {
          thePrefetchMessage.type() = MemoryMessage::PrefetchReadAllocReq;
          thePrefetchMessage.address() = PhysicalMemoryAddress( theTraceTracker.ghbGetPrefetch(theNodeId, cfg.CacheLevel) );
          FLEXUS_CHANNEL( RequestOutL1 ) << thePrefetchMessage;
        }
      }
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestInL2);
  void push( interface::RequestInL2 const &, MemoryMessage & aMessage) {
    uint32_t tagset = aMessage.address() & theBlockMask;
    DBG_( Iface, ( << "Request L2: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    if (cfg.BufferPrefetching && cfg.PrefetchLevel == eL1) {
      // handle the request with respect to the prefetch buffer
      bool completed = handleRequestMessage(aMessage);
      if (completed) {
        return;
      }
    }

    FLEXUS_CHANNEL( RequestOutL2 ) << aMessage;

    // try prefetching
    if (cfg.PrefetchLevel == eL2) {
      if (cfg.PrefetchEnable) {
        while (theTraceTracker.prefetchReady(theNodeId, cfg.CacheLevel)) {
          thePrefetchMessage.address() = PhysicalMemoryAddress( theTraceTracker.getPrefetch(theNodeId, cfg.CacheLevel) );
          if (cfg.BufferPrefetching) {
            bool send = sendPrefetch();
            if (send) {
              thePrefetchMessage.type() = MemoryMessage::PrefetchReadNoAllocReq;
              FLEXUS_CHANNEL( RequestOutL2 ) << thePrefetchMessage;
              handlePrefetchReply();
            }
          } else {
            thePrefetchMessage.type() = MemoryMessage::PrefetchReadAllocReq;
            FLEXUS_CHANNEL( RequestOutL2 ) << thePrefetchMessage;
          }
        }
      }
      if (cfg.GhbEnable) {
        while (theTraceTracker.ghbPrefetchReady(theNodeId, cfg.CacheLevel)) {
          thePrefetchMessage.type() = MemoryMessage::PrefetchReadAllocReq;
          thePrefetchMessage.address() = PhysicalMemoryAddress( theTraceTracker.ghbGetPrefetch(theNodeId, cfg.CacheLevel) );
          FLEXUS_CHANNEL( RequestOutL2 ) << thePrefetchMessage;
        }
      }
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopInL1);
  void push( interface::SnoopInL1 const &, MemoryMessage & aMessage) {
    uint32_t tagset = aMessage.address() & theBlockMask;
    DBG_( Iface, ( << "Snoop L1: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    switch (aMessage.type()) {
      case MemoryMessage::Invalidate:
        if (cfg.BufferPrefetching && cfg.PrefetchLevel == eL1) {
          handleInvalidation(aMessage);
        }
        break;
      case MemoryMessage::Downgrade:
      case MemoryMessage::ReturnReq:
        break;
      default:
        DBG_Assert(false, ( << "Invalid snoop inL1: " << aMessage ) );
    }

    FLEXUS_CHANNEL( SnoopOutL1 ) << aMessage;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopInL2);
  void push( interface::SnoopInL2 const &, MemoryMessage & aMessage) {
    uint32_t tagset = aMessage.address() & theBlockMask;
    DBG_( Iface, ( << "Snoop L2: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    switch (aMessage.type()) {
      case MemoryMessage::Invalidate:
        if (cfg.BufferPrefetching && cfg.PrefetchLevel == eL2) {
          handleInvalidation(aMessage);
        }
        break;
      case MemoryMessage::Downgrade:
      case MemoryMessage::ReturnReq:
        break;
      default:
        DBG_Assert(false, ( << "Invalid snoop inL2: " << aMessage ) );
    }

    FLEXUS_CHANNEL( SnoopOutL2 ) << aMessage;
  }

  void drive( interface::PrefetchDrive const &) {
    //theStats->update();
  }

private:
  // returns true if the request is handled here (i.e., it should not be
  // passed down the hierarchy)
  bool handleRequestMessage( MemoryMessage & aMessage) {
    uint32_t tagset = aMessage.address() & theBlockMask;
    BufIter iter = theBuffer.find(tagset);
    if (iter == theBuffer.end()) {
      // cannot do anything if the block is not in the buffer
      return false;
    }

    bool handled = false;
    switch (aMessage.type()) {
      case MemoryMessage::ReadReq:
        ++statReadHits;
        thePrefetchMessage.address() = PhysicalMemoryAddress(tagset);
        if (iter->second == MemoryMessage::PrefetchWritableReply) {
          aMessage.type() = MemoryMessage::MissReplyWritable;
          thePrefetchMessage.type() = MemoryMessage::PrefetchInsertWritable;
        } else {
          aMessage.type() = MemoryMessage::MissReply;
          thePrefetchMessage.type() = MemoryMessage::PrefetchInsert;
        }
        FLEXUS_CHANNEL( RequestOutL2 ) << thePrefetchMessage;

        aMessage.fillLevel() = eL2Prefetcher;
        handled = true;
        theBuffer.erase(iter);
        break;
      case MemoryMessage::WriteReq:
      case MemoryMessage::UpgradeReq:
        ++statWriteMisses;
        theBuffer.erase(iter);
        break;
      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
        DBG_Assert(false, ( << "Eviction while block present in buffer: " << aMessage) );
        break;
      default:
        DBG_Assert(false, ( << "Cannot handle message: " << aMessage) );
    }

    return handled;
  }

  void handleInvalidation( MemoryMessage & aMessage) {
    DBG_Assert( aMessage.address() == (aMessage.address() & theBlockMask) );
    BufIter iter = theBuffer.find(aMessage.address());
    if (iter != theBuffer.end()) {
      theBuffer.erase(iter);
      ++statInvalidatedBlocks;
    }
  }

  void handlePrefetchReply() {
    bool alloc = false;
    switch (thePrefetchMessage.type()) {
      case MemoryMessage::PrefetchReadReply:
      case MemoryMessage::PrefetchWritableReply:
        ++statCompleted;
        alloc = true;
        break;
      case MemoryMessage::PrefetchReadRedundant:
        ++statRedundant;
        if (thePrefetchMessage.fillLevel() == eL1) {
          ++statRedundantInL1;
        } else if (thePrefetchMessage.fillLevel() == eL2) {
          ++statRedundantInL2;
        } else {
          ++statRedundantUnknown;
        }
        break;
      default:
        DBG_Assert(false, ( << "Invalid prefetch response message: " << thePrefetchMessage) );
    }

    if (alloc) {
      if ((int)theBuffer.size() >= cfg.BufSize) {
        theBuffer.pop_front();
        ++statReplacedBlocks;
      }
      theBuffer.push_back( std::make_pair(thePrefetchMessage.address(), thePrefetchMessage.type()) );
    }
  }

  // returns true if the prefetch should be issued
  bool sendPrefetch() {
    DBG_Assert( thePrefetchMessage.address() == (thePrefetchMessage.address() & theBlockMask) );
    ++statPrefetches;
    BufIter iter = theBuffer.find(thePrefetchMessage.address());
    if (iter != theBuffer.end()) {
      theBuffer.move_back(iter);
      ++statRedundant;
      ++statRedundantInBuffer;
      return false;
    }
    return true;
  }

};  // end class SpatialPrefetcher

} // end namespace nSpatialPrefetcher

FLEXUS_COMPONENT_INSTANTIATOR( SpatialPrefetcher,  nSpatialPrefetcher);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SpatialPrefetcher

#define DBG_Reset
#include DBG_Control()
