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

#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/none.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

struct ReadRegisterAction : public BaseSemanticAction
{
    eOperandCode theRegisterCode;
    eOperandCode theOperandCode;
    bool theConnected;
    bool the64;
    bool theSP;

    ReadRegisterAction(SemanticInstruction* anInstruction,
                       eOperandCode aRegisterCode,
                       eOperandCode anOperandCode,
                       bool aSP,
                       bool is64)
      : BaseSemanticAction(anInstruction, 1)
      , theRegisterCode(aRegisterCode)
      , theOperandCode(anOperandCode)
      , theConnected(false)
      , the64(is64)
      , theSP(aSP)
    {
    }

    bool bypass(register_value aValue)
    {
        theSP = false;

        if (!theSP) {
            if (cancelled() || theInstruction->isRetired() || theInstruction->isSquashed()) { return true; }

            if (!signalled()) {
                aValue = (uint64_t)(boost::get<uint64_t>(aValue) & (the64 ? -1LL : 0xFFFFFFFF));
                theInstruction->setOperand(theOperandCode, aValue);
                DBG_(
                  VVerb,
                  (<< *this << " bypassed " << theRegisterCode << " = " << aValue << " written to " << theOperandCode));
                satisfyDependants();
                setReady(0, true);
            }
            return false;
        }
        return true;
    }

    void doEvaluate()
    {
        DBG_(Iface, (<< *this));

        theSP = false;

        register_value aValue;
        uint64_t val;
        if (theSP) {
            if (core()->_PSTATE().SP() == 0) {
                val = core()->getSP_el(EL0);
            } else {
                switch (core()->_PSTATE().EL()) {
                    case EL0: val = core()->getSP_el(EL0); break;
                    case EL1: val = core()->getSP_el(EL1); break;
                    case EL2:
                    case EL3:
                    default:
                        DBG_Assert(false);
                        //                    theInstruction->setWillRaise(kException_ILLEGALSTATE);
                        //                    core()->takeTrap(boost::intrusive_ptr<Instruction>(theInstruction),
                        //                    theInstruction->willRaise());
                        break;
                }
            }
        } else {
            if (!theConnected) {

                mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
                setReady(0,
                         core()->requestRegister(name, theInstruction->makeInstructionDependance(dependance())) ==
                           kReady);
                core()->connectBypass(name, theInstruction, ll::bind(&ReadRegisterAction::bypass, this, ll::_1));
                theConnected = true;
            }
            if (!signalled()) {
                SEMANTICS_DBG("Signalling");
                mapped_reg name        = theInstruction->operand<mapped_reg>(theRegisterCode);
                eResourceStatus status = core()->requestRegister(name);

                if (status == kReady) {
                    aValue = core()->readRegister(name);
                    val    = boost::get<uint64_t>(aValue);
                } else {
                    setReady(0, false);
                    squashDependants();
                    DBG_(Iface, (<< "Skipping adding reg by using set ready: " <<  status << " ready was set to: " << ready() << "address of instruction is: " << theInstruction << " and code for instruction of "  << theInstruction->instCode() << " the address of the action is: " << this));
                    return;
                }
            } else {
                DBG_(Iface, (<< "Just skipping without using set ready: "  << " address of instruction is: " << theInstruction << " and code for instruction of "  << theInstruction->instCode() << " the address of the action is: " << this));
                return;
            }
        }

        if (!the64) { val &= 0xffffffff; }
        aValue = val;

        DBG_(Iface, (<< "Reading register " << theRegisterCode << " with a value " << std::hex << aValue << std::dec));

        theInstruction->setOperand(theOperandCode, val);
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ReadRegister " << theRegisterCode << " store in "
                  << theOperandCode;
    }
};

simple_action
readRegisterAction(SemanticInstruction* anInstruction,
                   eOperandCode aRegisterCode,
                   eOperandCode anOperandCode,
                   bool aSP,
                   bool is64)
{
    ReadRegisterAction* act = new ReadRegisterAction(anInstruction, aRegisterCode, anOperandCode, aSP, is64);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
