#include <list>
#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/none.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/simics/mai_api.hpp>
#include <core/stats.hpp>

#include "v9Instruction.hpp"
#include "SemanticInstruction.hpp"
#include "SemanticActions.hpp"
#include "Effects.hpp"
#include "Validations.hpp"
#include "Constraints.hpp"
#include "Interactions.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

namespace Stat = Flexus::Stat;

using nuArch::rRegisters;
using nuArch::ccBits;
using namespace nuArch;

void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances) {
  std::list< InternalDependance >::iterator iter, end;
  for ( iter = dependances.begin(), end = dependances.end(); iter != end; ++iter) {
    inst->addDispatchEffect( satisfy( inst, *iter ) );
  }
}

void v9Instruction::describe(std::ostream & anOstream) const {
  Flexus::Simics::Processor cpu = Flexus::Simics::Processor::getProcessor(theCPU);
  anOstream << "#" << theSequenceNo << "[" << std::setfill('0') << std::right << std::setw(2) << cpu->id() <<  "] "
            << "@" << thePC  << " |" << std::hex << std::setw(8) << theOpcode << std::dec << "| "
            << std::left << std::setw(30) << std::setfill(' ') << disassemble();
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

std::string v9Instruction::disassemble() const {
  return Flexus::Simics::Processor::getProcessor(theCPU)->disassemble(thePC);
}

void v9Instruction::setWillRaise(int32_t aSetting) {
  DBG_( Verb, ( << *this << " setWillRaise: 0x" << std::hex << aSetting << std::dec ) ) ;
  theWillRaise = aSetting;
}

void v9Instruction::doDispatchEffects(  ) {
  if (bpState() && ! isBranch()) {
    //Branch predictor identified an instruction that is not a branch as a
    //branch.
    DBG_( Verb, ( << *this << " predicted as a branch, but is a non-branch.  Fixing" ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = pc();
    feedback->theActualType = kNonBranch;
    feedback->theActualDirection = kNotTaken;
    feedback->theActualTarget = VirtualMemoryAddress(0);
    feedback->theBPState = bpState();
    core()->branchFeedback(feedback);
    core()->applyToNext( boost::intrusive_ptr< nuArch::Instruction >( this ) , new BranchInteraction(VirtualMemoryAddress(0)) );

  }
}

struct BlackBoxInstruction : public v9Instruction {

  BlackBoxInstruction(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, uint32_t  aCPU, int64_t aSequenceNo)
    : v9Instruction(aPC, aNextPC, anOpcode, aBPState, aCPU, aSequenceNo) {
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
    v9Instruction::describe(anOstream);
    anOstream << instCode();
  }
};

struct MAGIC : public v9Instruction {
  MAGIC(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, uint32_t aCPU, int64_t aSequenceNo)
    : v9Instruction(aPC, aNextPC, anOpcode, aBPState, aCPU, aSequenceNo) {
    setClass(clsSynchronizing, codeMAGIC);

  }

  virtual bool mayRetire() const {
    return true;
  }
  virtual bool preValidate() {
    Flexus::Simics::Processor cpu = Flexus::Simics::Processor::getProcessor(theCPU);
    if ( cpu->getPC()  != thePC ) {
      DBG_( Crit, ( << *this << " PreValidation failed: PC mismatch flexus=" << thePC << " simics=" << cpu->getPC() ) );
    }
    if ( cpu->getNPC()  != theNPC ) {
      DBG_( Crit, ( << *this << " PreValidation failed: NPC mismatch flexus=" << theNPC << " simics=" << cpu->getNPC() ) );
    }
    return
      (   cpu->getPC()     == thePC
          &&  cpu->getNPC() == theNPC
      );
  }

  virtual bool postValidate() {
    Flexus::Simics::Processor cpu = Flexus::Simics::Processor::getProcessor(theCPU);
    if ( cpu->getPC()  != theNPC ) {
      DBG_( Crit, ( << *this << " PostValidation failed: PC mismatch flexus=" << theNPC << " simics=" << cpu->getPC() ) );
    }
    return ( cpu->getPC()  == theNPC ) ;
  }

  virtual void describe(std::ostream & anOstream) const {
    v9Instruction::describe(anOstream);
    anOstream << "MAGIC";
  }
};

boost::intrusive_ptr<v9Instruction> blackBox( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  return boost::intrusive_ptr<v9Instruction>( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

boost::intrusive_ptr<v9Instruction> grayBox( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, eInstructionCode aCode ) {
  boost::intrusive_ptr<v9Instruction> inst( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->forceResync();
  inst->setClass(clsSynchronizing, aCode);
  return inst;
}

boost::intrusive_ptr<v9Instruction> immu_exception( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->setClass(clsSynchronizing, codeITLBMiss );
  inst->addRetirementEffect( immuException(inst) );
  return boost::intrusive_ptr<v9Instruction>( inst );
}

boost::intrusive_ptr<v9Instruction> nop( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeNOP);
  return boost::intrusive_ptr<v9Instruction>( inst );
}

boost::intrusive_ptr<v9Instruction> magic( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  return boost::intrusive_ptr<v9Instruction>( new MAGIC(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

class Format3 {
  uint32_t opcode;
public:
  Format3( uint32_t op)
    : opcode(op)
  {}
  uint32_t rd() const {
    return (opcode >> 25) & 0x1F;  // opcode[29:25]
  }
  uint32_t rs1() const {
    return (opcode >> 14) & 0x1F;  // opcode[19:14]
  }
  bool is_simm13() const {
    return (opcode >> 13) & 1;  // opcode[13]
  }
  uint32_t rs2() const {
    return opcode & 0x1F;  // opcode[4:0]
  }
  uint32_t simm13() const {
    return opcode & 0x1FFF;  // opcode[12:0]
  }
  uint32_t simm11() const {
    return opcode & 0x07FF;  // opcode[10:0]
  }
  uint32_t simm10() const {
    return opcode & 0x03FF;  // opcode[9:0]
  }
  uint32_t shcnt64() const {
    return opcode & 0x3F;  // opcode[5:0]
  }
  uint32_t asi() const {
    return ( opcode >> 5) & 0xFF;  // opcode[12:5]
  }
  bool i() const {
    return (opcode >> 13) & 1;  // opcode[13]
  }
  bool x() const {
    return (opcode >> 12) & 1;  // opcode[12]
  }
  uint32_t cond() const {
    return (opcode >> 25) & 0xF;  // opcode[28:25]
  }
  bool cc2() const {
    return (opcode >> 18) & 1;  // opcode[18]
  }
  bool cc1() const {
    return (opcode >> 12) & 1;  // opcode[12]
  }
  bool cc0() const {
    return (opcode >> 11) & 1;  // opcode[11]
  }
  uint32_t fcc() const {
    return (opcode >> 11) & 3;  // opcode[12:11]
  }
};

void setRD( SemanticInstruction * inst, uint32_t rd) {
  unmapped_reg regRD;
  regRD.theType = rRegisters;
  regRD.theIndex = rd;
  inst->setOperand( kRD, regRD );
}

void setRD1( SemanticInstruction * inst, uint32_t rd1) {
  unmapped_reg regRD1;
  regRD1.theType = rRegisters;
  regRD1.theIndex = rd1;
  inst->setOperand( kRD1, regRD1 );
}

void setRS( SemanticInstruction * inst, eOperandCode rs_code, uint32_t rs) {
  unmapped_reg regRS;
  regRS.theType = rRegisters;
  regRS.theIndex = rs;
  inst->setOperand(rs_code, regRS);
}

void setCC( SemanticInstruction * inst, int32_t ccNum, eOperandCode rs_code) {
  unmapped_reg regICC;
  regICC.theType = ccBits;
  regICC.theIndex = ccNum;
  inst->setOperand(rs_code, regICC);
}

void setFD( SemanticInstruction * inst, eOperandCode fd_no, uint32_t fd) {
  unmapped_reg regFD;
  regFD.theType = fRegisters;
  regFD.theIndex = fd;
  inst->setOperand( fd_no, regFD );
}

void setFS( SemanticInstruction * inst, eOperandCode fs_no, uint32_t fs) {
  unmapped_reg regFS;
  regFS.theType = fRegisters;
  regFS.theIndex = fs;
  inst->setOperand(fs_no, regFS);
}

void setCCd( SemanticInstruction * inst) {
  unmapped_reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 0;
  inst->setOperand( kCCd, regCCd );
}

void setFCCd( int32_t fcc, SemanticInstruction * inst) {
  unmapped_reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 1 + fcc;
  inst->setOperand( kCCd, regCCd );
}

void addReadRRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances) {
  //Calculate operand codes from anOpNumber
  DBG_Assert( anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 || anOpNumber == 5 );
  eOperandCode cOperand = eOperandCode( kOperand1 + anOpNumber - 1);
  eOperandCode cRS = eOperandCode( kRS1 + anOpNumber - 1);
  eOperandCode cPS = eOperandCode( kPS1 + anOpNumber - 1);

  if (rs == 0 ) {
    //Handle the r0 source case - there is no need to read a register at all.
    inst->setOperand( cOperand, static_cast<uint64_t>(0) );
    satisfyAtDispatch( inst, dependances );

  } else {
    //Set up the mapping and readRegister actions.  Make the execute action
    //depend on reading the register value
    setRS( inst, cRS , rs );

    inst->addDispatchEffect( mapSource( inst, cRS, cPS ) );
    simple_action act = readRegisterAction( inst, cPS, cOperand );
    connect( dependances, act );
    inst->addDispatchAction( act );

    inst->addPrevalidation( validateRegister( rs, cOperand, inst ) );
  }
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

void addReadFRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, eSize aSize, std::vector< std::list<InternalDependance> > & dependances) {
  //Calculate operand codes from anOpNumber
  eOperandCode cFOperand_0, cFOperand_1, cFS_0, cFS_1, cPFS_0, cPFS_1;
  int32_t dep_idx = 0;
  if (anOpNumber == 1) {
    cFOperand_0 = kFOperand1_0;
    cFOperand_1 = kFOperand1_1;
    cFS_0 = kFS1_0;
    cFS_1 = kFS1_1;
    cPFS_0 = kPFS1_0;
    cPFS_1 = kPFS1_1;
    dep_idx = 0;
  } else if (anOpNumber == 2) {
    cFOperand_0 = kFOperand2_0;
    cFOperand_1 = kFOperand2_1;
    cFS_0 = kFS2_0;
    cFS_1 = kFS2_1;
    cPFS_0 = kPFS2_0;
    cPFS_1 = kPFS2_1;
    if (aSize == kWord) {
      dep_idx = 1;
    } else {
      dep_idx = 2;
    }
  } else {
    DBG_Assert( false ) ;
    return;
  }

  switch ( aSize ) {
    case kWord: {
      setFS( inst, cFS_0, rs );

      inst->addDispatchEffect( mapSource( inst, cFS_0, cPFS_0 ) );
      simple_action act = readRegisterAction( inst, cPFS_0, cFOperand_0 );
      connect( dependances[dep_idx], act );
      inst->addDispatchAction( act );

      inst->addPrevalidation( validateFRegister( rs, cFOperand_0, inst ) );

      break;
    }
    case kDoubleWord: {
      //Move fd[0] to fd[5]
      boost::dynamic_bitset<> fsb(6, rs);
      fsb[5] = fsb[0];
      fsb[0] = 0;
      int32_t actual_fs = fsb.to_ulong();

      setFS( inst, cFS_0, actual_fs );
      setFS( inst, cFS_1, actual_fs | 1 );

      inst->addDispatchEffect( mapSource( inst, cFS_0, cPFS_0 ) );
      inst->addDispatchEffect( mapSource( inst, cFS_1, cPFS_1 ) );
      simple_action act0 = readRegisterAction( inst, cPFS_0, cFOperand_0 );
      simple_action act1 = readRegisterAction( inst, cPFS_1, cFOperand_1 );
      connect( dependances[dep_idx], act0 );
      connect( dependances[dep_idx+1], act1 );
      inst->addDispatchAction( act0 );
      inst->addDispatchAction( act1 );

      inst->addPrevalidation( validateFRegister( actual_fs, cFOperand_0, inst ) );
      inst->addPrevalidation( validateFRegister( actual_fs | 1, cFOperand_1, inst ) );
      break;
    }
    default:
      DBG_Assert(false);
  }
}

void addSimm11( SemanticInstruction * inst, uint32_t aConstant, eOperandCode anOperand, std::list<InternalDependance> & rs_deps) {
  uint64_t operand(aConstant);
  //Mask off high bits
  operand &= 0x7FFULL;
  //sign extend
  if (operand & 0x400ULL) {
    operand |= 0xFFFFFFFFFFFFF800ULL;
  }
  satisfyAtDispatch( inst, rs_deps );
  inst->setOperand( anOperand, operand );
}

void addSimm10( SemanticInstruction * inst, uint32_t aConstant, eOperandCode anOperand, std::list<InternalDependance> & rs_deps) {
  uint64_t operand(aConstant);
  //Mask off high bits
  operand &= 0x3FFULL;
  //sign extend
  if (operand & 0x200ULL) {
    operand |= 0xFFFFFFFFFFFFFC00ULL;
  }

  satisfyAtDispatch( inst, rs_deps );
  inst->setOperand( anOperand, operand );
}

void addSimm13( SemanticInstruction * inst, uint32_t aConstant, eOperandCode anOperand, std::list<InternalDependance> & rs_deps) {
  uint64_t operand(aConstant);
  //Mask off high bits
  operand &= 0x1FFFULL;
  //sign extend
  if (operand & 0x1000ULL) {
    operand |= 0xFFFFFFFFFFFFE000ULL;
  }

  satisfyAtDispatch( inst, rs_deps );
  inst->setOperand( anOperand, operand );
}

void format3Operands( SemanticInstruction * inst, Format3 const & operands, std::vector<std::list<InternalDependance> > & rs_deps) {
  DBG_Assert( ! rs_deps[0].empty() , ( << "rs_deps empty in format3Operands - format3operands call must occur after execute action is constructed." ) );
  addReadRRegister( inst, 1, operands.rs1(), rs_deps[0] );
  if (operands.is_simm13()) {
    addSimm13( inst, operands.simm13(), kOperand2, rs_deps[1] );
  } else {
    addReadRRegister( inst, 2, operands.rs2(), rs_deps[1] );
  }
}

predicated_action addExecute_XTRA( SemanticInstruction * inst, Operation & anOperation, uint32_t rd, std::vector< std::list<InternalDependance> > & rs_deps, bool write_xtra) {
  predicated_action exec;
  if (rd == 0) {
    exec = executeAction_XTRA( inst, anOperation, rs_deps, boost::none, (write_xtra ? boost::optional<eOperandCode>(kXTRApd) : boost::none));
  } else {
    exec = executeAction_XTRA( inst, anOperation, rs_deps, kPD, (write_xtra ? boost::optional<eOperandCode>(kXTRApd) : boost::none ));
  }
  //inst->addDispatchAction( exec );
  return exec;
}

predicated_action addExecute( SemanticInstruction * inst, Operation & anOperation, uint32_t rd, std::vector< std::list<InternalDependance> > & rs_deps) {
  predicated_action exec;
  if (rd == 0) {
    exec = executeAction( inst, anOperation, rs_deps, kResult, boost::none );
  } else {
    exec = executeAction( inst, anOperation, rs_deps, kResult, kPD );
  }
  //inst->addDispatchAction( exec );
  return exec;
}

predicated_action addFPExecute( SemanticInstruction * inst, Operation & anOperation, int32_t num_ops, eSize dest_size, eSize src_size, uint32_t rd, std::vector< std::list<InternalDependance> > & deps) {
  predicated_action exec = fpExecuteAction( inst, anOperation, num_ops, dest_size, src_size, deps);
  //inst->addDispatchAction( exec );
  return exec;
}

predicated_action addVisOp( SemanticInstruction * inst, Operation & anOperation, std::vector< std::list<InternalDependance> > & deps) {
  predicated_action exec = visOp( inst, anOperation, deps);
  //inst->addDispatchAction( exec );
  return exec;
}

void addAddressCompute_DynamicASI( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps) {
  multiply_dependant_action update_address = updateAddressAction( inst );

  //Read the %asi register
  std::list<InternalDependance> asi_dep;
  asi_dep.push_back(update_address.dependances[1]);
  addReadRRegister( inst, 3, nuArch::kRegASI, asi_dep );

  simple_action exec = calcAddressAction( inst, rs_deps);
  //inst->addDispatchAction( exec );

  connectDependance( update_address.dependances[0], exec);
  connectDependance( inst->retirementDependance(), update_address );
}

void addAddressCompute_ConstantASI( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps, int32_t anASI) {
  multiply_dependant_action update_address = updateAddressAction( inst );
  inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );
  inst->setOperand( kOperand3, anASI );

  simple_action exec = calcAddressAction( inst, rs_deps);
  //inst->addDispatchAction( exec );

  connectDependance( update_address.dependances[0], exec);
  connectDependance( inst->retirementDependance(), update_address );
}

void addAddressCompute( SemanticInstruction * inst, bool alternate_asi, std::vector< std::list<InternalDependance> > & rs_deps, Format3 const & operands) {
  if ( alternate_asi ) {
    if (operands.i()) {
      addAddressCompute_DynamicASI( inst, rs_deps ) ;
    } else {
      addAddressCompute_ConstantASI( inst, rs_deps, operands.asi() ) ;
    }
  } else {
    addAddressCompute_ConstantASI( inst, rs_deps, 0x80 /* default ASI */ ) ;
  }
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

void addRD1Annulment( SemanticInstruction * inst, predicated_action & exec, InternalDependance const & aWritebackDependance) {
  predicated_action annul = annulRD1Action( inst );
  inst->addDispatchAction( annul );

  connectDependance( aWritebackDependance, annul );

  inst->addAnnulmentEffect( satisfy( inst, annul.predicate ) );
  inst->addAnnulmentEffect( squash( inst, exec.predicate ) );
  inst->addReinstatementEffect( squash( inst, annul.predicate ) );
  inst->addReinstatementEffect( satisfy ( inst, exec.predicate ) );
}

void addFloatingAnnulment( SemanticInstruction * inst, int32_t anIndex, predicated_action & exec, InternalDependance const & aWritebackDependance) {
  predicated_action annul = floatingAnnulAction( inst, anIndex );
  //inst->addDispatchAction( annul );

  connectDependance( aWritebackDependance, annul );

  inst->addDispatchEffect( recordFPRS(inst));
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

void addRD1Writeback( SemanticInstruction * inst, predicated_action & exec) {
  inst->addDispatchEffect( mapRD1Destination(inst) );

  //Create the writeback action
  dependant_action wb = writebackRD1Action( inst );
  //inst->addDispatchAction( wb );

  addRD1Annulment( inst, exec, wb.dependance );

  //Make writeback depend on execute, make retirement depend on writeback
  connectDependance( wb.dependance, exec );
  connectDependance( inst->retirementDependance(), wb );
}

void addFloatingWriteback( SemanticInstruction * inst, int32_t anIndex, predicated_action & exec) {
  inst->addDispatchEffect( mapFDestination( inst, anIndex) );

  //Create the writeback action
  dependant_action wb = floatingWritebackAction( inst, anIndex );
  //inst->addDispatchAction( wb );

  addFloatingAnnulment( inst, anIndex, exec, wb.dependance );

  //Make writeback depend on execute, make retirement depend on writeback
  connectDependance( wb.dependance, exec );
  connectDependance( inst->retirementDependance(), wb );
}

void addDoubleFloatingWriteback( SemanticInstruction * inst, predicated_action & exec) {
  inst->addDispatchEffect( mapFDestination( inst, 0) );
  inst->addDispatchEffect( mapFDestination( inst, 1) );

  //Create the writeback action
  dependant_action wb0 = floatingWritebackAction( inst, 0 );
  dependant_action wb1 = floatingWritebackAction( inst, 1 );

  predicated_action annul0 = floatingAnnulAction( inst, 0 );
  predicated_action annul1 = floatingAnnulAction( inst, 1 );

  connectDependance( wb0.dependance, annul0 );
  connectDependance( wb1.dependance, annul1 );

  inst->addDispatchEffect( recordFPRS(inst));
  inst->addAnnulmentEffect( satisfy( inst, annul0.predicate ) );
  inst->addAnnulmentEffect( satisfy( inst, annul1.predicate ) );
  inst->addAnnulmentEffect( squash( inst, exec.predicate ) );

  inst->addReinstatementEffect( squash( inst, annul0.predicate ) );
  inst->addReinstatementEffect( squash( inst, annul1.predicate ) );
  inst->addReinstatementEffect( satisfy ( inst, exec.predicate ) );

  //Make writeback depend on execute, make retirement depend on writeback
  connectDependance( wb0.dependance, exec );
  connectDependance( wb1.dependance, exec );
  connectDependance( inst->retirementDependance(), wb0 );
  connectDependance( inst->retirementDependance(), wb1 );
}

void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool addSquash = true) {
  if (rd != 0) {
    setRD( inst, rd);
    addWriteback( inst, rRegisters, exec, addSquash );
    inst->addPostvalidation( validateRegister( rd, kResult, inst  ) );
    inst->addOverride( overrideRegister( rd, kResult, inst ) );
  } else {
    //No need to write back.  Make retirement depend on execution
    InternalDependance dep( inst->retirementDependance() );
    connectDependance( dep, exec );
  }
}

void addFloatingDestination( SemanticInstruction * inst, uint32_t fd, eSize aSize, predicated_action & exec) {
  switch ( aSize ) {
    case kWord: {
      setFD(inst, kFD0, fd);
      addFloatingWriteback( inst, 0, exec );
      inst->addRetirementEffect( updateFPRS(inst, fd) );
      inst->addPostvalidation( validateFPRS(inst) );
      inst->addPostvalidation( validateFRegister( fd, kfResult0, inst  ) );
      inst->addOverride( overrideFloatSingle( fd, kfResult0, inst ) );
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
      inst->addRetirementEffect( updateFPRS(inst, actual_fd) );
      inst->addPostvalidation( validateFPRS(inst) );
//      addFloatingWriteback( inst, 0, exec );
//      addFloatingWriteback( inst, 1, exec );
      addDoubleFloatingWriteback( inst, exec ); // Use specialized function to avoid squash->satisfy->squash deadlock problem with annulment/reinstatement
      inst->addPostvalidation( validateFRegister( actual_fd, kfResult0, inst  ) );
      inst->addPostvalidation( validateFRegister( actual_fd | 1, kfResult1, inst  ) );
      inst->addOverride( overrideFloatDouble( actual_fd, kfResult0, kfResult1, inst ) );
      break;
    }
    default:
      DBG_Assert(false);
  }
}

void addFCCDestination( SemanticInstruction * inst, uint32_t fccn, predicated_action & exec, bool addSquash = true) {
  setFCCd( fccn, inst );
  addWriteback( inst, ccBits, exec, addSquash );
  inst->addPostvalidation( validateFCC( fccn, kResultCC, inst  ) );
}

boost::optional<eOperandCode> addXTRA( bool write_xtra, uint32_t xtra_reg, SemanticInstruction * inst, predicated_action & exec, bool addSquash = true) {
  if (write_xtra) {

    unmapped_reg reg;
    reg.theType = rRegisters;
    reg.theIndex = xtra_reg;
    inst->setOperand( kXTRAr, reg );
    inst->addDispatchEffect( mapXTRA(inst) );

    dependant_action wb = writeXTRA( inst );
    //inst->addDispatchAction( wb );

    predicated_action annul = annulXTRA( inst );
    //inst->addDispatchAction( annul );

    connectDependance( wb.dependance, annul );
    inst->addAnnulmentEffect( satisfy( inst, annul.predicate ) );
    inst->addAnnulmentEffect( squash( inst, exec.predicate ) );
    inst->addReinstatementEffect( squash( inst, annul.predicate ) );
    inst->addReinstatementEffect( satisfy ( inst, exec.predicate ) );

    //Make writeback depend on execute, make retirement depend on writeback
    connectDependance( wb.dependance, exec );
    connectDependance( inst->retirementDependance(), wb );

    inst->addPostvalidation( validateRegister( xtra_reg, kXTRAout, inst) );
    return kXTRApd;
  }
  return boost::none;
}

boost::optional<predicated_action> addCCcalc( bool write_cc, SemanticInstruction * inst, uint32_t op3, std::vector< std::list<InternalDependance> > & rs_deps ) {
  if (write_cc) {
    predicated_action exec = executeAction( inst, ccCalc(op3 & 0xF), rs_deps, kResultCC, kCCpd  );
    //inst->addDispatchAction( exec );
    return exec;
  }
  return boost::none;
}

void addCCdest( SemanticInstruction * inst, boost::optional<predicated_action> exec) {
  if (exec) {
    setCCd( inst );
    addWriteback( inst, ccBits, *exec );
    inst->addPostvalidation( validateCC( kResultCC, inst  ) );
  }
}

void addReadRD( SemanticInstruction * inst, uint32_t rd) {

  predicated_dependant_action update_value = updateStoreValueAction( inst );
  if (rd == 0 ) {
    //Handle the r0 source case - there is no need to read a register at all.
    inst->setOperand( kResult, static_cast<uint64_t>(0) );
    inst->addDispatchEffect( satisfy(inst, update_value.dependance) );
    inst->addDispatchAction( update_value );
    connectDependance( inst->retirementDependance(), update_value );

  } else {
    //Set up the mapping and readRegister actions.  Make the execute action
    //depend on reading the register value

    setRD( inst, rd);

    inst->addDispatchEffect( mapSource( inst, kRD, kPD ) );
    simple_action read_value = readRegisterAction( inst, kPD, kResult );
    inst->addDispatchAction( read_value );

    connectDependance( update_value.dependance, read_value );
    connectDependance( inst->retirementDependance(), update_value );

    inst->addPrevalidation( validateRegister( rd, kResult, inst ) );
  }

  inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
  inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );

}

void addReadRDs( SemanticInstruction * inst, uint32_t rd) {

  multiply_dependant_action update_value = updateSTDValueAction( inst );
  connectDependance( inst->retirementDependance(), update_value );

  if ( rd == 0 )  {
    inst->setOperand( kResult, static_cast<uint64_t>(0) );
    inst->addDispatchEffect( satisfy( inst, update_value.dependances[0]) );

  } else {
    setRD( inst, rd);
    inst->addDispatchEffect( mapSource( inst, kRD, kPD ) );
    simple_action read_value = readRegisterAction( inst, kPD, kResult );
    inst->addDispatchAction( read_value );
    connectDependance( update_value.dependances[0], read_value );
    inst->addPrevalidation( validateRegister( rd, kResult, inst ) );
  }

  setRD1( inst, rd + 1);
  inst->addDispatchEffect( mapSource( inst, kRD1, kPD1 ) );
  simple_action read_value1 = readRegisterAction( inst, kPD1, kResult1 );
  inst->addDispatchAction( read_value1 );
  connectDependance( update_value.dependances[1], read_value1 );
  inst->addPrevalidation( validateRegister( rd + 1, kResult1, inst ) );

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

SemanticInstruction  * sethi( uint32_t imm22, uint32_t rd, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  DBG_Assert( rd != 0 );
  DBG_Assert( imm22 < 0x400000 );
  inst->setClass(clsComputation, codeSETHI);

  uint64_t operand(0);
  //Mask off high bits
  operand |= (imm22 << 10);

  setRD( inst, rd);

  predicated_action exec = constantAction( inst, operand, kResult, kPD ) ;
  inst->addDispatchAction(exec);

  addWriteback( inst, rRegisters, exec );
  inst->addPostvalidation( validateRegister( rd, kResult, inst  ) );

  return inst;
}

SemanticInstruction  * alu_c( bool write_cc, uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(3);

  boost::optional<predicated_action> cc = addCCcalc( write_cc, inst, op3 & 0x0F, rs_deps );
  predicated_action exec = addExecute( inst, operation(op3 & 0x0F), operands.rd(), rs_deps ) ;

  format3Operands( inst, operands, rs_deps );
  addReadCC( inst, 0 /*icc*/, 3, rs_deps[2] );

  addCCdest( inst, cc );
  addDestination( inst, operands.rd(), exec );

  return inst;
}

SemanticInstruction  * mul( bool write_cc, bool write_y, uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeMul);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  boost::optional<predicated_action> cc = addCCcalc( write_cc, inst, op3, rs_deps );
  predicated_action exec = addExecute_XTRA( inst, operation(op3), operands.rd(), rs_deps, write_y ) ;

  format3Operands( inst, operands, rs_deps );

  addCCdest( inst, cc );
  addXTRA( write_y, nuArch::kRegY, inst, exec );
  addDestination( inst, operands.rd(), exec );

  return inst;
}

SemanticInstruction  * div_y( bool write_cc, uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeDiv);

  std::vector< std::list<InternalDependance> > rs_deps(3);

  boost::optional<predicated_action> cc = addCCcalc( write_cc, inst, op3, rs_deps );
  predicated_action exec = addExecute_XTRA( inst, operation(op3), operands.rd(), rs_deps, false ) ;

  format3Operands( inst, operands, rs_deps );
  addReadRRegister( inst, 3, nuArch::kRegY, rs_deps[2] );

  addCCdest( inst, cc );
  addDestination( inst, operands.rd(), exec);

  return inst;
}

SemanticInstruction  * alu( bool write_cc, uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  //Assumptions:
  DBG_Assert( op3 < 8  || ( op3 >= 16 && op3 < 24) );

  uint32_t alu_op = op3 & 7;
  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  boost::optional<predicated_action> cc = addCCcalc( write_cc, inst, alu_op, rs_deps );
  predicated_action exec = addExecute( inst, operation(alu_op), operands.rd(), rs_deps ) ;

  format3Operands( inst, operands, rs_deps );

  addCCdest( inst, cc );
  addDestination( inst, operands.rd(), exec );

  return inst;
}

SemanticInstruction  * shift( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  //Assumptions:
  DBG_Assert( ( op3 >= 37 && op3 <= 39) );

  uint32_t shift_op = op3 & 3;
  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, shift(shift_op, operands.x()), operands.rd(), rs_deps ) ;

  format3Operands( inst, operands, rs_deps );

  addDestination( inst, operands.rd(), exec );

  return inst;
}

SemanticInstruction  * save_and_restore( bool is_restore, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  if (is_restore) {
    inst->setClass(clsComputation, codeRestore);
  } else {
    inst->setClass(clsComputation, codeSave);
  }

  std::vector< std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, operation(0) /*add*/, operands.rd(), rs_deps ) ;

  format3Operands( inst, operands, rs_deps );

  if ( operands.rd() != 0 ) {
    inst->addSquashEffect( restorePreviousDestination(inst, rRegisters) );
    inst->addSquashEffect( unmapDestination(inst, rRegisters) );
  }

  if (is_restore) {
    inst->addDispatchEffect( restoreWindow(inst) );
    inst->addAnnulmentEffect(  forceResync(inst) );
    inst->addCheckTrapEffect( restoreWindowTrap(inst) );
    inst->addRetirementEffect( restoreWindowPriv(inst) );
    inst->addSquashEffect( saveWindow(inst) );
  } else {
    inst->addDispatchEffect( saveWindow(inst) );
    inst->addAnnulmentEffect(  forceResync(inst) );
    inst->addCheckTrapEffect( saveWindowTrap(inst) );
    inst->addRetirementEffect( saveWindowPriv(inst) );
    inst->addSquashEffect( restoreWindow(inst) );
  }

  inst->addPostvalidation( validateSaveRestore(inst) );

  addDestination( inst, operands.rd(), exec, false );

  return inst;
}

SemanticInstruction  * flushw( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeFLUSHW);

  inst->addCheckTrapEffect( flushWTrap(inst) );

  return inst;
}

boost::intrusive_ptr<v9Instruction>  saved_and_restored( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  if (operands.rd() == 0) {
    inst->setClass(clsComputation, codeSaved);
    inst->addRetirementEffect( savedWindow(inst) );

  } else if (operands.rd() == 1) {
    inst->setClass(clsComputation, codeRestored);
    inst->addRetirementEffect( restoredWindow(inst) );

  } else {
    //Invalid fcn for SAVED/RESTORED
    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
  }

  inst->addPostvalidation( validatePR(inst, 10 /*CANSAVE*/, kocCANSAVE) );
  inst->addPostvalidation( validatePR(inst, 11 /*CANRESTORE*/, kocCANRESTORE) );
  inst->addPostvalidation( validatePR(inst, 12 /*CLEANWIN*/, kocCLEANWIN) );
  inst->addPostvalidation( validatePR(inst, 13 /*OTHERWIN*/, kocOTHERWIN) );

  return inst;
}

boost::intrusive_ptr<v9Instruction>   ldfsr( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );
  inst->setClass(clsLoad, codeLoadFP);

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, false, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );

  predicated_dependant_action load = loadAction( inst, (operands.rd() == 0 ? kWord : kDoubleWord), false, boost::none );

  inst->addDispatchEffect( allocateLoad( inst, (operands.rd() == 0 ? kWord : kDoubleWord), load.dependance ) );
  inst->addCommitEffect( accessMem(inst) );
  inst->addRetirementConstraint( loadMemoryConstraint(inst) );

  connectDependance( inst->retirementDependance(), load );
  inst->addRetirementEffect( writeFSR(inst, (operands.rd() == 0 ? kWord : kDoubleWord)) );
  inst->setHaltDispatch();
  inst->addPostvalidation( validateFSR(inst) );

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction>   stfsr( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  eSize size = (operands.rd() == 0 ? kWord : kDoubleWord);

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsStore, codeStoreFP);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  addAddressCompute( inst, false, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addPrevalidation( validateFSR(inst) );

  inst->addDispatchEffect( allocateStore( inst, size, false ) );
  inst->addCommitEffect( commitStore(inst) );
  inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
  inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

  //Sneaky trick to make the FSR be read immediately prior to retirement
  inst->addCheckTrapEffect( storeFSR(inst, size) );

  return boost::intrusive_ptr<v9Instruction>(inst);
}

std::pair< boost::intrusive_ptr<v9Instruction>, bool>  fp_block_uop( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  eSize size = kDoubleWord;
  eOperation op = kLoad;
  bool alternate_asi = (op3 & 0x10);

  switch (op3 & (~0x10) ) {
    case 0x23:  //LDDF
      size = kDoubleWord;
      break;
    case 0x27:  //STDF
      op = kStore;
      size = kDoubleWord;
      break;
    default:
      return std::make_pair( blackBox( aFetchedOpcode, aCPU, aSequenceNo), true);
      break;
  }
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  if (op == kStore) {
    inst->setClass(clsStore, codeStoreFP);
  } else {
    inst->setClass(clsLoad, codeLoadFP);
  }

  std::vector< std::list<InternalDependance> > rs_deps(2);

  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  if (aUop > 0)  {
    inst->setOperand( kUopAddressOffset, static_cast<uint64_t>(8 * aUop) );
    inst->setIsMicroOp(true);
  }
  format3Operands( inst, operands, rs_deps );

  inst->addAnnulmentEffect(  forceResync(inst) );
  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );

  int32_t uop_rd = ( operands.rd() & ~1 )  | ( ( operands.rd() & 1) << 5); //Move bit 0 to bit 6
  uop_rd += 2 * aUop;
  uop_rd = ( uop_rd & ~0x20) | ( (uop_rd >> 5) & 1); //Move bit 6 back to bit 0

  if (op == kStore) {

    inst->addDispatchEffect( allocateStore( inst, size, true ) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadFValue( inst, uop_rd, size );
  } else if ( op == kLoad ) {

    predicated_dependant_action load;
    if (operands.rd() == 0) {
      load = loadFloatingAction( inst, size, boost::none, boost::none );
    } else {
      load = loadFloatingAction( inst, size, kPFD0, kPFD1);
    }

    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addFloatingDestination( inst, uop_rd, size, load);
  }

  return std::make_pair( boost::intrusive_ptr<v9Instruction>(inst), aUop == 7);

}

boost::intrusive_ptr<v9Instruction>  fp_memory_uop( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  eSize size = kWord;
  eOperation op = kLoad;
  bool alternate_asi = (op3 & 0x10);

  switch (op3 & (~0x10) ) {
    case 0x20:  //LDF
      size = kWord;
      break;
    case 0x23:  //LDDF
      size = kDoubleWord;
      break;
    case 0x24:  //STF
      op = kStore;
      size = kWord;
      break;
    case 0x27:  //STDF
      op = kStore;
      size = kDoubleWord;
      break;
    case 0x21:  //LDFSR - should not get to fp_memory
    case 0x22:  //LDQF
    case 0x26:  //STQF
    case 0x25:  //STFSR - should not get to fp_memory
    default:
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
      break;
  }
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  if (op == kStore) {
    inst->setClass(clsStore, codeStoreFP);
  } else {
    inst->setClass(clsLoad, codeLoadFP);
  }

  std::vector< std::list<InternalDependance> > rs_deps(2);

  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );

  if (op == kStore) {

    inst->addDispatchEffect( allocateStore( inst, size, false ) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadFValue( inst, operands.rd(), size );
  } else if ( op == kLoad ) {

    predicated_dependant_action load;
    if (operands.rd() == 0) {
      load = loadFloatingAction( inst, size, boost::none, boost::none );
    } else {
      load = loadFloatingAction( inst, size, kPFD0, kPFD1);
    }

    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addFloatingDestination( inst, operands.rd(), size, load);
  }

  return boost::intrusive_ptr<v9Instruction>(inst);
}

std::pair< boost::intrusive_ptr<v9Instruction>, bool>   fp_memory( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop ) {
  Format3 operands( aFetchedOpcode.theOpcode );
  if (aUop == 0) {
    bool constant_asi = (op3 & 0x10) && (! operands.i());
    if (constant_asi) {
      switch ( operands.asi()) {
        case 0x70: //ASI_BLK_AUIP
        case 0x71: //ASI_BLK_AUIS
        case 0x78: //ASI_BLK_AUIPL
        case 0x79: //ASI_BLK_AUISL
        case 0xF0: //ASI_BLK_P
        case 0xF1: //ASI_BLK_S
        case 0xF8: //ASI_BLK_PL
        case 0xF9: //ASI_BLK_SL
        case 0xE0: //ASI_BLK_COMMIT_P
        case 0xE1: //ASI_BLK_COMMIT_S
          return fp_block_uop( op3, aFetchedOpcode, aCPU, aSequenceNo, aUop);
        default:
          break;
      }
    }

    //Standard fp_memory operation
    return std::make_pair( fp_memory_uop( op3, aFetchedOpcode, aCPU, aSequenceNo, aUop), true);

  } else {
    return fp_block_uop( op3, aFetchedOpcode, aCPU, aSequenceNo, aUop);
  }

}

boost::intrusive_ptr<v9Instruction> ldd( bool alternate_asi, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  Format3 operands( aFetchedOpcode.theOpcode );
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsLoad, codeLDD);

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  predicated_dependant_action load;
  if (operands.rd() == 0) {
    load = lddAction( inst, boost::none, kPD1 );
  } else {
    load = lddAction( inst, kPD, kPD1 );
  }

  inst->addDispatchEffect( allocateLoad( inst, kDoubleWord, load.dependance ) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addRetirementConstraint( loadMemoryConstraint(inst) );

  if (operands.rd() != 0) {
    setRD( inst, operands.rd());
    addWriteback( inst, rRegisters, load );
    inst->addPostvalidation( validateRegister( operands.rd(), kResult, inst  ) );
    inst->addOverride( overrideRegister( operands.rd(), kResult, inst ) );
  }
  setRD1( inst, operands.rd() + 1);
  addRD1Writeback( inst, load );
  inst->addPostvalidation( validateRegister( operands.rd() + 1, kResult1, inst  ) );
  inst->addOverride( overrideRegister( operands.rd() + 1, kResult1, inst ) );

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction> std( bool alternate_asi, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  Format3 operands( aFetchedOpcode.theOpcode );
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsStore, codeSTD);

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  inst->addDispatchEffect( allocateStore( inst, kDoubleWord, false ) );
  inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
  inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

  addReadRDs( inst, operands.rd() );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( commitStore(inst) );
  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, kWord, inst ) );
  inst->addPostvalidation( validateMemory( kAddress, 4, kOperand3, kResult1, kWord, inst ) );

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction> memory( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  bool alternate_asi = (op3 & 0x10);
  eSize size = kWord;
  eOperation op = kLoad;
  bool sign_extend = false;

  switch (op3 & ~0x10) {
    case 0:  //LDUW
      break;
    case 1:  //LDUB
      size = kByte;
      break;
    case 2:  //LDUH
      size = kHalfWord;
      break;
    case 3:  //LDD
      return ldd( alternate_asi, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 4:  //STW
      op = kStore;
      break;
    case 5:  //STB
      op = kStore;
      size = kByte;
      break;
    case 6:  //STH
      op = kStore;
      size = kHalfWord;
      break;
    case 7:  //STD
      return std( alternate_asi, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 8:  //LDSW
      sign_extend = true;
      break;
    case 9:  //LDSB
      size = kByte;
      sign_extend = true;
      break;
    case 10: //LDSH
      size = kHalfWord;
      sign_extend = true;
      break;
    case 11: //LDX
      size = kDoubleWord;
      break;
    case 12: //invalid opcode
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
    case 13: //LDSTUB
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
    case 14: //STX
      op = kStore;
      size = kDoubleWord;
      break;
    case 15: //SWAP
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
  }

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  if (op == kStore) {
    inst->setClass(clsStore, codeStore);
  } else {
    inst->setClass(clsLoad, codeLoad);
  }

  std::vector< std::list<InternalDependance> > rs_deps(2);

  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;

  format3Operands( inst, operands, rs_deps );

  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addSquashEffect( eraseLSQ(inst) );

  if (op == kStore) {

    inst->addDispatchEffect( allocateStore( inst, size, false ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadRD( inst, operands.rd() );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, size, inst ) );
    inst->addCommitEffect( commitStore(inst) );
  } else if ( op == kLoad ) {

    predicated_dependant_action load;
    if (operands.rd() == 0) {
      load = loadAction( inst, size, sign_extend, boost::none );
    } else {
      load = loadAction( inst, size, sign_extend, kPD );
    }

    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, operands.rd(), load);
  }

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction> cas(  eSize aSize, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsAtomic, codeCAS);

  Format3 operands( aFetchedOpcode.theOpcode );

  //TWENISCH - reordered actions here so allocateCAS preceeds updateCASAddressAction
  //to solve a race on allocating an LSQ entry and updating the address when
  //the address can be computed at dispatch.

  //obtain the loaded value and write it back to the reg file.
  predicated_dependant_action cas;
  if (operands.rd() == 0) {
    cas = casAction( inst, aSize, boost::none );
  } else {
    cas = casAction( inst, aSize, kPD );
  }
  inst->addDispatchEffect( allocateCAS( inst, aSize, cas.dependance) );

  multiply_dependant_action update_address = updateCASAddressAction( inst );

  if (operands.i()) {
    //Read the %asi register
    std::list<InternalDependance> asi_dep;
    asi_dep.push_back(update_address.dependances[1]);
    addReadRRegister( inst, 3, nuArch::kRegASI, asi_dep );
  } else {
    inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );
    inst->setOperand( kOperand3, operands.asi() );
  }

  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addSquashEffect( eraseLSQ(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

  multiply_dependant_action update_value = updateCASValueAction( inst );

  InternalDependance dep( inst->retirementDependance() );
  connectDependance( dep, update_value );

  //Read the value which will be stored
  if (operands.rd() == 0 ) {
    //Handle the r0 source case - there is no need to read a register at all.
    inst->setOperand( kOperand4, static_cast<uint64_t>(0) );
    inst->addDispatchEffect( satisfy( inst, update_value.dependances[0] ) );
  } else {
    //Set up the mapping and readRegister actions.  Make the execute action
    //depend on reading the register value

    std::list<InternalDependance> uv_deps;
    uv_deps.push_back( update_value.dependances[0]  );
    addReadRRegister( inst, 4, operands.rd(), uv_deps );
  }

  //Read the comparison value
  std::list<InternalDependance> cmp_deps;
  cmp_deps.push_back( update_value.dependances[1]  );
  addReadRRegister( inst, 2, operands.rs2(), cmp_deps );

  // address dependencies
  std::list<InternalDependance> addr_dep;
  addr_dep.push_back( update_address.dependances[0] );
  addr_dep.push_back( update_value.dependances[2] );  // re-read the value as well b/c may have lost value on earlier read (w/ diff address)
  addReadRRegister( inst, 1, operands.rs1(), addr_dep );

  addDestination( inst, operands.rd(), cas);

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction> swap( bool alternate_asi, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsAtomic, codeSWAP);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  predicated_dependant_action update_value = updateRMWValueAction( inst );

  //Read the value which will be stored
  if (operands.rd() == 0 ) {
    //Handle the r0 source case - there is no need to read a register at all.
    inst->setOperand( kOperand4, static_cast<uint64_t>(0) );
    inst->addDispatchEffect( satisfy( inst, update_value.dependance ) );
  } else {
    //Set up the mapping and readRegister actions.  Make the execute action
    //depend on reading the register value

    std::list<InternalDependance> uv_dep;
    uv_dep.push_back( update_value.dependance  );
    addReadRRegister( inst, 4, operands.rd(), uv_dep );
  }

  //obtain the loaded value and write it back to the reg file.
  predicated_dependant_action rmw;
  if (operands.rd() == 0) {
    rmw = rmwAction( inst, kWord, boost::none );
  } else {
    rmw = rmwAction( inst, kWord, kPD );
  }
  inst->addDispatchEffect( allocateRMW( inst, kWord, rmw.dependance) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core
  inst->addSquashEffect( eraseLSQ(inst) );
  addDestination( inst, operands.rd(), rmw);

  return boost::intrusive_ptr<v9Instruction>(inst);
}

boost::intrusive_ptr<v9Instruction> ldstub( bool alternate_asi, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsAtomic, codeLDSTUB);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, alternate_asi, rs_deps, operands ) ;
  format3Operands( inst, operands, rs_deps );

  predicated_dependant_action update_value = updateRMWValueAction( inst );
  inst->addDispatchAction( update_value );

  inst->setOperand( kOperand4, static_cast<uint64_t>(0xFF) );
  inst->addDispatchEffect( satisfy( inst, update_value.dependance ) );

  //obtain the loaded value and write it back to the reg file.
  predicated_dependant_action rmw;
  if (operands.rd() == 0) {
    rmw = rmwAction( inst, kByte, boost::none );
  } else {
    rmw = rmwAction( inst, kByte, kPD );
  }
  inst->addDispatchEffect( allocateRMW( inst, kByte, rmw.dependance) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core
  inst->addSquashEffect( eraseLSQ(inst) );
  addDestination( inst, operands.rd(), rmw);

  return boost::intrusive_ptr<v9Instruction>(inst);
}

SemanticInstruction  * annul_next( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeBranchUnconditional);

  inst->addDispatchEffect( annulNext(inst) );
  inst->redirectNPC( aFetchedOpcode.thePC + 8 );

  return inst;
}

SemanticInstruction  * branch_always( bool annul, VirtualMemoryAddress target, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeBranchUnconditional);

  if (annul) {
    inst->addDispatchEffect( branch( inst, target ) );
  } else {
    inst->addDispatchEffect( branchAfterNext( inst, target ) );
  }
  inst->addRetirementEffect( updateUnconditional( inst, target ) );

  return inst;
}

SemanticInstruction  * branch_cc( bool annul, bool useXcc, uint32_t cond, VirtualMemoryAddress target, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  DBG_Assert(cond <= 0xF);

  inst->setClass(clsBranch, codeBranchConditional);

  std::list<InternalDependance>  rs_deps;
  dependant_action br = branchCCAction( inst, target, annul, packCondition(false, useXcc, cond), false) ;
  connectDependance( inst->retirementDependance(), br );
  rs_deps.push_back( br.dependance );

  addReadCC(inst, 0 , 1, rs_deps) ;
  //inst->addDispatchAction( br );

  inst->addRetirementEffect( updateConditional(inst) );

  return inst;
}

SemanticInstruction  * branch_fcc( bool annul, uint32_t fcc, uint32_t cond, VirtualMemoryAddress target, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeBranchFPConditional);

  std::list<InternalDependance>  rs_deps;
  dependant_action br = branchCCAction( inst, target, annul, cond, true) ;
  connectDependance( inst->retirementDependance(), br );
  rs_deps.push_back( br.dependance );

  addReadCC(inst, 1 + fcc , 1, rs_deps) ;
  //inst->addDispatchAction( br );
  inst->addRetirementEffect( updateConditional(inst) );

  return inst;
}

SemanticInstruction  * call( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeCALL);

  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 0 1|                         disp30

  int64_t disp30(opcode & 0x3FFFFFFF);    // opcode[29:0]

  //sign-extend
  if (disp30 & 0x20000000 ) {
    disp30 |=  0xFFFFFFFFC0000000LL;
  }
  //multiply by 4
  disp30 <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp30;
  //sign extend the target;

  VirtualMemoryAddress target(target_ll);

  inst->addDispatchEffect( branchAfterNext( inst, target ) );
  inst->addDispatchEffect( updateCall( inst, target ) );

  predicated_action exec = readPCAction( inst );
  inst->addDispatchAction( exec );

  addDestination( inst, 15 /* return address */ , exec);

  return inst;
}

SemanticInstruction  * return_( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeRETURN);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);

  simple_action tgt = calcAddressAction( inst, rs_deps);
  //inst->addDispatchAction( tgt );
  dependant_action br = branchToCalcAddressAction( inst );

  connectDependance( br.dependance, tgt );
  connectDependance( inst->retirementDependance(), br );

  format3Operands( inst, operands, rs_deps );

  //inst->addRetirementEffect( branchAfterNext( inst, kAddress ) ); - Now done via branchToCalcAddressAction
  inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

  inst->addDispatchEffect( restoreWindow(inst) );
  inst->addCheckTrapEffect( restoreWindowTrap(inst) );
  inst->addAnnulmentEffect(  forceResync(inst) );
  inst->addRetirementEffect( restoreWindowPriv(inst) );
  inst->addSquashEffect( saveWindow(inst) );

  inst->addPostvalidation( validatePR(inst, 9  /*CWP*/, kocCWP) );
  inst->addPostvalidation( validatePR(inst, 10 /*CANSAVE*/, kocCANSAVE) );
  inst->addPostvalidation( validatePR(inst, 11 /*CANRESTORE*/, kocCANRESTORE) );

  return inst;
}

SemanticInstruction  * jmpl( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeBranchUnconditional);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);

  simple_action tgt = calcAddressAction( inst, rs_deps );
  //inst->addDispatchAction( tgt );

  dependant_action br = branchToCalcAddressAction( inst );

  connectDependance( br.dependance, tgt );
  connectDependance( inst->retirementDependance(), br );

  format3Operands( inst, operands, rs_deps );

  //inst->addRetirementEffect( branchAfterNext( inst, kAddress ) ); - Now done via branchToCalcAddressAction
  inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

  predicated_action exec = readPCAction( inst );
  inst->addDispatchAction( exec );

  addDestination( inst, operands.rd(), exec);

  return inst;
}

SemanticInstruction  * retry( bool isRetry, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsBranch, codeRETRYorDONE);

  inst->addRetirementEffect( returnFromTrap(inst, isRetry) );

  return inst;
}

boost::intrusive_ptr<v9Instruction> fzero( eSize aSize, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeFP);

  inst->setOperand( kFOperand1_0, 0 );
  inst->setOperand( kFOperand1_1, 0 );
  inst->setOperand( kFOperand2_0, 0 );
  inst->setOperand( kFOperand2_1, 0 );

  std::vector< std::list<InternalDependance> > deps(4);
  predicated_action exec = addFPExecute( inst, fp_op1(0x42) /*FADDd*/, 2, aSize, kDoubleWord, operands.rd(), deps ) ;
  satisfyAtDispatch( inst, deps[0] );
  satisfyAtDispatch( inst, deps[1] );
  satisfyAtDispatch( inst, deps[2] );
  satisfyAtDispatch( inst, deps[3] );

  addFloatingDestination( inst, operands.rd(), aSize, exec);

  return inst;
}

boost::intrusive_ptr<v9Instruction> fsrc( int32_t anRs, eSize aSize, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeFP);

  std::vector< std::list<InternalDependance> > deps;

  predicated_action exec = addFPExecute( inst, fp_op1( (aSize == kDoubleWord ? 0x42 /* FADDd */ : 0x41 /* FADDs */ )), 2, aSize, aSize, operands.rd(), deps ) ;

  if (anRs == 1) {
    addReadFRegister( inst, 1, operands.rs1(), aSize, deps );
  } else {
    addReadFRegister( inst, 1, operands.rs2(), aSize, deps );
  }
  inst->setOperand( kFOperand2_0, 0 );
  if (aSize == kDoubleWord) {
    inst->setOperand( kFOperand2_1, 0 );
  }

  if (aSize == kDoubleWord) {
    deps.resize(4);
    satisfyAtDispatch( inst,  deps[2] );
    satisfyAtDispatch( inst, deps[3] );
  } else {
    deps.resize(2);
    satisfyAtDispatch( inst, deps[1] );
  }

  addFloatingDestination( inst, operands.rd(), aSize, exec);

  return inst;
}

boost::intrusive_ptr<v9Instruction> fp_calc( int32_t num_operands, unsigned opf, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  eSize size = kWord;
  if ( (opf & 3) == 2) {
    size = kDoubleWord;
  }

  inst->setClass(clsComputation, codeFP);

  std::vector< std::list<InternalDependance> > deps;

  predicated_action exec = addFPExecute( inst, fp_op1(opf), num_operands, size, size, operands.rd(), deps ) ;

  if (num_operands == 2) {
    addReadFRegister( inst, 1, operands.rs1(), size, deps );
    addReadFRegister( inst, 2, operands.rs2(), size, deps );
  } else {
    addReadFRegister( inst, 1, operands.rs2(), size, deps );
  }

  addFloatingDestination( inst, operands.rd(), size, exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> fsmuld( unsigned opf, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeFP);

  std::vector< std::list<InternalDependance> > deps;

  predicated_action exec = addFPExecute( inst, fp_op1(opf), 2, kDoubleWord, kWord, operands.rd(), deps ) ;

  addReadFRegister( inst, 1, operands.rs1(), kWord, deps );
  addReadFRegister( inst, 2, operands.rs2(), kWord, deps );

  addFloatingDestination( inst, operands.rd(), kDoubleWord, exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> fp_cvt( eSize src_size, eSize dest_size, uint32_t opf, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeFP);
  Format3 operands( aFetchedOpcode.theOpcode );
  std::vector< std::list<InternalDependance> > deps;
  predicated_action exec = addFPExecute( inst, fp_op1(opf), 1, dest_size, src_size, operands.rd(), deps ) ;
  addReadFRegister( inst, 1, operands.rs2(), src_size, deps );
  addFloatingDestination( inst, operands.rd(), dest_size, exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> fp_cmp( eSize size, unsigned opf, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeFP);

  std::vector< std::list<InternalDependance> > deps;
  predicated_action exec = addFPExecute( inst, fp_op2(opf), 2, kByte, size, operands.rd(), deps ) ;

  addReadFRegister( inst, 1, operands.rs1(), size, deps );
  addReadFRegister( inst, 2, operands.rs2(), size, deps );

  addFCCDestination( inst, operands.rd() & 3, exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> faligndata( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeALIGN);

  std::vector< std::list<InternalDependance> > deps;

  predicated_action exec = addVisOp( inst, /*faligndata*/ fp_op2(0xFF), deps ) ;

  addReadFRegister( inst, 1, operands.rs1(), kDoubleWord, deps );
  addReadFRegister( inst, 2, operands.rs2(), kDoubleWord, deps );
  addReadRRegister( inst, 5, nuArch::kRegGSR, deps[4] );

  addFloatingDestination( inst, operands.rd(), kDoubleWord, exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> alignaddr( bool little_endian, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  Format3 operands( aFetchedOpcode.theOpcode );

  inst->setClass(clsComputation, codeALIGN);

  std::vector< std::list<InternalDependance> > rs_deps(3);

  predicated_action exec = addExecute_XTRA( inst, operation( (little_endian ? /*ALIGNADDRESS_LITTLE*/ 0xFC : /*ALIGNADDRESS*/ 0xFB )  ), operands.rd(), rs_deps, true ) ;

  format3Operands( inst, operands, rs_deps );
  addReadRRegister( inst, 3, nuArch::kRegGSR, rs_deps[2] );

  addXTRA( true, nuArch::kRegGSR, inst, exec );
  addDestination( inst, operands.rd(), exec );

  return inst;
}

bool supported_pr( uint32_t aPR) {
  switch (aPR) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:  //CWP
    case 10: //CANSAVE
    case 11: //CANRESTORE
    case 12: //CLEANWIN
    case 13: //OTHERWIN
    case 14: //WSTATE
    case 31: //VER
      return true;
    default:
      return false;
  }
}

bool halt_dispatch_pr( uint32_t aPR) {
  switch (aPR) {
    case 9:  //CWP
      return true;
    default:
      return false;
  }
}

boost::intrusive_ptr<v9Instruction>  rdtick(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );
  predicated_action exec = readTICKAction( inst );
  inst->addDispatchAction( exec );  //Read tick at dispatch to assure monotonicity
  addDestination( inst, operands.rd(), exec );
  inst->overrideSimics();  //Read TICK will not match Simics; ours will invariably be lower because we read early (at dispatch, not retirement). we force Simics to use our value.
  return inst;
}

boost::intrusive_ptr<v9Instruction>  rdpr(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  if (operands.rs1() == 4 /*TICK*/) {
    return rdtick(aFetchedOpcode, aCPU, aSequenceNo);
  }

  //Check for supported PR's
  if (! supported_pr(operands.rs1()) ) {
    return grayBox( aFetchedOpcode, aCPU, aSequenceNo, codeRDPRUnsupported); //resynchronize on all other PRs
  }

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  if ( halt_dispatch_pr(operands.rs1()) ) {
    inst->setHaltDispatch();
  }

  inst->setClass(clsComputation, codeRDPR);

  if (operands.rd() != 0) {
    setRD( inst, operands.rd());
    inst->addDispatchEffect( mapDestination( inst, rRegisters ) );
    inst->addRetirementEffect( readPR(inst, operands.rs1()) );
    inst->addPostvalidation( validateRegister( operands.rd(), kResult, inst  ) );
  }

  return inst;
}

boost::intrusive_ptr<v9Instruction>  wrfprs(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeWRPR);

  std::vector < std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, operation(3) /*xor*/, 0, rs_deps ) ;

  connectDependance( inst->retirementDependance(), exec );

  format3Operands( inst, operands, rs_deps );

  inst->addRetirementEffect( writeFPRS( inst ) );
  inst->addPostvalidation( validateFPRS(inst) );

  return inst;
}

boost::intrusive_ptr<v9Instruction>  wrpr(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );
  if (operands.rd() == 4 || operands.rd() > 14 ) {
    return grayBox( aFetchedOpcode, aCPU, aSequenceNo, codeWRPRUnsupported); //TICK and all other ASRs force syncrhonization
  }

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  if (operands.rd() == 6 || operands.rd() == 9 ) {
    //PSTATE writes must halt dispatch
    inst->setHaltDispatch();
  }

  inst->setClass(clsComputation, codeWRPR);

  std::vector < std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, operation(3) /*xor*/, 0, rs_deps ) ;

  connectDependance( inst->retirementDependance(), exec );

  format3Operands( inst, operands, rs_deps );

  inst->addRetirementEffect( writePR( inst, operands.rd() ) );
  if (operands.rd() < 4) {
    inst->addPostvalidation( validateTPR(inst, operands.rd(), kResult, kocTL) );
  } else {
    inst->addPostvalidation( validatePR(inst, operands.rd(), kResult) );
  }

  return inst;
}

boost::intrusive_ptr<v9Instruction>  MEMBAR( Format3 const & operands, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  if (operands.i() == 0) {
    //MEMBAR #StoreStore

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    inst->setClass(clsMEMBAR, codeMEMBARStSt);
    inst->addRetirementConstraint( membarStoreStoreConstraint(inst) );

    return boost::intrusive_ptr<v9Instruction>(inst);

  } else if ( (aFetchedOpcode.theOpcode & 0x70) != 0 ) {
    //Synchronizing MEMBARS.
    //Although this is not entirely to spec, we implement synchronizing MEMBARs
    //as StoreLoad MEMBERS.

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
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

    return boost::intrusive_ptr<v9Instruction>(inst);

  } else if ( (aFetchedOpcode.theOpcode & 2) != 0 ) {
    //MEMBAR #StoreLoad

    //The only ordering MEMBAR that matters is the #StoreLoad, as all the other
    //constraints are implicit in TSO.  The #StoreLoad MEMBAR may not retire
    //until the store buffer is empty.

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    inst->setClass(clsMEMBAR, codeMEMBARStLd);
    inst->addRetirementConstraint( membarStoreLoadConstraint(inst) );
    inst->addDispatchEffect( allocateMEMBAR( inst ) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( eraseLSQ(inst) ); //The LSQ entry may already be gone if the MEMBAR retired non-speculatively (empty SB)
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

    return boost::intrusive_ptr<v9Instruction>(inst);

  } else if ( (aFetchedOpcode.theOpcode & 8) != 0 ) {
    //MEMBAR #StoreStore

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    inst->setClass(clsMEMBAR, codeMEMBARStSt);
    inst->addRetirementConstraint( membarStoreStoreConstraint(inst) );

    return boost::intrusive_ptr<v9Instruction>(inst);

  } else {
    //All other MEMBARs are NOPs under TSO
    return nop( aFetchedOpcode, aCPU, aSequenceNo );
  }
}

boost::intrusive_ptr<v9Instruction>  rdccr(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(1);
  predicated_action exec = addExecute( inst, operation(0xFE) /*convert ccr to integer*/, operands.rd(), rs_deps ) ;
  //inst->addDispatchAction( exec );

  addReadCC(inst, 0 /*icc*/, 1, rs_deps[0]) ;
  addDestination( inst, operands.rd(), exec );
  return inst;
}

boost::intrusive_ptr<v9Instruction>  rdfprs(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );

  if (operands.rd() != 0) {
    setRD( inst, operands.rd());
    inst->addDispatchEffect( mapDestination( inst, rRegisters ) );
    inst->addRetirementEffect( readFPRS(inst) );
    inst->addPostvalidation( validateRegister( operands.rd(), kResult, inst  ) );
    inst->setHaltDispatch();
  }

  return inst;
}

boost::intrusive_ptr<v9Instruction>  rdstick(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );
  predicated_action exec = readSTICKAction( inst );
  inst->addDispatchAction( exec );  //Read tick at dispatch to assure monotonicity
  addDestination( inst, operands.rd(), exec );
  inst->overrideSimics();  //Read STICK will not match Simics; ours will invariably be lower because we read early (at dispatch, not retirement). we force Simics to use our value.
  return inst;
}

boost::intrusive_ptr<v9Instruction>  rdpc(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );
  predicated_action exec = readPCAction( inst );
  inst->addDispatchAction( exec );
  addDestination( inst, operands.rd(), exec );
  return inst;
}

boost::intrusive_ptr<v9Instruction>  rd_(  Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  int32_t reg = 0;

  switch (operands.rs1()) {
    case 0:
      reg = nuArch::kRegY;
      break;
    case 2: //CCR
      return rdccr( aFetchedOpcode, aCPU, aSequenceNo);
    case 3:
      reg = nuArch::kRegASI;
      break;
    case 4: //TICK
      return rdtick( aFetchedOpcode, aCPU, aSequenceNo);
    case 5: //PC
      return rdpc( aFetchedOpcode, aCPU, aSequenceNo);
    case 6: //FPRS
      return rdfprs( aFetchedOpcode, aCPU, aSequenceNo);
    case 15:
      return MEMBAR( operands, aFetchedOpcode, aCPU, aSequenceNo);
    case 19:
      reg = nuArch::kRegGSR;
      break;
    case 24: //STICK
      return rdstick( aFetchedOpcode, aCPU, aSequenceNo);
    default: //No other ASRs are worth implementing
      return grayBox( aFetchedOpcode, aCPU, aSequenceNo, codeRDPRUnsupported);
  }
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, operation(0) /*Add*/, operands.rd(), rs_deps ) ;

  addReadRRegister( inst, 1, reg, rs_deps[0] );
  addSimm13( inst, 0, kOperand2, rs_deps[1] );

  addDestination( inst, operands.rd(), exec );
  return inst;
}

boost::intrusive_ptr<v9Instruction>  wrccr( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = executeAction( inst, operation(0xFD), rs_deps, kResultCC, kCCpd );
  inst->addDispatchAction( exec );
  setCCd( inst );
  addWriteback( inst, ccBits, exec );
  inst->addPostvalidation( validateCC( kResultCC, inst  ) );
  return inst;
}

boost::intrusive_ptr<v9Instruction>  wr_( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );
  int32_t op = 0;
  int32_t reg = 0;

  switch (operands.rd()) {
    case 0:
      reg = nuArch::kRegY;
      op = 0xFF; //Add and truncate
      break;
    //case 2:
      //return wrccr( aFetchedOpcode, aCPU, aSequenceNo);
    case 3:
      reg = nuArch::kRegASI;
      break;
    case 6:
      return wrfprs( aFetchedOpcode, aCPU, aSequenceNo);
      //Everything except Y, ASI, FPRS, and CCR will cause us to sync
    case 19:
      reg = nuArch::kRegGSR;
      break;
    case 20: //SOFTINT_SET
    case 21: //SOFTINT_CLR
      return nop(aFetchedOpcode, aCPU, aSequenceNo);
    default:
      return grayBox( aFetchedOpcode, aCPU, aSequenceNo, codeWRPRUnsupported);
  }
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(2);

  predicated_action exec = addExecute( inst, operation(op) /*add*/, reg, rs_deps) ;

  format3Operands( inst, operands, rs_deps );

  addDestination( inst, reg, exec );

  return inst;
}

SemanticInstruction  * movcc( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );
  bool floating = ! operands.cc2();

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeALU);

  std::vector< std::list<InternalDependance> > rs_deps(4);

  uint32_t cond =  (aFetchedOpcode.theOpcode >> 14) & 0xF;

  if (floating) {
    inst->setOperand( kOperand4, 0x20 | cond );
  } else {
    inst->setOperand( kOperand4, packCondition( false, operands.cc1(),  cond ) );
  }
  predicated_action exec = addExecute( inst, operation(op3), operands.rd(), rs_deps ) ;

  addReadRRegister( inst, 1, operands.rd(), rs_deps[0] );
  if (operands.is_simm13()) {
    addSimm11( inst, operands.simm11(), kOperand2, rs_deps[1] );
  } else {
    addReadRRegister( inst, 2, operands.rs2(), rs_deps[1] );
  }
  if (floating) {
    addReadCC( inst, 1 + operands.fcc(), 3, rs_deps[2] );
  } else {
    addReadCC( inst, 0/*icc*/, 3, rs_deps[2] );
  }
  satisfyAtDispatch( inst, rs_deps[3] );

  addDestination( inst, operands.rd(), exec );

  return inst;
}

SemanticInstruction  * tcc( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeTcc);

  std::vector< std::list<InternalDependance> > rs_deps(4);

  uint32_t cond =  (aFetchedOpcode.theOpcode >> 14) & 0xF;

  inst->setOperand( kOperand4, packCondition( false, operands.cc1(),  cond ) );
  predicated_action exec = addExecute( inst, operation(op3), 0, rs_deps ) ;

  addReadRRegister( inst, 1, operands.rd(), rs_deps[0] );
  if (operands.is_simm13()) {
    addSimm11( inst, operands.simm11(), kOperand2, rs_deps[1] );
  } else {
    addReadRRegister( inst, 2, operands.rs2(), rs_deps[1] );
  }
  addReadCC( inst, 0/*icc*/, 3, rs_deps[2] );

  satisfyAtDispatch( inst, rs_deps[3] );

  addDestination( inst, 0, exec );

  inst->addCheckTrapEffect( tccEffect(inst) );

  return inst;
}

#if 0
SemanticInstruction  * fmovcc( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {

  Format3 operands( aFetchedOpcode.theOpcode );
  bool floating = ! ( (aFetchedOpcode.theOpcode >> 13) & 1);

  eSize size = kWord;
  if ( (opf & 3) == 2) {
    size = kDoubleWord;
  }

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeFP);

  std::vector< std::list<InternalDependance> > rs_deps(4);

  uint32_t cond =  (aFetchedOpcode.theOpcode >> 14) & 0xF;

  if (floating) {
    inst->setOperand( kOperand4, 0x20 | cond );
  } else {
    inst->setOperand( kOperand4, packCondition( false, operands.cc1(),  cond ) );
  }
  predicated_action exec = addFPExecute( inst, operation(op3), 2,  operands.rd(), rs_deps ) ;

  addReadFRegister( inst, 1, operands.rd(), size, rs_deps[0] );
  addReadRRegister( inst, 2, operands.rs2(), size, rs_deps[1] );

  if (floating) {
    addReadCC( inst, 1 + operands.fcc(), 3, rs_deps[2] );
  } else {
    addReadCC( inst, 0/*icc*/, 3, rs_deps[2] );
  }
  satisfyAtDispatch( inst, rs_deps[3] );

  addDestination( inst, operands.rd(), exec );

  return inst;
}
#endif

SemanticInstruction  * movr( uint32_t op3, Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

  inst->setClass(clsComputation, codeALU);

  Format3 operands( aFetchedOpcode.theOpcode );

  std::vector< std::list<InternalDependance> > rs_deps(4);

  uint32_t cond  = (aFetchedOpcode.theOpcode  >> 10) & 0x7;    // opcode[12:10]

  inst->setOperand( kOperand4, cond );
  predicated_action exec = addExecute( inst, operation(op3), operands.rd(), rs_deps ) ;

  addReadRRegister( inst, 1, operands.rd(), rs_deps[0] );
  if (operands.is_simm13()) {
    addSimm10( inst, operands.simm10(), kOperand2, rs_deps[1] );
  } else {
    addReadRRegister( inst, 2, operands.rs2(), rs_deps[1] );
  }
  addReadRRegister( inst, 3, operands.rs1(), rs_deps[2] );
  satisfyAtDispatch( inst, rs_deps[3] );

  addDestination( inst, operands.rd(), exec );

  return inst;
}

boost::intrusive_ptr<v9Instruction> fpop1( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0          |    op3    |

  uint32_t opf = (aFetchedOpcode.theOpcode >> 5) & 0x1FF;

  switch ( opf ) {
    case 0x41: //FADDs
    case 0x42: //FADDd
    case 0x45: //FSUBs
    case 0x46: //FSUBd
    case 0x49: //FMULs
    case 0x4A: //FMULd
    case 0x4D: //FDIVs
    case 0x4E: //FDIVd
      return fp_calc(2, opf, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x01: //FMOVs
    case 0x02: //FMOVd
    case 0x05: //FNEGs
    case 0x06: //FNEGd
    case 0x09: //FABSs
    case 0x0A: //FABSd
    case 0x29: //FSQRTs
    case 0x2A: //FSQRTd
      return fp_calc(1, opf, aFetchedOpcode, aCPU, aSequenceNo);

    case 0x69: //FsMULd
      return fsmuld(opf, aFetchedOpcode, aCPU, aSequenceNo);

    case 0x84: //FxTOs
    case 0xC6: //FdTOs
    case 0xD2: //FdTOi
      return fp_cvt(kDoubleWord, kWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x88: //FxTOd
    case 0x82: //FdTOx
      return fp_cvt(kDoubleWord, kDoubleWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    case 0xC4: //FiTOs
    case 0xD1: //FsTOi
      return fp_cvt(kWord, kWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    case 0xC8: //FiTOd
    case 0xC9: //FsTOd
    case 0x81: //FsTOx
      return fp_cvt(kWord, kDoubleWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    default:
      break;
  }

  return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
}

boost::intrusive_ptr<v9Instruction> fpop2( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0          |    op3    |

  uint32_t opf = (aFetchedOpcode.theOpcode >> 5) & 0x1FF;

  switch ( opf ) {
    case 0x51: //FCMPs
    case 0x55: //FCMPEs
      return fp_cmp(kWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x52: //FCMPd
    case 0x56: //FCMPEd
      return fp_cmp(kDoubleWord, opf, aFetchedOpcode, aCPU, aSequenceNo);
    default:
      break;
  }

  return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
}

boost::intrusive_ptr<v9Instruction> visop( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0          |    op3    |

  uint32_t opf = (aFetchedOpcode.theOpcode >> 5) & 0x1FF;

  switch ( opf ) {
    case 0x18: //ALIGNADDR
      return alignaddr(false, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x1A: //ALIGNADDR_LITTLE
      return alignaddr(true, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x48: //FALIGNDATA
      return faligndata(aFetchedOpcode, aCPU, aSequenceNo);
    case 0x60: //FZERO
      return fzero(kDoubleWord, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x61: //FZERO
      return fzero(kWord, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x74:
      return fsrc(1, kDoubleWord, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x75:
      return fsrc(1, kWord, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x78:
      return fsrc(2, kDoubleWord, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x79:
      return fsrc(2, kWord, aFetchedOpcode, aCPU, aSequenceNo);

    default:
      break;
  }
  DBG_(Dev, ( << "Unimplemented extension visop : " << std::hex << opf << std::dec ) );

  return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
}

boost::intrusive_ptr<v9Instruction>  bicc( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0|a| cond  |0 1 0 |                 disp22

  bool annul = !! (opcode & (1ULL << 29));    // opcode[29]
  uint32_t cond  = (opcode >> 25) & 0xF;    // opcode[28:25]
  uint32_t disp22 = opcode & 0x3FFFFF;    // opcode[21:0]

  //sign-extend
  int64_t disp22_ll(disp22);
  if (disp22_ll & (1 << 21) ) {
    disp22_ll |=  0xFFFFFFFFFFC00000LL;
  }
  //multiply by 4
  disp22_ll <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp22_ll;

  VirtualMemoryAddress target(target_ll);

  if (cond == 0) {
    if (annul) {
      return annul_next( aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return nop( aFetchedOpcode, aCPU, aSequenceNo);
    }
  } else if (cond == 8) {
    return branch_always( annul, target, aFetchedOpcode, aCPU, aSequenceNo);
  } else {
    return branch_cc( annul, false, cond, target, aFetchedOpcode, aCPU, aSequenceNo);
  }
}

boost::intrusive_ptr<v9Instruction>  bfcc( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0|a| cond  |0 1 0 |                 disp22

  bool annul = !! (opcode & (1ULL << 29));    // opcode[29]
  uint32_t cond  = (opcode >> 25) & 0xF;    // opcode[28:25]
  uint32_t disp22 = opcode & 0x3FFFFF;    // opcode[21:0]

  //sign-extend
  int64_t disp22_ll(disp22);
  if (disp22_ll & (1 << 21) ) {
    disp22_ll |=  0xFFFFFFFFFFC00000LL;
  }
  //multiply by 4
  disp22_ll <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp22_ll;

  VirtualMemoryAddress target(target_ll);

  if (cond == 0) {
    if (annul) {
      return annul_next( aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return nop( aFetchedOpcode, aCPU, aSequenceNo);
    }
  } else if (cond == 8) {
    return branch_always( annul, target, aFetchedOpcode, aCPU, aSequenceNo);
  } else {
    return branch_fcc( annul, 0, cond, target, aFetchedOpcode, aCPU, aSequenceNo);
  }
}

boost::intrusive_ptr<v9Instruction>  bpcc( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0|a| cond  |0 1 0 |                 disp22

  bool annul = !! (opcode & (1ULL << 29));    // opcode[29]
  uint32_t cond  = (opcode >> 25) & 0xF;    // opcode[28:25]
  uint32_t disp19 = opcode & 0x7FFFF;    // opcode[18:0]
  bool xcc = !! ( opcode & (1ULL << 21));    // opcode[21]
  //bool predict = !! ( opcode & (1ULL << 19));    // opcode[19]

  //sign-extend
  int64_t disp19_ll(disp19);
  if (disp19_ll & (1 << 18) ) {
    disp19_ll |=  0xFFFFFFFFFFFC0000LL;
  }
  //multiply by 4
  disp19_ll <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp19_ll;

  VirtualMemoryAddress target(target_ll);

  if (cond == 0) {
    if (annul) {
      return annul_next( aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return nop( aFetchedOpcode, aCPU, aSequenceNo);
    }
  } else if (cond == 8) {
    return branch_always( annul, target, aFetchedOpcode, aCPU, aSequenceNo);
  } else {
    return branch_cc( annul, xcc, cond, target, aFetchedOpcode, aCPU, aSequenceNo);
  }
}

boost::intrusive_ptr<v9Instruction>  bpr( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 0 0|a|0|rcond|0 1 1|dhi|p|   rs1   |           d16lo

  bool annul = !! (opcode & (1ULL << 29));    // opcode[29]
  uint32_t cond  = (opcode >> 25) & 0x7;    // opcode[28:25]
  uint32_t disp16 = ((opcode >> 6) & 0xC000 ) | (opcode & 0x3FFF);    // opcode[21:20] opcode[13:0]
  uint32_t rs1 = (opcode >> 14) & 0x1F;
  //bool predict = !! ( opcode & (1ULL << 19));    // opcode[19]

  // Make sure it's a valid condition field
  if ( !rconditionValid ( cond ) )
    return blackBox ( aFetchedOpcode, aCPU, aSequenceNo );

  //sign-extend
  int64_t disp16_ll(disp16);
  if (disp16_ll & (1 << 15) ) {
    disp16_ll |=  0xFFFFFFFFFFFF0000LL;
  }
  //multiply by 4
  disp16_ll <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp16_ll;

  VirtualMemoryAddress target(target_ll);

  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theNextPC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsBranch, codeBranchConditional);

  std::list<InternalDependance>  rs_deps;
  dependant_action br = branchRegAction( inst, target, annul, cond ) ;
  //inst->addDispatchAction( br );
  connectDependance( inst->retirementDependance(), br );
  rs_deps.push_back( br.dependance );

  addReadRRegister( inst, 1, rs1, rs_deps );

  return inst;
}

boost::intrusive_ptr<v9Instruction>  fbpcc( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0|a| cond  |0 1 0 |                 disp22

  bool annul = !! (opcode & (1ULL << 29));    // opcode[29]
  uint32_t cond  = (opcode >> 25) & 0xF;    // opcode[28:25]
  uint32_t disp19 = opcode & 0x7FFFF;    // opcode[18:0]
  uint32_t fcc = (opcode >> 20) & 3;    // opcode[21:20]

  //sign-extend
  int64_t disp19_ll(disp19);
  if (disp19_ll & (1 << 18) ) {
    disp19_ll |=  0xFFFFFFFFFFFF8000LL;
  }
  //multiply by 4
  disp19_ll <<= 2;
  int64_t pc(aFetchedOpcode.thePC);
  int64_t target_ll = pc + disp19_ll;

  VirtualMemoryAddress target(target_ll);

  if (cond == 0) {
    if (annul) {
      return annul_next( aFetchedOpcode, aCPU, aSequenceNo);
    } else {
      return nop( aFetchedOpcode, aCPU, aSequenceNo);
    }
  } else if (cond == 8) {
    return branch_always( annul, target, aFetchedOpcode, aCPU, aSequenceNo);
  } else {
    return branch_fcc( annul, fcc, cond, target, aFetchedOpcode, aCPU, aSequenceNo);
  }
}

boost::intrusive_ptr<v9Instruction> format_2( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // ================================================================
  // 0 0          | op2 |

  uint32_t op2 = (opcode >> 22) & 0x7;

  switch (op2) {

    case 0: //ILLTRAP - Used to signal ITLB Miss
      return immu_exception( aFetchedOpcode, aCPU, aSequenceNo);
    case 1: //BPicc
      return bpcc( aFetchedOpcode, aCPU, aSequenceNo);
    case 2: //Bicc
      return bicc( aFetchedOpcode, aCPU, aSequenceNo);
    case 3: //BPr
      return bpr( aFetchedOpcode, aCPU, aSequenceNo);
    case 4: { //SETHI (nop)
      int32_t imm22 = opcode & 0x3FFFFF;  // opcode[21:0]
      int32_t rd = (opcode >> 25) & 0x1F; // opcode[29:25]
      if (0 == rd ) {
        if ( imm22 == 0 ) {
          //standard NOP
          return nop( aFetchedOpcode, aCPU, aSequenceNo);
        } else {
          //Simics MAGIC instruction
          return magic( aFetchedOpcode, aCPU, aSequenceNo);
        }
      } else {
        //SETHI
        return sethi( imm22, rd, aFetchedOpcode, aCPU, aSequenceNo);
      }
      break;
    }
    case 5: //FBPfcc
      return fbpcc( aFetchedOpcode, aCPU, aSequenceNo);
    case 6: //FBfcc
      return bfcc( aFetchedOpcode, aCPU, aSequenceNo);
    case 7: //Undefined
    default:
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
  }
}

boost::intrusive_ptr<v9Instruction> format_3_reg( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 0          |    op3    |

  uint32_t op3 = (opcode >> 19) & 0x3F;

  //op3 from 0 to 8 and 16 to 24 are logical/add
  switch (op3) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
      return alu( (op3 >= 16) , op3, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x08:
    case 0x0C:
    case 0x18:
    case 0x1C:
      return alu_c( (op3 >= 16) , op3, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x09:
      return mul( false, false, op3, aFetchedOpcode, aCPU, aSequenceNo );
    case 0x0A:
    case 0x0B:
    case 0x1A:
    case 0x1B:
      return mul( (op3 >= 0x10), true, op3, aFetchedOpcode, aCPU, aSequenceNo );
    case 0x0D:
    case 0x2D:
      return mul( false, false, op3, aFetchedOpcode, aCPU, aSequenceNo );
    case 0x0E:
    case 0x0F:
    case 0x1E:
    case 0x1F:
      return div_y( (op3 >= 0x10),  op3, aFetchedOpcode, aCPU, aSequenceNo );
    case 0x25:
    case 0x26:
    case 0x27:
      return shift ( op3, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x28:
      return rd_( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2A:
      return rdpr( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2B:
      //This is actually FLUSHW (flush register windows).
      //It is either a NOP, or it causes a trap.
      return flushw( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x2C:
      return movcc( op3, aFetchedOpcode, aCPU, aSequenceNo);

    case 0x2E:
      //POPC - not implemented in hardwate in Sparc IIIcu, always
      //causes a trap.
      return grayBox( aFetchedOpcode, aCPU, aSequenceNo, codePOPCUnsupported);
	case 0x2F: {
      uint32_t cond  = (aFetchedOpcode.theOpcode  >> 10) & 0x7;    // opcode[12:10]
      if (cond == 0x0) // reserved
        return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
      else
        return movr( op3, aFetchedOpcode, aCPU, aSequenceNo);
    }
    case 0x30:
      return wr_( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x31:
      return saved_and_restored( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x32:
      return wrpr( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x34:
      return fpop1( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x35:
      return fpop2( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x36:
      return visop( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x38:
      return jmpl( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x39:
      return return_( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x3A:
      //This is actually Tcc (trap on condition code).  If the condition is
      //false, it is a nop, and we do the right thing.  If the condition is
      //true, it causes a trap, which we will pick up from Simics.
      return tcc( op3, aFetchedOpcode, aCPU, aSequenceNo);
    case 0x3B:
      //This is actually FLUSH, but since we have no I-cache yet, we don't need
      //flush
      return nop( aFetchedOpcode, aCPU, aSequenceNo);
    case 0x3C:
    case 0x3D:
      return save_and_restore( (op3 & 1), aFetchedOpcode, aCPU, aSequenceNo);
    case 0x3E: //DONE and RETRY
      return retry( opcode & 0x2000000 /*retry if true*/, aFetchedOpcode, aCPU, aSequenceNo);
    default:
      DBG_( Verb, ( << "Unimplemented op3=0x" << std::hex << op3 << std::dec ) );
      return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
  }
}

std::pair<boost::intrusive_ptr<v9Instruction>, bool> format_3_mem( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop ) {
  uint32_t opcode = aFetchedOpcode.theOpcode;
  boost::intrusive_ptr<v9Instruction> ret_val;
  bool last_uop = true;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // 1 1          |    op3    |

  uint32_t op3 = (opcode >> 19) & 0x3F;

  //op3 from 0 to 8 are normal memory operations
  switch ( op3 ) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0E:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1E:
      ret_val = memory( op3, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x03:
      ret_val = ldd( false, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x13:
      ret_val = ldd( true, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x07:
      ret_val = std( false, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x17:
      ret_val = std( true, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x0D:
      ret_val = ldstub( false, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x1D:
      ret_val = ldstub( true, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x0F:
      ret_val = swap( false, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x1F:
      ret_val = swap( true, aFetchedOpcode, aCPU, aSequenceNo);
      break;

    case 0x20:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x26:
    case 0x27:
    case 0x30:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x36:
    case 0x37:
      std::tie( ret_val, last_uop) = fp_memory( op3, aFetchedOpcode, aCPU, aSequenceNo, aUop);
      break;
    case 0x21:
      ret_val = ldfsr( aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x25:
      ret_val = stfsr( aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x2D:
    case 0x3D:
      //PREFETCH - these need to be implemented properly.  For now, we punt
      ret_val = nop( aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x3C:
      ret_val = cas( kWord, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    case 0x3E:
      ret_val = cas( kDoubleWord, aFetchedOpcode, aCPU, aSequenceNo);
      break;
    default:
      ret_val = blackBox( aFetchedOpcode, aCPU, aSequenceNo);
      break;
  }
  return std::make_pair( ret_val, last_uop );
}

std::pair< boost::intrusive_ptr<AbstractInstruction>, bool> decode( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop ) {

  boost::intrusive_ptr<nuArch::AbstractInstruction> ret_val;
  bool last_uop = true;
  uint32_t opcode = aFetchedOpcode.theOpcode;

  //v9 Instruction Format
  // 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
  // 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  // =======+=======+=======+=======+=======+=======+=======+========
  // |op|

  //op - Select opcode format
  uint32_t op = opcode >> 30;

  DBG_( Verb, ( << "Start decode op=" << op));

  switch (op) {
    case 0: //Bicc, FBfcc, CBccc, SETHI
      ret_val = format_2( aFetchedOpcode, aCPU, aSequenceNo);
      break;

    case 1: //CALL
      ret_val = call( aFetchedOpcode, aCPU, aSequenceNo);
      break;

    case 2: //Integer/FP
      ret_val = format_3_reg( aFetchedOpcode, aCPU, aSequenceNo);
      break;

    case 3: //Memory
      std::tie(ret_val, last_uop) = format_3_mem( aFetchedOpcode, aCPU, aSequenceNo, aUop);
      break;

    default:
      ret_val = blackBox( aFetchedOpcode, aCPU, aSequenceNo);
  }

  DBG_( Verb, ( << "Decode: " << *ret_val));

  return std::make_pair(ret_val, last_uop);
}

bool v9Instruction::usesIntAlu() const {
  return theUsesIntAlu;
}

bool v9Instruction::usesIntMult() const {
  return theUsesIntMult;
}

bool v9Instruction::usesIntDiv() const {
  return theUsesIntDiv;
}

bool v9Instruction::usesFpAdd() const {
  return theUsesFpAdd;
}

bool v9Instruction::usesFpCmp() const {
  return theUsesFpCmp;
}

bool v9Instruction::usesFpCvt() const {
  return theUsesFpCvt;
}

bool v9Instruction::usesFpMult() const {
  return theUsesFpMult;
}

bool v9Instruction::usesFpDiv() const {
  return theUsesFpDiv;
}

bool v9Instruction::usesFpSqrt() const {
  return theUsesFpSqrt;
}

} //nv9Decoder
