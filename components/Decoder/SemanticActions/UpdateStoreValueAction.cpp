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

#include <boost/dynamic_bitset.hpp>
#include <boost/multiprecision/cpp_int.hpp>
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

struct UpdateStoreValueAction : public PredicatedSemanticAction
{
    eOperandCode theOperandCode, theOperandCode1, theOperandCode2;
    bool isPair;

    UpdateStoreValueAction(SemanticInstruction* anInstruction, eOperandCode anOperandCode)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theOperandCode(anOperandCode)
    {
    }

    void predicate_off(int)
    {
        if (!cancelled() && thePredicate) {
            DBG_(VVerb, (<< *this << " predicated off. "));
            DBG_Assert(core());
            DBG_(VVerb, (<< *this << " anulling store"));
            core()->annulStoreValue(boost::intrusive_ptr<Instruction>(theInstruction));
            thePredicate = false;
            satisfyDependants();
        }
    }

    void predicate_on(int)
    {
        if (!cancelled() && !thePredicate) {
            DBG_(VVerb, (<< *this << " predicated on. "));
            reschedule();
            thePredicate = true;
            squashDependants();
        }
    }

    void doEvaluate()
    {
        // Should ensure that we got execution units here
        if (!cancelled()) {
            if (thePredicate && ready()) {
                uint64_t value = theInstruction->operand<uint64_t>(theOperandCode);
                DBG_(Iface, (<< *this << " updating store value=" << value));
                core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
                satisfyDependants();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " UpdateStoreValue"; }
};

struct UpdateCASValueAction : public BaseSemanticAction
{

    eOperandCode theCompareCode, theCompareCode1, theNewCode, theNewCode1;
    bool thePair;

    UpdateCASValueAction(SemanticInstruction* anInstruction, eOperandCode aCompareCode, eOperandCode aNewCode)
      : BaseSemanticAction(anInstruction, 3)
      , theCompareCode(aCompareCode)
      , theNewCode(aNewCode)
      , thePair(false)
    {
    }

    UpdateCASValueAction(SemanticInstruction* anInstruction,
                         eOperandCode aCompareCode1,
                         eOperandCode aCompareCode2,
                         eOperandCode aNewCode1,
                         eOperandCode aNewCode2,
                         bool aPair)
      : BaseSemanticAction(anInstruction, 5)
      , theCompareCode(aCompareCode1)
      , theCompareCode1(aCompareCode2)
      , theNewCode(aNewCode1)
      , theNewCode1(aNewCode2)
      , thePair(aPair)
    {
    }

    void doEvaluate()
    {
        if (ready()) {
            if (!thePair) {
                uint64_t store_value = theInstruction->operand<uint64_t>(theNewCode);
                uint64_t cmp_value   = theInstruction->operand<uint64_t>(theCompareCode);
                if ((cmp_value & (uint64_t)kMaxStore) == (uint64_t)kCheckAndStore) {
                    uint64_t result = theInstruction->operand<uint64_t>(kResult);
                    DBG_(Iface, (<< *this << " Read result as " << std::hex << result));
                    cmp_value = (result << 32) | kAlwaysStore;
                }
                DBG_(Iface, (<< *this << " updating CAS write=" << std::hex << store_value << " cmp=" << cmp_value));
                core()->updateCASValue(boost::intrusive_ptr<Instruction>(theInstruction), store_value, cmp_value);
                satisfyDependants();
            } else {
                bits store_value  = theInstruction->operand<bits>(theNewCode);
                bits store_value1 = theInstruction->operand<bits>(theNewCode1);
                bits cmp_value    = theInstruction->operand<bits>(theCompareCode);
                bits cmp_value1   = theInstruction->operand<bits>(theCompareCode1);

                bits store = concat_bits(store_value, store_value1);
                bits cmp   = concat_bits(cmp_value, cmp_value1);

                core()->updateCASValue(boost::intrusive_ptr<Instruction>(theInstruction), store, cmp);
                satisfyDependants();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " UpdateCASValue"; }
};

struct UpdateSTPValueAction : public BaseSemanticAction
{

    eOperandCode theOperand;
    UpdateSTPValueAction(SemanticInstruction* anInstruction, eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 2)
      , theOperand(anOperandCode)
    {
    }

    void doEvaluate()
    {
        if (ready()) {
            if (theInstruction->hasOperand(theOperand)) {
                bits value = theInstruction->operand<bits>(theOperand);
                DBG_(Iface, (<< *this << " updating store value " << value << " from operand " << theOperand));
                core()->updateStoreValue(boost::intrusive_ptr<Instruction>(theInstruction), value);
                satisfyDependants();
            } else {
                reschedule();
            }
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " UpdateSTPValueAction"; }
};

predicated_dependant_action
updateStoreValueAction(SemanticInstruction* anInstruction, eOperandCode data)
{
    UpdateStoreValueAction* act = new UpdateStoreValueAction(anInstruction, data);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

multiply_dependant_action
updateSTPValueAction(SemanticInstruction* anInstruction, eOperandCode data)
{
    UpdateSTPValueAction* act = new UpdateSTPValueAction(anInstruction, data);
    anInstruction->addNewComponent(act);
    std::vector<InternalDependance> dependances;
    dependances.push_back(act->dependance(0));
    dependances.push_back(act->dependance(1));

    return multiply_dependant_action(act, dependances);
}

multiply_dependant_action
updateCASValueAction(SemanticInstruction* anInstruction, eOperandCode aCompareCode, eOperandCode aNewCode)
{
    UpdateCASValueAction* act = new UpdateCASValueAction(anInstruction, aCompareCode, aNewCode);
    anInstruction->addNewComponent(act);
    std::vector<InternalDependance> dependances;
    dependances.push_back(act->dependance(0));
    dependances.push_back(act->dependance(1));
    dependances.push_back(act->dependance(2));

    return multiply_dependant_action(act, dependances);
}

multiply_dependant_action
updateCASPValueAction(SemanticInstruction* anInstruction,
                      eOperandCode aCompareCode1,
                      eOperandCode aCompareCode2,
                      eOperandCode aNewCode1,
                      eOperandCode aNewCode2)
{
    UpdateCASValueAction* act =
      new UpdateCASValueAction(anInstruction, aCompareCode1, aCompareCode2, aNewCode1, aNewCode2, true);
    anInstruction->addNewComponent(act);
    std::vector<InternalDependance> dependances;
    dependances.push_back(act->dependance(0));
    dependances.push_back(act->dependance(1));
    dependances.push_back(act->dependance(2));
    dependances.push_back(act->dependance(3));
    dependances.push_back(act->dependance(4));

    return multiply_dependant_action(act, dependances);
}

} // namespace nDecoder
