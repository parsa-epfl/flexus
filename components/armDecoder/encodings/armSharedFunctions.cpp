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

#include "armSharedFunctions.hpp"

namespace narmDecoder {
using namespace nuArchARM;

eConstraint ConstrainUnpredictable(eUnpredictable which){
    switch (which) {
    case kUnpredictable_WBOVERLAPLD:
        return kConstraint_WBSUPPRESS; // return loaded value
    case kUnpredictable_WBOVERLAPST:
        return kConstraint_NONE; // store pre-writeback value
    case kUnpredictable_LDPOVERLAP:
        return kConstraint_UNDEF; // instruction is case kUnDEFINED
    case kUnpredictable_BASEOVERLAP:
        return kConstraint_NONE; // use original address
    case kUnpredictable_DATAOVERLAP:
        return kConstraint_NONE; // store original value
    case kUnpredictable_DEVPAGE2:
        return kConstraint_FAULT; // take an alignment fault
    case kUnpredictable_INSTRDEVICE:
        return kConstraint_NONE; // Do not take a fault
    case kUnpredictable_RESMAIR:
        return kConstraint_UNKNOWN; // Map to case kUnKNOWN value
    case kUnpredictable_RESTEXCB:
        return kConstraint_UNKNOWN; // Map to case kUnKNOWN value
    case kUnpredictable_RESDACR:
        return kConstraint_UNKNOWN; // Map to case kUnKNOWN value
    case kUnpredictable_RESPRRR:
        return kConstraint_UNKNOWN; // Map to case kUnKNOWN value
    case kUnpredictable_RESVTCRS:
        return kConstraint_UNKNOWN; // Map to case kUnKNOWN value
    case kUnpredictable_RESTnSZ:
        return kConstraint_FORCE; // Map to the limit value
    case kUnpredictable_LARGEIPA:
        return kConstraint_FORCE;// Restrict the inputsize to the PAMax value
    case kUnpredictable_ESRCONDPASS:
        return kConstraint_FALSE; // Report as "AL"
    case kUnpredictable_ILZEROIT:
        return kConstraint_FALSE; // Do not zero PSTATE.IT
    case kUnpredictable_ILZEROT:
        return kConstraint_FALSE; // Do not zero PSTATE.T
    case kUnpredictable_BPVECTORCATCHPRI:
        return kConstraint_TRUE; // Debug Vector Catch: match on 2nd halfword
    case kUnpredictable_VCMATCHHALF:
        return kConstraint_FALSE; // No match
    case kUnpredictable_VCMATCHDAPA:
        return kConstraint_FALSE; // No match on Data Abort or Prefetch abort
    case kUnpredictable_WPMASKANDBAS:
        return kConstraint_FALSE; // Watchpoint disabled
    case kUnpredictable_WPBASCONTIGUOUS:
        return kConstraint_FALSE; // Watchpoint disabled
    case kUnpredictable_RESWPMASK:
        return kConstraint_DISABLED; // Watchpoint disabled
    case kUnpredictable_WPMASKEDBITS:
        return kConstraint_FALSE; // Watchpoint disabled
    case kUnpredictable_RESBPWPCTRL:
        return kConstraint_DISABLED; // Breakpoint/watchpoint disabled
    case kUnpredictable_BPNOTIMPL:
        return kConstraint_DISABLED; // Breakpoint disabled
    case kUnpredictable_RESBPTYPE:
        return kConstraint_DISABLED; // Breakpoint disabled
    case kUnpredictable_BPNOTCTXCMP:
        return kConstraint_DISABLED; // Breakpoint disabled
    case kUnpredictable_BPMATCHHALF:
        return kConstraint_FALSE; // No match
    case kUnpredictable_BPMISMATCHHALF:
        return kConstraint_FALSE; // No match
    case kUnpredictable_RESTARTALIGNPC:
        return kConstraint_FALSE; // Do not force alignment
    case kUnpredictable_RESTARTZEROUPPERPC:
        return kConstraint_TRUE; // Force zero extension
    case kUnpredictable_ZEROUPPER:
        return kConstraint_TRUE; // zero top halves of X registers
    case kUnpredictable_ERETZEROUPPERPC:
        return kConstraint_TRUE; // zero top half of PC
    case kUnpredictable_A32FORCEALIGNPC:
        return kConstraint_FALSE; // Do not force alignment
    case kUnpredictable_SMD:
        return kConstraint_UNDEF; // disabled SMC is case kUnallocated
    case kUnpredictable_AFUPDATE: // AF update for alignment or permission fault
        return kConstraint_TRUE;
    case kUnpredictable_IESBinDebug: // Use SCTLR[].IESB in Debug state
        return kConstraint_TRUE;
    }
}



static void setRD( SemanticInstruction * inst, uint32_t rd) {
  DBG_(Tmp, (<< "\e[1;35m"<<"DECODER: EFFECT: Writing to x[" << rd << "]"<<"\e[0m"));
  reg regRD;
  regRD.theType = xRegisters;
  regRD.theIndex = rd;
  inst->setOperand( kRD, regRD );
}

static void setRD1( SemanticInstruction * inst, uint32_t rd1) {
  reg regRD1;
  regRD1.theType = xRegisters;
  regRD1.theIndex = rd1;
  inst->setOperand( kRD1, regRD1 );
}

static void setRS( SemanticInstruction * inst, eOperandCode rs_code, uint32_t rs) {
  reg regRS;
  regRS.theType = xRegisters;
  regRS.theIndex = rs;
  inst->setOperand(rs_code, regRS);
}

static void setCC( SemanticInstruction * inst, int32_t ccNum, eOperandCode rs_code) {
  reg regICC;
  regICC.theType = ccBits;
  regICC.theIndex = ccNum;
  inst->setOperand(rs_code, regICC);
}

static void setFD( SemanticInstruction * inst, eOperandCode fd_no, uint32_t fd) {
  reg regFD;
  regFD.theType = vRegisters;
  regFD.theIndex = fd;
  inst->setOperand( fd_no, regFD );
}

static void setFS( SemanticInstruction * inst, eOperandCode fs_no, uint32_t fs) {
  reg regFS;
  regFS.theType = vRegisters;
  regFS.theIndex = fs;
  inst->setOperand(fs_no, regFS);
}

static void setCCd( SemanticInstruction * inst) {
  reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 0;
  inst->setOperand( kCCd, regCCd );
}

static void setFCCd( int32_t fcc, SemanticInstruction * inst) {
  reg regCCd;
  regCCd.theType = ccBits;
  regCCd.theIndex = 1 + fcc;
  inst->setOperand( kCCd, regCCd );
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

/* Update the Sixty-Four bit (SF) registersize. This logic is derived
 * from the ARMv8 specs for LDR (Shared decode for all encodings).
 */
bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc)
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




} // narmDecoder







