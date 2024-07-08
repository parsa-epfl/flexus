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

#include <components/uArch/uArchInterfaces.hpp>

#include "BitManip.hpp"
#include "Conditions.hpp"
#include "Effects.hpp"
#include "Interactions.hpp"
#include "SemanticActions.hpp"

namespace nDecoder {

using namespace nuArch;

void connect(std::list<InternalDependance> const &dependances, simple_action &aSource) {
  BaseSemanticAction &act = *(aSource.action);
  for(auto &dep: dependances)
    act.addDependance(dep);
}

void BaseSemanticAction::addRef() {
  boost::intrusive_ptr_add_ref(theInstruction);
}

void BaseSemanticAction::releaseRef() {
  boost::intrusive_ptr_release(theInstruction);
}

void BaseSemanticAction::Dep::satisfy(int32_t anArg) {
  SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction << " " << anArg);
  theAction.satisfy(anArg);
}
void BaseSemanticAction::Dep::squash(int32_t anArg) {
  SEMANTICS_DBG(theAction.theInstruction->identify() << " " << theAction);
  theAction.squash(anArg);
}

void BaseSemanticAction::satisfyDependants() {
  if (!cancelled() && !signalled()) {
    for (int32_t i = 0; i < theEndOfDependances; ++i) {
      theDependances[i].satisfy();
    }
    theSignalled = true;
    theSquashed = false;
  } else {
    SEMANTICS_DBG(theInstruction << "NOTE: Dependants were canceled!");
  }
}

void BaseSemanticAction::satisfy(int32_t anArg) {
  if (!cancelled()) {
    bool was_ready = ready();
    setReady(anArg, true);
    theSquashed = false;
    if (!was_ready && ready() && core()) {
      setReady(anArg, true);
      reschedule();
    }
  } else {
    SEMANTICS_DBG(theInstruction << "NOTE: Action were canceled!");
  }
}

void BaseSemanticAction::squash(int32_t anOperand) {
  SEMANTICS_DBG(*theInstruction << *this);
  setReady(anOperand, false);
  squashDependants();
}

void BaseSemanticAction::squashDependants() {
  if (!theSquashed) {
    if (core()) {

      SEMANTICS_DBG(theInstruction);
      for (int32_t i = 0; i < theEndOfDependances; ++i) {
        theDependances[i].squash();
      }
    }
    theSignalled = false;
    theSquashed = true;
  }
}

void BaseSemanticAction::evaluate() {
  theScheduled = false;
  if (!cancelled()) {
    doEvaluate();
  }
}

void BaseSemanticAction::addDependance(InternalDependance const &aDependance) {
  DBG_Assert(theEndOfDependances < 4);
  theDependances[theEndOfDependances] = aDependance;
  ++theEndOfDependances;
}

void BaseSemanticAction::reschedule() {
  if (!theScheduled && core()) {
    DBG_(Iface, (<< *this << " rescheduled"));
    theScheduled = true;
    core()->reschedule(this);
  }
}

void connectDependance(InternalDependance const &aDependant, simple_action &aSource) {
  aSource.action->addDependance(aDependant);
}

uArch *BaseSemanticAction::core() {
  return theInstruction->core();
}

struct PredicatedSemanticAction : public BaseSemanticAction {
  bool thePredicate;

  PredicatedSemanticAction(SemanticInstruction *anInstruction, int32_t aNumArgs,
                           bool anInitialPredicate);

  struct Pred : public DependanceTarget {
    PredicatedSemanticAction &theAction;
    Pred(PredicatedSemanticAction &anAction) : theAction(anAction) {
    }
    void satisfy(int32_t anArg) {
      SEMANTICS_TRACE;
      theAction.predicate_on(anArg);
    }
    void squash(int32_t anArg) {
      theAction.predicate_off(anArg);
    }
  } thePredicateTarget;

  InternalDependance predicate() {
    return InternalDependance(&thePredicateTarget, 0);
  }

  virtual void squash(int);
  virtual void predicate_off(int);
  virtual void predicate_on(int);
  virtual void evaluate();
};

PredicatedSemanticAction::PredicatedSemanticAction(SemanticInstruction *anInstruction,
                                                   int32_t aNumArgs, bool anInitialPredicate)
    : BaseSemanticAction(anInstruction, aNumArgs), thePredicate(anInitialPredicate),
      thePredicateTarget(*this) {
}

void PredicatedSemanticAction::evaluate() {
  theScheduled = false;
  if (thePredicate) {
    BaseSemanticAction::evaluate();
  }
}

void PredicatedSemanticAction::squash(int32_t anOperand) {
  setReady(anOperand, false);
  if (thePredicate) {
    squashDependants();
  }
}

void PredicatedSemanticAction::predicate_off(int) {
  if (!cancelled() && thePredicate) {
    SEMANTICS_DBG(*theInstruction << *this);
    reschedule();
    thePredicate = false;
    squashDependants();
  }
}

void PredicatedSemanticAction::predicate_on(int) {
  if (!cancelled() && !thePredicate) {
    SEMANTICS_DBG(*theInstruction << *this);
    reschedule();
    thePredicate = true;
    squashDependants();
  }
}

struct register_value_extractor : boost::static_visitor<register_value> {
  register_value operator()(uint64_t v) const {
    return v;
  }

  register_value operator()(int64_t v) const {
    return v;
  }

  template <class T> register_value operator()(T aT) const {
    DBG_Assert(false, (<< "Attempting to store a non-register value operand "
                          "into a register"));
    return uint64_t(0ULL);
  }
};

struct AnnulAction : public PredicatedSemanticAction {
  eOperandCode theRegisterCode;
  eOperandCode theOutputCode;
  bool theMOVConnected;
  bool theTriggered;

  AnnulAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
              eOperandCode anOutputCode)
      : PredicatedSemanticAction(anInstruction, 1, false), theRegisterCode(aRegisterCode),
        theOutputCode(anOutputCode), theMOVConnected(false) {
  }

  void doEvaluate() {
    if (thePredicate) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
      eResourceStatus status;
      if (!theMOVConnected) {
        status =
            core()->requestRegister(name, theInstruction->makeInstructionDependance(dependance(0)));
        theMOVConnected = true;
      } else {
        status = core()->requestRegister(name);
        DBG_(VVerb, (<< *this << " scheduled. " << theRegisterCode << "(" << name << ")"
                     << " is " << status));
      }
      if (kReady == status) {
        setReady(0, true);
      }
      if (ready()) {
        Operand aValue = core()->readRegister(name);
        theInstruction->setOperand(theOutputCode, aValue);
        DBG_(VVerb, (<< *this << " read " << theRegisterCode << "(" << name << ") = " << aValue
                     << " written to " << theOutputCode));
        theInstruction->setExecuted(true);
        satisfyDependants();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " Annul " << theRegisterCode << " store in "
              << theOutputCode;
  }
};

predicated_action annulAction(SemanticInstruction *anInstruction) {
  AnnulAction *act = new AnnulAction(anInstruction, kPPD, kResult);
  anInstruction->addNewComponent(act);
  return predicated_action(act, act->predicate());
}

struct BranchCondAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  std::unique_ptr<Condition> theCondition;
  uint32_t theFeedbackCount;

  BranchCondAction(SemanticInstruction *anInstruction, VirtualMemoryAddress aTarget,
                   std::unique_ptr<Condition> &aCondition, size_t numOperands)
      : BaseSemanticAction(anInstruction, numOperands), theTarget(aTarget),
        theCondition(std::move(aCondition)), theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(eOperandCode(kOperand1 + i)));
        }

        if (theInstruction->hasOperand(kCondition)) {
          operands.push_back(theInstruction->operand(kCondition));
        }

        boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        theCondition->setInstruction(theInstruction);

        bool result = theCondition->operator()(operands);

        if (result) {
          // Taken
          theInstruction->redirectPC(theTarget);
          core()->applyToNext(theInstruction, branchInteraction(theTarget));
          feedback->theActualDirection = kTaken;
          DBG_(Iface, (<< "Branch taken! " << *theInstruction));
        } else {
          auto offs = ((theInstruction->getOpcode() & 0x3) != 0x3) ? 2 : 4;
          theInstruction->redirectPC(theInstruction->pc() + offs);
          core()->applyToNext(theInstruction, branchInteraction(theInstruction->pc() + offs));
          feedback->theActualDirection = kNotTaken;
          DBG_(Iface, (<< "Branch Not taken! " << *theInstruction));
        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};

dependant_action branchCondAction(SemanticInstruction *anInstruction, VirtualMemoryAddress aTarget,
                                  std::unique_ptr<Condition> aCondition,
                                  std::vector<std::list<InternalDependance>> &opDeps) {
  BranchCondAction *act = new BranchCondAction(anInstruction, aTarget, aCondition, opDeps.size());
  anInstruction->addNewComponent(act);

  for (size_t i = 0; i < opDeps.size(); i++)
    opDeps[i].push_back(act->dependance(i));

  return dependant_action(act, act->dependance());
}

struct BranchRegAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  eOperandCode theRegOperand;
  eBranchType theType;

  BranchRegAction(SemanticInstruction *anInstruction, eOperandCode aRegOperand, eBranchType aType)
      : BaseSemanticAction(anInstruction, 1), theRegOperand(aRegOperand), theType(aType) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        DBG_(Iface, (<< *this << " Branching to an address held in register " << theRegOperand));

        uint64_t target = boost::get<uint64_t>(theInstruction->operand<uint64_t>(theRegOperand));

        theTarget = VirtualMemoryAddress(target);

        boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = theType;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();
        theInstruction->setBranchFeedback(feedback);

        DBG_(Iface, (<< *this << " Checking for redirection PC= " << theInstruction->pc()
                     << " target= " << theTarget));

        theInstruction->redirectPC(theTarget);
        core()->applyToNext(theInstruction, branchInteraction(theTarget));

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchRegAction(SemanticInstruction *anInstruction, eOperandCode aRegOperand,
                                 eBranchType type) {
  BranchRegAction *act = new BranchRegAction(anInstruction, aRegOperand, type);
  anInstruction->addNewComponent(act);
  return dependant_action(act, act->dependance());
}

struct BranchToCalcAddressAction : public BaseSemanticAction {
  eOperandCode theTarget;
  uint32_t theFeedbackCount;

  BranchToCalcAddressAction(SemanticInstruction *anInstruction, eOperandCode aTarget)
      : BaseSemanticAction(anInstruction, 1), theTarget(aTarget), theFeedbackCount(0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {

        // Feedback is taken care of by the updateUncoditional effect at
        // retirement
        uint64_t target = theInstruction->operand<uint64_t>(theTarget);
        VirtualMemoryAddress target_addr(target);
        DBG_(Iface, (<< *this << " branc to mapped_reg target: " << target_addr));

        theInstruction->redirectPC(target_addr);
        core()->applyToNext(theInstruction, branchInteraction(target_addr));

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(VVerb, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " BranchToCalcAddress Action";
  }
};

dependant_action branchToCalcAddressAction(SemanticInstruction *anInstruction) {
  BranchToCalcAddressAction *act = new BranchToCalcAddressAction(anInstruction, kAddress);
  anInstruction->addNewComponent(act);

  return dependant_action(act, act->dependance());
}

struct ConstantAction : public PredicatedSemanticAction {
  bits theConstant;
  eOperandCode theResult;
  boost::optional<eOperandCode> theBypass;

  ConstantAction(SemanticInstruction *anInstruction, bits aConstant, eOperandCode aResult,
                 boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 1, true), theConstant(aConstant),
        theResult(aResult), theBypass(aBypass) {
    setReady(0, true);
  }

  void doEvaluate() {
    theInstruction->setOperand(theResult, static_cast<bits>(theConstant));
    DBG_(VVerb, (<< *this << " applied"));
    if (theBypass) {
      mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
      core()->bypass(name, theConstant);
    }
    satisfyDependants();
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " store constant " << theConstant << " in "
              << theResult;
  }
};

predicated_action constantAction(SemanticInstruction *anInstruction, uint64_t aConstant,
                                 eOperandCode aResult, boost::optional<eOperandCode> aBypass) {
  ConstantAction *act = new ConstantAction(anInstruction, aConstant, aResult, aBypass);
  anInstruction->addNewComponent(act);
  return predicated_action(act, act->predicate());
}

struct ExclusiveMonitorAction : public PredicatedSemanticAction {
  eOperandCode theAddressCode, theRegisterCode;
  eSize theSize;
  boost::optional<eOperandCode> theBypass;

  ExclusiveMonitorAction(SemanticInstruction *anInstruction, eOperandCode anAddressCode,
                         eSize aSize, boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 2, true), theAddressCode(anAddressCode),
        theSize(aSize), theBypass(aBypass) {
  }

  void doEvaluate() {
    DBG_(Iface, (<< *this));
    uint64_t addr = theInstruction->operand<uint64_t>(theAddressCode);
    Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
    PhysicalMemoryAddress pAddress =
        PhysicalMemoryAddress(c->translateVirtualAddress(VirtualMemoryAddress((addr >> 6) << 6),
            Flexus::Qemu::API::QEMU_Curr_Load));

    if (core()->isROBHead(theInstruction)) {
      int passed = core()->isExclusiveLocal(pAddress, theSize);
      if (passed == kMonitorDoesntExist) {
        DBG_(Trace, (<< "ExclusiveMonitorAction resulted in kMonitorDoesntExist!"));
        passed = kMonitorUnset;
      }
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
    } else {
      DBG_(VVerb, (<< "Recheduling Exclusive Monitor for address " << pAddress << ", "
                   << *theInstruction));
      setReady(0, false);
      satisfy(0);
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

struct ExecuteBase : public PredicatedSemanticAction {
  eOperandCode theOperands[5];
  eOperandCode theResult;
  std::unique_ptr<Operation> theOperation;

  ExecuteBase(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
              eOperandCode aResult, std::unique_ptr<Operation> &anOperation)
      : PredicatedSemanticAction(anInstruction, anOperands.size(), true), theResult(aResult),
        theOperation(std::move(anOperation)) {

    for (int32_t i = 0; i < numOperands(); ++i) {
      theOperands[i] = anOperands[i];
    }
  }

  Operand op(eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};

struct OperandPrintHelper {
  std::vector<Operand> &theOperands;
  OperandPrintHelper(std::vector<Operand> &operands) : theOperands(operands) {
  }
  friend std::ostream &operator<<(std::ostream &anOstream, OperandPrintHelper const &aHelper) {
    int32_t i = 0;
    for (const auto& op: aHelper.theOperands)
      anOstream << " op" << ++i << "=" << std::hex << op << std::dec;
    return anOstream;
  }
};

struct ExecuteAction : public ExecuteBase {
  boost::optional<eOperandCode> theBypass;
  bool theFlags;

  ExecuteAction(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands,
                eOperandCode aResult, std::unique_ptr<Operation> &anOperation,
                boost::optional<eOperandCode> aBypass)
      : ExecuteBase(anInstruction, anOperands, aResult, anOperation), theBypass(aBypass) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    DBG_(Iface, (<< "Trying to Execute " << *this));

    if (ready()) {

      DBG_(Iface, (<< "Executing " << *this));

      if (theInstruction->hasPredecessorExecuted()) {
        std::vector<Operand> operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back(op(theOperands[i]));
        }

        theOperation->theInstruction = theInstruction;
        Operand result = theOperation->operator()(operands);
        uint64_t res = boost::get<uint64_t>(result);
        theInstruction->setOperand(theResult, res);
        DBG_(Iface, (<< "Writing " << std::hex << res << std::dec << " in " << theResult));
        DBG_(Iface, (<< *this << " operands: " << OperandPrintHelper(operands) << std::hex
                     << " result=" << result << std::dec));
        if (theBypass) {
          mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
          register_value val = boost::apply_visitor(register_value_extractor(), result);
          core()->bypass(name, val);
        }
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(Iface, (<< *this << " waiting for predecessor "));
        reschedule();
      }
    } else {
      DBG_(Iface, (<< "cant Execute " << *this << " yet"));
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ExecuteAction " << theOperation->describe();
  }
};

predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass, int operandOffSet) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back(eOperandCode(kOperand1 + operandOffSet + i));
  }
  ExecuteAction *act = new ExecuteAction(anInstruction, operands, aResult, anOperation, aBypass);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action executeAction(SemanticInstruction *anInstruction,
                                std::unique_ptr<Operation> &anOperation,
                                std::vector<eOperandCode> &anOperands,
                                std::vector<std::list<InternalDependance>> &opDeps,
                                eOperandCode aResult, boost::optional<eOperandCode> aBypass) {

  ExecuteAction *act = new ExecuteAction(anInstruction, anOperands, aResult, anOperation, aBypass);
  anInstruction->addNewComponent(act);
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

simple_action calcAddressAction(SemanticInstruction *anInstruction,
                                std::vector<std::list<InternalDependance>> &opDeps) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back(eOperandCode(kOperand1 + i));
  }
  std::unique_ptr<Operation> mov = operation(kMOV);
  ExecuteAction *act = new ExecuteAction(anInstruction, operands, kAddress, mov, boost::none);
  anInstruction->addNewComponent(act);

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back(act->dependance(i));
  }
  return act;
}

struct LoadAction : public PredicatedSemanticAction {
  eSize theSize;
  eSignCode theSignExtend;
  boost::optional<eOperandCode> theBypass;
  bool theLoadExtended;

  LoadAction(SemanticInstruction *anInstruction, eSize aSize, eSignCode aSigncode,
             boost::optional<eOperandCode> aBypass, bool aLoadExtended)
      : PredicatedSemanticAction(anInstruction, 1, true), theSize(aSize), theSignExtend(aSigncode),
        theBypass(aBypass), theLoadExtended(aLoadExtended) {
  }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    SEMANTICS_DBG(*this);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void doLoad() {
    SEMANTICS_DBG(*this);
    bits value;

    if (theLoadExtended) {
      value = core()->retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
    } else {
      value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
    }

    switch (theSize) {
    case kByte:
      value &= 0xFFULL;
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
      }
      break;
    case kHalfWord:
      value &= 0xFFFFULL;
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x8000ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFF0000ULL : 0ULL;
      }
      break;
    case kWord:
      value &= 0xFFFFFFFFULL;
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80000000ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFF00000000ULL : 0ULL;
      }
      break;
    case kDoubleWord:
      break;
    case kQuadWord:
    default:
      DBG_Assert(false);
      break;
    }

    theInstruction->setOperand(kResult, (uint64_t)value);
    SEMANTICS_DBG(*this << " received load value=" << std::hex << value);
    if (theBypass) {
      mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
      SEMANTICS_DBG(*this << " bypassing value=" << std::hex << value << " to " << name);
      core()->bypass(name, (uint64_t)value);
    }
    satisfyDependants();
  }

  void doEvaluate() {
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " LoadAction";
  }
};

predicated_dependant_action loadAction(SemanticInstruction *anInstruction, eSize aSize,
                                       eSignCode aSignCode, boost::optional<eOperandCode> aBypass) {
  LoadAction *act = new LoadAction(anInstruction, aSize, aSignCode, aBypass, false);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

predicated_dependant_action amoAction(SemanticInstruction *anInstruction, eSize aSize,
                                      eSignCode aSignCode, boost::optional<eOperandCode> aBypass) {
  LoadAction *act = new LoadAction(anInstruction, aSize, aSignCode, aBypass, true);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

struct ReadConstantAction : public BaseSemanticAction {
  eOperandCode theOperandCode;
  uint64_t theVal;

  ReadConstantAction(SemanticInstruction *anInstruction, int64_t aVal, eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 1), theOperandCode(anOperandCode), theVal(aVal) {
  }

  void doEvaluate() {
    DBG_(Iface, (<< "Reading constant: 0x" << std::hex << theVal << std::dec << " into "
                 << theOperandCode));
    theInstruction->setOperand(theOperandCode, theVal);

    satisfyDependants();
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReadConstant " << theVal << " to "
              << theOperandCode;
  }
};

simple_action readConstantAction(SemanticInstruction *anInstruction, uint64_t aVal,
                                 eOperandCode anOperandCode) {
  ReadConstantAction *act = new ReadConstantAction(anInstruction, aVal, anOperandCode);
  anInstruction->addNewComponent(act);
  return simple_action(act);
}

struct ReadPCAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadPCAction(SemanticInstruction *anInstruction, eOperandCode aResult)
      : PredicatedSemanticAction(anInstruction, 1, true), theResult(aResult) {
    setReady(0, true);
  }

  void doEvaluate() {
    uint64_t pc = theInstruction->pc();
    theInstruction->setOperand(theResult, (bits)pc);

    DBG_(VVerb, (<< *this << " read PC"));
    satisfyDependants();
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " Read PC store in " << theResult;
  }
};

predicated_action readPCAction(SemanticInstruction *anInstruction) {
  ReadPCAction *act = new ReadPCAction(anInstruction, kResult);
  anInstruction->addNewComponent(act);

  return predicated_action(act, act->predicate());
}

struct ReadRegisterAction : public BaseSemanticAction {
  eOperandCode theRegisterCode;
  eOperandCode theOperandCode;
  bool theConnected;

  ReadRegisterAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                     eOperandCode anOperandCode)
      : BaseSemanticAction(anInstruction, 1), theRegisterCode(aRegisterCode),
        theOperandCode(anOperandCode), theConnected(false) {
  }

  bool bypass(register_value aValue) {
    if (cancelled() || theInstruction->isRetired() || theInstruction->isSquashed()) {
      return true;
    }

    if (!signalled()) {
      aValue = (uint64_t)(boost::get<uint64_t>(aValue));
      theInstruction->setOperand(theOperandCode, aValue);
      DBG_(VVerb, (<< *this << " bypassed " << theRegisterCode << " = " << aValue
                   << " written to " << theOperandCode));
      satisfyDependants();
      setReady(0, true);
    }
    return false;
  }

  void doEvaluate() {
    DBG_(Iface, (<< *this));

    register_value aValue;
    uint64_t val;

    if (!theConnected) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
      setReady(0, core()->requestRegister(
                      name, theInstruction->makeInstructionDependance(dependance())) == kReady);
      core()->connectBypass(name, theInstruction,
                            std::bind(&ReadRegisterAction::bypass, this, std::placeholders::_1));
      theConnected = true;
    }
    if (!signalled()) {
      SEMANTICS_DBG("Signalling");

      mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
      eResourceStatus status = core()->requestRegister(name);

      if (status == kReady) {
        aValue = core()->readRegister(name);
        val = boost::get<uint64_t>(aValue);
      } else {
        setReady(0, false);
        return;
      }
    } else {
      return;
    }

    aValue = val;

    DBG_(Iface, (<< "Reading register " << theRegisterCode << " with a value " << std::hex << aValue
                 << std::dec));

    theInstruction->setOperand(theOperandCode, val);
    satisfyDependants();
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReadRegister " << theRegisterCode << " store in "
              << theOperandCode;
  }
};

simple_action readRegisterAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                                 eOperandCode anOperandCode) {
  ReadRegisterAction *act =
      new ReadRegisterAction(anInstruction, aRegisterCode, anOperandCode);
  anInstruction->addNewComponent(act);
  return simple_action(act);
}

struct ShiftRegisterAction : public PredicatedSemanticAction {
  eOperandCode theRegisterCode;
  std::unique_ptr<Operation> theShiftOperation;
  uint64_t theShiftAmount;

  ShiftRegisterAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                      std::unique_ptr<Operation> &aShiftOperation, uint64_t aShiftAmount)
      : PredicatedSemanticAction(anInstruction, 1, true), theRegisterCode(aRegisterCode),
        theShiftOperation(std::move(aShiftOperation)), theShiftAmount(aShiftAmount) {
  }

  void doEvaluate() {
    SEMANTICS_DBG(*this);

    if (ready()) {
      Operand aValue = theInstruction->operand(theRegisterCode);

      aValue = theShiftOperation->operator()({aValue, theShiftAmount});

      uint64_t val = boost::get<uint64_t>(aValue);

      theInstruction->setOperand(theRegisterCode, val);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ShiftRegisterAction " << theRegisterCode;
  }
};

predicated_action shiftAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                              std::unique_ptr<Operation> &aShiftOp, uint64_t aShiftAmount) {
  ShiftRegisterAction *act =
      new ShiftRegisterAction(anInstruction, aRegisterCode, aShiftOp, aShiftAmount);
  anInstruction->addNewComponent(act);
  return predicated_action(act, act->predicate());
}

struct TranslationAction : public BaseSemanticAction {

  TranslationAction(SemanticInstruction *anInstruction) : BaseSemanticAction(anInstruction, 1) {
  }

  void squash(int32_t anOperand) {
    if (!cancelled()) {
      DBG_(VVerb, (<< *this << " Squashing paddr."));
      core()->resolvePAddr(boost::intrusive_ptr<Instruction>(theInstruction),
                           (PhysicalMemoryAddress)kUnresolved);
    }
    boost::intrusive_ptr<Instruction>(theInstruction)->setResolved(false);
    BaseSemanticAction::squash(anOperand);
  }

  void doEvaluate() {
    SEMANTICS_DBG(*this);

    if (ready()) {
      DBG_Assert(theInstruction->hasOperand(kAddress));
      VirtualMemoryAddress addr(theInstruction->operand<uint64_t>(kAddress));

      theInstruction->core()->translate(boost::intrusive_ptr<Instruction>(theInstruction));

      satisfyDependants();
    } else {
      reschedule();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " TranslationAction";
  }
};

simple_action translationAction(SemanticInstruction *anInstruction) {
  TranslationAction *act = new TranslationAction(anInstruction);
  anInstruction->addNewComponent(act);
  return simple_action(act);
}

struct UpdateAddressAction : public BaseSemanticAction {

  eOperandCode theAddressCode;
  bool theVirtual;

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

struct UpdateStoreValueAction : public PredicatedSemanticAction {
  eOperandCode theOperandCode;

  UpdateStoreValueAction(SemanticInstruction *anInstruction, eOperandCode anOperandCode)
      : PredicatedSemanticAction(anInstruction, 1, true), theOperandCode(anOperandCode) {
  }

  void predicate_off(int) {
    if (!cancelled() && thePredicate) {
      DBG_(VVerb, (<< *this << " predicated off. "));
      DBG_Assert(core());
      DBG_(VVerb, (<< *this << " anulling store"));
      core()->annulStoreValue(boost::intrusive_ptr<Instruction>(theInstruction));
      thePredicate = false;
      satisfyDependants();
    }
  }

  void predicate_on(int) {
    if (!cancelled() && !thePredicate) {
      DBG_(VVerb, (<< *this << " predicated on. "));
      reschedule();
      thePredicate = true;
      squashDependants();
    }
  }

  void doEvaluate() {
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

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateStoreValue";
  }
};

predicated_dependant_action updateStoreValueAction(SemanticInstruction *anInstruction,
                                                   eOperandCode data) {
  UpdateStoreValueAction *act = new UpdateStoreValueAction(anInstruction, data);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

struct UpdateRMWValueAction : public PredicatedSemanticAction {

  eOperandCode theOperandCode, theResultCode;
  eOperation theOperation;
  eRMWOperation theRMW;

  UpdateRMWValueAction(SemanticInstruction *anInstruction, eOperandCode anOperandCode, eOperandCode anResultCode,
                       eOperation anOperation, eRMWOperation anRMW)
      : PredicatedSemanticAction(anInstruction, 1, true), theOperandCode(anOperandCode), theResultCode(anResultCode),
        theOperation(anOperation), theRMW(anRMW) {
  }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    SEMANTICS_DBG(*this);
    if (!cancelled() && ready() && thePredicate)
      doRMW();
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate)
      doRMW();
  }

  void doRMW() {
    uint64_t value = theInstruction->operand<uint64_t>(theOperandCode);
    DBG_(Iface, (<< *this << " updating RMW value=" << std::hex << value
                 << " op=" << theOperation));
    uint64_t old = core()->updateRMWValue(boost::intrusive_ptr<Instruction>(theInstruction), value, theOperation, theRMW);
    theInstruction->setOperand(theResultCode, old);
    satisfyDependants();
  }

  void doEvaluate() {
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " UpdateRMWValue";
  }
};

predicated_dependant_action updateRMWValueAction(SemanticInstruction *anInstruction,
                                                 eOperandCode aOperandCode, eOperandCode aResultCode,
                                                 eOperation anOperation, eRMWOperation anRMW) {
  UpdateRMWValueAction *act = new UpdateRMWValueAction(anInstruction, aOperandCode, aResultCode, anOperation, anRMW);
  anInstruction->addNewComponent(act);
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

struct WritebackAction : public BaseSemanticAction {
  eOperandCode theResult;
  eOperandCode theRd;
  eSignCode theSign;

  WritebackAction(SemanticInstruction *anInstruction, eOperandCode aResult, eOperandCode anRd,
                  eSignCode aSign)
      : BaseSemanticAction(anInstruction, 1), theResult(aResult), theRd(anRd), theSign(aSign) {
  }

  void squash(int32_t anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand<mapped_reg>(theRd);
      core()->squashRegister(name);
      DBG_(VVerb, (<< *this << " squashing register rd=" << name));
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {

    if (ready()) {
      DBG_(Iface, (<< "Writing " << theResult << " to " << theRd));

      register_value result =
          boost::apply_visitor(register_value_extractor(), theInstruction->operand(theResult));
      uint64_t res = boost::get<uint64_t>(result);

      mapped_reg name = theInstruction->operand<mapped_reg>(theRd);

      switch (theSign) {
      case kSignExtend:
        if (res &  0x80000000lu) {
          res |= 0xffffffff00000000lu;
          result = res;
          theInstruction->setOperand(theResult, res);
        } else
      case kZeroExtend:
        if (res & ~0xfffffffflu) {
          res &= 0xffffffff;
          result = res;
          theInstruction->setOperand(theResult, res);
        }
        break;
      default:
        break;
      }

      DBG_(Iface,
           (<< "Writing " << std::hex << result << std::dec << " to X-REG ->" << name));
      core()->writeRegister(name, result);
      DBG_(VVerb, (<< *this << " rd= " << name << " result=" << result));
      core()->bypass(name, result);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " WritebackAction to " << theRd;
  }
};

dependant_action writebackAction(SemanticInstruction *anInstruction, eOperandCode aRegisterCode,
                                 eOperandCode aMappedRegisterCode, eSignCode aSign) {
  WritebackAction *act =
      new WritebackAction(anInstruction, aRegisterCode, aMappedRegisterCode, aSign);
  anInstruction->addNewComponent(act);
  return dependant_action(act, act->dependance());
}

struct TlbiAction: public PredicatedSemanticAction {
  TlbiAction(SemanticInstruction *anInstruction, std::vector<eOperandCode> &anOperands)
      : PredicatedSemanticAction(anInstruction, anOperands.size(), true) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    DBG_(Iface, (<< "trying to execute " << *this));

    if (ready()) {
      DBG_(Iface, (<< "executing " << *this));

      if (theInstruction->hasPredecessorExecuted()) {
        // the real tlb flush is done at the commit stage
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_(Iface, (<< *this << " waiting for predecessor"));
        reschedule();
      }
    } else {
      DBG_(Iface, (<< "cant execute " << *this << " yet"));
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " TlbiAction";
  }
};

predicated_action tlbiAction(SemanticInstruction *anInstruction, std::vector<std::list<InternalDependance>> &opDeps) {
  std::vector<eOperandCode> operands;

  for (uint32_t i = 0; i < opDeps.size(); ++i)
    operands.push_back(eOperandCode(kOperand1 + i));

  TlbiAction *tlbi = new TlbiAction(anInstruction, operands);

  anInstruction->addNewComponent(tlbi);

  for (uint32_t i = 0; i < opDeps.size(); ++i)
    opDeps[i].push_back(tlbi->dependance(i));

  return predicated_action(tlbi, tlbi->predicate());
}

struct EcallAction: public BaseSemanticAction {
  EcallAction(SemanticInstruction *anInstruction)
      : BaseSemanticAction(anInstruction, 0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    DBG_(Iface, (<< "trying to execute " << *this));

    if (ready()) {
      DBG_(Iface, (<< "executing " << *this));

      if (theInstruction->hasPredecessorExecuted()) {
        auto cpu = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
        auto pl  = Flexus::Qemu::API::qemu_api.get_pl(cpu);

        switch (pl) {
        case 0:
          theInstruction->setWillRaise(kException_ECallFromUMode);
          break;
        case 1:
          theInstruction->setWillRaise(kException_ECallFromSMode);
          break;
        case 3:
          theInstruction->setWillRaise(kException_ECallFromMMode);
          break;
        default:
          theInstruction->setWillRaise(kException_IllegalInstr);
        }

        satisfyDependants();
        theInstruction->setExecuted(true);

      } else {
        DBG_(Iface, (<< *this << " waiting for predecessor"));
        reschedule();
      }
    } else {
      DBG_(Iface, (<< "cant execute " << *this << " yet"));
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " EcallAction";
  }
};

simple_action ecallAction(SemanticInstruction *anInstruction) {
  EcallAction *ecall = new EcallAction(anInstruction);

  anInstruction->addNewComponent(ecall);
  anInstruction->addDispatchAction(ecall);

  return simple_action(ecall);
}

struct WfiAction: public BaseSemanticAction {
  WfiAction(SemanticInstruction *anInstruction)
      : BaseSemanticAction(anInstruction, 0) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    DBG_(Iface, (<< "trying to execute " << *this));

    if (ready()) {
      DBG_(Iface, (<< "executing " << *this));

      if (theInstruction->hasPredecessorExecuted()) {
        auto cpu = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
        auto pl  = Flexus::Qemu::API::qemu_api.get_pl (cpu);
        auto irq = Flexus::Qemu::API::qemu_api.get_irq(cpu);

        if (pl == 0)
          theInstruction->setWillRaise(kException_IllegalInstr);
        else if (irq == ~0u) {
          reschedule();
          return;
        }

        satisfyDependants();
        theInstruction->setExecuted(true);
        DBG_(Trace, (<< "WFI waked up"));

      } else {
        DBG_(Iface, (<< *this << " waiting for predecessor"));
        reschedule();
      }
    } else {
      DBG_(Iface, (<< "cant execute " << *this << " yet"));
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " WfiAction";
  }
};

simple_action wfiAction(SemanticInstruction *anInstruction) {
  WfiAction *wfi = new WfiAction(anInstruction);

  anInstruction->addNewComponent(wfi);
  anInstruction->addDispatchAction(wfi);

  return simple_action(wfi);
}


} // namespace nDecoder