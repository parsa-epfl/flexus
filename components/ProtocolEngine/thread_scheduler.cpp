#include <algorithm>
#include <boost/none.hpp>

#include "thread_scheduler.hpp"
#include "input_q_cntl.hpp"
#include "tSrvcProvider.hpp"
#include "exec_engine.hpp"

#define DBG_DefineCategories ThreadScheduler
#define DBG_DeclareCategories ProtocolEngine
#define DBG_SetDefaultOps AddCat(ProtocolEngine) AddCat(ThreadScheduler)
#include DBG_Control()

namespace nProtocolEngine {

using namespace Protocol;

tThreadScheduler::tThreadScheduler(std::string const & anEngineName, tTsrf & aTsrf, tInputQueueController & anInputQCntl, tSrvcProvider & aSrvcProv, tMicrocodeEmulator & anExecEngine)
  : theEngineName(anEngineName)
  , theTsrf(aTsrf)
  , theInputQCntl(anInputQCntl)
  , theSrvcProv(aSrvcProv)
  , theExecEngine(anExecEngine)
  , theNextThreadID(1)
  , statAvgThreadLifetime(anEngineName + "-Sched-AvgThreadLifetime")
  , statThreadLifetimeDistribution(anEngineName + "-Sched-ThreadLifetimeDistribution")
  , statAvgThreadRuntime(anEngineName + "-Sched-AvgThreadRuntime")
  , statThreadRuntimeDistribution(anEngineName + "-Sched-ThreadRuntimeDistribution")
  , statLocalQueueAddressConflictStallCycles(anEngineName + "-Sched-Local-ConflictStallCycles")
  , statLocalQueueTSRFExhaustedStallCycles(anEngineName + "-Sched-Local-NoTSRFStallCycles")
  , statNetworkQueueAddressConflictStallCycles(anEngineName + "-Sched-Network-ConflictStallCycles")
  , statNetworkQueueTSRFExhaustedStallCycles(anEngineName + "-Sched-Network-NoTSRFStallCycles")
  , statThreadUops(anEngineName + "-Sched-ThreadUopsDistribution") {

}

void tThreadScheduler::stallQueue(tThread aThread, tVC aQueue) {
  DBG_Assert(! isQueueStalled(aQueue) );

  DBG_(Iface, ( << theEngineName << " Stalling VC[" << aQueue << "] on thread " << aThread ));

  theStalledQueues.insert(aQueue);
  theThreadToStalledQueueMap.insert( std::make_pair( aThread, aQueue) );

  aThread.setDelayCause(theEngineName, "Stall Queue");

}

bool tThreadScheduler::isQueueStalled(tVC aQueue) {
  return ( theStalledQueues.count(aQueue) > 0);
}

void tThreadScheduler::releaseAllQueues(tThread aThread) {
  tThreadToStalledQueueMap::iterator  first_match, iter, last_match;
  boost::tie(first_match, last_match) = theThreadToStalledQueueMap.equal_range(aThread);

  iter = first_match;
  while (iter != last_match) {
    DBG_(Iface, ( << theEngineName << " Releasing VC[" << iter->second << "] stalled on thread " << aThread ));
    theStalledQueues.erase(iter->second);
    ++iter;
  }
  theThreadToStalledQueueMap.erase(first_match, last_match);
}

void tThreadScheduler::deleteBlockedThread(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Removing blocked thread " << aThread ));

  tBlockedThreads::iterator iter = theBlockedThreads.find(aThread.address());
  DBG_Assert( iter != theBlockedThreads.end() );
  std::list<tThread>::iterator titer, ttemp, tend;
  titer = iter->second.begin();
  tend = iter->second.end();
  while (titer != tend) {
    ttemp = titer;
    ++titer;
    if (*ttemp == aThread) {
      iter->second.erase(ttemp);
    }
  }
  if (iter->second.empty()) {
    theBlockedThreads.erase(iter);
  }
}

void tThreadScheduler::unblockThreads(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Checking for threads blocked on " << & std::hex << aThread.address() << & std::dec ));

  tBlockedThreads::iterator iter = theBlockedThreads.find(aThread.address());
  if (iter != theBlockedThreads.end() ) {
    if ( iter->second.empty()) {
      DBG_(Crit, ( << theEngineName << " an empty list was found in theBlockedThreads.  Should not be possible, but I can fix it.") );
      theBlockedThreads.erase(iter);
      return;
    }

    //See if we already have a running/waiting/suspended thread
    tThreadMap::iterator first_thread;
    tThreadMap::iterator last_thread;

    boost::tie(first_thread, last_thread) = theLiveThreads.equal_range(aThread.address());
    while (first_thread != last_thread) {
      if (  first_thread->second.isState( eRunnable )
            || first_thread->second.isState( eWaiting )
            || first_thread->second.isState( eSuspendedForLock )
            || first_thread->second.isState( eSuspendedForDir )
         ) {
        DBG_(Iface, ( << theEngineName << " Found a schedulable thread. Not waking a blocked thread."));
        return; //Don't unblock a thread
      }
      ++first_thread;
    }

    tThread lucky_thread = iter->second.front();
    iter->second.pop_front();
    if (iter->second.empty()) {
      theBlockedThreads.erase(iter);
    }
    DBG_(Iface, ( << theEngineName << " Waking blocked thread " << lucky_thread));
    launchThread(lucky_thread);

  }
}

void tThreadScheduler::registerThread( tThread aThread ) {
  DBG_Assert( aThread.isValid() );
  DBG_(Iface, ( << theEngineName << " Registered new thread " << aThread ));
  theLiveThreads.insert( std::make_pair (aThread.address(), aThread) );
}

void tThreadScheduler::reclaimThread(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Reclaiming thread " << aThread ));

  tThreadMap::iterator first_match;
  tThreadMap::iterator last_match;

  boost::tie(first_match, last_match) = theLiveThreads.equal_range(aThread.address());

  while (first_match != last_match) {
    if (first_match->second == aThread) {
      break;
    }
    ++first_match;
  }
  DBG_Assert( first_match != last_match, ( << "Thread does not appear in theLiveThreads during a reclaim!"));

  if (aThread.creationTime() != 0) {
    statAvgThreadLifetime << theSrvcProv.getCycleCount() - aThread.creationTime();
    statThreadLifetimeDistribution << theSrvcProv.getCycleCount() - aThread.creationTime();
  }

  if (aThread.startTime() != 0) {
    statAvgThreadRuntime << theSrvcProv.getCycleCount() - aThread.startTime();
    statThreadRuntimeDistribution << theSrvcProv.getCycleCount() - aThread.startTime();
  }

  //Free all queues stalled on this thread
  releaseAllQueues(aThread);

  unblockThreads(aThread);

  //Remove the thread from the live thread list
  theLiveThreads.erase(first_match);

  //Release the TSRF entry
  tTsrfEntry * released_entry = aThread.vacate();
  theTsrf.free(released_entry);
}

bool tThreadScheduler::hasLiveThread(tAddress anAddress) {
  return theLiveThreads.count(anAddress) > 0;
}

boost::optional<tThread> tThreadScheduler::findUniqueThread(tThreadState aState, tMatchedThreads aMatchedThreadRange) {
  tThreadMap::iterator iter = aMatchedThreadRange.first;
  tThreadMap::iterator ret_val = aMatchedThreadRange.second;
  while (iter != aMatchedThreadRange.second) {
    if (iter->second.isState(aState)) {
      if (ret_val == aMatchedThreadRange.second) {
        ret_val = iter;
      } else {
        DBG_Assert( false, ( << "Two threads for the same address which are both " << aState << " found: " << ret_val->second << " " << iter->second) );
      }
    }
    ++ iter;
  }
  if (ret_val != aMatchedThreadRange.second) {
    return boost::optional<tThread>( ret_val->second );
  } else {
    return boost::optional<tThread>();
  }
}

bool tThreadScheduler::anyThreadsAre(tThreadState aState, tMatchedThreads aMatchedThreadRange) {
  tThreadMap::iterator iter = aMatchedThreadRange.first;
  while (iter != aMatchedThreadRange.second) {
    if (iter->second.isState(aState)) {
      return true;
    }
    ++ iter;
  }
  return false;
}

bool tThreadScheduler::requiresDirectoryLock(tPacket const & packet) {
  return ( theSrvcProv.isAddressLocal( packet.address() ) && (! packet.isLocal() || ! packet.dirEntry() ) );
}

void tThreadScheduler::start(tThread aThread, unsigned anEntryPoint) {
  DBG_(Iface, ( << theEngineName << " Starting thread " << aThread << " at entry point " << anEntryPoint));
  // get timestamp at thread creation for stats
  aThread.setProgramCounter(anEntryPoint);
  aThread.setStartTime(theSrvcProv.getCycleCount());
  aThread.setEntryPC(anEntryPoint);
  aThread.markRunnable();
  aThread.setDelayCause(theEngineName, "Wait to Run");
  reschedule(aThread);
}

void tThreadScheduler::requestDirectoryLock(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Requesting directory lock for " << aThread));
  theSrvcProv.LockOp(LOCK, aThread.address());
  aThread.waitForLock();
  aThread.setDelayCause(theEngineName, "Wait for Lock");
  reschedule(aThread);
}

void tThreadScheduler::requestDirectoryEntry(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Requesting directory read for " << aThread));
  theSrvcProv.MemOp(READ, DIRECTORY, aThread.address());
  aThread.waitForDirectory();
  aThread.setDelayCause(theEngineName, "Wait for Dir");
  reschedule(aThread);
}

tThread tThreadScheduler::createThread(boost::intrusive_ptr<tPacket> packet, tVC aSourceVC, tTsrfEntry * anEntry) {
  DBG_Assert( isRequest(packet->type()) );

  //Create a thread associated with the TSRF entry;
  tThread thread( anEntry, theNextThreadID++, theSrvcProv.getCycleCount());

  DBG_(Iface, ( << theEngineName << " Created " << thread << " with packet " << *packet));

  //Fill in
  //thread.setInvalidationCount( 0 );  //Not so sure about these two.  Lets leave them out for now
  //thread.setTempReg( 0 );
  thread.setPacket( packet );
  thread.setTransactionTracker( packet->transactionTracker() );
  thread.setVC( aSourceVC );
  thread.setAddress( packet->address() );
  thread.setType( packet->type() );
  thread.setPrefetch(packet->isPrefetch());

  if (packet->isLocal()) {
    //For local packets, requester and respondent are the local node
    thread.setRequester( theSrvcProv.myNodeId() );
    thread.setRespondent( theSrvcProv.myNodeId() );
  } else {
    //For remote packets, get requester and respondent from the packet
    thread.setRequester( packet->requester() );
    thread.setRespondent( packet->respondent() );
  }

  thread.setCPI(theSrvcProv.getCPI());
  thread.resetStallCycles();
  registerThread( thread );

  return thread;
}

tThread tThreadScheduler::createThread(tVC aVC, tTsrfEntry * anEntry) {
  DBG_(Iface, ( << theEngineName << " Creating a new thread with packet from VC[" << aVC << "]"));
  //Make sure the TSRF entry really is free
  DBG_Assert(anEntry->state() == eNoThread);

  //Make sure there really is a packet at the specified VC, and it is a request packet
  DBG_Assert( theInputQCntl.available(aVC) );

  boost::intrusive_ptr<tPacket> packet(theInputQCntl.getQueueHead(aVC));
  tThread thread( createThread( packet, aVC, anEntry ) );

  theInputQCntl.dequeue(aVC);

  return thread;
}

void tThreadScheduler::blockThread(tThread aThread, tThread aBlockOn) {
  DBG_(Iface, ( << theEngineName << " Blocking Thread " << aThread ));
  theBlockedThreads[aThread.address()].push_back(aThread);
  aThread.block(aBlockOn);
  aThread.setDelayCause(theEngineName, "Block on Conflict");
}

void tThreadScheduler::launchThread(tThread aThread) {

  DBG_(Iface, ( << theEngineName << " Launching " << aThread << " with packet " << *aThread.packet()));

  //See if the thread must acquire a directory lock
  if (requiresDirectoryLock(*aThread.packet())) {
    requestDirectoryLock(aThread);
  } else {
    //If there is no dir entry (which is the case for the remote engine), we
    //pass DIR_STATE_INVALID to getEntryPoint.
    tDirState state = DIR_STATE_INVALID;
    if ( aThread.packet()->isLocal() && theSrvcProv.isAddressLocal( aThread.address() ) ) {
      //Local packets that don't need a directory lock come with a directory
      //entry.
      DBG_Assert( aThread.packet()->dirEntry() );

      //Copy the directory entry and put it into the tsrf
      boost::intrusive_ptr<tDirEntry> dir_entry( new tDirEntry( *aThread.packet()->dirEntry() ) );
      aThread.setDirEntry( dir_entry );
      state = aThread.dirState();
    }

    unsigned pc = theExecEngine.getEntryPoint(aThread.type(), aThread, state);
    start(aThread, pc);
  }
}

void tThreadScheduler::refusePacket(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Refusing packet on from thread " << aThread));
  //Ensure the thread is valid
  DBG_Assert(aThread.isValid());

  //Is this packet a request packet?
  if ( isRequest( aThread.packet()->type() ) ) {

    //See if we can give the packet a TSRF, blocked on this thread.  This packet
    //is allowed to use the reserved-for-WB TSRF
    if (theTsrf.isEntryAvail(tTsrf::eRequestWBEntry)) {
      tTsrfEntry * entry = theTsrf.allocate(tTsrf::eRequestWBEntry);
      tThread thread = createThread(aThread.packet(), aThread.VC(), entry);
      blockThread(thread, aThread);
    } else {
      //No TSRF space - put the packet on the back of the input queue
      theInputQCntl.refuseAndRotate(aThread.VC(), aThread.packet());
    }
  } else {

    //The input Q controller will assert if this call is invalid for some reason
    theInputQCntl.refuse(aThread.VC(), aThread.packet());

    //Place the input Q on the list of queues blocked for this thread
    stallQueue(aThread, aThread.VC());

  }
}

void tThreadScheduler::deliverReply(tThread aThread, tVC aVC) {
  DBG_Assert( aThread.isWaiting() );
  DBG_Assert( theInputQCntl.available(aVC) );

  boost::intrusive_ptr<tPacket> packet = theInputQCntl.getQueueHead(aVC);

  DBG_(Iface, ( << theEngineName << " Delivering packet " << *packet << " to thread " << aThread));

  DBG_Assert(packet->address() == aThread.address());

  aThread.setPacket( packet );
  aThread.setVC(aVC);
  aThread.setType(packet->type());
  if (! packet->isLocal()) {
    aThread.setRespondent(packet->respondent());
    aThread.setRacer(packet->requester());
  }

  theExecEngine.deliverReply(aThread, packet->type());

  theInputQCntl.dequeue(aVC);

  aThread.markRunnable();
  aThread.setDelayCause(theEngineName, "Wait to Run");
  reschedule(aThread);
}

void tThreadScheduler::processMatchedPacket(tVC aVC) {
  //Ensure that the packet has not been delivered yet, or something is very wrong
  DBG_Assert( theInputQCntl.available(aVC) );

  boost::intrusive_ptr<tPacket> packet = theInputQCntl.getQueueHead(aVC);

  //Get the list of threads this packet matches
  tMatchedThreads matches = theLiveThreads.equal_range(packet->address());

  //There better be at least one, or we shouldn't have gotten here.
  DBG_Assert( matches.first != matches.second ) ;

  //See if there are any runnable threads in the list.  If there are, we
  //return and check again next time we process the queues
  if (anyThreadsAre(eRunnable, matches))
    return;

  //See if there is a thread waiting in a receive call
  boost::optional<tThread> waiting_thread = findUniqueThread(eWaiting, matches);

  if (isPotentialReply(packet->type()) ) {
    if (waiting_thread) {
      //Try to deliver the packet to the waiting thread.
      deliverReply(*waiting_thread, aVC);
    } else {
      //The packet must wait (and block the queue) until the existing thread
      //begins waiting or completes
    }
  } else {
    //The packet is a request.

    //There are 2 possibilities:
    //1) The packet arrives with a directory lock.
    //   The packet should create a new thread, and begin running.  The
    //   existing thread will continue to wait until the new thread completes
    //   or cancels it.
    //2) The packet arrives without a lock.  It must wait until the existing
    //   thread completes.  However, we move it to a new thread to keep it
    //   from blocking the input queue
    if (packet->isLocal() && packet->dirEntry()) {
      //Start a new thread with the packet
      //Treat the packet as if it hadn't matched,
      processUnmatchedPacket(aVC);
    } else {
      //Move the packet to a new TSRF entry (if possible).
      //It blocks on waiting_thread (if it exists), or any other random
      //thread
      tThread block_on = matches.first->second;
      if (waiting_thread) {
        block_on = *waiting_thread;
      }
      createBlockedThread(aVC, block_on);
    }
  }

}

void tThreadScheduler::processUnmatchedPacket(tVC aVC) {

  //Ensure that the packet has not been delivered yet, or something is very wrong
  DBG_Assert( theInputQCntl.available(aVC) );

  boost::intrusive_ptr<tPacket> packet = theInputQCntl.getQueueHead(aVC);

  DBG_(Iface, ( << theEngineName << " processUnmatchedPacket() VC[" << aVC << "] packet: " << *packet) );

  //This packet must be a new request, or something has gone horribly wrong.
  DBG_Assert( isRequest(packet->type()), ( << theEngineName << " processUnmatchedPacket() VC[" << aVC << "] packet: " << *packet));

  if (packet->transactionTracker() && packet->transactionTracker()->inPE() && *packet->transactionTracker()->inPE()) {
    packet->transactionTracker()->setDelayCause(theEngineName, "Request TSRF");
  }

  //Determine if the message is eligible for any reserved TSRF entries
  tTsrf::eEntryRequestType entry_type = tTsrf::eNormalRequest;
  if ( packet->isLocal() ) {
    entry_type = tTsrf::eRequestLocalEntry;
  } else if ( packet->type() == WritebackReq || packet->type() == FlushReq  ) {
    entry_type = tTsrf::eRequestWBEntry;
  }

  //Request a TSRF
  if (theTsrf.isEntryAvail(entry_type)) {
    tTsrfEntry * entry = theTsrf.allocate(entry_type);
    tThread thread = createThread(aVC, entry);
    launchThread(thread);
  }
}

void tThreadScheduler::createBlockedThread(tVC aVC, tThread aBlockOn) {

  //Ensure that the packet has not been delivered yet, or something is very wrong
  DBG_Assert( theInputQCntl.available(aVC) );

  boost::intrusive_ptr<tPacket> packet = theInputQCntl.getQueueHead(aVC);

  DBG_(Iface, ( << theEngineName << " createBlockedThread() VC[" << aVC << "] packet: " << *packet) );

  //This packet must be a new request, or something has gone horribly wrong.
  DBG_Assert( isRequest(packet->type()), ( << theEngineName << " createBlockedThread() VC[" << aVC << "] packet: " << *packet));

  //When creating blocked threads, reserved entries are not allowed
  //Request a TSRF
  if (theTsrf.isEntryAvail(tTsrf::eNormalRequest)) {
    tTsrfEntry * entry = theTsrf.allocate(tTsrf::eNormalRequest);
    tThread thread = createThread(aVC, entry);
    blockThread(thread, aBlockOn);
  }
}

void tThreadScheduler::processLockAcquired(tAddress anAddress) {
  //Get the list of threads this packet matches
  tMatchedThreads matches = theLiveThreads.equal_range(anAddress);

  //There better be at least one, or we got a lock acquire we didn't want.
  DBG_Assert( matches.first != matches.second ) ;

  //Find the thread that is eSuspendedForLock.  There should be only one
  boost::optional<tThread> suspended_thread = findUniqueThread(eSuspendedForLock, matches);

  //There better be one.
  DBG_Assert(suspended_thread);

  DBG_(Iface, ( << theEngineName << " Acquired lock for thread " << *suspended_thread));

  //Mark it suspended for directory
  requestDirectoryEntry(*suspended_thread);
}

void tThreadScheduler::processDirectoryReply(tAddress anAddress, boost::intrusive_ptr<tDirEntry const> aDirEntry) {
  //Get the list of threads this packet matches
  tMatchedThreads matches = theLiveThreads.equal_range(anAddress);

  //There better be at least one, or we got a lock acquire we didn't want.
  DBG_Assert( matches.first != matches.second ) ;

  //Find the thread that is eSuspendedForLock.  There should be only one
  boost::optional<tThread> suspended_thread = findUniqueThread(eSuspendedForDir, matches);

  //There better be one.
  DBG_Assert(suspended_thread);

  //Fill in the directory reply
  boost::intrusive_ptr<tDirEntry> dir_entry( new tDirEntry( *aDirEntry ) );
  suspended_thread->setDirEntry( dir_entry );

  DBG_(Iface, ( << theEngineName << " Received directory reply for " << *suspended_thread << " directory state: " << dir_entry->state()));

  if (suspended_thread->transactionTracker() && suspended_thread->transactionTracker()->initiator() ) {
    if (dir_entry->state() == DIR_STATE_SHARED) {
      suspended_thread->transactionTracker()->setResponder(theSrvcProv.myNodeId());
    } else if (dir_entry->state() == DIR_STATE_MODIFIED) {
      suspended_thread->transactionTracker()->setResponder(dir_entry->owner());
    }
  }

  unsigned pc = theExecEngine.getEntryPoint(suspended_thread->type(), *suspended_thread, suspended_thread->dirState());
  start(*suspended_thread, pc);
}

void tThreadScheduler::processQueues() {
  DBG_(VVerb, ( << theEngineName << " processQueues()") );
  //For each queue in VC order
  for ( tVC vc = kMaxVC; vc >= kMinVC && ! ready() ; --vc) {
    //if the queue is not empty
    if (theInputQCntl.available(vc) && ! isQueueStalled(vc)) {
      boost::intrusive_ptr<tPacket> packet = theInputQCntl.getQueueHead(vc);

      //See if the packet matches the address of an existing thread
      if ( hasLiveThread(packet->address()) ) {
        //Address match.  Must be a reply, or a conflict.
        processMatchedPacket(vc);
      } else {
        //No conflict.  Must be a new request
        processUnmatchedPacket(vc);
      }
    }
  }

  //Count the cycles in which the the lowest VCs are stalled and have packets available
  if ( theInputQCntl.available(LocalVC0) && isQueueStalled(LocalVC0) ) {
    ++statLocalQueueAddressConflictStallCycles;
  } else if ( theInputQCntl.available(LocalVC0) && !theTsrf.isEntryAvail(tTsrf::eNormalRequest ) ) {
    ++statLocalQueueTSRFExhaustedStallCycles;
  }

  if ( theInputQCntl.available(VC0) && isQueueStalled(VC0) ) {
    ++statNetworkQueueAddressConflictStallCycles;
  } else if ( theInputQCntl.available(VC0) && !theTsrf.isEntryAvail(tTsrf::eNormalRequest )) {
    ++statNetworkQueueTSRFExhaustedStallCycles;
  }

  //Now see if any lock or directory entries have returned
  while (theSrvcProv.hasDirectoryResponse()) {
    std::pair< tAddress, boost::intrusive_ptr<tDirEntry const> > response = theSrvcProv.dequeueDirectoryResponse();

    if (response.second) {
      processDirectoryReply(response.first, response.second);
    } else {
      processLockAcquired(response.first);
    }
  }
}

bool tThreadScheduler::ready() {
  return theActiveThread || ! theRunnableThreads.empty();
}

tThread tThreadScheduler::activate() {
  if (theActiveThread) {
    return *theActiveThread;
  } else {
    theActiveThread = theRunnableThreads.front();
    DBG_(Iface, ( << theEngineName << " Activating " << *theActiveThread));
    theRunnableThreads.pop_front();
    theActiveThread->setDelayCause(theEngineName, "Run");
  }
  return *theActiveThread;
}

void tThreadScheduler::reschedule(tThread aThread) {
  DBG_Assert(aThread.isValid());
  switch ( aThread.state() ) {
    case eNoThread:
      DBG_Assert( false, ( << "a thread should never report NoThread state" ) );
    case eWaiting:
    case eSuspendedForLock:
    case eSuspendedForDir:
    case eBlocked:
      if (theActiveThread && aThread == *theActiveThread) {
        theActiveThread = boost::none;
      }
      //No longer active
      break;
    case eRunnable:
      if (theActiveThread && aThread == *theActiveThread) {
        //Do nothing, the thread may continue to run
      } else {
        //Place thread at the end of the runnable list
        DBG_Assert( std::count(theRunnableThreads.begin(), theRunnableThreads.end(), aThread) == 0);
        theRunnableThreads.push_back(aThread);
        aThread.setDelayCause(theEngineName, "Wait to Run");
      }
      break;
    case eComplete:
      if (theActiveThread && aThread == *theActiveThread) {
        theActiveThread = boost::none;
      }
      //Remove the thread from theLiveThreads and reclaim its TSRF
      statThreadUops << std::make_pair(aThread.uopCount(), 1);
      reclaimThread(aThread);
      break;
    default:
      DBG_Assert(false);
  }
}

void tThreadScheduler::waitForPacket(tThread aThread) {
  DBG_(Iface, ( << theEngineName << " Waiting for packet for " << aThread));

  //Make sure this is called for a running thread
  DBG_Assert( aThread.isRunnable() );

  //First, see if we have already delivered the message that this thread is
  //now blocking on.  This can happen if a message is both a request and a
  //potential reply.  If the message arrives before this thread is created,
  //then the message may have allocated a TSRF entry and be stuck in
  //the eSuspendedForLock state.

  //Get the list of threads this packet matches
  tMatchedThreads matches = theLiveThreads.equal_range(aThread.address());

  //There better be at least one, as aThread should match!.
  DBG_Assert( matches.first != matches.second ) ;

  tThreadMap::iterator iter = matches.first;
  while (iter != matches.second) {
    tThread suspended_thread = iter->second;
    if ( suspended_thread.isState(eSuspendedForLock) || (suspended_thread.isState(eBlocked) && suspended_thread.blockedOn() != aThread.threadID()) ) {
      if (isPotentialReply( suspended_thread.type() ) && isRequest( suspended_thread.type() )) {
        //We are going to steal the message from the suspended thread and cancel it.
        DBG_(Iface, ( << theEngineName << " Stealing the packet previously delivered to " << suspended_thread << " for thread " << aThread));

        //Tell the input Q that the message should be redelivered next cycle
        theInputQCntl.refuse(suspended_thread.VC(), suspended_thread.packet());

        if ( suspended_thread.isState(eSuspendedForLock) ) {
          //Squash the lock request from the suspended thread
          theSrvcProv.LockOp(LOCK_SQUASH, aThread.address());
        } else if ( suspended_thread.isState(eBlocked) ) {
          //Need to remove this thread from the blocked thread list
          deleteBlockedThread(suspended_thread);
        }

        //Cancel the previous thread
        suspended_thread.cancel();
        reschedule(suspended_thread);
        break;
      }
    }
    ++iter;
  }

  aThread.wait();
  aThread.setDelayCause(theEngineName, "Wait for Reply");
}

std::ostream & operator << ( std::ostream & anOstream, tThread const & aThread) {
  if (aThread.isValid()) {
    anOstream << "tid(" << aThread.threadID() << ") " << aThread.state() <<  " @0x" << &std::hex << aThread.address() << &std::dec;
  } else {
    anOstream << "tid(invalid)";
  }
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tThreadState x) {
  const char * const name[] = {
    "NoThread"
    , "Waiting"
    , "SuspendedForLock"
    , "SuspendedForDir"
    , "Runnable"
    , "Complete"
    , "Blocked"
  };
  DBG_Assert(x < static_cast<int>(sizeof(name)));
  anOstream << name[x];
  return anOstream;
}

bool tThreadScheduler::handleDowngrade(tAddress anAddress) {
  bool ret_val = false;
  tBlockedThreads::iterator block_list = theBlockedThreads.find(anAddress);
  if (block_list != theBlockedThreads.end() ) {
    std::list<tThread>::iterator iter = block_list->second.begin();
    while (iter != block_list->second.end()) {
      if (iter->type() == LocalFlush ||  iter->type() == LocalEvict) {
        //Cancel the thread.
        iter->cancel();
        reschedule(*iter);
        iter = block_list->second.erase(iter);
        //Set ret_val to true so we change the Downgrade to a DownUpdate
        ret_val = true;
      } else {
        ++iter;
      }
    }
    if (block_list->second.empty()) {
      theBlockedThreads.erase(block_list);
    }
  }
  return ret_val;
}

bool tThreadScheduler::handleInvalidate(tAddress anAddress) {
  bool ret_val = false;
  tBlockedThreads::iterator block_list = theBlockedThreads.find(anAddress);
  if (block_list != theBlockedThreads.end() ) {
    std::list<tThread>::iterator iter = block_list->second.begin();
    while (iter != block_list->second.end()) {
      if (iter->type() == LocalFlush ||  iter->type() == LocalEvict) {
        //Cancel the thread.
        iter->cancel();
        reschedule(*iter);
        iter = block_list->second.erase(iter);
        //Set ret_val to true so we change the Downgrade to a DownUpdate
        ret_val = true;
      } else if (iter->type() == LocalUpgradeAccess ) {
        iter->setType(LocalWriteAccess);
        ++iter;
      } else {
        ++iter;
      }
    }
    if (block_list->second.empty()) {
      theBlockedThreads.erase(block_list);
    }
  }
  return ret_val;
}

}  // namespace nProtocolEngine
