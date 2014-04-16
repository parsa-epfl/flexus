#ifndef FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED
#define FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED

#include "thread.hpp"
#include <core/stats.hpp>

namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

class tInputQueueController;
class tSrvcProvider;
class tMicrocodeEmulator;

class tThreadScheduler {
  std::string theEngineName;
  tTsrf & theTsrf;
  tInputQueueController & theInputQCntl;
  tSrvcProvider & theSrvcProv;
  tMicrocodeEmulator & theExecEngine;

  std::list<tThread> theRunnableThreads;
  boost::optional<tThread> theActiveThread;

  typedef std::multimap<tAddress, tThread> tThreadMap;
  typedef std::pair< tThreadMap::iterator, tThreadMap::iterator>  tMatchedThreads;
  tThreadMap theLiveThreads;

  typedef std::map<tAddress, std::list<tThread> > tBlockedThreads;
  tBlockedThreads theBlockedThreads;

  typedef std::multimap<tThread, tVC > tThreadToStalledQueueMap;
  tThreadToStalledQueueMap theThreadToStalledQueueMap;
  std::set<tVC> theStalledQueues;

  int32_t theNextThreadID;

  Stat::StatAverage statAvgThreadLifetime;
  Stat::StatLog2Histogram statThreadLifetimeDistribution;
  Stat::StatAverage statAvgThreadRuntime;
  Stat::StatLog2Histogram statThreadRuntimeDistribution;
  Stat::StatCounter statLocalQueueAddressConflictStallCycles;
  Stat::StatCounter statLocalQueueTSRFExhaustedStallCycles;
  Stat::StatCounter statNetworkQueueAddressConflictStallCycles;
  Stat::StatCounter statNetworkQueueTSRFExhaustedStallCycles;
  Stat::StatInstanceCounter<int64_t> statThreadUops;

public:
  bool isQuiesced() const {
    return theLiveThreads.empty() ;
  }

  //Construction
  tThreadScheduler(std::string const & anEngineName, tTsrf & aTsrf, tInputQueueController & anInputQCntl, tSrvcProvider & aSrvcProv, tMicrocodeEmulator & anExecEngine);

public:
  //Interface to ProtocolEngine
  //Instruct ThreadDriver to process all input queues and deliver packets
  void processQueues();
  //Returns true if any thread is runnable
  bool ready();
  //Returns a runnable thread selected by the thread scheduler
  tThread activate();
  //Reschedules a thread for further execution, cleanup, etc.
  void reschedule(tThread aThread);

  bool handleDowngrade(tAddress anAddress);
  bool handleInvalidate(tAddress anAddress);

public:
  //Interface to ExecEngine
  //Implementation of the REFUSE instruction.
  void refusePacket(tThread aThread);
  //Implementation of the RECEIVE instruction.
  void waitForPacket(tThread aThread);

private:
  //Input Queue Stalling management
  //Stall a virtual channel until a particular thread completes.
  //A channel may only be stalled on one thread (or assertions will fire)
  void stallQueue(tThread aThread, tVC aQueue);
  //Returns true if a virtual channel is stalled
  bool isQueueStalled(tVC aQueue);
  //Releases all virtual channels stalled on a thread
  void releaseAllQueues(tThread aThread);

  void deleteBlockedThread(tThread aThread);
  void unblockThreads(tThread aThread);
  void blockThread(tThread aThread, tThread aBlockOn);

  //Live Thread map management
  //Add a thread to theLiveThreads
  void registerThread( tThread aThread );
  //Reclaim all the resources associated with a thread (TSRF, stalled queues)
  void reclaimThread(tThread aThread);
  //Check if there is at least one live thread for an address
  bool hasLiveThread(tAddress anAddress);
  //Returns the unique thread in aMatchedThreadRange that is in aState, if it exists
  //ASSERTs if there are more than one such thread
  boost::optional<tThread> findUniqueThread(tThreadState aState, tMatchedThreads aMatchedThreadRange);
  //Returns true if there is at least one thread in aMatchedThreadRange that is in aState
  bool anyThreadsAre(tThreadState aState, tMatchedThreads aMatchedThreadRange);

  void launchThread(tThread aThread);

  void createBlockedThread(tVC aVC, tThread aBlockOn);

  //Main processing functions for input queue entries
  //Processes packets which do not have the same address as any existing thread
  void processUnmatchedPacket(tVC aVC);
  //Processes packets which have the same address as an existing thread
  void processMatchedPacket(tVC aVC);
  //Processes lock acquisition messages
  void processLockAcquired(tAddress anAddress);
  //Processes replies from the directory
  void processDirectoryReply(tAddress anAddress, boost::intrusive_ptr<tDirEntry const> aDirEntry);

  //Utility functions for managing thread state
  //Fills in the threads TSRF upon thread creation
  tThread createThread(tVC aVC, tTsrfEntry * anEntry);
  tThread createThread(boost::intrusive_ptr<tPacket> packet, tVC aSourceVC, tTsrfEntry * anEntry); //Used in implementation of createThread(tVC, tTsrfEntry *)
  //Transitions thread to eSuspendedForLock state
  void requestDirectoryLock(tThread aThread);
  //Transitions thread to eSuspendedForDir state
  void requestDirectoryEntry(tThread aThread);
  //Transitions a thread to eRunnable at the specified entry point
  void start(tThread aThread, unsigned anEntryPoint);
  //Transitions a thread to eRunnable and delivers a reply message
  void deliverReply(tThread aThread, tVC aVC);

  //Returns true if a packet needs to obtain a directory lock
  bool requiresDirectoryLock(tPacket const & packet);

};

}  // namespace nProtocolEngine

#endif //FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED
