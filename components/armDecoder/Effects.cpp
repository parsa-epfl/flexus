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


#include <iostream>
#include <iomanip>
#include <list>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/performance/profile.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>

#include "Effects.hpp"
#include "OperandCode.hpp"
#include "SemanticActions.hpp"
#include "SemanticInstruction.hpp"
#include "Interactions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using nuArchARM::uArchARM;
using nuArchARM::SemanticAction;
using nuArchARM::Interaction;
using nuArchARM::Instruction;

using nuArchARM::ccBits;

using nuArchARM::eSize;

using nuArchARM::kLoad;
using nuArchARM::kLDP;
using nuArchARM::kStore;
using nuArchARM::kCAS;
using nuArchARM::kCASP;
using nuArchARM::kRMW;

void EffectChain::invoke(SemanticInstruction & anInstruction) {
  if (theFirst) {
      DBG_(Tmp, (<<"\e[1;32m"<<"EFFECTCHAIN: invoking "<< anInstruction << "\e[0m"));
    theFirst->invoke(anInstruction);
  }
}

void EffectChain::describe(std::ostream & anOstream) const {
  if (theFirst) {
    theFirst->describe(anOstream);
  }
}

void EffectChain::append(Effect * anEffect) {
  if (! theFirst) {
    theFirst = anEffect;
  } else {
    theLast->theNext = anEffect;
  }
  theLast = anEffect;
}

EffectChain::EffectChain()
  : theFirst(0)
  , theLast(0)
{}

struct MapSourceEffect : public Effect {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;

  MapSourceEffect( eOperandCode anInputCode, eOperandCode anOutputCode )
    : theInputCode( anInputCode )
    , theOutputCode( anOutputCode )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    reg name( anInstruction.operand< reg > (theInputCode) );
    mapped_reg mapped_name = anInstruction.core()->map( name );
    anInstruction.setOperand(theOutputCode, mapped_name);
    DBG_( Tmp, ( <<"\e[1;32m"<< anInstruction.identify() << " MapEffect " <<  theInputCode << "(" << name <<
                 ")" << " mapped to " << mapped_name << " stored in " << theOutputCode<<"\e[0m") );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "MapSource " << theInputCode << " store mapping in " << theOutputCode;
    Effect::describe(anOstream);
  }
};

Effect * mapSource( SemanticInstruction * inst, eOperandCode anInputCode, eOperandCode anOutputCode) {
  return new(inst->icb()) MapSourceEffect( anInputCode, anOutputCode );
}


//struct DisconnectRegisterEffect : public Effect {
//  eOperandCode theMappingCode;

//  DisconnectRegisterEffect( eOperandCode aMappingCode )
//   : theMappingCode( aMappingCode )
//  { }

//  void invoke(SemanticInstruction & anInstruction) {
//    FLEXUS_PROFILE();
//    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );
//    DBG_( Tmp, ( << anInstruction.identify() << " Disconnect Register " << theMappingCode << "(" << mapping << ")" ) );
//    anInstruction.core()->disconnectRegister( mapping, &anInstruction );
//    //We no longer disconnect from bypass - we just wait for the register to
//    //get unmapped, even though this extends the life of the instruction object
//    //anInstruction.core()->disconnectBypass( mapping, &anInstruction );
//    Effect::invoke(anInstruction);
//  }

//  void describe( std::ostream & anOstream) const {
//    anOstream << "DisconnectRegister " << theMappingCode ;
//    Effect::describe(anOstream);
//  }
//};

//Effect * disconnectRegister( SemanticInstruction * inst, eOperandCode aMapping) {
//  return new(inst->icb()) DisconnectRegisterEffect( aMapping );
//}


struct FreeMappingEffect : public Effect {
  eOperandCode theMappingCode;

  FreeMappingEffect( eOperandCode aMappingCode )
    : theMappingCode( aMappingCode )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );
    DBG_( Tmp, ( << anInstruction.identify() << " MapEffect free mapping for " << theMappingCode << "(" << mapping << ")" ) );
    anInstruction.core()->free( mapping );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "FreeMapping " << theMappingCode ;
    Effect::describe(anOstream);
  }
};

Effect * freeMapping( SemanticInstruction * inst, eOperandCode aMapping) {
  return new(inst->icb()) FreeMappingEffect( aMapping );
}

struct RestoreMappingEffect : public Effect {
  eOperandCode theNameCode;
  eOperandCode theMappingCode;

  RestoreMappingEffect( eOperandCode aNameCode, eOperandCode aMappingCode)
    : theNameCode( aNameCode )
    , theMappingCode( aMappingCode )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    reg name ( anInstruction.operand< reg > (theNameCode) );
    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );

    DBG_( Tmp, ( << anInstruction.identify() << " MapEffect restore mapping for " << theNameCode << "(" << name << ") to " << theMappingCode << "( " << mapping << ")" ) );
    anInstruction.core()->restore( name, mapping );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "RestoreMapping " << theNameCode << " to " << theMappingCode;
    Effect::describe(anOstream);
  }
};

Effect * restoreMapping( SemanticInstruction * inst, eOperandCode aName, eOperandCode aMapping) {
  return new(inst->icb()) RestoreMappingEffect( aName, aMapping );
}

struct MapDestinationEffect : public Effect {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;
  eOperandCode thePreviousMappingCode;
  bool theSquashEffects;

  MapDestinationEffect( eOperandCode anInputCode, eOperandCode anOutputCode, eOperandCode aPreviousMappingCode, bool aSquashEffects)
    : theInputCode( anInputCode )
    , theOutputCode( anOutputCode )
    , thePreviousMappingCode( aPreviousMappingCode )
    , theSquashEffects(aSquashEffects)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_(Tmp, (<< "\e[1;32m"<<"EFFECT: invoking " << this << anInstruction << "\e[0m"));
    reg name( anInstruction.operand< reg > (theInputCode) );
    mapped_reg mapped_name;
    mapped_reg previous_mapping;
    boost::tie(mapped_name, previous_mapping ) = anInstruction.core()->create( name );
    anInstruction.setOperand(theOutputCode, mapped_name);
    anInstruction.setOperand(thePreviousMappingCode, previous_mapping );
    if (mapped_name.theType == ccBits) {
      anInstruction.core()->copyRegValue(previous_mapping, mapped_name);
    }

    //Add effect to free previous mapping upon retirement
    anInstruction.addRetirementEffect( new(anInstruction.icb()) FreeMappingEffect( thePreviousMappingCode ) );

    //Add effect to deallocate new register and restore previous mapping upon squash
    if (theSquashEffects) {
      anInstruction.addSquashEffect( new(anInstruction.icb())  RestoreMappingEffect( theInputCode, thePreviousMappingCode ) );
      anInstruction.addSquashEffect( new(anInstruction.icb())  FreeMappingEffect( theOutputCode ) );
    }

    DBG_( Tmp, ( <<"\e[1;32m EFFECT: "<< anInstruction.identify() << " MapEffect new mapping for " << theInputCode <<
                 "(" << name << ") to " << mapped_name << " stored in " << theOutputCode <<
                 " previous mapping " << previous_mapping << " stored in " << thePreviousMappingCode << "\e[0m" ) );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "MapDestination " << theInputCode << ", store mapping in " << theOutputCode;
    Effect::describe(anOstream);
  }
};

Effect * mapDestination( SemanticInstruction * inst, eRegisterType aMapTable ) {
  if ( aMapTable == ccBits ) {
    return new(inst->icb()) MapDestinationEffect( kCCd, kCCpd, kPCCpd, true );
  } else {
    return new(inst->icb()) MapDestinationEffect( kRD, kPD, kPPD, true );
  }
}

Effect * mapRD1Destination( SemanticInstruction * inst ) {
  return new(inst->icb()) MapDestinationEffect( kRD1, kPD1, kPPD1, true );
}

Effect * mapXTRA(SemanticInstruction * inst ) {
  return new(inst->icb()) MapDestinationEffect( eOperandCode(kXTRAr), eOperandCode(kXTRApd), eOperandCode(kXTRAppd), true);
}

Effect * mapFDestination( SemanticInstruction * inst, int32_t anIndex ) {
  return new(inst->icb()) MapDestinationEffect( eOperandCode(kFD0 + anIndex), eOperandCode(kPFD0 + anIndex), eOperandCode(kPPFD0 + anIndex), true);
}

Effect * mapDestination_NoSquashEffects( SemanticInstruction * inst, eRegisterType aMapTable ) {
  if ( aMapTable == ccBits ) {
    return new(inst->icb()) MapDestinationEffect( kCCd, kCCpd, kPCCpd, false);
  } else {
    return new(inst->icb()) MapDestinationEffect( kRD, kPD, kPPD, false );
  }
}

Effect * unmapDestination( SemanticInstruction * inst, eRegisterType aMapTable ) {
  if ( aMapTable == ccBits ) {
    return new(inst->icb()) FreeMappingEffect( kCCpd );
  } else {
    return new(inst->icb()) FreeMappingEffect( kPD );
  }
}

Effect * unmapFDestination( SemanticInstruction * inst, int32_t anIndex ) {
  return new(inst->icb()) FreeMappingEffect( eOperandCode(kPFD0 + anIndex) );
}

Effect * restorePreviousDestination(SemanticInstruction * inst,  eRegisterType aMapTable ) {
  if ( aMapTable == ccBits ) {
    return new(inst->icb()) RestoreMappingEffect( kCCd, kPCCpd );
  } else {
    return new(inst->icb()) RestoreMappingEffect( kRD, kPPD );
  }
}

struct SatisfyDependanceEffect : public Effect {
  InternalDependance theDependance;

  SatisfyDependanceEffect ( InternalDependance const & aDependance)
    : theDependance( aDependance )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();

    //DBG_( Tmp, ( << anInstruction.identify()  <<" SatisfyDependanceEffect ") );

    theDependance.satisfy();
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Satisfy Dependance Effect";
    Effect::describe(anOstream);
  }
};

struct SquashDependanceEffect : public Effect {
  InternalDependance theDependance;

  SquashDependanceEffect ( InternalDependance const & aDependance)
    : theDependance( aDependance )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " SquashDependanceEffect ") );

    theDependance.squash();
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Squash Dependance Effect";
    Effect::describe(anOstream);
  }
};

Effect  * satisfy( SemanticInstruction * inst, InternalDependance const & aDependance) {
  return new(inst->icb()) SatisfyDependanceEffect( aDependance );
}

Effect  * squash( SemanticInstruction * inst, InternalDependance const & aDependance) {
  return new(inst->icb()) SquashDependanceEffect( aDependance );
}

struct AnnulInstruction : public Interaction {
  void operator() (boost::intrusive_ptr<nuArchARM::Instruction> anInstruction, nuArchARM::uArchARM & aCore) {
    DBG_( Tmp, ( << *anInstruction << " Annulled") );
    anInstruction->annul();
  }
  bool annuls() {
    return true;
  }
  void describe(std::ostream & anOstream) const {
    anOstream << "Annul Instruction";
  }
};

nuArchARM::Interaction * annulInstructionInteraction() {
  return new AnnulInstruction;
}

struct ReinstateInstruction : public Interaction {
  void operator() (boost::intrusive_ptr<nuArchARM::Instruction> anInstruction, nuArchARM::uArchARM & aCore) {
    DBG_( Tmp, ( << *anInstruction << " Reinstated") );
    anInstruction->reinstate();
  }
  void describe(std::ostream & anOstream) const {
    anOstream << "Reinstate Instruction";
  }
};

nuArchARM::Interaction * reinstateInstructionInteraction() {
  return new ReinstateInstruction;
}

struct AnnulNextEffect : public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " Annul Next Instruction") );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new AnnulInstruction() );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Annul Next Instruction";
    Effect::describe(anOstream);
  }
};

Effect * annulNext(SemanticInstruction * inst) {
  return new(inst->icb()) AnnulNextEffect( );
}

BranchInteraction::BranchInteraction( VirtualMemoryAddress aTarget)
  : theTarget(aTarget)
{}

void BranchInteraction::operator() (boost::intrusive_ptr<nuArchARM::Instruction> anInstruction, nuArchARM::uArchARM & aCore) {
  DBG_( Tmp, ( << *anInstruction << " " << *this) );
  if (theTarget == 0) {
    theTarget = anInstruction->pc() + 4;
  }
}

void BranchInteraction::describe(std::ostream & anOstream) const {
  anOstream << "Branch to " << theTarget;
}

nuArchARM::Interaction * branchInteraction( VirtualMemoryAddress aTarget) {
  return new BranchInteraction( aTarget) ;
}

struct BranchFeedbackEffect: public Effect {
  BranchFeedbackEffect( )
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction << " BranchFeedbackEffect " ) );//NOOSHIN

    if ( anInstruction.branchFeedback() ) {
      DBG_( Tmp, ( << anInstruction << " Update Branch predictor: " << anInstruction.branchFeedback()->theActualType << " " << anInstruction.branchFeedback()->theActualDirection << " to " << anInstruction.branchFeedback()->theActualTarget ) );
      anInstruction.core()->branchFeedback(anInstruction.branchFeedback());
    }
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Update Branch Predictor";
    Effect::describe(anOstream);
  }
};

struct BranchFeedbackWithOperandEffect: public Effect {
  eDirection theDirection;
  eBranchType theType;
  eOperandCode theOperandCode;
  BranchFeedbackWithOperandEffect( eBranchType aType, eDirection aDirection, eOperandCode anOperandCode)
    : theDirection(aDirection)
    , theType(aType)
    , theOperandCode(anOperandCode)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = theType;
    feedback->theActualDirection = theDirection;
    VirtualMemoryAddress target(anInstruction.operand< uint64_t > (theOperandCode));
    DBG_( Tmp, ( << anInstruction << " Update Branch predictor: " << theType << " " << theDirection << " to " << target) );
    feedback->theActualTarget = target;
    feedback->theBPState = anInstruction.bpState();
    anInstruction.core()->branchFeedback(feedback);
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Update Branch Predictor";
    Effect::describe(anOstream);
  }
};

Effect * updateConditional(SemanticInstruction * inst) {
  return new(inst->icb()) BranchFeedbackEffect();
}

Effect * updateUnconditional(SemanticInstruction * inst, VirtualMemoryAddress aTarget) {
  boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
  feedback->thePC = inst->pc();
  feedback->theActualType = kUnconditional;
  feedback->theActualDirection = kTaken;
  feedback->theActualTarget = aTarget ;
  feedback->theBPState = inst->bpState();
  inst->setBranchFeedback(feedback);
  return new(inst->icb()) BranchFeedbackEffect();
}

Effect * updateNonBranch(SemanticInstruction * inst) {
  boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
  feedback->thePC = inst->pc();
  feedback->theActualType = kNonBranch;
  feedback->theActualDirection = kNotTaken;
  feedback->theActualTarget = VirtualMemoryAddress(0);
  feedback->theBPState = inst->bpState();
  inst->setBranchFeedback(feedback);
  return new(inst->icb()) BranchFeedbackEffect();
}

Effect * updateUnconditional(SemanticInstruction * inst, eOperandCode anOperandCode) {
  return new(inst->icb()) BranchFeedbackWithOperandEffect( kUnconditional, kTaken, anOperandCode);
}

Effect * updateCall(SemanticInstruction * inst, VirtualMemoryAddress aTarget) {
  boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
  feedback->thePC = inst->pc();
  feedback->theActualType = kUnconditional;
  feedback->theActualDirection = kTaken;
  feedback->theActualTarget = aTarget;
  feedback->theBPState = inst->bpState();
  inst->setBranchFeedback(feedback);
  return new(inst->icb()) BranchFeedbackEffect( );
}

struct BranchEffect: public Effect {
  VirtualMemoryAddress theTarget;
  bool theHasTarget;
  BranchEffect( VirtualMemoryAddress aTarget)
    : theTarget(aTarget)
    , theHasTarget(true)
  {}
  BranchEffect( )
    : theHasTarget(false)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction << " BranchEffect " ) );//NOOSHIN

//    if (!theHasTarget)
//        theTarget = anInstruction.operand(kAddress);

    if ( anInstruction.redirectPC(theTarget, anInstruction.pc() + 4 ) ) {
      DBG_( Tmp, ( << anInstruction << " BRANCH:  Must redirect.") );
      if ( anInstruction.core()->squashAfter(boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction)) ) {
        anInstruction.core()->redirectFetch(theTarget);
      }
    }
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Branch to " << theTarget;
    Effect::describe(anOstream);
  }
};

struct BranchAfterNext : public Effect {
  VirtualMemoryAddress theTarget;
  BranchAfterNext( VirtualMemoryAddress aTarget)
    : theTarget(aTarget)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " Branch after next instruction to " << theTarget ) );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Branch to " << theTarget << " after next instruction";
    Effect::describe(anOstream);
  }
};

struct BranchAfterNextWithOperand : public Effect {
  eOperandCode theOperandCode;
  BranchAfterNextWithOperand( eOperandCode anOperandCode)
    : theOperandCode(anOperandCode)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    VirtualMemoryAddress target(anInstruction.operand< bits > (theOperandCode).to_ulong());
    DBG_( Tmp, ( << anInstruction.identify() << " Branch after next instruction to " << theOperandCode << "(" << target << ")" ) );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(target) );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Branch to " << theOperandCode << " after next instruction";
    Effect::describe(anOstream);
  }
};

Effect * branch(SemanticInstruction * inst,  VirtualMemoryAddress aTarget) {
  return new(inst->icb()) BranchEffect( aTarget );
}
Effect * branchAfterNext(SemanticInstruction * inst, VirtualMemoryAddress aTarget) {
  return new(inst->icb()) BranchAfterNext(aTarget);
}

Effect * branchAfterNext(SemanticInstruction * inst, eOperandCode anOperandCode) {
  return new(inst->icb()) BranchAfterNextWithOperand(anOperandCode);
}

struct BranchConditionallyEffect: public Effect {
  VirtualMemoryAddress theTarget;
  bool theAnnul;
  Condition & theCondition;
  bool isFloating;
  eOperandCode theCCRCode;
  BranchConditionallyEffect( VirtualMemoryAddress aTarget, bool anAnnul, Condition & aCondition, bool floating, eOperandCode aCCRCode)
    : theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , isFloating(floating)
    , theCCRCode(aCCRCode)
  {}
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Tmp, ( << anInstruction << " BranchConditionallyEffect " ) );//NOOSHIN

    FLEXUS_PROFILE();
    mapped_reg name = anInstruction.operand< mapped_reg > (theCCRCode);
    bits cc = boost::get< bits >( anInstruction.core()->readRegister( name ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

    bool result/* = theCondition(operands)*/;

    if ( result ) {
      //Taken
      DBG_( Tmp, ( << anInstruction << " conditional branch CC: " << cc << " TAKEN" ) );
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );

      feedback->theActualDirection = kTaken;

    } else {
      //Not Taken
      DBG_( Tmp, ( << anInstruction << " conditional branch CC: " << cc << " NOT TAKEN" ) );
      if (theAnnul) {
        DBG_( Tmp, ( << anInstruction.identify() << " Annul Next Instruction") );
        anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new AnnulInstruction() );
//        anInstruction.redirectNPC( anInstruction.pc() + 8 );
      }
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction( anInstruction.pc() + 8 ) );
      feedback->theActualDirection = kNotTaken;
    }
    anInstruction.core()->branchFeedback(feedback);
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Condition Branch to " << theTarget ;
    Effect::describe(anOstream);
  }
};

Effect * branchConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, Condition & aCondition, bool isFloating) {
  return new(inst->icb()) BranchConditionallyEffect( aTarget, anAnnul, aCondition, isFloating, kCCps );
}

struct BranchRegConditionallyEffect: public Effect {
  VirtualMemoryAddress theTarget;
  bool theAnnul;
  Condition & theCondition;
  eOperandCode theValueCode;
  BranchRegConditionallyEffect( VirtualMemoryAddress aTarget, bool anAnnul, Condition & aCondition, eOperandCode aValueCode)
    : theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , theValueCode(aValueCode)
  {}
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Tmp, ( << anInstruction << " BranchRegConditionallyEffect " ) );//NOOSHIN

    FLEXUS_PROFILE();
    bits val = anInstruction.operand< bits > (theValueCode);

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

//    RCondition cond( rcondition(theCondition) );

//    if ( cond( val ) ) {
//      //Taken
//      DBG_( Tmp, ( << anInstruction << " register conditional branch TAKEN" ) );
//      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );
//      feedback->theActualDirection = kTaken;
//    } else {
//      //Not Taken
//      DBG_( Tmp, ( << anInstruction << " register conditional branch  NOT TAKEN" ) );
//      if (theAnnul) {
//        DBG_( Tmp, ( << anInstruction.identify() << " Annul Next Instruction") );
//        anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new AnnulInstruction() );
////        anInstruction.redirectNPC( anInstruction.pc() + 8 );
//      }
//      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction( anInstruction.pc() + 8 ) );
//      feedback->theActualDirection = kNotTaken;
//    }
    anInstruction.core()->branchFeedback(feedback);
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Register Condition Branch to " << theTarget ;
    Effect::describe(anOstream);
  }
};

Effect * branchRegConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, Condition & aCondition)  {
  return new(inst->icb()) BranchRegConditionallyEffect( aTarget, anAnnul, aCondition, kOperand1 );
}

struct AllocateLSQEffect : public Effect {
  eOperation theOperation;
  eSize theSize;
  bool hasDependance;
  InternalDependance theDependance;
  bool theBypassSB;
  eAccType theAccType;

  AllocateLSQEffect( eOperation anOperation, eSize aSize, bool aBypassSB, InternalDependance const & aDependance , eAccType type)
    : theOperation(anOperation)
    , theSize(aSize)
    , hasDependance(true)
    , theDependance(aDependance)
    , theBypassSB(aBypassSB)
    , theAccType(type)
  {}
  AllocateLSQEffect( eOperation anOperation, eSize aSize, bool aBypassSB, eAccType type)
    : theOperation(anOperation)
    , theSize(aSize)
    , hasDependance(false)
    , theBypassSB(aBypassSB)
    , theAccType(type)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << "\e[1;32m"<< anInstruction.identify() << " Allocate LSQ Entry"<< "\e[0m") );
    if (hasDependance) {
      anInstruction.core()->insertLSQ( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , theOperation, theSize, theBypassSB, anInstruction.makeInstructionDependance(theDependance), theAccType );
    } else {
      anInstruction.core()->insertLSQ( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , theOperation, theSize, theBypassSB, theAccType);
    }
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Allocate LSQ Entry";
    Effect::describe(anOstream);
  }
};

Effect * allocateStore(SemanticInstruction * inst, eSize aSize, bool aBypassSB, eAccType type)  {
  return new(inst->icb()) AllocateLSQEffect( kStore, aSize, aBypassSB, type);
}

Effect * allocateMEMBAR(SemanticInstruction * inst , eAccType type)  {
  return new(inst->icb()) AllocateLSQEffect( kMEMBARMarker, kWord, false, type );
}

Effect * allocateLoad(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance, eAccType type) {
  return new(inst->icb()) AllocateLSQEffect( kLoad, aSize, false, aDependance, type );
}

Effect * allocateCAS(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance, eAccType type) {
  return new(inst->icb()) AllocateLSQEffect( kCAS, aSize, false, aDependance, type );
}

Effect * allocateRMW(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance, eAccType type) {
  return new(inst->icb()) AllocateLSQEffect( kRMW, aSize, false, aDependance, type);
}

struct EraseLSQEffect : public Effect {
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Tmp, ( << anInstruction.identify() << " EraseLSQEffect ") );

    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " Free LSQ Entry") );
    anInstruction.core()->eraseLSQ( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction));
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Free LSQ Load Entry";
    Effect::describe(anOstream);
  }
};

Effect * eraseLSQ(SemanticInstruction * inst) {
  return new(inst->icb()) EraseLSQEffect( );
}

struct RetireMemEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Tmp, ( << anInstruction.identify() << " RetireMemEffect ") );

    FLEXUS_PROFILE();
    anInstruction.core()->retireMem( boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction) );
    DBG_( Tmp, ( << anInstruction << " Retiring memory instruction" ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Retire Memory";
    Effect::describe(anOstream);
  }
};

Effect * retireMem(SemanticInstruction * inst) {
  return new(inst->icb()) RetireMemEffect( );
}

struct CommitStoreEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Tmp, ( << anInstruction.identify() << " CommitStoreEffect ") );

    FLEXUS_PROFILE();
    anInstruction.core()->commitStore( boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction) );
    DBG_( Tmp, ( << anInstruction << " Commit store instruction" ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Commit Store";
    Effect::describe(anOstream);
  }
};

Effect * commitStore(SemanticInstruction * inst) {
  return new(inst->icb()) CommitStoreEffect( );
}

struct AccessMemEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " AccessMemEffect ") );

    anInstruction.core()->accessMem( anInstruction.getAccessAddress(), boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction)  );
    DBG_( Tmp, ( << anInstruction << " Access memory: " << anInstruction.getAccessAddress()) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Commit Store";
    Effect::describe(anOstream);
  }
};

Effect * accessMem(SemanticInstruction * inst) {
  return new(inst->icb()) AccessMemEffect( );
}

struct ForceResyncEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction.identify() << " ForceResyncEffect ") );

    anInstruction.forceResync();
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Force Resync ";
    Effect::describe(anOstream);
  }
};

Effect * forceResync(SemanticInstruction * inst) {
  return new(inst->icb()) ForceResyncEffect();
}

struct DMMUTranslationCheckEffect: public Effect {
  DMMUTranslationCheckEffect( )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction << " DMMUTranslationCheckEffect " ) );//NOOSHIN

    anInstruction.core()->checkTranslation( boost::intrusive_ptr<Instruction>(& anInstruction));

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " DMMU TranslationCheckEffect";
    Effect::describe(anOstream);
  }
};

Effect * dmmuTranslationCheck(SemanticInstruction * inst) {
  return new(inst->icb()) DMMUTranslationCheckEffect();
}

struct IMMUExceptionEffect: public Effect {
  IMMUExceptionEffect( )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Tmp, ( << anInstruction << " retryTranslation " ) );//NOOSHIN

    int32_t exception = anInstruction.retryTranslation();
    if (exception != 0) {
      anInstruction.setWillRaise( exception );
      DBG_( Tmp, ( << anInstruction << " IMMU Exception: " << anInstruction.willRaise()  ) );//NOOSHIN
//      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
    }

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " IMMU Exception ";
    Effect::describe(anOstream);
  }
};

Effect * immuException(SemanticInstruction * inst) {
  return new(inst->icb()) IMMUExceptionEffect();
}


} //narmDecoder
