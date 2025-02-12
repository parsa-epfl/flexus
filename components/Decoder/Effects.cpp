
#include "components/Decoder/Effects.hpp"
#include "Interactions.hpp"
#include "SemanticInstruction.hpp"
#include "components/uArch/systemRegister.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "components/uFetch/uFetchTypes.hpp"
#include "core/debug/debug.hpp"
#include "core/types.hpp"

#include <core/performance/profile.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

#pragma GCC diagnostic ignored "-Wunused-variable"

void
DependanceTarget::invokeSatisfy(int32_t anArg)
{
    void (nDecoder::DependanceTarget::*satisfy_pt)(int32_t) = &nDecoder::DependanceTarget::satisfy;
    DBG_(VVerb, (<< std::hex << "Satisfy: " << satisfy_pt << "\n"));
    satisfy(anArg);
    DBG_(VVerb, (<< "After satisfy"));
}

void
EffectChain::invoke(SemanticInstruction& anInstruction)
{
    if (theFirst) { theFirst->invoke(anInstruction); }
}

void
EffectChain::describe(std::ostream& anOstream) const
{
    if (theFirst) { theFirst->describe(anOstream); }
}

void
EffectChain::append(Effect* anEffect)
{
    if (!theFirst) {
        theFirst = anEffect;
    } else {
        theLast->theNext = anEffect;
    }
    theLast = anEffect;

    // There should not be only one effect in the chain.
    DBG_Assert(anEffect->theNext == 0);
}

EffectChain::EffectChain()
  : theFirst(0)
  , theLast(0)
{
}

struct MapSourceEffect : public Effect
{
    eOperandCode theInputCode;
    eOperandCode theOutputCode;

    MapSourceEffect(eOperandCode anInputCode, eOperandCode anOutputCode)
      : theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        reg name(anInstruction.operand<reg>(theInputCode));
        mapped_reg mapped_name = anInstruction.core()->map(name);
        anInstruction.setOperand(theOutputCode, mapped_name);

        DECODER_DBG(anInstruction.identify() << theInputCode << " (" << name << ")"
                                             << " mapped to " << theOutputCode << "(" << mapped_name << ")");

        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "MapSource " << theInputCode << " store mapping in " << theOutputCode;
        Effect::describe(anOstream);
    }
};

Effect*
mapSource(SemanticInstruction* inst, eOperandCode anInputCode, eOperandCode anOutputCode)
{
    MapSourceEffect* r = new MapSourceEffect(anInputCode, anOutputCode);
    inst->addNewComponent(r);
    return r;
}

struct FreeMappingEffect : public Effect
{
    eOperandCode theMappingCode;

    FreeMappingEffect(eOperandCode aMappingCode)
      : theMappingCode(aMappingCode)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        mapped_reg mapping(anInstruction.operand<mapped_reg>(theMappingCode));
        DBG_(
          Iface,
          (<< anInstruction.identify() << " MapEffect free mapping for " << theMappingCode << "(" << mapping << ")"));
        anInstruction.core()->free(mapping);
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "FreeMapping " << theMappingCode;
        Effect::describe(anOstream);
    }
};

Effect*
freeMapping(SemanticInstruction* inst, eOperandCode aMapping)
{
    FreeMappingEffect* fme = new FreeMappingEffect(aMapping);
    inst->addNewComponent(fme);
    return fme;
}

struct RestoreMappingEffect : public Effect
{
    eOperandCode theNameCode;
    eOperandCode theMappingCode;

    RestoreMappingEffect(eOperandCode aNameCode, eOperandCode aMappingCode)
      : theNameCode(aNameCode)
      , theMappingCode(aMappingCode)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        reg name(anInstruction.operand<reg>(theNameCode));
        mapped_reg mapping(anInstruction.operand<mapped_reg>(theMappingCode));

        DBG_(VVerb,
             (<< anInstruction.identify() << " MapEffect restore mapping for " << theNameCode << "(" << name << ") to "
              << theMappingCode << "( " << mapping << ")"));
        anInstruction.core()->restore(name, mapping);
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "RestoreMapping " << theNameCode << " to " << theMappingCode;
        Effect::describe(anOstream);
    }
};

Effect*
restoreMapping(SemanticInstruction* inst, eOperandCode aName, eOperandCode aMapping)
{
    RestoreMappingEffect* rme = new RestoreMappingEffect(aName, aMapping);
    inst->addNewComponent(rme);
    return rme;
}

struct MapDestinationEffect : public Effect
{
    eOperandCode theInputCode;
    eOperandCode theOutputCode;
    eOperandCode thePreviousMappingCode;
    bool theSquashEffects;

    MapDestinationEffect(eOperandCode anInputCode,
                         eOperandCode anOutputCode,
                         eOperandCode aPreviousMappingCode,
                         bool aSquashEffects)
      : theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
      , thePreviousMappingCode(aPreviousMappingCode)
      , theSquashEffects(aSquashEffects)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        reg name(anInstruction.operand<reg>(theInputCode));
        mapped_reg mapped_name;
        mapped_reg previous_mapping;
        boost::tie(mapped_name, previous_mapping) = anInstruction.core()->create(name);
        anInstruction.setOperand(theOutputCode, mapped_name);
        anInstruction.setOperand(thePreviousMappingCode, previous_mapping);

        // Add effect to free previous mapping upon retirement
        Effect* e = new FreeMappingEffect(thePreviousMappingCode);
        anInstruction.addNewComponent(e);
        anInstruction.addCommitEffect(e);

        // Add effect to deallocate new register and restore previous mapping upon
        // squash
        if (theSquashEffects) {
            Effect* e = new RestoreMappingEffect(theInputCode, thePreviousMappingCode);
            anInstruction.addNewComponent(e);
            anInstruction.addSquashEffect(e);
            Effect* f = new FreeMappingEffect(theOutputCode);
            anInstruction.addNewComponent(f);
            anInstruction.addSquashEffect(f);
        }

        DECODER_DBG(anInstruction.identify()
                    << " Mapping " << theInputCode << "(" << name << ")"
                    << " to " << theOutputCode << "(" << mapped_name << ")"
                    << " previous mapping " << thePreviousMappingCode << "(" << previous_mapping << ")");
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "MapDestination " << theInputCode << ", store mapping in " << theOutputCode;
        Effect::describe(anOstream);
    }
};

Effect*
mapCCDestination(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kCCd, kCCpd, kPCCpd, true);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapDestination(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD, kPD, kPPD, true);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapRD1Destination(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD1, kPD1, kPPD1, true);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapRD2Destination(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD2, kPD2, kPPD2, true);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapDestination_NoSquashEffects(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD, kPD, kPPD, false);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapRD1Destination_NoSquashEffects(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD1, kPD1, kPPD1, false);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
mapRD2Destination_NoSquashEffects(SemanticInstruction* inst)
{
    MapDestinationEffect* mde = new MapDestinationEffect(kRD2, kPD2, kPPD2, false);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
unmapDestination(SemanticInstruction* inst)
{
    FreeMappingEffect* mde = new FreeMappingEffect(kPD);
    inst->addNewComponent(mde);
    return mde;
}

Effect*
unmapFDestination(SemanticInstruction* inst, int32_t anIndex)
{
    FreeMappingEffect* mde = new FreeMappingEffect(eOperandCode(kPFD0 + anIndex));
    inst->addNewComponent(mde);
    return mde;
}

Effect*
restorePreviousDestination(SemanticInstruction* inst)
{
    RestoreMappingEffect* rme = new RestoreMappingEffect(kRD, kPPD);
    inst->addNewComponent(rme);
    return rme;
}

struct SatisfyDependanceEffect : public Effect
{
    InternalDependance theDependance;

    SatisfyDependanceEffect(InternalDependance const& aDependance)
      : theDependance(aDependance)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();

        SEMANTICS_TRACE;
        theDependance.satisfy();
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "Satisfy Dependance Effect";
        Effect::describe(anOstream);
    }
};

struct SquashDependanceEffect : public Effect
{
    InternalDependance theDependance;

    SquashDependanceEffect(InternalDependance const& aDependance)
      : theDependance(aDependance)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction.identify() << " SquashDependanceEffect "));

        theDependance.squash();
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "Squash Dependance Effect";
        Effect::describe(anOstream);
    }
};

Effect*
satisfy(SemanticInstruction* inst, InternalDependance const& aDependance)
{
    SatisfyDependanceEffect* s = new SatisfyDependanceEffect(aDependance);
    inst->addNewComponent(s);
    return s;
}

Effect*
squash(SemanticInstruction* inst, InternalDependance const& aDependance)
{
    SquashDependanceEffect* s = new SquashDependanceEffect(aDependance);
    inst->addNewComponent(s);
    return s;
}

struct AnnulInstruction : public Interaction
{
    void operator()(boost::intrusive_ptr<Instruction> anInstruction, uArch& aCore)
    {
        DBG_(VVerb, (<< *anInstruction << " Annulled"));
        anInstruction->annul();
    }
    bool annuls() { return true; }
    void describe(std::ostream& anOstream) const { anOstream << "Annul Instruction"; }
};

Interaction*
annulInstructionInteraction()
{
    return new AnnulInstruction;
}

struct ReinstateInstruction : public Interaction
{
    void operator()(boost::intrusive_ptr<Instruction> anInstruction, uArch& aCore)
    {
        DBG_(VVerb, (<< *anInstruction << " Reinstated"));
        anInstruction->reinstate();
    }
    void describe(std::ostream& anOstream) const { anOstream << "Reinstate Instruction"; }
};

Interaction*
reinstateInstructionInteraction()
{
    return new ReinstateInstruction;
}

struct AnnulNextEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction.identify() << " Annul Next Instruction"));
        anInstruction.core()->applyToNext(boost::intrusive_ptr<Instruction>(&anInstruction), new AnnulInstruction());
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "Annul Next Instruction";
        Effect::describe(anOstream);
    }
};

Effect*
annulNext(SemanticInstruction* inst)
{
    AnnulNextEffect* a = new AnnulNextEffect();
    inst->addNewComponent(a);
    return a;
}

BranchInteraction::BranchInteraction(boost::intrusive_ptr<nuArch::Instruction> anIssuer)
  : theIssuer(anIssuer)
{
}

void
BranchInteraction::operator()(boost::intrusive_ptr<Instruction> anInstruction, uArch& aCore)
{
    DBG_(VVerb, (<< *anInstruction << " " << *this));
    // DBG_Assert(theIssuer->bpState()->theActualTarget != VirtualMemoryAddress(0),
    //            (<< "BranchInteraction invoked without a target")); // This is possible, because of the misprediction.
    if (anInstruction->pc() != theIssuer->bpState()->theActualTarget) {
        DBG_(Verb, (<< *anInstruction << " Branch Redirection."));
        if (aCore.squashFrom(anInstruction)) {
            boost::intrusive_ptr<BPredRedictRequest> aRequest = new BPredRedictRequest();
            aRequest->theTarget = theIssuer->bpState()->theActualTarget;
            aRequest->theBPState = theIssuer->bpState();
            aRequest->theInsertNewHistory = true;
            aCore.redirectFetch(aRequest);
        }
    }
}

void
BranchInteraction::describe(std::ostream& anOstream) const
{
    anOstream << "Branch to " << theIssuer->bpState()->theActualTarget;
}

Interaction*
branchInteraction(boost::intrusive_ptr<Instruction> anIssuer)
{
    return new BranchInteraction(anIssuer);
}


// struct BranchPredictorTrainingEffect : public Effect
// {
//     BranchPredictorTrainingEffect() {}

//     void invoke(SemanticInstruction &anInstruction)
//     {
//         FLEXUS_PROFILE();
//         DBG_(VVerb, (<< anInstruction << " BranchTrainingEffect "));
//         anInstruction.core()->trainingBranch(anInstruction.bpState());

//         Effect::invoke(anInstruction);
//     }

//     void describe(std::ostream &anOstream) const
//     {
//         anOstream << "Training Branch Predictor";
//         Effect::describe(anOstream);
//     }
// };

BranchPredictorTrainingEffect::BranchPredictorTrainingEffect() {

}

void BranchPredictorTrainingEffect::invoke(SemanticInstruction &anInstruction)
{
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< anInstruction << " BranchTrainingEffect "));
    anInstruction.core()->trainingBranch(anInstruction.bpState());

    Effect::invoke(anInstruction);
}

void BranchPredictorTrainingEffect::describe(std::ostream &anOstream) const
{
    anOstream << "Training Branch Predictor";
    Effect::describe(anOstream);
}

Effect *
branchPredictorTraining(SemanticInstruction* inst){
    BranchPredictorTrainingEffect *b = new BranchPredictorTrainingEffect();
    inst->addNewComponent(b);
    return b;
}

struct BranchEffect : public Effect
{
    VirtualMemoryAddress theTarget;
    bool theHasTarget;
    BranchEffect(VirtualMemoryAddress aTarget)
      : theTarget(aTarget)
      , theHasTarget(true)
    {
    }
    BranchEffect()
      : theHasTarget(false)
    {
    }
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction << " BranchEffect "));

        if (!theHasTarget) {
            Operand address = anInstruction.operand(kAddress);
            theTarget       = VirtualMemoryAddress(boost::get<uint64_t>(address));
        }

        // This effect currently is only used by call and unconditional branch instructions
        DBG_Assert(anInstruction.bpState()->theActualType == kUnconditional || anInstruction.bpState()->theActualType == kCall,
                   (<< "BranchEffect invoked on an instruction that is not a call or unconditional branch: " << anInstruction));

        // Update the actual target.
        anInstruction.bpState()->theActualTarget = theTarget;
        anInstruction.bpState()->theActualDirection = kTaken;

        anInstruction.redirectPC(theTarget);

        boost::intrusive_ptr<Instruction> anInstructionPtr{&anInstruction};
        anInstruction.core()->applyToNext(anInstructionPtr, branchInteraction(anInstructionPtr));
        DBG_(VVerb, (<< "BRANCH:  Must redirect to " << theTarget));
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream& anOstream) const
    {
        anOstream << " Branch to " << theTarget;
        Effect::describe(anOstream);
    }
};

Effect*
branch(SemanticInstruction* inst, VirtualMemoryAddress aTarget)
{
    BranchEffect* b = new BranchEffect(aTarget);
    inst->addNewComponent(b);
    return b;
}

struct AllocateLSQEffect : public Effect
{
    eOperation theOperation;
    eSize theSize;
    bool hasDependance;
    InternalDependance theDependance;
    bool theBypassSB;
    eAccType theAccType;

    AllocateLSQEffect(eOperation anOperation,
                      eSize aSize,
                      bool aBypassSB,
                      InternalDependance const& aDependance,
                      eAccType type)
      : theOperation(anOperation)
      , theSize(aSize)
      , hasDependance(true)
      , theDependance(aDependance)
      , theBypassSB(aBypassSB)
      , theAccType(type)
    {
    }
    AllocateLSQEffect(eOperation anOperation, eSize aSize, bool aBypassSB, eAccType type)
      : theOperation(anOperation)
      , theSize(aSize)
      , hasDependance(false)
      , theBypassSB(aBypassSB)
      , theAccType(type)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        SEMANTICS_DBG(anInstruction.identify() << " Allocate LSQ Entry");
        if (hasDependance) {
            anInstruction.core()->insertLSQ(boost::intrusive_ptr<Instruction>(&anInstruction),
                                            theOperation,
                                            theSize,
                                            theBypassSB,
                                            anInstruction.makeInstructionDependance(theDependance),
                                            theAccType);
        } else {
            anInstruction.core()->insertLSQ(boost::intrusive_ptr<Instruction>(&anInstruction),
                                            theOperation,
                                            theSize,
                                            theBypassSB,
                                            theAccType);
        }
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "Allocate LSQ Entry";
        Effect::describe(anOstream);
    }
};

Effect*
allocateStore(SemanticInstruction* inst, eSize aSize, bool aBypassSB, eAccType type)
{
    AllocateLSQEffect* ae = new AllocateLSQEffect(kStore, aSize, aBypassSB, type);
    inst->addNewComponent(ae);
    return ae;
}

Effect*
allocateMEMBAR(SemanticInstruction* inst, eAccType type)
{
    AllocateLSQEffect* ae = new AllocateLSQEffect(kMEMBARMarker, kWord, false, type);
    inst->addNewComponent(ae);
    return ae;
}

Effect*
allocateLoad(SemanticInstruction* inst, eSize aSize, InternalDependance const& aDependance, eAccType type)
{
    AllocateLSQEffect* ae = new AllocateLSQEffect(kLoad, aSize, false, aDependance, type);
    inst->addNewComponent(ae);
    return ae;
}

Effect*
allocateCAS(SemanticInstruction* inst, eSize aSize, InternalDependance const& aDependance, eAccType type)
{
    AllocateLSQEffect* ae = new AllocateLSQEffect(kCAS, aSize, false, aDependance, type);
    inst->addNewComponent(ae);
    return ae;
}

Effect*
allocateRMW(SemanticInstruction* inst, eSize aSize, InternalDependance const& aDependance, eAccType type)
{
    AllocateLSQEffect* ae = new AllocateLSQEffect(kRMW, aSize, false, aDependance, type);
    inst->addNewComponent(ae);
    return ae;
}

struct EraseLSQEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        DBG_(VVerb, (<< anInstruction.identify() << " EraseLSQEffect "));

        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction.identify() << " Free LSQ Entry"));
        anInstruction.core()->eraseLSQ(boost::intrusive_ptr<Instruction>(&anInstruction));
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << "Free LSQ Load Entry";
        Effect::describe(anOstream);
    }
};

Effect*
eraseLSQ(SemanticInstruction* inst)
{
    EraseLSQEffect* e = new EraseLSQEffect();
    inst->addNewComponent(e);
    return e;
}

struct RetireMemEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        DBG_(VVerb, (<< anInstruction.identify() << " RetireMemEffect "));

        FLEXUS_PROFILE();
        anInstruction.core()->retireMem(boost::intrusive_ptr<Instruction>(&anInstruction));
        DBG_(VVerb, (<< anInstruction << " Retiring memory instruction"));
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " Retire Memory";
        Effect::describe(anOstream);
    }
};

Effect*
retireMem(SemanticInstruction* inst)
{
    RetireMemEffect* e = new RetireMemEffect();
    inst->addNewComponent(e);
    return e;
}

struct CheckSysRegAccess : public Effect
{

    ePrivRegs thePrivReg;
    uint8_t theIsRead;

    CheckSysRegAccess(ePrivRegs aPrivReg, uint8_t is_read)
      : thePrivReg(aPrivReg)
      , theIsRead(is_read)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        if (!anInstruction.isAnnulled()) {}

        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " CheckSysRegAccess ";
        Effect::describe(anOstream);
    }
};

Effect*
checkSysRegAccess(SemanticInstruction* inst, ePrivRegs aPrivReg, uint8_t is_read)
{
    CheckSysRegAccess* e = new CheckSysRegAccess(aPrivReg, is_read);
    inst->addNewComponent(e);
    return e;
}

struct CheckSystemAccess : public Effect
{
    uint8_t theOp0, theOp1, theOp2, theCRn, theCRm, theRT, theRead;
    CheckSystemAccess(uint8_t anOp0,
                      uint8_t anOp1,
                      uint8_t anOp2,
                      uint8_t aCRn,
                      uint8_t aCRm,
                      uint8_t aRT,
                      uint8_t aRead)
      : theOp0(anOp0)
      , theOp1(anOp1)
      , theOp2(anOp2)
      , theCRn(aCRn)
      , theCRm(aCRm)
      , theRT(aRT)
      , theRead(aRead)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        if (!anInstruction.isAnnulled()) {
#if 0
      // Perform the generic checks that an AArch64 MSR/MRS/SYS instruction is valid at the
      // current exception level, based on the opcode's 'op1' field value.
      // Further checks for enables/disables/traps specific to a particular system register
      // or operation will be performed in System_Put(), System_Get(), SysOp_W(), or SysOp_R().
      /* 64 bit registers have only CRm and Opc1 fields */

      std::unique_ptr<SysRegInfo> ri = getPriv(theNo);
      ri->setSystemRegisterEncodingValues(theNo);

      DBG_Assert(!((ri->type & kARM_64BIT) && (ri->opc2 || ri->crn)));
      /* op0 only exists in the AArch64 encodings */
      DBG_Assert((ri->state != kARM_STATE_AA32) || (ri->opc0 == 0));
      /* AArch64 regs are all 64 bit so ARM_CP_64BIT is meaningless */
      DBG_Assert((ri->state != kARM_STATE_AA64) || !(ri->type & kARM_64BIT));
      /* The AArch64 pseudocode CheckSystemAccess() specifies that op1
       * encodes a minimum access level for the register. We roll this
       * runtime check into our general permission check code, so check
       * here that the reginfo's specified permissions are strict enough
       * to encompass the generic architectural permission check.
       */
      if (ri->state != kARM_STATE_AA32) {
        int mask = 0;
        switch (ri->opc1) {
        case 0:
        case 1:
        case 2:
          /* min_EL EL1 */
          mask = kPL1_RW;
          break;
        case 3:
          /* min_EL EL0 */
          mask = kPL0_RW;
          break;
        case 4:
          /* min_EL EL2 */
          mask = kPL2_RW;
          break;
        case 5:
          /* unallocated encoding, so not possible */
          DBG_Assert(false);
          break;
        case 6:
          /* min_EL EL3 */
          mask = kPL3_RW;
          break;
        case 7:
          /* min_EL EL1, secure mode only (we don't check the latter) */
          mask = kPL1_RW;
          break;
        default:
          /* broken reginfo with out-of-range opc1 */
          DBG_Assert(false);
          break;
        }
        /* assert our permissions are not too lax (stricter is fine) */
        DBG_Assert((ri->access & ~mask) == 0);

        // Check for traps on access to all other system registers
        if (ri->accessfn(anInstruction.core()) != kACCESS_OK) {
          anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
          anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                         anInstruction.willRaise());
        }
      }
#endif
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " CheckSystemAccess ";
        Effect::describe(anOstream);
    }
};

Effect*
checkSystemAccess(SemanticInstruction* inst,
                  uint8_t anOp0,
                  uint8_t anOp1,
                  uint8_t anOp2,
                  uint8_t aCRn,
                  uint8_t aCRm,
                  uint8_t aRT,
                  uint8_t aRead)
{
    CheckSystemAccess* e = new CheckSystemAccess(anOp0, anOp1, anOp2, aCRn, aCRm, aRT, aRead);
    inst->addNewComponent(e);
    return e;
}

struct CheckDAIFAccess : public Effect
{
    uint8_t theOp1;
    CheckDAIFAccess(uint8_t anOp1)
      : theOp1(anOp1)
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        if (!anInstruction.isAnnulled()) {
            if (theOp1 == 0x3 /*011*/ && anInstruction.core()->_PSTATE().EL() == EL0 &&
                anInstruction.core()->_SCTLR(EL0).UMA() == 0) {
                anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
                anInstruction.core()->takeTrap(boost::intrusive_ptr<nuArch::Instruction>(&anInstruction),
                                               anInstruction.willRaise());
            }
        }
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream& anOstream) const
    {
        anOstream << " CheckDAIFAccess";
        Effect::describe(anOstream);
    }
};

Effect*
checkDAIFAccess(SemanticInstruction* inst, uint8_t anOp1)
{
    CheckDAIFAccess* e = new CheckDAIFAccess(anOp1);
    inst->addNewComponent(e);
    return e;
}

struct ReadPREffect : public Effect
{
    ePrivRegs thePR;
    std::unique_ptr<SysRegInfo> ri;

    ReadPREffect(ePrivRegs aPR, std::unique_ptr<SysRegInfo> anRi)
      : thePR(aPR)
      , ri(std::move(anRi))
    {
    }

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        if (!anInstruction.isAnnulled()) {
            // uint64_t pr = anInstruction.core()->readPR(thePR);
            mapped_reg name = anInstruction.operand<mapped_reg>(kPD);
            uint64_t prVal  = ri->readfn(anInstruction.core());
            DBG_(VVerb, (<< anInstruction << " Read " << ri->name << " value= " << std::hex << prVal << std::dec));

            anInstruction.setOperand(kResult, prVal);

            anInstruction.core()->writeRegister(name, prVal);
            anInstruction.core()->bypass(name, prVal);
        } else {
            // ReadPR was annulled.  Copy PPD to PD
            mapped_reg dest    = anInstruction.operand<mapped_reg>(kPD);
            mapped_reg prev    = anInstruction.operand<mapped_reg>(kPPD);
            register_value val = anInstruction.core()->readRegister(prev);
            anInstruction.core()->writeRegister(dest, val);
            anInstruction.setOperand(kResult, val);
        }
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " Read PR " << thePR;
        Effect::describe(anOstream);
    }
};

Effect*
readPR(SemanticInstruction* inst, ePrivRegs aPR, std::unique_ptr<SysRegInfo> ri)
{
    ReadPREffect* e = new ReadPREffect(aPR, std::move(ri));
    inst->addNewComponent(e);
    return e;
}

struct WritePREffect : public Effect {
 ePrivRegs thePR;
 std::unique_ptr<SysRegInfo> ri;
 WritePREffect(ePrivRegs aPR, std::unique_ptr<SysRegInfo> anRI) : thePR(aPR), ri(std::move(anRI)) {
 }

 void invoke(SemanticInstruction &anInstruction) {
   FLEXUS_PROFILE();
   if (!anInstruction.isAnnulled()) {
     uint64_t rs = 0;
     if (anInstruction.hasOperand(kResult)) {
       rs = anInstruction.operand<uint64_t>(kResult);
     } else if (anInstruction.hasOperand(kResult1)) {
       rs = anInstruction.operand<uint64_t>(kResult1);
     }
     DBG_(VVerb,
          (<< anInstruction << " Write " << ri->name << " value= " << std::hex << rs << std::dec));

     ri->writefn(anInstruction.core(), (uint64_t)rs);
   }
   Effect::invoke(anInstruction);
 }

 void describe(std::ostream &anOstream) const {
   anOstream << " Write PR " << thePR;
   Effect::describe(anOstream);
 }
};

Effect *writePR(SemanticInstruction *inst, ePrivRegs aPR, std::unique_ptr<SysRegInfo> anRI) {
 WritePREffect *e = new WritePREffect(aPR, std::move(anRI));
 inst->addNewComponent(e);
 return e;
}

struct WritePSTATE : public Effect {
 uint8_t theOp1, theOp2;
 WritePSTATE(uint8_t anOp1, uint8_t anOp2) : theOp1(anOp1), theOp2(anOp2) {
 }

 void invoke(SemanticInstruction &anInstruction) {
   FLEXUS_PROFILE();
   if (!anInstruction.isAnnulled()) {

     uint64_t val = anInstruction.operand<uint64_t>(kResult);
     switch ((theOp1 << 3) | theOp2) {
     case 0x3:
     case 0x4:
       anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
       anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                      anInstruction.willRaise());
       break;
     case 0x5: // sp
     {
       std::unique_ptr<SysRegInfo> ri = getPriv(kSPSel);
       ri->writefn(anInstruction.core(), (uint64_t)(val & 1));
       break;
     }
     case 0x1e: // daif set
       anInstruction.core()->setDAIF((uint32_t)val | anInstruction.core()->_PSTATE().DAIF());
       break;
     case 0x1f: // daif clr
       anInstruction.core()->setDAIF((uint32_t)(~val & anInstruction.core()->_PSTATE().DAIF()));
       break;
     default:
       anInstruction.setWillRaise(kException_UNCATEGORIZED);
       anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                      anInstruction.willRaise());
       break;
     }
   }
   Effect::invoke(anInstruction);
 }

 void describe(std::ostream &anOstream) const {
   anOstream << " Write PSTATE ";
   Effect::describe(anOstream);
 }
};

Effect *writePSTATE(SemanticInstruction *inst, uint8_t anOp1, uint8_t anOp2) {
 Effect *e = new WritePSTATE(anOp1, anOp2);
 inst->addNewComponent(e);
 return e;
}
//
// struct WriteNZCV : public Effect {
//  WriteNZCV() {
//  }
//
//  void invoke(SemanticInstruction &anInstruction) {
//    FLEXUS_PROFILE();
//    if (!anInstruction.isAnnulled()) {
//      std::unique_ptr<SysRegInfo> ri = getPriv(kNZCV);
//
//      uint64_t res = anInstruction.operand<uint64_t>(kResult);
//      uint64_t val = 0;
//      val = PSTATE_N & res;
//      if (res == 0)
//        val |= PSTATE_Z;
//
//      ri->writefn(anInstruction.core(), (uint64_t)val);
//    }
//    Effect::invoke(anInstruction);
//  }
//
//  void describe(std::ostream &anOstream) const {
//    anOstream << " Write NZCV ";
//    Effect::describe(anOstream);
//  }
//};
//
// Effect *writeNZCV(SemanticInstruction *inst) {
//  WriteNZCV *e = new WriteNZCV();
//  inst->addNewComponent(e);
//  return e;
//}
// #endif

struct ClearExclusiveMonitor : public Effect
{

    ClearExclusiveMonitor() {}
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        if (!anInstruction.isAnnulled()) {
            anInstruction.core()->clearExclusiveLocal();
            DBG_(VVerb, (<< "Clearing Exclusive Monitor Local for " << anInstruction));
        }
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream& anOstream) const
    {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};

Effect*
clearExclusiveMonitor(SemanticInstruction* inst)
{
    ClearExclusiveMonitor* e = new ClearExclusiveMonitor();
    inst->addNewComponent(e);
    return e;
}

struct MarkExclusiveMonitor : public Effect
{

    eOperandCode theAddressCode;
    eSize theSize;

    MarkExclusiveMonitor(eOperandCode anAddressCode, eSize aSize)
      : theAddressCode(anAddressCode)
      , theSize(aSize)
    {
    }
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        if (!anInstruction.isAnnulled()) {

            uint64_t addr             = anInstruction.operand<uint64_t>(theAddressCode);
            Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(anInstruction.cpu());
            PhysicalMemoryAddress pAddress =
              PhysicalMemoryAddress(c.translate_va2pa(VirtualMemoryAddress((addr >> 6) << 6), anInstruction.unprivAccess()));

            anInstruction.core()->markExclusiveGlobal(pAddress, theSize, kMonitorSet);
            anInstruction.core()->markExclusiveLocal(pAddress, theSize, kMonitorSet);
            anInstruction.core()->markExclusiveVA(VirtualMemoryAddress(pAddress), theSize,
                                                  kMonitorSet); // optional
            DBG_(VVerb, (<< "Marking Exclusive Monitor Local for " << anInstruction));
        }
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream& anOstream) const
    {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};

Effect*
markExclusiveMonitor(SemanticInstruction* inst, eOperandCode anAddressCode, eSize aSize)
{
    MarkExclusiveMonitor* e = new MarkExclusiveMonitor(anAddressCode, aSize);
    inst->addNewComponent(e);
    return e;
}

// It is IMPLEMENTATION DEFINED whether the detection of memory aborts happens
// before or after the check on the local Exclusives monitor. As a result a failure
// of the local monitor can occur on some implementations even if the memory
// access would give an memory abort.
struct ExclusiveMonitorPass : public Effect
{

    eOperandCode theAddressCode;
    eSize theSize;

    ExclusiveMonitorPass(eOperandCode anAddressCode, eSize aSize) {}

    void invoke(SemanticInstruction& anInstruction)
    {
        uint64_t status = 1;
        uint64_t addr   = anInstruction.operand<uint64_t>(theAddressCode);
        bool aligned    = (uint64_t(addr) == align((uint64_t)addr, theSize * 8));

        if (!aligned) {
            anInstruction.setWillRaise(kException_DATAABORT);
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                           anInstruction.willRaise());
        }

        bool passed = anInstruction.core()->isExclusiveVA(VirtualMemoryAddress(addr), theSize);
        if (!passed) {
            anInstruction.setWillRaise(kException_DATAABORT);
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
    void describe(std::ostream& anOstream) const
    {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};

Effect*
exclusiveMonitorPass(SemanticInstruction* inst, eOperandCode anAddressCode, eSize aSize)
{
    ExclusiveMonitorPass* e = new ExclusiveMonitorPass(anAddressCode, aSize);
    inst->addNewComponent(e);
    return e;
}

struct ExceptionEffect : public Effect
{
    eExceptionType theType;
    ExceptionEffect(eExceptionType aType)
      : theType(aType)
    {
    }
    void invoke(SemanticInstruction& anInstruction)
    {
        if (!anInstruction.isAnnulled()) {
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction), theType);
        }
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " ExceptionEffect ";
        Effect::describe(anOstream);
    }
};

Effect*
exceptionEffect(SemanticInstruction* inst, eExceptionType aType)
{
    ExceptionEffect* e = new ExceptionEffect(aType);
    inst->addNewComponent(e);
    return e;
}

struct CommitStoreEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        DBG_(VVerb, (<< anInstruction.identify() << " CommitStoreEffect "));

        FLEXUS_PROFILE();
        anInstruction.core()->commitStore(boost::intrusive_ptr<Instruction>(&anInstruction));
        DBG_(VVerb, (<< anInstruction << " Commit store instruction"));
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " Commit Store";
        Effect::describe(anOstream);
    }
};

Effect*
commitStore(SemanticInstruction* inst)
{
    CommitStoreEffect* e = new CommitStoreEffect();
    inst->addNewComponent(e);
    return e;
}

struct AccessMemEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction.identify()));

        anInstruction.core()->accessMem(anInstruction.getAccessAddress(),
                                        boost::intrusive_ptr<Instruction>(&anInstruction));
        DBG_(VVerb, (<< anInstruction << " Access memory: " << anInstruction.getAccessAddress()));
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " AccessMemEffect";
        Effect::describe(anOstream);
    }
};

Effect*
accessMem(SemanticInstruction* inst)
{
    AccessMemEffect* e = new AccessMemEffect();
    inst->addNewComponent(e);
    return e;
}

struct ForceResyncEffect : public Effect
{
    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction.identify() << " ForceResyncEffect "));

        anInstruction.forceResync();
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " Force Resync ";
        Effect::describe(anOstream);
    }
};

Effect*
forceResync(SemanticInstruction* inst)
{
    ForceResyncEffect* e = new ForceResyncEffect();
    inst->addNewComponent(e);
    return e;
}

struct MMUpageFaultCheckEffect : public Effect
{
    MMUpageFaultCheckEffect() {}

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< anInstruction << " MMUpageFaultCheckEffect ")); // NOOSHIN

        anInstruction.core()->checkPageFault(boost::intrusive_ptr<Instruction>(&anInstruction));

        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " DMMU TranslationCheckEffect";
        Effect::describe(anOstream);
    }
};

Effect*
mmuPageFaultCheck(SemanticInstruction* inst)
{
    Effect* e = new MMUpageFaultCheckEffect();
    inst->addNewComponent(e);
    return e;
}

struct IMMUExceptionEffect : public Effect
{
    IMMUExceptionEffect() {}

    void invoke(SemanticInstruction& anInstruction)
    {
        FLEXUS_PROFILE();

        eExceptionType exception = anInstruction.retryTranslation();
        if (exception != 0) {
            anInstruction.setWillRaise(exception);
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction),
                                           anInstruction.willRaise());
        }

        Effect::invoke(anInstruction);
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << " IMMU Exception ";
        Effect::describe(anOstream);
    }
};

Effect*
immuException(SemanticInstruction* inst)
{
    Effect* e = new IMMUExceptionEffect();
    inst->addNewComponent(e);
    return e;
}

} // namespace nDecoder
