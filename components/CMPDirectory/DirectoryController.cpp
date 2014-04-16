#include <boost/dynamic_bitset.hpp>
#include <core/flexus.hpp>

#include <core/simulator_layout.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/MessageQueues.hpp>

#include <components/CMPDirectory/DirectoryController.hpp>

#include <fstream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace nCMPDirectory {

DirectoryController::DirectoryController( const std::string & aName,
    int32_t aNumCores,
    int32_t aBlockSize,
    int32_t aNumBanks,
    int32_t aBankNum,
    int32_t anInterleaving,
    int32_t aDirLatency,
    int32_t aDirIssueLat,
    int32_t aQueueSize,
    int32_t aMAFSize,
    int32_t anEBSize,
    std::string & aDirPolicy,
    std::string & aDirType,
    std::string & aDirConfig
                                        )
  : theName(aName)
  , theDirectoryInfo(aBankNum, aName, aDirPolicy, aDirType, aDirConfig, aNumCores, aBlockSize, aNumBanks, anInterleaving, aMAFSize, anEBSize)
  , theMAFPipeline(aName + "-MafPipeline", 1, 1, 0,
                   boost::intrusive_ptr<Stat::StatLog2Histogram>(new Stat::StatLog2Histogram(aName + "-MafPipeline" + "-InterArrivalTimes")))
  , theDirectoryPipeline(aName + "-DirPipeline", 1, aDirIssueLat, aDirLatency,
                         boost::intrusive_ptr<Stat::StatLog2Histogram>(new Stat::StatLog2Histogram(aName + "-DirPipeline" + "-InterArrivalTimes")))
  , theMafEntriesUsed(aName + "-MafUsed")
  , theMafEntriesActive(aName + "-MafActive")
  , theMafUtilization(aName + "-MafUtilization")
  , theDirUtilization(aName + "-DirUtilization")
  , theRequestMAFStalls(aName + "-RequestStall:MAF")
  , theRequestEBStalls(aName + "-RequestStall:EB")
  , theRequestQueueStalls(aName + "-RequestStall:Queue")
  , RequestIn(aQueueSize)
  , SnoopIn(aQueueSize)
  , ReplyIn(aQueueSize)
  , RequestOut(aQueueSize)
  , SnoopOut(aQueueSize)
  , ReplyOut(aQueueSize) {
  thePolicy =  AbstractFactory<AbstractPolicy, DirectoryInfo>::createInstance(aDirPolicy, theDirectoryInfo);
  theMaxSnoopsPerRequest = thePolicy->maxSnoopsPerRequest();
}

DirectoryController::~DirectoryController() {
  delete thePolicy;
}

bool DirectoryController::isQuiesced() const {
  return  theMAFPipeline.empty()
          && theDirectoryPipeline.empty()
          && RequestIn.empty()
          && SnoopIn.empty()
          && ReplyIn.empty()
          && RequestOut.empty()
          && SnoopOut.empty()
          && ReplyOut.empty()
          && thePolicy->isQuiesced();
}

void DirectoryController::saveState(std::string const & aDirName) {
}

void DirectoryController::loadState(std::string const & aDirName) {
  std::string fname( aDirName);
  // This is a little ugly.
  // We hard-code the filename
  // The functional simulator saves all the directory state in one file
  // but the timing simulator has seperate objects for each directory bank
  // In practice, this would be better off as a parameter
  fname += "/sys-directory.gz";
  std::ifstream ifs(fname.c_str(), std::ios::binary);
  if (! ifs.good()) {
    DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
  } else {

    boost::iostreams::filtering_stream<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(ifs);

    if ( ! thePolicy->loadState( in ) ) {
      DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                    ".  Make sure your checkpoints match your current cache configuration." ) );
      DBG_Assert ( false );
    }
  }
  ifs.close();
}

void DirectoryController::enqueueRequest(const MemoryTransport & transport) {
  RequestIn.enqueue(transport);
  theMafEntriesUsed << std::make_pair( thePolicy->MAF().usedEntries(), 1);
  theMafEntriesActive << std::make_pair( thePolicy->MAF().activeEntries(), 1);
}

void DirectoryController::processMessages() {

  // Track utilization stats
  theMafUtilization << std::make_pair( theMAFPipeline.size(), 1);
  theDirUtilization << std::make_pair( theDirectoryPipeline.size(), 1);

  // Move messages from the input queues into the MAF pipeline
  // handles priorities and resource reservations
  // messages stall in the queues until all the resources are available
  // and they are the highest priority message that is ready
  scheduleNewProcesses();

  // Take processes out of the MAF pipeline and perform a lookup to determine
  // what action should be taken, and then take that action
  advanceMAFPipeline();

  // take processes out of the directory pipeline
  // and put any output messages in the appropriate queues
  advanceDirPipeline();
}

// Examine input queues and move messages into the MAF pipeline when resources are available
void DirectoryController::scheduleNewProcesses() {
  FLEXUS_PROFILE();

  bool scheduled = false;
  bool stall_recorded = false;

  // First, look for Woken MAF entries
  while (  theMAFPipeline.serverAvail()
           && thePolicy->MAF().hasWakingEntry()
           && SnoopOut.hasSpace(theMaxSnoopsPerRequest)
           && ReplyOut.hasSpace(1)
           && RequestOut.hasSpace(1)
           && thePolicy->EBHasSpace(thePolicy->MAF().peekWakingMAF()->transport())
        ) {
    ProcessEntry_p process(new ProcessEntry( eProcWakeMAF, thePolicy->MAF().getWakingMAF()));
    reserveSnoopOut( process, theMaxSnoopsPerRequest);
    reserveReplyOut( process, 1);
    reserveRequestOut( process, 1);
    reserveEB( process, thePolicy->getEBRequirements(process->transport()) );
    theMAFPipeline.enqueue(process);
    DBG_(Trace, ( << theName << " Scheduled WakeMAF: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && thePolicy->MAF().hasWakingEntry()) {
    DBG_(Trace, ( << theName << " Failed to schedule waking MAF: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
                  << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve()
                  << ", ReqOut " << RequestOut.getSize() << "/" << RequestOut.getReserve()
                  << ", EB full " << std::boolalpha << thePolicy->EB().full() ));

    // A MAF request is stalling
    // Determine whether we're int16_t on port buffers or evict buffers
    if (!thePolicy->EBHasSpace(thePolicy->MAF().peekWakingMAF()->transport())) {
      // The EB is full
      theRequestEBStalls++;
    } else {
      // There is a full queue somewhere
      theRequestQueueStalls++;
    }
    stall_recorded = true;
  }

  // We want to make sure we have at least one free evict buffer so we can keep handling incoming requests
  // While the "idleWork" below will free up evict buffers, we also need a higher priority mechanism for
  // Keeping one evict buffer free
  while (  theMAFPipeline.serverAvail()
           && !thePolicy->EB().freeSlotsPending() // "Pending" to account for scheduled evicts
           && SnoopOut.hasSpace(1)
        ) {
    DBG_(Trace, ( << theName << "freeSlotsPending() = " << std::boolalpha << thePolicy->EB().freeSlotsPending() ));
    ProcessEntry_p process(new ProcessEntry( eProcEvict, thePolicy->getEvictBlockTransport()));
    reserveSnoopOut( process, 1);
    theMAFPipeline.enqueue(process);
    DBG_(Trace, ( << theName << " Scheduled Evict: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && !thePolicy->EB().freeSlotsPending()) {
    DBG_(Trace, ( << theName << " Failed to schedule evict, no snoop buffer space." ));
  }

  // Second, look for new Reply Messages (highest priority)
  while (  theMAFPipeline.serverAvail()
           && !ReplyIn.empty()
           && SnoopOut.hasSpace(1)
           && ReplyOut.hasSpace(1)
        ) {
    ProcessEntry_p process(new ProcessEntry( eProcReply, ReplyIn.dequeue()));
    reserveSnoopOut( process, 1);
    reserveReplyOut( process, 1);
    theMAFPipeline.enqueue(process);
    DBG_(Trace, ( << theName << " Scheduled Reply: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && !ReplyIn.empty()) {
    DBG_(Trace, (  << theName << " Failed to schedule Reply: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
                   << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve() ));
  }

  // Third, look for new Snoop Messages (second highest priority)
  while (  theMAFPipeline.serverAvail()
           && !SnoopIn.empty()
           && SnoopOut.hasSpace(1)
           && ReplyOut.hasSpace(1)
           && !thePolicy->MAF().full()
        ) {
    ProcessEntry_p process(new ProcessEntry( eProcSnoop, SnoopIn.dequeue()));
    reserveSnoopOut( process, 1);
    reserveReplyOut( process, 1);
    reserveMAF( process );
    theMAFPipeline.enqueue(process);
    DBG_(Trace, ( << theName << " Scheduled Snoop: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && !SnoopIn.empty()) {
    DBG_(Trace, (  << theName << " Failed to schedule Snoop: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
                   << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve() << std::boolalpha << ", MAF full " << thePolicy->MAF().full() ));
  }

  // Fourth, look for new Request Messages (lowest priority)
  while (  theMAFPipeline.serverAvail()
           && !RequestIn.empty()
           && SnoopOut.hasSpace(theMaxSnoopsPerRequest)
           && ReplyOut.hasSpace(1)
           && RequestOut.hasSpace(1)
           && thePolicy->EBHasSpace(RequestIn.peek())
           && !thePolicy->MAF().full()
        ) {
    ProcessEntry_p process(new ProcessEntry( eProcRequest, RequestIn.dequeue()));
    reserveSnoopOut( process, theMaxSnoopsPerRequest);
    reserveReplyOut( process, 1);
    reserveRequestOut( process, 1);
    reserveEB( process, thePolicy->getEBRequirements(process->transport()) );
    reserveMAF( process );
    theMAFPipeline.enqueue(process);
    DBG_Assert(process->transport()[MemoryMessageTag]);

    DBG_(Trace, ( << theName << " Scheduled Request: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && !RequestIn.empty()) {
    DBG_(Trace, (  << theName << " Failed to schedule Request: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
                   << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve()
                   << ", ReqOut " << RequestOut.getSize() << "/" << RequestOut.getReserve()
                   << ", EB full " << std::boolalpha << thePolicy->EB().full() << ", MAF full " << thePolicy->MAF().full() ));

    if (!stall_recorded) {
      // A new request is stalling
      // Determine whether we're int16_t on port buffers or evict buffers
      if (thePolicy->MAF().full()) {
        theRequestMAFStalls++;
      } else if (!thePolicy->EBHasSpace(RequestIn.peek())) {
        // The EB is full
        theRequestEBStalls++;
      } else {
        // There is a full queue somewhere
        theRequestQueueStalls++;
      }
    }
    stall_recorded = true;
  }

  // Fifth, look for idle work (ie. invalidates for maintaining inclusion, etc.)
  while (  theMAFPipeline.serverAvail()
           && thePolicy->hasIdleWorkAvailable()
           && SnoopOut.hasSpace(1)
        ) {
    ProcessEntry_p process(new ProcessEntry( eProcIdleWork, thePolicy->getIdleWorkTransport()));
    reserveSnoopOut( process, 1);
    theMAFPipeline.enqueue(process);
    // Get any additional, policy specific reservations
    thePolicy->getIdleWorkReservations(process);
    DBG_(Trace, ( << theName << " Scheduled IdleWork: " << *(process->transport()[MemoryMessageTag]) ));
    scheduled = true;
  }
  if (!scheduled && thePolicy->hasIdleWorkAvailable()) {
    DBG_(Trace, ( << theName << " Failed to schedule idle work, no space in Snoop Out" ));
  }

}

void DirectoryController::advanceMAFPipeline() {
  FLEXUS_PROFILE();

  while ( theMAFPipeline.ready() && theDirectoryPipeline.serverAvail()) {
    ProcessEntry_p process = theMAFPipeline.dequeue();
    switch (process->type()) {
      case eProcWakeMAF:
        runWakeMAFProcess(process);
        break;
      case eProcEvict:
        runEvictProcess(process);
        break;
      case eProcReply:
        runReplyProcess(process);
        break;
      case eProcSnoop:
        runSnoopProcess(process);
        break;
      case eProcRequest:
        runRequestProcess(process);
        break;
      case eProcIdleWork:
        runIdleWorkProcess(process);
        break;
      default:
        DBG_Assert(false, ( << "Invalid process type " << process->type() ));
    }
  }
}

void DirectoryController::advanceDirPipeline() {
  FLEXUS_PROFILE();

  while ( theDirectoryPipeline.ready() ) {
    ProcessEntry_p process = theDirectoryPipeline.dequeue();
    finalizeProcess(process);
  }
}

// Determine action for Request Process and enqueue in directory pipeline
void DirectoryController::runRequestProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  DBG_(Trace, ( << theName << " handleRequest: " << *process << " for " << *(process->transport()[MemoryMessageTag]) ));

  thePolicy->handleRequest(process);

  switch (process->action()) {
    case eFwdAndWaitAck:
      // Un-reserve 1 snoop, 1 reply, 1 eb and 1 maf
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveReplyOut( process, 1);
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      break;
    case eFwdRequest:
      // Un-reserve 1 snoop, 1 reply, 1 eb and 1 maf
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveReplyOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      break;
    case eNotifyAndWaitAck:
      // Un-reserve 1 eb and 1 maf
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveEB( process );
      unreserveMAF( process );
      unreserveRequestOut( process, 1);
      // We need to send a snoop and a reply
      // Create a MAF entry
      break;
    case eReply:
    case eReplyAndRemoveMAF:
      // Un-reserve 2 snoops, 1 eb and 1 maf
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      break;
    case eStall:
      // Un-reserve 2 snoops, 1 reply, 1 eb and 1 maf
      if (process->getSnoopTransports().size() > 0) {
        DBG_(Crit, ( << theName << " Process stalled while holding SnoopOut reservations: " << *(process->transport()[MemoryMessageTag]) ));
      }
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveReplyOut( process, 1);
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      dumpMAFEntries(process->transport()[MemoryMessageTag]->address());
      break;

// Actions for Evict Messages (now come in on Request channel, still sent out on Snoop Channel (no, I don't know why))
    case eFwdAndWakeEvictMAF:
    case eForward:
      unreserveReplyOut( process, 1);
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      // Send a snoop message
      break;

    case eFwdAndReply:
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      break;

    case eWakeEvictMAF:
    case eNoAction:
      // Un-reserve 2 snoops, 1 request, 1 reply, 1 eb and 1 maf
      unreserveReplyOut( process, 1);
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      unreserveMAF( process );
      break;
// End evict actions

    default:
      DBG_Assert( false, ( << "Unexpected action type for Request process: " << process->action() ));
      break;
  }

  DBG_(Trace, ( << theName << " runRequestProcess: " << *process << " for " << *(process->transport()[MemoryMessageTag]) ));

  // Either put into the directory pipeline, or finalize the process
  if (process->lookupRequired()) {
    theDirectoryPipeline.enqueue(process);
  } else {
    finalizeProcess(process);
  }
}

// Determine action for Snoop Process and enqueue in directory pipeline
void DirectoryController::runSnoopProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  DBG_(Trace, ( << theName << " runSnoopProcess: " << *process->transport()[MemoryMessageTag] ));
  thePolicy->handleSnoop(process);

  switch (process->action()) {
    case eFwdAndWakeEvictMAF:
    case eForward:
      // Un-reserve 1 reply
      unreserveReplyOut( process, 1);
      unreserveMAF( process );
      // Send a snoop message
      break;
    case eReply:
      // Un-reserve 1 snoop
      unreserveSnoopOut( process, 1);
      unreserveMAF( process );
      // Send a reply message (mimic a reply that would have been sent if the evict hadn't happened)
      break;

    case eWakeEvictMAF:
    case eNoAction:
      // Un-reserve 1 snoop and 1 reply
      unreserveSnoopOut( process, 1);
      unreserveReplyOut( process, 1);
      unreserveMAF( process );
      break;

    default:
      DBG_Assert( false, ( << "Unexpected action type for Snoop process: " << process->action() ));
      break;
  }

  // Either put into the directory pipeline, or finalize the process
  if (process->lookupRequired()) {
    theDirectoryPipeline.enqueue(process);
  } else {
    finalizeProcess(process);
  }

}

// Determine action for Reply Process and enqueue in directory pipeline
void DirectoryController::runReplyProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  DBG_(Trace, ( << theName << " runReplyProcess: " << *process->transport()[MemoryMessageTag] ));
  thePolicy->handleReply(process);

  switch (process->action()) {
    case eForward:
    case eFwdAndWakeEvictMAF:
      unreserveReplyOut( process, 1);
      break;
    case eReply:
      // Un-reserve 1 snoop
      unreserveSnoopOut( process, 1);
      // Send a reply message (mimic a reply that would have been sent if the evict hadn't happened)
      break;
    case eRemoveAndWakeMAF:
    case eRemoveMAF:
    case eWakeEvictMAF:
    case eNoAction:
      // Un-reserve 1 snoop
      unreserveSnoopOut( process, 1);
      unreserveReplyOut( process, 1);
      break;
    default:
      DBG_Assert( false, ( << "Unexpected action type for Reply process: " << process->action() ));
      break;
  }
  DBG_(Trace, ( << theName << " runReplyProcess: " << *process << " FOR " << *process->transport()[MemoryMessageTag] ));

  // Either put into the directory pipeline, or finalize the process
  if (process->lookupRequired()) {
    theDirectoryPipeline.enqueue(process);
  } else {
    finalizeProcess(process);
  }

}

// Determine action for Evict Process
void DirectoryController::runEvictProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  DBG_(Trace, ( << theName << " runEvictProcess: " << *process->transport()[MemoryMessageTag] ));
  thePolicy->handleEvict(process);

  DBG_Assert( process->action() == eForward, ( << "Unexpected action type for Evict process: " << process->action() ));

  // Send out a snoop
  // No pipeline, we don't need the directory pipeline (we already used the MAF pipeline)
  finalizeProcess(process);

}

// Determine action for IdleWork Process
void DirectoryController::runIdleWorkProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  DBG_(Trace, ( << theName << " runIdleWorkProcess: " << *process->transport()[MemoryMessageTag] ));
  thePolicy->handleIdleWork(process);

  if (process->action() == eNoAction) {
    unreserveSnoopOut( process, 1);
  } else {
    DBG_Assert( process->action() == eForward, ( << "Unexpected action type for IdleWork process: " << process->action() ));
  }

  // Let the IdleWork process use either
  // Either put into the directory pipeline, or finalize the process
  if (process->lookupRequired()) {
    theDirectoryPipeline.enqueue(process);
  } else {
    finalizeProcess(process);
  }

}

// Determine action for WakeMAFProcess and enqueue in directory pipeline
void DirectoryController::runWakeMAFProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();
  thePolicy->handleWakeMAF(process);

  switch (process->action()) {
    case eFwdAndWaitAck:
      // Un-reserve 1 snoop, 1 reply, 1 eb
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveReplyOut( process, 1);
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      // We need to forward a snoop message somewhere
      // Update MAF entry
      // Then enqueue in the Directory Pipeline
      break;
    case eNotifyAndWaitAck:
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      // Un-reserve 1 eb
      unreserveEB( process );
      // We need to send a snoop and a reply
      // Update MAF entry
      // Enqueue in the Directory Pipeline
      break;
    case eReply:
    case eReplyAndRemoveMAF:
      // Un-reserve 2 snoops, 1 eb
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveRequestOut( process, 1);
      unreserveEB( process );
      // Update MAF entry (will be removed when we leave the pipeline)
      break;
    case eStall:
      // Un-reserve 2 snoops, 1 reply, 1 eb
      unreserveSnoopOut( process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
      unreserveReplyOut( process, 1);
      unreserveEB( process );
      unreserveRequestOut( process, 1);
      // Update MAF entry that will be woken up later
      break;
    default:
      DBG_Assert( false, ( << "Unexpected action type for Request process: " << process->action() ));
      break;
  }

  DBG_(Trace, ( << theName << " runWakeMAFProcess: " << *process << " FOR " << *process->transport()[MemoryMessageTag] ));

  // Either put into the directory pipeline, or finalize the process
  if (process->lookupRequired()) {
    theDirectoryPipeline.enqueue(process, process->requiredLookups());
  } else {
    finalizeProcess(process);
  }

}

// Take final process actions, including removing MAF entry
// and sending final messages
void DirectoryController::finalizeProcess(ProcessEntry_p process) {
  FLEXUS_PROFILE();

  ProcessEntry_p temp_p(process);

  DBG_(Trace, ( << "Finalize process: " << *temp_p ));

  switch (process->action()) {
    case eFwdAndWaitAck:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      break;
    case eFwdRequest:
      unreserveRequestOut( process, 1);
      sendRequest(process);
      break;
    case eNotifyAndWaitAck:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      unreserveReplyOut(process, 1);
      DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != NULL, ( << "No reply msg for " << *(process->transport()[MemoryMessageTag]) ));
      sendReply(process);
      break;
    case eFwdAndReply:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      unreserveReplyOut(process, 1);
      DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != NULL, ( << "No reply msg for " << *(process->transport()[MemoryMessageTag]) ));
      sendReply(process);
      break;
    case eForwardAndRemoveMAF:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      thePolicy->MAF().remove(process->maf());
      break;
    case eFwdAndWakeEvictMAF:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      DBG_(Trace, ( << theName << " Waking MAF Waiting on Evict: " << *(process->maf()->transport()[MemoryMessageTag]) ));
      thePolicy->MAF().wakeAfterEvict(process->maf());
      break;
    case eWakeEvictMAF:
      DBG_(Trace, ( << theName << " Waking MAF Waiting on Evict: " << *(process->maf()->transport()[MemoryMessageTag]) ));
      thePolicy->MAF().wakeAfterEvict(process->maf());
      break;
    case eForward:
      DBG_Assert(process->getSnoopTransports().size() != 0);
      unreserveSnoopOut( process, process->getSnoopTransports().size());
      sendSnoops(process);
      break;
    case eReply:
      unreserveReplyOut(process, 1);
      DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != NULL, ( << "No reply msg for " << *(process->transport()[MemoryMessageTag]) ));
      sendReply(process);
      break;
    case eReplyAndRemoveMAF:
      unreserveReplyOut(process, 1);
      DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != NULL, ( << "No reply msg for " << *(process->transport()[MemoryMessageTag]) ));
      sendReply(process);
      thePolicy->MAF().remove(process->maf());
      thePolicy->wakeMAFs(process->transport()[MemoryMessageTag]->address());
      break;
    case eRemoveMAF:
      thePolicy->MAF().remove(process->maf());
      break;
    case eRemoveAndWakeMAF:
      thePolicy->MAF().remove(process->maf());
      thePolicy->wakeMAFs(process->transport()[MemoryMessageTag]->address());
      break;
    case eStall:
    case eNoAction:
      break;
  }

  DBG_Assert(process->getSnoopTransports().size() == 0);
  DBG_Assert(process->noReservations(), ( << "Process finalized while holding reservations: " << *process ));
}

void DirectoryController::sendSnoops(ProcessEntry_p process) {
  std::list<MemoryTransport>& snoops = process->getSnoopTransports();
  while (!snoops.empty()) {
    SnoopOut.enqueue(snoops.front());
    snoops.pop_front();
  }
}

void DirectoryController::sendReply(ProcessEntry_p process) {
  ReplyOut.enqueue(process->getReplyTransport());
}

void DirectoryController::sendRequest(ProcessEntry_p process) {
  RequestOut.enqueue(process->transport());
}

void DirectoryController::dumpMAFEntries(MemoryAddress addr) {
  MissAddressFile::iterator iter = thePolicy->MAF().find(addr);
  for (; iter != thePolicy->MAF().end(); iter++) {
    DBG_(Trace, ( << theName << " MAF contains: " << *(iter->transport()[MemoryMessageTag]) << " in state " << (int)(iter->state()) ));
  }
}
}; // namespace nCMPDirectory

