#include <components/FastVictimCache/FastVictimCache.hpp>
#include <core/stats.hpp>

#include <components/Common/seq_map.hpp>

#define DBG_DefineCategories VictimCache
#define DBG_SetDefaultOps AddCat(VictimCache) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastVictimCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nFastVictimCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(FastVictimCache) {
  FLEXUS_COMPONENT_IMPL( FastVictimCache );

  typedef uint64_t tAddress;
  typedef flexus_boost_seq_map < tAddress, bool > tVictimCache;
  typedef tVictimCache::iterator tVictimCacheIter;
  tVictimCache theVictimCache;

  tAddress theBlockMask;
  MemoryMessage theEvictMessage;

  bool theIsPurgeOutstanding; /* CMU-ONLY */

  Stat::StatCounter theHits_Read;
  Stat::StatCounter theHits_Write;
  Stat::StatCounter theHits_ReturnReq;
  Stat::StatCounter theHits_Invalidate;
  Stat::StatCounter theHits_Downgrade;
  Stat::StatCounter theHits_Purge; /* CMU-ONLY */

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastVictimCache)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theEvictMessage(MemoryMessage::EvictDirty)

    , theIsPurgeOutstanding(false) /* CMU-ONLY */

    , theHits_Read(statName() + "-Hits:Read")
    , theHits_Write(statName() + "-Hits:Write")
    , theHits_ReturnReq(statName() + "-Hits:ReturnReq")
    , theHits_Invalidate(statName() + "-Hits:Invalidate")
    , theHits_Downgrade(statName() + "-Hits:Downgrade")
    , theHits_Purge(statName() + "-Hits:Purge") /* CMU-ONLY */
  {}

public:
  // Initialization
  void initialize() {
    theBlockMask = ~(static_cast<tAddress>(cfg.CacheLineSize) - 1ULL);
  }

  tAddress blockAddr( const tAddress anAddr ) const {
    return (anAddr & theBlockMask);
  }

  bool isQuiesced() const {
    return true; //FastVictimCache is always quiesced
  }

  // request input port
  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &
             , MemoryMessage & aMessage
           ) {
    if (   !cfg.Enable
           || theIsPurgeOutstanding /* CMU-ONLY */
       ) {
      FLEXUS_CHANNEL( RequestOut ) << aMessage;
      return;
    }

    DBG_( Iface, Addr(aMessage.address()) ( << "RequestIn: " << aMessage ));

    const uint64_t tagset = blockAddr(aMessage.address());
    tVictimCacheIter addrIter = theVictimCache.find( tagset );

    switch (aMessage.type()) {
      case MemoryMessage::ReadReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::UpgradeReq: {
        if (addrIter != theVictimCache.end()) {
          // found - this is a victim cache hit
          if (aMessage.type() == MemoryMessage::ReadReq) {
            ++theHits_Read;
          } else {
            ++theHits_Write;
          }
          if (addrIter->second) {
            aMessage.type() = MemoryMessage::MissReplyDirty;
          } else if (aMessage.type() == MemoryMessage::WriteReq) {
            DBG_( VVerb, Addr(tagset) ( << "Victim write -> upgrade: " << std::hex << tagset << std::dec << " msg: " << aMessage ));
            aMessage.type() = MemoryMessage::UpgradeReq;
            FLEXUS_CHANNEL( RequestOut ) << aMessage;
          } else {
            aMessage.type() = MemoryMessage::MissReply;
          }
          theVictimCache.erase(addrIter);
          DBG_( VVerb, Addr(tagset) ( << "Victim hit. Erase entry. Msg: " << aMessage ));
        } else {
          // not a hit here - pass to L2
          DBG_( VVerb, Addr(tagset) ( << "Victim miss: " << std::hex << tagset << std::dec << " msg: " << aMessage ));
          FLEXUS_CHANNEL( RequestOut ) << aMessage;
        }
        break;
      }

      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty: {
        if (theVictimCache.size() >= cfg.NumEntries) {
          // evict to L2
          theEvictMessage.type() = (theVictimCache.front() ? MemoryMessage::EvictDirty : MemoryMessage::EvictClean);
          theEvictMessage.address() = PhysicalMemoryAddress(theVictimCache.front_key());
          DBG_( Iface, Addr(theEvictMessage.address()) ( << "Evict: " << theEvictMessage ));
          FLEXUS_CHANNEL( RequestOut ) << theEvictMessage;
          theVictimCache.pop_front();
        }
        bool isDirty = (aMessage.type() == MemoryMessage::EvictClean ? false : true);
        theVictimCache.push_back( std::make_pair(tagset, isDirty) );
        DBG_( VVerb, Addr(tagset) ( << "Victim alloc: " << std::hex << tagset << std::dec << " msg: " << aMessage ));
        DBG_Assert(theVictimCache.find(tagset) != theVictimCache.end(), ( << "addr: " << std::hex << tagset
                   << " search for: " << (tagset | 0x3F) << std::dec
                   << " Msg: " << aMessage ));
        break;
      }

      default:
        DBG_Assert(false, ( << "Invalid message: " << aMessage ) );
    }
  }

  // snoop input port
  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
  void push( interface::SnoopIn const &,
             MemoryMessage & aMessage ) {
    if (!cfg.Enable) {
      FLEXUS_CHANNEL( SnoopOut ) << aMessage;
      return;
    }

    DBG_( Iface, Addr(aMessage.address()) ( << "SnoopIn: " << aMessage ));

    const uint64_t tagset = blockAddr(aMessage.address());
    tVictimCacheIter addrIter = theVictimCache.find( tagset );

    if (addrIter == theVictimCache.end()) {
      // not a hit here - pass to L1
      DBG_( VVerb, Addr(tagset) ( << "Snoop miss: " << aMessage ));
      /* CMU-ONLY-BLOCK-BEGIN */
      if (    aMessage.type() == MemoryMessage::PurgeIReq
              || aMessage.type() == MemoryMessage::PurgeDReq
         ) {
        theIsPurgeOutstanding = true;
      }
      /* CMU-ONLY-BLOCK-END */
      FLEXUS_CHANNEL( SnoopOut ) << aMessage;
      theIsPurgeOutstanding = false;    /* CMU-ONLY */
      return;
    }

    DBG_( VVerb, Addr(tagset) ( << "Snoop hit: " << aMessage ));
    switch (aMessage.type()) {
      case MemoryMessage::ReturnReq: {
        ++theHits_ReturnReq;
        aMessage.type() = MemoryMessage::ReturnReply;
        // Do not remove from victim buffer. L1 may still want it!
        break;
      }

      case MemoryMessage::Downgrade: {
        ++theHits_Downgrade;
        DBG_Assert(addrIter->second);
        addrIter->second = false; // downgrade in place
        aMessage.type() = MemoryMessage::DownUpdateAck;
        // Do not remove from victim buffer. L1 may still want it!
        break;
      }

      case MemoryMessage::Invalidate: {
        ++theHits_Invalidate;
        if (addrIter->second) {
          aMessage.type() = MemoryMessage::InvUpdateAck;
        } else {
          aMessage.type() = MemoryMessage::InvalidateAck;
        }
        theVictimCache.erase(addrIter);
        break;
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::PurgeIReq:
      case MemoryMessage::PurgeDReq: {
        ++theHits_Purge;

        // evict to L2
        theEvictMessage.type() = (addrIter->second ? MemoryMessage::EvictDirty : MemoryMessage::EvictClean);
        theEvictMessage.address() = PhysicalMemoryAddress(tagset);
        DBG_( Iface, Addr(tagset) ( << "Evict: " << theEvictMessage ));
        FLEXUS_CHANNEL( RequestOut ) << theEvictMessage;

        aMessage.type() = MemoryMessage::PurgeAck;
        theVictimCache.erase(addrIter);
        break;
      }
      /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert( false );
    }
  }

};

} //End Namespace nFastVictimCache

FLEXUS_COMPONENT_INSTANTIATOR( FastVictimCache, nFastVictimCache );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastVictimCache
