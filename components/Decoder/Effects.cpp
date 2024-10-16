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

#include <core/performance/profile.hpp>
#include <core/qemu/mai_api.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "Interactions.hpp"
#include "SemanticInstruction.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

void DependanceTarget::invokeSatisfy(int32_t anArg) {
  void (nDecoder::DependanceTarget::*satisfy_pt)(int32_t) =
      &nDecoder::DependanceTarget::satisfy;
  DBG_(VVerb, (<< std::hex << "Satisfy: " << satisfy_pt));
  satisfy(anArg);
  DBG_(VVerb, (<< "After satisfy"));
}

void EffectChain::invoke(SemanticInstruction &anInstruction) {
  if (theFirst) {
    theFirst->invoke(anInstruction);
  }
}

void EffectChain::describe(std::ostream &anOstream) const {
  if (theFirst) {
    anOstream << "\t  ";
    theFirst->describe(anOstream);
    anOstream << "\n";
  }
}

void EffectChain::append(Effect *anEffect) {
  if (!theFirst) {
    theFirst = anEffect;
  } else {
    theLast->theNext = anEffect;
  }
  theLast = anEffect;
}

EffectChain::EffectChain() : theFirst(0), theLast(0) {
}

struct MapSourceEffect : public Effect {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;

  MapSourceEffect(eOperandCode anInputCode, eOperandCode anOutputCode)
      : theInputCode(anInputCode), theOutputCode(anOutputCode) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    reg name(anInstruction.operand<reg>(theInputCode));
    mapped_reg mapped_name = anInstruction.core()->map(name);
    anInstruction.setOperand(theOutputCode, mapped_name);

    DECODER_DBG(anInstruction.identify()
                << theInputCode << " (" << name << ")"
                << " mapped to " << theOutputCode << "(" << mapped_name << ")");

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "MapSource " << theInputCode << " store mapping in " << theOutputCode;
    Effect::describe(anOstream);
  }
};

Effect *mapSource(SemanticInstruction *inst, eOperandCode anInputCode, eOperandCode anOutputCode) {
  MapSourceEffect *r = new MapSourceEffect(anInputCode, anOutputCode);
  inst->addNewComponent(r);
  return r;
}

struct FreeMappingEffect : public Effect {
  eOperandCode theMappingCode;

  FreeMappingEffect(eOperandCode aMappingCode) : theMappingCode(aMappingCode) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    mapped_reg mapping(anInstruction.operand<mapped_reg>(theMappingCode));
    DBG_(Iface, (<< anInstruction.identify() << " MapEffect free mapping for " << theMappingCode
                 << "(" << mapping << ")"));
    anInstruction.core()->free(mapping);
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "FreeMapping " << theMappingCode;
    Effect::describe(anOstream);
  }
};

Effect *freeMapping(SemanticInstruction *inst, eOperandCode aMapping) {
  FreeMappingEffect *fme = new FreeMappingEffect(aMapping);
  inst->addNewComponent(fme);
  return fme;
}

struct RestoreMappingEffect : public Effect {
  eOperandCode theNameCode;
  eOperandCode theMappingCode;

  RestoreMappingEffect(eOperandCode aNameCode, eOperandCode aMappingCode)
      : theNameCode(aNameCode), theMappingCode(aMappingCode) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    reg name(anInstruction.operand<reg>(theNameCode));
    mapped_reg mapping(anInstruction.operand<mapped_reg>(theMappingCode));

    DBG_(VVerb, (<< anInstruction.identify() << " MapEffect restore mapping for " << theNameCode
                 << "(" << name << ") to " << theMappingCode << "( " << mapping << ")"));
    anInstruction.core()->restore(name, mapping);
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "RestoreMapping " << theNameCode << " to " << theMappingCode;
    Effect::describe(anOstream);
  }
};

Effect *restoreMapping(SemanticInstruction *inst, eOperandCode aName, eOperandCode aMapping) {
  RestoreMappingEffect *rme = new RestoreMappingEffect(aName, aMapping);
  inst->addNewComponent(rme);
  return rme;
}

struct MapDestinationEffect : public Effect {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;
  eOperandCode thePreviousMappingCode;
  bool theSquashEffects;

  MapDestinationEffect(eOperandCode anInputCode, eOperandCode anOutputCode,
                       eOperandCode aPreviousMappingCode, bool aSquashEffects)
      : theInputCode(anInputCode), theOutputCode(anOutputCode),
        thePreviousMappingCode(aPreviousMappingCode), theSquashEffects(aSquashEffects) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    reg name(anInstruction.operand<reg>(theInputCode));
    mapped_reg mapped_name;
    mapped_reg previous_mapping;
    boost::tie(mapped_name, previous_mapping) = anInstruction.core()->create(name);
    anInstruction.setOperand(theOutputCode, mapped_name);
    anInstruction.setOperand(thePreviousMappingCode, previous_mapping);

    // Add effect to free previous mapping upon retirement
    Effect *e = new FreeMappingEffect(thePreviousMappingCode);
    anInstruction.addNewComponent(e);
    anInstruction.addRetirementEffect(e);

    // Add effect to deallocate new register and restore previous mapping upon
    // squash
    if (theSquashEffects) {
      Effect *e = new RestoreMappingEffect(theInputCode, thePreviousMappingCode);
      anInstruction.addNewComponent(e);
      anInstruction.addSquashEffect(e);
      Effect *f = new FreeMappingEffect(theOutputCode);
      anInstruction.addNewComponent(f);
      anInstruction.addSquashEffect(f);
    }

    DECODER_DBG(anInstruction.identify() << " Mapping " << theInputCode << "(" << name << ")"
                                         << " to " << theOutputCode << "(" << mapped_name << ")"
                                         << " previous mapping " << thePreviousMappingCode << "("
                                         << previous_mapping << ")");
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "MapDestination " << theInputCode << ", store mapping in " << theOutputCode;
    Effect::describe(anOstream);
  }
};

Effect *mapDestination(SemanticInstruction *inst) {
  MapDestinationEffect *mde = new MapDestinationEffect(kRD, kPD, kPPD, true);
  inst->addNewComponent(mde);
  return mde;
}

Effect *mapDestination_NoSquashEffects(SemanticInstruction *inst) {
  MapDestinationEffect *mde = new MapDestinationEffect(kRD, kPD, kPPD, false);
  inst->addNewComponent(mde);
  return mde;
}

struct SatisfyDependanceEffect : public Effect {
  InternalDependance theDependance;

  SatisfyDependanceEffect(InternalDependance const &aDependance) : theDependance(aDependance) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();

    SEMANTICS_TRACE;
    theDependance.satisfy();
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Satisfy Dependance Effect";
    Effect::describe(anOstream);
  }
};

struct SquashDependanceEffect : public Effect {
  InternalDependance theDependance;

  SquashDependanceEffect(InternalDependance const &aDependance) : theDependance(aDependance) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction.identify() << " SquashDependanceEffect "));

    theDependance.squash();
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Squash Dependance Effect";
    Effect::describe(anOstream);
  }
};

Effect *satisfy(SemanticInstruction *inst, InternalDependance const &aDependance) {
  SatisfyDependanceEffect *s = new SatisfyDependanceEffect(aDependance);
  inst->addNewComponent(s);
  return s;
}

Effect *squash(SemanticInstruction *inst, InternalDependance const &aDependance) {
  SquashDependanceEffect *s = new SquashDependanceEffect(aDependance);
  inst->addNewComponent(s);
  return s;
}

struct AnnulInstruction : public Interaction {
  void operator()(boost::intrusive_ptr<Instruction> anInstruction,
                  uArch &aCore) {
    DBG_(VVerb, (<< *anInstruction << " Annulled"));
    anInstruction->annul();
  }
  bool annuls() {
    return true;
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "Annul Instruction";
  }
};

Interaction *annulInstructionInteraction() {
  return new AnnulInstruction;
}

struct ReinstateInstruction : public Interaction {
  void operator()(boost::intrusive_ptr<Instruction> anInstruction,
                  uArch &aCore) {
    DBG_(VVerb, (<< *anInstruction << " Reinstated"));
    anInstruction->reinstate();
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "Reinstate Instruction";
  }
};

Interaction *reinstateInstructionInteraction() {
  return new ReinstateInstruction;
}

BranchInteraction::BranchInteraction(VirtualMemoryAddress aTarget) : theTarget(aTarget) {
}

void BranchInteraction::operator()(boost::intrusive_ptr<Instruction> anInstruction,
                                   uArch &aCore) {
  DBG_(VVerb, (<< *anInstruction << " " << *this));
  if (theTarget == 0) {
    theTarget = anInstruction->pc() + (((anInstruction->getOpcode() & 0x3) != 0x3) ? 2 : 4);
  }
  if (anInstruction->pc() != theTarget) {
    DBG_(Verb, (<< *anInstruction << " Branch Redirection."));
    if (aCore.squashFrom(anInstruction)) {
      aCore.redirectFetch(theTarget);
    }
  }
}

void BranchInteraction::describe(std::ostream &anOstream) const {
  anOstream << "Branch to " << theTarget;
}

Interaction *branchInteraction(VirtualMemoryAddress aTarget) {
  return new BranchInteraction(aTarget);
}

struct BranchFeedbackEffect : public Effect {
  BranchFeedbackEffect() {
  }
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction << " BranchFeedbackEffect ")); // NOOSHIN

    if (anInstruction.branchFeedback()) {
      DBG_(VVerb, (<< anInstruction
                   << " Update Branch predictor: " << anInstruction.branchFeedback()->theActualType
                   << " " << anInstruction.branchFeedback()->theActualDirection << " to "
                   << anInstruction.branchFeedback()->theActualTarget));
      anInstruction.core()->branchFeedback(anInstruction.branchFeedback());
    }
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "Update Branch Predictor";
    Effect::describe(anOstream);
  }
};

struct BranchFeedbackWithOperandEffect : public Effect {
  eDirection theDirection;
  eBranchType theType;
  eOperandCode theOperandCode;
  BranchFeedbackWithOperandEffect(eBranchType aType, eDirection aDirection,
                                  eOperandCode anOperandCode)
      : theDirection(aDirection), theType(aType), theOperandCode(anOperandCode) {
  }
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = theType;
    feedback->theActualDirection = theDirection;
    VirtualMemoryAddress target(anInstruction.operand<uint64_t>(theOperandCode));
    DBG_(Iface, (<< anInstruction << " Update Branch predictor: " << theType << " " << theDirection
                 << " to " << target));
    feedback->theActualTarget = target;
    feedback->theBPState = anInstruction.bpState();
    anInstruction.core()->branchFeedback(feedback);
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "Update Branch Predictor";
    Effect::describe(anOstream);
  }
};

Effect *updateConditional(SemanticInstruction *inst) {
  BranchFeedbackEffect *b = new BranchFeedbackEffect();
  inst->addNewComponent(b);
  return b;
}

Effect *updateUnconditional(SemanticInstruction *inst, VirtualMemoryAddress aTarget) {
  boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
  feedback->thePC = inst->pc();
  feedback->theActualType = kUnconditional;
  feedback->theActualDirection = kTaken;
  feedback->theActualTarget = aTarget;
  feedback->theBPState = inst->bpState();
  inst->setBranchFeedback(feedback);
  BranchFeedbackEffect *b = new BranchFeedbackEffect();
  inst->addNewComponent(b);
  return b;
}

Effect *updateUnconditional(SemanticInstruction *inst, eOperandCode anOperandCode) {
  BranchFeedbackWithOperandEffect *b =
      new BranchFeedbackWithOperandEffect(kUnconditional, kTaken, anOperandCode);
  inst->addNewComponent(b);
  return b;
}

Effect *updateCall(SemanticInstruction *inst, VirtualMemoryAddress aTarget) {
  boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
  feedback->thePC = inst->pc();
  feedback->theActualType = kCall;
  feedback->theActualDirection = kTaken;
  feedback->theActualTarget = aTarget;
  feedback->theBPState = inst->bpState();
  inst->setBranchFeedback(feedback);
  BranchFeedbackEffect *b = new BranchFeedbackEffect();
  inst->addNewComponent(b);
  return b;
}

Effect *updateIndirect(SemanticInstruction *inst, eOperandCode anOperandCode, eBranchType aType) {
  BranchFeedbackWithOperandEffect *b =
      new BranchFeedbackWithOperandEffect(aType, kTaken, anOperandCode);
  inst->addNewComponent(b);
  return b;
}

struct BranchEffect : public Effect {
  VirtualMemoryAddress theTarget;
  bool theHasTarget;
  BranchEffect(VirtualMemoryAddress aTarget) : theTarget(aTarget), theHasTarget(true) {
  }
  BranchEffect() : theHasTarget(false) {
  }
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction << " BranchEffect "));

    if (!theHasTarget) {
      Operand address = anInstruction.operand(kAddress);
      theTarget = VirtualMemoryAddress(boost::get<uint64_t>(address));
    }

    anInstruction.redirectPC(theTarget);
    anInstruction.core()->applyToNext(boost::intrusive_ptr<Instruction>(&anInstruction),
                                      branchInteraction(theTarget));
    DBG_(Iface, (<< "BRANCH:  Must redirect to " << theTarget));
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "Branch to " << theTarget;
    Effect::describe(anOstream);
  }
};

Effect *branch(SemanticInstruction *inst, VirtualMemoryAddress aTarget) {
  BranchEffect *b = new BranchEffect(aTarget);
  inst->addNewComponent(b);
  return b;
}

struct AllocateLSQEffect : public Effect {
  eOperation theOperation;
  eSize theSize;
  bool hasDependance;
  InternalDependance theDependance;
  bool theBypassSB;
  eAccType theAccType;

  AllocateLSQEffect(eOperation anOperation, eSize aSize, bool aBypassSB,
                    InternalDependance const &aDependance, eAccType type)
      : theOperation(anOperation), theSize(aSize), hasDependance(true), theDependance(aDependance),
        theBypassSB(aBypassSB), theAccType(type) {
  }
  AllocateLSQEffect(eOperation anOperation, eSize aSize, bool aBypassSB, eAccType type)
      : theOperation(anOperation), theSize(aSize), hasDependance(false), theBypassSB(aBypassSB),
        theAccType(type) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    SEMANTICS_DBG(anInstruction.identify() << " Allocate LSQ Entry");
    if (hasDependance) {
      anInstruction.core()->insertLSQ(
          boost::intrusive_ptr<Instruction>(&anInstruction), theOperation, theSize,
          theBypassSB, anInstruction.makeInstructionDependance(theDependance), theAccType);
    } else {
      anInstruction.core()->insertLSQ(boost::intrusive_ptr<Instruction>(&anInstruction),
                                      theOperation, theSize, theBypassSB, theAccType);
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Allocate LSQ Entry";
    Effect::describe(anOstream);
  }
};

Effect *allocateStore(SemanticInstruction *inst, eSize aSize, bool aBypassSB, eAccType type) {
  AllocateLSQEffect *ae = new AllocateLSQEffect(kStore, aSize, aBypassSB, type);
  inst->addNewComponent(ae);
  return ae;
}

Effect *allocateMEMBAR(SemanticInstruction *inst, eAccType type) {
  AllocateLSQEffect *ae = new AllocateLSQEffect(kMEMBARMarker, kWord, false, type);
  inst->addNewComponent(ae);
  return ae;
}

Effect *allocateLoad(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                     eAccType type) {
  AllocateLSQEffect *ae = new AllocateLSQEffect(kLoad, aSize, false, aDependance, type);
  inst->addNewComponent(ae);
  return ae;
}

Effect *allocateRMW(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    eAccType type) {
  AllocateLSQEffect *ae = new AllocateLSQEffect(kRMW, aSize, false, aDependance, type);
  inst->addNewComponent(ae);
  return ae;
}

struct EraseLSQEffect : public Effect {
  void invoke(SemanticInstruction &anInstruction) {
    DBG_(VVerb, (<< anInstruction.identify() << " EraseLSQEffect "));

    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction.identify() << " Free LSQ Entry"));
    anInstruction.core()->eraseLSQ(boost::intrusive_ptr<Instruction>(&anInstruction));
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Free LSQ Load Entry";
    Effect::describe(anOstream);
  }
};

Effect *eraseLSQ(SemanticInstruction *inst) {
  EraseLSQEffect *e = new EraseLSQEffect();
  inst->addNewComponent(e);
  return e;
}

struct RetireMemEffect : public Effect {
  void invoke(SemanticInstruction &anInstruction) {
    DBG_(VVerb, (<< anInstruction.identify() << " RetireMemEffect "));

    FLEXUS_PROFILE();
    anInstruction.core()->retireMem(boost::intrusive_ptr<Instruction>(&anInstruction));
    DBG_(VVerb, (<< anInstruction << " Retiring memory instruction"));
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Retire Memory";
    Effect::describe(anOstream);
  }
};

Effect *retireMem(SemanticInstruction *inst) {
  RetireMemEffect *e = new RetireMemEffect();
  inst->addNewComponent(e);
  return e;
}

struct CheckSysRegAccess : public Effect {

  ePrivRegs thePrivReg;
  uint8_t theIsRead;

  CheckSysRegAccess(ePrivRegs aPrivReg, uint8_t is_read)
      : thePrivReg(aPrivReg), theIsRead(is_read) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    if (!anInstruction.isAnnulled()) {
    }

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "CheckSysRegAccess";
    Effect::describe(anOstream);
  }
};

Effect *checkSysRegAccess(SemanticInstruction *inst, ePrivRegs aPrivReg, uint8_t is_read) {
  CheckSysRegAccess *e = new CheckSysRegAccess(aPrivReg, is_read);
  inst->addNewComponent(e);
  return e;
}

struct ClearExclusiveMonitor : public Effect {

  ClearExclusiveMonitor() {
  }
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    if (!anInstruction.isAnnulled()) {
      anInstruction.core()->clearExclusiveLocal();
      DBG_(Iface, (<< "Clearing Exclusive Monitor Local for " << anInstruction));
    }
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "ClearExclusiveMonitor";
    Effect::describe(anOstream);
  }
};

Effect *clearExclusiveMonitor(SemanticInstruction *inst) {
  ClearExclusiveMonitor *e = new ClearExclusiveMonitor();
  inst->addNewComponent(e);
  return e;
}

struct MarkExclusiveMonitor : public Effect {

  eOperandCode theAddressCode;
  eSize theSize;

  MarkExclusiveMonitor(eOperandCode anAddressCode, eSize aSize)
      : theAddressCode(anAddressCode), theSize(aSize) {
  }
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    if (!anInstruction.isAnnulled()) {

      uint64_t addr = anInstruction.operand<uint64_t>(theAddressCode);
      Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(anInstruction.cpu());
      PhysicalMemoryAddress pAddress =
          PhysicalMemoryAddress(c->translateVirtualAddress(VirtualMemoryAddress((addr >> 6) << 6),
              Flexus::Qemu::API::QEMU_Curr_Load));

      anInstruction.core()->markExclusiveGlobal(pAddress, theSize, kMonitorSet);
      anInstruction.core()->markExclusiveLocal(pAddress, theSize, kMonitorSet);
      anInstruction.core()->markExclusiveVA(VirtualMemoryAddress(pAddress), theSize,
                                            kMonitorSet); // optional
      DBG_(Iface, (<< "Marking Exclusive Monitor Local for " << anInstruction));
    }
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "MarkExclusiveMonitor";
    Effect::describe(anOstream);
  }
};

Effect *markExclusiveMonitor(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize) {
  MarkExclusiveMonitor *e = new MarkExclusiveMonitor(anAddressCode, aSize);
  inst->addNewComponent(e);
  return e;
}

// It is IMPLEMENTATION DEFINED whether the detection of memory aborts happens
// before or after the check on the local Exclusives monitor. As a result a failure
// of the local monitor can occur on some implementations even if the memory
// access would give an memory abort.
struct ExclusiveMonitorPass : public Effect {

  eOperandCode theAddressCode;
  eSize theSize;

  ExclusiveMonitorPass(eOperandCode anAddressCode, eSize aSize) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    uint64_t status = 1;
    uint64_t addr = anInstruction.operand<uint64_t>(theAddressCode);
    bool aligned = (uint64_t(addr) == align((uint64_t)addr, theSize * 8));

    if (!aligned) {
      anInstruction.setWillRaise(kException_StoreAddrMisaligned);
      anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                     anInstruction.willRaise());
    }

    bool passed = anInstruction.core()->isExclusiveVA(VirtualMemoryAddress(addr), theSize);
    if (!passed) {
      anInstruction.setWillRaise(kException_StoreAccessFault);
      anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                     anInstruction.willRaise());
    }
    PhysicalMemoryAddress pAddress = anInstruction.translate();

    passed = anInstruction.core()->isExclusiveLocal(pAddress, theSize);
    if (passed) {
      anInstruction.core()->clearExclusiveLocal();
      passed = anInstruction.core()->isExclusiveGlobal(pAddress, theSize);
      status = 0;
    } else {
      anInstruction.annul();
    }

    mapped_reg name = anInstruction.operand<mapped_reg>(kStatus);
    anInstruction.core()->writeRegister(name, status);

    Effect::invoke(anInstruction);
  }
  void describe(std::ostream &anOstream) const {
    anOstream << "ExclusiveMonitorPass";
    Effect::describe(anOstream);
  }
};

Effect *exclusiveMonitorPass(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize) {
  ExclusiveMonitorPass *e = new ExclusiveMonitorPass(anAddressCode, aSize);
  inst->addNewComponent(e);
  return e;
}

struct ExceptionEffect : public Effect {
  eExceptionType theType;
  ExceptionEffect(eExceptionType aType) : theType(aType) {
  }
  void invoke(SemanticInstruction &anInstruction) {
    if (!anInstruction.isAnnulled()) {
      anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction), theType);
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "ExceptionEffect";
    Effect::describe(anOstream);
  }
};

Effect *exceptionEffect(SemanticInstruction *inst, eExceptionType aType) {
  ExceptionEffect *e = new ExceptionEffect(aType);
  inst->addNewComponent(e);
  return e;
}

struct CommitStoreEffect : public Effect {
  void invoke(SemanticInstruction &anInstruction) {
    DBG_(Iface, (<< anInstruction.identify() << " CommitStoreEffect "));

    FLEXUS_PROFILE();
    anInstruction.core()->commitStore(boost::intrusive_ptr<Instruction>(&anInstruction));
    DBG_(Iface, (<< anInstruction << " Commit store instruction"));
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Commit Store";
    Effect::describe(anOstream);
  }
};

Effect *commitStore(SemanticInstruction *inst) {
  CommitStoreEffect *e = new CommitStoreEffect();
  inst->addNewComponent(e);
  return e;
}

struct AccessMemEffect : public Effect {
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction.identify()));

    anInstruction.core()->accessMem(anInstruction.getAccessAddress(),
                                    boost::intrusive_ptr<Instruction>(&anInstruction));
    DBG_(VVerb, (<< anInstruction << " Access memory: " << anInstruction.getAccessAddress()));
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "AccessMemEffect";
    Effect::describe(anOstream);
  }
};

Effect *accessMem(SemanticInstruction *inst) {
  AccessMemEffect *e = new AccessMemEffect();
  inst->addNewComponent(e);
  return e;
}

struct ForceResyncEffect : public Effect {
  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction.identify() << " ForceResyncEffect "));

    anInstruction.forceResync();
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "Force Resync";
    Effect::describe(anOstream);
  }
};

Effect *forceResync(SemanticInstruction *inst) {
  ForceResyncEffect *e = new ForceResyncEffect();
  inst->addNewComponent(e);
  return e;
}

struct MMUpageFaultCheckEffect : public Effect {
  MMUpageFaultCheckEffect() {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction << " MMUpageFaultCheckEffect ")); // NOOSHIN

    anInstruction.core()->checkPageFault(boost::intrusive_ptr<Instruction>(&anInstruction));

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "DMMU TranslationCheckEffect";
    Effect::describe(anOstream);
  }
};

Effect *mmuPageFaultCheck(SemanticInstruction *inst) {
  Effect *e = new MMUpageFaultCheckEffect();
  inst->addNewComponent(e);
  return e;
}

struct IMMUExceptionEffect : public Effect {
  IMMUExceptionEffect() {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();

    eExceptionType exception = anInstruction.retryTranslation();
    if (exception != 0) {
      anInstruction.setWillRaise(exception);
      anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                     anInstruction.willRaise());
    }

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "IMMU Exception";
    Effect::describe(anOstream);
  }
};

Effect *immuException(SemanticInstruction *inst) {
  Effect *e = new IMMUExceptionEffect();
  inst->addNewComponent(e);
  return e;
}

struct TlbiEffect: public Effect {
  eOperandCode theAddr;
  eOperandCode theAsid;

  TlbiEffect(eOperandCode addr, eOperandCode asid)
    : theAddr(addr), theAsid(asid) {
  }

  void invoke(SemanticInstruction &anInstruction) {
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction << " TlbiEffect "));

    anInstruction.core()->tlbi(boost::get<uint64_t>(anInstruction.operand(theAddr)),
                               boost::get<uint64_t>(anInstruction.operand(theAsid)));

    // also make no one waiting for annulled page walks
    anInstruction.forceResync();

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << "TlbiEffect";
    Effect::describe(anOstream);
  }
};

Effect *tlbiEffect(SemanticInstruction *inst, eOperandCode addr, eOperandCode asid) {
  Effect *e = new TlbiEffect(addr, asid);
  inst->addNewComponent(e);
  return e;
}

} // namespace nDecoder
