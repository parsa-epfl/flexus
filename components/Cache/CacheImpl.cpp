#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/Cache/Cache.hpp>

#include <boost/scoped_ptr.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include "CacheController.hpp"

#define DBG_DefineCategories Cache
#define DBG_SetDefaultOps AddCat(Cache)
#include DBG_Control()
#define DBG_DefineCategories CacheMissTracking
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT Cache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::scoped_ptr;

class FLEXUS_COMPONENT(Cache) {
  FLEXUS_COMPONENT_IMPL(Cache);

private:

  std::auto_ptr<CacheController> theController; //Deleted automagically when the cache goes away

  Stat::StatCounter theBusDeadlocks;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(Cache)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBusDeadlocks( statName() + "-BusDeadlocks" )
    , theBusDirection(kIdle)
  {}

  enum eDirection {
    kIdle
    , kToBackSideIn_Request
    , kToBackSideIn_Reply
    , kToBackSideOut_Request
    , kToBackSideOut_Prefetch
    , kToBackSideOut_Snoop
    , kToBackSideOut_Reply
  };

  uint32_t theBusTxCountdown;
  eDirection theBusDirection;
  MemoryTransport theBusContents;
  boost::optional< MemoryTransport> theBackSideIn_RequestBuffer;
  boost::optional< MemoryTransport> theBackSideIn_ReplyBuffer;
  std::list<MemoryTransport> theBackSideIn_ReplyInfiniteQueue;
  std::list<MemoryTransport> theBackSideIn_RequestInfiniteQueue;

  bool isQuiesced() const {
    return theBusDirection == kIdle
           && !theBackSideIn_ReplyBuffer
           && !theBackSideIn_RequestBuffer
           && (theController.get() ? theController->isQuiesced() : true )
           ;
  }

  void saveState(std::string const & aDirName) {
    theController->saveState( aDirName );
  }

  void loadState(std::string const & aDirName) {
    theController->loadState( aDirName, cfg.TextFlexpoints, cfg.GZipFlexpoints );
  }

  // Initialization
  void initialize() {
    theBusTxCountdown = 0;
    theBusDirection = kIdle;

    theController.reset(new CacheController(statName(),
                                            cfg.Cores,
                                            cfg.ArrayConfiguration,
                                            cfg.BlockSize,
                                            cfg.Banks,
                                            cfg.Ports,
                                            cfg.TagLatency,
                                            cfg.TagIssueLatency,
                                            cfg.DataLatency,
                                            cfg.DataIssueLatency,
                                            (int)flexusIndex(),
                                            cfg.CacheLevel,
                                            cfg.QueueSizes,
                                            cfg.PreQueueSizes,
                                            cfg.MAFSize,
                                            cfg.MAFTargetsPerRequest,
                                            cfg.EvictBufferSize,
                                            cfg.SnoopBufferSize,
                                            cfg.ProbeFetchMiss,
                                            cfg.EvictClean,
                                            cfg.EvictWritableHasData,
                                            cfg.CacheType,
                                            cfg.TraceAddress,
                                            false, /* cfg.AllowOffChipStreamFetch */
                                            cfg.EvictOnSnoop,
                                            cfg.UseReplyChannel
                                           ));

    DBG_Assert( cfg.BusTime_Data > 0);
    DBG_Assert( cfg.BusTime_NoData > 0);
  }

  void finalize() {}

  // Ports
  //======

  //FrontSideIn_Snoop
  //-----------------
  bool available( interface::FrontSideIn_Snoop const &,
                  index_t anIndex ) {
    return ! theController->FrontSideIn_Snoop[0].full();
  }
  void push( interface::FrontSideIn_Snoop const &,
             index_t           anIndex,
             MemoryTransport & aMessage ) {
    DBG_Assert(! theController->FrontSideIn_Snoop[0].full());
    aMessage[MemoryMessageTag]->coreIdx() = anIndex;
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Snoop) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Snoop[0].enqueue(aMessage);
  }

  //FrontSideIn_Prefetch
  //-------------------
  bool available( interface::FrontSideIn_Prefetch const &,
                  index_t anIndex) {
    return ! theController->FrontSideIn_Prefetch[0].full();
  }
  void push( interface::FrontSideIn_Prefetch const &,
             index_t           anIndex,
             MemoryTransport & aMessage) {
    DBG_Assert(! theController->FrontSideIn_Prefetch[0].full(), ( << statName() ) );
    aMessage[MemoryMessageTag]->coreIdx() = anIndex;
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Prefetch) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Prefetch[0].enqueue(aMessage);
  }

  //FrontSideIn_Request
  //-------------------
  bool available( interface::FrontSideIn_Request const &,
                  index_t anIndex) {
    return ! theController->FrontSideIn_Request[0].full();
  }
  void push( interface::FrontSideIn_Request const &,
             index_t           anIndex,
             MemoryTransport & aMessage) {
    DBG_Assert(! theController->FrontSideIn_Request[0].full(), ( << statName() ) );
    aMessage[MemoryMessageTag]->coreIdx() = anIndex;
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Request) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Request[0].enqueue(aMessage);
  }

  //BackeSideIn_Reply
  //-----------
  bool available( interface::BackSideIn_Reply const &) {
    return ! theBackSideIn_ReplyBuffer;
  }
  void push( interface::BackSideIn_Reply const &, MemoryTransport & aMessage) {
    if (cfg.NoBus) {
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideIn_ReplyInfiniteQueue.push_back(aMessage);
      return;
    }
    DBG_Assert(! theBackSideIn_ReplyBuffer);
    // In non-piranha caches, the core index should always be zero, since
    // things above (such as the processor) do not want to know about core numbers
    aMessage[MemoryMessageTag]->coreIdx() = 0;
    theBackSideIn_ReplyBuffer = aMessage;
  }

  //BackeSideIn_Request
  //-----------
  bool available( interface::BackSideIn_Request const &) {
    return ! theBackSideIn_RequestBuffer;
  }
  void push( interface::BackSideIn_Request const &, MemoryTransport & aMessage) {
    if (cfg.NoBus) {
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideIn_RequestInfiniteQueue.push_back(aMessage);
      return;
    }
    DBG_Assert(! theBackSideIn_RequestBuffer);
    // In non-piranha caches, the core index should always be zero, since
    // things above (such as the processor) do not want to know about core numbers
    aMessage[MemoryMessageTag]->coreIdx() = 0;
    theBackSideIn_RequestBuffer = aMessage;
  }

  //CacheDrive
  //----------
  void drive(interface::CacheDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "CacheDrive" ) ) ;
    theController->processMessages();
    busCycle();
  }

  uint32_t transferTime(const MemoryTransport & trans) {
    if (theFlexus->isFastMode()) {
      return 0;
    }
    DBG_Assert(trans[MemoryMessageTag] != NULL);
    return ( (trans[MemoryMessageTag]->reqSize() > 0) ? cfg.BusTime_Data : cfg.BusTime_NoData) - 1;
  }

  void busCycle() {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "bus cycle" ) );

    for ( int32_t i = 0; i < cfg.Cores; i++ ) {
      //Send as much on FrontSideOut as possible
      while (! theController->FrontSideOut_D[i].empty() && FLEXUS_CHANNEL_ARRAY(FrontSideOut_D, i).available()) {
        MemoryTransport transport = theController->FrontSideOut_D[i].dequeue();
        DBG_(Trace, Comp(*this) ( << "Sent on Port FrontSideOut_D [" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL_ARRAY(FrontSideOut_D, i) << transport;
      }
      while (! theController->FrontSideOut_I[i].empty() && FLEXUS_CHANNEL_ARRAY(FrontSideOut_I, i).available()) {
        MemoryTransport transport = theController->FrontSideOut_I[i].dequeue();
        DBG_(Trace, Comp(*this) ( << "Sent on Port FrontSideOut_I [" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL_ARRAY(FrontSideOut_I, i) << transport;
      }
    }

    if (cfg.NoBus) {
      while ( !theController->BackSideIn_Reply[0].full() && !theBackSideIn_ReplyInfiniteQueue.empty() ) {
        theController->BackSideIn_Reply[0].enqueue( theBackSideIn_ReplyInfiniteQueue.front() );
        theBackSideIn_ReplyInfiniteQueue.pop_front();
      }
      while ( !theController->BackSideIn_Request[0].full() && !theBackSideIn_RequestInfiniteQueue.empty() ) {
        theController->BackSideIn_Request[0].enqueue( theBackSideIn_RequestInfiniteQueue.front() );
        theBackSideIn_RequestInfiniteQueue.pop_front();
      }
      while ( !theController->BackSideOut_Request.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available()
              && (theController->BackSideOut_Snoop.empty()
                  || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Request.headTimestamp())
              && (theController->BackSideOut_Reply.empty()
                  || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Request.headTimestamp()) ) {
        MemoryTransport transport = theController->BackSideOut_Request.dequeue();
        transport[MemoryMessageTag]->coreIdx()=flexusIndex();
        FLEXUS_CHANNEL(BackSideOut_Request) << transport;
      }
      while ( !theController->BackSideOut_Prefetch.empty() && FLEXUS_CHANNEL(BackSideOut_Prefetch).available()
              && (theController->BackSideOut_Snoop.empty()
                  || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp())
              && (theController->BackSideOut_Reply.empty()
                  || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp()) ) {
        MemoryTransport transport = theController->BackSideOut_Prefetch.dequeue();
        FLEXUS_CHANNEL(BackSideOut_Prefetch) << transport;
      }
      while ( !theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available()
              && (theController->BackSideOut_Reply.empty()
                  || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Snoop.headTimestamp()) ) {
        MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
        transport[MemoryMessageTag]->coreIdx()=flexusIndex();
        FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
      }
      while ( !theController->BackSideOut_Reply.empty() && FLEXUS_CHANNEL(BackSideOut_Reply).available() ) {
        MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
        transport[MemoryMessageTag]->coreIdx()=flexusIndex();
        FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
      }
      return;
    }

    //If we want fast clean evicts, propagate as many CleanEvicts as possible
    //regardless of bus state.  We use this hack with SimplePrefetchController
    //because it uses clean evict messages to maintain duplicate tags, but the
    //CleanEvicts would not be propagated over the bus in the real system
    if (cfg.FastEvictClean) {
      if (theBusDirection == kIdle) {
        if ( !theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available()) {
          MemoryMessage::MemoryMessageType type = theController->BackSideOut_Snoop.peek()[MemoryMessageTag]->type();
          if (type == MemoryMessage::EvictClean || type == MemoryMessage::EvictWritable ) {
            MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
            transport[MemoryMessageTag]->coreIdx()=flexusIndex();
            DBG_(Trace, Comp(*this) ( << "Fast Transmit evict on port BackSideOut(Snoop): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
          }
        }
      }
    }

    //If a bus transfer is in process, continue it
    if (theBusTxCountdown > 0) {
      DBG_Assert( theBusDirection != kIdle );
      --theBusTxCountdown;
    } else if (theBusDirection == kIdle) {
      //The bus is available, initiate a transfer, if there are any to start

      if (  //Back side buffer gets max priority if it can transfer
        theBackSideIn_ReplyBuffer
        && !theController->BackSideIn_Reply[0].full()
      ) {
        theBusContents = *theBackSideIn_ReplyBuffer;
        theBackSideIn_ReplyBuffer = boost::none;
        theBusDirection = kToBackSideIn_Reply;
        theBusTxCountdown = transferTime( theBusContents );
      } else if (  //Back side buffer gets max priority if it can transfer
        theBackSideIn_RequestBuffer
        && !theController->BackSideIn_Request[0].full()
      ) {
        theBusContents = *theBackSideIn_RequestBuffer;
        theBackSideIn_RequestBuffer = boost::none;
        theBusDirection = kToBackSideIn_Request;
        theBusTxCountdown = transferTime( theBusContents );
      } else if ( //Prefer a request message if it is older than any snoop messages
        !theController->BackSideOut_Request.empty()
        && FLEXUS_CHANNEL(BackSideOut_Request).available()
        && (theController->BackSideOut_Snoop.empty() || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Request.headTimestamp() )
        && (theController->BackSideOut_Reply.empty() || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Request.headTimestamp() )
      ) {
        theBusDirection = kToBackSideOut_Request;
        theBusTxCountdown = transferTime( theController->BackSideOut_Request.peek() );
      } else if ( //Prefer a prefetch message if it is older than any snoop messages and there are no requests
        theController->BackSideOut_Request.empty()
        && !theController->BackSideOut_Prefetch.empty()
        && FLEXUS_CHANNEL(BackSideOut_Prefetch).available()
        && (theController->BackSideOut_Snoop.empty() || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp() )
        && (theController->BackSideOut_Reply.empty() || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp() )
      ) {
        theBusDirection = kToBackSideOut_Prefetch;
        theBusTxCountdown = transferTime( theController->BackSideOut_Prefetch.peek() );
      } else if ( //Take any snoop message
        !theController->BackSideOut_Snoop.empty()
        && FLEXUS_CHANNEL(BackSideOut_Snoop).available()
        && (theController->BackSideOut_Reply.empty() || theController->BackSideOut_Reply.headTimestamp() > theController->BackSideOut_Snoop.headTimestamp() )
      ) {
        theBusDirection = kToBackSideOut_Snoop;
        theBusTxCountdown = transferTime( theController->BackSideOut_Snoop.peek() );
      } else if ( //Take any snoop message
        !theController->BackSideOut_Reply.empty()
        && FLEXUS_CHANNEL(BackSideOut_Reply).available()
      ) {
        theBusDirection = kToBackSideOut_Reply;
        theBusTxCountdown = transferTime( theController->BackSideOut_Reply.peek() );
      }
    }

    if ( theBusTxCountdown == 0 ) {
      switch (theBusDirection) {
        case kIdle:
          break; //Nothing to do
        case kToBackSideIn_Reply:
          if ( ! theController->BackSideIn_Reply[0].full()) {
            DBG_(Trace, Comp(*this) ( << "Received on Port BackSideIn_Reply: " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            theController->BackSideIn_Reply[0].enqueue( theBusContents );
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideIn_Request:
          if ( ! theController->BackSideIn_Request[0].full()) {
            DBG_(Trace, Comp(*this) ( << "Received on Port BackSideIn_Request: " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            theController->BackSideIn_Request[0].enqueue( theBusContents );
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Snoop:
          if ( FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
            DBG_Assert( !theController->BackSideOut_Snoop.empty() );
            theBusContents = theController->BackSideOut_Snoop.dequeue();
            theBusContents[MemoryMessageTag]->coreIdx()=flexusIndex();
            DBG_(Trace, Comp(*this) ( << "Sent on Port BackSideOut(Snoop): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Snoop) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Reply:
          if ( FLEXUS_CHANNEL(BackSideOut_Reply).available() ) {
            DBG_Assert( !theController->BackSideOut_Reply.empty() );
            theBusContents = theController->BackSideOut_Reply.dequeue();
            theBusContents[MemoryMessageTag]->coreIdx()=flexusIndex();
            DBG_(Trace, Comp(*this) ( << "Sent on Port BackSideOut(Reply): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Reply) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Request:
          if ( FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
            DBG_Assert( !theController->BackSideOut_Request.empty() );
            theBusContents = theController->BackSideOut_Request.dequeue();
            theBusContents[MemoryMessageTag]->coreIdx()=flexusIndex();
            DBG_(Trace, Comp(*this) ( << "Sent on Port BackSideOut(Request): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Request) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Prefetch:
          if ( FLEXUS_CHANNEL(BackSideOut_Prefetch).available() ) {
            DBG_Assert( !theController->BackSideOut_Prefetch.empty() );
            theBusContents = theController->BackSideOut_Prefetch.dequeue();
            DBG_(Trace, Comp(*this) ( << "Sent on Port BackSideOut(Prefetch): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Prefetch) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
      }
    }

  }

};

} //End Namespace nCache

FLEXUS_COMPONENT_INSTANTIATOR( Cache, nCache );

FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideOut_D )           {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideOut_I )           {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Snoop )      {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Request )    {
  return (cfg.Cores);
}
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Prefetch )   {
  return (cfg.Cores);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Cache

#define DBG_Reset
#include DBG_Control()
