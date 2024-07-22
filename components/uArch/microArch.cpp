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

#include "microArch.hpp"

#include "CoreModel.hpp"
#include "ValueTracker.hpp"

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include "core/debug/debug.hpp"
#include "core/stats.hpp"
#include "core/performance/profile.hpp"
#include "core/qemu/configuration_api.hpp"

#include "core/boost_extensions/padded_string_cast.hpp"
#include "core/target.hpp"
#include "core/types.hpp"
#include "components/CommonQEMU/Slices/MemOp.hpp"
#include "core/qemu/api.h"
#include "core/qemu/mai_api.hpp"

namespace API = Flexus::Qemu::API;


#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

#define MAX_CLIENT_SIZE 64

namespace Stat = Flexus::Stat;
namespace ll   = boost::lambda;

namespace nuArch {

using Flexus::Core::theFlexus;
using namespace std::chrono;

/* For A64 the FPSCR is split into two logically distinct registers,
 * FPCR and FPSR and they still use non-overlapping bits.
 */

static const int32_t kClientIPC = 1;

class microArchImpl : public microArch
{

    std::string theName;

    std::unique_ptr<CoreModel> theCore;
    int32_t theAvailableROB;
    Flexus::Qemu::Processor theCPU;
    Stat::StatCounter theResynchronizations;
    Stat::StatCounter theResyncInstructions;
    Stat::StatCounter theOtherResyncs;
    Stat::StatCounter theExceptions;
    int32_t theExceptionRaised;
    bool theBreakOnResynchronize;
    bool theDriveClients;
    Flexus::Qemu::Processor theClientCPUs[MAX_CLIENT_SIZE];
    int32_t theNumClients;
    int32_t theNode;
    std::function<void(eSquashCause)> squash;
    std::function<void(VirtualMemoryAddress)> redirect;
    std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback;
    std::function<void(bool)> signalStoreForwardingHit;
    std::function<void(int32_t)> mmuResync;

  public:
    microArchImpl(uArchOptions_t options,
                  std::function<void(eSquashCause)> _squash,
                  std::function<void(VirtualMemoryAddress)> _redirect,
                  std::function<void(boost::intrusive_ptr<BranchFeedback>)> _feedback,
                  std::function<void(bool)> _signalStoreForwardingHit,
                  std::function<void(int32_t)> _mmuResync

                  )
      : theName(options.name)
      , theCore(CoreModel::construct(options,
                                     ll::bind(&microArchImpl::advance, this, ll::_1),
                                     _squash,
                                     _redirect,
                                     _feedback,
                                     _signalStoreForwardingHit,
                                     _mmuResync))
      , theAvailableROB(0)
      , theResynchronizations(options.name + "-ResyncsCaught")
      , theResyncInstructions(options.name + "-ResyncsCaught:Instruction")
      , theOtherResyncs(options.name + "-ResyncsCaught:Other")
      , theExceptions(options.name + "-ResyncsCaught:Exception")
      , theExceptionRaised(0)
      , theBreakOnResynchronize(options.breakOnResynchronize)
      , theDriveClients(false)
      , theNumClients(0)
      , theNode(options.node)
      , squash(_squash)
      , redirect(_redirect)
      , feedback(_feedback)
      , signalStoreForwardingHit(_signalStoreForwardingHit)
      , mmuResync(_mmuResync)

    {
        theCPU = Flexus::Qemu::Processor::getProcessor(theNode);

        if (theNode == 0) { setupDriveClients(); }

        theAvailableROB = theCore->availableROB();

        resetArchitecturalState(true);

        // DBG_(Crit, (<< theName << " connected to "
        //             << theCPU.id));
        // DBG_(VVerb, (<< "CORE:  Initializing MMU "));

        if (theBreakOnResynchronize && (theNode == 0)) {
            DBG_(Crit, (<< "Simulation will stop on unexpected synchronizations"));
        }
    }

    void setupDriveClients()
    {
        for (int i = 0; i < MAX_CLIENT_SIZE; i++) {
            theClientCPUs[i] = Flexus::Qemu::Processor::getProcessor(i);
            if (!theDriveClients) { theDriveClients = true; }
        }
        theNumClients = 1;
    }

    void driveClients()
    {
        CORE_DBG(theName << " Driving " << theNumClients << " client CPUs at IPC: " << kClientIPC);
        for (int32_t i = 0; i < theNumClients; ++i) {
            for (int32_t ipc = 0; ipc < kClientIPC; ++ipc) {
                theClientCPUs[i].advance();
            }
        }
    }

    void dispatch(boost::intrusive_ptr<AbstractInstruction> anInstruction)
    {
        FLEXUS_PROFILE();
        try {
            boost::intrusive_ptr<Instruction> insn = boost::polymorphic_pointer_downcast<Instruction>(anInstruction);
            theCore->dispatch(insn);
        } catch (...) {
            DBG_(Crit, (<< "Unable to cast from AbstractInstruction to Instruction"));
        }
    }

    bool canPushMemOp() { return theCore->canPushMemOp(); }

    void pushMemOp(boost::intrusive_ptr<MemOp> op)
    {
        FLEXUS_PROFILE();
        if (op->theOperation == kLoadReply || op->theOperation == kAtomicPreloadReply) {

            bits val     = ValueTracker::valueTracker(theCPU.id()).load(theCPU.id(), op->thePAddr, op->theSize);
            op->theValue = val;
            //      }
        } else if (op->theOperation == kRMWReply || op->theOperation == kCASReply) {
            // RMW operations load int32_t theExtendedValue
            ValueTracker::valueTracker(theCPU.id()).access(theCPU.id(), op->thePAddr);
            Flexus::SharedTypes::Translation xlat;
            xlat.theVaddr        = op->theVAddr;
            xlat.theType         = Flexus::SharedTypes::Translation::eStore;
            op->theExtendedValue = theCPU.read_va(xlat.theVaddr, op->theSize);
        } else if (op->theOperation == kStoreReply && !op->theSideEffect && !op->theAtomic) {
            // Need to inform ValueTracker that this store is complete
            bits value = op->theValue;
            DBG_(VVerb, (<< "u is " << op->theValue));

            //      if (op->theReverseEndian) {
            //        value = bits(Flexus::Qemu::endianFlip(value.to_ulong(),
            //        op->theSize)); DBG_(Verb, ( << "Performing inverse endian store
            //        for addr " << std::hex << op->thePAddr << " val: " <<
            //        op->theValue << " inv: " << value << std::dec ));
            //      }
            ValueTracker::valueTracker(theCPU.id()).commitStore(theCPU.id(), op->thePAddr, op->theSize, value);
        }
        theCore->pushMemOp(op);
    }

    boost::intrusive_ptr<MemOp> popMemOp() { return theCore->popMemOp(); }

    TranslationPtr popTranslation() { return theCore->popTranslation(); }

    void pushTranslation(TranslationPtr aTranslation) { return theCore->pushTranslation(aTranslation); }

    void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker)
    {
        return theCore->markExclusiveLocal(anAddress, aSize, marker);
    }

    int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        return theCore->isExclusiveLocal(anAddress, aSize);
    }

    void clearExclusiveLocal() { return theCore->clearExclusiveLocal(); }

    bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction) { return theCore->isROBHead(anInstruction); }

    boost::intrusive_ptr<MemOp> popSnoopOp() { return theCore->popSnoopOp(); }

    int32_t availableROB() { return theAvailableROB; }

    const uint32_t core() const { return theNode; }

    bool isSynchronized() { return theCore->isSynchronized(); }

    bool isQuiesced() { return theCore->isQuiesced(); }

    bool isStalled() { return theCore->isStalled(); }

    bool isHalted() { return theCore->isHalted(); }

    int32_t iCount() { return theCore->iCount(); }

    void writePermissionLost(PhysicalMemoryAddress anAddress)
    {
        theCore->loseWritePermission(eLosePerm_Replacement, anAddress);
    }

    void skipCycle()
    {
        FLEXUS_PROFILE();
        if ((theCPU.id() == 0) && ((theFlexus->cycleCount() % 10000) == 0)) {
            time_t now = time(0);
            DBG_(Dev, (<< "Timestamp: " << ctime(&now)));
        }

        //    if (theDriveClients) {
        //      CORE_TRACE;
        //      driveClients();
        //    }

        theAvailableROB = theCore->availableROB();
        theCore->skipCycle();
    }

    virtual void issueMMU(TranslationPtr aTranslation) { theCore->issueMMU(aTranslation); }

    void cycle()
    {
        CORE_DBG("--------------START MICROARCH------------------------");

        try {

            // Record free ROB space for next cycle
            theAvailableROB = theCore->availableROB();

            eExceptionType interrupt = theCPU.has_irq() ? kException_IRQ : kException_None; // HEHE
            theCore->cycle(interrupt);

        } catch (ResynchronizeWithQemuException& e) {
            ++theResynchronizations;
            if (theExceptionRaised) {
                // DBG_( Verb, ( << "CPU[" << std::setfill('0') << std::setw(2) <<
                // theCPU.id() << "] Exception Raised: " <<
                // Flexus::Qemu::API::SIM_get_exception_name(theCPU, theExceptionRaised)
                // << "(" << theExceptionRaised << "). Resynchronizing with Simics.") );
                DBG_(Verb,
                     (<< "CPU[" << std::setfill('0') << std::setw(2) << theCPU.id()
                      << "] Exception Raised: " << theExceptionRaised << ". Resynchronizing with Simics."));
                ++theExceptions;
            } else if (e.expected) {
                DBG_(Verb,
                     (<< "CPU[" << std::setfill('0') << std::setw(2) << theCPU.id()
                      << "] Resynchronizing Instruction. Resynchronizing with Qemu."));
                ++theResyncInstructions;
            } else {
                DBG_(Verb,
                     (<< "CPU[" << std::setfill('0') << std::setw(2) << theCPU.id()
                      << "] Flexus Implementation missing/wrong. Resynchronizing with "
                         "Qemu."));
                ++theOtherResyncs;
            }

            resynchronize(e.expected);

            if (theBreakOnResynchronize) {
                DBG_(Dev,
                     (<< "CPU[" << std::setfill('0') << std::setw(2) << theCPU.id()
                      << "] Resynchronize "
                         "complete\n==================================================="
                         "=====\n"));
                //theCPU.breakSimulation(); TODO
            }
            theExceptionRaised = 0;
        }

        CORE_DBG("--------------FINISH MICROARCH------------------------");
    }

    //  void translate(boost::intrusive_ptr<Translation> aTranslation) {
    // Msutherl, Oct'18, move to direct CoreModel call
    //      theCore->translate(aTranslation);
    //  }

    //  void intermediateTranslationStep(boost::intrusive_ptr<Translation>&
    //  aTranslation) {
    //      theCore->intermediateTranslationStep(aTranslation);
    //  }

  private:
    void resynchronize(bool was_expected)
    {
        FLEXUS_PROFILE();

        DBG_(Dev, Cond(!was_expected)(<< "Unexpected! Resynchronizing..."));

        // Clear out all state in theCore
        theCore->reset();
        theAvailableROB = theCore->availableROB();

        if (theExceptionRaised) {
            squash(kException);
        } else {
            squash(kResynchronize);
        }

        resetArchitecturalState(was_expected);

        // Obtain new state from simics
        VirtualMemoryAddress redirect_address(theCPU.get_pc());
        DBG_(Dev, Cond(!was_expected)(<< "Unexpected! Redirecting to address " << redirect_address));
        redirect(redirect_address);
    }

    int32_t advance(bool count_tick = true)
    {
        CORE_TRACE;
        FLEXUS_PROFILE();
        theExceptionRaised = theCPU.advance(count_tick);
        theFlexus->watchdogReset(theCPU.id());
        return theExceptionRaised;
    }

    void resetArchitecturalState(bool was_expected)
    {
        theCore->setPC(theCPU.get_pc());
        DBG_(Dev, Cond(!was_expected)(<< "setting PC to " << std::hex << theCore->pc() << std::dec));
        fillXRegisters();
        fillVRegisters();

        mmuResync(theNode);
    }

    void resetException()
    {
        API::exception_t exp;
        // theCPU.readException(&exp); TODO
        theCore->setException(exp);
    }

    // fills/re-sets global registers to the state they are
    // in the VM (i.e. client)
    void fillXRegisters()
    {
        for (int32_t i = 0; i < 32; ++i) {
            uint64_t val = theCPU.read_register(Flexus::Qemu::API::GENERAL, (std::size_t)i);
            theCore->initializeRegister(xReg(i), val);
        }
    }
    // fills/re-sets floating point registers to the state they are
    // in the VM (i.e. client)
    void fillVRegisters()
    {
        for (int32_t i = 0; i < 32; ++i) {
            uint64_t val = theCPU.read_register(Flexus::Qemu::API::FLOATING_POINT, (std::size_t)i);
            theCore->initializeRegister(vReg(i), val);
        }
    }

    // checkpoint restore

    void testCkptRestore()
    {
        State state;
        theCore->getState(state);
        theCore->reset();
        theCore->restoreState(state);

        theAvailableROB = theCore->availableROB();
        squash(kResynchronize);

        // Obtain new state from simics
        VirtualMemoryAddress redirect_address(theCore->pc());
    }

    void printROB() { theCore->printROB(); }
    void printSRB() { theCore->printSRB(); }
    void printMemQueue() { theCore->printMemQueue(); }
    void printMSHR() { theCore->printMSHR(); }
    void printRegMappings(std::string aStr) { theCore->printRegMappings(aStr); }
    void printRegFreeList(std::string aStr) { theCore->printRegFreeList(aStr); }
    void printRegReverseMappings(std::string aStr) { theCore->printRegReverseMappings(aStr); }
    void printAssignments(std::string aStr) { theCore->printAssignments(aStr); }

    void pregsSys(State const& state)
    {
        std::cout << "          %pc                %npc                %tba        "
                     "        %cwp\n";
        std::cout << "  0x" << std::hex << std::setw(16) << std::setfill('0') << state.thePC;
        std::cout << "          %ccr               %fpsr               %fpsr       "
                     "         %pstate\n";
        std::cout << std::endl;
    }

    void pregsAll()
    {
        std::cout << "Flexus processor " << theName << std::endl;
        State state;
        theCore->getState(state);
        std::cout << "      %g (normal   )      %g (alternate)      %g (mmu      ) "
                     "     %g (interrupt)\n";
        for (int32_t i = 1; i < 8; ++i) {
            std::cout << i << " ";
            std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[i] << "  ";
            std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[8 + i] << "  ";
            std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[16 + i] << "  ";
            std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << state.theGlobalRegs[24 + i] << "  ";
            std::cout << std::endl;
        }
        pregsSys(state);
    }

    void pregs()
    {
        //    char const * reg_sets[] = { "normal   ", "alternate", "mmu      ",
        //    "interrupt" };
        std::cout << "Flexus processor " << theName << std::endl;
        State state;
        theCore->getState(state);
        for (int32_t i = 0; i < 8; ++i) {
            std::cout << i << " ";
            std::cout << std::endl;
        }
        std::cout << std::endl;

        pregsSys(state);
    }
};

std::shared_ptr<microArch>
microArch::construct(uArchOptions_t options,
                     std::function<void(eSquashCause)> squash,
                     std::function<void(VirtualMemoryAddress)> redirect,
                     std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                     std::function<void(bool)> signalStoreForwardingHit,
                     std::function<void(int32_t)> mmuResync

)
{
    return std::make_shared<microArchImpl>(options, squash, redirect, feedback, signalStoreForwardingHit, mmuResync);
}

} // namespace nuArchARM