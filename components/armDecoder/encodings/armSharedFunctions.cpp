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

eExtendType DecodeRegExtend(int anOption) {
    switch (anOption) {
    case 0:        return kExtendType_UXTB;
    case 1:        return kExtendType_UXTH;
    case 2:        return kExtendType_UXTW;
    case 3:        return kExtendType_UXTX;
    case 4:        return kExtendType_SXTB;
    case 5:        return kExtendType_SXTH;
    case 6:        return kExtendType_SXTW;
    case 7:        return kExtendType_SXTX;
    default:        assert(false); break;
    }
}

eIndex getIndex ( unsigned int index) {
    switch (index) {
    case 0x1:
        return kPostIndex;
    case 0x2:
        return kSingedOffset;
    case 0x3:
        return kPreIndex;
    default:
        return kNoOffset;
        break;
    }
}

std::unique_ptr<Operation> extend(eExtendType anExtend){
    switch (anExtend) {
    case kExtendType_SXTB:              return operation(kSextB_);
    case kExtendType_SXTH:              return operation(kSextH_);
    case kExtendType_SXTW:              return operation(kSextW_);
    case kExtendType_SXTX:              return operation(kSextX_);
    case kExtendType_UXTB:              return operation(kZextB_);
    case kExtendType_UXTH:              return operation(kZextH_);
    case kExtendType_UXTW:              return operation(kZextW_);
    case kExtendType_UXTX:              return operation(kZextX_);
    default:        assert(false); break;
    }
}

/* The input should be a value in the bottom e bits (with higher
 * bits zero); returns that value replicated into every element
 * of size e in a 64 bit integer.
 */
uint64_t bitfield_replicate(bits mask, unsigned int e)
{
    assert(e != 0);
    while (e < 64) {
        mask |= mask << e;
        e *= 2;
    }
    return mask;
}

uint64_t bitmask64(unsigned int length)
{
    assert(length > 0 && length <= 64);
    return ~0ULL >> (64 - length);
}


void setRD( SemanticInstruction * inst, uint32_t rd) {
  reg regRD;
  regRD.theType = xRegisters;
  regRD.theIndex = rd;
  inst->setOperand( kRD, regRD );
  DECODER_DBG( "Writing to x[" << rd << "]"<<"\e[0m");

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

void satisfyAtDispatch( SemanticInstruction * inst, std::list<InternalDependance> & dependances) {
  std::list< InternalDependance >::iterator iter, end;
  for ( iter = dependances.begin(), end = dependances.end(); iter != end; ++iter) {
    inst->addDispatchEffect( satisfy( inst, *iter ) );
  }
}

//addReadRDs(SemanticInstruction * inst, uint32_t rd, uint32_t rd1 ) {

//    setRD( inst, rd);
//    setRD1( inst, rd1);

//    inst->addDispatchEffect( mapSource( inst, kRD, kPD ) );
//    inst->addDispatchEffect( mapSource( inst, kRD1, kPD1 ) );

//    simple_action read_value = readRegisterAction( inst, kPD, kResult );
//    inst->addDispatchAction( read_value );

//    connectDependance( update_value.dependance, read_value );
//    connectDependance( inst->retirementDependance(), update_value );

//    inst->addPrevalidation( validateRegister( rd, kResult, inst ) );

//    inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
//    inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );


//}


void addReadXRegister( SemanticInstruction * inst, int32_t anOpNumber, uint32_t rs, std::list<InternalDependance> & dependances, bool is_64) {

    //Calculate operand codes from anOpNumber
    DBG_Assert( anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 || anOpNumber == 5 );
    eOperandCode cOperand = eOperandCode( kOperand1 + anOpNumber - 1);
    eOperandCode cRS = eOperandCode( kRS1 + anOpNumber - 1);
    eOperandCode cPS = eOperandCode( kPS1 + anOpNumber - 1);

    DECODER_DBG("Reading x[" << rs << "] and mapping it [" << cRS << " -> " << cPS << "]");

    setRS( inst, cRS , rs );
    inst->addDispatchEffect( mapSource( inst, cRS, cPS ) );
    simple_action act = readRegisterAction( inst, cPS, cOperand, is_64 );
    connect( dependances, act );
    inst->addDispatchAction( act );
    inst->addPrevalidation( validateXRegister( rs, cOperand, inst ) );

}

void addReadConstant (SemanticInstruction * inst, int32_t anOpNumber, bits val, std::list<InternalDependance> & dependances){

    DBG_Assert( anOpNumber == 1 || anOpNumber == 2 || anOpNumber == 3 || anOpNumber == 4 || anOpNumber == 5 );
    eOperandCode cOperand = eOperandCode( kOperand1 + anOpNumber - 1);

    simple_action act = readConstantAction( inst, val, cOperand );
    connect( dependances, act );
    inst->addDispatchAction( act );

}

void addAnnulment( SemanticInstruction * inst, predicated_action & exec, InternalDependance const & aWritebackDependance) {
  predicated_action annul = annulAction( inst );
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

void addWriteback( SemanticInstruction * inst, eOperandCode aRegisterCode,eOperandCode aMappedRegisterCode, predicated_action & exec, bool a64, bool setflags, bool addSquash) {
  if (addSquash) {
    inst->addDispatchEffect( mapDestination( inst ) );
  } else {
    inst->addDispatchEffect( mapDestination_NoSquashEffects( inst ) );
  }

  //Create the writeback action
  dependant_action wb = writebackAction( inst, aRegisterCode, aMappedRegisterCode, a64, setflags );
//  inst->addDispatchAction( wb );

//  addAnnulment( inst, exec, wb.dependance );

  //Make writeback depend on execute, make retirement depend on writeback
  connectDependance( wb.dependance, exec );
  connectDependance( inst->retirementDependance(), wb );
}


void addDestination( SemanticInstruction * inst, uint32_t rd, predicated_action & exec, bool is64, bool setflags, bool addSquash) {

    setRD( inst, rd);
    addWriteback( inst, kResult, kPD, exec, is64, setflags, addSquash );
    inst->addPostvalidation( validateXRegister( rd, kResult, inst  ) );
//    inst->addOverride( overrideRegister( rd, kResult, inst ) );
}

void addPairDestination( SemanticInstruction * inst, uint32_t rd, uint32_t rd1, predicated_action & exec, bool is64, bool addSquash) {

    setRD( inst, rd);
    addWriteback( inst, kResult, kPD, exec, is64, false, addSquash);
    inst->addPostvalidation( validateXRegister( rd, kResult, inst  ) );
//    inst->addOverride( overrideRegister( operands.rd(), kResult, inst ) );
    setRD1( inst, rd1);
    addWriteback( inst, kResult1, kPD1, exec, is64, false, addSquash);
    inst->addPostvalidation( validateXRegister( rd1, kResult1, inst  ) );
//    inst->addOverride( overrideRegister( operands.rd() + 1, kResult1, inst ) );
}

predicated_action addExecute( SemanticInstruction * inst, std::unique_ptr<Operation> anOperation, std::vector< std::list<InternalDependance> > & rs_deps, eOperandCode aResult, boost::optional<eOperandCode> aBypass ) {
  predicated_action exec;

    exec = executeAction( inst, anOperation, rs_deps, aResult, aBypass );
//    inst->addDispatchAction( exec );
    return exec;
}

predicated_action addExecute( SemanticInstruction * inst, std::unique_ptr<Operation> anOperation, eOperandCode anOperand1,  eOperandCode anOperand2, std::vector< std::list<InternalDependance> > & rs_deps ,eOperandCode aResult, boost::optional<eOperandCode> aBypass ) {
  predicated_action exec;

    exec = executeAction( inst, anOperation, anOperand1, anOperand2, rs_deps, aResult, aBypass );
//    inst->addDispatchAction( exec );
    return exec;
}

void addAddressCompute( SemanticInstruction * inst, std::vector< std::list<InternalDependance> > & rs_deps) {
  DECODER_TRACE;

  simple_action tr = translationAction(inst);
  multiply_dependant_action update_address = updateVirtualAddressAction( inst, rs_deps.size() );
  inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );
  simple_action exec = calcAddressAction( inst, rs_deps);

  connectDependance( update_address.dependances[0], exec);
  connectDependance(tr.action->dependance(0), exec);
  connectDependance( inst->retirementDependance(), update_address );

}

void MEMBAR( SemanticInstruction * inst, uint32_t anAccess) {
  DECODER_TRACE;

  switch (anAccess & 0xf) {
  case kMO_ST_ST:
    //MEMBAR #StoreStore
    inst->setClass(clsMEMBAR, codeMEMBARStSt);
    inst->addRetirementConstraint( membarStoreStoreConstraint(inst) );
    break;
  case kMO_ST_LD: case kMO_LD_LD:
    //Synchronizing MEMBARS.
    //Although this is not entirely to spec, we implement synchronizing MEMBARs
    //as StoreLoad MEMBERS.
    inst->setClass(clsMEMBAR, codeMEMBARSync);
    //#Sync implementation - waits for core to drain/synchronize

    //ordering implementation - allows TSO++ speculation
    inst->addRetirementConstraint( membarSyncConstraint(inst) );
    inst->addDispatchEffect( allocateMEMBAR( inst ) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( eraseLSQ(inst) ); //The LSQ entry may already be gone if the MEMBAR retired non-speculatively (empty SB)
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core
    break;

  default:
    return;
  }
}

/* Update the Sixty-Four bit (SF) registersize. This logic is derived
 * from the ARMv8 specs for LDR (Shared decode for all encodings).
 */
bool disas_ldst_compute_iss_sf(int size, bool is_signed, int opc)
{
    uint32_t opc0 = extract32(opc, 0, 1);
    int regsize;

    if (is_signed) {
        regsize = opc0 ? 32 : 64;
    } else {
        regsize = size == 3 ? 64 : 32;
    }
    return regsize == 64;
}


bits highestSetBit(bits val, bits bitSize){
    for (int i = bitSize -1; i >= 0; i--){
        if ((val & (1 << i)) == 1){
            return i;
        }
    }
    return 0;
}

bits ones(bits length){
    bits tmp;
    for (bits i=0; i<length; i++){
        tmp |= (1 << i);
    }
    return tmp;
}

bits ror(bits input, bits input_size, bits shift_size){
    bits mask = ones(shift_size);
    bits remaining_bits = input & mask;

    input >>= shift_size;
    remaining_bits <<= (input_size - shift_size);

    bits filter = ones(input_size);

    return (remaining_bits | input) & filter;
}

bits lsl(bits input, bits input_size, bits shift_size){
    input <<= shift_size;
    bits filter = ones(input_size);
    return input & filter;
}

bits lsr(bits input, bits input_size, bits shift_size){
    input >>= shift_size;
    bits filter = ones(input_size);
    return input & filter;
}

bits asr(bits input, bits input_size, bits shift_size){
    bool is_signed = ((input & (1 << input_size)) != 0) ? true : false;
    input >>= shift_size;
    if (is_signed){
        input |= (ones(4) << (input_size - 4));
    }
    bits filter = ones(input_size);
    return input & filter;
}

bool decodeBitMasks(bits & tmask, bits & wmask, bool immN, char imms, char immr, bool immediate, uint32_t dataSize){

    uint32_t levels;

    // Compute log2 of element size
    // 2^len must be in range [2, M]
    uint32_t len = highestSetBit( ((immN << 6) | ~imms) , 7 );
    if (len < 1 ) return false;
    assert (dataSize >= ((uint32_t)(1 << len)));

    // Determine S, R and S - R parameters
    levels = ones(len);

    // For logical immediates an all-ones value of S is reserved
    // since it would generate a useless all-ones result (many times)
    if (immediate && ((imms & levels) == levels)){
        return false;
    }

    uint32_t S = (imms & levels);
    uint32_t R = (immr & levels);

    uint32_t diff = S - R; // 6-bit subtract with borrow

    // From a software perspective, the remaining code is equivalant to:
    uint32_t esize = 1 << len;
    uint32_t d = diff & ones(len-1);
    uint32_t welem = ones(S + 1);
    uint32_t telem = ones(d + 1);
    wmask = ror(welem, R, esize);
    tmask = telem;
    return true;
}

arminst generateException(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, eExceptionType aType){
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->addRetirementEffect(exceptionEffect(inst, kException_AA64_SVC));
    return inst;
}



} // narmDecoder







