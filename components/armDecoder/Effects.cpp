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

#include <components/uArchARM/systemRegister.hpp>

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
    mapped_reg  mapped_name = anInstruction.core()->map( name );
    anInstruction.setOperand(theOutputCode, mapped_name);

    DECODER_DBG(anInstruction.identify() <<  theInputCode << " (" << name << ")" <<
                                              " mapped to " << theOutputCode << "(" << mapped_name << ")");

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
//    DBG_( VVerb, ( << anInstruction.identify() << " Disconnect Register " << theMappingCode << "(" << mapping << ")" ) );
//    anInstruction.core()->disconnectRegister( mapping, &anInstruction );
//    //We no longer disconnect from bypass - we just wait for the register to
//    //get unmapped, even though this extends the life of the instruction object
//    anInstruction.core()->disconnectBypass( mapping, &anInstruction );
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
    DBG_( Dev, ( << anInstruction.identify() << " MapEffect free mapping for " << theMappingCode << "(" << mapping << ")" ) );
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

    DBG_( VVerb, ( << anInstruction.identify() << " MapEffect restore mapping for " << theNameCode << "(" << name << ") to " << theMappingCode << "( " << mapping << ")" ) );
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

    DECODER_DBG(anInstruction.identify() << " Mapping " << theInputCode <<"(" << name << ")" <<
                                              " to " << theOutputCode <<"(" << mapped_name << ")" <<
                                              " previous mapping " << thePreviousMappingCode <<"(" <<  previous_mapping << ")" );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "MapDestination " << theInputCode << ", store mapping in " << theOutputCode;
    Effect::describe(anOstream);
  }
};

Effect * mapCCDestination( SemanticInstruction * inst) {
    return new(inst->icb()) MapDestinationEffect( kCCd, kCCpd, kPCCpd, true );
}
Effect * mapDestination( SemanticInstruction * inst) {
    return new(inst->icb()) MapDestinationEffect( kRD, kPD, kPPD, true );
}
Effect * mapRD1Destination( SemanticInstruction * inst ) {
  return new(inst->icb()) MapDestinationEffect( kRD1, kPD1, kPPD1, true );
}

Effect * mapRD2Destination( SemanticInstruction * inst ) {
  return new(inst->icb()) MapDestinationEffect( kRD2, kPD2, kPPD2, true );
}

Effect * mapDestination_NoSquashEffects( SemanticInstruction * inst) {
    return new(inst->icb()) MapDestinationEffect( kRD, kPD, kPPD, false );
}

Effect * mapRD1Destination_NoSquashEffects( SemanticInstruction * inst ) {
    return new(inst->icb()) MapDestinationEffect( kRD1, kPD1, kPPD1, false );
}

Effect * mapRD2Destination_NoSquashEffects( SemanticInstruction * inst ) {
    return new(inst->icb()) MapDestinationEffect( kRD2, kPD2, kPPD2, false );
}

Effect * unmapDestination( SemanticInstruction * inst ) {
    return new(inst->icb()) FreeMappingEffect( kPD );
}

Effect * unmapFDestination( SemanticInstruction * inst, int32_t anIndex ) {
  return new(inst->icb()) FreeMappingEffect( eOperandCode(kPFD0 + anIndex) );
}

Effect * restorePreviousDestination(SemanticInstruction * inst ) {
    return new(inst->icb()) RestoreMappingEffect( kRD, kPPD );
}

struct SatisfyDependanceEffect : public Effect {
  InternalDependance theDependance;

  SatisfyDependanceEffect ( InternalDependance const & aDependance)
    : theDependance( aDependance )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();

    SEMANTICS_TRACE;
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
    DBG_( VVerb, ( << anInstruction.identify() << " SquashDependanceEffect ") );

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
    DBG_( VVerb, ( << *anInstruction << " Annulled") );
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
    DBG_( VVerb, ( << *anInstruction << " Reinstated") );
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
    DBG_( VVerb, ( << anInstruction.identify() << " Annul Next Instruction") );
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
  DBG_( VVerb, ( << *anInstruction << " " << *this) );
  if (theTarget == 0) {
    theTarget = anInstruction->pc() + 4;
  }
  if (anInstruction->redirectPC(theTarget) ) {
    DBG_( Verb, ( << *anInstruction << " Branch Redirection.") );
    if ( aCore.squashAfter(anInstruction) ) {
      aCore.redirectFetch(theTarget);
    }
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
    DBG_( VVerb, ( << anInstruction << " BranchFeedbackEffect " ) );//NOOSHIN

    if ( anInstruction.branchFeedback() ) {
      DBG_( VVerb, ( << anInstruction << " Update Branch predictor: " << anInstruction.branchFeedback()->theActualType << " " << anInstruction.branchFeedback()->theActualDirection << " to " << anInstruction.branchFeedback()->theActualTarget ) );
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
    DBG_( Dev, ( << anInstruction << " Update Branch predictor: " << theType << " " << theDirection << " to " << target) );
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
    DBG_( VVerb, ( << anInstruction << " BranchEffect " ) );

    if (!theHasTarget){
        Operand address = anInstruction.operand(kAddress);
        theTarget = VirtualMemoryAddress(boost::get<uint64_t>(address));
    }

    anInstruction.redirectPC(theTarget);
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , branchInteraction(theTarget) );
    DBG_( Dev, ( << "BRANCH:  Must redirect to " << theTarget) );
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
    DBG_( VVerb, ( << anInstruction.identify() << " Branch after next instruction to " << theTarget ) );
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
    VirtualMemoryAddress target(anInstruction.operand< uint64_t > (theOperandCode));
    DBG_( VVerb, ( << anInstruction.identify() << " Branch after next instruction to " << theOperandCode << "(" << target << ")" ) );
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
      DBG_( VVerb, ( << anInstruction << " BranchConditionallyEffect " ) );//NOOSHIN

    FLEXUS_PROFILE();
    mapped_reg name = anInstruction.operand< mapped_reg > (theCCRCode);
    uint64_t cc = boost::get< uint64_t >( anInstruction.core()->readRegister( name ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

    std::vector<Operand> operands = {cc};

    bool result = theCondition(operands);

    if ( result ) {
      //Taken
      DBG_( VVerb, ( << anInstruction << " conditional branch CC: " << cc << " TAKEN" ) );
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );

      feedback->theActualDirection = kTaken;

    } else {
      //Not Taken
      DBG_( VVerb, ( << anInstruction << " conditional branch CC: " << cc << " NOT TAKEN" ) );
      if (theAnnul) {
        DBG_( VVerb, ( << anInstruction.identify() << " Annul Next Instruction") );
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
      DBG_( VVerb, ( << anInstruction << " BranchRegConditionallyEffect " ) );//NOOSHIN

    FLEXUS_PROFILE();
    // uint64_t val = anInstruction.operand< uint64_t > (theValueCode);

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

//    RCondition cond( rcondition(theCondition) );

//    if ( cond( val ) ) {
//      //Taken
//      DBG_( VVerb, ( << anInstruction << " register conditional branch TAKEN" ) );
//      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );
//      feedback->theActualDirection = kTaken;
//    } else {
//      //Not Taken
//      DBG_( VVerb, ( << anInstruction << " register conditional branch  NOT TAKEN" ) );
//      if (theAnnul) {
//        DBG_( VVerb, ( << anInstruction.identify() << " Annul Next Instruction") );
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
    SEMANTICS_DBG(anInstruction.identify() << " Allocate LSQ Entry");
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
      DBG_( VVerb, ( << anInstruction.identify() << " EraseLSQEffect ") );

    FLEXUS_PROFILE();
    DBG_( VVerb, ( << anInstruction.identify() << " Free LSQ Entry") );
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
      DBG_( VVerb, ( << anInstruction.identify() << " RetireMemEffect ") );

    FLEXUS_PROFILE();
    anInstruction.core()->retireMem( boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction) );
    DBG_( VVerb, ( << anInstruction << " Retiring memory instruction" ) );
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


struct CheckSysRegAccess : public Effect {

    ePrivRegs thePrivReg;
    uint8_t theIsRead;

    CheckSysRegAccess ( ePrivRegs aPrivReg, uint8_t is_read)
            : thePrivReg (aPrivReg)
            , theIsRead (is_read)
          {}

    void invoke(SemanticInstruction & anInstruction) {
        if (! anInstruction.isAnnulled()) {
        }

        Effect::invoke(anInstruction);
    }

    void describe(std::ostream & anOstream) const {
      anOstream << " CheckSysRegAccess ";
      Effect::describe(anOstream);
    }

};

Effect * checkSysRegAccess(SemanticInstruction * inst, ePrivRegs aPrivReg, uint8_t is_read) {
  return new(inst->icb()) CheckSysRegAccess(aPrivReg, is_read);
}




struct CheckSystemAccess : public Effect {
    uint8_t theOp0,theOp1,theOp2,theCRn,theCRm, theRT, theRead;
    CheckSystemAccess(uint8_t anOp0, uint8_t anOp1, uint8_t anOp2, uint8_t aCRn, uint8_t aCRm, uint8_t aRT, uint8_t aRead)
      : theOp0(anOp0), theOp1(anOp1), theOp2(anOp2), theCRn(aCRn), theCRm(aCRm), theRT(aRT), theRead(aRead)
    {}

    void invoke(SemanticInstruction & anInstruction) {
      FLEXUS_PROFILE();
      if (! anInstruction.isAnnulled()) {
          // Perform the generic checks that an AArch64 MSR/MRS/SYS instruction is valid at the
          // current exception level, based on the opcode's 'op1' field value.
          // Further checks for enables/disables/traps specific to a particular system register
          // or operation will be performed in System_Put(), System_Get(), SysOp_W(), or SysOp_R().
          /* 64 bit registers have only CRm and Opc1 fields */

          SysRegInfo& ri = anInstruction.core()->getSysRegInfo(theOp0,theOp1,theOp2,theCRn,theCRm);

          DBG_Assert(!((ri.type & kARM_64BIT) && (ri.opc2 || ri.crn)));
          /* op0 only exists in the AArch64 encodings */
          DBG_Assert((ri.state != kARM_STATE_AA32) || (ri.opc0 == 0));
          /* AArch64 regs are all 64 bit so ARM_CP_64BIT is meaningless */
          DBG_Assert((ri.state != kARM_STATE_AA64) || !(ri.type & kARM_64BIT));
          /* The AArch64 pseudocode CheckSystemAccess() specifies that op1
           * encodes a minimum access level for the register. We roll this
           * runtime check into our general permission check code, so check
           * here that the reginfo's specified permissions are strict enough
           * to encompass the generic architectural permission check.
           */
          if (ri.state != kARM_STATE_AA32) {
              int mask = 0;
              switch (ri.opc1) {
              case 0: case 1: case 2:
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
              DBG_Assert((ri.access & ~mask) == 0);

            // Check for traps on access to all other system registers
            if (ri.accessfn(anInstruction.core()) != kACCESS_OK){
                anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
                anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
            }

          }
      }
      Effect::invoke(anInstruction);
    }

    void describe(std::ostream & anOstream) const {
      anOstream << " CheckSystemAccess ";
      Effect::describe(anOstream);
    }
  };

  Effect * checkSystemAccess(SemanticInstruction * inst, uint8_t anOp0, uint8_t anOp1, uint8_t anOp2, uint8_t aCRn, uint8_t aCRm, uint8_t aRT, uint8_t aRead) {
    return new(inst->icb()) CheckSystemAccess(anOp0, anOp1, anOp2, aCRn, aCRm, aRT, aRead );
  }



  struct CheckDAIFAccess : public Effect {
      uint8_t theOp1;
      CheckDAIFAccess( uint8_t anOp1)
        : theOp1(anOp1)
      {}

      void invoke(SemanticInstruction & anInstruction) {
        FLEXUS_PROFILE();
        if (! anInstruction.isAnnulled()) {
            if (theOp1 == 0x3 /*011*/ && anInstruction.core()->_PSTATE().EL() == EL0 && anInstruction.core()->_SCTLR(EL0).UMA() == 0){
                anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
                anInstruction.core()->takeTrap(boost::intrusive_ptr<nuArchARM::Instruction> (& anInstruction), anInstruction.willRaise());
            }
        }
        Effect::invoke(anInstruction);
      }

      void describe(std::ostream & anOstream) const {
        anOstream << " CheckDAIFAccess";
        Effect::describe(anOstream);
      }
    };

    Effect * checkDAIFAccess(SemanticInstruction * inst, uint8_t anOp1) {
      return new(inst->icb()) CheckDAIFAccess(anOp1);
    }



struct ReadPREffect: public Effect {
  ePrivRegs thePR;
  ReadPREffect( ePrivRegs aPR)
    : thePR(aPR)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t pr = anInstruction.core()->readPR(thePR);
      mapped_reg name = anInstruction.operand< mapped_reg > (kPD);
      SysRegInfo& ri = getPriv(thePR);
      DBG_( Dev, ( << anInstruction << " Read " << ri.name << " value= " << std::hex << pr << std::dec ) );

      uint64_t prVal = ri.readfn(anInstruction.core());
      anInstruction.setOperand(kResult, prVal);

      anInstruction.core()->writeRegister( name, pr );
      anInstruction.core()->bypass( name, pr);
    } else {
      //ReadPR was annulled.  Copy PPD to PD
      mapped_reg dest = anInstruction.operand< mapped_reg > (kPD);
      mapped_reg prev = anInstruction.operand< mapped_reg > (kPPD);
      register_value val = anInstruction.core()->readRegister( prev );
      anInstruction.core()->writeRegister( dest, val);
      anInstruction.setOperand(kResult, val );
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Read PR " << thePR;
    Effect::describe(anOstream);
  }
};

Effect * readPR(SemanticInstruction * inst, ePrivRegs aPR) {
  return new(inst->icb()) ReadPREffect(aPR);
}

struct WritePREffect: public Effect {
  ePrivRegs thePR;
  WritePREffect( ePrivRegs aPR)
    : thePR(aPR)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t rs  = 0;
      SysRegInfo& ri = getPriv(thePR);
      if (anInstruction.hasOperand(kResult)){
          rs = anInstruction.operand< uint64_t > (kResult);
      } else if (anInstruction.hasOperand(kResult1)){
          rs = anInstruction.operand< uint64_t > (kResult1);
      }
      DBG_( Dev, ( << anInstruction << " Write " << ri.name << " value= " << std::hex << rs << std::dec ) );

      ri.writefn(anInstruction.core(), (uint64_t)rs);
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write PR " << thePR;
    Effect::describe(anOstream);
  }
};

Effect * writePR(SemanticInstruction * inst, ePrivRegs aPR) {
  return new(inst->icb()) WritePREffect(aPR);
}

struct WritePSTATE: public Effect {
  uint8_t theOp1, theOp2;
  WritePSTATE( uint8_t anOp1, uint8_t anOp2)
    : theOp1(anOp1)
    , theOp2(anOp2)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {

      uint64_t val = anInstruction.operand< uint64_t > (kResult);
      switch ( (theOp1 << 3) | theOp2) {
      case 0x3: case 0x4:
          anInstruction.setWillRaise(kException_SYSTEMREGISTERTRAP);
          anInstruction.core()->takeTrap(boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction), anInstruction.willRaise());
          break;
      case 0x5: // sp
      {
          SysRegInfo& ri = getPriv(kSPSel);
          ri.writefn(anInstruction.core(), (uint64_t)(val & 1));
        break;
      }
      case 0x1e: // daif set
          anInstruction.core()->setDAIF((uint32_t)val | anInstruction.core()->_PSTATE().DAIF());
        break;
      case 0x1f:  // daif clr
          anInstruction.core()->setDAIF((uint32_t)val ^ anInstruction.core()->_PSTATE().DAIF());
        break;
      default:
          anInstruction.setWillRaise(kException_UNCATEGORIZED);
          anInstruction.core()->takeTrap(boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction), anInstruction.willRaise());
          break;
      }
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write PSTATE ";
    Effect::describe(anOstream);
  }
};

Effect * writePSTATE(SemanticInstruction * inst, uint8_t anOp1, uint8_t anOp2) {
  return new(inst->icb()) WritePSTATE(anOp1, anOp2);
}

struct WriteNZCV: public Effect {
  WriteNZCV()
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
        SysRegInfo & ri = getPriv(kNZCV);

        uint64_t res = anInstruction.operand< uint64_t > (kResult);
        uint64_t val = 0;
        val = PSTATE_N & res;
        if (res == 0) val |= PSTATE_Z;

        ri.writefn(anInstruction.core(), (uint64_t)val);

    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write NZCV ";
    Effect::describe(anOstream);
  }
};

Effect * writeNZCV(SemanticInstruction * inst) {
  return new(inst->icb()) WriteNZCV();
}



struct ClearExclusiveMonitor: public Effect {

    ClearExclusiveMonitor(){}
    void invoke(SemanticInstruction & anInstruction) {
        FLEXUS_PROFILE();
        if (! anInstruction.isAnnulled()) {
            anInstruction.core()->clearExclusiveLocal();
        }
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream & anOstream) const {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};
Effect * clearExclusiveMonitor(SemanticInstruction * inst) {
  return new(inst->icb()) ClearExclusiveMonitor();
}


struct MarkExclusiveMonitor: public Effect {

    eOperandCode theAddressCode;
    eSize theSize;

    MarkExclusiveMonitor(eOperandCode anAddressCode, eSize aSize)
        : theAddressCode (anAddressCode)
        , theSize (aSize)
        {}
    void invoke(SemanticInstruction & anInstruction) {
        FLEXUS_PROFILE();
        if (! anInstruction.isAnnulled()) {

            uint64_t addr = anInstruction.operand< uint64_t > (theAddressCode);
//            bool aligned = (bits(addr) == align(addr, theSize * 8));
            PhysicalMemoryAddress pAddress = anInstruction.translate();

//            FIXME -- waiting for mmu
//            if (anInstruction.core()->transateAddress(addr, kAccType_ATOMIC/*acctype*/, /*iswrite*/false, aligned, theSize) != kNoFault)
//                return;

//            if (address is shareable)
                    anInstruction.core()->markExclusiveGlobal(pAddress, theSize);

            anInstruction.core()->markExclusiveLocal(pAddress, theSize);
            anInstruction.core()->markExclusiveVA(VirtualMemoryAddress(addr), theSize);  // optional
        }
        Effect::invoke(anInstruction);
    }
    void describe(std::ostream & anOstream) const {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};
Effect * markExclusiveMonitor(SemanticInstruction * inst, eOperandCode anAddressCode, eSize aSize) {
  return new(inst->icb()) MarkExclusiveMonitor(anAddressCode, aSize);
}






//It is IMPLEMENTATION DEFINED whether the detection of memory aborts happens
//before or after the check on the local Exclusives monitor. As a result a failure
//of the local monitor can occur on some implementations even if the memory
//access would give an memory abort.
struct ExclusiveMonitorPass: public Effect {

    eOperandCode theAddressCode;
    eSize theSize;

    ExclusiveMonitorPass(eOperandCode anAddressCode, eSize aSize){}

    void invoke(SemanticInstruction & anInstruction) {
//        eAccType acctype = kAccType_ATOMIC;
//        bool iswrite = true;
        uint64_t status = 1;
        uint64_t addr = anInstruction.operand< uint64_t > (theAddressCode);
        bool aligned = (uint64_t(addr) == align((uint64_t)addr, theSize * 8));

        if (!aligned) {
//            bool secondstage = false;
//            AArch64.Abort(address, AArch64.AlignmentFault(acctype, iswrite, secondstage));
            anInstruction.setWillRaise( kException_DATAABORT );
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
          }

        bool passed = anInstruction.core()->isExclusiveVA(VirtualMemoryAddress(addr), theSize);
        if (!passed ) {
            anInstruction.setWillRaise(kException_DATAABORT);
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction> (&anInstruction), anInstruction.willRaise());
        }
        // pending MMY work - check for fault when transalting
//        memaddrdesc = AArch64.TranslateAddress(address, acctype, iswrite, aligned, size);
        // Check for aborts or debug exceptions
//        if IsFault(memaddrdesc) then
//            AArch64.Abort(address, memaddrdesc.fault);
        PhysicalMemoryAddress pAddress = anInstruction.translate();

        passed = anInstruction.core()->isExclusiveLocal(pAddress, theSize);
        if (passed) {
            anInstruction.core()->clearExclusiveLocal();
//            if memaddrdesc.memattrs.shareable then
                passed = anInstruction.core()->isExclusiveGlobal(pAddress, theSize);
                status = 0;
        } else {
            anInstruction.annul();
        }

        mapped_reg name = anInstruction.operand< mapped_reg > (kStatus);
        anInstruction.core()->writeRegister( name, status );


        Effect::invoke(anInstruction);
    }
    void describe(std::ostream & anOstream) const {
        anOstream << " ClearExclusiveMonitor ";
        Effect::describe(anOstream);
    }
};
Effect * exclusiveMonitorPass(SemanticInstruction * inst, eOperandCode anAddressCode, eSize aSize) {
  return new(inst->icb()) ExclusiveMonitorPass(anAddressCode, aSize);
}



struct ExceptionEffect : public Effect {
    eExceptionType theType;
    ExceptionEffect(eExceptionType aType)
        :   theType(aType)
    {}
    void invoke(SemanticInstruction & anInstruction) {
        if (! anInstruction.isAnnulled()) {
            anInstruction.core()->takeTrap(boost::intrusive_ptr<Instruction>(&anInstruction), theType);
        }
        Effect::invoke(anInstruction);
    }

    void describe(std::ostream & anOstream) const {
        anOstream << " ExceptionEffect ";
        Effect::describe(anOstream);
    }
};


Effect * exceptionEffect(SemanticInstruction * inst, eExceptionType aType){
    return new(inst->icb()) ExceptionEffect(aType);
}

struct CommitStoreEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
      DBG_( Dev, ( << anInstruction.identify() << " CommitStoreEffect ") );

    FLEXUS_PROFILE();
    anInstruction.core()->commitStore( boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction) );
    DBG_( Dev   , ( << anInstruction << " Commit store instruction" ) );
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
    DBG_( VVerb, ( << anInstruction.identify() ) );

    anInstruction.core()->accessMem( anInstruction.getAccessAddress(), boost::intrusive_ptr<nuArchARM::Instruction> (&anInstruction)  );
    DBG_( VVerb, ( << anInstruction << " Access memory: " << anInstruction.getAccessAddress()) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " AccessMemEffect";
    Effect::describe(anOstream);
  }
};

Effect * accessMem(SemanticInstruction * inst) {
  return new(inst->icb()) AccessMemEffect( );
}

struct ForceResyncEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( VVerb, ( << anInstruction.identify() << " ForceResyncEffect ") );

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

struct MMUpageFaultCheckEffect: public Effect {
  MMUpageFaultCheckEffect( )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( VVerb, ( << anInstruction << " MMUpageFaultCheckEffect " ) );//NOOSHIN

    anInstruction.core()->checkPageFault( boost::intrusive_ptr<Instruction>(& anInstruction));

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " DMMU TranslationCheckEffect";
    Effect::describe(anOstream);
  }
};

Effect * mmuPageFaultCheck(SemanticInstruction * inst) {
  return new(inst->icb()) MMUpageFaultCheckEffect();
}

struct IMMUExceptionEffect: public Effect {
  IMMUExceptionEffect( )
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();

    eExceptionType exception = anInstruction.retryTranslation();
    if (exception != 0) {
      anInstruction.setWillRaise( exception );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
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
