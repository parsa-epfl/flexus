#ifndef FLEXUS_v9DECODER_SEMANTICACTIONS_HPP_INCLUDED
#define FLEXUS_v9DECODER_SEMANTICACTIONS_HPP_INCLUDED

#include "OperandCode.hpp"
#include <components/uArch/RegisterType.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include "OperandMap.hpp"
#include "Effects.hpp"
#include "SemanticInstruction.hpp"

namespace nuArch {
struct SemanticAction;
struct uArch;
}

namespace nv9Decoder {

using nuArch::SemanticAction;
using nuArch::eRegisterType;
using nuArch::uArch;
using nuArch::eSize;

struct Operation {
  virtual ~Operation() {}
  virtual Operand operator()( std::vector< Operand > const & operands ) = 0;
  virtual Operand evalExtra( std::vector< Operand > const & operands ) {
    return 0;
  }
  virtual void setContext( uint64_t aContext ) {}
  virtual char const * describe() const = 0;
};

Operation & operation( int32_t anOp3Code );
Operation & ccCalc( int32_t anOp3Code );
Operation & fp_op1( int32_t anfop );
Operation & fp_op2( int32_t anfop );
Operation & shift( int32_t aShiftCode, bool a64Bit );

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

  nuArch::uArch * core();

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
);

predicated_action executeAction
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
);

predicated_action executeAction_XTRA
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , boost::optional<eOperandCode> aBypass
  , boost::optional<eOperandCode> aBypassY
);

predicated_action fpExecuteAction
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , int32_t num_ops
  , eSize aDestSize
  , eSize aSrcSize
  , std::vector< std::list<InternalDependance> > & deps
);

predicated_action visOp
( SemanticInstruction * anInstruction
  , Operation & anOperation
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
( SemanticInstruction * anInstruction );

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

predicated_dependant_action rmwAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass );

predicated_dependant_action lddAction
( SemanticInstruction * anInstruction, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1  );

predicated_dependant_action loadFloatingAction
( SemanticInstruction * anInstruction, eSize aSize, boost::optional<eOperandCode> aBypass0, boost::optional<eOperandCode> aBypass1);

predicated_action readPCAction
( SemanticInstruction * anInstruction
);

predicated_action readFPRSAction
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

dependant_action branchCCAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition, bool floating);

dependant_action branchRegAction
( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, uint32_t aCondition);

dependant_action branchToCalcAddressAction
( SemanticInstruction * anInstruction);

} //nv9Decoder

#endif //FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED
