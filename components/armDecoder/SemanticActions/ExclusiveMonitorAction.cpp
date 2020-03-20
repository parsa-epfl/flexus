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
#include <boost/lambda/bind.hpp>
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

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct ExclusiveMonitorAction : public PredicatedSemanticAction {
  eOperandCode theAddressCode, theRegisterCode;
  eSize theSize;
  boost::optional<eOperandCode> theBypass;

  ExclusiveMonitorAction(SemanticInstruction *anInstruction, eOperandCode anAddressCode,
                         eSize aSize, boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 1, true), theAddressCode(anAddressCode),
        theSize(aSize), theBypass(aBypass) {
  }

  void doEvaluate() {
    DBG_(Iface, (<< *this));
    uint64_t addr = theInstruction->operand<uint64_t>(theAddressCode);
    Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
    PhysicalMemoryAddress pAddress =
        PhysicalMemoryAddress(c->translateVirtualAddress(VirtualMemoryAddress((addr >> 6) << 6)));

    // passed &= core()->isExclusiveGlobal(pAddress, theSize);
    // passed &= core()->isExclusiveVA(VirtualMemoryAddress(addr), theSize);
    int passed = core()->isExclusiveLocal(pAddress, theSize);
    if (passed == kMonitorDoesntExist) {
      DBG_(VVerb, (<< "Recheduling: Exclusive Monitor resulted in " << passed << " for address "
                   << pAddress << ", " << *theInstruction));
      setReady(0, false);
      satisfy(0);
    } else {
      core()->markExclusiveLocal(pAddress, theSize, kMonitorDoesntExist);
      theInstruction->setOperand(kResult, (uint64_t)passed);
      DBG_(Iface, (<< "Exclusive Monitor resulted in " << passed << " for address " << pAddress
                   << ", " << *theInstruction));
      if (theBypass) {
        mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
        SEMANTICS_DBG(*this << " bypassing value=" << std::hex << passed << " to " << name);
        core()->bypass(name, (uint64_t)passed);
      }
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ExclusiveMonitor " << theAddressCode;
  }
};

predicated_action exclusiveMonitorAction(SemanticInstruction *anInstruction,
                                         eOperandCode anAddressCode, eSize aSize,
                                         boost::optional<eOperandCode> aBypass) {
  ExclusiveMonitorAction *act =
      new ExclusiveMonitorAction(anInstruction, anAddressCode, aSize, aBypass);
  anInstruction->addNewComponent(act);
  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
