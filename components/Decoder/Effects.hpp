
#ifndef FLEXUS_DECODER_EFFECTS_HPP_INCLUDED
#define FLEXUS_DECODER_EFFECTS_HPP_INCLUDED

#include "InstructionComponentBuffer.hpp"
#include "OperandCode.hpp"

#include <components/uArch/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>

namespace nDecoder {

using Flexus::SharedTypes::VirtualMemoryAddress;

class BaseSemanticAction;
struct SemanticInstruction;
struct Condition;

struct Effect : UncountedComponent
{
    Effect* theNext;
    Effect()
      : theNext(0)
    {
    }
    virtual ~Effect() {}
    virtual void invoke(SemanticInstruction& anInstruction)
    {
        if (theNext) { theNext->invoke(anInstruction); }
    }
    virtual void describe(std::ostream& anOstream) const
    {
        if (theNext) { theNext->describe(anOstream); }
    }
    // NOTE: No virtual destructor because effects are never destructed.
};

struct EffectChain
{
    Effect* theFirst;
    Effect* theLast;
    EffectChain();
    void invoke(SemanticInstruction& anInstruction);
    void describe(std::ostream& anOstream) const;
    void append(Effect* anEffect);
    bool empty() const { return theFirst == 0; }
};

struct DependanceTarget
{
    void invokeSatisfy(int32_t anArg);
    void invokeSquash(int32_t anArg) { squash(anArg); }
    virtual ~DependanceTarget() {}
    virtual void satisfy(int32_t anArg) = 0;
    virtual void squash(int32_t anArg)  = 0;

  protected:
    DependanceTarget() {}
};

struct InternalDependance
{
    DependanceTarget* theTarget;
    int32_t theArg;
    InternalDependance()
      : theTarget(0)
      , theArg(0)
    {
    }
    InternalDependance(InternalDependance const& id)
      : theTarget(id.theTarget)
      , theArg(id.theArg)
    {
    }
    InternalDependance(DependanceTarget* tgt, int32_t arg)
      : theTarget(tgt)
      , theArg(arg)
    {
    }
    void satisfy() { theTarget->satisfy(theArg); }
    void squash() { theTarget->squash(theArg); }
};

Effect*
mapSource(SemanticInstruction* inst, eOperandCode anInputCode, eOperandCode anOutputCode);
Effect*
freeMapping(SemanticInstruction* inst, eOperandCode aMapping);
Effect*
disconnectRegister(SemanticInstruction* inst, eOperandCode aMapping);
Effect*
mapCCDestination(SemanticInstruction* inst);
Effect*
mapDestination(SemanticInstruction* inst);
Effect*
mapRD1Destination(SemanticInstruction* inst);
Effect*
mapRD2Destination(SemanticInstruction* inst);
Effect*
mapDestination_NoSquashEffects(SemanticInstruction* inst);
Effect*
mapRD1Destination_NoSquashEffects(SemanticInstruction* inst);
Effect*
mapRD2Destination_NoSquashEffects(SemanticInstruction* inst);
Effect*
unmapDestination(SemanticInstruction* inst);
Effect*
mapFDestination(SemanticInstruction* inst, int32_t anIndex);
Effect*
unmapFDestination(SemanticInstruction* inst, int32_t anIndex);
Effect*
restorePreviousDestination(SemanticInstruction* inst);
Effect*
satisfy(SemanticInstruction* inst, InternalDependance const& aDependance);
Effect*
squash(SemanticInstruction* inst, InternalDependance const& aDependance);
Effect*
annulNext(SemanticInstruction* inst);
Effect*
branch(SemanticInstruction* inst, VirtualMemoryAddress aTarget);
Effect*
returnFromTrap(SemanticInstruction* inst, bool isDone);
Effect*
branchAfterNext(SemanticInstruction* inst, VirtualMemoryAddress aTarget);
Effect*
branchAfterNext(SemanticInstruction* inst, eOperandCode aCode);
Effect*
branchConditionally(SemanticInstruction* inst,
                    VirtualMemoryAddress aTarget,
                    bool anAnnul,
                    Condition& aCondition,
                    bool isFloating);
Effect*
branchRegConditionally(SemanticInstruction* inst, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition);
Effect*
allocateLoad(SemanticInstruction* inst,
             nuArch::eSize aSize,
             InternalDependance const& aDependance,
             nuArch::eAccType type);
Effect*
allocateCAS(SemanticInstruction* inst,
            nuArch::eSize aSize,
            InternalDependance const& aDependance,
            nuArch::eAccType type);
Effect*
allocateCAS(SemanticInstruction* inst,
            nuArch::eSize aSize,
            InternalDependance const& aDependance,
            nuArch::eAccType type);
Effect*
allocateCASP(SemanticInstruction* inst,
             nuArch::eSize aSize,
             InternalDependance const& aDependance,
             nuArch::eAccType type);
Effect*
allocateCAS(SemanticInstruction* inst,
            nuArch::eSize aSize,
            InternalDependance const& aDependance,
            nuArch::eAccType type);
Effect*
allocateRMW(SemanticInstruction* inst,
            nuArch::eSize aSize,
            InternalDependance const& aDependance,
            nuArch::eAccType type);
Effect*
eraseLSQ(SemanticInstruction* inst);
Effect*
allocateStore(SemanticInstruction* inst, nuArch::eSize aSize, bool aBypassSB, nuArch::eAccType type);
Effect*
allocateMEMBAR(SemanticInstruction* inst, nuArch::eAccType type);
Effect*
retireMem(SemanticInstruction* inst);
Effect*
commitStore(SemanticInstruction* inst);
Effect*
accessMem(SemanticInstruction* inst);
Effect*
updateConditional(SemanticInstruction* inst);
Effect*
updateUnconditional(SemanticInstruction* inst, VirtualMemoryAddress aTarget);
Effect*
updateUnconditional(SemanticInstruction* inst, eOperandCode anOperandCode);
Effect*
updateCall(SemanticInstruction* inst, VirtualMemoryAddress aTarget);
Effect*
updateIndirect(SemanticInstruction* inst, eOperandCode anOperandCode, nuArch::eBranchType aType);
Effect*
updateNonBranch(SemanticInstruction* inst);
Effect*
readPR(SemanticInstruction* inst, nuArch::ePrivRegs aPR, std::unique_ptr<nuArch::SysRegInfo> aRI);
Effect*
writePR(SemanticInstruction* inst, nuArch::ePrivRegs aPR, std::unique_ptr<nuArch::SysRegInfo> aRI);
Effect*
writePSTATE(SemanticInstruction* inst, uint8_t anOp1, uint8_t anOp2);
Effect*
writeNZCV(SemanticInstruction* inst);
Effect*
clearExclusiveMonitor(SemanticInstruction* inst);
Effect*
SystemRegisterTrap(SemanticInstruction* inst);
Effect*
checkSystemAccess(SemanticInstruction* inst,
                  uint8_t anOp0,
                  uint8_t anOp1,
                  uint8_t anOp2,
                  uint8_t aCRn,
                  uint8_t aCRm,
                  uint8_t aRT,
                  uint8_t aRead);
Effect*
exceptionEffect(SemanticInstruction* inst, nuArch::eExceptionType aType);
Effect*
markExclusiveMonitor(SemanticInstruction* inst, eOperandCode anAddressCode, nuArch::eSize aSize);
Effect*
exclusiveMonitorPass(SemanticInstruction* inst,
                     eOperandCode anAddressCode,
                     nuArch::eSize aSize,
                     InternalDependance const& aDependance);

Effect*
checkDAIFAccess(SemanticInstruction* inst, uint8_t anOp1);
Effect*
checkSysRegAccess(SemanticInstruction* inst, nuArch::ePrivRegs aPrivReg, uint8_t is_read);

Effect*
mapXTRA(SemanticInstruction* inst);
Effect*
forceResync(SemanticInstruction* inst);
Effect*
immuException(SemanticInstruction* inst);
Effect*
mmuPageFaultCheck(SemanticInstruction* inst);

} // namespace nDecoder

#endif // FLEXUS_DECODER_EFFECTS_HPP_INCLUDED
