#ifndef FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED
#define FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED

#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

#include "OperandCode.hpp"
#include <components/uArch/RegisterType.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include "Conditions.hpp"
#include "InstructionComponentBuffer.hpp"
#include "Interactions.hpp"

namespace nuArch {
struct uArch;
struct SemanticAction;
}

namespace nv9Decoder {

using Flexus::SharedTypes::VirtualMemoryAddress;
using nuArch::uArch;
using nuArch::eRegisterType;
using nuArch::eOperation;
using nuArch::SemanticAction;
using nuArch::eSize;

struct BaseSemanticAction;
struct SemanticInstruction;

struct Effect : UncountedComponent {
  Effect * theNext;
  Effect() : theNext(0) {}
  virtual ~Effect() {}
  virtual void invoke(SemanticInstruction & anInstruction) {
    if (theNext) {
      theNext->invoke(anInstruction);
    }
  }
  virtual void describe(std::ostream & anOstream) const {
    if (theNext) {
      theNext->describe(anOstream);
    }
  }
  //NOTE: No virtual destructor because effects are never destructed.
};

struct EffectChain {
  Effect * theFirst;
  Effect * theLast;
  EffectChain();
  void invoke(SemanticInstruction & anInstruction);
  void describe(std::ostream & anOstream) const ;
  void append(Effect * anEffect);
  bool empty() const {
    return theFirst == 0;
  }
};

struct DependanceTarget {
  void invokeSatisfy( int32_t anArg ) {
    satisfy(anArg);
  }
  void invokeSquash( int32_t anArg ) {
    squash(anArg);
  }
  virtual ~DependanceTarget() {}
  virtual void satisfy( int32_t anArg) = 0 ;
  virtual void squash( int32_t anArg ) = 0;

protected:
  DependanceTarget() {}
};

struct InternalDependance {
  DependanceTarget * theTarget;
  int32_t theArg;
  InternalDependance( )
    : theTarget( 0 )
    , theArg( 0 )
  {}
  InternalDependance( InternalDependance const & id)
    : theTarget( id.theTarget )
    , theArg( id.theArg)
  {}
  InternalDependance( DependanceTarget * tgt, int32_t arg)
    : theTarget(tgt)
    , theArg(arg)
  {}
  void satisfy() {
    theTarget->satisfy(theArg);
  }
  void squash() {
    theTarget->squash(theArg);
  }
};

Effect * mapSource( SemanticInstruction * inst, eOperandCode anInputCode, eOperandCode anOutputCode);
Effect * freeMapping( SemanticInstruction * inst, eOperandCode aMapping);
Effect * disconnectRegister( SemanticInstruction * inst, eOperandCode aMapping);
Effect * mapDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * mapRD1Destination(SemanticInstruction * inst);
Effect * mapDestination_NoSquashEffects( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * unmapDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * mapFDestination( SemanticInstruction * inst, int32_t anIndex );
Effect * unmapFDestination( SemanticInstruction * inst, int32_t anIndex );
Effect * restorePreviousDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * satisfy( SemanticInstruction * inst, InternalDependance const & aDependance);
Effect * squash( SemanticInstruction * inst, InternalDependance const & aDependance);
Effect * annulNext(SemanticInstruction * inst);
Effect * branch(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * returnFromTrap(SemanticInstruction * inst,  bool isDone);
Effect * branchAfterNext(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * branchAfterNext(SemanticInstruction * inst, eOperandCode aCode);
Effect * branchConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool isFloating);
Effect * branchRegConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition);
Effect * allocateLoad(SemanticInstruction * inst, eSize aSize, InternalDependance const  & aDependance);
Effect * allocateCAS(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance);
Effect * allocateRMW(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance);
Effect * eraseLSQ(SemanticInstruction * inst);
Effect * allocateStore(SemanticInstruction * inst, eSize aSize, bool aBypassSB);
Effect * allocateMEMBAR(SemanticInstruction * inst);
Effect * retireMem(SemanticInstruction * inst);
Effect * commitStore(SemanticInstruction * inst);
Effect * accessMem(SemanticInstruction * inst);
Effect * saveWindow(SemanticInstruction * inst);
Effect * restoreWindow(SemanticInstruction * inst);
Effect * saveWindowPriv(SemanticInstruction * inst);
Effect * restoreWindowPriv(SemanticInstruction * inst);
Effect * saveWindowTrap(SemanticInstruction * inst);
Effect * restoreWindowTrap(SemanticInstruction * inst);
Effect * flushWTrap(SemanticInstruction * inst);
Effect * savedWindow(SemanticInstruction * inst);
Effect * restoredWindow(SemanticInstruction * inst);
Effect * updateConditional(SemanticInstruction * inst);
Effect * updateUnconditional(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * updateUnconditional(SemanticInstruction * inst, eOperandCode anOperandCode);
Effect * updateCall(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * updateNonBranch(SemanticInstruction * inst);
Effect * readPR(SemanticInstruction * inst, uint32_t aPR);
Effect * writePR(SemanticInstruction * inst, uint32_t aPR);
Effect * mapXTRA(SemanticInstruction * inst);
Effect * forceResync(SemanticInstruction * inst);
Effect * immuException(SemanticInstruction * inst);
Effect * dmmuTranslationCheck(SemanticInstruction * inst);
Effect * tccEffect(SemanticInstruction * inst);
Effect * updateFPRS(SemanticInstruction * inst, uint32_t aDestReg);
Effect * writeFPRS(SemanticInstruction * inst);
Effect * recordFPRS(SemanticInstruction * inst);
Effect * readFPRS(SemanticInstruction * inst);
Effect * writeFSR(SemanticInstruction * inst, eSize aSize);
Effect * storeFSR(SemanticInstruction * inst, eSize aSize);

} //nv9Decoder

#endif //FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED
