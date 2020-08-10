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

#ifndef FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED
#define FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iostream>

#include <core/types.hpp>

#include "OperandCode.hpp"
#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/uArchARM/RegisterType.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include "InstructionComponentBuffer.hpp"
#include "Interactions.hpp"

namespace nuArchARM {
struct uArchARM;
struct SemanticAction;
enum eAccType;
} // namespace nuArchARM

namespace narmDecoder {

using Flexus::SharedTypes::VirtualMemoryAddress;
using nuArchARM::eOperation;
using nuArchARM::eRegisterType;
using nuArchARM::eSize;
using nuArchARM::SemanticAction;
using nuArchARM::uArchARM;

struct BaseSemanticAction;
struct SemanticInstruction;
struct Condition;

struct Effect : UncountedComponent {
  Effect *theNext;
  Effect() : theNext(0) {
  }
  virtual ~Effect() {
  }
  virtual void invoke(SemanticInstruction &anInstruction) {
    if (theNext) {
      theNext->invoke(anInstruction);
    }
  }
  virtual void describe(std::ostream &anOstream) const {
    if (theNext) {
      theNext->describe(anOstream);
    }
  }
  // NOTE: No virtual destructor because effects are never destructed.
};

struct EffectChain {
  Effect *theFirst;
  Effect *theLast;
  EffectChain();
  void invoke(SemanticInstruction &anInstruction);
  void describe(std::ostream &anOstream) const;
  void append(Effect *anEffect);
  bool empty() const {
    return theFirst == 0;
  }
};

struct DependanceTarget {
  void invokeSatisfy(int32_t anArg);
  void invokeSquash(int32_t anArg) {
    squash(anArg);
  }
  virtual ~DependanceTarget() {
  }
  virtual void satisfy(int32_t anArg) = 0;
  virtual void squash(int32_t anArg) = 0;

protected:
  DependanceTarget() {
  }
};

struct InternalDependance {
  DependanceTarget *theTarget;
  int32_t theArg;
  InternalDependance() : theTarget(0), theArg(0) {
  }
  InternalDependance(InternalDependance const &id) : theTarget(id.theTarget), theArg(id.theArg) {
  }
  InternalDependance(DependanceTarget *tgt, int32_t arg) : theTarget(tgt), theArg(arg) {
  }
  void satisfy() {
    theTarget->satisfy(theArg);
  }
  void squash() {
    theTarget->squash(theArg);
  }
};

Effect *mapSource(SemanticInstruction *inst, eOperandCode anInputCode, eOperandCode anOutputCode);
Effect *freeMapping(SemanticInstruction *inst, eOperandCode aMapping);
Effect *disconnectRegister(SemanticInstruction *inst, eOperandCode aMapping);
Effect *mapCCDestination(SemanticInstruction *inst);
Effect *mapDestination(SemanticInstruction *inst);
Effect *mapRD1Destination(SemanticInstruction *inst);
Effect *mapRD2Destination(SemanticInstruction *inst);
Effect *mapDestination_NoSquashEffects(SemanticInstruction *inst);
Effect *mapRD1Destination_NoSquashEffects(SemanticInstruction *inst);
Effect *mapRD2Destination_NoSquashEffects(SemanticInstruction *inst);
Effect *unmapDestination(SemanticInstruction *inst);
Effect *mapFDestination(SemanticInstruction *inst, int32_t anIndex);
Effect *unmapFDestination(SemanticInstruction *inst, int32_t anIndex);
Effect *restorePreviousDestination(SemanticInstruction *inst);
Effect *satisfy(SemanticInstruction *inst, InternalDependance const &aDependance);
Effect *squash(SemanticInstruction *inst, InternalDependance const &aDependance);
Effect *annulNext(SemanticInstruction *inst);
Effect *branch(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *returnFromTrap(SemanticInstruction *inst, bool isDone);
Effect *branchAfterNext(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *branchAfterNext(SemanticInstruction *inst, eOperandCode aCode);
Effect *branchConditionally(SemanticInstruction *inst, VirtualMemoryAddress aTarget, bool anAnnul,
                            Condition &aCondition, bool isFloating);
Effect *branchRegConditionally(SemanticInstruction *inst, VirtualMemoryAddress aTarget,
                               bool anAnnul, uint32_t aCondition);
Effect *allocateLoad(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                     nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateCASP(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                     nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateRMW(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *eraseLSQ(SemanticInstruction *inst);
Effect *allocateStore(SemanticInstruction *inst, eSize aSize, bool aBypassSB,
                      nuArchARM::eAccType type);
Effect *allocateMEMBAR(SemanticInstruction *inst, eAccType type);
Effect *retireMem(SemanticInstruction *inst);
Effect *commitStore(SemanticInstruction *inst);
Effect *accessMem(SemanticInstruction *inst);
Effect *updateConditional(SemanticInstruction *inst);
Effect *updateUnconditional(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *updateUnconditional(SemanticInstruction *inst, eOperandCode anOperandCode);
Effect *updateCall(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *updateIndirect(SemanticInstruction *inst, eOperandCode anOperandCode, eBranchType aType);
Effect *updateNonBranch(SemanticInstruction *inst);
Effect *readPR(SemanticInstruction *inst, ePrivRegs aPR, std::unique_ptr<SysRegInfo> aRI);
Effect *writePR(SemanticInstruction *inst, ePrivRegs aPR, std::unique_ptr<SysRegInfo> aRI);
Effect *writePSTATE(SemanticInstruction *inst, uint8_t anOp1, uint8_t anOp2);
Effect *writeNZCV(SemanticInstruction *inst);
Effect *clearExclusiveMonitor(SemanticInstruction *inst);
Effect *SystemRegisterTrap(SemanticInstruction *inst);
Effect *checkSystemAccess(SemanticInstruction *inst, uint8_t anOp0, uint8_t anOp1, uint8_t anOp2,
                          uint8_t aCRn, uint8_t aCRm, uint8_t aRT, uint8_t aRead);
Effect *exceptionEffect(SemanticInstruction *inst, eExceptionType aType);
Effect *markExclusiveMonitor(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize);
Effect *exclusiveMonitorPass(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize,
                             InternalDependance const &aDependance);

Effect *checkDAIFAccess(SemanticInstruction *inst, uint8_t anOp1);
Effect *checkSysRegAccess(SemanticInstruction *inst, ePrivRegs aPrivReg, uint8_t is_read);

Effect *mapXTRA(SemanticInstruction *inst);
Effect *forceResync(SemanticInstruction *inst);
Effect *immuException(SemanticInstruction *inst);
Effect *mmuPageFaultCheck(SemanticInstruction *inst);

} // namespace narmDecoder

#endif // FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED
