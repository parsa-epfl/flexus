#include "../ValueTracker.hpp"
#include "components/uFetch/uFetchTypes.hpp"
#include "coreModelImpl.hpp"

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/CommonQEMU/TraceTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <components/Decoder/BitManip.hpp>
#include <components/Decoder/Instruction.hpp>
#include <core/debug/severity.hpp>
#include <iostream>
#include <algorithm>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

namespace nDecoder {
extern uint32_t theInsnCount;
}

namespace nuArch {

/**
 * This function is called by the Flexus simulation engine to
 * dump the PC and registers of the core to the state variable.
 */
void
CoreImpl::dumpState(Flexus::SharedTypes::CPU_State& state)
{
    state.pc = theDumpPC;
    for (int i = 0; i < 32; i++) {
        mapped_reg mreg;
        mreg.theType  = xRegisters;
        mreg.theIndex = theMapTables[0]->mapArchitectural(i);
        state.regs[i] = boost::get<uint64_t>(theRegisters.peek(mreg));
    }
}

/**
 * This function is called by the Flexus simulation engine to
 * check if the QEMU and Flexus states are the same.
 */
bool
CoreImpl::checkValidatation()
{
    CPU_State flexus_dump, qemu_dump = { 0 };

    dumpState(flexus_dump);
    Flexus::Qemu::Processor::getProcessor(theNode).dump_state(qemu_dump);

    DBG_(Dev, (<< "Trying to [VALIDATE] Flexus CORE against QEMU"));
    bool same = true;

    if (qemu_dump.pc != flexus_dump.pc) {
        DBG_(Dev,
             (<< "PC mismatch: QEMU= 0x" << std::hex << qemu_dump.pc << " Flexus= 0x" << flexus_dump.pc << std::dec));
        same = false;
    }
    for (std::size_t i{ 0 }; i < 32; i++) {
        if (qemu_dump.regs[i] != flexus_dump.regs[i]) {
            DBG_(Dev,
                 (<< "Register " << (i == 31 ? "SP" : std::to_string(i)) << " mismatch: QEMU= 0x" << std::hex
                  << qemu_dump.regs[i] << " Flexus= 0x" << flexus_dump.regs[i] << std::dec));
            same = false;
        }
    }

    return same;
}

void
CoreImpl::cycle(eExceptionType aPendingInterrupt)
{
    // qemu warmup
    if (theFlexus->cycleCount() == 1) {
        advance_fn(true);
        throw ResynchronizeWithQemuException(true, false, nullptr);
    }

    CORE_DBG("--------------START CORE------------------------");

    FLEXUS_PROFILE();

    if (aPendingInterrupt != thePendingInterrupt) {

        DBG_(Dev, (<< "Interrupting..."));

        if (aPendingInterrupt == kException_None) {
            DBG_(Dev,
                 (<< theName << " Interrupt: " << thePendingInterrupt << " pending for "
                  << (theFlexus->cycleCount() - theInterruptReceived)));
        } else {
            theInterruptReceived = theFlexus->cycleCount();
        }
        thePendingInterrupt = aPendingInterrupt;
    }

    if (nDecoder::theInsnCount > 10000ULL && (static_cast<uint64_t>(theFlexus->cycleCount() - theLastGarbageCollect) >
                                              1000ULL - nDecoder::theInsnCount / 100)) {
        DBG_(VVerb, (<< theName << "Garbage-collect count before clean:  " << nDecoder::theInsnCount));

        FLEXUS_PROFILE_N("CoreImpl::cycle() collect-dead-dependencies");
        DBG_(VVerb, (<< theName << "Garbage-collect count before clean:  " << nDecoder::theInsnCount));
        theBypassNetwork.collectAll();
        DBG_(VVerb, (<< theName << "Garbage-collect between bypass and registers:  " << nDecoder::theInsnCount));
        theRegisters.collectAll();
        DBG_(VVerb, (<< theName << "Garbage-collect count after clean:  " << nDecoder::theInsnCount));
        theLastGarbageCollect = theFlexus->cycleCount();

        if (nDecoder::theInsnCount > 1000000) {
            DBG_(VVerb,
                 (<< theName
                  << "Garbage-collect detects too many live instructions.  "
                     "Forcing resynchronize."));
            ++theResync_GarbageCollect;
            throw ResynchronizeWithQemuException(false, false, nullptr);
        }
    }

    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 3) {
        //      DBG_( VVerb, ( << "Dumping..." ) );

        dumpROB();
        dumpSRB();
        dumpMemQueue();
        dumpMSHR();
        dumpCheckpoints();
        dumpSBPermissions();
        //    dumpActions();
        //    DBG_( VVerb, ( << "Dumping done" ) );
    }

    if (theIsSpeculating) {
        CORE_DBG("Speculating");
        DBG_Assert(theCheckpoints.size() > 0);
        DBG_Assert(!theSRB.empty());
        DBG_Assert(theSBCount + theSBNAWCount > 0);
        DBG_Assert(theOpenCheckpoint != 0);
        DBG_Assert(theCheckpoints.find(theOpenCheckpoint) != theCheckpoints.end());

    } else {
        CORE_DBG("Not Speculating");

        DBG_Assert(theCheckpoints.size() == 0);
        DBG_Assert(theSRB.empty());
        DBG_Assert(theOpenCheckpoint == 0);
    }

    DBG_Assert((theSBCount + theSBNAWCount) >= 0);
    DBG_Assert((static_cast<int>(theSBLines_Permission.size())) <= theSBCount + theSBNAWCount);

    DBG_(VVerb, (<< "*** Prepare *** "));

    processMemoryReplies();
    prepareCycle();

    for (const auto& tr : thePageWalkReissues)
        issueMMU(tr);
    thePageWalkReissues.clear();

    DBG_(VVerb, (<< "*** Eval *** "));
    evaluate();

    DBG_(VVerb, (<< "*** Issue Mem *** "));

    issuePartialSnoop();
    issueStore();
    issueAtomic();
    issueAtomicSpecWrite();
    issueSpecial();
    valuePredictAtomic();
    //  checkExtraLatencyTimeout();
    resolveCheckpoint();

    DBG_(Verb, (<< "*** Arb *** "));
    arbitrate();

    if (cpuHalted) {
        int qemu_rcode = advance_fn(false); // don't count instructions in halt state
        if (qemu_rcode != QEMU_EXCP_HALTED) {
            DBG_(Dev, (<< "Core " << theNode << " leaving halt state, after QEMU sent execution code " << qemu_rcode));
            cpuHalted = false;
            throw ResynchronizeWithQemuException(true, false, nullptr);
        }

        return;
    }

    // Retire instruction from the ROB to the SRB
    retire();

    if (theRetireCount > 0) { theIdleThisCycle = false; }

    // Do cycle accounting
    completeAccounting();

    if (theTSOBReplayStalls > 0) { --theTSOBReplayStalls; }

    if (theSquashRequested) {
        DBG_(Verb, (<< " Core triggering Squash: " << theSquashReason));
        doSquash();
        squash_fn(theSquashReason);
        theSquashRequested = false;
        theIdleThisCycle   = false;
    }

    // Commit instructions the SRB and compare to simics
    commit();

    handleTrap();
    //  handlePopTL();

    if (theRedirectRequested) {
        DBG_(VVerb, (<< " Core triggering Redirect to " << theRedirectRequest));
        DBG_Assert(theRedirectRequest);
        redirect_fn(theRedirectRequest);
        thePC                = theRedirectRequest->theTarget;
        theRedirectRequested = false;
        theIdleThisCycle     = false;
    }
    if (theIdleThisCycle) {
        ++theIdleCycleCount;
    } else {
        theIdleCycleCount = 0;
        CORE_DBG(theName << " Non-Idle");
    }
    theIdleThisCycle = true;

    rob_t::iterator i;
    for (i = theROB.begin(); i != theROB.end(); ++i) {
        i->get()->decrementCanRetireCounter();
    }

    CORE_DBG("--------------FINISH CORE------------------------");
}

void
CoreImpl::prepareCycle()
{
    FLEXUS_PROFILE();
    thePreserveInteractions = false;
    if (!theRescheduledActions.empty() || !theActiveActions.empty()) { theIdleThisCycle = false; }

    std::swap(theRescheduledActions, theActiveActions);
}

void
CoreImpl::evaluate()
{
    FLEXUS_PROFILE();
    CORE_DBG("--------------START EVALUATING------------------------");

    while (!theActiveActions.empty()) {
        theActiveActions.top()->evaluate();
        theActiveActions.pop();
    }
    CORE_DBG("--------------FINISH EVALUATING------------------------");
}

void
CoreImpl::arbitrate()
{
    FLEXUS_PROFILE();
    theMemoryPortArbiter.arbitrate();
}

void
CoreImpl::satisfy(InstructionDependance const& aDep)
{
    aDep.satisfy();
}
void
CoreImpl::squash(InstructionDependance const& aDep)
{
    aDep.squash();
}
void
CoreImpl::satisfy(std::list<InstructionDependance>& dependances)
{
    std::list<InstructionDependance>::iterator iter = dependances.begin();
    std::list<InstructionDependance>::iterator end  = dependances.end();
    while (iter != end) {
        iter->satisfy();
        ++iter;
    }
}
void
CoreImpl::squash(std::list<InstructionDependance>& dependances)
{
    std::list<InstructionDependance>::iterator iter = dependances.begin();
    std::list<InstructionDependance>::iterator end  = dependances.end();
    while (iter != end) {
        iter->squash();
        ++iter;
    }
}

void
CoreImpl::spinDetect(memq_t::index<by_insn>::type::iterator iter)
{
    if (theSpinning) {
        // Check leave conditions
        bool leave = false;

        if (!leave && iter->theOperation != kLoad) { leave = true; }

        if (!leave && iter->isAbnormalAccess()) { leave = true; }

        if (!leave && theSpinMemoryAddresses.count(iter->thePaddr) == 0) { leave = true; }

        if (!leave && theSpinPCs.count(iter->theInstruction->pc()) == 0) { leave = true; }

        ++theSpinDetectCount;

        if (leave) {
            std::string spin_string                           = "Addrs {";
            std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
            while (ma_iter != theSpinMemoryAddresses.end()) {
                spin_string += std::to_string(*ma_iter) + ",";
                ++ma_iter;
            }
            spin_string += "} PCs {";
            std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
            while (va_iter != theSpinPCs.end()) {
                spin_string += std::to_string(*va_iter) + ",";
                ++va_iter;
            }
            spin_string += "}";
            DBG_(VVerb, (<< theName << " leaving spin mode. " << spin_string));

            theSpinning = false;
            theSpinCount += theSpinDetectCount;
            theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
            theSpinRetireSinceReset = 0;
            theSpinDetectCount      = 0;
            theSpinMemoryAddresses.clear();
            theSpinPCs.clear();
        }
    } else {
        // Check all reset conditions
        bool reset = false;

        if (!reset && iter->theOperation != kLoad) { reset = true; }

        if (!reset && iter->isAbnormalAccess()) { reset = true; }

        theSpinMemoryAddresses.insert(iter->thePaddr);
        if (!reset && theSpinMemoryAddresses.size() > 3) { reset = true; }

        theSpinPCs.insert(iter->theInstruction->pc());
        if (!reset && theSpinPCs.size() > 3) { reset = true; }

        ++theSpinDetectCount;
        if (reset) {
            theSpinRetireSinceReset = 0;
            theSpinDetectCount      = 0;
            theSpinMemoryAddresses.clear();
            theSpinPCs.clear();
        } else {
            // Check condition for entering spin mode
            if (theSpinDetectCount > 24 || theSpinRetireSinceReset > 99) {
                std::string spin_string                           = "Addrs {";
                std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
                while (ma_iter != theSpinMemoryAddresses.end()) {
                    spin_string += std::to_string(*ma_iter) + ",";
                    ++ma_iter;
                }
                spin_string += "} PCs {";
                std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
                while (va_iter != theSpinPCs.end()) {
                    spin_string += std::to_string(*va_iter) + ",";
                    ++va_iter;
                }
                spin_string += "}";
                DBG_(VVerb, (<< theName << " entering spin mode. " << spin_string));

                theSpinning       = true;
                theSpinStartCycle = theFlexus->cycleCount();
            }
        }
    }
}

void
CoreImpl::checkPageFault(boost::intrusive_ptr<Instruction> anInsn)
{
    memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(iter != theMemQueue.get<by_insn>().end());

    if (iter->theException != kException_None) {
        DBG_(Dev, (<< theName << " Taking MMU exception: " << iter->theException << " " << *iter));
        takeTrap(anInsn, iter->theException);
        anInsn->setAccessAddress(PhysicalMemoryAddress(0));
        //      anInsn->squash();
    }
}

void
CoreImpl::createCheckpoint(boost::intrusive_ptr<Instruction> anInsn)
{
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt;
    bool is_new;
    std::tie(ckpt, is_new) = theCheckpoints.insert(std::make_pair(anInsn, Checkpoint()));
    anInsn->setHasCheckpoint(true);
    // getv9State( ckpt->second.theState );
    getState(ckpt->second.theState);
    //  Flexus::Qemu::Processor::getProcessor( theNode )->ckptMMU();
    DBG_(Verb, (<< theName << "Collecting checkpoint for " << *anInsn));
    DBG_Assert(is_new);

    theMaxSpeculativeCheckpoints << theCheckpoints.size();
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    DBG_Assert(theAllowedSpeculativeCheckpoints == 0 ||
               (static_cast<int>(theCheckpoints.size())) <= theAllowedSpeculativeCheckpoints);
    theOpenCheckpoint         = anInsn;
    theRetiresSinceCheckpoint = 0;

    startSpeculating();
}

void
CoreImpl::freeCheckpoint(boost::intrusive_ptr<Instruction> anInsn)
{
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt = theCheckpoints.find(anInsn);
    DBG_Assert(ckpt != theCheckpoints.end());
    theCheckpoints.erase(ckpt);
    anInsn->setHasCheckpoint(false);
    if (theOpenCheckpoint == anInsn) { theOpenCheckpoint = 0; }
    checkStopSpeculating();
}

void
CoreImpl::requireWritePermission(memq_t::index<by_insn>::type::iterator aWrite)
{
    PhysicalMemoryAddress addr(aWrite->thePaddr & ~(theCoherenceUnit - 1));
    if (aWrite->thePaddr != kUnresolved) {
        std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline;
        bool is_new;
        DBG_(VVerb, (<< theName << "requireWritePermission : " << *aWrite));
        std::tie(sbline, is_new) = theSBLines_Permission.insert(std::make_pair(addr, std::make_pair(1, false)));
        if (is_new) {
            ++theSBLines_Permission_falsecount;
            DBG_Assert(theSBLines_Permission_falsecount >= 0);
            DBG_Assert(theSBLines_Permission_falsecount <= (static_cast<int>(theSBLines_Permission.size())));
        } else {
            sbline->second.first++;
        }
        if (!sbline->second.second) { requestStorePrefetch(aWrite); }
        if (theOpenCheckpoint) {
            if (theConsistencyModel == kSC || aWrite->isAtomic()) {
                theCheckpoints[theOpenCheckpoint].theRequiredPermissions.insert(
                  std::make_pair(addr, aWrite->theInstruction));
                if (sbline->second.second) { theCheckpoints[theOpenCheckpoint].theHeldPermissions.insert(addr); }
            }
        }
    }
}

void
CoreImpl::unrequireWritePermission(PhysicalMemoryAddress anAddress)
{
    PhysicalMemoryAddress addr(static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1));
    std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline = theSBLines_Permission.find(addr);
    if (sbline != theSBLines_Permission.end()) {
        --sbline->second.first;
        DBG_Assert(sbline->second.first >= 0);
        if (sbline->second.first == 0) {
            DBG_(VVerb, (<< theName << " Stop tracking: " << anAddress));
            if (!sbline->second.second) {
                --theSBLines_Permission_falsecount;
                DBG_Assert(theSBLines_Permission_falsecount >= 0);
            }

            theSBLines_Permission.erase(sbline);
        }
    }
}

void
CoreImpl::acquireWritePermission(PhysicalMemoryAddress anAddress)
{
    PhysicalMemoryAddress addr(static_cast<uint64_t>(anAddress) & ~(theCoherenceUnit - 1));
    std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline = theSBLines_Permission.find(addr);
    if (sbline != theSBLines_Permission.end()) {
        if (!sbline->second.second) {
            DBG_(VVerb, (<< theName << " Acquired write permission for address in SB: " << addr));
            --theSBLines_Permission_falsecount;
            DBG_Assert(theSBLines_Permission_falsecount >= 0);
        }
        sbline->second.second                                                  = true;
        std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt = theCheckpoints.begin();
        while (ckpt != theCheckpoints.end()) {
            if (ckpt->second.theRequiredPermissions.find(addr) != ckpt->second.theRequiredPermissions.end()) {
                ckpt->second.theHeldPermissions.insert(addr);
            }
            ++ckpt;
        }
    }
}

void
CoreImpl::loseWritePermission(eLoseWritePermission aReason, PhysicalMemoryAddress anAddress)
{
    PhysicalMemoryAddress addr(anAddress & ~(theCoherenceUnit - 1));
    std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator sbline = theSBLines_Permission.find(addr);
    if (sbline != theSBLines_Permission.end()) {
        if (sbline->second.second) {
            switch (aReason) {
                case eLosePerm_Invalidate:
                    DBG_(Verb,
                         (<< theName << " Invalidate removed write permission for line in store buffer:" << addr));
                    ++theInv_HitSB;
                    break;
                case eLosePerm_Downgrade:
                    DBG_(Verb,
                         (<< theName << " Downgrade removed write permission for line in store buffer: " << addr));
                    ++theDG_HitSB;
                    break;
                case eLosePerm_Replacement:
                    DBG_(Verb,
                         (<< theName
                          << " Replacement removed write permission for line in "
                             "store buffer:"
                          << addr));
                    ++theRepl_HitSB;
                    break;
            }
            ++theSBLines_Permission_falsecount;
            DBG_Assert(theSBLines_Permission_falsecount >= 0);
            DBG_Assert(theSBLines_Permission_falsecount <= (static_cast<int>(theSBLines_Permission.size())));
        }
        sbline->second.second = false;

        std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt = theCheckpoints.begin();
        bool requested                                                         = false;
        while (ckpt != theCheckpoints.end()) {
            if (ckpt->second.theHeldPermissions.count(addr) > 0) {
                ckpt->second.theHeldPermissions.erase(addr);
                ckpt->second.theLostPermissionCount++;

                if (ckpt->second.theLostPermissionCount > 5) {
                    // Abort if lost permission count exceeds threshold
                    if (!theAbortSpeculation || theViolatingInstruction->sequenceNo() >
                                                  ckpt->second.theRequiredPermissions[addr]->sequenceNo()) {
                        theAbortSpeculation     = true;
                        theViolatingInstruction = ckpt->second.theRequiredPermissions[addr];
                    }
                } else if (!requested) {
                    // otherwise, re-request permission
                    requestStorePrefetch(ckpt->second.theRequiredPermissions[addr]);
                }
            }
            ++ckpt;
        }
    }
}

void
CoreImpl::retireMem(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
    DBG_Assert(iter != theMemQueue.get<by_insn>().end());

    // Spin detection / control
    if (!anInsn->isAnnulled()) { spinDetect(iter); }

    DBG_Assert(iter->theQueue == kLSQ);

    if (iter->theOperation == kCAS || iter->theOperation == kRMW) {
        if (!iter->isAbnormalAccess() && iter->thePaddr != kUnresolved) {
            PhysicalMemoryAddress block_addr(static_cast<uint64_t>(iter->thePaddr) & ~(theCoherenceUnit - 1));
            addSLATEntry(block_addr, anInsn);

            // TRACE TRACKER : Notify trace tracker of store
            // uint64_t logical_timestamp = theCommitNumber + theSRB.size();
            theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/, isPrivileged(), 0);
        }

        if (iter->theOperation == kRMW) {
            ++theAtomicVal_RMWs;
            DBG_Assert(iter->theExtendedValue);
            if (*iter->theExtendedValue == 0) {
                ++theAtomicVal_RMWs_Zero;
            } else {
                ++theAtomicVal_RMWs_NonZero;
            }
        } else { /* CAS */
            ++theAtomicVal_CASs;
            DBG_Assert(iter->theExtendedValue, (<< *iter));
            DBG_Assert(iter->theCompareValue, (<< *iter));
            if (*iter->theExtendedValue == 0) {
                ++theAtomicVal_CASs_Zero;
            } else {
                ++theAtomicVal_CASs_NonZero;
            }
            if (*iter->theExtendedValue == *iter->theCompareValue) {
                ++theAtomicVal_CASs_Match;
                if (theAtomicVal_LastCASMismatch) { ++theAtomicVal_CASs_MatchAfterMismatch; }
                theAtomicVal_LastCASMismatch = false;
            } else {
                ++theAtomicVal_CASs_Mismatch;
                if (theAtomicVal_LastCASMismatch) { ++theAtomicVal_CASs_MismatchAfterMismatch; }
                theAtomicVal_LastCASMismatch = true;
            }
        }

        if (iter->theStoreComplete == true) {
            // Completed non-speculatively
            eraseLSQ(anInsn); // Will setAccessAddress
        } else {
            // Speculatively retiring an atomic
            DBG_Assert(theSpeculativeOrder);
            DBG_(VVerb, (<< theName << " Spec-Retire Atomic: " << *iter));

            // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
            // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
            theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
            --theLSQCount;
            DBG_Assert(theLSQCount >= 0);
            ++theSBCount;

            iter->theIssued = false;

            DBG_Assert(iter->thePaddr != kUnresolved);
            DBG_Assert(!iter->isAbnormalAccess());
            iter->theInstruction->setAccessAddress(iter->thePaddr);

            createCheckpoint(iter->theInstruction);
            requireWritePermission(iter);
        }
    } else if (iter->theOperation == kLoad) {
        DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
        bool speculate = false;
        if (!iter->isAbnormalAccess() && ((iter->thePaddr & ~(theCoherenceUnit - 1)) != 0)) {
            if (consistencyModel() == kSC && theSpeculativeOrder) {
                if ((!isSpeculating() && !sbEmpty()) // mandatory checkpoint
                    || (isSpeculating()              // optional checkpoint
                        && theCheckpointThreshold > 0 && theRetiresSinceCheckpoint > theCheckpointThreshold &&
                        (theAllowedSpeculativeCheckpoints == 0 ||
                         (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints)) ||
                    (isSpeculating() // Checkpoint may not require more than 15 stores
                     && theCheckpoints[theOpenCheckpoint].theRequiredPermissions.size() > 15 &&
                     (theAllowedSpeculativeCheckpoints == 0 ||
                      (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints))) {
                    speculate = true;
                    // Must enter speculation mode to retire load past store
                    DBG_(Verb, (<< theName << " Entering SC speculation on: " << *iter));
                    iter->theInstruction->setMayCommit(false);

                    // Move the MEMBAR to the SSB and update counts
                    // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
                    // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
                    theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
                    --theLSQCount;
                    DBG_Assert(theLSQCount >= 0);
                    ++theSBNAWCount;
                    iter->theBypassSB = true;
                    createCheckpoint(iter->theInstruction);
                }
            }

            PhysicalMemoryAddress block_addr(static_cast<uint64_t>(iter->thePaddr) & ~(theCoherenceUnit - 1));
            addSLATEntry(block_addr, anInsn);

            // TRACE TRACKER : Notify trace tracker of load commit
            // uint64_t logical_timestamp = theCommitNumber + theSRB.size();
            theTraceTracker
              .access(theNode, eCore, iter->thePaddr, anInsn->pc(), false, false, false, isPrivileged(), 0);
            theTraceTracker.commit(theNode, eCore, iter->thePaddr, anInsn->pc(), 0);
            /* CMU-ONLY-BLOCK-BEGIN */
            //      if (theTrackParallelAccesses ) {
            //        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr,
            //        iter->theParallelAddresses);
            //      }
            /* CMU-ONLY-BLOCK-END */
        }
        if (!speculate) {
            eraseLSQ(anInsn); // Will setAccessAddress
        }
    } else if (iter->theOperation == kStore) {
        if (anInsn->isAnnulled() || iter->isAbnormalAccess()) {
            //      if (! anInsn->isAnnulled()  && iter->theMMU) {
            ////        DBG_Assert(mmuASI(iter->theASI));
            //        DBG_( Verb, ( << theName << " MMU write: " << *iter ) );
            ////        Flexus::Qemu::Processor::getProcessor( theNode
            ///)->mmuWrite(iter->theVaddr/*, iter->theASI*/, *iter->theValue );
            //      }
            eraseLSQ(anInsn);
        } else {
            // See if this store is to the same cache block as the preceding store
            memq_t::index<by_seq>::type::iterator pred = theMemQueue.project<by_seq>(iter);
            if (pred != theMemQueue.begin()) {
                --pred;
                DBG_Assert(pred->theQueue != kLSQ);
                PhysicalMemoryAddress pred_aligned(pred->thePaddr & ~(theCoherenceUnit - 1));
                PhysicalMemoryAddress iter_aligned(iter->thePaddr & ~(theCoherenceUnit - 1));
                if (iter_aligned != 0 && pred_aligned == iter_aligned && pred->theOperation == kStore) {
                    // Can coalesce this with preceding SB enry
                    ++theCoalescedStores;
                    iter->theBypassSB;
                }
            }

            // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
            // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
            theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
            --theLSQCount;
            DBG_Assert(theLSQCount >= 0);

            if (iter->theBypassSB) {
                ++theSBNAWCount;
            } else {
                ++theSBCount;
            }

            // TRACE TRACKER : Notify trace tracker of store
            //      uint64_t logical_timestamp = theCommitNumber + theSRB.size();
            theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/, isPrivileged(), 0);

            requireWritePermission(iter);
        }
    } else if (iter->theOperation == kMEMBARMarker) {
        // Retiring a MEMBAR Marker
        if (sbEmpty()) {
            DBG_(Verb, (<< theName << " Non-Spec-Retire MEMBAR: " << *iter));
            iter->theInstruction->resolveSpeculation();
            eraseLSQ(anInsn);
        } else {
            DBG_(Verb, (<< theName << " Spec-Retire MEMBAR: " << *iter));

            //! sbFull() should be in the retirement constraint for MEMBARs
            DBG_Assert(!sbFull());

            // Move the MEMBAR to the SSB and update counts
            // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue =
            // kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
            theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSSB);
            --theLSQCount;
            DBG_Assert(theLSQCount >= 0);
            ++theSBCount;

            if (theConsistencyModel == kSC) {
                if (!isSpeculating()) {
                    createCheckpoint(iter->theInstruction);
                } else {
                    iter->theInstruction->resolveSpeculation(); // Will commit with its atomic sequence
                }
            } else if (theConsistencyModel == kTSO) {
                createCheckpoint(iter->theInstruction);
            } else if (theConsistencyModel == kRMO) { // FIXME
                createCheckpoint(iter->theInstruction);
            }
        }
    } else {
        DBG_Assert(false, (<< "Tried to retire non-load or non-store: " << *iter));
    }
}

bool
CoreImpl::mayRetire_MEMBARStSt() const
{
    if (consistencyModel() != kRMO) {
        return true;
    } else {
        return sbEmpty();
    }
}

bool
CoreImpl::mayRetire_MEMBARStLd() const
{
    switch (consistencyModel()) {
        case kSC:
            if (theSpeculativeOrder) {
                return !sbFull();
            } else {
                return sbEmpty();
            }
        case kTSO:
            DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
            if (theSpeculativeOrder) {
                return !sbFull() && (theAllowedSpeculativeCheckpoints == 0
                                       ? true
                                       : (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints);
            } else {
                return sbEmpty();
            }
        case kRMO:
            DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
            if (theSpeculativeOrder) {
                return !sbFull() && (theAllowedSpeculativeCheckpoints == 0
                                       ? true
                                       : (static_cast<int>(theCheckpoints.size())) < theAllowedSpeculativeCheckpoints);
            } else {
                return sbEmpty();
            }
    }

    return true;
}

bool
CoreImpl::mayRetire_MEMBARSync() const
{
    if (theSBNAWCount > 0 && theNAWWaitAtSync) { return false; }
    return mayRetire_MEMBARStLd();
}

uint32_t
CoreImpl::currentEL()
{
    return extract32(thePSTATE, 2, 2);
}

void
CoreImpl::invalidateCache(eCacheType aType, eShareableDomain aDomain, eCachePoint aPoint)
{
    DBG_Assert(false);
}

void
CoreImpl::invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, eCachePoint aPoint)
{
    DBG_Assert(false);
}

void
CoreImpl::invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize, eCachePoint aPoint)
{
    DBG_Assert(false);
}

eAccessResult
CoreImpl::accessZVA()
{
    /* We don't implement EL2, so the only control on DC ZVA is the
     * bit in the SCTLR which can prohibit access for EL0.
     */
    if (currentEL() == EL0 && !(_SCTLR(1).DZE())) { return kACCESS_TRAP; }
    return kACCESS_OK;
}

uint32_t
CoreImpl::readDCZID_EL0()
{

    int dzp_bit = 1 << 4;

    /* DZP indicates whether DC ZVA access is allowed */
    if (accessZVA() == kACCESS_OK) { dzp_bit = 0; }
    return theDCZID_EL0 | dzp_bit;
}

bool
CoreImpl::_SECURE()
{
    return true;
}

void
CoreImpl::SystemRegisterTrap(uint8_t target_el,
                             uint8_t op0,
                             uint8_t op2,
                             uint8_t op1,
                             uint8_t crn,
                             uint8_t rt,
                             uint8_t crm,
                             uint8_t dir)
{
    //    assert (target_el >= _PSTATE().EL());
    //    theException = ExceptionSyndrome(Exception_SystemRegisterTrap);
    //    exception.syndrome<21:20> = op0;
    //    exception.syndrome<19:17> = op2;
    //    exception.syndrome<16:14> = op1;
    //    exception.syndrome<13:10> = crn;
    //    exception.syndrome<9:5> = rt;
    //    exception.syndrome<4:1> = crm;
    //    exception.syndrome<0> = dir;vbar

    //    uint64_t preferred_exception_return = ThisInstrAddr();
    //    vect_offset = 0x0;

    //    if target_el == EL1 && AArch64.GeneralExceptionsToEL2() then
    //        AArch64.TakeException(EL2, exception, preferred_exception_return,
    //        vect_offset);
    //    else
    //        AArch64.TakeException(target_el, exception,
    //        preferred_exception_return, vect_offset);
    DBG_Assert(false);
}

std::ostream&
operator<<(std::ostream& anOstream, eExceptionType aCode)
{
    std::string exceptionTypes[] = {
        "Exception_UNCATEGORIZED         ", "Exception_WFX_TRAP              ", "Exception_CP15RTTRAP            ",
        "Exception_CP15RRTTRAP           ", "Exception_CP14RTTRAP            ", "Exception_CP14DTTRAP            ",
        "Exception_ADVSIMDFPACCESSTRAP   ", "Exception_FPIDTRAP              ", "Exception_CP14RRTTRAP           ",
        "Exception_ILLEGALSTATE          ", "Exception_AA32_SVC              ", "Exception_AA32_HVC              ",
        "Exception_AA32_SMC              ", "Exception_AA64_SVC              ", "Exception_AA64_HVC              ",
        "Exception_AA64_SMC              ", "Exception_SYSTEMREGISTERTRAP    ", "Exception_INSNABORT             ",
        "Exception_INSNABORT_SAME_EL     ", "Exception_PCALIGNMENT           ", "Exception_DATAABORT             ",
        "Exception_DATAABORT_SAME_EL     ", "Exception_SPALIGNMENT           ", "Exception_AA32_FPTRAP           ",
        "Exception_AA64_FPTRAP           ", "Exception_SERROR                ", "Exception_BREAKPOINT            ",
        "Exception_BREAKPOINT_SAME_EL    ", "Exception_SOFTWARESTEP          ", "Exception_SOFTWARESTEP_SAME_EL  ",
        "Exception_WATCHPOINT            ", "Exception_WATCHPOINT_SAME_EL    ", "Exception_AA32_BKPT             ",
        "Exception_VECTORCATCH           ", "Exception_AA64_BKPT             ", "Exception_IRQ",
        "Exception_None                  "
    };

    if (aCode == kException_None) {
        anOstream << "InvalidExceptionType(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << exceptionTypes[aCode];
    }
    return anOstream;
}

PSTATE
CoreImpl::_PSTATE()
{
    return PSTATE(thePSTATE);
}

SCTLR_EL
CoreImpl::_SCTLR(uint32_t anELn)
{
    DBG_Assert(anELn >= 0 || anELn <= 3);
    return Flexus::Qemu::Processor::getProcessor(theNode).read_register(Flexus::Qemu::API::SCTLR, anELn);
}

void
CoreImpl::increaseEL()
{
    uint32_t el = extract32(thePSTATE, 2, 2);
    if (el < 0 || el >= 1) DBG_Assert(false);

    el++;
    // update pstate
    thePSTATE &= 0xfffffff3;
    thePSTATE |= (el << 2);
}

void
CoreImpl::decreaseEL()
{
    uint32_t el = extract32(thePSTATE, 2, 2);
    if (el <= 0 || el > 1) DBG_Assert(false);

    el--;
    // update pstate
    thePSTATE &= 0xfffffff3;
    thePSTATE |= (el << 2);
}

void
CoreImpl::clearExclusiveLocal()
{
    theLocalExclusivePhysicalMonitor.clear();
    theLocalExclusiveVirtualMonitor.clear();
}

void
CoreImpl::clearExclusiveGlobal()
{
    GLOBAL_EXCLUSIVE_MONITOR[theNode].clear();
}

void
CoreImpl::markExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize, uint64_t marker)
{
    if (theLocalExclusiveVirtualMonitor.find(anAddress) != theLocalExclusiveVirtualMonitor.end()) {
        theLocalExclusiveVirtualMonitor[anAddress] = (marker << 8) | aSize;
        return;
    }
    if (theLocalExclusiveVirtualMonitor.size() > 0) {
        // for(auto it = theLocalExclusiveVirtualMonitor.cbegin(); it !=
        // theLocalExclusiveVirtualMonitor.cend(); ++it){
        //     DBG_(Dev, ( << "theLocalExclusiveVirtualMonitor " << it->first << " " << it->second));
        // }
        DBG_(Dev,
             (<< "Only one local exclusive tag allowed per core, clearing original! " << anAddress << ", " << marker));
        clearExclusiveLocal();
    }
    theLocalExclusiveVirtualMonitor[anAddress] = (marker << 8) | aSize;
}

void
CoreImpl::markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker)
{
    if (theLocalExclusivePhysicalMonitor.find(anAddress) != theLocalExclusivePhysicalMonitor.end()) {
        theLocalExclusivePhysicalMonitor[anAddress] = (marker << 8) | aSize;
        return;
    }
    if (theLocalExclusivePhysicalMonitor.size() > 0) {
        // for(auto it = theLocalExclusivePhysicalMonitor.cbegin(); it !=
        // theLocalExclusivePhysicalMonitor.cend(); ++it){
        //     DBG_(Dev, ( << "theLocalExclusivePhysicalMonitor " << it->first << " " << it->second));
        // }
        DBG_(Dev,
             (<< "Only one local exclusive tag allowed per core, clearing original! " << anAddress << ", " << marker));
        clearExclusiveLocal();
    }
    theLocalExclusivePhysicalMonitor[anAddress] = (marker << 8) | aSize;
}

void
CoreImpl::markExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker)
{
    GLOBAL_EXCLUSIVE_MONITOR[theNode][anAddress] = (marker << 8) | aSize;
}

int
CoreImpl::isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize)
{
    if (theLocalExclusivePhysicalMonitor.find(anAddress) == theLocalExclusivePhysicalMonitor.end())
        return kMonitorDoesntExist;
    return int(theLocalExclusivePhysicalMonitor[anAddress] >> 8);
}

int
CoreImpl::isExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize)
{
    if (GLOBAL_EXCLUSIVE_MONITOR[theNode].find(anAddress) == GLOBAL_EXCLUSIVE_MONITOR[theNode].end())
        return kMonitorDoesntExist;
    return int(GLOBAL_EXCLUSIVE_MONITOR[theNode][anAddress] >> 8);
}

int
CoreImpl::isExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize)
{
    if (theLocalExclusiveVirtualMonitor.find(anAddress) == theLocalExclusiveVirtualMonitor.end())
        return kMonitorDoesntExist;
    return int(theLocalExclusiveVirtualMonitor[anAddress] >> 8);
}

bool
CoreImpl::isROBHead(boost::intrusive_ptr<Instruction> anInstruction)
{
    return (anInstruction == theROB.front());
}

void
CoreImpl::commitStore(boost::intrusive_ptr<Instruction> anInsn)
{
    FLEXUS_PROFILE();
    memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
    if (iter != theMemQueue.get<by_insn>().end()) {
        DBG_Assert(iter->theOperation == kStore);
        DBG_Assert(iter->theQueue == kSSB);
        DBG_Assert(!anInsn->isAnnulled());
        DBG_Assert(!anInsn->isSquashed());
        DBG_Assert(!iter->isAbnormalAccess());
        DBG_Assert(iter->theValue);
        DBG_Assert(iter->thePaddr != kUnresolved);

        bits value = *iter->theValue;
        //    if (iter->theInverseEndian) {
        //      value = Flexus::Qemu::endianFlip(value, iter->theSize);
        //      DBG_(Verb, ( << theName << " Inverse endian store: " << *iter << "
        //      inverted value: " <<  std::hex << value << std::dec ) );
        //    }
        ValueTracker::valueTracker(theNode).store(theNode, iter->thePaddr, iter->theSize, value);
        // theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSB;
        // });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSB );
        theMemQueue.get<by_insn>().modify(iter, ll::bind(&MemQueueEntry::theQueue, ll::_1) = kSB);
    }
}

void
CoreImpl::retire()
{
    FLEXUS_PROFILE();
    bool stop_retire = false;

    CORE_DBG("ROB size: " << theROB.size());
    if (theROB.empty()) {
        return;
    }

    theRetireCount = 0;
    while (!theROB.empty() && !stop_retire) {

        if (!theROB.front()->mayRetire()) {
            // wfi still executing
            if (theROB.front()->getOpcode() == 0x7F2003D5) theFlexus->reset_core_watchdog(theNode);
            CORE_DBG("Cant Retire due to pending retirement dependance " << *theROB.front());
            break;
        }

        if (theTSOBReplayStalls > 0) { break; }

        // Check for sufficient speculative checkpoints
        DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
        if (theIsSpeculating && theAllowedSpeculativeCheckpoints > 0 && theROB.front()->instClass() == clsAtomic &&
            !theROB.front()->isAnnulled() &&
            (static_cast<int>(theCheckpoints.size())) >= theAllowedSpeculativeCheckpoints) {
            DBG_(VVerb, (<< " theROB.front()->instClass() == clsAtomic "));
            break;
        }

        if ((thePendingInterrupt != kException_None) && theIsSpeculating) {
            // stop retiring so we can stop speculating and handle the interrupt
            DBG_(VVerb, (<< " thePendingInterrupt && theIsSpeculating "));
            break;
        }

        if ((theROB.front()->resync() || (theROB.front() == theInterruptInstruction)) && theIsSpeculating) {
            // Do not retire sync instructions or interrupts while speculating
            break;
        }

        // Stop retirement for the cycle if we retire an instruction that
        // requires resynchronization
        if (theROB.front()->resync()) { stop_retire = true; }

        if (theSquashRequested && (theROB.begin() == theSquashInstruction)) {
            if (!theSquashInclusive) {
                ++theSquashInstruction;
                theSquashInclusive = true;
            } else {
                break;
            }
        }

        // the remaining instrs in the ROB are all invalid
        if (theROB.front()->isSquashed()) break;

        // FOR in-order SMS Experiments only - not normal in-order
        // Under theInOrderMemory, we do not allow stores or atomics to retire
        // unless the OoO core is fully idle.  This is not neccessary for loads -
        // they will only issue when they reach the MemQueue head and the core is
        // fully stalled
        // if ( theInOrderMemory && (theROB.front()->instClass() == clsStore ||
        // theROB.front()->instClass() == clsAtomic ) && ( !isFullyStalled() ||
        // theRetireCount > 0)) { break;
        //}

        theSpinRetireSinceReset++;

        CORE_DBG(theName << " Retire:" << *theROB.front());
        if (!acceptInterrupt()) {
            theROB.front()->checkTraps(); // take traps only if we don't take interrupt
        }
        if (thePendingTrap != kException_None) {
            theROB.front()->changeInstCode(codeException);
            DBG_(Verb, (<< theName << " Trap raised by " << *theROB.front()));
            stop_retire = true;
        } else {
            theROB.front()->doRetirementEffects();
            if (thePendingTrap != kException_None) {
                theROB.front()->changeInstCode(codeException);
                DBG_(Verb, (<< theName << " Trap raised by " << *theROB.front()));
                stop_retire = true;
            }
        }

        //    if (theValidateMMU) {
        //        DBG_( VVerb, ( << "Validate MMU " ));

        //      // save MMU with this instruction so we can check at commit
        //      theROB.front()->setMMU( Flexus::Qemu::Processor::getProcessor(
        //      theNode )->getMMU() );
        //    }

        accountRetire(theROB.front());

        ++theRetireCount;
        if (theRetireCount >= theRetireWidth) { stop_retire = true; }

        // Value prediction enabled after forward progress is made
        theValuePredictInhibit = false;

        thePC = theROB.front()->pc() + 4;

        CORE_DBG("Move instruction to the secondary retirement buffer " << *(theROB.front()));
        theSRB.push_back(theROB.front());

        if (thePendingTrap == kException_None) {
            theROB.pop_front();
            // Need to squash and retire instructions that cause traps
        }
    }
}

void
CoreImpl::doAbortSpeculation()
{
    DBG_Assert(theIsSpeculating);

    // Need to treat ROB head as if it retired to account for stall cycles
    // properly
    if (!theROB.empty()) { accountRetire(theROB.front()); }

    // Ensure that the violating instruction is in the SRB.
    DBG_Assert(!theSRB.empty());
    rob_t::index<by_insn>::type::iterator iter = theSRB.get<by_insn>().find(theViolatingInstruction);
    if (iter == theSRB.get<by_insn>().end()) {
        DBG_Assert(false, (<< " Violating instruction is not in SRB: " << *theViolatingInstruction));
    }

    // Locate the instruction that caused the violation and the nearest
    // preceding checkpoint
    rob_t::iterator srb_iter      = theSRB.project<0>(iter);
    rob_t::iterator srb_begin     = theSRB.begin();
    rob_t::iterator srb_ckpt      = srb_begin;
    int32_t saved_discard_count   = 0;
    int32_t ckpt_discard_count    = 0;
    int32_t remaining_checkpoints = 0;
    bool checkpoint_found         = false;
    bool done                     = false;
    do {
        if (!checkpoint_found) {
            if ((*srb_iter)->hasCheckpoint()) {
                srb_ckpt         = srb_iter;
                checkpoint_found = true;
            } else {
                ++ckpt_discard_count;
            }
        } else {
            ++saved_discard_count;
            if ((*srb_iter)->hasCheckpoint()) { ++remaining_checkpoints; }
        }
        if (srb_iter == srb_begin) {
            done = true;
        } else {
            --srb_iter;
        }
    } while (!done);

    // Account for location of checkpoint, track discarded instructions
    int32_t required = (theSRB.size() - saved_discard_count - ckpt_discard_count);
    DBG_Assert(required >= 0);
    theSavedDiscards += saved_discard_count;
    theSavedDiscards_Histogram << saved_discard_count;
    theRequiredDiscards += required;
    theRequiredDiscards_Histogram << required;
    theNearestCkptDiscards += ckpt_discard_count;
    theNearestCkptDiscards_Histogram << ckpt_discard_count;

    // Determine which checkpoint we rolled back to.  1 is the eldest,
    // 2 is the next, and so on
    theRollbackToCheckpoint << std::make_pair(remaining_checkpoints + 1LL, 1LL);

    // Determine how many checkpoints were crossed.  Minimum is 1 (rolling back
    // to the most recently created checkpoint.
    int32_t num_ckpts_discarded = theCheckpoints.size() - remaining_checkpoints;
    theCheckpointsDiscarded << std::make_pair(static_cast<int64_t>(num_ckpts_discarded), 1LL);

    // Find the checkpoint corresponding to srb_ckpt
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt;
    ckpt                 = theCheckpoints.find(*srb_ckpt);
    int64_t ckpt_seq_num = (*srb_ckpt)->sequenceNo();
    DBG_Assert(ckpt != theCheckpoints.end());

    DBG_(Dev,
         (<< theName << " Aborting Speculation.  Violating Instruction: " << *theViolatingInstruction
          << " Roll back to: " << **srb_ckpt << " Required: " << required << " Ckpt: " << ckpt_discard_count
          << " Saved: " << saved_discard_count));

    // Wipe uArch and LSQ
    // resetv9();
    resetCore();

    cleanMSHRS(ckpt_seq_num);

    // Restore checkpoint
    // restorev9State( ckpt->second.theState);
    restoreState(ckpt->second.theState);

    // restore MMU
    //  Flexus::Qemu::Processor::getProcessor( theNode
    //  )->rollbackMMUCkpts(num_ckpts_discarded - 1);

    // Clean up SRB and SSB.
    theSRB.erase(srb_ckpt, theSRB.end());
    srb_ckpt              = theSRB.end(); // srb_ckpt is no longer valid;
    int32_t remaining_ssb = clearSSB(ckpt_seq_num);

    // redirect fetch
    squash_fn(kFailedSpec);
    theRedirectRequested = true;
    DBG_Assert(ckpt->second.theState.thePC == ckpt->first->bpState()->pc);
    // theRedirectInstruction = ckpt->first;
    theRedirectRequest = boost::intrusive_ptr<BPredRedictRequest>(new BPredRedictRequest);
    theRedirectRequest->theTarget = ckpt->second.theState.thePC;
    theRedirectRequest->theBPState = ckpt->first->bpState();
    theRedirectRequest->theInsertNewHistory = false;

    // Clean up SLAT
    SpeculativeLoadAddressTracker::iterator slat_iter = theSLAT.begin();
    SpeculativeLoadAddressTracker::iterator slat_item;
    SpeculativeLoadAddressTracker::iterator slat_end = theSLAT.end();
    while (slat_iter != slat_end) {
        slat_item = slat_iter;
        ++slat_iter;
        if (slat_item->second->sequenceNo() >= ckpt_seq_num) { theSLAT.erase(slat_item); }
    }

    // Clean up checkpoints
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_iter = theCheckpoints.begin();
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_item;
    std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ck_end = theCheckpoints.end();
    theOpenCheckpoint                                                        = 0;
    while (ck_iter != ck_end) {
        ck_item = ck_iter;
        ++ck_iter;
        if (ck_item->first->sequenceNo() >= ckpt_seq_num) {
            theCheckpoints.erase(ck_item);
        } else {
            if (!theOpenCheckpoint || theOpenCheckpoint->sequenceNo() < ck_item->first->sequenceNo()) {
                theOpenCheckpoint = ck_item->first;
            }
        }
    }
    theRetiresSinceCheckpoint = 0;

    // Stop speculating
    theAbortSpeculation = false;

    theEmptyROBCause = kFailedSpeculation;
    theIsSpeculating = !theCheckpoints.empty();
    accountAbortSpeculation(ckpt_seq_num);

    // Charge stall cycles for TSOB replay
    if (remaining_ssb > 0) { theTSOBReplayStalls = remaining_ssb / theNumMemoryPorts; }
}

void
CoreImpl::commit()
{
    FLEXUS_PROFILE();

    if (theAbortSpeculation) { doAbortSpeculation(); }

    while (!theSRB.empty() && (theSRB.front()->mayCommit() || theSRB.front()->isSquashed())) {

        if (theSRB.front()->hasCheckpoint()) { freeCheckpoint(theSRB.front()); }

        DBG_(VVerb, (<< theName << " FinalCommit:" << *theSRB.front()));
        if (theSRB.front()->willRaise() == kException_None) {
            theSRB.front()->doCommitEffects();
            DBG_(VVerb, (<< theName << " commit effects complete"));
        }

        theLastTrainingFeedback = nullptr;

        commit(theSRB.front());
        DBG_(VVerb, (<< theName << " committed in Qemu"));

        theSRB.pop_front();
    }
}

int s_validation = 0;
int f_validation = 0;

void
CoreImpl::commit(boost::intrusive_ptr<Instruction> anInstruction)
{
    FLEXUS_PROFILE();
    CORE_DBG(*anInstruction);
    bool validation_passed = true;
    eExceptionType raised  = kException_None;
    ;
    bool resync_accounted = false;

    if (anInstruction->advancesSimics() || anInstruction->resync()) {
        CORE_DBG("Instruction is neither annuled nor is a micro-op");

        validation_passed &= anInstruction->preValidate();
        DBG_(VVerb, (<< "Pre Validating... " << validation_passed));

        //    DBG_( VVerb, Condition(!validation_passed) ( << *anInstruction <<
        //    "Prevalidation failure." ) ); bool take_interrupt =
        //    theInterruptSignalled && (anInstruction == theInterruptInstruction);
        theInterruptSignalled   = false;
        theInterruptInstruction = 0;

        int qemu_rcode = advance_fn(true); // count time

        DBG_(Dev, (<< "c" << theNode << " commit [" << std::hex << qemu_rcode << "] " << *anInstruction << "PC:" << std::hex << anInstruction->pc() << std::dec));


        if (qemu_rcode == QEMU_EXCP_HALTED) { // QEMU CPU Halted
            /* If cpu is halted, turn off insn counting until the CPU is woken up again */
            cpuHalted = true;
            DBG_(Dev,
                 (<< "Core " << theNode << " entering halt state, after executing instruction " << *anInstruction));
            anInstruction->forceResync();
        }
        if ((qemu_rcode != (int)(kException_None)) && (qemu_rcode < QEMU_EXCP_INTERRUPT))
            raised = (eExceptionType)(qemu_rcode);

        if (raised != kException_None) {
            if (anInstruction->willRaise() != raised) { // FIXME get exception mapper
                DBG_(VVerb,
                     (<< *anInstruction
                      << " Core did not predict correct exception for this "
                         "instruction raised=0x"
                      << std::hex << raised << " will_raise=0x" << anInstruction->willRaise() << std::dec));
                if (anInstruction->instCode() != codeITLBMiss) { anInstruction->changeInstCode(codeException); }
                anInstruction->forceResync();
                resync_accounted = true;

                // TODO: what does it means
                if (raised < 0x400) {
                    ++theResync_UnexpectedException;
                } else {
                    ++theResync_Interrupt;
                }
            } else {
                DBG_(VVerb,
                     (<< *anInstruction << " Core correctly identified raise=0x" << std::hex << raised << std::dec));
            }
            anInstruction->raise(raised);
        } else if (anInstruction->willRaise() != kException_None) {
            DBG_(VVerb,
                 (<< *anInstruction << " Core predicted exception: " << std::hex << anInstruction->willRaise()
                  << " but QEMU says no exception"));
        }
    }

    accountCommit(anInstruction, raised != kException_None);

    theDumpPC = anInstruction->pcNext();
    if (anInstruction->resync()) {
        DBG_(Dev, Cond(!cpuHalted)(<< "Forced Resync:" << *anInstruction));

        // Subsequent Empty ROB stalls (until next dispatch) are the result of a
        // synchronizing instruction.
        theEmptyROBCause = kSync;
        if (!resync_accounted) { accountResyncReason(anInstruction); }
        throw ResynchronizeWithQemuException(true, true, anInstruction);
    }

    if (anInstruction->advancesSimics()) {
        validation_passed &= checkValidatation();
        validation_passed &= anInstruction->postValidate();
    }

    DBG_(VVerb, (<< "Post Validating... " << validation_passed));

    if (!validation_passed) {
        DBG_(VVerb, (<< "Failed Validated " << *anInstruction));
        DBG_(VVerb, (<< std::internal << *anInstruction << std::left));

        // Subsequent Empty ROB stalls (until next dispatch) are the result of a
        // modelling error resynchronization instruction.
        theEmptyROBCause = kResync;
        ++theResync_FailedValidation;

        throw ResynchronizeWithQemuException(true, true, anInstruction);
    }
    /* Dump PC to file if logging is enabled */
    if (collectTrace) { trace_stream << anInstruction->pc() << std::endl; }
    DBG_(VVerb, (<< "uARCH Validated "));
    DBG_(VVerb, (<< std::internal << *anInstruction << std::left));
}

bool
CoreImpl::squashFrom(boost::intrusive_ptr<Instruction> anInsn, bool inclusive)
{
    if (!theSquashRequested || (anInsn->sequenceNo() <= (*theSquashInstruction)->sequenceNo())) {
        theSquashRequested   = true;
        theSquashReason      = kBranchMispredict;
        theEmptyROBCause     = kMispredict;
        theSquashInstruction = theROB.project<0>(theROB.get<by_insn>().find(anInsn));
        theSquashInclusive   = inclusive;
        return true;
    }
    return false;
}

void
CoreImpl::redirectFetch(boost::intrusive_ptr<BPredRedictRequest> request)
{
    theRedirectRequested = true;
    theRedirectRequest = request;
}

void
CoreImpl::trainingBranch(boost::intrusive_ptr<BPredState> feedback)
{
    // Well, this training should only be called once.

    DBG_(VVerb, (<< "Training branch predictor: " << feedback->pc));

    DBG_Assert(theLastTrainingFeedback == nullptr);

    trainBP_fn(feedback);

    theLastTrainingFeedback = feedback;
}

void
CoreImpl::doSquash()
{
    FLEXUS_PROFILE();
    if (theSquashInstruction != theROB.end()) {

        // There is at least one instruction in the ROB
        rob_t::iterator erase_iter = theSquashInstruction;
        if (!theSquashInclusive) { ++erase_iter; }

        {
            FLEXUS_PROFILE_N("CoreImpl::doSquash() squash");
            rob_t::reverse_iterator iter = theROB.rbegin();
            rob_t::reverse_iterator end  = boost::make_reverse_iterator(erase_iter);
            while (iter != end) {
                (*iter)->squash();
                ++iter;
            }
        }

        if (!thePreserveInteractions) {
            FLEXUS_PROFILE_N("CoreImpl::doSquash() clean-interactions");
            // Discard any defferred interactions
            theDispatchInteractions.clear();
        }

        {
            FLEXUS_PROFILE_N("CoreImpl::doSquash() ROB-clear");
            theROB.erase(erase_iter, theROB.end());
        }
    }
}

void
CoreImpl::startSpeculating()
{
    if (!isSpeculating()) {
        DBG_(VVerb, (<< theName << " Starting Speculation"));
        theIsSpeculating    = true;
        theAbortSpeculation = false;
        accountStartSpeculation();
    }
}

void
CoreImpl::checkStopSpeculating()
{
    if (isSpeculating() && !theAbortSpeculation && theCheckpoints.size() == 0) {
        DBG_(VVerb, (<< theName << " Ending Successful Speculation"));
        theIsSpeculating = false;
        accountEndSpeculation();
    } else {
        DBG_(VVerb,
             (<< theName << " continuing speculation checkpoints: " << theCheckpoints.size() << " Abort pending? "
              << theAbortSpeculation));
    }
}

void
CoreImpl::takeTrap(boost::intrusive_ptr<Instruction> anInstruction, eExceptionType aTrapType)
{

    CORE_DBG(theName << " Take trap: " << aTrapType);

    // Ensure ROB is not empty
    DBG_Assert(!theROB.empty());
    // Only ROB head should raise
    DBG_Assert(anInstruction == theROB.front());
    anInstruction->forceResync();
    // return;

    // Clear ROB
    theSquashRequested   = true;
    theSquashReason      = kException;
    theEmptyROBCause     = kRaisedException;
    theSquashInstruction = theROB.begin();

    auto wfi = (anInstruction->getOpcode() == 0x7F2003D5) && (aTrapType & 0x80000000);

    // different from the pure emulation where the wfi instruction is nevertheless
    // executed first and the emulator then waits for the interrupt, here, we drive
    // the simulator first, waiting for the emulator to signal the interrupt.
    // this means that at the time we see the interrupt and are about to retire
    // the wfi, it is not yet executed in the emulator, neither is the required
    // pc advancement (+4) performed.
    if (wfi) {
        DBG_Assert(
          false,
          (<< "[==BRYAN==] You seems to have entered the WAIT FOR INTERRUPT (wfi) realm proceed with caution because "
              "it was NOT tested at all, and the condition where left unchanged from the RISC-V version"));
        // Flexus::Qemu::API::qemu_api.set_pc(theQEMUCPU, theROB.front()->pc() + 4);
    }

    theSquashInclusive = !wfi;

    // Record the pending trap
    thePendingTrap     = aTrapType;
    theTrapInstruction = anInstruction;

    anInstruction->setWillRaise(aTrapType);
    anInstruction->raise(aTrapType);
}

bool
CoreImpl::acceptInterrupt()
{
    if ((thePendingInterrupt != kException_None)) { DBG_(Dev, (<< "IRQ is pending...")); }

    if ((thePendingInterrupt != kException_None) // Interrupt is pending
        && !theInterruptSignalled                // Already accepted the interrupt
        && !theROB.empty()                       // Need an instruction to take the interrupt on
        && !theROB.front()->isAnnulled()         // Do not take interrupts on annulled instructions
        && !theROB.front()->isSquashed()         // Do not take interrupts on squashed instructions
        && !theROB.front()->isMicroOp()          // Do not take interrupts on squashed micro-ops
        && !theIsSpeculating                     // Do not take interrupts while speculating
        // comply with qemu that exceptions have higher priority
        && (theROB.front()->willRaise() == kException_None)) {
        // Interrupt was signalled this cycle.  Clear the ROB
        theInterruptSignalled = false;

        // theROB.front()->makePriv();

        DBG_(Dev,
             (<< theName << " Accepting interrupt " << thePendingInterrupt << " on instruction " << *theROB.front()));

        theInterruptInstruction = theROB.front();
        takeTrap(theInterruptInstruction, thePendingInterrupt);

        return true;
    }
    return false;
}

void
CoreImpl::handleTrap()
{
    if (thePendingTrap == kException_None) { return; }
    DBG_(Dev, (<< theName << " Handling trap: " << thePendingTrap << " raised by: " << *theTrapInstruction));
    DBG_(Crit, (<< theName << " ROB non-empty in handle trap.  Resynchronize instead."));
    theEmptyROBCause = kRaisedException;
    ++theResync_FailedHandleTrap;
    throw ResynchronizeWithQemuException(false, true, theTrapInstruction);
}

void
CoreImpl::valuePredictAtomic()
{
    FLEXUS_PROFILE();
    if (theSpeculateOnAtomicValue && theLSQCount > 0) {
        memq_t::index<by_queue>::type::iterator lsq_head =
          theMemQueue.get<by_queue>().lower_bound(std::make_tuple(kLSQ));
        if ((lsq_head->isAtomic()) && (!lsq_head->isAbnormalAccess()) && (!lsq_head->theExtendedValue) &&
            (!lsq_head->thePartialSnoop) && ((lsq_head->theOperation == kCAS) ? (!!lsq_head->theCompareValue) : true) &&
            (lsq_head->theInstruction == theROB.front()) &&
            (!theValuePredictInhibit) // Disables value prediction for one
                                      // instruction after a rollback
            && (lsq_head->status() == kIssuedToMemory)) {
            DBG_(VVerb, (<< theName << " Value-predicting " << *lsq_head));
            ++theValuePredictions;

            if (theSpeculateOnAtomicValuePerfect) {
                bits val = ValueTracker::valueTracker(theNode).load(theNode, lsq_head->thePaddr, lsq_head->theSize);
                lsq_head->theExtendedValue = val;
                if (val == -1) {
                    lsq_head->theInstruction->forceResync(true);
                }
            } else {
                if (lsq_head->theOperation == kCAS) {
                    lsq_head->theExtendedValue = lsq_head->theCompareValue;
                } else {
                    lsq_head->theExtendedValue = 0;
                }
            }
            if (lsq_head->theDependance) { lsq_head->theDependance->satisfy(); }
            lsq_head->theSpeculatedValue = true;
        }
    }
}

bool
CoreImpl::isIdleLoop()
{
    // /   assert(false);
    return false; // ALEX - we don't currently have WhiteBox in QEMU
                  // /*uint64_t idle =
                  // nWhiteBox::WhiteBox::getWhiteBox()->get_idle_thread_t(theNode);
                  //  mapped_reg mapped_reg;
                  // mapped_reg.theType = xRegisters;
                  // mapped_reg.theIndex = 7;  // %g7 holds current thread
                  // uint64_t ct = boost::get<uint64_t>(readArchitecturalRegister(mapped_reg,
                  // false));

    //  DBG_(Verb, ( << theName << " isIdleLoop: idle=0x" << std::hex << idle <<
    //  "== ct=0x" << ct << " : " << (ct == idle)));

    //  return ((idle != 0) && (ct == idle));
}

uint64_t
CoreImpl::pc() const
{
    // if (! theROB.empty()) {
    //   if (theROB.front()->isAnnulled() || theROB.front()->isMicroOp() ) {
    //    return theROB.front()->npc();
    //  } else {
    //    return theROB.front()->pc();
    //  }
    //  } else {
    return thePC;
    //}
}
// uint64_t CoreImpl::npc() const {
//  if (! theROB.empty()) {
//    if (theROB.front()->isAnnulled() || theROB.front()->isMicroOp() ) {
//      DBG_(VVerb, ( << "NPC= front->npc + 4" ) );
//      return theROB.front()->npc() + 4;
//    } else {
//      DBG_(VVerb, ( << "NPC= front->npcReg" ) );
//      return theROB.front()->npcReg();
//    }
//  } else if (theNPC) {
//    DBG_(VVerb, ( << "NPC= *NPC" ) );
//    return *theNPC;
//  } else {
//    if (!theDispatchInteractions.empty()) {
//      std::list< boost::intrusive_ptr< Interaction > >::const_iterator iter,
//      end; iter = theDispatchInteractions.begin(); end =
//      theDispatchInteractions.end(); uint64_t npc = 0; bool found = false;
//      while (iter != end)  {
//        DBG_(VVerb, ( << "interaction: " << **iter) );
//        if ((*iter)->npc()) {
//          npc = *((*iter)->npc());
//          found = true;
//        } else if ((*iter)->annuls()) {
//          found = false;
//          break;
//        }
//        ++iter;
//      }
//      if (found) {
//        DBG_(VVerb, ( << "NPC= from interaction " ) );
//        return npc;
//      }
//    }
//  }
//  DBG_(VVerb, ( << "NPC= PC + 4" ) );
//  return thePC + 4;
//}

} // namespace nuArch
