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

#include <iomanip>
#include <iostream>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"
#include "RegisterValueExtractor.hpp"
#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct BitFieldAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode1, theOperandCode2;
  uint64_t theS, theR;
  uint64_t thewmask, thetmask;
  bool theExtend, the64;

  BitFieldAction(SemanticInstruction *anInstruction, eOperandCode anOperandCode1,
                 eOperandCode anOperandCode2, uint64_t imms, uint64_t immr, uint64_t wmask,
                 uint64_t tmask, bool anExtend, bool a64)
      : PredicatedSemanticAction(anInstruction, 2, true), theOperandCode1(anOperandCode1),
        theOperandCode2(anOperandCode2), theS(imms), theR(immr), thewmask(wmask), thetmask(tmask),
        theExtend(anExtend), the64(a64) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        uint64_t src = boost::get<uint64_t>(theInstruction->operand(theOperandCode1));
        uint64_t dst = boost::get<uint64_t>(theInstruction->operand(theOperandCode2));

        std::unique_ptr<Operation> ror = operation(kROR_);
        std::vector<Operand> operands = {src, theR, uint64_t(the64)};
        uint64_t res = boost::get<uint64_t>(ror->operator()(operands));

        // perform bitfield move on low bits
        uint64_t bot = (dst & ~thewmask) | (res & thewmask);
        // determine extension bits (sign, zero or dest register)
        uint64_t top_mask = the64 ? (uint64_t)-1 : 0xFFFFFFFF;
        uint64_t top = theExtend ? ((src & ((uint64_t)1 << theS)) ? top_mask : 0) : dst;
        // combine extension bits and result bits
        uint64_t result = ((top & ~thetmask) | (bot & thetmask));
        theInstruction->setOperand(kResult, result);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BitFieldAction ";
  }
};

predicated_action bitFieldAction(SemanticInstruction *anInstruction,
                                 std::vector<std::list<InternalDependance>> &opDeps,
                                 eOperandCode anOperandCode1, eOperandCode anOperandCode2,
                                 uint64_t imms, uint64_t immr, uint64_t wmask, uint64_t tmask,
                                 bool anExtend, bool a64) {
  BitFieldAction *act = new BitFieldAction(anInstruction, anOperandCode1, anOperandCode2, imms,
                                           immr, wmask, tmask, anExtend, a64);
  anInstruction->addNewComponent(act);

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }

  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
