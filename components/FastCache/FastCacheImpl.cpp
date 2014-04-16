#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/FastCache/FastCache.hpp>

#include <components/FastCache/LookupResult.hpp>

#include <components/FastCache/CoherenceProtocol.hpp>
#include <components/FastCache/InclusiveMOESI.hpp>

#include <components/FastCache/CacheStats.hpp>
#include <components/FastCache/AbstractCache.hpp>
#include <components/FastCache/RTCache.hpp>
#include <components/FastCache/StdCache.hpp>

#include <components/Common/TraceTracker.hpp>

#include <core/performance/profile.hpp>

#include <boost/bind.hpp>
#include <fstream>
#include <string>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/export.hpp>

#define DBG_DefineCategories RTCacheCat
#define DBG_SetDefaultOps AddCat(RTCacheCat) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nFastCache {

using namespace Flexus;

class FLEXUS_COMPONENT(FastCache) {
  FLEXUS_COMPONENT_IMPL( FastCache );

  CoherenceProtocol * theProtocol;
  AbstractCache * theCache;
  CacheStats * theStats;

  uint64_t theBlockMask;

  MemoryMessage theEvictMessage;
  MemoryMessage theInvalidateMessage;

  int32_t theIndex;

  LookupResult_p lookup;
  LookupResult_p snp_lookup;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastCache)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theEvictMessage(MemoryMessage::EvictDirty)
    , theInvalidateMessage(MemoryMessage::Invalidate) {
  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
    std::string fname( aDirName );
    fname += "/" + statName();
    if (cfg.GZipFlexpoints) {
      fname += ".gz";
    }

    std::ios_base::openmode omode = std::ios::out;
    if (!cfg.TextFlexpoints) {
      omode |= std::ios::binary;
    }

    std::ofstream ofs(fname.c_str(), omode);

    boost::iostreams::filtering_stream<boost::iostreams::output> out;
    if (cfg.GZipFlexpoints) {
      out.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(9)));
    }
    out.push(ofs);

    theCache->saveState ( out );
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + statName();
    if (cfg.GZipFlexpoints) {
      fname += ".gz";
    }

    std::ios_base::openmode omode = std::ios::in;
    if (!cfg.TextFlexpoints) {
      omode |= std::ios::binary;
    }

    std::ifstream ifs(fname.c_str(), omode);
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      //ifs >> std::skipws;

      boost::iostreams::filtering_stream<boost::iostreams::input> in;
      if (cfg.GZipFlexpoints) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(ifs);

      if ( ! theCache->loadState( in ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                      ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
    }
  }

  void initialize(void) {
    static volatile bool widthPrintout = true;

    if (widthPrintout) {
      DBG_( Crit, ( << "Running with MT width " << cfg.MTWidth ) );
      widthPrintout = false;
    }

    theProtocol = GenerateCoherenceProtocol(cfg.Protocol, cfg.UsingTraces,
                                            boost::bind( &FastCacheComponent::forwardMessage, this, _1),
                                            boost::bind( &FastCacheComponent::continueSnoop, this, _1),
                                            boost::bind( &FastCacheComponent::sendInvalidate, this, _1, _2, _3),
                                            cfg.DowngradeLRU,
                                            cfg.SnoopLRU);
    theStats = new CacheStats(statName());
    theIndex = flexusIndex();

    theEvictMessage.coreIdx() = theIndex;

    //Confirm that BlockSize is a power of 2
    DBG_Assert( (cfg.BlockSize & (cfg.BlockSize - 1)) == 0);
    DBG_Assert( cfg.BlockSize  >= 4);

    int32_t num_sets = cfg.Size / cfg.BlockSize / cfg.Associativity;

    //Confirm that num_sets is a power of 2
    DBG_Assert( (num_sets & (num_sets  - 1)) == 0);

    //Confirm that settings are consistent
    DBG_Assert( cfg.BlockSize * num_sets * cfg.Associativity == cfg.Size);

    //Calculate shifts and masks
    theBlockMask = ~(cfg.BlockSize - 1);

    if (cfg.StdArray) {
      theCache = new StdCache(statName(),
                              cfg.BlockSize,
                              num_sets,
                              cfg.Associativity,
                              boost::bind( &FastCacheComponent::evict, this, _1, _2),
                              boost::bind( &FastCacheComponent::sendInvalidate, this, _1, _2, _3),
                              theIndex,
                              cfg.CacheLevel,
                              cfg.RTReplPolicy,
                              cfg.TextFlexpoints
                             );
    } else {
      theCache = new RTCache( cfg.BlockSize,
                              num_sets,
                              cfg.Associativity,
                              boost::bind( &FastCacheComponent::evict, this, _1, _2),
                              boost::bind( &FastCacheComponent::evictRegion, this, _1, _2),
                              boost::bind( &FastCacheComponent::sendInvalidate, this, _1, _2, _3),
                              theIndex,
                              cfg.CacheLevel,
                              cfg.RegionSize,
                              cfg.RTAssoc,
                              cfg.RTSize,
                              cfg.ERBSize,
                              cfg.SkewBlockSet,
                              cfg.RTReplPolicy
                            );
    }

    Flexus::Stat::getStatManager()->addFinalizer(ll::bind( &nFastCache::FastCacheComponent::finalize, this ));

  }

  void finalize(void) {
    theStats->update();
  }

  void notifyRead( MemoryMessage & aMessage) {
    if (cfg.NotifyReads) {
      FLEXUS_CHANNEL( Reads ) << aMessage;
    }
  }

  void notifyWrite( MemoryMessage & aMessage) {
    if (cfg.NotifyWrites) {
      FLEXUS_CHANNEL( Writes ) << aMessage;
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchRequestIn);
  void push( interface::FetchRequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage) {
    aMessage.dstream() = false;
    DBG_( Iface, Addr(aMessage.address()) ( << "FetchRequestIn[" << anIndex << "]: " << aMessage << " tagset: " << std::hex << (aMessage.address() & theBlockMask) << std::dec ));
    push( interface::RequestIn(), anIndex, aMessage);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage) {
    FLEXUS_PROFILE();
    MemoryMessage orig_message(aMessage);
    orig_message.coreIdx() = aMessage.coreIdx();

    //Create a set and tag from the message's address
    uint64_t tagset = aMessage.address() & theBlockMask;
    DBG_( Iface, Addr(aMessage.address()) ( << "Request[" << anIndex << "]: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    // Map memory message type to protocol request types
    CoherenceProtocol::access_t access_type = CoherenceProtocol::message2access(aMessage.type());
    CoherenceProtocol::CoherenceActionPtr fn_ptr;
    CoherenceProtocol::StatMemberPtr stat_ptr;

    // Perform Cache Lookup
    lookup = theCache->lookup(tagset);

    DBG_( Iface, Addr(aMessage.address()) ( << flexusIndex() << ": Request[" << anIndex << "]: " << aMessage << " Initial State: " << std::hex << lookup->getState() << std::dec ));

    // Determine Actions based on state and request
    tie(fn_ptr, stat_ptr) = theProtocol->getCoherenceAction(lookup->getState(), access_type);

    // Perform Actions this includes setting reply type
    (theProtocol->*fn_ptr)(lookup, aMessage);

    // Increment stat counter
    (theStats->*stat_ptr)++;

    DBG_( Iface, Addr(aMessage.address()) ( << "Done, reply: " << aMessage ));
    DBG_( Iface, Addr(aMessage.address()) ( << "Request Left Lookup tagset: " << std::hex << lookup->address() << " in state " << state2String(lookup->getState()) << std::dec ));
    if (snp_lookup != NULL) {
      DBG_( Iface, Addr(aMessage.address()) ( << "Request Left Snoop Lookup tagset: " << std::hex << snp_lookup->address() << " in state " << state2String(snp_lookup->getState()) << std::dec ));
    }
  }

  void evict(uint64_t aTagset, CoherenceState_t aLineState) {

    theEvictMessage.address() = PhysicalMemoryAddress(aTagset);

    theEvictMessage.type() = theProtocol->evict(aTagset, aLineState);

    if (theEvictMessage.type() == MemoryMessage::EvictDirty || cfg.CleanEvictions) {
      DBG_( Iface, Addr(aTagset) ( << "Evict: " << theEvictMessage ));
      FLEXUS_CHANNEL( RequestOut ) << theEvictMessage;
    }
  }

  void evictRegion(uint64_t aTagset, int32_t owner) {
    RegionScoutMessage message(RegionScoutMessage::eRegionEvict, PhysicalMemoryAddress(aTagset));
    message.setOwner(owner);

    FLEXUS_CHANNEL( RegionNotify ) << message;
  }

  void forwardMessage(MemoryMessage & msg) {

    FLEXUS_CHANNEL( RequestOut ) << msg;

  }

  void continueSnoop(MemoryMessage & msg) {
    MemoryMessage dup_msg(msg);

    if (msg.type() != MemoryMessage::Downgrade) {
      FLEXUS_CHANNEL( SnoopOutI ) << dup_msg;
    }

    FLEXUS_CHANNEL( SnoopOutD ) << msg;

    // ReturnReq can get data from I or D cache.
    // All other requests, D cache response is the correct response
    if (msg.type() == MemoryMessage::ReturnNAck && dup_msg.type() != MemoryMessage::ReturnNAck) {
      msg.type() = dup_msg.type();
    }

  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
  void push( interface::SnoopIn const &, MemoryMessage & aMessage) {
    FLEXUS_PROFILE();
    uint64_t tagset = aMessage.address() & theBlockMask;

    DBG_( Iface, Addr(aMessage.address()) ( << "Snoop: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));
    snp_lookup = theCache->lookup(tagset);
    DBG_( Iface, Addr(aMessage.address()) ( << "Snoop Found tagset: " << std::hex << tagset << " in state " << state2String(snp_lookup->getState()) << std::dec ));

    CoherenceProtocol::BaseSnoopAction * fn_ptr;
    CoherenceProtocol::StatMemberPtr stat_ptr;

    // Determine Actions based on state and snoop type
    boost::tie(fn_ptr, stat_ptr) = theProtocol->getSnoopAction(snp_lookup->getState(), aMessage.type());

    // Perform Actions
    (*fn_ptr)(snp_lookup, aMessage);

    // Increment counter
    (theStats->*stat_ptr)++;

    snp_lookup = theCache->lookup(tagset);
    DBG_( Iface, Addr(aMessage.address()) ( << "Snoop Left tagset: " << std::hex << tagset << " in state " << state2String(snp_lookup->getState()) << std::dec ));
  }

  bool sendInvalidate(uint64_t addr, bool icache, bool dcache) {
    bool was_dirty = false;
    theInvalidateMessage.address() = PhysicalMemoryAddress(addr);

    if (icache) {
      theInvalidateMessage.type() = MemoryMessage::Invalidate;
      DBG_(Iface, Addr(addr) ( << "Sending ICache Invalidate: " << std::hex << addr << std::dec ));
      FLEXUS_CHANNEL( SnoopOutI) << theInvalidateMessage;
      // We can cached a dirty copy in the ICache too!!
      was_dirty = (theInvalidateMessage.type() == MemoryMessage::InvUpdateAck);
    }

    if (dcache) {
      theInvalidateMessage.type() = MemoryMessage::Invalidate;
      DBG_(Iface, Addr(addr) ( << "Sending DCache Invalidate: " << std::hex << addr << std::dec ));
      FLEXUS_CHANNEL( SnoopOutD ) << theInvalidateMessage;
    }

    return (was_dirty || (theInvalidateMessage.type() == MemoryMessage::InvUpdateAck));
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(RegionProbe);
  void push( interface::RegionProbe const &, RegionScoutMessage & aMessage) {
    switch (aMessage.type()) {
      case RegionScoutMessage::eRegionProbe: {
        uint32_t present = theCache->regionProbe(aMessage.region());
        if (present) {
          aMessage.setType(RegionScoutMessage::eRegionHitReply);
          aMessage.setBlocks(present);
        } else {
          aMessage.setType(RegionScoutMessage::eRegionMissReply);
        }
        break;
      }
      case RegionScoutMessage::eBlockProbe: {
        uint32_t present = theCache->blockProbe(aMessage.region());
        if (present) {
          aMessage.setType(RegionScoutMessage::eBlockHitReply);
        } else {
          aMessage.setType(RegionScoutMessage::eBlockMissReply);
        }
        break;
      }
      case RegionScoutMessage::eBlockScoutProbe: {
        uint32_t present = theCache->blockScoutProbe(aMessage.region());
        if (present) {
          aMessage.setType(RegionScoutMessage::eRegionHitReply);
          aMessage.setBlocks(present);
        } else {
          aMessage.setType(RegionScoutMessage::eRegionMissReply);
        }
        break;
      }
      case RegionScoutMessage::eRegionStateProbe: {
        bool non_shared = theCache->isNonSharedRegion(aMessage.region());
        if (non_shared) {
          aMessage.setType(RegionScoutMessage::eRegionNonShared);
        } else {
          aMessage.setType(RegionScoutMessage::eRegionIsShared);
        }
        break;
      }
      case RegionScoutMessage::eRegionGlobalMiss: {
        theCache->setNonSharedRegion(aMessage.region());
        break;
      }
      case RegionScoutMessage::eRegionPartialMiss: {
        theCache->setPartialSharedRegion(aMessage.region(), aMessage.blocks());
        break;
      }
      case RegionScoutMessage::eRegionProbeOwner: {
        int32_t owner = theCache->getOwner(lookup, aMessage.region());
        aMessage.setType(RegionScoutMessage::eRegionOwnerReply);
        aMessage.setOwner(owner);
        break;
      }
      case RegionScoutMessage::eRegionSetOwner: {
        theCache->updateOwner(lookup, aMessage.owner(), aMessage.region(), aMessage.shared());
        break;
      }
      case RegionScoutMessage::eSetTagProbe:
        theCache->getSetTags(aMessage.region(), aMessage.getTags());
        break;
      default:
        DBG_Assert( false, ( << "Received invalid RegionScoutMessage " << aMessage ));
        break;
    }
  }

  void drive( interface::UpdateStatsDrive const &) {
    theStats->update();
  }

};  // end class FastCache

}  // end Namespace nFastCache

FLEXUS_COMPONENT_INSTANTIATOR( FastCache, nFastCache);

FLEXUS_PORT_ARRAY_WIDTH( FastCache, RequestIn)      {
  return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH( FastCache, FetchRequestIn) {
  return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH( FastCache, SnoopOutD)       {
  return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH( FastCache, SnoopOutI)       {
  return (cfg.MTWidth);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCache

#define DBG_Reset
#include DBG_Control()
