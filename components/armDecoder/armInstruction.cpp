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


#include <list>
#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/none.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

#include "armInstruction.hpp"
#include "SemanticInstruction.hpp"
#include "SemanticActions.hpp"
#include "Effects.hpp"
#include "Validations.hpp"
#include "Constraints.hpp"
#include "Interactions.hpp"
#include "armBitManip.hpp"

#include <bitset>         // std::bitset


#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

namespace Stat = Flexus::Stat;

using nuArchARM::xRegisters;
using nuArchARM::vRegisters;
using nuArchARM::ccBits;

using namespace nuArchARM;

typedef boost::intrusive_ptr<armInstruction>  arminst;
typedef Flexus::SharedTypes::FetchedOpcode armcode;


struct BlackBoxInstruction : public armInstruction {

  BlackBoxInstruction(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, uint32_t  aCPU, int64_t aSequenceNo)
    : armInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo) {
      DBG_(Tmp,(<< "\033[2;31m DECODER: BLACKBOXING \033[0m"));
    setClass(clsSynchronizing, codeBlackBox);
    forceResync();
  }

  //BlackBoxInstruction has no dependencies and is always ready to retire
  virtual bool mayRetire() const {
    return true;
  }

  //BlackBoxInstruction always causes a resync with Simics
  virtual bool postValidate() {
    return false;
  }

  virtual void describe(std::ostream & anOstream) const {
    armInstruction::describe(anOstream);
    anOstream << instCode();
  }
};

struct MAGIC : public armInstruction {
  MAGIC(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, uint32_t aCPU, int64_t aSequenceNo)
    : armInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo) {
    DBG_(Tmp, (<<"In MAGIC"));//NOOSHIN
    setClass(clsSynchronizing, codeMAGIC);

  }

  virtual bool mayRetire() const {
    return true;
  }
  virtual bool preValidate() {
    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
    if ( cpu->getPC()  != thePC ) {
      DBG_( Tmp, ( << *this << " PreValidation failed: PC mismatch flexus=" << thePC << " simics=" << cpu->getPC() ) );
    }
//    if ( cpu->getNPC()  != theNPC ) {
//      DBG_( Tmp, ( << *this << " PreValidation failed: NPC mismatch flexus=" << theNPC << " simics=" << cpu->getNPC() ) );
//    }
    return
      (   cpu->getPC()     == thePC
          //&&  cpu->getNPC() == theNPC
      );
  }

  virtual bool postValidate() {
//    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
//    if ( cpu->getPC()  != theNPC ) {
//      DBG_( Tmp, ( << *this << " PostValidation failed: PC mismatch flexus=" << theNPC << " simics=" << cpu->getPC() ) );
//    }
//    return ( cpu->getPC()  == theNPC ) ;
      return true;
  }

  virtual void describe(std::ostream & anOstream) const {
    armInstruction::describe(anOstream);
    anOstream << "MAGIC";
  }
};


void armInstruction::describe(std::ostream & anOstream) const {
  Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
  anOstream << printInstClass()
            << "#" << theSequenceNo << "[" << std::setfill('0') << std::right << std::setw(2) << cpu->id() <<  "] "
            << "@" << thePC  << " |" << std::hex << std::setw(8) << theOpcode << std::dec << "| "
            << std::left << std::setw(30) << std::setfill(' '); //<< disassemble();
  if ( theRaisedException) {
    anOstream << " {raised " << cpu->describeException(theRaisedException) << "(" << theRaisedException << ")} ";
  }
  if (theResync) {
    anOstream << " {force-resync}";
  }
  if (haltDispatch()) {
    anOstream << " {halt-dispatch}";
  }
}

std::string armInstruction::disassemble() const {
  return Flexus::Qemu::Processor::getProcessor(theCPU)->disassemble(thePC);
}

void armInstruction::setWillRaise(int32_t aSetting) {
  DBG_( Tmp, ( << *this << " setWillRaise: 0x" << std::hex << aSetting << std::dec ) ) ;//NOOSHIN
  theWillRaise = aSetting;
}

void armInstruction::doDispatchEffects(  ) {
  if (bpState() && ! isBranch()) {
    //Branch predictor identified an instruction that is not a branch as a
    //branch.
    DBG_( Tmp, ( << *this << " predicted as a branch, but is a non-branch.  Fixing" ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = pc();
    feedback->theActualType = kNonBranch;
    feedback->theActualDirection = kNotTaken;
    feedback->theActualTarget = VirtualMemoryAddress(0);
    feedback->theBPState = bpState();
    core()->branchFeedback(feedback);
    core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( this ) , new BranchInteraction(VirtualMemoryAddress(0)) );

  }
}


////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
///
///
///




class ArmFormat
{
  uint32_t opcode;
public:
  ArmFormat( uint32_t op)
    : opcode(op)
  {}
  uint32_t rd() const {
    return extract32(opcode, 0, 5);
  }
  uint32_t rn() const {
    return extract32(opcode, 5, 5);
  }
  uint32_t immr() const {
    return extract32(opcode, 16, 6);
  }
  uint32_t imms() const {
    return extract32(opcode, 10, 6);
  }
  uint32_t opc() const {
    return extract32(opcode, 29, 2);
  }
  uint32_t sf() const {
    return extract32(opcode, 31, 1);
  }
  uint32_t is_n() const {
    return extract32(opcode, 22, 1);
  }
  uint32_t is_64bit() const {
    return extract32(opcode, 31, 1);
  }
  uint32_t imm() const {
    return extract32(opcode, 5, 16);
  }
  uint32_t pos() const {
    return extract32(opcode, 21, 2) << 4;
  }
  uint32_t ri() const {
    return extract32(opcode, 16, 6);
  }
  uint32_t si() const {
    return extract32(opcode, 10, 6);
  }
  uint32_t n() const {
    return extract32(opcode, 22, 1);
  }
  uint32_t rm() const {
    return extract32(opcode, 16, 5);
  }
  uint32_t op21() const {
    return extract32(opcode, 29, 2);
  }
  uint32_t op0() const {
    return extract32(opcode, 21, 1);
  }
  uint32_t op2() const {
    return extract32(opcode, 16, 5);
  }
  uint32_t op3() const {
    return extract32(opcode, 10, 6);
  }
  uint32_t l() const {
    return extract32(opcode, 21, 1);
  }
  uint32_t op2_ll() const {
    return extract32(opcode, 0, 5);
  }
  uint32_t imm16() const {
      return extract32(opcode, 5, 16);
    }
};
void setRD( SemanticInstruction * inst, uint32_t rd) {
  DBG_(Tmp, (<< "\e[1;35m"<<"DECODER: EFFECT: Writing to x[" << rd << "]"<<"\e[0m"));
  reg regRD;
  regRD.theType = xRegisters;
  regRD.theIndex = rd;
  inst->setOperand( kRD, regRD );
}

void setRD1( SemanticInstruction * inst, uint32_t rd1) {
  reg regRD1;
  regRD1.theType = xRegisters;
  regRD1.theIndex = rd1;
  inst->setOperand( kRD1, regRD1 );
}

void setRS( SemanticInstruction * inst, eOperandCode rs_code, uint32_t rs) {
  reg regRS;
  regRS.theType = xRegisters;
  regRS.theIndex = rs;
  inst->setOperand(rs_code, regRS);
}

void setCC( SemanticInstruction * inst, int32_t ccNum, eOperandCode rs_code) {
  reg regICC;
  regICC.theType = ccBits;
  regICC.theIndex = ccNum;
  inst->setOperand(rs_code, regICC);
}

void setFD( SemanticInstruction * inst, eOperandCode fd_no, uint32_t fd) {
  reg regFD;
  regFD.theType = vRegisters;
  regFD.theIndex = fd;
  inst->setOperand( fd_no, regFD );
}

void setFS( SemanticInstruction * inst, eOperandCode fs_no, uint32_t fs) {
  reg regFS;
  regFS.theType = vRegisters;
  regFS.theIndex = fs;
  inst->setOperand(fs_no, regFS);
}

void setCCd( SemanticInstruction * inst) {
  reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 0;
  inst->setOperand( kCCd, regCCd );
}

void setFCCd( int32_t fcc, SemanticInstruction * inst) {
  reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 1 + fcc;
  inst->setOperand( kCCd, regCCd );
}

arminst nop( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeNOP);
  return inst;
}

void addReadRD( SemanticInstruction * inst, uint32_t rd) {

    predicated_dependant_action update_value = updateStoreValueAction( inst );

    setRD( inst, rd);

    inst->addDispatchEffect( mapSource( inst, kRD, kPD ) );
    simple_action read_value = readRegisterAction( inst, kPD, kResult );
    inst->addDispatchAction( read_value );

    connectDependance( update_value.dependance, read_value );
    connectDependance( inst->retirementDependance(), update_value );

    inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
    inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );
}

void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances) {
  std::list< InternalDependance >::iterator iter, end;
  for ( iter = dependances.begin(), end = dependances.end(); iter != end; ++iter) {
    inst->addDispatchEffect( satisfy( inst, *iter ) );
  }
}


void addReadFDs( SemanticInstruction * inst, uint32_t fd, eSize aSize, std::vector<InternalDependance> & aValueDependance) {

  switch ( aSize ) {
    case kWord: {
      setFD(inst, kFD0, fd);
      inst->addDispatchEffect( mapSource( inst, kFD0, kPFD0 ) );
      simple_action read_value = readRegisterAction( inst, kPFD0, kfResult0 );
      inst->addDispatchAction( read_value );
      inst->addPrevalidation( validateFRegister( fd, kfResult0, inst ) );

      connectDependance( aValueDependance[0], read_value );
      inst->addDispatchEffect( satisfy( inst, aValueDependance[1] ) );
      inst->addPostvalidation( validateMemory( kAddress, kOperand3, kfResult0, kWord, inst ) );

      break;
    }
    case kDoubleWord: {
      //Move fd[0] to fd[5]
      boost::dynamic_bitset<> fdb(6, fd);
      fdb[5] = fdb[0];
      fdb[0] = 0;
      int32_t actual_fd = fdb.to_ulong();
      setFD(inst, kFD0, actual_fd);
      setFD(inst, kFD1, actual_fd | 1);

      inst->addDispatchEffect( mapSource( inst, kFD0, kPFD0 ) );
      simple_action read_value = readRegisterAction( inst, kPFD0, kfResult0 );
      inst->addDispatchAction( read_value );
      inst->addPrevalidation( validateFRegister( actual_fd, kfResult0, inst ) );

      inst->addDispatchEffect( mapSource( inst, kFD1, kPFD1 ) );
      simple_action read_value1 = readRegisterAction( inst, kPFD1, kfResult1 );
      inst->addDispatchAction( read_value1 );
      inst->addPrevalidation( validateFRegister( actual_fd | 1, kfResult1, inst ) );

      connectDependance( aValueDependance[0], read_value );
      connectDependance( aValueDependance[1], read_value1 );
      inst->addPostvalidation( validateMemory( kAddress, kOperand3, kfResult0, kWord, inst ) );
      inst->addPostvalidation( validateMemory( kAddress, 4, kOperand3, kfResult1, kWord, inst ) );

      break;
    }
    default:
      DBG_Assert(false);
  }
}

void addReadFValue( SemanticInstruction * inst, uint32_t fd, eSize aSize) {
  //Set up the mapping and readRegister actions.  Make the execute action
  //depend on reading the register value

  multiply_dependant_action update_value = updateFloatingStoreValueAction( inst, aSize );

  addReadFDs( inst, fd, aSize, update_value.dependances );

  connectDependance( inst->retirementDependance(), update_value );
}

void addReadCC( SemanticInstruction * inst, int32_t ccNum, int32_t anOpNumber, std::list<InternalDependance> & dependances) {
  //Calculate operand codes from anOpNumber
  DBG_Assert( anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3);
  eOperandCode cOperand = eOperandCode( kOperand1 + anOpNumber - 1);
  eOperandCode cRS = eOperandCode( kRS1 + anOpNumber - 1);
  eOperandCode cPS = eOperandCode( kPS1 + anOpNumber - 1);

  //Set up the mapping and readRegister actions.  Make the execute action
  //depend on reading the register value
  setCC( inst, ccNum, cRS);

  inst->addDispatchEffect( mapSource( inst, cRS, cPS ) );
  simple_action act = readRegisterAction( inst, cPS, cOperand );
  connect( dependances, act );
  inst->addDispatchAction( act );
}

void addReadXRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances, bool is_64 = true) {

    //Calculate operand codes from anOpNumber
    DBG_Assert( anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 || anOpNumber == 5 );
    eOperandCode cOperand = eOperandCode( kOperand1 + anOpNumber - 1);
    eOperandCode cRS = eOperandCode( kRS1 + anOpNumber - 1);
    eOperandCode cPS = eOperandCode( kPS1 + anOpNumber - 1);

    DBG_(Tmp, (<< "\e[1;31m"<<"DECODER: Reading x[" << rs << "] with opNumber " << cRS << " and storing it in " << cPS << "\e[0m"));   
    setRS( inst, cRS , rs );
    inst->addDispatchEffect( mapSource( inst, cRS, cPS ) );
    simple_action act = readRegisterAction( inst, cPS, cOperand, is_64 );
    connect( dependances, act );
    inst->addDispatchAction( act );
//    inst->addPrevalidation( validateRegister( rs, cOperand, inst ) );

}

void addAnnulment( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, InternalDependance const & aWritebackDependance) {
  predicated_action annul = annulAction( inst, aType );
  //inst->addDispatchAction( annul );

  connectDependance( aWritebackDependance, annul );

  inst->addAnnulmentEffect( satisfy( inst, annul.predicate ) );
  inst->addAnnulmentEffect( squash( inst, exec.predicate ) );
  inst->addReinstatementEffect( squash( inst, annul.predicate ) );
  inst->addReinstatementEffect( satisfy ( inst, exec.predicate ) );
}

void addWriteback( SemanticInstruction * inst, eRegisterType aType, predicated_action & exec, bool addSquash = true) {
  if (addSquash) {
    inst->addDispatchEffect( mapDestination( inst, aType ) );
  } else {
    inst->addDispatchEffect( mapDestination_NoSquashEffects( inst, aType ) );
  }

  //Create the writeback action
  dependant_action wb = writebackAction( inst, aType );
  //inst->addDispatchAction( wb );

  addAnnulment( inst, aType, exec, wb.dependance );

  //Make writeback depend on execute, make retirement depend on writeback
  connectDependance( wb.dependance, exec );
  connectDependance( inst->retirementDependance(), wb );
}
void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool addSquash = true) {
//  if (rd != 0) {
    setRD( inst, rd);
    addWriteback( inst, xRegisters, exec, addSquash );
//    inst->addPostvalidation( validateRegister( rd, kResult, inst  ) );
    inst->addOverride( overrideRegister( rd, kResult, inst ) );
//  } else {
    //No need to write back.  Make retirement depend on execution
//    InternalDependance dep( inst->retirementDependance() );
//    connectDependance( dep, exec );
//  }


}

void ArmFormatOperands( SemanticInstruction * inst, uint32_t reg,
                      std::vector<std::list<InternalDependance> > & rs_deps)
{
    DBG_Assert( ! rs_deps[0].empty() , ( << "rs_deps empty in format3Operands - format3operands call must occur after execute action is constructed." ) );
    addReadXRegister( inst, 1, reg, rs_deps[0] );
//    if (operands.is_simm13()) {
//      addSimm13( inst, operands.simm13(), kOperand2, rs_deps[1] );
//    } else {
//      addReadXRegister( inst, 2, operands.rs2(), rs_deps[1] );
//    }
}

predicated_action addExecute_XTRA( SemanticInstruction * inst, Operation & anOperation, uint32_t rd, std::vector< std::list<InternalDependance> > & rs_deps, bool write_xtra) {
  predicated_action exec;
//  if (rd == 0) {
//    exec = executeAction_XTRA( inst, anOperation, rs_deps, boost::none, (write_xtra ? boost::optional<eOperandCode>(kXTRApd) : boost::none));
//  } else {
    exec = executeAction_XTRA( inst, anOperation, rs_deps, kPD, (write_xtra ? boost::optional<eOperandCode>(kXTRApd) : boost::none ));
//  }
  //inst->addDispatchAction( exec );
  return exec;
}

predicated_action addExecute( SemanticInstruction * inst, Operation & anOperation, std::vector< std::list<InternalDependance> > & rs_deps) {
  predicated_action exec;

    exec = executeAction( inst, anOperation, rs_deps, kResult, kPD );
    inst->addDispatchAction( exec );
    return exec;
}

void  mov( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps ,uint64_t dst)
{
    predicated_action exec = addExecute( inst, operation(kMOV_), rs_deps ) ;
    addDestination( inst, dst, exec );
}
void addFloatingDestination( SemanticInstruction * inst, uint32_t fd, eSize aSize, predicated_action & exec) {

//  switch ( aSize ) {
//    case kWord: {
//      setFD(inst, kFD0, fd);
//      addFloatingWriteback( inst, 0, exec );
//      inst->addRetirementEffect( updateFPRS(inst, fd) );
//      inst->addPostvalidation( validateFPRS(inst) );
//      inst->addPostvalidation( validateFRegister( fd, kfResult0, inst  ) );
//      inst->addOverride( overrideFloatSingle( fd, kfResult0, inst ) );
//      break;
//    }
//    case kDoubleWord: {
//      //Move fd[0] to fd[5]
//      boost::dynamic_bitset<> fdb(6, fd);
//      fdb[5] = fdb[0];
//      fdb[0] = 0;
//      int32_t actual_fd = fdb.to_ulong();
//      setFD(inst, kFD0, actual_fd);
//      setFD(inst, kFD1, actual_fd | 1);
//      inst->addRetirementEffect( updateFPRS(inst, actual_fd) );
//      inst->addPostvalidation( validateFPRS(inst) );
////      addFloatingWriteback( inst, 0, exec );
////      addFloatingWriteback( inst, 1, exec );
//      addDoubleFloatingWriteback( inst, exec ); // Use specialized function to avoid squash->satisfy->squash deadlock problem with annulment/reinstatement
//      inst->addPostvalidation( validateFRegister( actual_fd, kfResult0, inst  ) );
//      inst->addPostvalidation( validateFRegister( actual_fd | 1, kfResult1, inst  ) );
//      inst->addOverride( overrideFloatDouble( actual_fd, kfResult0, kfResult1, inst ) );
//      break;
//    }
//    default:
//      DBG_Assert(false);
//  }
}

void addAddressCompute( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps) {
    DBG_(Tmp, (<< "\e[1;31m"<<"DECODER: Computing address" << "\e[0m"));

  multiply_dependant_action update_address = updateAddressAction( inst );
  inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );
  simple_action exec = calcAddressAction( inst, rs_deps);
  inst->addDispatchAction( exec );
  connectDependance( update_address.dependances[0], exec);
  connectDependance( inst->retirementDependance(), update_address );
}

SemanticInstruction  * jmpl( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeBranchUnconditional);

  ArmFormat operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);

  simple_action tgt = calcAddressAction( inst, rs_deps );
  //inst->addDispatchAction( tgt );

  dependant_action br = branchToCalcAddressAction( inst );

  connectDependance( br.dependance, tgt );
  connectDependance( inst->retirementDependance(), br );

  ArmFormatOperands( inst, operands.rn(), rs_deps );

  //inst->addRetirementEffect( branchAfterNext( inst, kAddress ) ); - Now done via branchToCalcAddressAction
  inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

  predicated_action exec = readPCAction( inst );
  inst->addDispatchAction( exec );

  addDestination( inst, operands.rd(), exec);

  return inst;
}


arminst blackBox( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
  return arminst( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

arminst grayBox( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, eInstructionCode aCode )
{
  arminst inst( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->forceResync();
  DBG_(Tmp, (<<"In grayBox"));
  inst->setClass(clsSynchronizing, aCode);
  return inst;
}

/* MRS - move from system register
 * MSR (register) - move to system register
 * SYS
 * SYSL
 * These are all essentially the same insn in 'read' and 'write'
 * versions, with varying op0 fields.
 */
static void handle_sys(uint32_t insn, bool isread,
                       unsigned int op0, unsigned int op1, unsigned int op2,
                       unsigned int crn, unsigned int crm, unsigned int rt)
{
//    const ARMCPRegInfo *ri;
//    TCGv_i64 tcg_rt;

//    ri = get_arm_cp_reginfo(s->cp_regs,
//                            ENCODE_AA64_CP_REG(CP_REG_ARM64_SYSREG_CP,
//                                               crn, crm, op0, op1, op2));

//    if (!ri) {
//        /* Unknown register; this might be a guest error or a QEMU
//         * unimplemented feature.
//         */
//        qemu_log_mask(LOG_UNIMP, "%s access to unsupported AArch64 "
//                      "system register op0:%d op1:%d crn:%d crm:%d op2:%d",
//                      isread ? "read" : "write", op0, op1, crn, crm, op2);
//        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        return;
//    }

//    /* Check access permissions */
//    if (!cp_access_ok(s->current_el, ri, isread)) {
//        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        return;
//    }

//    if (ri->accessfn) {
//        /* Emit code to perform further access permissions checks at
//         * runtime; this may result in an exception.
//         */
//        TCGv_ptr tmpptr;
//        TCGv_i32 tcg_syn, tcg_isread;
//        uint32_t syndrome;

//        gen_a64_set_pc_im(s->pc - 4);
//        tmpptr = tcg_const_ptr(ri);
//        syndrome = syn_aa64_sysregtrap(op0, op1, op2, crn, crm, rt, isread);
//        tcg_syn = tcg_const_i32(syndrome);
//        tcg_isread = tcg_const_i32(isread);
//        gen_helper_access_check_cp_reg(cpu_env, tmpptr, tcg_syn, tcg_isread);
//        tcg_temp_free_ptr(tmpptr);
//        tcg_temp_free_i32(tcg_syn);
//        tcg_temp_free_i32(tcg_isread);
//    }

//    /* Handle special cases first */
//    switch (ri->type & ~(ARM_CP_FLAG_MASK & ~ARM_CP_SPECIAL)) {
//    case ARM_CP_NOP:
//        return;
//    case ARM_CP_NZCV:
//        tcg_rt = cpu_reg(s, rt);
//        if (isread) {
//            gen_get_nzcv(tcg_rt);
//        } else {
//            gen_set_nzcv(tcg_rt);
//        }
//        return;
//    case ARM_CP_CURRENTEL:
//        /* Reads as current EL value from pstate, which is
//         * guaranteed to be constant by the tb flags.
//         */
//        tcg_rt = cpu_reg(s, rt);
//        tcg_gen_movi_i64(tcg_rt, s->current_el << 2);
//        return;
//    case ARM_CP_DC_ZVA:
//        /* Writes clear the aligned block of memory which rt points into. */
//        tcg_rt = cpu_reg(s, rt);
//        gen_helper_dc_zva(cpu_env, tcg_rt);
//        return;
//    default:
//        break;
//    }

//    if ((s->base.tb->cflags & CF_USE_ICOUNT) && (ri->type & ARM_CP_IO)) {
//        gen_io_start();
//    }

//    tcg_rt = cpu_reg(s, rt);

//    if (isread) {
//        if (ri->type & ARM_CP_CONST) {
//            tcg_gen_movi_i64(tcg_rt, ri->resetvalue);
//        } else if (ri->readfn) {
//            TCGv_ptr tmpptr;
//            tmpptr = tcg_const_ptr(ri);
//            gen_helper_get_cp_reg64(tcg_rt, cpu_env, tmpptr);
//            tcg_temp_free_ptr(tmpptr);
//        } else {
//            tcg_gen_ld_i64(tcg_rt, cpu_env, ri->fieldoffset);
//        }
//    } else {
//        if (ri->type & ARM_CP_CONST) {
//            /* If not forbidden by access permissions, treat as WI */
//            return;
//        } else if (ri->writefn) {
//            TCGv_ptr tmpptr;
//            tmpptr = tcg_const_ptr(ri);
//            gen_helper_set_cp_reg64(cpu_env, tmpptr, tcg_rt);
//            tcg_temp_free_ptr(tmpptr);
//        } else {
//            tcg_gen_st_i64(tcg_rt, cpu_env, ri->fieldoffset);
//        }
//    }

//    if ((s->base.tb->cflags & CF_USE_ICOUNT) && (ri->type & ARM_CP_IO)) {
//        /* I/O operations must end the TB here (whether read or write) */
//        gen_io_end();
//        s->base.is_jmp = DISAS_UPDATE;
//    } else if (!isread && !(ri->type & ARM_CP_SUPPRESS_TB_END)) {
//        /* We default to ending the TB on a coprocessor register write,
//         * but allow this to be suppressed by the register definition
//         * (usually only necessary to work around guest bugs).
//         */
//        s->base.is_jmp = DISAS_UPDATE;
//    }
}

/* HINT instruction group, including various allocated HINTs */
static void handle_hint(uint32_t insn,
                        unsigned int op1, unsigned int op2, unsigned int crm)
{
    unsigned int selector = crm << 3 | op2;

    if (op1 != 3) {
        //return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (selector) {
    case 0: /* NOP */
        return;
    case 3: /* WFI */
        //s->base.is_jmp = DISAS_WFI;
        return;
    case 1: /* YIELD */
//        if (!parallel_cpus) {
//            s->base.is_jmp = DISAS_YIELD;
//        }
        return;
    case 2: /* WFE */
//        if (!parallel_cpus) {
//            s->base.is_jmp = DISAS_WFE;
//        }
        return;
    case 4: /* SEV */
    case 5: /* SEVL */
        /* we treat all as NOP at least for now */
        return;
    default:
        /* default specified as NOP equivalent */
        return;
    }
}

/* CLREX, DSB, DMB, ISB */
static void handle_sync(uint32_t insn,
                        unsigned int op1, unsigned int op2, unsigned int crm)
{
//    TCGBar bar;

    if (op1 != 3) {
        //return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (op2) {
    case 2: /* CLREX */
//        gen_clrex(s);
        return;
    case 4: /* DSB */
    case 5: /* DMB */
        switch (crm & 3) {
        case 1: /* MBReqTypes_Reads */
            //bar = TCG_BAR_SC | TCG_MO_LD_LD | TCG_MO_LD_ST;
            break;
        case 2: /* MBReqTypes_Writes */
            //bar = TCG_BAR_SC | TCG_MO_ST_ST;
            break;
        default: /* MBReqTypes_All */
            //bar = TCG_BAR_SC | TCG_MO_ALL;
            break;
        }
        //tcg_gen_mb(bar);
        return;
    case 6: /* ISB */
        /* We need to break the TB after this insn to execute
         * a self-modified code correctly and also to take
         * any pending interrupts immediately.
         */
        //gen_goto_tb(s, 0, s->pc);
        return;
    default:
        //return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
}

/* MSR (immediate) - move immediate to processor state field */
static void handle_msr_i(uint32_t insn,
                         unsigned int op1, unsigned int op2, unsigned int crm)
{
    int op = op1 << 3 | op2;
    switch (op) {
    case 0x05: /* SPSel */
//        if (s->current_el == 0) {
//            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//            return;
//        }
        /* fall through */
    case 0x1e: /* DAIFSet */
    case 0x1f: /* DAIFClear */
    {
//        TCGv_i32 tcg_imm = tcg_const_i32(crm);
//        TCGv_i32 tcg_op = tcg_const_i32(op);
//        gen_a64_set_pc_im(s->pc - 4);
//        gen_helper_msr_i_pstate(cpu_env, tcg_op, tcg_imm);
//        tcg_temp_free_i32(tcg_imm);
//        tcg_temp_free_i32(tcg_op);
//        /* For DAIFClear, exit the cpu loop to re-evaluate pending IRQs.  */
//        gen_a64_set_pc_im(s->pc);
//        s->base.is_jmp = (op == 0x1f ? DISAS_EXIT : DISAS_JUMP);
        break;
    }
    default:
        //return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
}












////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
////////////////////////////////
///
///
enum a64_shift_type {
A64_SHIFT_TYPE_LSL = 0,
A64_SHIFT_TYPE_LSR = 1,
A64_SHIFT_TYPE_ASR = 2,
A64_SHIFT_TYPE_ROR = 3
};

//SemanticInstruction  * branch_cc( bool annul, bool useXcc, uint32_t cond, VirtualMemoryAddress target, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
//  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

//  DBG_Assert(cond <= 0xF);

//  inst->setClass(clsBranch, codeBranchConditional);

//  std::list<InternalDependance>  rs_deps;
//  dependant_action br = branchCCAction( inst, target, annul, packCondition(false, useXcc, cond), false) ;
//  connectDependance( inst->retirementDependance(), br );
//  rs_deps.push_back( br.dependance );

//  addReadCC(inst, 0 , 1, rs_deps) ;
//  //inst->addDispatchAction( br );

//  inst->addRetirementEffect( updateConditional(inst) );

//  return inst;
//}


//void branch_cc( SemanticInstruction * inst, VirtualMemoryAddress target, eCondCode aCode, std::vector< std::list<InternalDependance> > & rs_deps)
//{
//  inst->setClass(clsBranch, codeBranchConditional);

//  dependant_action br = branchCCAction( inst, target, false, condition(aCode), rs_deps, false) ;
//  connectDependance( inst->retirementDependance(), br );
//  rs_deps.push_back( br.dependance );

//  inst->addDispatchAction( br );
//  inst->addRetirementEffect( updateConditional(inst) );
//}

void branch_always( SemanticInstruction * inst, bool annul, VirtualMemoryAddress target)
{
    DBG_(Tmp, (<<"DECODER: In branch_always"));

    inst->setClass(clsBranch, codeBranchUnconditional);

    if (annul) {
      inst->addDispatchEffect( branch( inst, target ) );
    } else {
      inst->addDispatchEffect( branchAfterNext( inst, target ) );
    }
    inst->addRetirementEffect( updateUnconditional( inst, target ) );
}

void  MEMBAR( SemanticInstruction * inst, armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, uint32_t i ) {

  if (i) {
    //MEMBAR #StoreStore

    inst->setClass(clsMEMBAR, codeMEMBARStSt);
    inst->addRetirementConstraint( membarStoreStoreConstraint(inst) );

        return;
  } else if ( (aFetchedOpcode.theOpcode & 0x70) != 0 ) {
    //Synchronizing MEMBARS.
    //Although this is not entirely to spec, we implement synchronizing MEMBARs
    //as StoreLoad MEMBERS.

    inst->setClass(clsMEMBAR, codeMEMBARSync);

    //#Sync implementation - waits for core to drain/synchronize
    //inst->setHaltDispatch();
    //inst->addRetirementConstraint( storeQueueEmptyConstraint(inst) );

    //ordering implementation - allows TSO++ speculation
    inst->addRetirementConstraint( membarSyncConstraint(inst) );
    inst->addDispatchEffect( allocateMEMBAR( inst ) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( eraseLSQ(inst) ); //The LSQ entry may already be gone if the MEMBAR retired non-speculatively (empty SB)
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

    return inst;

  } else if ( (aFetchedOpcode.theOpcode & 2) != 0 ) {
    //MEMBAR #StoreLoad

    //The only ordering MEMBAR that matters is the #StoreLoad, as all the other
    //constraints are implicit in TSO.  The #StoreLoad MEMBAR may not retire
    //until the store buffer is empty.

    inst->setClass(clsMEMBAR, codeMEMBARStLd);
    inst->addRetirementConstraint( membarStoreLoadConstraint(inst) );
    inst->addDispatchEffect( allocateMEMBAR( inst ) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( eraseLSQ(inst) ); //The LSQ entry may already be gone if the MEMBAR retired non-speculatively (empty SB)
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

    return;

  } else if ( (aFetchedOpcode.theOpcode & 8) != 0 ) {
    //MEMBAR #StoreStore

    inst->setClass(clsMEMBAR, codeMEMBARStSt);
    inst->addRetirementConstraint( membarStoreStoreConstraint(inst) );

    return;

  } else {
    //All other MEMBARs are NOPs under TSO
//    return nop( aFetchedOpcode, aCPU, aSequenceNo );
  }
}

void ldrex( SemanticInstruction * inst, uint32_t rs, uint32_t rs2,  uint32_t dest) {

  inst->setClass(clsAtomic, codeLDREX);

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, rs_deps ) ;

  addReadXRegister(inst, 1, rs, rs_deps[0]);
  addReadXRegister(inst, 1, rs2, rs_deps[1]);

  predicated_dependant_action update_value = updateRMWValueAction( inst );
  inst->addDispatchAction( update_value );

  inst->setOperand( kOperand4, static_cast<uint64_t>(0xFF) );
  inst->addDispatchEffect( satisfy( inst, update_value.dependance ) );

  //obtain the loaded value and write it back to the reg file.
  predicated_dependant_action rmw;
  rmw = rmwAction( inst, kByte, kPD );
  inst->addDispatchEffect( allocateRMW( inst, kByte, rmw.dependance) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core
  inst->addSquashEffect( eraseLSQ(inst) );
  addDestination( inst, dest, rmw);
}
void strex(SemanticInstruction * inst, int srcReg, int addrReg, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
    inst->setClass(clsAtomic, codeSTREX);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    // get memory address
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, kAccType_ORDERED) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadRD( inst, srcReg );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
    inst->addCommitEffect( commitStore(inst) );
}

//void STR( SemanticInstruction * inst, uint64_t source,
//            /*uint64_t addr, */int size, int memidx,
//            bool iss_valid,
//            unsigned int iss_srt,
//            bool iss_sf, bool iss_ar)

void STR(SemanticInstruction * inst, int srcReg, int addrReg, int size, eAccType type)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
    inst->setClass(clsStore, codeStore);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    // get memory address
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, type ));
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadRD( inst, srcReg );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
    inst->addCommitEffect( commitStore(inst) );
}

void ldgpr(SemanticInstruction * inst, int destReg, int addrReg, int size, bool sign_extend)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Load General Reg \033[0m"));

    inst->setClass(clsLoad, codeLoad);

    std::vector< std::list<InternalDependance> > rs_deps(2);

    // compute address from register
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);
//    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, eSize(1<<size), sign_extend, kPD );

    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_NORMAL ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, destReg, load);
}

void ldfpr(SemanticInstruction * inst, int addrReg, int destReg, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Load Floating Reg \033[0m"));

    inst->setClass(clsLoad, codeLoadFP);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );

    predicated_dependant_action load;
    load = loadFloatingAction( inst, eSize(1<<size), kPFD0, kPFD1);

    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addFloatingDestination( inst, destReg, eSize(1<<size), load);
}



void stfpr(SemanticInstruction * inst, int addrReg, int dest, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store Floating Reg \033[0m"));


    inst->setClass(clsStore, codeStoreFP);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addAnnulmentEffect(  forceResync(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), true, kAccType_ORDERED ) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadFValue( inst, dest, eSize(1<<size) );
}

/* Update the Sixty-Four bit (SF) registersize. This logic is derived
 * from the ARMv8 specs for LDR (Shared decode for all encodings).
 */
static bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc)
{
    int opc0 = extract32(opc, 0, 1);
    int regsize;

    if (is_signed) {
        regsize = opc0 ? 32 : 64;
    } else {
        regsize = size == 3 ? 64 : 32;
    }
    return regsize == 64;
}

arminst unallocated_encoding(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: unallocated_encoding \033[0m"));
    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
    /* Unallocated and reserved encodings are uncategorized */
}

arminst unsupported_encoding(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: unsupported_encoding \033[0m"));

    /* Unallocated and reserved encodings are uncategorized */
    assert(false);
    //throw Exception;
}

void ADR(SemanticInstruction* inst, uint64_t base, uint64_t offset, uint64_t rd)
{
    std::vector<std::list<InternalDependance> > rs_deps;
    inst->setOperand(kOperand1, base);
    inst->setOperand(kOperand2, offset);
    predicated_action exec = addExecute(inst, operation(kADD_), rs_deps);
    addDestination(inst, rd, exec);
}

/* PC-rel. addressing
 *   31  30   29 28       24 23                5 4    0
 * +----+-------+-----------+-------------------+------+
 * | op | immlo | 1 0 0 0 0 |       immhi       |  Rd  |
 * +----+-------+-----------+-------------------+------+
 */
arminst disas_pc_rel_adr(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));

    unsigned int  rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint64_t offset = sextract64(aFetchedOpcode.theOpcode, 5, 19);
    offset = offset << 2 | extract32(aFetchedOpcode.theOpcode, 29, 2);
    unsigned int op = extract32(aFetchedOpcode.theOpcode, 31, 1);
    uint64_t base = aFetchedOpcode.thePC;


    if (op) {
        /* ADRP (page based) */
        base &= ~0xfff;
        offset <<= 12;
    }

    ADR(inst, base, offset, rd);

    return inst;
}


void EXTR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t rm  , uint64_t imm, bool sf)
{
    std::vector<std::list<InternalDependance> > rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    addReadXRegister(inst, 2, rm, rs_deps[1], sf ? true : false);
    inst->setOperand(kOperand3, imm);
    predicated_action exec = addExecute(inst, operation(sf ? kCONCAT64_ : kCONCAT32_),rs_deps);
    addDestination(inst, rd, exec);
}

void SBFM(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint32_t imms, uint64_t immr, bool sf)
{

//    if (sf){
//        uint64_t *wmask, *tmask;
//        if (!logic_imm_decode_wmask_tmask(&wmask, &tmask, n, imms, immr)) {
//            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        }
//    }

//    bits(datasize) src = X[n];
//    // perform bitfield move on low bits
//    bits(datasize) bot = ROR(src, R) AND wmask;
//    // determine extension bits (sign, zero or dest register)
//    bits(datasize) top = Replicate(src<S>);
//    // combine extension bits and result bits
//    X[d] = (top AND NOT(tmask)) OR (bot AND tmask);

//    std::vector<std::list<InternalDependance> > rs_deps(2);
//    addReadXRegister(inst, 1, rn, rs_deps[0], sf);
//    inst->setOperand(kOperand2, immr);
//    predicated_action e1 = addExecute(inst, operation(kROR_),rs_deps);

//    Operation * op = operation(kAND_);
//    op->setOperands(kOperand1, wmask);



//    predicated_action e2 = addExecute(inst, operation(kAND_),rs_deps);
//    uint64_t top = src & (1 << imms);

//    (top & ~tmask) | (bot & tmask);


}

void MOVK(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kMOVK_),rs_deps);
    addDestination(inst, rd, exec);
}

void MOV(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool is_not)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, is_not ? operation(kMOVN_) : operation(kMOV_),rs_deps);
    addDestination(inst, rd, exec);
}
void XOR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kXOR_) ,rs_deps);
    addDestination(inst, rd, exec);
}
void ORR(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, operation(kORR_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void AND(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    predicated_action exec = addExecute(inst, S ? operation(kANDS_) : operation(kAND_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void ADD(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    if (S) { // ADDS
        inst->setOperand(kOperand3, 0); // carry in bit
    }
    predicated_action exec = addExecute(inst, S ? operation(kADDS_) : operation(kADD_) ,rs_deps);
    addDestination(inst, rd, exec);
}

void SUB(SemanticInstruction* inst, uint32_t rd, uint32_t rn, uint64_t imm, bool sf, bool S)
{
    std::vector<std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst, 1, rn, rs_deps[0], sf ? true : false);
    inst->setOperand(kOperand2, imm);
    if (S) { // SUBS
        inst->setOperand(kOperand3, 1); // carry in bit
    }
    predicated_action exec = addExecute(inst, S ? operation(kSUBS_) : operation(kSUB_) ,rs_deps);
    addDestination(inst, rd, exec);
}

/*
 * Add/subtract (immediate)
 *
 *  31 30 29 28       24 23 22 21         10 9   5 4   0
 * +--+--+--+-----------+-----+-------------+-----+-----+
 * |sf|op| S| 1 0 0 0 1 |shift|    imm12    |  Rn | Rd  |
 * +--+--+--+-----------+-----+-------------+-----+-----+
 *
 *    sf: 0 -> 32bit, 1 -> 64bit
 *    op: 0 -> add  , 1 -> sub
 *     S: 1 -> set flags
 * shift: 00 -> LSL imm by 0, 01 -> LSL imm by 12
 */
arminst disas_add_sub_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint64_t imm = extract32(aFetchedOpcode.theOpcode, 10, 12);
    int shift = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool S = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);


    switch (shift) {
    case 0x0:
        break;
    case 0x1:
        imm <<= 12;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }


    if (op) // sub
    {
        SUB(inst, rd, rn, imm, sf, S);
    }
    else { // add
        ADD(inst, rd, rn, imm, sf, S);

    }

    return inst;
}

/* Return a value with the bottom len bits set (where 0 < len <= 64) */
static inline uint64_t bitmask64(unsigned int length)
{
    assert(length > 0 && length <= 64);
    return ~0ULL >> (64 - length);
}

/* The input should be a value in the bottom e bits (with higher
 * bits zero); returns that value replicated into every element
 * of size e in a 64 bit integer.
 */
static uint64_t bitfield_replicate(uint64_t mask, unsigned int e)
{
    assert(e != 0);
    while (e < 64) {
        mask |= mask << e;
        e *= 2;
    }
    return mask;
}

/* Simplified variant of pseudocode DecodeBitMasks() for the case where we
 * only require the wmask. Returns false if the imms/immr/immn are a reserved
 * value (ie should cause a guest UNDEF exception), and true if they are
 * valid, in which case the decoded bit pattern is written to result.
 */
static bool logic_imm_decode_wmask_tmask(uint64_t *wmask, uint64_t *tmask, unsigned int immn,
                                   unsigned int imms, unsigned int immr)
{
    uint64_t mask, mask2;
    unsigned e, levels, s, r, diff, d;
    int len;

    assert(immn < 2 && imms < 64 && immr < 64);

    /* The bit patterns we create here are 64 bit patterns which
     * are vectors of identical elements of size e = 2, 4, 8, 16, 32 or
     * 64 bits each. Each element contains the same value: a run
     * of between 1 and e-1 non-zero bits, rotated within the
     * element by between 0 and e-1 bits.
     *
     * The element size and run length are encoded into immn (1 bit)
     * and imms (6 bits) as follows:
     * 64 bit elements: immn = 1, imms = <length of run - 1>
     * 32 bit elements: immn = 0, imms = 0 : <length of run - 1>
     * 16 bit elements: immn = 0, imms = 10 : <length of run - 1>
     *  8 bit elements: immn = 0, imms = 110 : <length of run - 1>
     *  4 bit elements: immn = 0, imms = 1110 : <length of run - 1>
     *  2 bit elements: immn = 0, imms = 11110 : <length of run - 1>
     * Notice that immn = 0, imms = 11111x is the only combination
     * not covered by one of the above options; this is reserved.
     * Further, <length of run - 1> all-ones is a reserved pattern.
     *
     * In all cases the rotation is by immr % e (and immr is 6 bits).
     */

    /* First determine the element size */
    len = 31 - clz32((immn << 6) | (~imms & 0x3f));
    if (len < 1) {
        /* This is the immn == 0, imms == 0x11111x case */
        return false;
    }
    e = 1 << len;

    levels = e - 1;
    s = imms & levels;
    r = immr & levels;
    diff = s - r;
    d = diff & (len-1 >> ~0);

    if (s == levels) {
        /* <length of run - 1> mustn't be all-ones. */
        return false;
    }

    /* Create the value of one element: s+1 set bits rotated
     * by r within the element (which is e bits wide)...
     */

    mask = bitmask64(s + 1);
    *tmask = bitmask64(d + 1);
    if (r) {
        mask = (mask >> r) | (mask << (e - r));
        mask &= bitmask64(e);
    }
    /* ...then replicate the element over the whole 64 bit value */
    mask = bitfield_replicate(mask, e);
    *wmask = mask;
    return true;
}

static bool logic_imm_decode_wmask(uint64_t *wmask, unsigned int immn,
                                   unsigned int imms, unsigned int immr)
{
    uint64_t mask;
    unsigned e, levels, s, r;
    int len;

    assert(immn < 2 && imms < 64 && immr < 64);

    /* The bit patterns we create here are 64 bit patterns which
     * are vectors of identical elements of size e = 2, 4, 8, 16, 32 or
     * 64 bits each. Each element contains the same value: a run
     * of between 1 and e-1 non-zero bits, rotated within the
     * element by between 0 and e-1 bits.
     *
     * The element size and run length are encoded into immn (1 bit)
     * and imms (6 bits) as follows:
     * 64 bit elements: immn = 1, imms = <length of run - 1>
     * 32 bit elements: immn = 0, imms = 0 : <length of run - 1>
     * 16 bit elements: immn = 0, imms = 10 : <length of run - 1>
     *  8 bit elements: immn = 0, imms = 110 : <length of run - 1>
     *  4 bit elements: immn = 0, imms = 1110 : <length of run - 1>
     *  2 bit elements: immn = 0, imms = 11110 : <length of run - 1>
     * Notice that immn = 0, imms = 11111x is the only combination
     * not covered by one of the above options; this is reserved.
     * Further, <length of run - 1> all-ones is a reserved pattern.
     *
     * In all cases the rotation is by immr % e (and immr is 6 bits).
     */

    /* First determine the element size */
    len = 31 - clz32((immn << 6) | (~imms & 0x3f));
    if (len < 1) {
        /* This is the immn == 0, imms == 0x11111x case */
        return false;
    }
    e = 1 << len;

    levels = e - 1;
    s = imms & levels;
    r = immr & levels;

    if (s == levels) {
        /* <length of run - 1> mustn't be all-ones. */
        return false;
    }

    /* Create the value of one element: s+1 set bits rotated
     * by r within the element (which is e bits wide)...
     */

    mask = bitmask64(s + 1);
    if (r) {
        mask = (mask >> r) | (mask << (e - r));
        mask &= bitmask64(e);
    }
    /* ...then replicate the element over the whole 64 bit value */
    mask = bitfield_replicate(mask, e);
    *wmask = mask;
    return true;
}


/* Logical (immediate)
 *   31  30 29 28         23 22  21  16 15  10 9    5 4    0
 * +----+-----+-------------+---+------+------+------+------+
 * | sf | opc | 1 0 0 1 0 0 | N | immr | imms |  Rn  |  Rd  |
 * +----+-----+-------------+---+------+------+------+------+
 */
arminst disas_logic_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int sf, opc, is_n, immr, imms, rn, rd;
    uint64_t wmask;
    bool is_and = false;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    is_n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    immr = extract32(aFetchedOpcode.theOpcode, 16, 6);
    imms = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (!logic_imm_decode_wmask(&wmask, is_n, imms, immr)) {
        /* some immediate field values are reserved */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (!sf) {
        wmask &= 0xffffffff;
    }



    switch (opc) {
    case 0x3: /* ANDS */
        AND(inst, rd, rn, wmask, sf, true);
    case 0x0: /* AND */
        //    if sf == '0' && N != '0' then ReservedValue();
        //    ReservedValue()
        //    if UsingAArch32() && !AArch32.GeneralExceptionsToAArch64() then
        //    AArch32.TakeUndefInstrException();
        //    else
        //    AArch64.UndefinedFault();
            if (sf == 0 && is_n != 0 ){
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }

         AND(inst, rd, rn, wmask, sf, false);
         break;
    case 0x1: /* ORR */
        ORR(inst, rd, rn, wmask, sf, false);
        break;
    case 0x2: /* EOR */
        XOR(inst, rd, rn, wmask, sf, false);
        break;
    default:
        DBG_Assert(false); /* must handle all above */
        break;
    }

    return inst;
}

/*
 * Move wide (immediate)
 *
 *  31 30 29 28         23 22 21 20             5 4    0
 * +--+-----+-------------+-----+----------------+------+
     * |sf| opc | 1 0 0 1 0 1 |  hw |  imm16         |  Rd  |
 * +--+-----+-------------+-----+----------------+------+
 *
 * sf: 0 -> 32 bit, 1 -> 64 bit
 * opc: 00 -> N, 10 -> Z, 11 -> K
 * hw: shift/16 (0,16, and sf only 32, 48)
 */
arminst disas_movw_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint64_t imm = extract32(aFetchedOpcode.theOpcode, 5, 16);
    int sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    int opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    int pos = extract32(aFetchedOpcode.theOpcode, 21, 2) << 4;

    if (!sf && (pos >= 32)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (opc) {
    case 0: /* MOVN */
    case 2: /* MOVZ */
        imm <<= pos;
        if (opc == 0) { // MOVN
            imm = ~imm;
        }
        if (!sf) {
            imm &= 0xffffffffu;
        }
//        MOV(inst, rd, rn, imm, sf, (opc == 0) ? false : true);
        break;
    case 3: /* MOVK */
        MOVK(inst, rd, rd, imm ,sf);
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }

    return inst;
}

/* Bitfield
 *   31  30 29 28         23 22  21  16 15  10 9    5 4    0
 * +----+-----+-------------+---+------+------+------+------+
 * | sf | opc | 1 0 0 1 1 0 | N | immr | imms |  Rn  |  Rd  |
 * +----+-----+-------------+---+------+------+------+------+
 */
arminst disas_bitfield(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int sf, n, opc, ri, si, rn, rd, bitsize, pos, len;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    ri = extract32(aFetchedOpcode.theOpcode, 16, 6); // immr
    si = extract32(aFetchedOpcode.theOpcode, 10, 6); // imms
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    bitsize = sf ? 64 : 32;

    if (!sf && n != 1){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // ReservedValue()
    }
    if (!sf && n != 0 || ri != 0 || si != 0){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); // ReservedValue()
    }


    switch (opc) {
    case 0:
        SBFM(inst, rd, rd, si, ri, sf);
        break;
    case 1:
        break;
    case 2:
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

}

/* Extract
 *   31  30  29 28         23 22   21  20  16 15    10 9    5 4    0
 * +----+------+-------------+---+----+------+--------+------+------+
 * | sf | op21 | 1 0 0 1 1 1 | N | o0 |  Rm  |  imms  |  Rn  |  Rd  |
 * +----+------+-------------+---+----+------+--------+------+------+
 */
arminst disas_extract(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState, aCPU,aSequenceNo));

    unsigned int sf, n, rm, imm, rn, rd, bitsize, op21, op0;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    n = extract32(aFetchedOpcode.theOpcode, 22, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    imm = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    op21 = extract32(aFetchedOpcode.theOpcode, 29, 2);
    op0 = extract32(aFetchedOpcode.theOpcode, 21, 1);
    bitsize = sf ? 64 : 32;

    if (sf != n || op21 || op0 || imm >= bitsize) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        EXTR(inst, rd, rn, rm, imm, sf);
    }

    return inst;
}

/* Data processing - immediate */
arminst disas_data_proc_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 23, 6)) {
    case 0x20: case 0x21: /* PC-rel. addressing */
        DBG_(Tmp,(<< "\033[1;31mDECODER: PC-rel. addressing \033[0m"));
        return disas_pc_rel_adr(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x22: case 0x23: /* Add/subtract (immediate) */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Add/subtract (immediate) \033[0m"));
        return disas_add_sub_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x24: /* Logical (immediate) */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Logical (immediate) \033[0m"));
        return disas_logic_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x25: /* Move wide (immediate) */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Move wide (immediate) \033[0m"));
        return disas_movw_imm(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x26: /* Bitfield */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Bitfield \033[0m"));
        return disas_bitfield(aFetchedOpcode,  aCPU, aSequenceNo);
    case 0x27: /* Extract */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Extract \033[0m"));
        return disas_extract(aFetchedOpcode,  aCPU, aSequenceNo);
    default:
        DBG_(Tmp,(<< "\033[1;31mDECODER: Extract \033[0m"));
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    DBG_Assert(false);
}

/* Unconditional branch (register)
 *  31           25 24   21 20   16 15   10 9    5 4     0
 * +---------------+-------+-------+-------+------+-------+
 * | 1 1 0 1 0 1 1 |  opc  |  op2  |  op3  |  Rn  |  op4  |
 * +---------------+-------+-------+-------+------+-------+
 */
arminst disas_uncond_b_reg( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC,
                                                        aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState,
                                                        aCPU,
                                                        aSequenceNo) );

    unsigned int opc, op2, op3, rn, op4;
    std::vector<std::list<InternalDependance> > rs_deps(1);


    opc = extract32(aFetchedOpcode.theOpcode, 21, 4);
    op2 = extract32(aFetchedOpcode.theOpcode, 16, 5);
    op3 = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    op4 = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (op4 != 0x0 || op3 != 0x0 || op2 != 0x1f) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    switch (opc) {
    case 0: /* BR */
    case 1: /* BLR */
    case 2: /* RET */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register) : BR?BLR?RET \033[0m"));
        DBG_(Tmp,(<< "\033[1;31m DECODER: Will set the PC using the value in X30 \033[0m"));

        addReadXRegister(inst, 1, rn, rs_deps[0]);
        simple_action target = calcAddressAction( inst, rs_deps);
        dependant_action br = branchToCalcAddressAction( inst );
        connectDependance( br.dependance, target );
        connectDependance( inst->retirementDependance(), br );
        inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

        if (opc == 1) { // BLR

            inst->setOperand(kOperand2, 4);
            predicated_action act = addExecute(inst, operation(kADD_), rs_deps);
            addDestination(inst, 30, act);
        }

        break;
    case 4: /* ERET */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register) : ERET \033[0m"));

//        if (s->current_el == 0) {
//            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//            return;
//        }
//        gen_helper_exception_return(cpu_env);
        /* Must exit loop to check un-masked IRQs */
//        s->base.is_jmp = DISAS_EXIT;
        return;
    case 5: /* DRPS */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register) : DRPS \033[0m"));

//        System
//        DRPS
//        if !Halted() || PSTATE.EL == EL0 then UnallocatedEncoding();
//        Operation
//        DRPSInstruction();

        if (rn != 0x1f) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            return unsupported_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        return;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    return inst;
}

/* Exception generation
 *
 *  31             24 23 21 20                     5 4   2 1  0
 * +-----------------+-----+------------------------+-----+----+
 * | 1 1 0 1 0 1 0 0 | opc |          imm16         | op2 | LL |
 * +-----------------------+------------------------+----------+
 */
arminst disas_exc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);

    int opc = extract32(aFetchedOpcode.theOpcode, 21, 3);
    int op2_ll = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int imm16 = extract32(aFetchedOpcode.theOpcode, 5, 16);
//    TCGv_i32 tmp;

    switch (opc) {
    case 0:
        /* For SVC, HVC and SMC we advance the single-step state
         * machine before taking the exception. This is architecturally
         * mandated, to ensure that single-stepping a system call
         * instruction works properly.
         */
        switch (op2_ll) {
        case 1:                                                     /* SVC */
//            gen_ss_advance(s);
//            gen_exception_insn(s, 0, EXCP_SWI, syn_aa64_svc(imm16),
//                               default_exception_el(s));
            break;
        case 2:                                                     /* HVC */
//            if (s->current_el == 0) {
//                unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//                break;
//            }
            /* The pre HVC helper handles cases when HVC gets trapped
             * as an undefined insn by runtime configuration.
             */
//            gen_a64_set_pc_im(s->pc - 4);
//            gen_helper_pre_hvc(cpu_env);
//            gen_ss_advance(s);
//            gen_exception_insn(s, 0, EXCP_HVC, syn_aa64_hvc(imm16), 2);
            break;
        case 3:                                                     /* SMC */
//            if (s->current_el == 0) {
//                unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//                break;
//            }
//            gen_a64_set_pc_im(s->pc - 4);
//            tmp = tcg_const_i32(syn_aa64_smc(imm16));
//            gen_helper_pre_smc(cpu_env, tmp);
//            tcg_temp_free_i32(tmp);
//            gen_ss_advance(s);
//            gen_exception_insn(s, 0, EXCP_SMC, syn_aa64_smc(imm16), 3);
            break;
        default:
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        break;
    case 1:
        if (op2_ll != 0) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        /* BRK */
//        gen_exception_insn(s, 4, EXCP_BKPT, syn_aa64_bkpt(imm16),
//                           default_exception_el(s));
        break;
    case 2:
        if (op2_ll != 0) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        /* HLT. This has two purposes.
         * Architecturally, it is an external halting debug instruction.
         * Since QEMU doesn't implement external debug, we treat this as
         * it is required for halting debug disabled: it will UNDEF.
         * Secondly, "HLT 0xf000" is the A64 semihosting syscall instruction.
         */
//        if (semihosting_enabled() && imm16 == 0xf000) {
//#ifndef CONFIG_USER_ONLY
//            /* In system mode, don't allow userspace access to semihosting,
//             * to provide some semblance of security (and for consistency
//             * with our 32-bit semihosting).
//             */
//            if (s->current_el == 0) {
//                unsupported_encoding(aFetchedOpcode,  aCPU, aSequenceNo);
//                break;
//            }
//#endif
//            gen_exception_internal_insn(s, 0, EXCP_SEMIHOST);
//        } else {
//            unsupported_encoding(aFetchedOpcode,  aCPU, aSequenceNo);
//        }
        break;
    case 5:
        if (op2_ll < 1 || op2_ll > 3) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        /* DCPS1, DCPS2, DCPS3 */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* System
 *  31                 22 21  20 19 18 16 15   12 11    8 7   5 4    0
 * +---------------------+---+-----+-----+-------+-------+-----+------+
 * | 1 1 0 1 0 1 0 1 0 0 | L | op0 | op1 |  CRn  |  CRm  | op2 |  Rt  |
 * +---------------------+---+-----+-----+-------+-------+-----+------+
 */
arminst disas_system(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
//    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC,
//                                                        aFetchedOpcode.theOpcode,
//                                                        aFetchedOpcode.theBPState,
//                                                        aCPU,
//                                                        aSequenceNo) );

    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);

    unsigned int l, op0, op1, crn, crm, op2, rt;
    l = extract32(aFetchedOpcode.theOpcode, 21, 1);
    op0 = extract32(aFetchedOpcode.theOpcode, 19, 2);
    op1 = extract32(aFetchedOpcode.theOpcode, 16, 3);
    crn = extract32(aFetchedOpcode.theOpcode, 12, 4);
    crm = extract32(aFetchedOpcode.theOpcode, 8, 4);
    op2 = extract32(aFetchedOpcode.theOpcode, 5, 3);
    rt = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (op0 == 0) {
        if (l || rt != 31) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            DBG_(Tmp,(<< "\033[1;31m DECODER: unallocated_encoding \033[0m"));

            return;
        }
        switch (crn) {
        case 2: /* HINT (including allocated hints like NOP, YIELD, etc) */
            DBG_(Tmp,(<< "\033[1;31m DECODER: NOP, YIELD, etc \033[0m"));

            handle_hint(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        case 3: /* CLREX, DSB, DMB, ISB */
            DBG_(Tmp,(<< "\033[1;31m DECODER: CLREX, DSB, DMB, ISB \033[0m"));

            handle_sync(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        case 4: /* MSR (immediate) */
            DBG_(Tmp,(<< "\033[1;31m DECODER: MSR (immediate)  \033[0m"));

            handle_msr_i(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        default:
            DBG_(Tmp,(<< "\033[1;31m DECODER: unallocated_encoding  \033[0m"));

            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        //return inst;
    }
    handle_sys(aFetchedOpcode.theOpcode,  l, op0, op1, op2, crn, crm, rt);
    DBG_(Tmp,(<< "\033[1;31m DECODER: handle_sys  \033[0m"));

    //return inst;
}

/* Conditional branch (immediate)
 *  31           25  24  23                  5   4  3    0
 * +---------------+----+---------------------+----+------+
 * | 0 1 0 1 0 1 0 | o1 |         imm19       | o0 | cond |
 * +---------------+----+---------------------+----+------+
 */
arminst disas_cond_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int cond;
    uint64_t addr;

    std::vector<std::list<InternalDependance> > rs_deps(1);

    if ((aFetchedOpcode.theOpcode & (1 << 4)) || (aFetchedOpcode.theOpcode & (1 << 24))) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4;
    cond = extract32(aFetchedOpcode.theOpcode, 0, 4);

    VirtualMemoryAddress target(addr);
    inst->setOperand(kOperand1, cond);


    if (cond < 0x0e) {
        /* genuinely conditional branches */
//        branch_cc(inst, target, kBCOND_, rs_deps);

    } else {
        /* 0xe and 0xf are both "always" conditions */
        branch_always(inst, false, target);
    }
}


/* Test and branch (immediate)
 *   31  30         25  24  23   19 18          5 4    0
 * +----+-------------+----+-------+-------------+------+
 * | b5 | 0 1 1 0 1 1 | op |  b40  |    imm14    |  Rt  |
 * +----+-------------+----+-------+-------------+------+
 */
arminst disas_test_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int bit_pos, op, rt;
    uint64_t addr;
    std::vector<std::list<InternalDependance> > rs_deps(2);


    bit_pos = (extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) | extract32(aFetchedOpcode.theOpcode, 19, 5);
    op = extract32(aFetchedOpcode.theOpcode, 24, 1); /* 0: TBZ; 1: TBNZ */
    addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 14) * 4 - 4;
    rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    VirtualMemoryAddress target(addr);

    addReadXRegister(inst, 1, rt, rs_deps[0]);
    inst->setOperand(kOperand2, (1ULL<<bit_pos));

    if (op == 0) {  // TBZ
//        branch_cc(inst,target, kTBZ_, rs_deps);
    } else {
//        branch_cc(inst,target, kTBNZ_, rs_deps);
    }

}

//static brcondi_i64(bool cond, TCGv_i64 arg1, int64_t arg2, TCGLabel *l)
//{
//    if (cond == COND_ALWAYS) {
//        tcg_gen_br(l);
//    } else if (cond != COND_NEVER) {
//        tcg_gen_brcond_i64(cond, arg1, t0, l);
//    }
//}

/* Compare and branch (immediate)
 *   31  30         25  24  23                  5 4      0
 * +----+-------------+----+---------------------+--------+
 * | sf | 0 1 1 0 1 0 | op |         imm19       |   Rt   |
 * +----+-------------+----+---------------------+--------+
 */
arminst disas_comp_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    std::vector<std::list<InternalDependance> > rs_deps(1);
    unsigned int sf, op, rt;
    uint64_t addr;


    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    op = extract32(aFetchedOpcode.theOpcode, 24, 1); /* 0: CBZ; 1: CBNZ */
    rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    addr = aFetchedOpcode.thePC + (sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4);
    VirtualMemoryAddress target(addr);

    addReadXRegister(inst, 1, rt, rs_deps[0], sf);

    if (op == 0) // CBZ
    {
//        branch_cc(inst, target, kCBZ_);
    }
    else
    {
//        branch_cc(inst, target, kCBNZ_);
    }


    return inst;
}

/*
 * The instruction disassembly implemented here matches
 * the instruction encoding classifications in chapter C4
 * of the ARM Architecture Reference Manual (DDI0487B_a);
 * classification names and decode diagrams here should generally
 * match up with those in the manual.
 */

/* Unconditional branch (immediate)
 *   31  30       26 25                                  0
 * +----+-----------+-------------------------------------+
 * | op | 0 0 1 0 1 |                 imm26               |
 * +----+-----------+-------------------------------------+
 */
arminst disas_uncond_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 0, 26) * 4 - 4;
    VirtualMemoryAddress target(addr);

    if (aFetchedOpcode.theOpcode & (1U << 31)) {  // BL
        /* Branch with Link branches to a PC-relative offset,
         * setting the register X30 to PC+4.
         * It provides a hint that this is a subroutine call.*/

        std::vector< std::list<InternalDependance> > rs_deps(2);
        inst->setOperand(kOperand1, aFetchedOpcode.thePC);
        inst->setOperand(kOperand2, 4);
        predicated_action exec = addExecute(inst, operation(kADD_),rs_deps);

        addDestination(inst, 30, exec);
        inst->addDispatchEffect( branch( inst, target ) );
    } else { // B
        inst->addDispatchEffect( branch( inst, target ) );
    }

    return inst;
}

/* Branches, exception generating and system instructions */
arminst disas_b_exc_sys(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Branches, exception generating and system instructions \033[0m"));
    switch (extract32(aFetchedOpcode.theOpcode, 25, 7)) {
    case 0x0a: case 0x0b:
    case 0x4a: case 0x4b: /* Unconditional branch (immediate) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (immediate)  \033[0m"));
        return disas_uncond_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x1a: case 0x5a: /* Compare & branch (immediate) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Compare & branch (immediate)  \033[0m"));
        return disas_comp_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x1b: case 0x5b: /* Test & branch (immediate) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Test & branch (immediate) \033[0m"));
        return disas_test_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2a: /* Conditional branch (immediate) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Conditional branch (immediate)  \033[0m"));
        return disas_cond_b_imm(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x6a: /* Exception generation / System */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Exception generation / System \033[0m"));
        if (aFetchedOpcode.theOpcode & (1 << 24)) {
            DBG_(Tmp,(<< "\033[1;31m DECODER: System \033[0m"));
            return disas_system(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Exception \033[0m"));
            return disas_exc(aFetchedOpcode, aCPU, aSequenceNo);
        }
        break;
    case 0x6b: /* Unconditional branch (register) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register)  \033[0m"));
        return disas_uncond_b_reg(aFetchedOpcode, aCPU, aSequenceNo);
    default:
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch: unallocated_encoding \033[0m"));
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    assert(false);
}

/* Load/store exclusive
 * 31 30             24 23 22 21 20    16 15       10     5    0                    0
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 * |size| 0 0 1 0 0 0 | o2|L |o1|  Rs    |o0|  Rt2   |  Rn | Rt |
 * +----+-------------+---+--+--+--------+--+--------+-----+----+
 */

arminst disas_ldst_excl(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    int is_lasr = extract32(aFetchedOpcode.theOpcode, 15, 1); // o0
    int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int is_pair = extract32(aFetchedOpcode.theOpcode, 21, 1);  // o1
    int is_store = !extract32(aFetchedOpcode.theOpcode, 22, 1); // L
    int is_excl = !extract32(aFetchedOpcode.theOpcode, 23, 1); // o2
    int size = extract32(aFetchedOpcode.theOpcode, 30, 2);

    if ((!is_excl && !is_pair && !is_lasr) ||
        (!is_excl && is_pair) ||
        (is_pair && size < 2)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);

    }

//    if (is_excl) {  // o2
//        if (!is_store) {  // ! L
//            if (!is_pair) {  // o1
//                 STR(inst, rt, rn, (1<<size), is_lasr ? kAccType_ORDERED : kAccType_LIMITEDORDERED); // STLRB , STLLRB
//            } else {
//       c          CAS(inst, rt, rn, rs, (1<<size), is_lasr ? kAccType_ORDEREDRW : kAccType_ATOMICRW, kAccType_ATOMICRW); // STLRB , STLLRB
//              }
//        } else {
//            bool iss_sf = disas_ldst_compute_iss_sf(size, false, 0);
//            /* Generate ISS for non-exclusive accesses including LASR.  */
//        } else {
//            }
//        }
//    }

    return inst;
}

/*
 * Load register (literal)
 *
 *  31 30 29   27  26 25 24 23                5 4     0
 * +-----+-------+---+-----+-------------------+-------+
 * | opc | 0 1 1 | V | 0 0 |     imm19         |  Rt   |
 * +-----+-------+---+-----+-------------------+-------+
 *
 * V: 1 -> vector (simd/fp)
 * opc (non-vector): 00 -> 32 bit, 01 -> 64 bit,
 *                   10-> 32 bit signed, 11 -> prefetch
 * opc (vector): 00 -> 32 bit, 01 -> 64 bit, 10 -> 128 bit (11 unallocated)
 */
arminst disas_ld_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int64_t imm = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
    int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_signed = false;
    int size = 2;
    //TCGv_i64 tcg_rt, tcg_addr;

    if (is_vector) {
        if (opc == 3) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
        size = 2 + opc;
//        if (!fp_access_check(s)) {
//            return;
//        }
//    } else {
        if (opc == 3) {
            /* PRFM (literal) : prefetch */
            return;
        }
        size = 2 + extract32(opc, 0, 1);
        is_signed = extract32(opc, 1, 1);
    }

    ArmFormat operands( aFetchedOpcode.theOpcode );
    std::vector< std::list<InternalDependance> > rs_deps(2);
    addAddressCompute( inst, rs_deps ) ;
    ArmFormatOperands( inst, operands.rn(), rs_deps );

    //tcg_rt = cpu_reg(s, rt);
    //tcg_addr = tcg_const_i64((s->pc - 4) + imm);

    predicated_dependant_action load = loadAction( inst, (operands.rd() == 0 ? kWord : kDoubleWord), false, boost::none );

    if (is_vector) {
        inst->setClass(clsLoad, codeLoadFP);
        //do_fp_ld(s, rt, tcg_addr, size);
        inst->addSquashEffect( eraseLSQ(inst) );
//        inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
        inst->addRetirementEffect( retireMem(inst) );


        inst->addDispatchEffect( allocateLoad( inst, (operands.rd() == 0 ? kWord : kDoubleWord), load.dependance, kAccType_ORDERED ) );
        inst->addCommitEffect( accessMem(inst) );
        inst->addRetirementConstraint( loadMemoryConstraint(inst) );

        connectDependance( inst->retirementDependance(), load );
        inst->addRetirementEffect( writeFPSR(inst, (operands.rd() == 0 ? kWord : kDoubleWord)) );
        inst->setHaltDispatch();
//        inst->addPostvalidation( validateFPSR(inst) );

    } else {
        inst->setClass(clsLoad, codeLoad);
//        inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
        inst->addRetirementEffect( retireMem(inst) );
        inst->addSquashEffect( eraseLSQ(inst) );
        /* Only unsigned 32bit loads target 32bit registers.  */
        bool iss_sf = opc != 0;
        //do_gpr_ld(s, tcg_rt, tcg_addr, size, is_signed, false, true, rt, iss_sf, false);

        if (operands.rd() == 0) {
          load = loadAction( inst, eSize(1<<size), is_signed, boost::none );
        } else {
          load = loadAction( inst, eSize(1<<size), is_signed, kPD );
        }
    }
    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );
    addDestination( inst, operands.rd(), load);
    //tcg_temp_free_i64(tcg_addr);
    return boost::intrusive_ptr<armInstruction>(inst);

}

/*
 * LDNP (Load Pair - non-temporal hint)
 * LDP (Load Pair - non vector)
 * LDPSW (Load Pair Signed Word - non vector)
 * STNP (Store Pair - non-temporal hint)
 * STP (Store Pair - non vector)
 * LDNP (Load Pair of SIMD&FP - non-temporal hint)
 * LDP (Load Pair of SIMD&FP)
 * STNP (Store Pair of SIMD&FP - non-temporal hint)
 * STP (Store Pair of SIMD&FP)
 *
 *  31 30 29   27  26  25 24   23  22 21   15 14   10 9    5 4    0
 * +-----+-------+---+---+-------+---+-----------------------------+
 * | opc | 1 0 1 | V | 0 | index | L |  imm7 |  Rt2  |  Rn  | Rt   |
 * +-----+-------+---+---+-------+---+-------+-------+------+------+
 *
 * opc: LDP/STP/LDNP/STNP        00 -> 32 bit, 10 -> 64 bit
 *      LDPSW                    01
 *      LDP/STP/LDNP/STNP (SIMD) 00 -> 32 bit, 01 -> 64 bit, 10 -> 128 bit
 *   V: 0 -> GPR, 1 -> Vector
 * idx: 00 -> signed offset with non-temporal hint, 01 -> post-index,
 *      10 -> signed offset, 11 -> pre-index
 *   L: 0 -> Store 1 -> Load
 *
 * Rt, Rt2 = GPR or SIMD registers to be stored
 * Rn = general purpose register containing address
 * imm7 = signed offset (multiple of 4 or 8 depending on size)
 */
arminst disas_ldst_pair(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,aFetchedOpcode.theBPState,
                                                      aCPU,aSequenceNo));
    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint64_t offset = sextract64(aFetchedOpcode.theOpcode, 15, 7);
    int index = extract32(aFetchedOpcode.theOpcode, 23, 2);
    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
    bool is_load = extract32(aFetchedOpcode.theOpcode, 22, 1);
    int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    int memidx = 0;
    bool is_signed = false;
    bool postindex = false;
    bool wback = false;

    //TCGv_i64 tcg_addr; /* calculated address */
    int size;

    if (opc == 3) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (is_vector) {
        size = 2 + opc;
    } else {
        size = 2 + extract32(opc, 1, 1);
        is_signed = extract32(opc, 0, 1);
        if (!is_load && is_signed) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
    }

    switch (index) {
    case 1: /* post-index */
        postindex = true;
        wback = true;
        break;
    case 0:
        /* signed offset with "non-temporal" hint. Since we don't emulate
         * caches we don't care about hints to the cache system about
         * data access patterns, and handle this identically to plain
         * signed offset.
         */
        if (is_signed) {
            /* There is no non-temporal-hint version of LDPSW */
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        postindex = false;
        break;
    case 2: /* signed offset, rn not updated */
        postindex = false;
        break;
    case 3: /* pre-index */
        postindex = false;
        wback = true;
        break;
    }
//    if (is_vector && !fp_access_check(s)) {
//        return;
//    }
    offset <<= size;
//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }
//    tcg_addr = read_cpu_reg_sp(s, rn, 1);

    std::vector<std::list<InternalDependance>> rs_deps(3);
    addReadXRegister(inst, 1, rn, rs_deps[0]);

    if (!postindex) {
//        //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset);
        inst->setOperand( kUopAddressOffset, offset );
    }

    addAddressCompute(inst, rs_deps);
    satisfyAtDispatch(inst, rs_deps[0]);

    if (is_vector) {
        assert(false);
//        if (is_load) {
//            do_fp_ld(s, rt, tcg_addr, size);
//        } else {
//            do_fp_st(s, rt, tcg_addr, size);
//        }
//        //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//        if (is_load) {
//            do_fp_ld(s, rt2, tcg_addr, size);
//        } else {
//            do_fp_st(s, rt2, tcg_addr, size);
//        }
    } else {
        //TCGv_i64 tcg_rt = cpu_reg(s, rt);
        //TCGv_i64 tcg_rt2 = cpu_reg(s, rt2);
//        addReadXRegister(inst, 1, rt, rs_deps[1]);
//        addReadXRegister(inst, 2, rt2, rs_deps[2]);
        if (is_load) {
            //TCGv_i64 tmp = tcg_temp_new_i64();
            /* Do not modify tcg_rt before recognizing any exception
             * from the second load.
             */
//            do_gpr_ld(s, tmp, tcg_addr, size, is_signed, false,
//                      false, 0, false, false);
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//            do_gpr_ld(s, tcg_rt2, tcg_addr, size, is_signed, false,
//                      false, 0, false, false);
//            //tcg_gen_mov_i64(tcg_rt, tmp);
//            tcg_temp_free_i64(tmp);

            ldgpr(inst, rt, rn, size,false);
            inst->setOperand( kUopAddressOffset, static_cast<uint64_t>(1 << size) );
            addAddressCompute(inst,rs_deps);
//            satisfyAtDispatch(inst, rs_deps[1]);

//            ldgpr(inst, rt2, rn, size, false);
//            predicated_action mov = addExecute(inst, operation(kMOV_),rs_deps);
//            addDestination(inst, rt, mov);
//            satisfyAtDispatch(inst, rs_deps[2]);


        } else {
//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      false, 0, false, false);
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, 1 << size);
//            do_gpr_st(s, tcg_rt2, tcg_addr, size,
//                      false, 0, false, false);

//            STR(inst, rt, rn, size);
            DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
            inst->setClass(clsStore, codeStore);

//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
            inst->addSquashEffect( eraseLSQ(inst) );

            inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, kAccType_ORDERED) );
            inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
            inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

            addReadRD( inst, rt );
//            inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
            inst->addCommitEffect( commitStore(inst) );

//            inst->setOperand( kUopAddressOffset, static_cast<uint64_t>(1 << size) );
//            addAddressCompute(inst,rs_deps);

//            satisfyAtDispatch(inst, rs_deps[0]);

//            STR(inst, rt2, rn, size);
        }
    }

    if (wback) {
        predicated_action e;
        if (postindex) {
//            //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset - (1 << size));
            inst->setOperand(kOperand2, offset - (1 << size));
            e = addExecute(inst, operation(kADD_),rs_deps);
        } else {
//            //tcg_gen_subi_i64(tcg_addr, tcg_addr, 1 << size);
            e = addExecute(inst, operation(kSUB_),rs_deps);
        }
//        //tcg_gen_mov_i64(cpu_reg_sp(s, rn), tcg_addr);
        addDestination(inst, rn, e);
    }
    return inst;
}


/*
 * Load/store (immediate post-indexed)
 * Load/store (immediate pre-indexed)
 * Load/store (unscaled immediate)
 *
 * 31 30 29   27  26 25 24 23 22 21  20    12 11 10 9    5 4    0
 * +----+-------+---+-----+-----+---+--------+-----+------+------+
 * |size| 1 1 1 | V | 0 0 | opc | 0 |  imm9  | idx |  Rn  |  Rt  |
 * +----+-------+---+-----+-----+---+--------+-----+------+------+
 *
 * idx = 01 -> post-indexed, 11 pre-indexed, 00 unscaled imm. (no writeback)
         10 -> unprivileged
 * V = 0 -> non-vector
 * size: 00 -> 8 bit, 01 -> 16 bit, 10 -> 32 bit, 11 -> 64bit
 * opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 */
arminst disas_ldst_reg_imm9(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                                int opc,
                                int size,
                                int rt,
                                bool is_vector)
{
    assert(false);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int imm9 = sextract32(aFetchedOpcode.theOpcode, 12, 9);
    int idx = extract32(aFetchedOpcode.theOpcode, 10, 2);
    bool is_signed = false;
    bool is_store = false;
    bool is_extended = false;
    bool is_unpriv = (idx == 2);
    bool iss_valid = !is_vector;
    bool post_index;
    bool writeback;

//    TCGv_i64 tcg_addr;

    if (is_vector) {
        assert(false);
        size |= (opc & 2) << 1;
        if (size > 4 || is_unpriv) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
        is_store = ((opc & 1) == 0);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
            if (is_unpriv) {
                unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
                return;
            }
            return;
        }
        if (opc == 3 && size > 1) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

    switch (idx) {
    case 0:
    case 2:
        post_index = false;
        writeback = false;
        break;
    case 1:
        post_index = true;
        writeback = true;
        break;
    case 3:
        post_index = false;
        writeback = true;
        break;
    }

    //if (rn == 31) {
        /*
         * The AArch64 architecture mandates that (if enabled via PSTATE
         * or SCTLR bits) there is a check that SP is 16-aligned on every
         * SP-relative load or store (with an exception generated if it is not).
         * In line with general QEMU practice regarding misaligned accesses
        */
        //gen_check_sp_alignment(s);

    //}

    //tcg_addr = read_cpu_reg_sp(s, rn, 1);
    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rn, rs_deps[0]);
    if (!post_index) {
        //tcg_gen_addi_i64(tcg_addr, tcg_addr, imm9);
        inst->setOperand(kUopAddressOffset, static_cast<uint64_t>(imm9));
        addAddressCompute(inst,rs_deps);
    }

    if (is_vector) {
        assert(false);
//        if (is_store) {
//            do_fp_st(s, rt, tcg_addr, size);
//        } else {
//            do_fp_ld(s, rt, tcg_addr, size);
//        }
    } else {
//        TCGv_i64 tcg_rt = cpu_reg(s, rt);
//        int memidx = is_unpriv ? get_a64_user_mem_index(s) : get_mem_index(s);
        int memidx = 0;
//        memidx = aFetchedCode.theMemidx;
        bool iss_sf = disas_ldst_compute_iss_sf(size, is_signed, opc);

        if (is_store) {
//            do_gpr_st_memidx(s, tcg_rt, tcg_addr, size, memidx,
//                             iss_valid, rt, iss_sf, false);
//            STR(inst, rt, rn, size);
        } else {
//            do_gpr_ld_memidx(s, tcg_rt, tcg_addr, size,
//                             is_signed, is_extended, memidx,
//                             iss_valid, rt, iss_sf, false);
            ldgpr(inst, rt, rn, size, false);
        }
    }

    if (writeback) {
//        TCGv_i64 tcg_rn = cpu_reg_sp(s, rn);
        if (post_index) {
            predicated_action add = addExecute(inst,operation(kADD_),rs_deps);
            addDestination(inst,rn,add);
            //tcg_gen_addi_i64(tcg_addr, tcg_addr, imm9);
        }
        predicated_action mov = addExecute(inst,operation(kMOV_),rs_deps);
        addDestination(inst,rn,mov);
//        tcg_gen_mov_i64(tcg_rn, tcg_addr);
    }
}


static void ext_and_shift_reg(uint_fast64_t rd, uint_fast64_t rs,
                              int option, unsigned int shift)
{
    int extsize = extract32(option, 0, 2);
    bool is_signed = extract32(option, 2, 1);

    if (is_signed) {
        switch (extsize) {
        case 0:
//            tcg_gen_ext8s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 1:
//            tcg_gen_ext16s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 2:
//            tcg_gen_ext32s_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 3:
//            tcg_gen_mov_i64(tcg_out, tcg_in);
            assert(false);
            break;
        }
    } else {
        switch (extsize) {
        case 0:
//            tcg_gen_ext8u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 1:
//            tcg_gen_ext16u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 2:
//            tcg_gen_ext32u_i64(tcg_out, tcg_in);
            assert(false);
            break;
        case 3:
//            tcg_gen_mov_i64(tcg_out, tcg_in);
            assert(false);
            break;
        }
    }

    if (shift) {
//        tcg_gen_shli_i64(tcg_out, tcg_out, shift);
        assert(false);
    }
}


/*
 * Load/store (register offset)
 *
 * 31 30 29   27  26 25 24 23 22 21  20  16 15 13 12 11 10 9  5 4  0
 * +----+-------+---+-----+-----+---+------+-----+--+-----+----+----+
 * |size| 1 1 1 | V | 0 0 | opc | 1 |  Rm  | opt | S| 1 0 | Rn | Rt |
 * +----+-------+---+-----+-----+---+------+-----+--+-----+----+----+
 *
 * For non-vector:
 *   size: 00-> byte, 01 -> 16 bit, 10 -> 32bit, 11 -> 64bit
 *   opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 * For vector:
 *   size is opc<1>:size<1:0> so 100 -> 128 bit; 110 and 111 unallocated
 *   opc<0>: 0 -> store, 1 -> load
 * V: 1 -> vector/simd
 * opt: extend encoding (see DecodeRegExtend)
 * S: if S=1 then scale (essentially index by sizeof(size))
 * Rt: register to transfer into/out of
 * Rn: address register or SP for base
 * Rm: offset register or ZR for offset
 */
arminst disas_ldst_reg_roffset(armcode const & aFetchedOpcode,uint32_t  aCPU, int64_t aSequenceNo,
                                   int opc,
                                   int size,
                                   int rt,
                                   bool is_vector)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int shift = extract32(aFetchedOpcode.theOpcode, 12, 1);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int opt = extract32(aFetchedOpcode.theOpcode, 13, 3);
    bool is_signed = false;
    bool is_store = false;
    bool is_extended = false;

//    TCGv_i64 tcg_rm;
//    TCGv_i64 tcg_addr;

    if (extract32(opt, 1, 1) == 0) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    if (is_vector) {
        size |= (opc & 2) << 1;
        if (size > 4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = !extract32(opc, 0, 1);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
            return;
        }
        if (opc == 3 && size > 1) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }

//    tcg_addr = read_cpu_reg_sp(s, rn, 1);
//    tcg_rm = read_cpu_reg(s, rm, 1);
//    ext_and_shift_reg(tcg_rm, tcg_rm, opt, shift ? size : 0);
//    tcg_gen_add_i64(tcg_addr, tcg_addr, tcg_rm);

    std::vector<std::list<InternalDependance> > rs_deps(2);
//    addReadXRegister(inst, 1, rn, rs_deps[0]);
//    addReadXRegister(inst, 2, rm, rs_deps[1]);
//    ext_and_shift_reg(rm, rm, opt, shift ? size : 0);



    if (is_vector) {
        if (is_store) {
//            do_fp_st(s, rt, tcg_addr, size);
            stfpr(inst, rn, rt, size);
        } else {
//            do_fp_ld(s, rt, tcg_addr, size);
            ldfpr(inst, rn, rt, size);
        }
    } else {
//        TCGv_i64 tcg_rt = cpu_reg(s, rt);
        bool iss_sf = disas_ldst_compute_iss_sf(size, is_signed, opc);
        if (is_store) {
//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      true, rt, iss_sf, false);
//            STR(inst, rn, rt, size);
        } else {
//            do_gpr_ld(s, tcg_rt, tcg_addr, size,
//                      is_signed, is_extended,
//                      true, rt, iss_sf, false);
            ldgpr(inst, rn, rt, size, is_signed);
        }
    }
    return inst;
}

/*
 * Load/store (unsigned immediate)
 *
 * 31 30 29   27  26 25 24 23 22 21        10 9     5
 * +----+-------+---+-----+-----+------------+-------+------+
 * |size| 1 1 1 | V | 0 1 | opc |   imm12    |  Rn   |  Rt  |
 * +----+-------+---+-----+-----+------------+-------+------+
 *
 * For non-vector:
 *   size: 00-> byte, 01 -> 16 bit, 10 -> 32bit, 11 -> 64bit
 *   opc: 00 -> store, 01 -> loadu, 10 -> loads 64, 11 -> loads 32
 * For vector:
 *   size is opc<1>:size<1:0> so 100 -> 128 bit; 110 and 111 unallocated
 *   opc<0>: 0 -> store, 1 -> load
 * Rn: base address register (inc SP)
 * Rt: target register
 */
arminst disas_ldst_reg_unsigned_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                                        int opc,
                                        int size,
                                        int rt,
                                        bool is_vector)
{
    SemanticInstruction * inst (new SemanticInstruction(aFetchedOpcode.thePC,aFetchedOpcode.theOpcode,aFetchedOpcode.theBPState,aCPU,aSequenceNo));
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int imm12 = extract32(aFetchedOpcode.theOpcode, 10, 12);
    unsigned int offset;

//    TCGv_i64 tcg_addr;

    uint64_t addr;
    bool is_store;
    bool is_signed = false;
    bool is_extended = false;
    int memidx = 0;

    if (is_vector) {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register  VECTOR#1\033[0m"));

        size |= (opc & 2) << 1;
        if (size > 4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = !extract32(opc, 0, 1);
//        if (!fp_access_check(s)) {
//            return;
//        }
    } else {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register  non-VECTOR#1\033[0m"));

        if (size == 3 && opc == 2) {
            /* PRFM - prefetch */
        }
        if (opc == 3 && size > 1) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        is_store = (opc == 0);
        is_signed = extract32(opc, 1, 1);
        is_extended = (size < 3) && extract32(opc, 0, 1);
    }

//    if (rn == 31) {
//        gen_check_sp_alignment(s);
//    }

    //tcg_addr = read_cpu_reg_sp(s, rn, 1);
    //tcg_gen_addi_i64(tcg_addr, tcg_addr, offset);

    std::vector< std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst,1,rn,rs_deps[0]);
    offset = imm12 << size;

    inst->setOperand( kUopAddressOffset, offset );
    addAddressCompute(inst,rs_deps);



    if (is_vector) {

        if (is_store) {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-STF-VECTOR #2\033[0m"));

            stfpr(inst, rt, rn, size);
            //do_fp_st(s, rt, tcg_addr, size);
        } else {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-LDF-VECTOR #2\033[0m"));

            ldfpr(inst, rn, rt, size);
            //do_fp_ld(s, rt, tcg_addr, size);
        }
    } else {
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register nonVECTOR #2\033[0m"));

        //TCGv_i64 tcg_rt = cpu_reg(s, rt);

        bool iss_sf = disas_ldst_compute_iss_sf(eSize(1 << size), is_signed, opc);
        if (is_store) {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-ST nonVECTOR #2\033[0m"));

//            STR(inst, rt, size, memidx,
//                   true, rt, iss_sf, false);
            inst->setClass(clsStore, codeStoreFP);
            addReadXRegister(inst,2,rt, rs_deps[1]);
            addAddressCompute( inst, rs_deps ) ;
            inst->addSquashEffect( eraseLSQ(inst) );
//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
//            inst->addPrevalidation( validateFPSR(inst) );
            inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false , kAccType_ORDERED) );
            inst->addCommitEffect( commitStore(inst) );
//            inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
//            inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

//            do_gpr_st(s, tcg_rt, tcg_addr, size,
//                      true, rt, iss_sf, false);
        } else {
            DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register-LD nonVECTOR #2\033[0m"));

//            ldgpr(inst, rt, size, is_signed, is_extended, memidx,
//                  true, rt, iss_sf, false);
            inst->setClass(clsLoad, codeLoad);
            addAddressCompute( inst, rs_deps ) ;
            inst->addSquashEffect( eraseLSQ(inst) );
//            inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
            inst->addRetirementEffect( retireMem(inst) );
            predicated_dependant_action load;
            load = loadAction( inst, eSize(1 << size), is_extended, kPD );
            inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
            inst->addCommitEffect( accessMem(inst) );
            inst->addRetirementConstraint( loadMemoryConstraint(inst) );
            addDestination( inst, rt, load);
//            do_gpr_ld(s, tcg_rt, tcg_addr, size, is_signed, is_extended,
//                      true, rt, iss_sf, false);
        }
    }
    return inst;
}

/* Load/store register (all forms) */
arminst disas_ldst_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool is_vector = extract32(aFetchedOpcode.theOpcode, 26, 1);
    int size = extract32(aFetchedOpcode.theOpcode, 30, 2);

    switch (extract32(aFetchedOpcode.theOpcode, 24, 2)) {
    case 0:
        if (extract32(aFetchedOpcode.theOpcode, 21, 1) == 1 && extract32(aFetchedOpcode.theOpcode, 10, 2) == 2) {
            DBG_(Tmp,(<< "\033[1;31m disas_ldst_reg_roffset\033[0m"));
            return disas_ldst_reg_roffset(aFetchedOpcode, aCPU,aSequenceNo, opc, size, rt, is_vector);
        } else {
            /* Load/store register (unscaled immediate)
             * Load/store immediate pre/post-indexed
             * Load/store register unprivileged
             */
            DBG_(Tmp,(<< "\033[1;31m Load/store - register (unscaled immediate)"
                      " - immediate pre/post-indexed"
                      " - register unprivilege\033[0m"));

            return disas_ldst_reg_imm9( aFetchedOpcode, aCPU,aSequenceNo,opc, size, rt, is_vector);
        }
        break;
    case 1:
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/Store Unsigned Immediate - Register\033[0m"));
        return disas_ldst_reg_unsigned_imm(aFetchedOpcode, aCPU,aSequenceNo,opc, size, rt, is_vector);
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* Loads and stores */
arminst disas_ldst(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 24, 6)) {
    case 0x08: /* Load/store exclusive */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Load/store exclusiv \033[0m"));

        return disas_ldst_excl(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x18: case 0x1c: /* Load register (literal) */
        DBG_(Tmp,(<< "\033[1;31mDECODER: Load register (literal) \033[0m"));

        return disas_ld_lit(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x28: case 0x29:
    case 0x2c: case 0x2d: /* Load/store pair (all forms) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/store pair (all forms) \033[0m"));
        return disas_ldst_pair(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x38: case 0x39:
    case 0x3c: case 0x3d: /* Load/store register (all forms) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Load/store register (all forms) \033[0m"));
        return disas_ldst_reg(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    case 0x0c: /* AdvSIMD load/store multiple structures */
        DBG_(Tmp,(<< "\033[1;31mDECODER: AdvSIMD load/store multiple structures \033[0m"));

        assert(false);
        //disas_ldst_multiple_struct(s, aFetchedOpcode.theOpcode);
        break;
    case 0x0d: /* AdvSIMD load/store single structure */
        DBG_(Tmp,(<< "\033[1;31mDECODER: AdvSIMD load/store single structure \033[0m"));

        assert(false);
        //disas_ldst_single_struct(aFetchedOpcode, aCPU,aSequenceNo);
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

arminst disas_logic_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                       aFetchedOpcode.theOpcode,
                                                       aFetchedOpcode.theBPState,
                                                       aCPU,
                                                       aSequenceNo));
    //TCGv_i64 tcg_rd, tcg_rn, tcg_rm;
    unsigned int sf, opc, shift_type, invert, rm, shift_amount, rn, rd;

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opc = extract32(aFetchedOpcode.theOpcode, 29, 2);
    shift_type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    invert = extract32(aFetchedOpcode.theOpcode, 21, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    shift_amount = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (!sf && (shift_amount & (1 << 5))) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    tcg_rd = cpu_reg(s, rd);

    if (opc == 1 && shift_amount == 0 && shift_type == 0 && rn == 31) {
        /* Unshifted ORR and ORN with WZR/XZR is the standard encoding for
         * register-register MOV and MVN, so it is worth special casing.
         */
//        tcg_rm = cpu_reg(s, rm);
        std::vector< std::list<InternalDependance> > rs_deps(1);
        addReadXRegister(inst,1,rm,rs_deps[0]);
        if (invert) {
//          tcg_gen_not_i64(tcg_rd, tcg_rm);
            predicated_action exec = addExecute(inst,operation(kNot_),rs_deps);
            addDestination(inst, rd, exec);
            inst->addDispatchAction(exec);

            if (!sf) {
//              tcg_gen_ext32u_i64(tcg_rd, tcg_rd);

                predicated_action exec2 = addExecute(inst,operation(kZext_),rs_deps);
                addDestination(inst,rd,exec2);
                inst->addDispatchAction(exec);

            }
        } else {
            if (sf) {
//              tcg_gen_mov_i64(tcg_rd, tcg_rm);
                predicated_action exec = addExecute(inst,operation(kMOV_),rs_deps);
                addDestination(inst,rd,exec);
                inst->addDispatchAction(exec);

            } else {
//              tcg_gen_ext32u_i64(tcg_rd, tcg_rm);
                predicated_action exec = addExecute(inst,operation(kZext_),rs_deps);
                addDestination(inst,rd,exec);
                inst->addDispatchAction(exec);
            }
        }
        return inst;
    }

    std::vector< std::list<InternalDependance> > rs_deps(2);

//  tcg_rm = read_cpu_reg(s, rm, sf);

    addReadXRegister(inst, 1, rm , rs_deps[0]);

    if (shift_amount) {
//      shift_reg_imm(tcg_rm, tcg_rm, sf, shift_type, shift_amount);
        predicated_action exec = addExecute(inst,shift(shift_amount, shift_type),rs_deps);
        addDestination(inst,rm,exec);
        inst->addDispatchAction(exec);
    }

//    tcg_rn = cpu_reg(s, rn);
    addReadXRegister(inst, 2, rn , rs_deps[1]);
    predicated_action e;

    switch (opc | (invert << 2)) {
    case 0: /* AND */
    case 3: /* ANDS */        
        //tcg_gen_and_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kADD_),rs_deps);
        break;
    case 1: /* ORR */
        //tcg_gen_or_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kORR_),rs_deps);
        break;
    case 2: /* EOR */
        //tcg_gen_xor_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kXOR_),rs_deps);
        break;
    case 4: /* BIC */
    case 7: /* BICS */
        //tcg_gen_andc_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kAND_),rs_deps);
        break;
    case 5: /* ORN */
        //tcg_gen_orc_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kOrN_),rs_deps);
        break;
    case 6: /* EON */
        //tcg_gen_eqv_i64(tcg_rd, tcg_rn, tcg_rm);
        e = addExecute(inst,operation(kOrN_),rs_deps);
        break;
    default:
        assert(false);
        break;
    }

    addDestination(inst, rd, e);

    if (!sf) {
//      tcg_gen_ext32u_i64(tcg_rd, tcg_rd);
        addReadXRegister(inst, 1, rd, rs_deps[0]);
        predicated_action e = addExecute(inst,operation(kZext_),rs_deps);
        addDestination(inst, rd, e);
    }

//    if (opc == 3) {
//        gen_logic_CC(sf, tcg_rd);
//    }

    return inst;
}

/*
 * Add/subtract (extended register)
 *
 *  31|30|29|28       24|23 22|21|20   16|15  13|12  10|9  5|4  0|
 * +--+--+--+-----------+-----+--+-------+------+------+----+----+
 * |sf|op| S| 0 1 0 1 1 | opt | 1|  Rm   |option| imm3 | Rn | Rd |
 * +--+--+--+-----------+-----+--+-------+------+------+----+----+
 *
 *  sf: 0 -> 32bit, 1 -> 64bit
 *  op: 0 -> add  , 1 -> sub
 *   S: 1 -> set flags
 * opt: 00
 * option: extension type (see DecodeRegExtend)
 * imm3: optional shift to Rm
 *
 * Rd = Rn + LSL(extend(Rm), amount)
 */
arminst disas_add_sub_ext_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int imm3 = extract32(aFetchedOpcode.theOpcode, 10, 3);
    int option = extract32(aFetchedOpcode.theOpcode, 13, 3);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    bool setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

//    TCGv_i64 tcg_rm, tcg_rn; /* temps */
//    TCGv_i64 tcg_rd;
//    TCGv_i64 tcg_result;

    if (imm3 > 4) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    /* non-flag setting ops may use SP */
//    if (!setflags) {
//        tcg_rd = cpu_reg_sp(s, rd);
//    } else {
//        tcg_rd = cpu_reg(s, rd);
//    }
//    tcg_rn = read_cpu_reg_sp(s, rn, sf);

//    tcg_rm = read_cpu_reg(s, rm, sf);
//    ext_and_shift_reg(tcg_rm, tcg_rm, option, imm3);

//    tcg_result = tcg_temp_new_i64();

//    if (!setflags) {
//        if (sub_op) {
//            //tcg_gen_sub_i64(tcg_result, tcg_rn, tcg_rm);
//        } else {
//            //tcg_gen_add_i64(tcg_result, tcg_rn, tcg_rm);
//        }
//    } else {
//        if (sub_op) {
//            gen_sub_CC(sf, tcg_result, tcg_rn, tcg_rm);
//        } else {
//            gen_add_CC(sf, tcg_result, tcg_rn, tcg_rm);
//        }
//    }

//    if (sf) {
//        //tcg_gen_mov_i64(tcg_rd, tcg_result);
//    } else {
//        //tcg_gen_ext32u_i64(tcg_rd, tcg_result);
//    }

//    tcg_temp_free_i64(tcg_result);
}

/*
 * Add/subtract (shifted register)
 *
 *  31 30 29 28       24 23 22 21 20   16 15     10 9    5 4    0
 * +--+--+--+-----------+-----+--+-------+---------+------+------+
 * |sf|op| S| 0 1 0 1 1 |shift| 0|  Rm   |  imm6   |  Rn  |  Rd  |
 * +--+--+--+-----------+-----+--+-------+---------+------+------+
 *
 *    sf: 0 -> 32bit, 1 -> 64bit
 *    op: 0 -> add  , 1 -> sub
 *     S: 1 -> set flags
 * shift: 00 -> LSL, 01 -> LSR, 10 -> ASR, 11 -> RESERVED
 *  imm6: Shift amount to apply to Rm before the add/sub
 */
arminst disas_add_sub_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int imm6 = extract32(aFetchedOpcode.theOpcode, 10, 6);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int shift_type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sub_op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

//    TCGv_i64 tcg_rd = cpu_reg(s, rd);
//    TCGv_i64 tcg_rn, tcg_rm;
//    TCGv_i64 tcg_result;

    std::vector< std::list<InternalDependance> > rs_deps(3);


    if ((shift_type == 3) || (!sf && (imm6 > 31))) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

//    tcg_rn = read_cpu_reg(s, rn, sf);
//    tcg_rm = read_cpu_reg(s, rm, sf);
//    shift_reg_imm(tcg_rm, tcg_rm, sf, shift_type, imm6);

    addReadXRegister(inst, 1, rn, rs_deps[0]);
    addReadXRegister(inst, 2, rm, rs_deps[1]);
    predicated_action e = addExecute(inst,shift_type==0 ? operation(kMOV_) : shift(imm6, shift_type),rs_deps);
    addDestination(inst,rm, e);

    predicated_action exec;

//    tcg_result = tcg_temp_new_i64();
    if (!setflags) {
        if (sub_op) {
            //tcg_gen_sub_i64(tcg_result, tcg_rn, tcg_rm);
            exec = addExecute(inst,operation(kSUB_),rs_deps);
        } else {
            //tcg_gen_add_i64(tcg_result, tcg_rn, tcg_rm);
            exec = addExecute(inst,operation(kADD_),rs_deps);
        }
    } else {
        if (sub_op) {
            //  gen_sub_CC(sf, tcg_result, tcg_rn, tcg_rm);
            exec = addExecute(inst,operation(kSUBS_),rs_deps);
        } else {
            //  gen_add_CC(sf, tcg_result, tcg_rn, tcg_rm);
            exec = addExecute(inst,operation(kADDS_),rs_deps);
        }
    }

    if (sf) {
//        //tcg_gen_mov_i64(tcg_rd, tcg_result);
        addDestination(inst, rd, exec);
    } else {
        addDestination(inst, rd, exec);

//        //tcg_gen_ext32u_i64(tcg_rd, tcg_result);
    }
    return inst;

//    tcg_temp_free_i64(tcg_result);
}

/* Data-processing (3 source)
 *
 *    31 30  29 28       24 23 21  20  16  15  14  10 9    5 4    0
 *  +--+------+-----------+------+------+----+------+------+------+
 *  |sf| op54 | 1 1 0 1 1 | op31 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 *  +--+------+-----------+------+------+----+------+------+------+
 */
arminst disas_data_proc_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    SemanticInstruction* inst(new SemanticInstruction
                              (aFetchedOpcode.thePC,
                              aFetchedOpcode.theOpcode,
                              aFetchedOpcode.theBPState,
                              aCPU,
                              aSequenceNo));

    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int ra = extract32(aFetchedOpcode.theOpcode, 10, 5);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int op_id = (extract32(aFetchedOpcode.theOpcode, 29, 3) << 4) |
        (extract32(aFetchedOpcode.theOpcode, 21, 3) << 1) |
        extract32(aFetchedOpcode.theOpcode, 15, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool is_sub = extract32(op_id, 0, 1);
    bool is_high = extract32(op_id, 2, 1);
    bool is_signed = false;
//    TCGv_i64 tcg_op1;
//    TCGv_i64 tcg_op2;
//    TCGv_i64 tcg_tmp;

    /* Note that op_id is sf:op54:op31:o0 so it includes the 32/64 size flag */
    switch (op_id) {
    case 0x42: /* SMADDL */
    case 0x43: /* SMSUBL */
    case 0x44: /* SMULH */
        is_signed = true;
        break;
    case 0x0: /* MADD (32bit) */
    case 0x1: /* MSUB (32bit) */
    case 0x40: /* MADD (64bit) */
    case 0x41: /* MSUB (64bit) */
    case 0x4a: /* UMADDL */
    case 0x4b: /* UMSUBL */
    case 0x4c: /* UMULH */
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    std::vector< std::list<InternalDependance> > rs_deps(2);
    addReadXRegister(inst,1,rn, rs_deps[0]);
    addReadXRegister(inst,2,rm, rs_deps[1]);

    if (is_high) {
//        TCGv_i64 low_bits = tcg_temp_new_i64(); /* low bits discarded */
//        TCGv_i64 tcg_rd = cpu_reg(s, rd);
//        TCGv_i64 tcg_rn = cpu_reg(s, rn);
//        TCGv_i64 tcg_rm = cpu_reg(s, rm);
        predicated_action e = addExecute( inst, is_signed ? operation(kSMul_) : operation(kUMul_), rs_deps ) ;
        connectDependance( inst->retirementDependance(), e );
        return inst;

//        if (is_signed) {
//            tcg_gen_muls2_i64(low_bits, tcg_rd, tcg_rn, tcg_rm);
//        } else {
//            tcg_gen_mulu2_i64(low_bits, tcg_rd, tcg_rn, tcg_rm);
//        }
//        tcg_temp_free_i64(low_bits);
    }

//    tcg_op1 = tcg_temp_new_i64();
//    tcg_op2 = tcg_temp_new_i64();
//    tcg_tmp = tcg_temp_new_i64();

    if (op_id < 0x42) {
//      tcg_gen_mov_i64(tcg_op1, cpu_reg(s, rn));
//      tcg_gen_mov_i64(tcg_op2, cpu_reg(s, rm));
    } else {
        predicated_action e = addExecute( inst, is_signed ? operation(kSext_) : operation(kZext_), rs_deps ) ;
        predicated_action e1 = addExecute( inst, is_signed ? operation(kSext_) : operation(kZext_), rs_deps ) ;
        inst->addDispatchAction( e );
        inst->addDispatchAction( e );

//        if (is_signed) {
//          tcg_gen_ext32s_i64(tcg_op1, cpu_reg(s, rn));
//          tcg_gen_ext32s_i64(tcg_op2, cpu_reg(s, rm));
//        } else {
//          tcg_gen_ext32u_i64(tcg_op1, cpu_reg(s, rn));
//          tcg_gen_ext32u_i64(tcg_op2, cpu_reg(s, rm));
//        }
    }

    if (ra == 31 && !is_sub) {
        /* Special-case MADD with rA == XZR; it is the standard MUL alias */
//      tcg_gen_mul_i64(cpu_reg(s, rd), tcg_op1, tcg_op2);
        predicated_action e = addExecute( inst, operation(kMulX_), rs_deps ) ;
        addDestination(inst,rd,e);
    } else {
//      tcg_gen_mul_i64(tcg_tmp, tcg_op1, tcg_op2);
        addReadXRegister(inst,1,rd,rs_deps[0]);
        addReadXRegister(inst,2,ra,rs_deps[1]);
        predicated_action e = addExecute( inst, is_sub ? operation(kSUB_) : operation(kADD_), rs_deps );
        addDestination(inst,rd,e);
//        if (is_sub) {
//          tcg_gen_sub_i64(cpu_reg(s, rd), cpu_reg(s, ra), tcg_tmp);
//        } else {
//            //tcg_gen_add_i64(cpu_reg(s, rd), cpu_reg(s, ra), tcg_tmp);
//        }
    }

    if (!sf) {
        predicated_action e = addExecute( inst, operation(kZext_), rs_deps );
        connectDependance(inst->retirementDependance(), e);
//        tcg_gen_ext32u_i64(cpu_reg(s, rd), cpu_reg(s, rd));
    }

    return inst;

//    tcg_temp_free_i64(tcg_op1);
//    tcg_temp_free_i64(tcg_op2);
//    tcg_temp_free_i64(tcg_tmp);
}

/* Add/subtract (with carry)
 *  31 30 29 28 27 26 25 24 23 22 21  20  16  15   10  9    5 4   0
 * +--+--+--+------------------------+------+---------+------+-----+
 * |sf|op| S| 1  1  0  1  0  0  0  0 |  rm  | opcode2 |  Rn  |  Rd |
 * +--+--+--+------------------------+------+---------+------+-----+
 *                                            [000000]
 */

arminst disas_adc_sbc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, op, setflags, rm, rn, rd;
//    TCGv_i64 tcg_y, tcg_rn, tcg_rd;

    if (extract32(aFetchedOpcode.theOpcode, 10, 6) != 0) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    setflags = extract32(aFetchedOpcode.theOpcode, 29, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

//    tcg_rd = cpu_reg(s, rd);
//    tcg_rn = cpu_reg(s, rn);

//    if (op) {
//        tcg_y = new_tmp_a64(s);
//        //tcg_gen_not_i64(tcg_y, cpu_reg(s, rm));
//    } else {
//        tcg_y = cpu_reg(s, rm);
//    }

//    if (setflags) {
//        gen_adc_CC(sf, tcg_rd, tcg_rn, tcg_y);
//    } else {
//        gen_adc(sf, tcg_rd, tcg_rn, tcg_y);
//    }
}

/* Conditional compare (immediate / register)
 *  31 30 29 28 27 26 25 24 23 22 21  20    16 15  12  11  10  9   5  4 3   0
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 * |sf|op| S| 1  1  0  1  0  0  1  0 |imm5/rm | cond |i/r |o2|  Rn  |o3|nzcv |
 * +--+--+--+------------------------+--------+------+----+--+------+--+-----+
 *        [1]                             y                [0]       [0]
 */
arminst disas_cc(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, op, y, cond, rn, nzcv, is_imm;
//    TCGv_i32 tcg_t0, tcg_t1, tcg_t2;
//    TCGv_i64 tcg_tmp, tcg_y, tcg_rn;
//    DisasCompare c;

    if (!extract32(aFetchedOpcode.theOpcode, 29, 1)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
    if (aFetchedOpcode.theOpcode & (1 << 10 | 1 << 4)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    op = extract32(aFetchedOpcode.theOpcode, 30, 1);
    is_imm = extract32(aFetchedOpcode.theOpcode, 11, 1);
    y = extract32(aFetchedOpcode.theOpcode, 16, 5); /* y = rm (mapped_reg) or imm5 (imm) */
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    nzcv = extract32(aFetchedOpcode.theOpcode, 0, 4);

    /* Set T0 = !COND.  */
//    tcg_t0 = tcg_temp_new_i32();
//    arm_test_cc(&c, cond);
//    //tcg_gen_setcondi_i32(tcg_invert_cond(c.cond), tcg_t0, c.value, 0);
//    arm_free_cc(&c);

//    /* Load the arguments for the new comparison.  */
//    if (is_imm) {
//        tcg_y = new_tmp_a64(s);
//        //tcg_gen_movi_i64(tcg_y, y);
//    } else {
//        tcg_y = cpu_reg(s, y);
//    }
//    tcg_rn = cpu_reg(s, rn);

//    /* Set the flags for the new comparison.  */
//    tcg_tmp = tcg_temp_new_i64();
//    if (op) {
//        gen_sub_CC(sf, tcg_tmp, tcg_rn, tcg_y);
//    } else {
//        gen_add_CC(sf, tcg_tmp, tcg_rn, tcg_y);
//    }
//    tcg_temp_free_i64(tcg_tmp);

//    /* If COND was false, force the flags to #nzcv.  Compute two masks
//     * to help with this: T1 = (COND ? 0 : -1), T2 = (COND ? -1 : 0).
//     * For tcg hosts that support ANDC, we can make do with just T1.
//     * In either case, allow the tcg optimizer to delete any unused mask.
//     */
//    tcg_t1 = tcg_temp_new_i32();
//    tcg_t2 = tcg_temp_new_i32();
//    //tcg_gen_neg_i32(tcg_t1, tcg_t0);
//    //tcg_gen_subi_i32(tcg_t2, tcg_t0, 1);

//    if (nzcv & 8) { /* N */
//        //tcg_gen_or_i32(cpu_NF, cpu_NF, tcg_t1);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_NF, cpu_NF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_NF, cpu_NF, tcg_t2);
//        }
//    }
//    if (nzcv & 4) { /* Z */
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_ZF, cpu_ZF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_ZF, cpu_ZF, tcg_t2);
//        }
//    } else {
//        //tcg_gen_or_i32(cpu_ZF, cpu_ZF, tcg_t0);
//    }
//    if (nzcv & 2) { /* C */
//        //tcg_gen_or_i32(cpu_CF, cpu_CF, tcg_t0);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_CF, cpu_CF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_CF, cpu_CF, tcg_t2);
//        }
//    }
//    if (nzcv & 1) { /* V */
//        //tcg_gen_or_i32(cpu_VF, cpu_VF, tcg_t1);
//    } else {
//        if (TCG_TARGET_HAS_andc_i32) {
//            //tcg_gen_andc_i32(cpu_VF, cpu_VF, tcg_t1);
//        } else {
//            //tcg_gen_and_i32(cpu_VF, cpu_VF, tcg_t2);
//        }
//    }
//    tcg_temp_free_i32(tcg_t0);
//    tcg_temp_free_i32(tcg_t1);
//    tcg_temp_free_i32(tcg_t2);
}

/* Conditional select
 *   31   30  29  28             21 20  16 15  12 11 10 9    5 4    0
 * +----+----+---+-----------------+------+------+-----+------+------+
 * | sf | op | S | 1 1 0 1 0 1 0 0 |  Rm  | cond | op2 |  Rn  |  Rd  |
 * +----+----+---+-----------------+------+------+-----+------+------+
 */
arminst disas_cond_select(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, else_inv, rm, cond, else_inc, rn, rd;
//    TCGv_i64 tcg_rd, zero;
//    DisasCompare64 c;

    if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 11, 1)) {
        /* S == 1 or op2<1> == 1 */
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    else_inv = extract32(aFetchedOpcode.theOpcode, 30, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    else_inc = extract32(aFetchedOpcode.theOpcode, 10, 1);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

//    tcg_rd = cpu_reg(s, rd);

//    a64_test_cc(&c, cond);
//    zero = tcg_const_i64(0);

//    if (rn == 31 && rm == 31 && (else_inc ^ else_inv)) {
//        /* CSET & CSETM.  */
//        //tcg_gen_setcond_i64(tcg_invert_cond(c.cond), tcg_rd, c.value, zero);
//        if (else_inv) {
//            //tcg_gen_neg_i64(tcg_rd, tcg_rd);
//        }
//    } else {
//        TCGv_i64 t_true = cpu_reg(s, rn);
//        TCGv_i64 t_false = read_cpu_reg(s, rm, 1);
//        if (else_inv && else_inc) {
//            //tcg_gen_neg_i64(t_false, t_false);
//        } else if (else_inv) {
//            //tcg_gen_not_i64(t_false, t_false);
//        } else if (else_inc) {
//            //tcg_gen_addi_i64(t_false, t_false, 1);
//        }
//        //tcg_gen_movcond_i64(c.cond, tcg_rd, c.value, zero, t_true, t_false);
//    }

//    tcg_temp_free_i64(zero);
//    a64_free_cc(&c);

//    if (!sf) {
//        //tcg_gen_ext32u_i64(tcg_rd, tcg_rd);
//    }
}


//arminst handle_clz(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, unsigned int sf,
//                       unsigned int rn, unsigned int rd)
//{
//    TCGv_i64 tcg_rd, tcg_rn;
//    tcg_rd = cpu_reg(s, rd);
//    tcg_rn = cpu_reg(s, rn);

//    if (sf) {
//        tcg_gen_clzi_i64(tcg_rd, tcg_rn, 64);
//    } else {
//        TCGv_i32 tcg_tmp32 = tcg_temp_new_i32();
//        //tcg_gen_extrl_i64_i32(tcg_tmp32, tcg_rn);
//        //tcg_gen_clzi_i32(tcg_tmp32, tcg_tmp32, 32);
//        //tcg_gen_extu_i32_i64(tcg_rd, tcg_tmp32);
//        tcg_temp_free_i32(tcg_tmp32);
//    }
//}

//arminst handle_cls(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, unsigned int sf,
//                       unsigned int rn, unsigned int rd)
//{
//    TCGv_i64 tcg_rd, tcg_rn;
//    tcg_rd = cpu_reg(s, rd);
//    tcg_rn = cpu_reg(s, rn);

//    if (sf) {
//        //tcg_gen_clrsb_i64(tcg_rd, tcg_rn);
//    } else {
//        TCGv_i32 tcg_tmp32 = tcg_temp_new_i32();
//        //tcg_gen_extrl_i64_i32(tcg_tmp32, tcg_rn);
//        //tcg_gen_clrsb_i32(tcg_tmp32, tcg_tmp32);
//        //tcg_gen_extu_i32_i64(tcg_rd, tcg_tmp32);
//        tcg_temp_free_i32(tcg_tmp32);
//    }
//}

//arminst handle_rbit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, unsigned int sf,
//                        unsigned int rn, unsigned int rd)
//{
//    TCGv_i64 tcg_rd, tcg_rn;
//    tcg_rd = cpu_reg(s, rd);
//    tcg_rn = cpu_reg(s, rn);

//    if (sf) {
//        gen_helper_rbit64(tcg_rd, tcg_rn);
//    } else {
//        TCGv_i32 tcg_tmp32 = tcg_temp_new_i32();
//        //tcg_gen_extrl_i64_i32(tcg_tmp32, tcg_rn);
//        gen_helper_rbit(tcg_tmp32, tcg_tmp32);
//        //tcg_gen_extu_i32_i64(tcg_rd, tcg_tmp32);
//        tcg_temp_free_i32(tcg_tmp32);
//    }
//}

/* REV with sf==1, opcode==3 ("REV64") */
//arminst handle_rev64(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, unsigned int sf,
//                         unsigned int rn, unsigned int rd)
//{
//    if (!sf) {
//        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        return;
//    }
    //tcg_gen_bswap64_i64(cpu_reg(s, rd), cpu_reg(s, rn));
//}

/* REV with sf==0, opcode==2
 * REV32 (sf==1, opcode==2)
 */
//arminst handle_rev32(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, unsigned int sf,
//                         unsigned int rn, unsigned int rd)
//{
//    TCGv_i64 tcg_rd = cpu_reg(s, rd);

//    if (sf) {
//        TCGv_i64 tcg_tmp = tcg_temp_new_i64();
//        TCGv_i64 tcg_rn = read_cpu_reg(s, rn, sf);

//        /* bswap32_i64 requires zero high word */
//        //tcg_gen_ext32u_i64(tcg_tmp, tcg_rn);
//        //tcg_gen_bswap32_i64(tcg_rd, tcg_tmp);
//        //tcg_gen_shri_i64(tcg_tmp, tcg_rn, 32);
//        //tcg_gen_bswap32_i64(tcg_tmp, tcg_tmp);
//        //tcg_gen_concat32_i64(tcg_rd, tcg_rd, tcg_tmp);

//        tcg_temp_free_i64(tcg_tmp);
//    } else {
//        //tcg_gen_ext32u_i64(tcg_rd, cpu_reg(s, rn));
//        //tcg_gen_bswap32_i64(tcg_rd, tcg_rd);
//    }
//}

/* REV16 (opcode==1) */
//arminst handle_rev16(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
//                     unsigned int sf,
//                         unsigned int rn, unsigned int rd)
//{
//    TCGv_i64 tcg_rd = cpu_reg(s, rd);
//    TCGv_i64 tcg_tmp = tcg_temp_new_i64();
//    TCGv_i64 tcg_rn = read_cpu_reg(s, rn, sf);
//    TCGv_i64 mask = tcg_const_i64(sf ? 0x00ff00ff00ff00ffull : 0x00ff00ff);

//    tcg_gen_shri_i64(tcg_tmp, tcg_rn, 8);
//    tcg_gen_and_i64(tcg_rd, tcg_rn, mask);
//    tcg_gen_and_i64(tcg_tmp, tcg_tmp, mask);
//    tcg_gen_shli_i64(tcg_rd, tcg_rd, 8);
//    tcg_gen_or_i64(tcg_rd, tcg_rd, tcg_tmp);

//    tcg_temp_free_i64(mask);
//    tcg_temp_free_i64(tcg_tmp);
//}

/* Data-processing (1 source)
 *   31  30  29  28             21 20     16 15    10 9    5 4    0
 * +----+---+---+-----------------+---------+--------+------+------+
 * | sf | 1 | S | 1 1 0 1 0 1 1 0 | opcode2 | opcode |  Rn  |  Rd  |
 * +----+---+---+-----------------+---------+--------+------+------+
 */
arminst disas_data_proc_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, opcode, rn, rd;

    if (extract32(aFetchedOpcode.theOpcode, 29, 1) || extract32(aFetchedOpcode.theOpcode, 16, 5)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    opcode = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    switch (opcode) {
    case 0: /* RBIT */
//        handle_rbit(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 1: /* REV16 */
//        handle_rev16(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 2: /* REV32 */
//        handle_rev32(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 3: /* REV64 */
//        handle_rev64(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 4: /* CLZ */
//        handle_clz(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    case 5: /* CLS */
//        handle_cls(aFetchedOpcode, aCPU, aSequenceNo, sf, rn, rd);
        break;
    }
}


arminst handle_div(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                        bool is_signed, unsigned int sf,
                       unsigned int rm, unsigned int rn, unsigned int rd)
{
//    TCGv_i64 tcg_n, tcg_m, tcg_rd;
//    tcg_rd = cpu_reg(s, rd);

//    if (!sf && is_signed) {
//        tcg_n = new_tmp_a64(s);
//        tcg_m = new_tmp_a64(s);
//        tcg_gen_ext32s_i64(tcg_n, cpu_reg(s, rn));
//        tcg_gen_ext32s_i64(tcg_m, cpu_reg(s, rm));
//    } else {
//        tcg_n = read_cpu_reg(s, rn, sf);
//        tcg_m = read_cpu_reg(s, rm, sf);
//    }

//    if (is_signed) {
//        gen_helper_sdiv64(tcg_rd, tcg_n, tcg_m);
//    } else {
//        gen_helper_udiv64(tcg_rd, tcg_n, tcg_m);
//    }

//    if (!sf) { /* zero extend final result */
//        tcg_gen_ext32u_i64(tcg_rd, tcg_rd);
//    }
}


/* LSLV, LSRV, ASRV, RORV */
arminst handle_shift_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                             enum a64_shift_type shift_type, unsigned int sf,
                             unsigned int rm, unsigned int rn, unsigned int rd)
{
//    TCGv_i64 tcg_shift = tcg_temp_new_i64();
//    TCGv_i64 tcg_rd = cpu_reg(s, rd);
//    TCGv_i64 tcg_rn = read_cpu_reg(s, rn, sf);

//    tcg_gen_andi_i64(tcg_shift, cpu_reg(s, rm), sf ? 63 : 31);
//    shift_reg(tcg_rd, tcg_rn, sf, shift_type, tcg_shift);
//    tcg_temp_free_i64(tcg_shift);
}

/* CRC32[BHWX], CRC32C[BHWX] */
arminst handle_crc32(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo,
                         unsigned int sf, unsigned int sz, bool crc32c,
                         unsigned int rm, unsigned int rn, unsigned int rd)
{
//    TCGv_i64 tcg_acc, tcg_val;
//    TCGv_i32 tcg_bytes;

//    if (!arm_dc_feature(s, ARM_FEATURE_CRC)
//        || (sf == 1 && sz != 3)
//        || (sf == 0 && sz == 3)) {
//        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//        return;
//    }

//    if (sz == 3) {
//        tcg_val = cpu_reg(s, rm);
//    } else {
//        uint64_t mask;
//        switch (sz) {
//        case 0:
//            mask = 0xFF;
//            break;
//        case 1:
//            mask = 0xFFFF;
//            break;
//        case 2:
//            mask = 0xFFFFFFFF;
//            break;
//        default:
//            g_assert_not_reached();
//        }
//        tcg_val = new_tmp_a64(s);
//        tcg_gen_andi_i64(tcg_val, cpu_reg(s, rm), mask);
//    }

//    tcg_acc = cpu_reg(s, rn);
//    tcg_bytes = tcg_const_i32(1 << sz);

//    if (crc32c) {
//        gen_helper_crc32c_64(cpu_reg(s, rd), tcg_acc, tcg_val, tcg_bytes);
//    } else {
//        gen_helper_crc32_64(cpu_reg(s, rd), tcg_acc, tcg_val, tcg_bytes);
//    }

//    tcg_temp_free_i32(tcg_bytes);
}


/* Floating-point data-processing (3 source) - single precision */
arminst handle_fp_3src_single(bool o0, bool o1,
                                  int rd, int rn, int rm, int ra)
{
//    TCGv_i32 tcg_op1, tcg_op2, tcg_op3;
//    TCGv_i32 tcg_res = tcg_temp_new_i32();
//    TCGv_ptr fpst = get_fpstatus_ptr();

//    tcg_op1 = read_fp_sreg(s, rn);
//    tcg_op2 = read_fp_sreg(s, rm);
//    tcg_op3 = read_fp_sreg(s, ra);

    /* These are fused multiply-add, and must be done as one
     * floating point operation with no rounding between the
     * multiplication and addition steps.
     * NB that doing the negations here as separate steps is
     * correct : an input NaN should come out with its sign bit
     * flipped if it is a negated-input.
     */
    if (o1 == true) {
//        gen_helper_vfp_negs(tcg_op3, tcg_op3);
    }

    if (o0 != o1) {
//        gen_helper_vfp_negs(tcg_op1, tcg_op1);
    }

//    gen_helper_vfp_muladds(tcg_res, tcg_op1, tcg_op2, tcg_op3, fpst);

//    write_fp_sreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i32(tcg_op1);
//    tcg_temp_free_i32(tcg_op2);
//    tcg_temp_free_i32(tcg_op3);
//    tcg_temp_free_i32(tcg_res);
}

/* Floating-point data-processing (3 source) - double precision */
arminst handle_fp_3src_double(bool o0, bool o1,
                                  int rd, int rn, int rm, int ra)
{
//    TCGv_i64 tcg_op1, tcg_op2, tcg_op3;
//    TCGv_i64 tcg_res = tcg_temp_new_i64();
//    TCGv_ptr fpst = get_fpstatus_ptr();

//    tcg_op1 = read_fp_dreg(s, rn);
//    tcg_op2 = read_fp_dreg(s, rm);
//    tcg_op3 = read_fp_dreg(s, ra);

    /* These are fused multiply-add, and must be done as one
     * floating point operation with no rounding between the
     * multiplication and addition steps.
     * NB that doing the negations here as separate steps is
     * correct : an input NaN should come out with its sign bit
     * flipped if it is a negated-input.
     */
    if (o1 == true) {
//        gen_helper_vfp_negd(tcg_op3, tcg_op3);
    }

    if (o0 != o1) {
//        gen_helper_vfp_negd(tcg_op1, tcg_op1);
    }

//    gen_helper_vfp_muladdd(tcg_res, tcg_op1, tcg_op2, tcg_op3, fpst);

//    write_fp_dreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i64(tcg_op1);
//    tcg_temp_free_i64(tcg_op2);
//    tcg_temp_free_i64(tcg_op3);
//    tcg_temp_free_i64(tcg_res);
}



/* Floating point <-> integer conversions
 *   31   30  29 28       24 23  22  21 20   19 18 16 15         10 9  5 4  0
 * +----+---+---+-----------+------+---+-------+-----+-------------+----+----+
 * | sf | 0 | S | 1 1 1 1 0 | type | 1 | rmode | opc | 0 0 0 0 0 0 | Rn | Rd |
 * +----+---+---+-----------+------+---+-------+-----+-------------+----+----+
 */
arminst disas_fp_int_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int opcode = extract32(aFetchedOpcode.theOpcode, 16, 3);
    int rmode = extract32(aFetchedOpcode.theOpcode, 19, 2);
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool sbit = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);

    if (sbit) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    if (opcode > 5) {
        /* FMOV */
        bool itof = opcode & 1;

        if (rmode >= 2) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }

        switch (sf << 3 | type << 1 | rmode) {
        case 0x0: /* 32 bit */
        case 0xa: /* 64 bit */
        case 0xd: /* 64 bit to top half of quad */
            break;
        default:
            /* all other sf/type/rmode combinations are invalid */
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }

//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fmov(s, rd, rn, type, itof);
    } else {
        /* actual FP conversions */
        bool itof = extract32(opcode, 1, 1);

        if (type > 1 || (rmode != 0 && opcode > 1)) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }

//        if (!fp_access_check(s)) {
//            return;
//        }
//        handle_fpfpcvt(s, rd, rn, opcode, itof, rmode, 64, sf, type);
    }
}


/* Floating point conditional compare
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5  4   3    0
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 0 1 |  Rn  | op | nzcv |
 * +---+---+---+-----------+------+---+------+------+-----+------+----+------+
 */
arminst disas_fp_ccomp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, cond, rn, op, nzcv;
//    TCGv_i64 tcg_flags;
//    TCGLabel *label_continue = NULL;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    op = extract32(aFetchedOpcode.theOpcode, 4, 1);
    nzcv = extract32(aFetchedOpcode.theOpcode, 0, 4);

    if (mos || type > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    if (cond < 0x0e) { /* not always */
//        TCGLabel *label_match = gen_new_label();
//        label_continue = gen_new_label();
//        arm_gen_test_cc(cond, label_match);
//        /* nomatch: */
//        tcg_flags = tcg_const_i64(nzcv << 28);
//        gen_set_nzcv(tcg_flags);
//        tcg_temp_free_i64(tcg_flags);
//        tcg_gen_br(label_continue);
//        gen_set_label(label_match);
//    }

//    handle_fp_compare(s, type, rn, rm, false, op);

    if (cond < 0x0e) {
//        gen_set_label(label_continue);
    }
}

/* Floating point conditional select
 *   31  30  29 28       24 23  22  21 20  16 15  12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | cond | 1 1 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+------+-----+------+------+
 */
arminst disas_fp_csel(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, cond, rn, rd;
//    TCGv_i64 t_true, t_false, t_zero;
//    DisasCompare64 c;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    cond = extract32(aFetchedOpcode.theOpcode, 12, 4);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (mos || type > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

    /* Zero extend sreg inputs to 64 bits now.  */
//    t_true = tcg_temp_new_i64();
//    t_false = tcg_temp_new_i64();
//    read_vec_element(s, t_true, rn, 0, type ? MO_64 : MO_32);
//    read_vec_element(s, t_false, rm, 0, type ? MO_64 : MO_32);

//    a64_test_cc(&c, cond);
//    t_zero = tcg_const_i64(0);
//    tcg_gen_movcond_i64(c.cond, t_true, c.value, t_zero, t_true, t_false);
//    tcg_temp_free_i64(t_zero);
//    tcg_temp_free_i64(t_false);
//    a64_free_cc(&c);

    /* Note that sregs write back zeros to the high bits,
       and we've already done the zero-extension.  */
//    write_fp_dreg(s, rd, t_true);
//    tcg_temp_free_i64(t_true);
}


/* Floating point <-> fixed point conversions
 *   31   30  29 28       24 23  22  21 20   19 18    16 15   10 9    5 4    0
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 * | sf | 0 | S | 1 1 1 1 0 | type | 0 | rmode | opcode | scale |  Rn  |  Rd  |
 * +----+---+---+-----------+------+---+-------+--------+-------+------+------+
 */
arminst disas_fp_fixed_conv(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int scale = extract32(aFetchedOpcode.theOpcode, 10, 6);
    int opcode = extract32(aFetchedOpcode.theOpcode, 16, 3);
    int rmode = extract32(aFetchedOpcode.theOpcode, 19, 2);
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    bool sbit = extract32(aFetchedOpcode.theOpcode, 29, 1);
    bool sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    bool itof;

    if (sbit || (type > 1)
        || (!sf && scale < 32)) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch ((rmode << 3) | opcode) {
    case 0x2: /* SCVTF */
    case 0x3: /* UCVTF */
        itof = true;
        break;
    case 0x18: /* FCVTZS */
    case 0x19: /* FCVTZU */
        itof = false;
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    handle_fpfpcvt(s, rd, rn, opcode, itof, FPROUNDING_ZERO, scale, sf, type);
}



/* Floating-point data-processing (2 source) - single precision */
arminst handle_fp_2src_single(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int opcode,
                                  int rd, int rn, int rm)
{
//    TCGv_i32 tcg_op1;
//    TCGv_i32 tcg_op2;
//    TCGv_i32 tcg_res;
//    TCGv_ptr fpst;

//    tcg_res = tcg_temp_new_i32();
//    fpst = get_fpstatus_ptr();
//    tcg_op1 = read_fp_sreg(s, rn);
//    tcg_op2 = read_fp_sreg(s, rm);

    switch (opcode) {
    case 0x0: /* FMUL */
//        gen_helper_vfp_muls(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x1: /* FDIV */
//        gen_helper_vfp_divs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x2: /* FADD */
//        gen_helper_vfp_adds(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x3: /* FSUB */
//        gen_helper_vfp_subs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x4: /* FMAX */
//        gen_helper_vfp_maxs(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x5: /* FMIN */
//        gen_helper_vfp_mins(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x6: /* FMAXNM */
//        gen_helper_vfp_maxnums(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x7: /* FMINNM */
//        gen_helper_vfp_minnums(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x8: /* FNMUL */
//        gen_helper_vfp_muls(tcg_res, tcg_op1, tcg_op2, fpst);
//        gen_helper_vfp_negs(tcg_res, tcg_res);
        break;
    }

//    write_fp_sreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i32(tcg_op1);
//    tcg_temp_free_i32(tcg_op2);
//    tcg_temp_free_i32(tcg_res);
}

/* Floating-point data-processing (2 source) - double precision */
arminst handle_fp_2src_double(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int opcode,
                                  int rd, int rn, int rm)
{
//    TCGv_i64 tcg_op1;
//    TCGv_i64 tcg_op2;
//    TCGv_i64 tcg_res;
//    TCGv_ptr fpst;

//    tcg_res = tcg_temp_new_i64();
//    fpst = get_fpstatus_ptr();
//    tcg_op1 = read_fp_dreg(s, rn);
//    tcg_op2 = read_fp_dreg(s, rm);

    switch (opcode) {
    case 0x0: /* FMUL */
//        gen_helper_vfp_muld(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x1: /* FDIV */
//        gen_helper_vfp_divd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x2: /* FADD */
//        gen_helper_vfp_addd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x3: /* FSUB */
//        gen_helper_vfp_subd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x4: /* FMAX */
//        gen_helper_vfp_maxd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x5: /* FMIN */
//        gen_helper_vfp_mind(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x6: /* FMAXNM */
//        gen_helper_vfp_maxnumd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x7: /* FMINNM */
//        gen_helper_vfp_minnumd(tcg_res, tcg_op1, tcg_op2, fpst);
        break;
    case 0x8: /* FNMUL */
//        gen_helper_vfp_muld(tcg_res, tcg_op1, tcg_op2, fpst);
//        gen_helper_vfp_negd(tcg_res, tcg_res);
        break;
    }

//    write_fp_dreg(s, rd, tcg_res);

//    tcg_temp_free_ptr(fpst);
//    tcg_temp_free_i64(tcg_op1);
//    tcg_temp_free_i64(tcg_op2);
//    tcg_temp_free_i64(tcg_res);
}




/* Floating point data-processing (1 source)
 *   31  30  29 28       24 23  22  21 20    15 14       10 9    5 4    0
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 | opcode | 1 0 0 0 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+--------+-----------+------+------+
 */
arminst disas_fp_1src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int opcode = extract32(aFetchedOpcode.theOpcode, 15, 6);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    switch (opcode) {
    case 0x4: case 0x5: case 0x7:
    {
        /* FCVT between half, single and double precision */
        int dtype = extract32(opcode, 0, 2);
        if (type == 2 || dtype == type) {
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            return;
        }
//        if (!fp_access_check(s)) {
//            return;
//        }

//        handle_fp_fcvt(s, opcode, rd, rn, dtype, type);
        break;
    }
    case 0x0 ... 0x3:
    case 0x8 ... 0xc:
    case 0xe ... 0xf:
        /* 32-to-32 and 64-to-64 ops */
        switch (type) {
        case 0:
//            if (!fp_access_check(s)) {
//                return;
//            }

//            handle_fp_1src_single(s, opcode, rd, rn);
            break;
        case 1:
//            if (!fp_access_check(s)) {
//                return;
//            }

//            handle_fp_1src_double(s, opcode, rd, rn);
//            break;
        default:
            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
        break;
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* Floating point compare
 *   31  30  29 28       24 23  22  21 20  16 15 14 13  10    9    5 4     0
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | op  | 1 0 0 0 |  Rn  |  op2  |
 * +---+---+---+-----------+------+---+------+-----+---------+------+-------+
 */
arminst disas_fp_compare(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int mos, type, rm, op, rn, opc, op2r;

    mos = extract32(aFetchedOpcode.theOpcode, 29, 3);
    type = extract32(aFetchedOpcode.theOpcode, 22, 2); /* 0 = single, 1 = double */
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    op = extract32(aFetchedOpcode.theOpcode, 14, 2);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    opc = extract32(aFetchedOpcode.theOpcode, 3, 2);
    op2r = extract32(aFetchedOpcode.theOpcode, 0, 3);

    if (mos || op || op2r || type > 1) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

//    handle_fp_compare(s, type, rn, rm, opc & 1, opc & 2);
}

/* Floating point data-processing (2 source)
 *   31  30  29 28       24 23  22  21 20  16 15    12 11 10 9    5 4    0
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |  Rm  | opcode | 1 0 |  Rn  |  Rd  |
 * +---+---+---+-----------+------+---+------+--------+-----+------+------+
 */
arminst disas_fp_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    int opcode = extract32(aFetchedOpcode.theOpcode, 12, 4);

    if (opcode > 8) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (type) {
    case 0:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_2src_single(aFetchedOpcode,  aCPU, aSequenceNo, opcode, rd, rn, rm);
        break;
    case 1:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_2src_double(aFetchedOpcode, aCPU, aSequenceNo, opcode, rd, rn, rm);
        break;
    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}



/* Floating point data-processing (3 source)
 *   31  30  29 28       24 23  22  21  20  16  15  14  10 9    5 4    0
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 * | M | 0 | S | 1 1 1 1 1 | type | o1 |  Rm  | o0 |  Ra  |  Rn  |  Rd  |
 * +---+---+---+-----------+------+----+------+----+------+------+------+
 */
arminst disas_fp_3src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
    int type = extract32(aFetchedOpcode.theOpcode, 22, 2);
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    int ra = extract32(aFetchedOpcode.theOpcode, 10, 5);
    int rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    bool o1 = extract32(aFetchedOpcode.theOpcode, 21, 1);

    switch (type) {
    case 0:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_3src_single(o0, o1, rd, rn, rm, ra);
        break;
    case 1:
//        if (!fp_access_check(s)) {
//            return;
//        }
        handle_fp_3src_double(o0, o1, rd, rn, rm, ra);
        break;
    default:
       return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* Floating point immediate
 *   31  30  29 28       24 23  22  21 20        13 12   10 9    5 4    0
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 * | M | 0 | S | 1 1 1 1 0 | type | 1 |    imm8    | 1 0 0 | imm5 |  Rd  |
 * +---+---+---+-----------+------+---+------------+-------+------+------+
 */
arminst disas_fp_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    int rd = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int imm8 = extract32(aFetchedOpcode.theOpcode, 13, 8);
    int is_double = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint64_t imm;
//    TCGv_i64 tcg_res;

    if (is_double > 1) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

//    if (!fp_access_check(s)) {
//        return;
//    }

    /* The imm8 encodes the sign bit, enough bits to represent
     * an exponent in the range 01....1xx to 10....0xx,
     * and the most significant 4 bits of the mantissa; see
     * VFPExpandImm() in the v8 ARM ARM.
     */
    if (is_double) {
        imm = (extract32(imm8, 7, 1) ? 0x8000 : 0) |
            (extract32(imm8, 6, 1) ? 0x3fc0 : 0x4000) |
            extract32(imm8, 0, 6);
        imm <<= 48;
    } else {
        imm = (extract32(imm8, 7, 1) ? 0x8000 : 0) |
            (extract32(imm8, 6, 1) ? 0x3e00 : 0x4000) |
            (extract32(imm8, 0, 6) << 3);
        imm <<= 16;
    }

//    tcg_res = tcg_const_i64(imm);
//    write_fp_dreg(s, rd, tcg_res);
//    tcg_temp_free_i64(tcg_res);
}


/* FP-specific subcases of table C3-6 (SIMD and FP data processing)
 *   31  30  29 28     25 24                          0
 * +---+---+---+---------+-----------------------------+
 * |   | 0 |   | 1 1 1 1 |                             |
 * +---+---+---+---------+-----------------------------+
 */
arminst disas_data_proc_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    assert(false);
    if (extract32(aFetchedOpcode.theOpcode, 24, 1)) {
        /* Floating point data-processing (3 source) */
        disas_fp_3src(aFetchedOpcode, aCPU, aSequenceNo);
    } else if (extract32(aFetchedOpcode.theOpcode, 21, 1) == 0) {
        /* Floating point to fixed point conversions */
        disas_fp_fixed_conv(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        switch (extract32(aFetchedOpcode.theOpcode, 10, 2)) {
        case 1:
            /* Floating point conditional compare */
            disas_fp_ccomp(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 2:
            /* Floating point data-processing (2 source) */
            disas_fp_2src(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 3:
            /* Floating point conditional select */
            disas_fp_csel(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        case 0:
            switch (ctz32(extract32(aFetchedOpcode.theOpcode, 12, 4))) {
            case 0: /* [15:12] == xxx1 */
                /* Floating point immediate */
                disas_fp_imm(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 1: /* [15:12] == xx10 */
                /* Floating point compare */
                disas_fp_compare(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 2: /* [15:12] == x100 */
                /* Floating point data-processing (1 source) */
                disas_fp_1src(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            case 3: /* [15:12] == 1000 */
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            default: /* [15:12] == 0000 */
                /* Floating point <-> integer conversions */
                return disas_fp_int_conv(aFetchedOpcode, aCPU, aSequenceNo);
                break;
            }
            break;
        }
    }
}

arminst disas_data_proc_simd(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    /* Note that this is called with all non-FP cases from
     * table C3-6 so it must UNDEF for entries not specifically
     * allocated to instructions in that table.
     */
//    AArch64DecodeFn *fn = lookup_disas_fn(&data_proc_simd[0]);
//    if (fn) {
//        fn(s);
//    } else {
//        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
//    }
}

/* C3.6 Data processing - SIMD and floating point */
arminst disas_data_proc_simd_fp(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (extract32(aFetchedOpcode.theOpcode, 28, 1) == 1 && extract32(aFetchedOpcode.theOpcode, 30, 1) == 0) {
        return disas_data_proc_fp(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        /* SIMD, including crypto */
        return disas_data_proc_simd(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* Data-processing (2 source)
 *   31   30  29 28             21 20  16 15    10 9    5 4    0
 * +----+---+---+-----------------+------+--------+------+------+
 * | sf | 0 | S | 1 1 0 1 0 1 1 0 |  Rm  | opcode |  Rn  |  Rd  |
 * +----+---+---+-----------------+------+--------+------+------+
 */
arminst disas_data_proc_2src(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned int sf, rm, opcode, rn, rd;
    sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    opcode = extract32(aFetchedOpcode.theOpcode, 10, 6);
    rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    rd = extract32(aFetchedOpcode.theOpcode, 0, 5);

    if (extract32(aFetchedOpcode.theOpcode, 29, 1)) {
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        return;
    }

    switch (opcode) {
    case 2: /* UDIV */
        handle_div(aFetchedOpcode, aCPU, aSequenceNo, false, sf, rm, rn, rd);
        break;
    case 3: /* SDIV */
        handle_div(aFetchedOpcode, aCPU, aSequenceNo, true, sf, rm, rn, rd);
        break;
    case 8: /* LSLV */
        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_LSL, sf, rm, rn, rd);
        break;
    case 9: /* LSRV */
        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_LSR, sf, rm, rn, rd);
        break;
    case 10: /* ASRV */
        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_ASR, sf, rm, rn, rd);
        break;
    case 11: /* RORV */
        handle_shift_reg(aFetchedOpcode, aCPU, aSequenceNo, A64_SHIFT_TYPE_ROR, sf, rm, rn, rd);
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23: /* CRC32 */
    {
        int sz = extract32(opcode, 0, 2);
        bool crc32c = extract32(opcode, 2, 1);
        handle_crc32(aFetchedOpcode, aCPU, aSequenceNo, sf, sz, crc32c, rm, rn, rd);
        break;
    }
    default:
        unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    }
}

/* Data processing - register */
arminst disas_data_proc_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    switch (extract32(aFetchedOpcode.theOpcode, 24, 5))
    {
        case 0x0a: /* Logical (shifted register) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_logic_reg  \033[0m"));
            return disas_logic_reg(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x0b: /* Add/subtract */
            if (aFetchedOpcode.theOpcode & (1 << 21)) { /* (extended register) */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_add_sub_ext_reg  \033[0m"));
                return disas_add_sub_ext_reg(aFetchedOpcode, aCPU, aSequenceNo);
            } else {
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: disas_add_sub_reg  \033[0m"));
                return disas_add_sub_reg(aFetchedOpcode, aCPU, aSequenceNo);
            }
            break;
        case 0x1b: /* Data-processing (3 source) */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register: Data-processing (3 source)  \033[0m"));
            return disas_data_proc_3src(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x1a:
            switch (extract32(aFetchedOpcode.theOpcode, 21, 3))
            {
                case 0x0: /* Add/subtract (with carry) */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Add/subtract (with carry)  \033[0m"));
                    return disas_adc_sbc(aFetchedOpcode, aCPU, aSequenceNo);
                case 0x2: /* Conditional compare */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Conditional compare  \033[0m"));
                    return disas_cc(aFetchedOpcode, aCPU, aSequenceNo); /* both imm and reg forms */
                case 0x4: /* Conditional select */
                DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - Conditional select  \033[0m"));
                    return disas_cond_select(aFetchedOpcode, aCPU, aSequenceNo);
                case 0x6: /* Data-processing */
                    if (aFetchedOpcode.theOpcode & (1 << 30)) { /* (1 source) */
                        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - (1 source)  \033[0m"));
                        return disas_data_proc_1src(aFetchedOpcode, aCPU, aSequenceNo);
                    } else {            /* (2 source) */
                        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - (2 source)   \033[0m"));
                        return disas_data_proc_2src(aFetchedOpcode, aCPU, aSequenceNo);
                    }
                    break;
                default:
                    return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
        default:
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* C3.1 A64 instruction index by encoding */
std::pair< boost::intrusive_ptr<AbstractInstruction>, bool> decode( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop )
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Decoding " << std::hex << aFetchedOpcode.theOpcode << std::dec << "\033[0m"));
//    DBG_( Tmp, ( << *this << " setWillRaise: 0x" << std::hex << aSetting << std::dec ) ) ;//NOOSHIN

    boost::intrusive_ptr<AbstractInstruction> ret_val;
    int r = extract32(aFetchedOpcode.theOpcode, 25, 4);
    bool last_uop = true;

    switch (extract32(aFetchedOpcode.theOpcode, 25, 4)) {
    case 0x0: case 0x1: case 0x2: case 0x3: /* UNALLOCATED */
        DBG_(Tmp,(<< "\033[1;31m DECODER: UNALLOCATED \033[0m"));
        ret_val = unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 0x8: case 0x9: /* Data processing - immediate */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - immediate \033[0m"));
        ret_val = disas_data_proc_imm(aFetchedOpcode,  aCPU, aSequenceNo);
        break;
    case 0xa: case 0xb: /* Branch, exception generation and system insns */
         DBG_(Tmp,(<< "\033[1;31m DECODER: Branch, exception generation and system insns \033[0m"));
         ret_val =disas_b_exc_sys(aFetchedOpcode, aCPU, aSequenceNo);
        break;
    case 0x4:
    case 0x6:
    case 0xc:
    case 0xe:      /* Loads and stores */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Loads and stores \033[0m"));
        ret_val = disas_ldst(aFetchedOpcode,  aCPU, aSequenceNo);
        break;
    case 0x5:
    case 0xd:      /* Data processing - register */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - register  \033[0m"));
        ret_val = disas_data_proc_reg(aFetchedOpcode,  aCPU, aSequenceNo);
        break;
    case 0x7:
    case 0xf:      /* Data processing - SIMD and floating point */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Data processing - SIMD and floating point \033[0m"));
        ret_val = disas_data_proc_simd_fp(aFetchedOpcode,  aCPU, aSequenceNo);
        break;
    default:
        DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should be handled above */
        break;
    }

      return std::make_pair(ret_val, last_uop);
}

bool armInstruction::usesIntAlu() const {
  return theUsesIntAlu;
}

bool armInstruction::usesIntMult() const {
  return theUsesIntMult;
}

bool armInstruction::usesIntDiv() const {
  return theUsesIntDiv;
}

bool armInstruction::usesFpAdd() const {
  return theUsesFpAdd;
}

bool armInstruction::usesFpCmp() const {
  return theUsesFpCmp;
}

bool armInstruction::usesFpCvt() const {
  return theUsesFpCvt;
}

bool armInstruction::usesFpMult() const {
  return theUsesFpMult;
}

bool armInstruction::usesFpDiv() const {
  return theUsesFpDiv;
}

bool armInstruction::usesFpSqrt() const {
  return theUsesFpSqrt;
}




} //narmDecoder
