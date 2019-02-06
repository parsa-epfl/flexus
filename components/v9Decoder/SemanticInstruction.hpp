#ifndef FLEXUS_v9DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
#define FLEXUS_v9DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

#include <components/uArch/uArchInterfaces.hpp>
#include "v9Instruction.hpp"
#include "OperandMap.hpp"
#include "InstructionComponentBuffer.hpp"
#include "Effects.hpp"

namespace nv9Decoder {

struct simple_action;

struct SemanticInstruction : public v9Instruction {
private:
  OperandMap theOperands;

  mutable InstructionComponentBuffer theICB;

  EffectChain theDispatchEffects;
  std::list< BaseSemanticAction * > theDispatchActions;
  EffectChain theSquashEffects;
  EffectChain theRetirementEffects;
  EffectChain theCheckTrapEffects;
  EffectChain theCommitEffects;
  EffectChain theAnnulmentEffects;
  EffectChain theReinstatementEffects;

  boost::intrusive_ptr< BranchFeedback > theBranchFeedback;

  std::list< boost::function< bool() > > theRetirementConstraints;

  std::list< boost::function< bool() > > thePreValidations;
  std::list< boost::function< bool() > > thePostValidations;
  std::list< boost::function< void() > > theOverrideFns;

  bool theOverrideSimics;
  bool thePrevalidationsPassed;
  bool theRetirementDepends[4];
  int32_t theRetireDepCount;
  bool theIsMicroOp;

  struct Dep : public DependanceTarget {
    SemanticInstruction & theInstruction;
    Dep( SemanticInstruction & anInstruction)
      : theInstruction(anInstruction)
    {}
    void satisfy(int32_t anArg) {
      theInstruction.setMayRetire(anArg, true);
    }
    void squash(int32_t anArg) {
      theInstruction.setMayRetire(anArg, false);
    }
  } theRetirementTarget;

  boost::optional<PhysicalMemoryAddress> theAccessAddress;

  // This is a better-than-nothing way of accounting for the time taken by non-memory istructions
  // When an instruction dispatches, this counter is set to the earliest possible cycle it could retire
  // Right now this is just current cycle + functional unit latency
  // The uarch decrements this counter by 1 every cycle; the instruction can retire when it is 0
  uint32_t theCanRetireCounter;

public:
  void setCanRetireCounter(const uint32_t numCycles);
  void decrementCanRetireCounter();

  SemanticInstruction(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> bp_state, uint32_t aCPU, int64_t aSequenceNo);
  virtual ~SemanticInstruction();

  InstructionComponentBuffer & icb() const {
    return theICB;
  }

  bool advancesSimics() const;
  void setIsMicroOp(bool isUop) {
    theIsMicroOp = isUop;
  }
  bool isMicroOp() const {
    return theIsMicroOp;
  }

  bool preValidate();
  bool postValidate();
  void doDispatchEffects();
  void squash();
  void doRetirementEffects();
  void checkTraps();
  void doCommitEffects();
  void annul();
  void reinstate();
  void overrideSimics();
  bool willOverrideSimics() const {
    return theOverrideSimics;
  }

  bool mayRetire() const ;

  int32_t retryTranslation();

  nuArch::InstructionDependance makeInstructionDependance( InternalDependance const & aDependance);
public:
  template <class T>
  void setOperand( eOperandCode anOperand, T aT ) {
    theOperands.set<T>( anOperand, aT );
  }

  void setOperand( eOperandCode anOperand, Operand const & aT ) {
    theOperands.set( anOperand, aT );
  }

  template <class T>
  T  & operand( eOperandCode anOperand );

  Operand & operand( eOperandCode anOperand )  {
    DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
    return theOperands.operand( anOperand );
  }

  bool hasOperand( eOperandCode anOperand )  {
    return theOperands.hasOperand( anOperand );
  }

  void addDispatchEffect( Effect * );
  void addDispatchAction( simple_action const & );
  void addRetirementEffect( Effect * );
  void addCheckTrapEffect( Effect * );
  void addCommitEffect( Effect * );
  void addSquashEffect( Effect * );
  void addAnnulmentEffect( Effect * );
  void addReinstatementEffect( Effect * );

  void addRetirementConstraint( boost::function< bool()> );

  void addOverride( boost::function< void()> anOverrideFn);
  void addPrevalidation( boost::function< bool() > aValidation);
  void addPostvalidation( boost::function< bool() > aValidation);

  void describe(std::ostream & anOstream) const;

  void setMayRetire(int32_t aBit, bool aFlag);

  InternalDependance retirementDependance();

  void setBranchFeedback(boost::intrusive_ptr<BranchFeedback> aFeedback) {
    theBranchFeedback = aFeedback;
  }
  boost::intrusive_ptr<BranchFeedback> branchFeedback() const {
    return theBranchFeedback;
  }
  void setAccessAddress(PhysicalMemoryAddress anAddress) {
    theAccessAddress = anAddress;
  }
  PhysicalMemoryAddress getAccessAddress() const {
    return theAccessAddress ? *theAccessAddress : PhysicalMemoryAddress(0) ;
  }

};

template<> inline nuArch::mapped_reg  & SemanticInstruction::operand< nuArch::mapped_reg >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand<mapped_reg>( anOperand );
}
template<> inline nuArch::unmapped_reg  & SemanticInstruction::operand< nuArch::unmapped_reg >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand<unmapped_reg>( anOperand );
}

template<> inline std::bitset<8>  & SemanticInstruction::operand< std::bitset<8> >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand< std::bitset<8> >( anOperand );
}

template <> inline uint64_t & SemanticInstruction::operand< uint64_t >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand< uint64_t >( anOperand );
}

} //nv9Decoder

#endif //FLEXUS_v9DECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
