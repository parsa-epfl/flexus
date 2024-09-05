//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#include "coreModelImpl.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

std::ostream&
operator<<(std::ostream& anOstream, eQueue aCode)
{
    const char* codes[] = { "LSQ", "SSB", "SB" };
    if (aCode >= kLastQueueType) {
        anOstream << "Invalid Queue Code(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << codes[aCode];
    }
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, eStatus aCode)
{
    const char* codes[] = { "Complete",     "Annulled",        "IssuedToMemory", "AwaitingIssue",
                            "AwaitingPort", "AwaitingAddress", "AwaitingValue" };
    anOstream << codes[aCode];
    return anOstream;
}

void
MemQueueEntry::describe(std::ostream& anOstream) const
{
    if (theOperation == kMEMBARMarker) {
        anOstream << theQueue << "(" << status() << ")"
                  << "[" << theSequenceNum << "] " << theOperation;
    } else {
        anOstream << theQueue << "(" << status() << ")"
                  << "[" << theSequenceNum << "] " << theOperation << "(" << static_cast<int>(theSize) << ") "
                  << theVaddr << " " << thePaddr;

        if (theValue) { anOstream << " =" << std::hex << *theValue; }
        if (theExtendedValue) { anOstream << " x=" << *theExtendedValue; }

        if (theAnnulled) { anOstream << " {annulled}"; }
        if (theStoreComplete) { anOstream << " {store-complete}"; }
        if (thePartialSnoop) { anOstream << " {partial-snoop}"; }
        if (theException != kException_None) { anOstream << " {raise " << theException << " }"; }
        if (theSideEffect) { anOstream << " {side-effect}"; }
        if (theSpeculatedValue) { anOstream << " {value-speculation}"; }
        if (theBypassSB) { anOstream << " {bypass-SB}"; }
        //    if (theMMU) {
        //      anOstream << " {mmu}";
        //    }a
        if (theNonCacheable) { anOstream << " {non-cacheable}"; }
        if (theExtraLatencyTimeout) { anOstream << " {completes at " << *theExtraLatencyTimeout << "}"; }
    }
    anOstream << " {" << *theInstruction << "}";
}
std::ostream&
operator<<(std::ostream& anOstream, MemQueueEntry const& anEntry)
{
    anEntry.describe(anOstream);
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, MSHR const& anMSHR)
{
    anOstream << "MSHR " << anMSHR.theOperation << " " << anMSHR.thePaddr << "[";
    std::for_each(anMSHR.theWaitingLSQs.begin(), anMSHR.theWaitingLSQs.end(), ll::var(anOstream) << *ll::_1);
    anOstream << "]";
    return anOstream;
}

void
CoreImpl::dumpROB()
{
    if (!theROB.empty()) {
        DBG_(VVerb, (<< theName << "*** ROB Contents ***"));
        rob_t::iterator iter, end;
        for (iter = theROB.begin(), end = theROB.end(); iter != end; ++iter) {
            DBG_(VVerb, (/*<< std::internal*/ << **iter));
        }
    }
}

void
CoreImpl::dumpSRB()
{
    if (!theSRB.empty()) {
        DBG_(VVerb, (<< theName << "*** SRB Contents ***"));
        rob_t::iterator iter, end;
        for (iter = theSRB.begin(), end = theSRB.end(); iter != end; ++iter) {
            DBG_(VVerb, (/*<< std::internal*/ << **iter));
        }
    }
}

void
CoreImpl::dumpMemQueue()
{
    if (!theMemQueue.empty()) {
        DBG_(VVerb, (<< theName << "*** MemQueue Contents ***"));
        memq_t::iterator iter, end;
        for (iter = theMemQueue.begin(), end = theMemQueue.end(); iter != end; ++iter) {
            DBG_(VVerb, (<< *iter));
        }
    }
}

void
CoreImpl::dumpCheckpoints()
{
    if (!theCheckpoints.empty()) {
        DBG_(VVerb, (<< theName << "*** Checkpoints ***"));
        std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator iter, end;
        for (iter = theCheckpoints.begin(), end = theCheckpoints.end(); iter != end; ++iter) {
            DBG_(VVerb, (<< "   " << *iter->first));
            DBG_(VVerb, (<< "   Lost Permissions: " << iter->second.theLostPermissionCount));
            DBG_(VVerb, (<< "   Required Addresses"));
            std::map<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>>::iterator req =
              iter->second.theRequiredPermissions.begin();
            while (req != iter->second.theRequiredPermissions.end()) {
                DBG_(VVerb, (<< "        " << req->first << "\t" << *req->second));
                ++req;
            }
            DBG_(VVerb, (<< "  Held Addresses "));
            std::set<PhysicalMemoryAddress>::iterator held = iter->second.theHeldPermissions.begin();
            while (held != iter->second.theHeldPermissions.end()) {
                DBG_(VVerb, (<< "        " << *held));
                ++held;
            }
        }
    }
}

void
CoreImpl::dumpSBPermissions()
{
    if (!theSBLines_Permission.empty()) {
        DBG_(VVerb, (<< theName << "*** SB Line Tracker ***"));
        std::map<PhysicalMemoryAddress, std::pair<int, bool>>::iterator iter, end;
        for (iter = theSBLines_Permission.begin(), end = theSBLines_Permission.end(); iter != end; ++iter) {
            DBG_(VVerb,
                 (<< "   " << iter->first << " SB: " << iter->second.first << " onchip: " << iter->second.second));
        }
    }
}

void
CoreImpl::dumpMSHR()
{
    if (!theMSHRs.empty()) {
        DBG_(VVerb, (<< theName << "*** MSHR Contents *** " << theMSHRs.size()));
        MSHRs_t::iterator iter, end;
        for (iter = theMSHRs.begin(), end = theMSHRs.end(); iter != end; ++iter) {
            DBG_(VVerb, (<< " " << iter->second));
        }
    }
}

void
CoreImpl::dumpActions()
{

    //    action_list_t::iterator iter, end;

    //    if ( ! theRescheduledActions.empty() ) {
    //      DBG_( VVerb, ( << " *** Rescheduled SemanticActions ***" ) );
    //      for ( iter = theRescheduledActions.begin(), end =
    //      theRescheduledActions.end(); iter != end; ++iter) {
    //        DBG_( VVerb, ( << **iter ) );
    //      }
    //    }
}

void
CoreImpl::printROB()
{
    std::cout << theName << " *** ROB Contents ***" << std::endl;
    if (!theROB.empty()) {
        rob_t::iterator iter, end;
        for (iter = theROB.begin(), end = theROB.end(); iter != end; ++iter) {
            //   std::cout << **iter << " NPC=" << (*iter)->npc() << std::endl;
        }
    }
}

void
CoreImpl::printSRB()
{
    std::cout << theName << " *** SRB Contents ***" << std::endl;
    if (!theSRB.empty()) {
        rob_t::iterator iter, end;
        for (iter = theSRB.begin(), end = theSRB.end(); iter != end; ++iter) {
            std::cout << **iter << std::endl;
        }
    }
}

void
CoreImpl::printMemQueue()
{
    std::cout << theName << " *** MemQueue Contents ***" << std::endl;
    if (!theMemQueue.empty()) {
        memq_t::iterator iter, end;
        for (iter = theMemQueue.begin(), end = theMemQueue.end(); iter != end; ++iter) {
            std::cout << *iter << std::endl;
        }
    }
    if (theMemQueue.front().theInstruction->hasCheckpoint()) {
        std::map<boost::intrusive_ptr<Instruction>, Checkpoint>::iterator ckpt =
          theCheckpoints.find(theMemQueue.front().theInstruction);
        std::cout << theName << " *** Head Checkpoint ***" << std::endl;
        std::cout << "  Lost Permissions: " << ckpt->second.theLostPermissionCount << std::endl;
        std::cout << "  Required Addresses" << std::endl;
        std::map<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>>::iterator req =
          ckpt->second.theRequiredPermissions.begin();
        while (req != ckpt->second.theRequiredPermissions.end()) {
            std::cout << "        " << req->first << "\t" << *req->second << std::endl;
            ++req;
        }
        std::cout << "  Held Addresses " << std::endl;
        std::set<PhysicalMemoryAddress>::iterator held = ckpt->second.theHeldPermissions.begin();
        while (held != ckpt->second.theHeldPermissions.end()) {
            std::cout << "        " << *held << std::endl;
            ++held;
        }
    }
}

void
CoreImpl::printMSHR()
{
    std::cout << theName << " *** MSHR Contents *** " << std::endl;
    if (!theMSHRs.empty()) {
        MSHRs_t::iterator iter, end;
        for (iter = theMSHRs.begin(), end = theMSHRs.end(); iter != end; ++iter) {
            std::cout << " " << iter->second << std::endl;
        }
    }
    std::cout << theName << " *** Outstanding prefetches *** " << std::endl;
    if (!theOutstandingStorePrefetches.empty()) {
        std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter, end;
        for (iter = theOutstandingStorePrefetches.begin(), end = theOutstandingStorePrefetches.end(); iter != end;
             ++iter) {
            std::cout << "   " << iter->first << std::endl;
            std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
            for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end; ++insn_iter) {
                std::cout << "         " << **insn_iter << std::endl;
            }
        }
    }
#ifdef VALIDATE_STORE_PREFETCHING
    std::cout << theName << " *** Waiting prefetches *** " << std::endl;
    if (!theWaitingStorePrefetches.empty()) {
        std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter, end;
        for (iter = theWaitingStorePrefetches.begin(), end = theWaitingStorePrefetches.end(); iter != end; ++iter) {
            std::cout << "   " << iter->first << std::endl;
            std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
            for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end; ++insn_iter) {
                std::cout << "         " << **insn_iter << std::endl;
            }
        }
    }
    std::cout << theName << " *** Blocked prefetches *** " << std::endl;
    if (!theBlockedStorePrefetches.empty()) {
        std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction>>>::iterator iter, end;
        for (iter = theBlockedStorePrefetches.begin(), end = theBlockedStorePrefetches.end(); iter != end; ++iter) {
            std::cout << "   " << iter->first << std::endl;
            std::set<boost::intrusive_ptr<Instruction>>::iterator insn_iter, insn_end;
            for (insn_iter = iter->second.begin(), insn_end = iter->second.end(); insn_iter != insn_end; ++insn_iter) {
                std::cout << "         " << **insn_iter << std::endl;
            }
        }
    }
#endif // VALIDATE_STORE_PREFETCHING
    std::cout << theName << " *** Prefetch Queue *** " << std::endl;
    if (!theMemoryPortArbiter.theStorePrefetchRequests.empty()) {
        prefetch_queue_t::iterator iter, end;
        for (iter = theMemoryPortArbiter.theStorePrefetchRequests.begin(),
            end   = theMemoryPortArbiter.theStorePrefetchRequests.end();
             iter != end;
             ++iter) {
            std::cout << "   " << iter->theAge << "\t" << *iter->theInstruction << std::endl;
        }
    }
}

void
CoreImpl::printRegMappings(std::string aRegSet)
{
    if (aRegSet == "r") {
        std::cout << theName << " *** r register mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpMappings(std::cout);
    } else if (aRegSet == "f") {
        std::cout << theName << " *** f register mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpMappings(std::cout);
    } else if (aRegSet == "ccr") {
        std::cout << theName << " *** ccr register mappings *** " << std::endl;
        //    theMapTables[ccBits]->dumpMappings(std::cout);
    } else if (aRegSet == "all") {
        std::cout << theName << " *** r register mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpMappings(std::cout);
        std::cout << theName << " *** f register mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpMappings(std::cout);
        std::cout << theName << " *** ccr register mappings *** " << std::endl;
        //    theMapTables[ccBits]->dumpMappings(std::cout);
    } else {
        std::cout << "Unknown register set " << aRegSet << std::endl;
    }
}
void
CoreImpl::printRegFreeList(std::string aRegSet)
{
    if (aRegSet == "r") {
        std::cout << theName << " *** r register free list *** " << std::endl;
        //    theMapTables[xRegisters]->dumpFreeList(std::cout);
    } else if (aRegSet == "f") {
        std::cout << theName << " *** f register free list *** " << std::endl;
        //    theMapTables[xRegisters]->dumpFreeList(std::cout);
    } else if (aRegSet == "ccr") {
        std::cout << theName << " *** ccr register free list *** " << std::endl;
        //    theMapTables[ccBits]->dumpFreeList(std::cout);
    } else if (aRegSet == "all") {
        std::cout << theName << " *** r register free list *** " << std::endl;
        //    theMapTables[xRegisters]->dumpFreeList(std::cout);
        std::cout << theName << " *** f register free list *** " << std::endl;
        //    theMapTables[xRegisters]->dumpFreeList(std::cout);
        std::cout << theName << " *** ccr register free list *** " << std::endl;
        //    theMapTables[ccBits]->dumpFreeList(std::cout);
    } else {
        std::cout << "Unknown register set " << aRegSet << std::endl;
    }
}
void
CoreImpl::printRegReverseMappings(std::string aRegSet)
{
    if (aRegSet == "r") {
        std::cout << theName << " *** r register reverse mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
    } else if (aRegSet == "f") {
        std::cout << theName << " *** f register reverse mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
    } else if (aRegSet == "ccr") {
        std::cout << theName << " *** ccr register reverse mappings *** " << std::endl;
        //    theMapTables[ccBits]->dumpReverseMappings(std::cout);
    } else if (aRegSet == "all") {
        std::cout << theName << " *** r register reverse mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
        std::cout << theName << " *** f register reverse mappings *** " << std::endl;
        //    theMapTables[xRegisters]->dumpReverseMappings(std::cout);
        std::cout << theName << " *** ccr register reverse mappings *** " << std::endl;
        //    theMapTables[ccBits]->dumpReverseMappings(std::cout);
    } else {
        std::cout << "Unknown register set " << aRegSet << std::endl;
    }
}
void
CoreImpl::printAssignments(std::string aRegSet)
{
    if (aRegSet == "r") {
        std::cout << theName << " *** r register assignments *** " << std::endl;
        //    theMapTables[xRegisters]->dumpAssignments(std::cout);
    } else if (aRegSet == "f") {
        std::cout << theName << " *** f register assignments *** " << std::endl;
        //    theMapTables[xRegisters]->dumpAssignments(std::cout);
    } else if (aRegSet == "ccr") {
        std::cout << theName << " *** ccr register assignments *** " << std::endl;
        //    theMapTables[ccBits]->dumpAssignments(std::cout);
    } else if (aRegSet == "all") {
        std::cout << theName << " *** r register assignments *** " << std::endl;
        //    theMapTables[xRegisters]->dumpAssignments(std::cout);
        std::cout << theName << " *** f register assignments *** " << std::endl;
        //    theMapTables[xRegisters]->dumpAssignments(std::cout);
        std::cout << theName << " *** ccr register assignments *** " << std::endl;
        //    theMapTables[ccBits]->dumpAssignments(std::cout);
    } else {
        std::cout << "Unknown register set " << aRegSet << std::endl;
    }
}

} // namespace nuArch
