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
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

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
using nuArch::Instruction;

struct LoadFloatingAction : public PredicatedSemanticAction
{
    eSize theSize;
    boost::optional<eOperandCode> theBypass0;
    boost::optional<eOperandCode> theBypass1;

    LoadFloatingAction(SemanticInstruction* anInstruction,
                       eSize aSize,
                       boost::optional<eOperandCode> const& aBypass0,
                       boost::optional<eOperandCode> const& aBypass1)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theSize(aSize)
      , theBypass0(aBypass0)
      , theBypass1(aBypass1)
    {
    }

    void satisfy(int32_t anArg)
    {
        BaseSemanticAction::satisfy(anArg);
        if (!cancelled() && ready() && thePredicate) {
            // Bypass
            doLoad();
        }
    }

    void predicate_on(int32_t anArg)
    {
        PredicatedSemanticAction::predicate_on(anArg);
        if (!cancelled() && ready() && thePredicate) { doLoad(); }
    }

    void doEvaluate() {}

    void doLoad()
    {
        if (ready() && thePredicate) {
            switch (theSize) {
                case kWord: {
                    bits value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
                    theInstruction->setOperand(kfResult0, value & bits(0xFFFFFFFFULL));
                    if (theBypass0) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
                        core()->bypass(name, value & bits(0xFFFFFFFFULL));
                    }

                    break;
                }
                case kDoubleWord: {
                    bits value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
                    theInstruction->setOperand(kfResult1, value & bits(0xFFFFFFFFULL));
                    theInstruction->setOperand(kfResult0, value >> 32);
                    if (theBypass1) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass1);
                        core()->bypass(name, value & bits(0xFFFFFFFFULL));
                    }
                    if (theBypass0) {
                        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
                        core()->bypass(name, value >> 32);
                    }

                    break;
                }
                default: DBG_Assert(false);
            }

            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " LoadFloatingAction"; }
};

predicated_dependant_action
loadFloatingAction(SemanticInstruction* anInstruction,
                   eSize aSize,
                   boost::optional<eOperandCode> aBypass0,
                   boost::optional<eOperandCode> aBypass1)
{
    LoadFloatingAction* act = new LoadFloatingAction(anInstruction, aSize, aBypass0, aBypass1);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

} // namespace nDecoder
