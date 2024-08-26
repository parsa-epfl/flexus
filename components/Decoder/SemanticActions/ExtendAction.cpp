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
#include "PredicatedSemanticAction.hpp"

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

struct ExtendAction : public PredicatedSemanticAction
{
    eOperandCode theRegisterCode;
    bool the64;
    std::unique_ptr<Operation> theExtendOperation;

    ExtendAction(SemanticInstruction* anInstruction,
                 eOperandCode aRegisterCode,
                 std::unique_ptr<Operation>& anExtendOperation,
                 bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theRegisterCode(aRegisterCode)
      , the64(is64)
      , theExtendOperation(std::move(anExtendOperation))
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (!signalled()) {
            SEMANTICS_DBG("Signalling");

            Operand aValue = theInstruction->operand(theRegisterCode);

            aValue = theExtendOperation->operator()({ aValue });

            bits val = boost::get<bits>(aValue);

            if (!the64) { val &= 0xffffffff; }

            theInstruction->setOperand(theRegisterCode, val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ExtendRegisterAction " << theRegisterCode;
    }
};

predicated_action
extendAction(SemanticInstruction* anInstruction,
             eOperandCode aRegisterCode,
             std::unique_ptr<Operation>& anExtendOp,
             std::vector<std::list<InternalDependance>>& opDeps,
             bool is64)
{

    ExtendAction* act = new ExtendAction(anInstruction, aRegisterCode, anExtendOp, is64);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < opDeps.size(); ++i) {
        opDeps[i].push_back(act->dependance(i));
    }
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
