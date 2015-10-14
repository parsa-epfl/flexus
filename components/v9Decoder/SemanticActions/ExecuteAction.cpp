#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "PredicatedSemanticAction.hpp"
#include "RegisterValueExtractor.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

struct ExecuteBase : public PredicatedSemanticAction {
  eOperandCode theOperands[5];
  eOperandCode theResult;
  Operation & theOperation;

  ExecuteBase ( SemanticInstruction * anInstruction, std::vector<eOperandCode> & anOperands, eOperandCode aResult, Operation & anOperation)
    : PredicatedSemanticAction( anInstruction, anOperands.size(), true )
    , theResult( aResult )
    , theOperation( anOperation ) {
    for (int32_t i = 0; i < numOperands(); ++i) {
      theOperands[i] = anOperands[i];
    }
  }

  Operand op( eOperandCode aCode) {
    return theInstruction->operand(aCode);
  }
};

struct OperandPrintHelper {
  std::vector< Operand > & theOperands;
  OperandPrintHelper( std::vector<Operand> & operands)
    : theOperands(operands)
  {}
  friend std::ostream & operator << (std::ostream & anOstream, OperandPrintHelper const & aHelper) {
    int32_t i = 0;
    for(auto& anOperand: aHelper.theOperands){
      anOstream << " op" << ++i << "=" << anOperand;
    }
    // std::for_each
    // ( aHelper.theOperands.begin()
    //   , aHelper.theOperands.end()
    //   , ll::var(anOstream) << " op" << ++ ll::var(i) << "=" << ll::_1
    // );
    return anOstream;
  }
};

struct ExecuteAction : public ExecuteBase {
  boost::optional<eOperandCode> theBypass;

  ExecuteAction ( SemanticInstruction * anInstruction, std::vector<eOperandCode> & anOperands, eOperandCode aResult, Operation & anOperation, boost::optional<eOperandCode> aBypass )
    : ExecuteBase( anInstruction, anOperands, aResult, anOperation )
    , theBypass(aBypass) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {

    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {
        std::vector< Operand > operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back( op( theOperands[i] ) );
        }

        Operand result = theOperation( operands );
        theInstruction->setOperand(theResult, result);
        DBG_( Verb, ( << *this << " operands: " << OperandPrintHelper(operands) << " result=" << result ) );
        if (theBypass) {
          mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass);
          register_value val = boost::apply_visitor( register_value_extractor(), result);
          core()->bypass( name,  val);
        }
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " ExecuteAction " << theOperation.describe();
  }
};

struct ExecuteAction_WithXTRA : public ExecuteBase {
  eOperandCode theXTRA;
  boost::optional<eOperandCode> theBypass;
  boost::optional<eOperandCode> theBypassXTRA;

  ExecuteAction_WithXTRA ( SemanticInstruction * anInstruction, std::vector<eOperandCode> & anOperands, eOperandCode aResult, eOperandCode anXTRA, Operation & anOperation, boost::optional<eOperandCode> aBypassResult, boost::optional<eOperandCode> aBypassXTRA  )
    : ExecuteBase( anInstruction, anOperands, aResult, anOperation )
    , theXTRA(anXTRA)
    , theBypass(aBypassResult)
    , theBypassXTRA(aBypassXTRA) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {
        std::vector< Operand > operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back( op( theOperands[i] ) );
        }

        Operand result = theOperation( operands );
        Operand xtra = theOperation.evalExtra( operands );
        theInstruction->setOperand(theResult, result);
        theInstruction->setOperand(theXTRA, xtra);
        DBG_( Verb, ( << *this << " operands: " << OperandPrintHelper(operands) << " result=" << result << " xtra=" << xtra) );
        if (theBypass) {
          mapped_reg name = theInstruction->operand< mapped_reg > (*theBypass);
          register_value val = boost::apply_visitor( register_value_extractor(), result);
          core()->bypass( name,  val);
        }
        if (theBypassXTRA) {
          mapped_reg name = theInstruction->operand< mapped_reg > (*theBypassXTRA);
          register_value val = boost::apply_visitor( register_value_extractor(), xtra);
          core()->bypass( name, val);
        }
        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " ExecuteAction " << theOperation.describe();
  }
};

struct FPExecuteAction : public ExecuteBase {
  eSize theSize;

  FPExecuteAction ( SemanticInstruction * anInstruction, std::vector<eOperandCode> & anOperands, Operation & anOperation, eSize aSize)
    : ExecuteBase( anInstruction, anOperands, (aSize == kByte ? kResultCC : kfResult0), anOperation )
    , theSize(aSize) {
    theInstruction->setExecuted(false);
  }

  void doEvaluate() {
    if (ready()) {
      if (theInstruction->hasPredecessorExecuted()) {
        std::vector< Operand > operands;
        for (int32_t i = 0; i < numOperands(); ++i) {
          operands.push_back( op( theOperands[i] ) );
        }

        theOperation.setContext( core()->getRoundingMode() );
        Operand result = theOperation( operands );
        if (theSize == kDoubleWord) {
          uint64_t val = boost::get<uint64_t>(result);
          theInstruction->setOperand(theResult, val >> 32);
          mapped_reg name0 = theInstruction->operand< mapped_reg > (kPFD0);
          core()->bypass( name0,  val >> 32);
          theInstruction->setOperand(eOperandCode(theResult + 1), val & 0xFFFFFFFFULL);
          mapped_reg name1 = theInstruction->operand< mapped_reg > (kPFD1);
          core()->bypass( name1,  val & 0xFFFFFFFFULL);

        } else if (theSize == kWord) {
          //Word
          theInstruction->setOperand(theResult, result );
          mapped_reg name = theInstruction->operand< mapped_reg > (kPFD0);
          core()->bypass( name,  boost::apply_visitor( register_value_extractor(), result) );
        } else if (theSize == kByte) {
          //Indicates the CC output of an fp_cmp
          theInstruction->setOperand(theResult, result );
          mapped_reg name = theInstruction->operand< mapped_reg > (kCCpd);
          core()->bypass( name, boost::apply_visitor( register_value_extractor(), result));
        }
        DBG_( Verb, ( << *this << " operands: " << OperandPrintHelper(operands) << " val=" << result << " theResult=" << theResult) );

        uint64_t fsr = core()->readFSR();
        if (fsr & 0xf) {
          DBG_(Iface, ( << *this << " clearing exceptions in FSR"));
          core()->writeFSR(fsr & ~0xf);
        }

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " FPExecuteAction " << theOperation.describe();
  }
};

predicated_action executeAction
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , eOperandCode aResult
  , boost::optional<eOperandCode> aBypass
) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back( eOperandCode( kOperand1 + i) );
  }
  ExecuteAction * act(new(anInstruction->icb()) ExecuteAction( anInstruction, operands, aResult, anOperation, aBypass) );

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back( act->dependance(i) );
  }
  return predicated_action( act, act->predicate() );
}

predicated_action executeAction_XTRA
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , std::vector< std::list<InternalDependance> > & opDeps
  , boost::optional<eOperandCode> aBypass
  , boost::optional<eOperandCode> aBypassXTRA
) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back( eOperandCode( kOperand1 + i) );
  }
  ExecuteAction_WithXTRA * act(new(anInstruction->icb()) ExecuteAction_WithXTRA( anInstruction, operands, kResult, kXTRAout, anOperation, aBypass, aBypassXTRA) );

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back( act->dependance(i) );
  }
  return predicated_action( act, act->predicate() );
}

predicated_action fpExecuteAction
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , int32_t num_ops
  , eSize aDestSize
  , eSize aSrcSize
  , std::vector< std::list<InternalDependance> > & deps
) {
  std::vector<eOperandCode> operands;
  if (aSrcSize == kWord) {
    operands.push_back( kFOperand1_0 );
    if ( num_ops == 2) {
      operands.push_back( kFOperand2_0 );
    }
  } else {
    //DoubleWord
    operands.push_back( kFOperand1_0 );
    operands.push_back( kFOperand1_1 );
    if ( num_ops == 2) {
      operands.push_back( kFOperand2_0 );
      operands.push_back( kFOperand2_1 );
    }
  }
  FPExecuteAction * act(new(anInstruction->icb()) FPExecuteAction( anInstruction, operands, anOperation, aDestSize) );

  if (aSrcSize == kWord) {
    deps.resize(2);
    deps[0].push_back( act->dependance(0) );
    if ( num_ops == 2) {
      deps[1].push_back( act->dependance(1) );
    }
  } else {
    //DoubleWord
    deps.resize(4);
    deps[0].push_back( act->dependance(0) );
    deps[1].push_back( act->dependance(1) );
    if ( num_ops == 2) {
      deps[2].push_back( act->dependance(2) );
      deps[3].push_back( act->dependance(3) );
    }
  }

  return predicated_action( act, act->predicate() );
}

simple_action calcAddressAction
( SemanticInstruction * anInstruction
  , std::vector< std::list<InternalDependance> > & opDeps
) {
  std::vector<eOperandCode> operands;
  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    operands.push_back( eOperandCode( kOperand1 + i) );
  }
  ExecuteAction * act(new(anInstruction->icb()) ExecuteAction( anInstruction, operands, kAddress, operation(0 /*add*/), boost::none ) );

  for (uint32_t i = 0; i < opDeps.size(); ++i) {
    opDeps[i].push_back( act->dependance(i) );
  }
  return act;
}

predicated_action visOp
( SemanticInstruction * anInstruction
  , Operation & anOperation
  , std::vector< std::list<InternalDependance> > & deps
) {
  std::vector<eOperandCode> operands;
  //DoubleWord
  operands.push_back( kFOperand1_0 );
  operands.push_back( kFOperand1_1 );
  operands.push_back( kFOperand2_0 );
  operands.push_back( kFOperand2_1 );
  operands.push_back( kOperand5 );

  FPExecuteAction * act(new(anInstruction->icb()) FPExecuteAction( anInstruction, operands, anOperation, kDoubleWord) );

  //DoubleWord
  deps.resize(5);
  deps[0].push_back( act->dependance(0) );
  deps[1].push_back( act->dependance(1) );
  deps[2].push_back( act->dependance(2) );
  deps[3].push_back( act->dependance(3) );
  deps[4].push_back( act->dependance(4) );

  return predicated_action( act, act->predicate() );
}

} //nv9Decoder
