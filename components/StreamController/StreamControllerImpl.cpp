#include <components/StreamController/StreamController.hpp>

#define FLEXUS_BEGIN_COMPONENT StreamController
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <core/stats.hpp>

#include <components/Common/MessageQueues.hpp>

#define DBG_DefineCategories StreamController
#define DBG_SetDefaultOps AddCat(StreamController)
#include DBG_Control()

namespace nStreamController {

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace SharedTypes = Flexus::SharedTypes;

using namespace nMessageQueues;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct StreamQueue {
  int64_t theStreamID;
  std::list<MemoryAddress> thePrefetchList;
  int32_t theSVBBlocks;
  StreamQueue()
    : theStreamID(0)
    , theSVBBlocks(0)
  {}
  StreamQueue(int64_t anId, std::list<MemoryAddress> & aList)
    : theStreamID(anId)
    , thePrefetchList(aList) //Must copy
    , theSVBBlocks(0) {
  }
};

class FLEXUS_COMPONENT(StreamController)  {
  FLEXUS_COMPONENT_IMPL(StreamController);

  MessageQueue< PrefetchTransport > qSVBIn;
  MessageQueue< PrefetchTransport > qSVBOut;
  MessageQueue< PrefetchTransport > qPredictorIn;

  Stat::StatCounter statPrefetchesIssued;
  Stat::StatCounter statPrefetches_Completed;

  Stat::StatCounter statPrefetches_Hits;
  Stat::StatCounter statPrefetches_Redundant;
  Stat::StatCounter statPrefetches_Replaced;
  Stat::StatCounter statPrefetches_Removed;

  Stat::StatCounter statOrphans;

  int64_t theNextStreamID;
  typedef std::map < long, StreamQueue > streams_t;
  streams_t theStreamQueues;
  std::list<long> theReadyStreams;

private:
  //Helper functions
  void enqueue( MessageQueue< PrefetchTransport> & aQueue, PrefetchTransport const & aTransport) {
    DBG_Assert( ! isFull( aQueue) );
    aQueue.enqueue( aTransport );
  }

  template <class Queue>
  bool isFull( Queue & aQueue) {
    return aQueue.full();
  }

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(StreamController)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , statPrefetchesIssued( statName() + "-IssuedPrefetches" )
    , statPrefetches_Completed( statName() + "-Prefetches:Completed" )
    , statPrefetches_Hits( statName() + "-Prefetches:Hits" )
    , statPrefetches_Redundant( statName() + "-Prefetches:Redundant" )
    , statPrefetches_Replaced( statName() + "-Prefetches:Replaced" )
    , statPrefetches_Removed( statName() + "-Prefetches:Removed" )
    , statOrphans( statName() + "-Orphans" )
    , theNextStreamID(0)
  {}

  bool isQuiesced() const {
    return    qSVBOut.empty()
              &&      qSVBIn.empty()
              &&      qPredictorIn.empty()
              ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) { }

  // Initialization
  void initialize() {
    qSVBIn.setSize( cfg.MessageQueueSizes);
    qSVBOut.setSize( cfg.MessageQueueSizes);
    qPredictorIn.setSize( cfg.MessageQueueSizes);
  }

  // Ports
  bool available(interface::SVBIn const &) {
    return ! isFull( qSVBIn );
  }
  void push(interface::SVBIn const &,  PrefetchTransport & aMessage) {
    DBG_Assert( aMessage[PrefetchMessageTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Port SVBIn: " << *(aMessage[PrefetchMessageTag]) ) Addr(aMessage[PrefetchMessageTag]->address()) );
    enqueue( qSVBIn, aMessage );
  }

  bool available(interface::PredictorIn const &) {
    return ! isFull( qPredictorIn );
  }
  void push(interface::PredictorIn const &,  PrefetchTransport & aMessage) {
    DBG_Assert( aMessage[PrefetchCommandTag] );
    DBG_(Iface, Comp(*this) ( << "Received on Port PredictorIn: " << *(aMessage[PrefetchCommandTag]) ) );
    enqueue( qPredictorIn, aMessage );
  }

  // Drive Interfaces
  void drive(interface::StreamDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "StreamDrive" ) ) ;
    process();
    transmit();
  }

public:
  void sendSVB( boost::intrusive_ptr<PrefetchMessage> & aMessage) {
    DBG_Assert( ! qSVBOut.full() );
    PrefetchTransport trans;
    trans.set(PrefetchMessageTag, aMessage);
    qSVBOut.enqueue( trans );
  }

private:

  void process() {
    doSVBIn();
    doPredictorIn();
    doPrefetches();
    transmit();
  }

  void doSVBIn() {
    while ( !qSVBIn.empty() ) {
      PrefetchTransport trans( qSVBIn.dequeue() );
      processSVBMessage(trans[PrefetchMessageTag]);
    }
  }

  void doPredictorIn() {
    while ( !qPredictorIn.empty() ) {
      PrefetchTransport trans( qPredictorIn.dequeue() );
      processPredictorMessage(trans[PrefetchCommandTag]);
    }
  }

  void doPrefetches() {
    while ( ! theReadyStreams.empty() && !qSVBOut.full() ) {
      int64_t qid = theReadyStreams.front();
      theReadyStreams.pop_front();
      processPrefetch(qid);
    }
  }

  void transmit() {
    //Transmit will send as many messages as possible
    while (!qSVBOut.empty() && FLEXUS_CHANNEL(SVBOut).available() ) {
      PrefetchTransport transport = qSVBOut.dequeue();
      DBG_(Iface, Comp(*this) ( << "Sent on Port SVBOut: " << *(transport[PrefetchMessageTag]) ) Addr(transport[PrefetchMessageTag]->address()) );
      FLEXUS_CHANNEL(SVBOut) << transport;
    }

  }

  //Processing code
  void processSVBMessage( boost::intrusive_ptr<PrefetchMessage> aMessage) {
    DBG_Assert(aMessage);
    DBG_Assert( aMessage->streamID() != 0);
    DBG_(Trace, Comp(*this) ( << "Received(SVB): " << *aMessage) );
    switch (aMessage->type() ) {
      case PrefetchMessage::PrefetchComplete:
        //Do nothing
        ++statPrefetches_Completed;
        break;
      case PrefetchMessage::LineHit: {
        ++statPrefetches_Hits;
        streams_t::iterator iter = theStreamQueues.find(aMessage->streamID());
        if (iter != theStreamQueues.end()) {
          --(iter->second.theSVBBlocks);
          DBG_Assert(iter->second.theSVBBlocks >= 0);
        }
        theReadyStreams.push_back(aMessage->streamID());
        break;
      }
      case PrefetchMessage::PrefetchRedundant: {
        ++statPrefetches_Redundant;
        streams_t::iterator iter = theStreamQueues.find(aMessage->streamID());
        if (iter != theStreamQueues.end()) {
          --(iter->second.theSVBBlocks);
          DBG_Assert(iter->second.theSVBBlocks >= 0);
        }
        theReadyStreams.push_back(aMessage->streamID());
        break;
      }
      case PrefetchMessage::LineReplaced: {
        ++statPrefetches_Replaced;
        streams_t::iterator iter = theStreamQueues.find(aMessage->streamID());
        if (iter != theStreamQueues.end()) {
          --(iter->second.theSVBBlocks);
          DBG_Assert(iter->second.theSVBBlocks >= 0);
        }
        if (iter->second.theSVBBlocks == 0) {
          //Kill orphaned stream
          statOrphans += iter->second.thePrefetchList.size();
          theStreamQueues.erase(aMessage->streamID());
        }
        break;
      }
      case PrefetchMessage::LineRemoved: {
        ++statPrefetches_Removed;
        streams_t::iterator iter = theStreamQueues.find(aMessage->streamID());
        if (iter != theStreamQueues.end()) {
          --(iter->second.theSVBBlocks);
          DBG_Assert(iter->second.theSVBBlocks >= 0);
        }
        if (iter->second.theSVBBlocks == 0) {
          //Kill orphaned stream
          statOrphans += iter->second.thePrefetchList.size();
          theStreamQueues.erase(aMessage->streamID());
        }
        break;
      }
      default:
        DBG_Assert(false, ( << *aMessage) );
    }
  }

  void processPredictorMessage( boost::intrusive_ptr<PrefetchCommand> aMessage) {
    DBG_Assert(aMessage);
    DBG_Assert(aMessage->type() == PrefetchCommand::ePrefetchAddressList);
    DBG_(Trace, Comp(*this) ( << "Received(Predictor): " << *aMessage) );
    ++theNextStreamID;
    theStreamQueues[theNextStreamID] = StreamQueue(theNextStreamID, aMessage->addressList());
    theReadyStreams.push_back(theNextStreamID);
    DBG_Assert(theStreamQueues.size() < 65); //To catch bugs for now
  }

  void processPrefetch(int64_t aQID) {
    streams_t::iterator iter = theStreamQueues.find(aQID);
    if (iter != theStreamQueues.end()) {
      StreamQueue & q = iter->second;
      while (!qSVBOut.full() && !q.thePrefetchList.empty() && (q.theSVBBlocks < static_cast<long>(cfg.FetchTarget)) ) {
        boost::intrusive_ptr<PrefetchMessage> msg(new PrefetchMessage( PrefetchMessage::PrefetchReq, q.thePrefetchList.front() ) );
        msg->streamID() = aQID;
        q.thePrefetchList.pop_front();
        DBG_(Trace, Comp(*this) ( << "Prefetching: " << *msg) );
        sendSVB(msg);
        ++statPrefetchesIssued;
        ++q.theSVBBlocks;
      }
      if (q.thePrefetchList.empty()) {
        DBG_(Trace, Comp(*this) ( << "Deleting stream queue: " << aQID) );
        theStreamQueues.erase(iter);
      } else if (q.theSVBBlocks < static_cast<long>(cfg.FetchTarget) ) {
        theReadyStreams.push_back(aQID);
      }
    }
  }

};

} //End Namespace nStreamController

FLEXUS_COMPONENT_INSTANTIATOR( StreamController, nStreamController);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT StreamController

#define DBG_Reset
#include DBG_Control()

