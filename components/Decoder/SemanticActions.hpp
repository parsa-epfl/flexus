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

#ifndef FLEXUS_DECODER_SEMANTICACTIONS_HPP_INCLUDED
#define FLEXUS_DECODER_SEMANTICACTIONS_HPP_INCLUDED

#include "SemanticInstruction.hpp"
#include "components/Decoder/Effects.hpp"

namespace nDecoder {

enum eSignCode { kSignExtend, kZeroExtend, kNoExtension, kLastExt };

using nuArch::eSize;
using nuArch::SemanticAction;

struct Operation {
  Operation() {
  }
  virtual ~Operation() {
  }
  virtual Operand operator()(std::vector<Operand> const &operands) = 0;
  virtual Operand evalExtra(std::vector<Operand> const &operands) {
    return (uint64_t)0;
  }
  virtual char const *describe() const = 0;

  void setOperands(Operand aValue) {
    theOperands.push_back(aValue);
  }

  std::vector<Operand> theOperands;
  SemanticInstruction *theInstruction;
};

std::unique_ptr<Operation> operation(eOpType aType);

class BaseSemanticAction : public SemanticAction, public UncountedComponent {
private:
  InternalDependance theDependances[5];
  int32_t theEndOfDependances;
  bool theSignalled;
  bool theSquashed;
  bool theReady[5];
  int32_t theNumOperands;

  struct Dep : public DependanceTarget {
    BaseSemanticAction &theAction;
    Dep(BaseSemanticAction &anAction) : theAction(anAction) {
    }
    void satisfy(int32_t anArg);
    void squash(int32_t anArg);
  } theDependanceTarget;

protected:
  SemanticInstruction *theInstruction;
  bool theScheduled;

  BaseSemanticAction(SemanticInstruction *anInstruction, int32_t aNumOperands)
      : theEndOfDependances(0), theSignalled(false), theSquashed(false),
        theNumOperands(aNumOperands), theDependanceTarget(*this), theInstruction(anInstruction),
        theScheduled(false) {
    theReady[0] = (aNumOperands < 1);
    theReady[1] = (aNumOperands < 2);
    theReady[2] = (aNumOperands < 3);
    theReady[3] = (aNumOperands < 4);
    theReady[4] = (aNumOperands < 5);
  }

public:
  void evaluate();
  virtual void satisfy(int);
  virtual void squash(int);
  int32_t numOperands() const {
    return theNumOperands;
  }

  int64_t instructionNo() const {
    return theInstruction->sequenceNo();
  }

  InternalDependance dependance(int32_t anArg = 0) {
    DBG_Assert(anArg < theNumOperands);
    return InternalDependance(&theDependanceTarget, anArg);
  }

protected:
  virtual void doEvaluate() {
    DBG_Assert(false);
  }

  void satisfyDependants();
  void squashDependants();

  nuArch::uArch *core();

  bool cancelled() const {
    return theInstruction->isSquashed() || theInstruction->isRetired();
  }
  bool ready() const {
    return theReady[0] && theReady[1] && theReady[2] && theReady[3] && theReady[4];
  }
  bool signalled() const {
    return theSignalled;
  }
  void setReady(int32_t anArg, bool aReady) {
    theReady[anArg] = aReady;
  }

  void addRef();
  void releaseRef();
  void reschedule();

public:
  void addDependance(InternalDependance const &aDependance);
  virtual ~BaseSemanticAction() {
  }
};

struct simple_action {
  BaseSemanticAction *action;
  simple_action() {
  }
  simple_action(BaseSemanticAction *act) : action(act) {
  }
};

struct predicated_action : virtual simple_action {
  InternalDependance predicate;
  predicated_action() {
  }
  predicated_action(BaseSemanticAction *act, InternalDependance const &pred)
      : simple_action(act), predicate(pred) {
  }
};

struct dependant_action : virtual simple_action {
  InternalDependance dependance;
  dependant_action() {
  }
  dependant_action(BaseSemanticAction *act, InternalDependance const &dep)
      : simple_action(act), dependance(dep) {
  }
};

struct predicated_dependant_action : predicated_action, dependant_action {
  predicated_dependant_action() {
  }
  predicated_dependant_action(BaseSemanticAction *act, InternalDependance const &dep,
                              InternalDependance const &pred)
      : simple_action(act), predicated_action(act, pred), dependant_action(act, dep) {
  }
};

struct multiply_dependant_action : virtual simple_action {
  std::vector<InternalDependance> dependances;
  multiply_dependant_action() {
  }
  multiply_dependant_action(BaseSemanticAction *act, std::vector<InternalDependance> &dep)
      : simple_action(act) {
    std::swap(dependances, dep);
  }
};

void connectDependance(InternalDependance const &aDependant, simple_action &aSource);
void connect(std::list<InternalDependance> const &dependances, simple_action &aSource);

simple_action readRegisterAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                                 eOperandCode anOperandCode);
simple_action readConstantAction(SemanticInstruction *anInstruction, uint64_t aVal,
                                 eOperandCode anOperandCode);
simple_action calcAddressAction(SemanticInstruction *anInstruction,
                                std::vector<std::list<InternalDependance>> &opDeps);
simple_action translationAction(SemanticInstruction *anInstruction);

predicated_action exclusiveMonitorAction(SemanticInstruction *anInstruction,
                                         eOperandCode anAddressCode, eSize aSize,
                                         boost::optional<eOperandCode> aBypass);
predicated_action shiftAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                              std::unique_ptr<Operation> &aShiftOp, uint64_t aShiftAmount,
                              bool is64);
predicated_action constantAction(SemanticInstruction *anInstruction, uint64_t aConstant,
                                 eOperandCode aResult, boost::optional<eOperandCode> aBypass);
predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass, int operandOffSet = 0);
predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<eOperandCode> &anOperands,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass);
predicated_action annulAction(SemanticInstruction *anInstruction);

dependant_action writebackAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                                 eOperandCode aMappedRegisterCode, eSignCode aSign);
dependant_action branchCondAction(SemanticInstruction *anInstruction, VirtualMemoryAddress aTarget,
                                  std::unique_ptr<Condition> aCondition,
                                  std::vector<std::list<InternalDependance>> &opDeps);
dependant_action branchRegAction(SemanticInstruction *anInstruction, eOperandCode aRegOperand,
                                 eBranchType type);
dependant_action branchToCalcAddressAction(SemanticInstruction *anInstruction);

multiply_dependant_action updateVirtualAddressAction(SemanticInstruction *anInstruction,
                                                     eOperandCode aCode = kAddress);

predicated_dependant_action updateRMWValueAction(SemanticInstruction *anInstruction,
                                                 eOperandCode aOperandCode, eOperandCode aResultCode,
                                                 eOperation anOperation,
                                                 eRMWOperation anAMO);

predicated_dependant_action updateStoreValueAction(SemanticInstruction *anInstruction,
                                                   eOperandCode data);

predicated_dependant_action loadAction(SemanticInstruction *anInstruction, eSize aSize,
                                       eSignCode aSignExtend,
                                       boost::optional<eOperandCode> aBypass);
predicated_dependant_action amoAction(SemanticInstruction *anInstruction, eSize aSize,
                                      eSignCode aSignExtend, boost::optional<eOperandCode> aBypass);

predicated_action tlbiAction(SemanticInstruction *anInstruction, std::vector<std::list<InternalDependance>> &opDeps);

simple_action    ecallAction(SemanticInstruction *anInstruction);
simple_action      wfiAction(SemanticInstruction *anInstruction);

} // namespace nDecoder

#endif // FLEXUS_DECODER_EFFECTS_HPP_INCLUDED
