
#include "components/CMPCache/CMPCacheController.hpp"

#include "components/CommonQEMU/MessageQueues.hpp"
#include "components/CommonQEMU/Transports/MemoryTransport.hpp"
#include "core/boost_extensions/intrusive_ptr.hpp"
#include "core/debug/debug.hpp"
#include "core/flexus.hpp"
#include "core/performance/profile.hpp"
#include "core/qemu/configuration_api.hpp"
#include "core/simulator_layout.hpp"
#include "core/stats.hpp"
#include "core/target.hpp"
#include "core/types.hpp"

#include <boost/dynamic_bitset.hpp>
#include <fstream>

namespace nCMPCache {

REGISTER_CMP_CACHE_CONTROLLER(CMPCacheController, "Default");

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

CMPCacheController::CMPCacheController(const CMPCacheInfo& anInfo)
  : AbstractCacheController(anInfo)
  , theMAFPipeline(anInfo.theName + "-MafPipeline",
                   1,
                   1,
                   0,
                   boost::intrusive_ptr<Stat::StatLog2Histogram>(
                     new Stat::StatLog2Histogram(anInfo.theName + "-MafPipeline" + "-InterArrivalTimes")))
  , theDirTagPipeline(anInfo.theName + "-DirPipeline",
                      1,
                      MAX(anInfo.theDirIssueLatency, anInfo.theTagIssueLatency),
                      MAX(anInfo.theDirLatency, anInfo.theTagLatency),
                      boost::intrusive_ptr<Stat::StatLog2Histogram>(
                        new Stat::StatLog2Histogram(anInfo.theName + "-DirPipeline" + "-InterArrivalTimes")))
  , theDataPipeline(anInfo.theName + "-DataPipeline",
                    1,
                    anInfo.theDataIssueLatency,
                    anInfo.theDataLatency,
                    boost::intrusive_ptr<Stat::StatLog2Histogram>(
                      new Stat::StatLog2Histogram(anInfo.theName + "-DataPipeline" + "-InterArrivalTimes")))
  , theRequestInUtilization(anInfo.theName + "-RequestInUtilization")
  , theMafUtilization(anInfo.theName + "-MafUtilization")
  , theDirTagUtilization(anInfo.theName + "-DirTagUtilization")
  , theDataUtilization(anInfo.theName + "-DataUtilization")
  , theIdleWorkScheduled(false)
{
    theMaxSnoopsPerRequest = thePolicy->maxSnoopsPerRequest();
}

CMPCacheController::~CMPCacheController() {}

AbstractCacheController*
CMPCacheController::createInstance(std::list<std::pair<std::string, std::string>>& args, const CMPCacheInfo& params)
{
    DBG_Assert(args.empty(), NoDefaultOps());
    return new CMPCacheController(params);
}

bool
CMPCacheController::isQuiesced() const
{
    return theMAFPipeline.empty() && theDirTagPipeline.empty() && theDataPipeline.empty() && RequestIn.empty() &&
           SnoopIn.empty() && ReplyIn.empty() && RequestOut.empty() && SnoopOut.empty() && ReplyOut.empty() &&
           thePolicy->isQuiesced();
}

void
CMPCacheController::loadState(std::string const& ckpt_dirname)
{

    std::string real_name = theName;
    // -- LOAD DIRECTORY
    std::string ckpt_filename_cachedir(ckpt_dirname);
    ckpt_filename_cachedir += "/" + theName + "-dir-slice.json";
    DBG_(Dev, (<< " Start loading Directory state from " << ckpt_filename_cachedir));
    thePolicy->load_dir_from_ckpt(ckpt_filename_cachedir);

    // -- LOAD CACHE
    std::string ckpt_filename_cache(ckpt_dirname);
    ckpt_filename_cache += "/" + theName + "-cache-slice.json";
    DBG_(Dev, (<< " Start loading Cache state from " << ckpt_filename_cache));
    thePolicy->load_cache_from_ckpt(ckpt_filename_cache);
}

void
CMPCacheController::processMessages()
{

    // Track utilization stats
    theRequestInUtilization << std::make_pair(RequestIn.getSize(), 1);
    theMafUtilization << std::make_pair(theMAFPipeline.size(), 1);
    theDirTagUtilization << std::make_pair(theDirTagPipeline.size(), 1);
    theDataUtilization << std::make_pair(theDataPipeline.size(), 1);
    theDataUtilization << std::make_pair(theDataPipeline.size(), 1);
    // nSplitDestinationMapper::SplitDestinationMapperStats::getSplitDestinationMapperStats()->update(RequestIn.getSize(),theCMPCacheInfo.theNodeId,theCMPCacheInfo.theCores);

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
    advancePipeline();
}

// Examine input queues and move messages into the MAF pipeline when resources
// are available
void
CMPCacheController::scheduleNewProcesses()
{
    FLEXUS_PROFILE();

    bool scheduled = false;

    // First, look for Woken MAF entries
    while (theMAFPipeline.serverAvail() && thePolicy->MAF().hasWakingEntry() &&
           SnoopOut.hasSpace(theMaxSnoopsPerRequest) && ReplyOut.hasSpace(1) && RequestOut.hasSpace(1) &&
           thePolicy->EBHasSpace(thePolicy->MAF().peekWakingMAF()->transport())) {
        ProcessEntry_p process(new ProcessEntry(eProcWakeMAF, thePolicy->MAF().getWakingMAF()));
        reserveSnoopOut(process, theMaxSnoopsPerRequest);
        reserveReplyOut(process, 1);
        reserveRequestOut(process, 1);
        reserveEB(process, thePolicy->getEBRequirements(process->transport()));
        theMAFPipeline.enqueue(process);

        DBG_(VVerb, (<< theName << " Scheduled WakeMAF: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && thePolicy->MAF().hasWakingEntry()) {
        DBG_(VVerb,
             (<< theName << " Failed to schedule waking MAF: SnoopOut " << SnoopOut.getSize() << "/"
              << SnoopOut.getReserve() << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve()
              << ", ReqOut " << RequestOut.getSize() << "/" << RequestOut.getReserve() << ", EB full " << std::boolalpha
              << thePolicy->EBFull()));
    }

    // We want to make sure we have at least one free evict buffer so we can keep
    // handling incoming requests While the "idleWork" below will free up evict
    // buffers, we also need a higher priority mechanism for Keeping one evict
    // buffer free Do the Cache Evict Buffer and Directory Evict Buffer separately

    while (theMAFPipeline.serverAvail() && !theIdleWorkScheduled &&
           !thePolicy->freeCacheEBPending()            // "Pending" to account for scheduled evicts
           && (thePolicy->CacheEB().usedEntries() > 0) // Make sure we have used entries available
           && (thePolicy->CacheEB().usedEntries() > 0) // Make sure we have used entries available
           && SnoopOut.hasSpace(1)) {
        ProcessEntry_p process(new ProcessEntry(eProcCacheEvict, thePolicy->getCacheEvictTransport()));
        reserveSnoopOut(process, 1);

        theMAFPipeline.enqueue(process);
        DBG_(VVerb, (<< theName << " Scheduled Cache Evict: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && !thePolicy->freeCacheEBPending()) {
        DBG_(VVerb, (<< theName << " Failed to schedule evict, no snoop buffer space."));
    }

    while (theMAFPipeline.serverAvail() && !theIdleWorkScheduled &&
           !thePolicy->freeDirEBPending() // "Pending" to account for scheduled evicts
           && thePolicy->DirEB().idleWorkReady() && thePolicy->CacheEBHasSpace() && SnoopOut.hasSpace(1)) {
        ProcessEntry_p process(new ProcessEntry(eProcDirEvict, thePolicy->getDirEvictTransport()));
        reserveSnoopOut(process, 1);
        reserveCacheEB(process, 1);

        theMAFPipeline.enqueue(process);
        DBG_(VVerb, (<< theName << " Scheduled Dir Evict: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && !thePolicy->freeDirEBPending()) {
        DBG_(VVerb, (<< theName << " Failed to schedule evict, no snoop buffer space."));
    } else if (!scheduled && thePolicy->DirEB().full()) {
        DBG_(VVerb,
             (<< theName
              << " Failed to schedule Dir Evict when DirEB FULL: "
                 "Used/Reserved/Size = "
              << thePolicy->DirEB().used() << "/" << thePolicy->DirEB().reserved() << "/" << thePolicy->DirEB().size()
              << ", invalidates pending = " << thePolicy->DirEB().invalidates() << ": "
              << thePolicy->DirEB().listInvalidates()));
    }

    // Second, look for new Reply Messages (highest priority)
    while (theMAFPipeline.serverAvail() && !ReplyIn.empty() && SnoopOut.hasSpace(1) && ReplyOut.hasSpace(1)) {
        ProcessEntry_p process(new ProcessEntry(eProcReply, ReplyIn.dequeue()));
        reserveSnoopOut(process, 1);
        reserveReplyOut(process, 1);
        theMAFPipeline.enqueue(process);

        DBG_(VVerb, (<< theName << " Scheduled Reply: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && !ReplyIn.empty()) {
        DBG_(VVerb,
             (<< theName << " Failed to schedule Reply: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
              << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve()));
    }

    // Third, look for new Snoop Messages (second highest priority)
    while (theMAFPipeline.serverAvail() && !SnoopIn.empty() && SnoopOut.hasSpace(2) && ReplyOut.hasSpace(1) &&
           !thePolicy->MAF().full()) {
        ProcessEntry_p process(new ProcessEntry(eProcSnoop, SnoopIn.dequeue()));
        reserveSnoopOut(process, 2);
        reserveReplyOut(process, 1);
        reserveMAF(process);
        theMAFPipeline.enqueue(process);

        DBG_(VVerb, (<< theName << " Scheduled Snoop: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && !SnoopIn.empty()) {
        DBG_(VVerb,
             (<< theName << " Failed to schedule Snoop: SnoopOut " << SnoopOut.getSize() << "/" << SnoopOut.getReserve()
              << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve() << std::boolalpha << ", MAF full "
              << thePolicy->MAF().full()));
    }

    // Fourth, look for new Request Messages (lowest priority)
    while (theMAFPipeline.serverAvail() && !RequestIn.empty() && SnoopOut.hasSpace(theMaxSnoopsPerRequest) &&
           ReplyOut.hasSpace(1) && RequestOut.hasSpace(1) && thePolicy->EBHasSpace(RequestIn.peek()) &&
           !thePolicy->MAF().full()) {
        ProcessEntry_p process(new ProcessEntry(eProcRequest, RequestIn.dequeue()));
        reserveSnoopOut(process, theMaxSnoopsPerRequest);
        reserveReplyOut(process, 1);
        reserveRequestOut(process, 1);
        reserveEB(process, thePolicy->getEBRequirements(process->transport()));
        reserveMAF(process);
        theMAFPipeline.enqueue(process);

        DBG_(VVerb, (<< theName << " Scheduled Request: " << *(process->transport()[MemoryMessageTag])));
        scheduled = true;
    }
    if (!scheduled && !RequestIn.empty()) {
        DBG_(VVerb,
             (<< theName << " Failed to schedule Request: SnoopOut " << SnoopOut.getSize() << "/"
              << SnoopOut.getReserve() << ", ReplyOut " << ReplyOut.getSize() << "/" << ReplyOut.getReserve()
              << ", ReqOut " << RequestOut.getSize() << "/" << RequestOut.getReserve() << ", EB full " << std::boolalpha
              << thePolicy->EBFull() << ", MAF full " << thePolicy->MAF().full()));
        if (thePolicy->EBFull()) {
            DBG_(VVerb,
                 (<< theName << " DirEB.full() = " << std::boolalpha << thePolicy->DirEB().full()
                  << ", CacheEB (reserved,used,size) = (" << thePolicy->CacheEB().reservedEntries() << ", "
                  << thePolicy->CacheEB().usedEntries() << ", " << theCMPCacheInfo.theCacheEBSize
                  << "), ERB available = " << thePolicy->arrayEvictResourcesAvailable()
                  << ", freeDirEBPending() = " << thePolicy->freeDirEBPending()
                  << ", Dir.idleWorkReady() = " << thePolicy->DirEB().idleWorkReady()));
        }
    }

    // Fifth, look for idle work (ie. invalidates for maintaining inclusion, etc.)
    while (theMAFPipeline.serverAvail() && thePolicy->CacheEBHasSpace() && thePolicy->hasIdleWorkAvailable() &&
           SnoopOut.hasSpace(1) && !theIdleWorkScheduled // Only schedule 1 idle work item at a time
    ) {
        ProcessEntry_p process(new ProcessEntry(eProcIdleWork, thePolicy->getIdleWorkTransport()));
        reserveSnoopOut(process, 1);
        reserveCacheEB(process, 1);
        theMAFPipeline.enqueue(process);

        // Get any additional, policy specific reservations
        thePolicy->getIdleWorkReservations(process);

        DBG_(VVerb, (<< theName << " Scheduled IdleWork: " << *(process->transport()[MemoryMessageTag])));
        theIdleWorkScheduled = true;
        scheduled            = true;
    }
    if (!scheduled && thePolicy->hasIdleWorkAvailable()) {
        DBG_(VVerb, (<< theName << " Failed to schedule idle work, no space in Snoop Out"));
    }
}

void
CMPCacheController::advanceMAFPipeline()
{
    FLEXUS_PROFILE();

    while (theMAFPipeline.ready() && theDirTagPipeline.serverAvail()) {
        ProcessEntry_p process = theMAFPipeline.dequeue();
        switch (process->type()) {
            case eProcWakeMAF: runWakeMAFProcess(process); break;
            case eProcCacheEvict: runCacheEvictProcess(process); break;
            case eProcDirEvict: runDirEvictProcess(process); break;
            case eProcReply: runReplyProcess(process); break;
            case eProcSnoop: runSnoopProcess(process); break;
            case eProcRequest: runRequestProcess(process); break;
            case eProcIdleWork: runIdleWorkProcess(process); break;
            default: DBG_Assert(false, (<< "Invalid process type " << process->type()));
        }
    }
}

void
CMPCacheController::advancePipeline()
{
    FLEXUS_PROFILE();

    while (theDirTagPipeline.ready()) {
        ProcessEntry_p process = theDirTagPipeline.peek();
        if (process->requiresData()) {
            if (theDataPipeline.serverAvail()) {
                if (process->transmitAfterTag()) { finalizeProcess(process); }
                theDirTagPipeline.dequeue();
                theDataPipeline.enqueue(process);
            } else {
                theDirTagPipeline.stall();
                break;
            }
        } else {
            theDirTagPipeline.dequeue();
            finalizeProcess(process);
        }
    }

    while (theDataPipeline.ready()) {
        ProcessEntry_p process = theDataPipeline.dequeue();
        if (!process->transmitAfterTag()) { finalizeProcess(process); }
    }
}

// Determine action for Request Process and enqueue in directory pipeline
void
CMPCacheController::runRequestProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " handleRequest: " << *process << " for " << *(process->transport()[MemoryMessageTag])));

    thePolicy->handleRequest(process);
    auto req   = process->transport()[MemoryMessageTag];
    auto msg_a = req->address();

    switch (process->action()) {
        case eFwdAndWaitAck:
            // Un-reserve 1 snoop, 1 reply, 1 eb and 1 maf
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            break;
        case eFwdRequest:
            // Un-reserve 1 snoop, 1 reply, 1 eb and 1 maf
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            break;
        case eNotifyAndWaitAck:
            // Un-reserve 1 eb and 1 maf
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveEB(process);
            unreserveMAF(process);
            unreserveRequestOut(process, 1);
            // We need to send a snoop and a reply
            // Create a MAF entry
            break;
        case eReply:
        case eReplyAndRemoveMAF:
            // Un-reserve 2 snoops, 1 eb and 1 maf
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            break;
        case eStall:
            // Un-reserve 2 snoops, 1 reply, 1 eb and 1 maf
            if (process->getSnoopTransports().size() > 0) {
                DBG_(Crit,
                     (<< theName << " Process stalled while holding SnoopOut reservations: "
                      << *(process->transport()[MemoryMessageTag])));
            }
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            dumpMAFEntries(msg_a);
            break;

            // Actions for Evict Messages (now come in on Request channel, still sent
            // out on Snoop Channel (no, I don't know why))
        case eFwdAndWakeEvictMAF:
        case eForward:
            unreserveReplyOut(process, 1);
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            // Send a snoop message
            break;

        case eFwdAndReply:
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            break;

        case eWakeEvictMAF:
        case eNoAction:
            // Un-reserve 2 snoops, 1 request, 1 reply, 1 eb and 1 maf
            unreserveReplyOut(process, 1);
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            unreserveMAF(process);
            break;
            // End evict actions

        default: DBG_Assert(false, (<< "Unexpected action type for Request process: " << process->action())); break;
    }

    DBG_(VVerb,
         (<< theName << " runRequestProcess: " << *process << " for " << *(process->transport()[MemoryMessageTag])));

    // Either put into the directory pipeline, or finalize the process
    if (process->lookupRequired()) {
        theDirTagPipeline.enqueue(process);
    } else {
        finalizeProcess(process);
    }
}

// Determine action for Snoop Process and enqueue in directory pipeline
void
CMPCacheController::runSnoopProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " runSnoopProcess: " << *process->transport()[MemoryMessageTag]));
    thePolicy->handleSnoop(process);

    switch (process->action()) {
        case eFwdAndWakeEvictMAF:
        case eForward:
            // Un-reserve 1 reply
            unreserveSnoopOut(process, 2 - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveMAF(process);
            // Send a snoop message
            break;
        case eReply:
            // Un-reserve 1 snoop
            unreserveSnoopOut(process, 2);
            unreserveMAF(process);
            // Send a reply message (mimic a reply that would have been sent if the
            // evict hadn't happened)
            break;

        case eWakeEvictMAF:
        case eNoAction:
            // Un-reserve 1 snoop and 1 reply
            unreserveSnoopOut(process, 2);
            unreserveReplyOut(process, 1);
            unreserveMAF(process);
            break;

        default: DBG_Assert(false, (<< "Unexpected action type for Snoop process: " << process->action())); break;
    }

    // Either put into the directory pipeline, or finalize the process
    if (process->lookupRequired()) {
        theDirTagPipeline.enqueue(process);
    } else {
        finalizeProcess(process);
    }
}

// Determine action for Reply Process and enqueue in directory pipeline
void
CMPCacheController::runReplyProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " runReplyProcess: " << *process->transport()[MemoryMessageTag]));
    thePolicy->handleReply(process);

    switch (process->action()) {
        case eForward:
        case eFwdAndWakeEvictMAF: unreserveReplyOut(process, 1); break;
        case eReply:
        case eReplyAndRemoveMAF:
            // Un-reserve 1 snoop
            unreserveSnoopOut(process, 1);
            unreserveEB(process); // Make sure we get rid of any EB reservations we
                                  // might have gotten from the MAF
            // Send a reply message (mimic a reply that would have been sent if the
            // evict hadn't happened)
            break;
        case eRemoveAndWakeMAF:
        case eRemoveMAF:
        case eWakeEvictMAF:
        case eNoAction:
            // Un-reserve 1 snoop
            unreserveSnoopOut(process, 1);
            unreserveReplyOut(process, 1);
            unreserveEB(process); // Make sure we get rid of any EB reservations we
                                  // might have gotten from the MAF
            break;
        default: DBG_Assert(false, (<< "Unexpected action type for Reply process: " << process->action())); break;
    }
    DBG_(VVerb, (<< theName << " runReplyProcess: " << *process << " FOR " << *process->transport()[MemoryMessageTag]));

    // Either put into the directory pipeline, or finalize the process
    if (process->lookupRequired()) {
        theDirTagPipeline.enqueue(process);
    } else {
        finalizeProcess(process);
    }
}

// Determine action for Cache Evict Process
void
CMPCacheController::runCacheEvictProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " runEvictProcess: " << *process->transport()[MemoryMessageTag]));
    thePolicy->handleCacheEvict(process);

    DBG_Assert(process->action() == eForward, (<< "Unexpected action type for Evict process: " << process->action()));

    // Send out a snoop
    // No pipeline, we don't need the directory pipeline (we already used the MAF
    // pipeline)
    finalizeProcess(process);
}

// Determine action for Dir Evict Process
void
CMPCacheController::runDirEvictProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " runEvictProcess: " << *process->transport()[MemoryMessageTag]));
    thePolicy->handleDirEvict(process);

    DBG_Assert(process->action() == eForward, (<< "Unexpected action type for Evict process: " << process->action()));

    // Send out a snoop
    // Evictions come from evict buffer, accessed in parallel with MAF pipeline
    finalizeProcess(process);
}

// Determine action for IdleWork Process
void
CMPCacheController::runIdleWorkProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< theName << " runIdleWorkProcess: " << *process->transport()[MemoryMessageTag]));
    thePolicy->handleIdleWork(process);

    // Remove any remaining EB reservations
    unreserveEB(process);

    if (process->action() == eNoAction) {
        unreserveSnoopOut(process, 1);
    } else {
        DBG_Assert(process->action() == eForward,
                   (<< "Unexpected action type for IdleWork process: " << process->action()));
    }

    theIdleWorkScheduled = false;

    // Let the IdleWork process use either
    // Either put into the directory pipeline, or finalize the process
    if (process->lookupRequired()) {
        theDirTagPipeline.enqueue(process);
    } else {
        finalizeProcess(process);
    }
}

// Determine action for WakeMAFProcess and enqueue in directory pipeline
void
CMPCacheController::runWakeMAFProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();
    thePolicy->handleWakeMAF(process);

    switch (process->action()) {
        case eFwdAndWaitAck:
            // Un-reserve 1 snoop, 1 reply, 1 eb
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            // We need to forward a snoop message somewhere
            // Update MAF entry
            // Then enqueue in the Directory Pipeline
            break;
        case eNotifyAndWaitAck:
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            // Un-reserve 1 eb
            unreserveEB(process);
            // We need to send a snoop and a reply
            // Update MAF entry
            // Enqueue in the Directory Pipeline
            break;
        case eReply:
        case eReplyAndRemoveMAF:
            // Un-reserve 2 snoops, 1 eb
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            // Update MAF entry (will be removed when we leave the pipeline)
            break;
        case eRemoveMAF:
            // Remvoe the MAF now
            thePolicy->MAF().remove(process->maf());
            process->setAction(eNoAction);
            // Fall through!
        case eStall:
        case eNoAction:
            // Un-reserve 2 snoops, 1 reply, 1 eb
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveReplyOut(process, 1);
            unreserveEB(process);
            unreserveRequestOut(process, 1);
            // Update MAF entry that will be woken up later
            break;

            // Actions for Evict Messages (now come in on Request channel, still sent
            // out on Snoop Channel (no, I don't know why))
        case eFwdAndWakeEvictMAF:
        case eForward:
            unreserveReplyOut(process, 1);
            unreserveSnoopOut(process, theMaxSnoopsPerRequest - process->getSnoopTransports().size());
            unreserveRequestOut(process, 1);
            unreserveEB(process);
            break;

        default: DBG_Assert(false, (<< "Unexpected action type for WakeMAF process: " << *process)); break;
    }

    DBG_(VVerb,
         (<< theName << " runWakeMAFProcess: " << *process << " FOR " << *process->transport()[MemoryMessageTag]));

    // Either put into the directory pipeline, or finalize the process
    if (process->lookupRequired()) {
        theDirTagPipeline.enqueue(process, process->requiredLookups());
    } else {
        finalizeProcess(process);
    }
}

// Take final process actions, including removing MAF entry
// and sending final messages
void
CMPCacheController::finalizeProcess(ProcessEntry_p process)
{
    FLEXUS_PROFILE();

    DBG_(VVerb, (<< "Finalize process: " << *process));
    auto req   = process->transport()[MemoryMessageTag];
    auto msg_a = req->address();

    switch (process->action()) {
        case eFwdAndWaitAck:
            DBG_Assert(process->getSnoopTransports().size() != 0);
            unreserveSnoopOut(process, process->getSnoopTransports().size());
            sendSnoops(process);
            break;
        case eFwdRequest:
            unreserveRequestOut(process, 1);
            sendRequest(process);
            break;
        case eNotifyAndWaitAck:
            DBG_Assert(process->getSnoopTransports().size() != 0);
            unreserveSnoopOut(process, process->getSnoopTransports().size());
            sendSnoops(process);
            unreserveReplyOut(process, 1);
            DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != nullptr,
                       (<< "No reply msg for " << *(process->transport()[MemoryMessageTag])));
            sendReply(process);
            break;
        case eFwdAndReply:
            DBG_Assert(process->getSnoopTransports().size() != 0);
            unreserveSnoopOut(process, process->getSnoopTransports().size());
            sendSnoops(process);
            unreserveReplyOut(process, 1);
            DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != nullptr,
                       (<< "No reply msg for " << *(process->transport()[MemoryMessageTag])));
            sendReply(process);
            break;
        case eFwdAndWakeEvictMAF:
            DBG_Assert(process->getSnoopTransports().size() != 0);
            unreserveSnoopOut(process, process->getSnoopTransports().size());
            sendSnoops(process);
            DBG_(VVerb,
                 (<< theName << " Waking MAF Waiting on Evict: " << *(process->maf()->transport()[MemoryMessageTag])));
            thePolicy->MAF().wakeAfterEvict(process->maf());
            break;
        case eWakeEvictMAF:
            DBG_(VVerb,
                 (<< theName << " Waking MAF Waiting on Evict: " << *(process->maf()->transport()[MemoryMessageTag])));
            thePolicy->MAF().wakeAfterEvict(process->maf());
            break;
        case eForward:
            DBG_Assert(process->getSnoopTransports().size() != 0);
            unreserveSnoopOut(process, process->getSnoopTransports().size());
            sendSnoops(process);
            break;
        case eReply:
            unreserveReplyOut(process, 1);
            DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != nullptr,
                       (<< "No reply msg for " << *(process->transport()[MemoryMessageTag])));
            sendReply(process);
            break;
        case eReplyAndRemoveMAF:
            unreserveReplyOut(process, 1);
            DBG_Assert(process->getReplyTransport()[MemoryMessageTag] != nullptr,
                       (<< "No reply msg for " << *(process->transport()[MemoryMessageTag])));
            sendReply(process);
            // thePolicy->MAF().removeFirst(process->transport()[MemoryMessageTag]->address(),
            // process->transport()[DestinationTag]->requester);
            thePolicy->MAF().remove(process->maf());
            thePolicy->wakeMAFs(msg_a);
            break;
        case eRemoveMAF: thePolicy->MAF().remove(process->maf()); break;
        case eRemoveAndWakeMAF:
            DBG_Assert(process->transport()[MemoryMessageTag]);
            DBG_Assert(process->transport()[DestinationTag]);
            // thePolicy->MAF().removeFirst(process->transport()[MemoryMessageTag]->address(),
            // process->transport()[DestinationTag]->requester);
            thePolicy->MAF().remove(process->maf());
            thePolicy->wakeMAFs(msg_a);
            break;
        case eStall:
        case eNoAction: break;
    }

    DBG_Assert(process->getSnoopTransports().size() == 0);
    DBG_Assert(process->noReservations(), (<< "Process finalized while holding reservations: " << *process));
}

void
CMPCacheController::sendSnoops(ProcessEntry_p process)
{
    std::list<MemoryTransport>& snoops = process->getSnoopTransports();
    DBG_(VVerb, (<< theName << " enqueueing " << snoops.size() << " snoop transports."));
    while (!snoops.empty()) {
        DBG_(VVerb, (<< theName << " enqueuing Snoop msg: " << *(snoops.front()[MemoryMessageTag])));
        SnoopOut.enqueue(snoops.front());
        snoops.pop_front();
    }
}

void
CMPCacheController::sendReply(ProcessEntry_p process)
{
    ReplyOut.enqueue(process->getReplyTransport());
}

void
CMPCacheController::sendRequest(ProcessEntry_p process)
{
    RequestOut.enqueue(process->transport());
}

}; // namespace nCMPCache
