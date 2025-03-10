
#ifndef FLEXUS_DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
#define FLEXUS_DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED

#include "Effects.hpp"
#include "Instruction.hpp"
#include "OperandMap.hpp"
#include "components/uArch/uArchInterfaces.hpp"

namespace nDecoder {

struct simple_action;

struct SemanticInstruction : public ArchInstruction
{
  private:
    OperandMap theOperands;

    mutable InstructionComponentBuffer theICB;

    EffectChain theDispatchEffects;
    std::list<BaseSemanticAction*> theDispatchActions;
    EffectChain theSquashEffects;
    EffectChain theRetirementEffects;
    EffectChain theCheckTrapEffects;
    EffectChain theCommitEffects;
    EffectChain theAnnulmentEffects;
    EffectChain theReinstatementEffects;

    std::list<std::function<bool()>> theRetirementConstraints;

    std::list<std::function<bool()>> thePreValidations;
    std::list<std::function<bool()>> thePostValidations;
    std::list<std::function<void()>> theOverrideFns;

    bool theOverrideSimics;
    bool thePrevalidationsPassed;
    bool theRetirementDepends[4];
    int32_t theRetireDepCount;
    bool theIsMicroOp;

    struct Dep : public DependanceTarget
    {
        SemanticInstruction& theInstruction;
        Dep(SemanticInstruction& anInstruction)
          : theInstruction(anInstruction)
        {
        }
        void satisfy(int32_t anArg) { theInstruction.setMayRetire(anArg, true); }
        void squash(int32_t anArg) { theInstruction.setMayRetire(anArg, false); }
    } theRetirementTarget;

    boost::optional<PhysicalMemoryAddress> theAccessAddress;

    // This is a better-than-nothing way of accounting for the time taken by
    // non-memory istructions When an instruction dispatches, this counter is set
    // to the earliest possible cycle it could retire Right now this is just
    // current cycle + functional unit latency The uarch decrements this counter
    // by 1 every cycle; the instruction can retire when it is 0
    uint32_t theCanRetireCounter;

    void constructorInitValidations();
    void constructorTrackLiveInsns();

  public:
    void setCanRetireCounter(const uint32_t numCycles);
    void decrementCanRetireCounter();

    SemanticInstruction(VirtualMemoryAddress aPC,
                        Opcode anOpcode,
                        boost::intrusive_ptr<BPredState> bp_state,
                        uint32_t aCPU,
                        int64_t aSequenceNo);

    /* Constructor w. explicit Class & Code */
    SemanticInstruction(VirtualMemoryAddress aPC,
                        Opcode anOpcode,
                        boost::intrusive_ptr<BPredState> bp_state,
                        uint32_t aCPU,
                        int64_t aSequenceNo,
                        eInstructionClass aClass,
                        eInstructionCode aCode);
    virtual ~SemanticInstruction();

    size_t addNewComponent(UncountedComponent* aComponent);

    bool advancesSimics() const;
    void setIsMicroOp(bool isUop) { theIsMicroOp = isUop; }
    bool isMicroOp() const { return theIsMicroOp; }

    bool preValidate();
    bool postValidate();
    void doDispatchEffects();
    void squash();
    void pageFault();
    bool isPageFault() const;
    void doRetirementEffects();
    void checkTraps();
    void doCommitEffects();
    void annul();
    void reinstate();
    void overrideSimics();
    bool willOverrideSimics() const { return theOverrideSimics; }

    bool mayRetire() const;

    eExceptionType retryTranslation();
    PhysicalMemoryAddress translate();

    nuArch::InstructionDependance makeInstructionDependance(InternalDependance const& aDependance);

  public:
    template<class T>
    void setOperand(eOperandCode anOperand, T aT)
    {
        theOperands.set<T>(anOperand, aT);
    }

    void setOperand(eOperandCode anOperand, Operand const& aT)
    {
        SEMANTICS_DBG(identify() << ": [ " << anOperand << "->" << std::hex << aT << " ]");
        theOperands.set(anOperand, aT);
    }

    template<class T>
    T& operand(eOperandCode anOperand);

    Operand& operand(eOperandCode anOperand)
    {
        DBG_Assert(theOperands.hasOperand(anOperand),
                   (<< "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n"
                    << std::internal << *this << std::left));
        SEMANTICS_DBG(identify() << "requesting " << anOperand);
        return theOperands.operand(anOperand);
    }

    bool hasOperand(eOperandCode anOperand) { return theOperands.hasOperand(anOperand); }

    void addDispatchEffect(Effect*);
    void addDispatchAction(simple_action const&);
    void addRetirementEffect(Effect*);
    void addCheckTrapEffect(Effect*);
    void addCommitEffect(Effect*);
    void addSquashEffect(Effect*);
    void addAnnulmentEffect(Effect*);
    void addReinstatementEffect(Effect*);

    void addRetirementConstraint(std::function<bool()>);

    void addOverride(std::function<void()> anOverrideFn);
    void addPrevalidation(std::function<bool()> aValidation);
    void addPostvalidation(std::function<bool()> aValidation);

    void describe(std::ostream& anOstream) const;

    void setMayRetire(int32_t aBit, bool aFlag);

    InternalDependance retirementDependance();

    void setAccessAddress(PhysicalMemoryAddress anAddress) { theAccessAddress = anAddress; }
    PhysicalMemoryAddress getAccessAddress() const
    {
        return theAccessAddress ? *theAccessAddress : PhysicalMemoryAddress(0);
    }
};

template<>
inline nuArch::mapped_reg&
SemanticInstruction::operand<nuArch::mapped_reg>(eOperandCode anOperand)
{
    DBG_Assert(theOperands.hasOperand(anOperand),
               (<< "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n"
                << std::internal << *this << std::left));
    return theOperands.operand<mapped_reg>(anOperand);
}
template<>
inline nuArch::reg&
SemanticInstruction::operand<nuArch::reg>(eOperandCode anOperand)
{
    return theOperands.operand<reg>(anOperand);
}

template<>
inline bits&
SemanticInstruction::operand<bits>(eOperandCode anOperand)
{
    return theOperands.operand<bits>(anOperand);
}

template<>
inline uint64_t&
SemanticInstruction::operand<uint64_t>(eOperandCode anOperand)
{
    return theOperands.operand<uint64_t>(anOperand);
}
template<>
inline int64_t&
SemanticInstruction::operand<int64_t>(eOperandCode anOperand)
{
    return theOperands.operand<int64_t>(anOperand);
}

} // namespace nDecoder

#endif // FLEXUS_DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED