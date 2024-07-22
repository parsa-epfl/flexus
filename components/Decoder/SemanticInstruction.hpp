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

#ifndef FLEXUS_DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
#define FLEXUS_DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED

#include "components/uArch/uArchInterfaces.hpp"
#include "Effects.hpp"
#include "Instruction.hpp"
#include "OperandMap.hpp"

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

    boost::intrusive_ptr<BranchFeedback> theBranchFeedback;

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

    void setBranchFeedback(boost::intrusive_ptr<BranchFeedback> aFeedback) { theBranchFeedback = aFeedback; }
    boost::intrusive_ptr<BranchFeedback> branchFeedback() const { return theBranchFeedback; }
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