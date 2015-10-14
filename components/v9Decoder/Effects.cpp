#include <iostream>
#include <iomanip>
#include <list>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/performance/profile.hpp>
#include <components/uArch/uArchInterfaces.hpp>

#include "Effects.hpp"
#include "OperandCode.hpp"
#include "SemanticActions.hpp"
#include "SemanticInstruction.hpp"
#include "Interactions.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using nuArch::uArch;
using nuArch::SemanticAction;
using nuArch::Interaction;
using nuArch::Instruction;

using nuArch::ccBits;

using nuArch::eSize;

using nuArch::kLoad;
using nuArch::kStore;
using nuArch::kCAS;
using nuArch::kRMW;

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
    unmapped_reg name( anInstruction.operand< unmapped_reg > (theInputCode) );
    mapped_reg mapped_name = anInstruction.core()->map( name );
    anInstruction.setOperand(theOutputCode, mapped_name);
    DBG_( Verb, ( << anInstruction.identify() << " MapEffect " <<  theInputCode << "(" << name << ")" << " mapped to " << mapped_name << " stored in " << theOutputCode) );
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

/*
struct DisconnectRegisterEffect : public Effect {
  eOperandCode theMappingCode;

  DisconnectRegisterEffect( eOperandCode aMappingCode )
   : theMappingCode( aMappingCode )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );
    DBG_( Verb, ( << anInstruction.identify() << " Disconnect Register " << theMappingCode << "(" << mapping << ")" ) );
    anInstruction.core()->disconnectRegister( mapping, &anInstruction );
    //We no longer disconnect from bypass - we just wait for the register to
    //get unmapped, even though this extends the life of the instruction object
    //anInstruction.core()->disconnectBypass( mapping, &anInstruction );
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "DisconnectRegister " << theMappingCode ;
    Effect::describe(anOstream);
  }
};

Effect * disconnectRegister( SemanticInstruction * inst, eOperandCode aMapping) {
  return new(inst->icb()) DisconnectRegisterEffect( aMapping );
}
*/

struct FreeMappingEffect : public Effect {
  eOperandCode theMappingCode;

  FreeMappingEffect( eOperandCode aMappingCode )
    : theMappingCode( aMappingCode )
  { }

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );
    DBG_( Verb, ( << anInstruction.identify() << " MapEffect free mapping for " << theMappingCode << "(" << mapping << ")" ) );
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
    unmapped_reg name ( anInstruction.operand< unmapped_reg > (theNameCode) );
    mapped_reg mapping ( anInstruction.operand< mapped_reg > (theMappingCode) );

    DBG_( Verb, ( << anInstruction.identify() << " MapEffect restore mapping for " << theNameCode << "(" << name << ") to " << theMappingCode << "( " << mapping << ")" ) );
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
    unmapped_reg name( anInstruction.operand< unmapped_reg > (theInputCode) );
    mapped_reg mapped_name;
    mapped_reg previous_mapping;
    std::tie(mapped_name, previous_mapping ) = anInstruction.core()->create( name );
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

    DBG_( Verb, ( << anInstruction.identify() << " MapEffect new mapping for " << theInputCode << "(" << name << ") to " << mapped_name << " stored in " << theOutputCode << " previous mapping " << previous_mapping << " stored in " << thePreviousMappingCode ) );
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
  void operator() (boost::intrusive_ptr<nuArch::Instruction> anInstruction, nuArch::uArch & aCore) {
    DBG_( Verb, ( << *anInstruction << " Annulled") );
    anInstruction->annul();
  }
  bool annuls() {
    return true;
  }
  void describe(std::ostream & anOstream) const {
    anOstream << "Annul Instruction";
  }
};

nuArch::Interaction * annulInstructionInteraction() {
  return new AnnulInstruction;
}

struct ReinstateInstruction : public Interaction {
  void operator() (boost::intrusive_ptr<nuArch::Instruction> anInstruction, nuArch::uArch & aCore) {
    DBG_( Verb, ( << *anInstruction << " Reinstated") );
    anInstruction->reinstate();
  }
  void describe(std::ostream & anOstream) const {
    anOstream << "Reinstate Instruction";
  }
};

nuArch::Interaction * reinstateInstructionInteraction() {
  return new ReinstateInstruction;
}

struct AnnulNextEffect : public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Verb, ( << anInstruction.identify() << " Annul Next Instruction") );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new AnnulInstruction() );
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

void BranchInteraction::operator() (boost::intrusive_ptr<nuArch::Instruction> anInstruction, nuArch::uArch & aCore) {
  DBG_( Verb, ( << *anInstruction << " " << *this) );
  if (theTarget == 0) {
    theTarget = anInstruction->pc() + 4;
  }

  if (anInstruction->redirectNPC(theTarget) ) {
    DBG_( Verb, ( << *anInstruction << " Branch Redirection.") );
    if ( aCore.squashAfter(anInstruction) ) {
      aCore.redirectFetch(theTarget);
    }
  }
}

void BranchInteraction::describe(std::ostream & anOstream) const {
  anOstream << "Branch to " << theTarget;
}

nuArch::Interaction * branchInteraction( VirtualMemoryAddress aTarget) {
  return new BranchInteraction( aTarget) ;
}

struct BranchFeedbackEffect: public Effect {
  BranchFeedbackEffect( )
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if ( anInstruction.branchFeedback() ) {
      DBG_( Verb, ( << anInstruction << " Update Branch predictor: " << anInstruction.branchFeedback()->theActualType << " " << anInstruction.branchFeedback()->theActualDirection << " to " << anInstruction.branchFeedback()->theActualTarget ) );
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
    DBG_( Verb, ( << anInstruction << " Update Branch predictor: " << theType << " " << theDirection << " to " << target) );
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

struct ReturnFromTrapEffect: public Effect {
  bool theRetry;
  ReturnFromTrapEffect(bool isRetry)
    : theRetry(isRetry)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    VirtualMemoryAddress target;
    VirtualMemoryAddress tnpc;
    int32_t tl = anInstruction.core()->getTL();
    if (tl >= 5 || tl < 0) {
      DBG_( Crit, ( << anInstruction << " TL is out of range in ReturnFromTrapEffect.") );
    }

    //Redirect NPC
    if (theRetry) {
      DBG_( Iface, ( << anInstruction << " RETRY.") );
      target = VirtualMemoryAddress(anInstruction.core()->getTPC(tl));
      tnpc = target + 4;
      if ( anInstruction.core()->getTNPC(tl) != anInstruction.core()->getTPC(tl) + 4) {
        DBG_( Iface, ( << anInstruction << " TNPC is not TPC + 4.  Creating BranchInteraction.") );
        anInstruction.core()->deferInteraction( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction(VirtualMemoryAddress(anInstruction.core()->getTNPC(tl))) );
        tnpc = VirtualMemoryAddress(anInstruction.core()->getTNPC(tl));
      }
    } else {
      DBG_( Iface, ( << anInstruction << " DONE.") );
      target = VirtualMemoryAddress(anInstruction.core()->getTNPC(tl));
      tnpc = VirtualMemoryAddress(anInstruction.core()->getTNPC(tl) + 4);
    }

    if ( anInstruction.redirectNPC(target, tnpc) ) {
      DBG_( Iface, ( << anInstruction << " Return from Trap.") );
      if ( anInstruction.core()->squashAfter(boost::intrusive_ptr<nuArch::Instruction> (&anInstruction)) ) {
        anInstruction.core()->redirectFetch(target);
      }
    }

    //Restore saved state
    anInstruction.core()->popTL();

    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Return from Trap";
    Effect::describe(anOstream);
  }
};

Effect * returnFromTrap(SemanticInstruction * inst,  bool isRetry) {
  return new(inst->icb()) ReturnFromTrapEffect( isRetry);
}

struct BranchEffect: public Effect {
  VirtualMemoryAddress theTarget;
  BranchEffect( VirtualMemoryAddress aTarget)
    : theTarget(aTarget)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    //This effect is only used by ba,a, which need npcReg to be PC + 4
    if ( anInstruction.redirectNPC(theTarget, anInstruction.pc() + 4 ) ) {
      DBG_( Verb, ( << anInstruction << " Branch Misprediction.  Must redirect.") );
      if ( anInstruction.core()->squashAfter(boost::intrusive_ptr<nuArch::Instruction> (&anInstruction)) ) {
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
    DBG_( Verb, ( << anInstruction.identify() << " Branch after next instruction to " << theTarget ) );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );
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
    DBG_( Verb, ( << anInstruction.identify() << " Branch after next instruction to " << theOperandCode << "(" << target << ")" ) );
    anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction(target) );
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
  uint32_t theCondition;
  bool isFloating;
  eOperandCode theCCRCode;
  BranchConditionallyEffect( VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool floating, eOperandCode aCCRCode)
    : theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , isFloating(floating)
    , theCCRCode(aCCRCode)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    mapped_reg name = anInstruction.operand< mapped_reg > (theCCRCode);
    std::bitset<8> cc = boost::get< std::bitset<8> >( anInstruction.core()->readRegister( name ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

    bool result;
    if (isFloating) {
      FCondition cond = fcondition( theCondition );
      result = cond(cc);
      DBG_( Verb, ( << anInstruction << " Floating condition: " << theCondition << " cc: " << cc.to_ulong() << " result: " << result ) );
    } else {
      Condition cond = condition( theCondition );

      result = cond(cc);
    }

    if ( result ) {
      //Taken
      DBG_( Verb, ( << anInstruction << " conditional branch CC: " << cc << " TAKEN" ) );
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );

      feedback->theActualDirection = kTaken;

    } else {
      //Not Taken
      DBG_( Verb, ( << anInstruction << " conditional branch CC: " << cc << " NOT TAKEN" ) );
      if (theAnnul) {
        DBG_( Verb, ( << anInstruction.identify() << " Annul Next Instruction") );
        anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new AnnulInstruction() );
        anInstruction.redirectNPC( anInstruction.pc() + 8 );
      }
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction( anInstruction.pc() + 8 ) );
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

Effect * branchConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool isFloating) {
  return new(inst->icb()) BranchConditionallyEffect( aTarget, anAnnul, aCondition, isFloating, kCCps );
}

struct BranchRegConditionallyEffect: public Effect {
  VirtualMemoryAddress theTarget;
  bool theAnnul;
  uint32_t theCondition;
  eOperandCode theValueCode;
  BranchRegConditionallyEffect( VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, eOperandCode aValueCode)
    : theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , theValueCode(aValueCode)
  {}
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    uint64_t val = anInstruction.operand< uint64_t > (theValueCode);

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = anInstruction.pc();
    feedback->theActualType = kConditional;
    feedback->theActualTarget = theTarget;
    feedback->theBPState = anInstruction.bpState();

    RCondition cond( rcondition(theCondition) );

    if ( cond( val ) ) {
      //Taken
      DBG_( Verb, ( << anInstruction << " register conditional branch TAKEN" ) );
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction(theTarget) );
      feedback->theActualDirection = kTaken;
    } else {
      //Not Taken
      DBG_( Verb, ( << anInstruction << " register conditional branch  NOT TAKEN" ) );
      if (theAnnul) {
        DBG_( Verb, ( << anInstruction.identify() << " Annul Next Instruction") );
        anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new AnnulInstruction() );
        anInstruction.redirectNPC( anInstruction.pc() + 8 );
      }
      anInstruction.core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , new BranchInteraction( anInstruction.pc() + 8 ) );
      feedback->theActualDirection = kNotTaken;
    }
    anInstruction.core()->branchFeedback(feedback);
    Effect::invoke(anInstruction);
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Register Condition Branch to " << theTarget ;
    Effect::describe(anOstream);
  }
};

Effect * branchRegConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition)  {
  return new(inst->icb()) BranchRegConditionallyEffect( aTarget, anAnnul, aCondition, kOperand1 );
}

struct AllocateLSQEffect : public Effect {
  eOperation theOperation;
  eSize theSize;
  bool hasDependance;
  InternalDependance theDependance;
  bool theBypassSB;
  AllocateLSQEffect( eOperation anOperation, eSize aSize, bool aBypassSB, InternalDependance const & aDependance )
    : theOperation(anOperation)
    , theSize(aSize)
    , hasDependance(true)
    , theDependance(aDependance)
    , theBypassSB(aBypassSB)
  {}
  AllocateLSQEffect( eOperation anOperation, eSize aSize, bool aBypassSB)
    : theOperation(anOperation)
    , theSize(aSize)
    , hasDependance(false)
    , theBypassSB(aBypassSB)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Verb, ( << anInstruction.identify() << " Allocate LSQ Entry") );
    if (hasDependance) {
      anInstruction.core()->insertLSQ( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , theOperation, theSize, theBypassSB, anInstruction.makeInstructionDependance(theDependance) );
    } else {
      anInstruction.core()->insertLSQ( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction) , theOperation, theSize, theBypassSB);
    }
    Effect::invoke(anInstruction);
  }

  void describe( std::ostream & anOstream) const {
    anOstream << "Allocate LSQ Entry";
    Effect::describe(anOstream);
  }
};

Effect * allocateStore(SemanticInstruction * inst, eSize aSize, bool aBypassSB)  {
  return new(inst->icb()) AllocateLSQEffect( kStore, aSize, aBypassSB );
}

Effect * allocateMEMBAR(SemanticInstruction * inst )  {
  return new(inst->icb()) AllocateLSQEffect( kMEMBARMarker, kWord, false );
}

Effect * allocateLoad(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance) {
  return new(inst->icb()) AllocateLSQEffect( kLoad, aSize, false, aDependance );
}

Effect * allocateCAS(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance) {
  return new(inst->icb()) AllocateLSQEffect( kCAS, aSize, false, aDependance );
}

Effect * allocateRMW(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance) {
  return new(inst->icb()) AllocateLSQEffect( kRMW, aSize, false, aDependance );
}

struct EraseLSQEffect : public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    DBG_( Verb, ( << anInstruction.identify() << " Free LSQ Entry") );
    anInstruction.core()->eraseLSQ( boost::intrusive_ptr< nuArch::Instruction >( & anInstruction));
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
    FLEXUS_PROFILE();
    anInstruction.core()->retireMem( boost::intrusive_ptr<nuArch::Instruction> (&anInstruction) );
    DBG_( Verb, ( << anInstruction << " Retiring memory instruction" ) );
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
    FLEXUS_PROFILE();
    anInstruction.core()->commitStore( boost::intrusive_ptr<nuArch::Instruction> (&anInstruction) );
    DBG_( Verb, ( << anInstruction << " Commit store instruction" ) );
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
    anInstruction.core()->accessMem( anInstruction.getAccessAddress(), boost::intrusive_ptr<nuArch::Instruction> (&anInstruction)  );
    DBG_( Verb, ( << anInstruction << " Access memory: " << anInstruction.getAccessAddress()) );
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

int32_t regWinTrapType(bool isFill, SemanticInstruction & anInstruction ) {
  int32_t tt;
  if (isFill) {
    tt = 0xC0; /* fill */
  } else {
    tt = 0x80;
  }
  DBG_( Verb, ( << anInstruction << " regWinTrapType isFill: " << isFill << " OTHERWIN: " << std::hex << anInstruction.core()->getOTHERWIN() << " WSTATE: " << anInstruction.core()->getWSTATE() << std::dec) );
  if (anInstruction.core()->getOTHERWIN()) { //OTHERWIN?
    tt |= 0x20;
    tt |= ( ( anInstruction.core()->getWSTATE() >> 3) & 0x7 ) << 2;  //WSTATE.OTHER
  } else {
    tt |= ( anInstruction.core()->getWSTATE() & 0x7 ) << 2; //WSTATE.NORMAL
  }
  return tt;
}

struct SaveTrapEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    DBG_( Verb, ( << anInstruction << " Checking if save will raise.") );
    if (anInstruction.core()->getCANSAVE() == 0) {

      anInstruction.setWillRaise( regWinTrapType( false, anInstruction ) );
      DBG_( Iface, ( << anInstruction << " SaveWindow Raise exception: " << anInstruction.willRaise()  ) );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
    } else if (anInstruction.core()->getCLEANWIN() - anInstruction.core()->getCANRESTORE() == 0) {

      anInstruction.setWillRaise( 0x24 /*clean_win*/ );
      DBG_( Iface, ( << anInstruction << " SaveWindow Raise exception: " << anInstruction.willRaise()  ) );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
    }
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Save Register Window (Check Trap Effects)";
    Effect::describe(anOstream);
  }
};

struct FlushWTrapEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    DBG_( Verb, ( << anInstruction << " Checking if FLUSHW will raise.") );
    if (8 /*NWINDOWS*/ - 2 - anInstruction.core()->getCANSAVE() != 0) {
      anInstruction.setWillRaise( regWinTrapType( false, anInstruction ) );
      DBG_( Iface, ( << anInstruction << " FlushW Raise exception: " << anInstruction.willRaise()  ) );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
    }
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Flush Register Window (Check Trap Effects)";
    Effect::describe(anOstream);
  }
};

Effect * flushWTrap(SemanticInstruction * inst) {
  return new(inst->icb()) FlushWTrapEffect();
}

struct RestoreTrapEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    DBG_( Verb, ( << anInstruction << " Checking if restore will raise.") );
    if (anInstruction.core()->getCANRESTORE() == 0) {

      anInstruction.setWillRaise( regWinTrapType( true, anInstruction ) );
      DBG_( Iface, ( << anInstruction << " RestoreWindow Raise exception: " << anInstruction.willRaise()  ) );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());
    }
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Restore Register Window (Check Trap Effects)";
    Effect::describe(anOstream);
  }
};

Effect * saveWindowTrap(SemanticInstruction * inst) {
  return new(inst->icb()) SaveTrapEffect();
}

Effect * restoreWindowTrap(SemanticInstruction * inst) {
  return new(inst->icb()) RestoreTrapEffect();
}

struct SaveWindowPrivEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    anInstruction.core()->saveWindowPriv();
    anInstruction.setOperand(kocCANSAVE, anInstruction.core()->getCANSAVE() );
    anInstruction.setOperand(kocCANRESTORE, anInstruction.core()->getCANRESTORE() );

    DBG_( Verb, ( << anInstruction << " Save register window privileged effects."  ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Save Register Window (Privileged Effects)";
    Effect::describe(anOstream);
  }
};

Effect * saveWindowPriv(SemanticInstruction * inst) {
  return new(inst->icb()) SaveWindowPrivEffect();
}

struct RestoreWindowPrivEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    anInstruction.core()->restoreWindowPriv();
    anInstruction.setOperand(kocCANSAVE, anInstruction.core()->getCANSAVE() );
    anInstruction.setOperand(kocCANRESTORE, anInstruction.core()->getCANRESTORE() );
    DBG_( Verb, ( << anInstruction << " Restore register window privileged effects."  ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Restore Register Window (Privileged Effects)";
    Effect::describe(anOstream);
  }
};

Effect * restoreWindowPriv(SemanticInstruction * inst) {
  return new(inst->icb()) RestoreWindowPrivEffect();
}

struct SavedEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    anInstruction.core()->saved();
    anInstruction.setOperand(kocCANSAVE, anInstruction.core()->getCANSAVE() );
    anInstruction.setOperand(kocCANRESTORE, anInstruction.core()->getCANRESTORE() );
    anInstruction.setOperand(kocCLEANWIN, anInstruction.core()->getCLEANWIN() );
    anInstruction.setOperand(kocOTHERWIN, anInstruction.core()->getOTHERWIN() );
    DBG_( Verb, ( << anInstruction << " Saved register window."  ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Saved (Privileged Effects)";
    Effect::describe(anOstream);
  }
};

Effect * savedWindow(SemanticInstruction * inst) {
  return new(inst->icb())  SavedEffect();
}

struct RestoredEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    anInstruction.core()->restored();
    anInstruction.setOperand(kocCANSAVE, anInstruction.core()->getCANSAVE() );
    anInstruction.setOperand(kocCANRESTORE, anInstruction.core()->getCANRESTORE() );
    anInstruction.setOperand(kocCLEANWIN, anInstruction.core()->getCLEANWIN() );
    anInstruction.setOperand(kocOTHERWIN, anInstruction.core()->getOTHERWIN() );
    DBG_( Verb, ( << anInstruction << " Restored register window."  ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Restored (Privileged Effects)";
    Effect::describe(anOstream);
  }
};

Effect * restoredWindow(SemanticInstruction * inst) {
  return new(inst->icb()) RestoredEffect();
}

struct SaveWindowEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    uint64_t cwp = anInstruction.core()->saveWindow();
    anInstruction.setOperand(kocCWP, cwp);
    DBG_( Verb, ( << anInstruction << " saving a register window. CWP=" << cwp ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Save Register Window";
    Effect::describe(anOstream);
  }
};

Effect * saveWindow(SemanticInstruction * inst) {
  return new(inst->icb())  SaveWindowEffect();
}

struct RestoreWindowEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    uint64_t cwp = anInstruction.core()->restoreWindow();
    anInstruction.setOperand(kocCWP, cwp);
    DBG_( Verb, ( << anInstruction << " restoring a register window. CWP=" << cwp ) );
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Restore Register Window";
    Effect::describe(anOstream);
  }
};

Effect * restoreWindow(SemanticInstruction * inst) {
  return new(inst->icb()) RestoreWindowEffect();
}

struct ReadPREffect: public Effect {
  uint32_t thePR;
  ReadPREffect( uint32_t aPR)
    : thePR(aPR)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t pr = anInstruction.core()->readPR(thePR);
      mapped_reg name = anInstruction.operand< mapped_reg > (kPD);
      DBG_( Verb, ( << anInstruction << " Read " << anInstruction.core()->prName(thePR) << " value= " << std::hex << pr << std::dec << " rd=" << name) );

      anInstruction.setOperand(kResult, pr);
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

Effect * readPR(SemanticInstruction * inst, uint32_t aPR) {
  return new(inst->icb()) ReadPREffect(aPR);
}

struct WritePREffect: public Effect {
  uint32_t thePR;
  WritePREffect( uint32_t aPR)
    : thePR(aPR)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t pr = anInstruction.operand< uint64_t > (kResult);
      DBG_( Verb, ( << anInstruction << " Write " << anInstruction.core()->prName(thePR) << " value= " << std::hex << pr << std::dec ) );

      anInstruction.setOperand(kResult, pr);
      anInstruction.core()->writePR( thePR, pr );
      anInstruction.setOperand(kocTL, anInstruction.core()->getTL() );
    } else {
      anInstruction.setOperand(kResult, anInstruction.core()->readPR(thePR) );
      anInstruction.setOperand(kocTL, anInstruction.core()->getTL() );

    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write PR " << thePR;
    Effect::describe(anOstream);
  }
};

Effect * writePR(SemanticInstruction * inst, uint32_t aPR) {
  return new(inst->icb()) WritePREffect(aPR);
}

struct ForceResyncEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
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

    int32_t exception = anInstruction.retryTranslation();
    if (exception != 0) {
      anInstruction.setWillRaise( exception );
      DBG_( Verb, ( << anInstruction << " IMMU Exception: " << anInstruction.willRaise()  ) );
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

struct TccEffect: public Effect {
  void invoke(SemanticInstruction & anInstruction) {
    DBG_( Verb, ( << anInstruction << " Checking trap condition.") );

    uint64_t val = anInstruction.operand< uint64_t > (kResult);
    if (val == 0) {
      DBG_( Verb, ( << anInstruction << " TCC condition false.  No trap. " ) );
    } else {
      DBG_( Verb, ( << anInstruction << " TCC raises trap: " << val ) );
      anInstruction.setWillRaise( 0x100 + val );
      anInstruction.core()->takeTrap( boost::intrusive_ptr<Instruction>(& anInstruction), anInstruction.willRaise());

    }
  }
  void describe(std::ostream & anOstream) const {
    anOstream << " Trap on condition";
    Effect::describe(anOstream);
  }
};

Effect * tccEffect(SemanticInstruction * inst) {
  return new(inst->icb()) TccEffect();
}

struct UpdateFPRSEffect: public Effect {
  uint32_t theDestFPReg;
  UpdateFPRSEffect( uint32_t aDestReg)
    : theDestFPReg(aDestReg)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {

      uint64_t fprs = anInstruction.core()->getFPRS();
      if (theDestFPReg < 32) {
        fprs |= 1;  //set DL
        anInstruction.core()->setFPRS(fprs);
      } else {
        fprs |= 2; //set DU
        anInstruction.core()->setFPRS(fprs);
      }
      anInstruction.setOperand(kocFPRS, fprs);
      DBG_( Verb, ( << anInstruction << " Update FPRS value= " << std::hex << fprs << std::dec ) );
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Update FPRS";
    Effect::describe(anOstream);
  }
};

Effect * updateFPRS(SemanticInstruction * inst, uint32_t aDestReg) {
  return new(inst->icb()) UpdateFPRSEffect(aDestReg);
}

struct WriteFPRSEffect: public Effect {
  WriteFPRSEffect()
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t fprs = anInstruction.operand< uint64_t > (kResult);
      DBG_( Verb, ( << anInstruction << " Write FPRS value= " << std::hex << fprs << std::dec ) );

      anInstruction.setOperand(kocFPRS, fprs);
      anInstruction.core()->setFPRS( fprs );
    } else {
      anInstruction.setOperand(kocFPRS, anInstruction.core()->getFPRS() );
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write FPRS";
    Effect::describe(anOstream);
  }
};

Effect * writeFPRS(SemanticInstruction * inst) {
  return new(inst->icb()) WriteFPRSEffect();
}

struct RecordFPRSEffect: public Effect {
  RecordFPRSEffect()
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    uint64_t fprs = anInstruction.core()->getFPRS();
    anInstruction.setOperand(kocFPRS, fprs);
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Record FPRS";
    Effect::describe(anOstream);
  }
};

Effect * recordFPRS(SemanticInstruction * inst) {
  return new(inst->icb()) RecordFPRSEffect();
}

struct ReadFPRSEffect: public Effect {
  ReadFPRSEffect()
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();

    if (! anInstruction.isAnnulled()) {
      uint64_t val = anInstruction.core()->getFPRS();
      mapped_reg name = anInstruction.operand< mapped_reg > (kPD);
      DBG_( Verb, ( << anInstruction << " ReadFPRS value= " << std::hex << val << std::dec << " rd=" << name) );

      anInstruction.setOperand(kResult, val);
      anInstruction.core()->writeRegister( name, val );
      anInstruction.core()->bypass( name, val);
    } else {
      //ReadFPRS was annulled.  Copy PPD to PD
      mapped_reg dest = anInstruction.operand< mapped_reg > (kPD);
      mapped_reg prev = anInstruction.operand< mapped_reg > (kPPD);
      register_value val = anInstruction.core()->readRegister( prev );
      anInstruction.core()->writeRegister( dest, val);
      anInstruction.setOperand(kResult, val );
    }

    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Read FPRS";
    Effect::describe(anOstream);
  }
};

Effect * readFPRS(SemanticInstruction * inst) {
  return new(inst->icb()) ReadFPRSEffect();
}

struct WriteFSREffect: public Effect {
  eSize theSize;
  WriteFSREffect(eSize aSize)
    : theSize(aSize)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t fsr = anInstruction.operand< uint64_t > (kResult);
      DBG_( Verb, ( << anInstruction << " Write FSR value= " << std::hex << fsr << std::dec ) );

      if (theSize == kWord) {
        uint64_t current_fsr = anInstruction.core()->readFSR();
        fsr = (current_fsr & 0xFFFFFFFF00000000ULL) | fsr;
      }
      anInstruction.setOperand(kocFSR, fsr);
      anInstruction.core()->writeFSR( fsr );
    } else {
      anInstruction.setOperand(kocFSR, anInstruction.core()->readFSR() );
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Write FSR";
    Effect::describe(anOstream);
  }
};

Effect * writeFSR(SemanticInstruction * inst, eSize aSize) {
  return new(inst->icb()) WriteFSREffect(aSize);
}

struct StoreFSREffect: public Effect {
  eSize theSize;
  StoreFSREffect(eSize aSize)
    : theSize(aSize)
  {}

  void invoke(SemanticInstruction & anInstruction) {
    FLEXUS_PROFILE();
    if (! anInstruction.isAnnulled()) {
      uint64_t fsr = anInstruction.core()->readFSR();
      anInstruction.setOperand(kocFSR, fsr );
      if (theSize == kWord) {
        fsr &= 0xFFFFFFFFULL;
      }
      DBG_( Verb, ( << anInstruction << " Store FSR value= " << std::hex << fsr << std::dec ) );
      anInstruction.core()->updateStoreValue( boost::intrusive_ptr<Instruction>(& anInstruction), fsr);
    } else {
      anInstruction.setOperand(kocFSR, anInstruction.core()->readFSR() );
    }
    Effect::invoke(anInstruction);
  }

  void describe(std::ostream & anOstream) const {
    anOstream << " Store FSR";
    Effect::describe(anOstream);
  }
};

Effect * storeFSR(SemanticInstruction * inst, eSize aSize) {
  return new(inst->icb()) StoreFSREffect(aSize);
}

} //nv9Decoder
