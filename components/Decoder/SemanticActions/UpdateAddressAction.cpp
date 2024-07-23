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

#include <components/uArch/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;
using nuArch::Instruction;

struct UpdateAddressAction : public BaseSemanticAction {

  eOperandCode theAddressCode, theAddressCode2;
  bool thePair, theVirtual;

  UpdateAddressAction(SemanticInstruction *anInstruction, eOperandCode anAddressCode,
                      bool isVirtual)
      : BaseSemanticAction(anInstruction, 2), theAddressCode(anAddressCode), theVirtual(isVirtual) {
  }

  void squash(int32_t anOperand) {
    if (!cancelled()) {
      DBG_(VVerb, (<< *this << " Squashing vaddr."));
      core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction),
                           kUnresolved /*, 0x80*/);
    }
    BaseSemanticAction::squash(anOperand);
  }

  void satisfy(int32_t anOperand) {
    // updateAddress as soon as dependence is satisfied
    BaseSemanticAction::satisfy(anOperand);
    updateAddress();
  }

  void doEvaluate() {
    // Address is now updated when satisfied.
  }

  void updateAddress() {
    if (ready()) {
      DBG_(Iface, (<< "Executing " << *this));

      if (theVirtual) {
        DBG_Assert(theInstruction->hasOperand(theAddressCode));

        uint64_t addr = theInstruction->operand<uint64_t>(theAddressCode);
        if (theInstruction->hasOperand(kUopAddressOffset)) {
          uint64_t offset = theInstruction->operand<uint64_t>(kUopAddressOffset);
          DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address "
                                         << std::hex << addr << std::dec);
          addr += offset;
          theInstruction->setOperand(theAddressCode, addr);
          DECODER_DBG("final address is " << std::hex << addr << std::dec);
        } else if (theInstruction->hasOperand(kSopAddressOffset)) {
          int64_t offset = theInstruction->operand<int64_t>(kSopAddressOffset);
          DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address "
                                         << std::hex << addr << std::dec);
          addr += offset;
          theInstruction->setOperand(theAddressCode, addr);
          DECODER_DBG("final address is " << std::hex << addr << std::dec);
        }
        VirtualMemoryAddress vaddr(addr);
        core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction), vaddr);
        SEMANTICS_DBG(*this << " updating vaddr = " << vaddr);

      } else {
        DBG_Assert(false);
      }
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateAddressAction";
  }
};

multiply_dependant_action updateVirtualAddressAction(SemanticInstruction *anInstruction,
                                                     eOperandCode aCode) {
  UpdateAddressAction *act = new UpdateAddressAction(anInstruction, aCode, true);
  anInstruction->addNewComponent(act);
  std::vector<InternalDependance> dependances;
  dependances.push_back(act->dependance(0));
  dependances.push_back(act->dependance(1));
  return multiply_dependant_action(act, dependances);
}

} // namespace nDecoder
