
// #include <iostream>
// #include <iomanip>

// #include <core/boost_extensions/intrusive_ptr.hpp>
// #include <boost/throw_exception.hpp>
// #include <boost/function.hpp>
// #include <boost/lambda/lambda.hpp>
//  namespace ll = boost::lambda;

// #include <boost/none.hpp>

// #include <boost/dynamic_bitset.hpp>

// #include <core/target.hpp>
// #include <core/debug/debug.hpp>
// #include <core/types.hpp>

// #include <components/uArch/uArchInterfaces.hpp>

// #include "../SemanticInstruction.hpp"
// #include "../Effects.hpp"
// #include "../SemanticActions.hpp"
// #include "PredicatedSemanticAction.hpp"

// #include <components/uArch/systemRegister.hpp>

// #define DBG_DeclareCategories Decoder
// #define DBG_SetDefaultOps AddCat(Decoder)
// #include DBG_Control()

// namespace nDecoder {

// using namespace nuArch;

// struct MSRAction : public PredicatedSemanticAction {
//  uint8_t theOp1, theOp2, theCRm;

//  MSRAction( SemanticInstruction * anInstruction,
//                  eOperandCode anOperandCode,
//                  uint8_t op0,
//                  uint8_t op1,
//                  uint8_t crn,
//                  uint8_t crm,
//                  uint8_t op2,
//                  bool hasCP )
//    : PredicatedSemanticAction( anInstruction, 1, true )
//    , theOp1( op1 )
//    , theOp2( op2 )
//    , theCRm( crm )
//  {
//    setReady( 0, true );
//  }

//  void doEvaluate() {

//    uint8_t op = theOp1 << 3 | theOp2;

//    switch (op) {
//    case 0x3: // FIXME: find out which model of uarch were modelling

//      break;
//    default:
//      break;
//    }

//      // further access checks
//      SysRegInfo& ri = theInstruction->core()->getSysRegInfo(theOp0, theOp1,
//      theOp2, theCRn, theCRm, thehasCP); if
//      (ri->accessfn(theInstruction->core()) == kACCESS_OK){
//          Operand val = ri->readfn(theInstruction->core());
//            theInstruction->setOperand(theOperandCode, val);
//      }
//    satisfyDependants();
//  }

//  void describe( std::ostream & anOstream) const {
//    anOstream << theInstruction->identify() << " Read SYS store in " <<
//    theOperandCode;
//  }

//};

// predicated_action MSRAction
//( SemanticInstruction * anInstruction, uint8_t op1, uint8_t op2, uint8_t crm
//) {
//  MSRAction * act = new MSRAction( anInstruction, op1,
//  op2, crm);
//  anInstruction->addNewComponent(act);

//  return predicated_action( act, act->predicate() );
//}

//} //nDecoder
