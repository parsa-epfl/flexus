#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/debug/debug.hpp>

#include "ProtSharedTypes.hpp"
#include "tSrvcProvider.hpp"
#include "input_q_cntl.hpp"
#include "util.hpp"
#include "protocol_engine.hpp"
#include "thread_scheduler.hpp"

#define DBG_DefineCategories InputQueueControl
#define DBG_DeclareCategories ProtocolEngine
#define DBG_SetDefaultOps AddCat(ProtocolEngine) AddCat(InputQueueControl)
#include DBG_Control()

namespace nProtocolEngine {

using namespace Protocol;

void tInputQueueController::dequeue(const tVC VC, tThread aThread) {

  DBG_(Iface, ( << theEngineName
                << " InQ " << ((unsigned) VC)
                << " dequeue: " << *getQueueHead(VC) ));

  int64_t delay = theSrvcProvider.getCycleCount() - getQueueHead(VC)->theInputQTimestamp;
  statAvgInputQWaitTime << delay;
  statInputQWaitTimeDistribution << delay;

  theQueues[VC].theQueue.pop_front();
}

tInputQueueController::tInputQueueController(std::string const & engine_name, tSrvcProvider & aSrvcProvider, tThreadScheduler & aThreadScheduler)
  : theEngineName(engine_name)
  , theSrvcProvider(aSrvcProvider)
  , theThreadScheduler(aThreadScheduler)
  , statAvgInputQWaitTime(engine_name + "-InputQ-AvgWaitTime")
  , statInputQWaitTimeDistribution(engine_name + "-InputQ-WaitTimeDistribution")
{ }

void tInputQueueController::enqueue(boost::intrusive_ptr<tPacket> packet) {
  if (packet->isLocal()) {
    DBG_(Iface, ( << theEngineName
                  << " InQ " << ((unsigned) packet->VC())
                  << " local enqueue addr=0x" << &std::hex << ((uint64_t)  packet->address()) << &std::dec
                  << " " << packet->type()
                  << " from local node"
                ));
  } else {
    DBG_(Iface, ( << theEngineName
                  << " InQ " << ((unsigned) packet->VC())
                  << " remote enqueue addr=0x" << &std::hex << ((uint64_t)  packet->address()) << &std::dec
                  << " " << packet->type()
                  << " from node " << packet->source()
                  << " req=" << packet->requester()
                  << " inv=" << packet->invalidationCount()
                  << " homeShr=" << packet->anyInvalidations()
                ));
  }

  switch ( packet->type() ) {
    case InvAck:
    case InvUpdateAck:
      handleInvalidate(packet);
      break;
    case DowngradeAck:
    case DowngradeUpdateAck:
      handleDowngrade(packet);
      break;
    default:
      //No special action required
      break;
  }

  packet->theInputQTimestamp = theSrvcProvider.getCycleCount();

  if (packet->transactionTracker() && packet->transactionTracker()->inPE() && *packet->transactionTracker()->inPE()) {
    packet->transactionTracker()->setDelayCause(theEngineName, "Input Q");
  }

  theQueues[packet->VC()].theQueue.push_back(packet);
}

void tInputQueueController::refuse(tVC aVC, boost::intrusive_ptr<tPacket> aPacket) {
  DBG_(Iface, ( << theEngineName
                << " ToQ " << ((unsigned) aVC)
                << " refuse: " << *aPacket));

  DBG_Assert(aPacket);
  DBG_Assert(aPacket->VC() == aVC);
  DBG_Assert(aVC >= 0);
  DBG_Assert(aVC <= kMaxVC);

  theQueues[aVC].theQueue.push_front(aPacket);
  aPacket->theInputQTimestamp = theSrvcProvider.getCycleCount();

  if (aPacket->transactionTracker() && aPacket->transactionTracker()->inPE() && *aPacket->transactionTracker()->inPE()) {
    aPacket->transactionTracker()->setDelayCause(theEngineName, "Input Q");
  }

}

void tInputQueueController::refuseAndRotate(tVC aVC, boost::intrusive_ptr<tPacket> aPacket) {
  DBG_(Iface, ( << theEngineName
                << " ToQ " << ((unsigned) aVC)
                << " refuse-and-rotate: " << *aPacket));

  DBG_Assert(aPacket->VC() == aVC);

  theQueues[aVC].theQueue.push_back(aPacket);
  aPacket->theInputQTimestamp = theSrvcProvider.getCycleCount();

  if (aPacket->transactionTracker() && aPacket->transactionTracker()->inPE() && *aPacket->transactionTracker()->inPE()) {
    aPacket->transactionTracker()->setDelayCause(theEngineName, "Input Q");
  }
}

bool tInputQueueController::available(tVC aVC) const {
  return ! theQueues[aVC].theQueue.empty();
}

void tInputQueueController::handleInvalidate(boost::intrusive_ptr<tPacket> aPacket) {

  DBG_(Iface, ( << theEngineName << " Scanning queue because of Invalidate message."));

  tInputQueue::iterator iter = theQueues[LocalVCLow].theQueue.begin();
  tInputQueue::iterator end = theQueues[LocalVCLow].theQueue.end();

  while (iter != end) {
    if ( (*iter) -> address() == aPacket->address() ) {
      switch ( (*iter) -> type() ) {
        case LocalUpgradeAccess:
          DBG_(Iface, ( << theEngineName << " Converting " << (*iter) -> type() << " for " << (*iter) -> address() << " to LocalWriteAccess because of Invalidate"));
          (*iter) -> setType(LocalWriteAccess);
          ++iter;
          break;
        case LocalFlush:
        case LocalEvict:
          DBG_(Iface, ( << theEngineName << " Removing message " << (*iter) -> type()  << " for " << (*iter) -> address()  << " because of Invalidate"));
          iter = theQueues[LocalVCLow].theQueue.erase(iter);
          aPacket->setType(InvUpdateAck); //If it was't already an InvUpdateAck, make it so
          if (aPacket->transactionTracker()) {
            aPacket->transactionTracker()->setPreviousState(eModified);
          }
          break;
        default:
          //Do nothing and return false
          ++iter;
          break;
      }
    } else {
      ++iter;
    }
  }
  if (theThreadScheduler.handleInvalidate(aPacket->address())) {
    aPacket->setType(InvUpdateAck); //If it was't already an DowngradeUpdateAck, make it so
    if (aPacket->transactionTracker()) {
      aPacket->transactionTracker()->setPreviousState(eModified);
    }
  }
}

void tInputQueueController::handleDowngrade(boost::intrusive_ptr<tPacket> aPacket) {
  // A downgrade comes up. Look at the queue heads and do the right thing.
  // Returns true if the input queues contained a flush or evict (with data).
  DBG_(Iface, ( << theEngineName << " Scanning queue because of Downgrade message."));

  tInputQueue::iterator iter = theQueues[LocalVCLow].theQueue.begin();
  tInputQueue::iterator end = theQueues[LocalVCLow].theQueue.end();

  while (iter != end) {
    if ( (*iter) -> address() == aPacket->address() ) {
      switch ( (*iter) -> type() ) {
        case LocalFlush:
        case LocalEvict:
          DBG_(Iface, ( << theEngineName << " Removing message " << (*iter) -> type()  << " for " << &std::hex << (*iter) -> address()  << &std::dec << " because of Downgrade"));
          iter = theQueues[LocalVCLow].theQueue.erase(iter);
          aPacket->setType(DowngradeUpdateAck); //If it was't already an DowngradeUpdateAck, make it so
          if (aPacket->transactionTracker()) {
            aPacket->transactionTracker()->setPreviousState(eModified);
          }
          break;
        default:
          //Do nothing and return false
          ++iter;
          break;
      }
    } else {
      ++iter;
    }
  }
  if (theThreadScheduler.handleDowngrade(aPacket->address())) {
    aPacket->setType(DowngradeUpdateAck); //If it was't already an DowngradeUpdateAck, make it so
    if (aPacket->transactionTracker()) {
      aPacket->transactionTracker()->setPreviousState(eModified);
    }
  }
}

}  // namespace nProtocolEngine
