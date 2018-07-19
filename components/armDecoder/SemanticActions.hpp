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


#ifndef FLEXUS_armDECODER_SEMANTICACTIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_SEMANTICACTIONS_HPP_INCLUDED

#include <boost/tuple/tuple.hpp>

#include "OperandCode.hpp"
#include <components/uArchARM/RegisterType.hpp>
#include <components/CommonQEMU/Slices/MemOp.hpp>
#include "OperandMap.hpp"
#include "Conditions.hpp"
#include "Effects.hpp"
#include "SemanticInstruction.hpp"

namespace nuArchARM {
struct SemanticAction;
struct uArchARM;
}

namespace narmDecoder {



enum eSignCode {
    kSignExtend,
    kZeroExtend,
    kNoExtention,
    kLastExt
};

using nuArchARM::SemanticAction;
using nuArchARM::eRegisterType;
using nuArchARM::uArchARM;
using nuArchARM::eSize;

struct Operation {
  virtual ~Operation() {}
  virtual Operand operator()( std::vector< Operand > const & operands ) = 0;
  virtual Operand evalExtra( std::vector< Operand > const & operands ) {
    return 0;
  }
  virtual char const * describe() const = 0;

  void setOperands(Operand aValue){
        theOperands.push_back(aValue);
        if (!hasOperands)
            hasOperands = true;
  }
  bool hasOwnOperands(){
      return hasOperands;
  }


  virtual uint64_t getNZCVbits() { return theNZCV; }
  virtual bool hasNZCVFlags() { return theNZCVFlags; }
  bool theNZCVFlags;
  uint64_t theNZCV;
  std::vector< Operand > const theOperands;
  bool hasOperands;
};



std::unique_ptr<Operation> operation( eOpType T );

class BaseSemanticAction : public SemanticAction, public UncountedComponent {
private:
  InternalDependance theDependances[5];
  int32_t theEndOfDependances;
  bool theSignalled;
  bool theSquashed;
  bool theReady[5];
  int32_t theNumOperands;

  struct Dep : public DependanceTarget {
    BaseSemanticAction & theAction;
    Dep( BaseSemanticAction & anAction)
      : theAction(anAction)
    {}
    void satisfy(int32_t anArg);
    void squash(int32_t anArg);
  } theDependanceTarget;

protected:
  SemanticInstruction * theInstruction;
  bool theScheduled;

  BaseSemanticAction( SemanticInstruction * anInstruction, int32_t aNumOperands )
    : theEndOfDependances(0)
    , theSignalled( false )
    , theSquashed( false )
    , theNumOperands( aNumOperands )
    , theDependanceTarget(*this)
    , theInstruction(anInstruction)
    , theScheduled( false ) {
    theReady[0] = ( aNumOperands < 1) ;
    theReady[1] = ( aNumOperands < 2) ;
    theReady[2] = ( aNumOperands < 3) ;
    theReady[3] = ( aNumOperands < 4) ;
    theReady[4] = ( aNumOperands < 5) ;
  }

public:
  void evaluate();
  virtual void satisfy(int);
  virtual void squash(int);
  int32_t numOperands() const {
    return theNumOperands;
  }

  int64_t instructionNo() const {
    return theInstruction->sequenceNo();
  }

  InternalDependance dependance(int32_t anArg = 0) {
    DBG_Assert(anArg < theNumOperands);
    return InternalDependance(&theDependanceTarget, anArg);
  }
protected:
  virtual void doEvaluate() {
    DBG_Assert(false);
  }

  void satisfyDependants();
  void squashDependants();

  nuArchARM::uArchARM * core();

  bool cancelled() const {
    return theInstruction->isSquashed() || theInstruction->isRetired();
  }
  bool ready() const {
    return theReady[0] && theReady[1] && theReady[2] && theReady[3] && theReady[4];
  }
  bool signalled() const {
    return theSignalled;
  }
  void setReady(int32_t anArg, bool aReady) {
    theReady[anArg] = aReady;
  }

  void addRef();
  void releaseRef();
  void reschedule();

public:
  void addDependance( InternalDependance const & aDependance);
  virtual ~BaseSemanticAction() {}
};

struct simple_action {
  BaseSemanticAction * action;
  simple_action() {}
  simple_action( BaseSemanticAction * act )
    : action(act)
  {}
};

struct predicated_action : virtual simple_action {
  InternalDependance predicate;
  predicated_action( ) {}
  predicated_action( BaseSemanticAction * act, InternalDependance const & pred)
    : simple_action(act)
    , predicate(pred)
  {}
};

struct dependant_action : virtual simple_action {
  InternalDependance dependance;
  dependant_action( ) {}
  dependant_action( BaseSemanticAction * act, InternalDependance const & dep)
    : simple_action(act)
    , dependance(dep)
  {}
};

struct predicated_dependant_action : predicated_action, dependant_action {
  predicated_dependant_action( ) {}
  predicated_dependant_action( BaseSemanticAction * act, InternalDependance const & dep, InternalDependance const & pred)
    : simple_action(act)
    , predicated_action(act, pred)
    , dependant_action( act, dep)
  {}
};

struct multiply_dependant_action : virtual simple_action {
  std::vector< InternalDependance > dependances;
  multiply_dependant_action( ) {}
  multiply_dependant_action( BaseSemanticAction * act, std::vector<InternalDependance> & dep)
    : simple_action(act) {
    std::swap(dependances, dep);
  }
};

void connectDependance( InternalDependance const & aDependant, simple_action  & aSource);
void connect( std::list<InternalDependance > const & dependances, simple_action & aSource);

simple_action readRegisterAction
( SemanticInstruction * anInstruction
  , eOperandCode aRegisterCode
  , eOperandCode anOperandCode
  , bool is64  = true
);

predicated_action executeAction
( SemanticInstruction * anInstruction
  , std::unique_ptr<Operation> & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
);

predicated_action executeAction_XTRA
( SemanticInstruction * anInstruction
  , std::unique_ptr<Operation> & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , boost::optional<eOperandCode> aBypass
  , boost::optional<eOperandCode> aBypassY
);

predicated_action fpExecuteAction
( SemanticInstruction * anInstruction
  , std::unique_ptr<Operation> & anOperation
  , int32_t num_ops
  , eSize aDestSize
  , eSize aSrcSize
  , std::vector< std::list<InternalDependance> > & deps
);

predicated_action visOp
( SemanticInstruction * anInstruction
  , std::unique_ptr<Operation> & anOperation
  , std::vector< std::list<InternalDependance> > & deps
);

simple_action calcAddressAction
( SemanticInstruction * anInstruction
  , std::vector< std::list<InternalDependance> > & opDeps
);

dependant_action writebackAction
( SemanticInstruction * anInstruction
  , eRegisterType aType
);

dependant_action writebackRD1Action ( SemanticInstruction * anInstruction );

dependant_action writeXTRA
( SemanticInstruction * anInstruction );

dependant_action floatingWritebackAction
( SemanticInstruction * anInstruction
  , int32_t anIndex
);

predicated_action annulAction
( SemanticInstruction * anInstruction
  , eRegisterType aType
);

predicated_action annulRD1Action( SemanticInstruction * anInstruction );

predicated_action annulXTRA
( SemanticInstruction * anInstruction );

predicated_action floatingAnnulAction
( SemanticInstruction * anInstruction
  , int32_t anIndex
);

multiply_dependant_action updateAddressAction
( SemanticInstruction * anInstruction );

multiply_dependant_action updateCASAddressAction
( SemanticInstruction * anInstruction );

predicated_dependant_action updateRMWValueAction
( SemanticInstruction * anInstruction );

predicated_dependant_action updateStoreValueAction
( SemanticInstruction * anInstruction, eOperandCode data );


multiply_dependant_action updateSTDValueAction
( SemanticInstruction * anInstruction );

multiply_dependant_action updateCASValueAction
( SemanticInstruction * anInstruction );

multiply_dependant_action updateFloatingStoreValueAction
( SemanticInstruction * anInstruction, eSize aSize );

predicated_dependant_action loadAction
( SemanticInstruction * anInstruction, eSize aSize, bool aSignExtend, boost::optional<eOperandCode> aBypass );

predicated_dependant_action casAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass );

predicated_dependant_action caspAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass );


predicated_dependant_action rmwAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass );

predicated_dependant_action lddAction
( SemanticInstruction * anInstruction, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1  );

predicated_dependant_action ldpAction
( SemanticInstruction * anInstruction, eSize aSize, bool aSignCode, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1  );


predicated_dependant_action loadFloatingAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1);

predicated_action readPCAction
( SemanticInstruction * anInstruction
);

predicated_action readFPSRAction
( SemanticInstruction * anInstruction
);

predicated_action readTICKAction
( SemanticInstruction * anInstruction
);

predicated_action readSTICKAction
( SemanticInstruction * anInstruction
);

predicated_action constantAction
( SemanticInstruction * anInstruction
  , uint64_t aConstant
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
);

predicated_action operandAction
( SemanticInstruction * anInstruction
  , eOperandCode anOperand
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
);

dependant_action branchCCAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, Condition & aCondition, bool floating);

dependant_action branchRegAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition);

dependant_action branchToCalcAddressAction
( SemanticInstruction * anInstruction);

} //narmDecoder

#endif //FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED
