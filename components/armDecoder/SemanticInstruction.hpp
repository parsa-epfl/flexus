// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_armDECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
#define FLEXUS_armDECODER_SEMANTICINSTRUCTION_HPP_INCLUDED

#include <memory>
#include <boost/dynamic_bitset.hpp>
//#include "components/armDecoder/Conditions.hpp"

#include "Effects.hpp"

#include <components/uArchARM/uArchInterfaces.hpp>
#include "armInstruction.hpp"
#include "OperandMap.hpp"
#include "InstructionComponentBuffer.hpp"

namespace narmDecoder {

struct simple_action;
struct EffectChain;

struct SemanticInstruction : public armInstruction {
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

  std::list< std::function< bool() > > theRetirementConstraints;

  std::list< std::function< bool() > > thePreValidations;
  std::list< std::function< bool() > > thePostValidations;
  std::list< std::function< void() > > theOverrideFns;

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

  SemanticInstruction(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> bp_state, uint32_t aCPU, int64_t aSequenceNo);
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
  void pageFault();
  bool isPageFault();
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

  eExceptionType retryTranslation();
  PhysicalMemoryAddress translate();

  nuArchARM::InstructionDependance makeInstructionDependance( InternalDependance const & aDependance);
public:
  template <class T>
  void setOperand( eOperandCode anOperand, T aT ) {
    theOperands.set<T>( anOperand, aT );
  }

  void setOperand( eOperandCode anOperand, Operand const & aT ) {
    SEMANTICS_DBG(identify() << ": [ " << anOperand << "->" << aT << " ]");
    theOperands.set( anOperand, aT );
  }

  template <class T>
  T  & operand( eOperandCode anOperand );

  Operand & operand( eOperandCode anOperand )  {
    DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
    SEMANTICS_DBG(identify() << "requesting " << anOperand);
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

  void addRetirementConstraint( std::function< bool()> );

  void addOverride( std::function< void()> anOverrideFn);
  void addPrevalidation( std::function< bool() > aValidation);
  void addPostvalidation( std::function< bool() > aValidation);

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

template<> inline nuArchARM::mapped_reg  & SemanticInstruction::operand< nuArchARM::mapped_reg >( eOperandCode anOperand ) {
  DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand<mapped_reg>( anOperand );
}
template<> inline nuArchARM::reg  & SemanticInstruction::operand< nuArchARM::reg >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand<reg>( anOperand );
}

template<> inline bits  & SemanticInstruction::operand< bits >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand< bits >( anOperand );
}

template <> inline uint64_t & SemanticInstruction::operand< uint64_t >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand< uint64_t >( anOperand );
}
template <> inline int64_t & SemanticInstruction::operand< int64_t >( eOperandCode anOperand ) {
  //DBG_Assert( theOperands.hasOperand(anOperand), ( << "Request for unavailable operand " << anOperand << "(" << static_cast<int>(anOperand) << ") by\n" << std::internal << *this << std::left ) );
  return theOperands.operand< int64_t >( anOperand );
}

} //narmDecoder

#endif //FLEXUS_armDECODER_SEMANTICINSTRUCTION_HPP_INCLUDED
