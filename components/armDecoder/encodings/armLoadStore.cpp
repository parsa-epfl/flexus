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

#include "armLoadStore.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {
using namespace nuArchARM;



typedef enum eIndex{
    kPostIndex,
    kPreIndex,
    kSingedOffset,
    kNone,
}eIndex;


eIndex getIndex ( unsigned int index) {
    switch (index) {
    case 0x1:
        return kPostIndex;
    case 0x2:
        return kSingedOffset;
    case 0x3:
        return kPreIndex;
    default:
        return kNone;
        break;
    }
}

/* Store Exclusive Register Byte stores a byte from a register to memory
 * if the PE has exclusive access to the memory address, and returns a status
 * value of 0 if the store was successful, or of 1 if no store was performed.
 * See Synchronization and semaphores. The memory access is atomic.
 * For information about memory accesses see Load/Store addressing modes.
 */
arminst STXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    unsigned int size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    unsigned int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5); // ignored by load/store single register
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);

    eAccType acctype = o0 == 1 ? kAccType_ORDERED : kAccType_ATOMIC;

    inst->addDispatchEffect( allocateStore( inst, size, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    inst->setOperand(kStatus, 0);
    inst->addPrevalidation(AArch64ExclusiveMonitorsPass(inst, size, aCPU));
    addUpdateData(inst, rn, rt );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, size, inst ) );
    inst->addCommitEffect( commitStore(inst) );

    return arminst(inst);
}

/*Compare and Swap Pair of words or doublewords in memory reads a pair of 32-bit words or 64-bit doublewords from memory, and compares
 * them against the values held in the first pair of registers. If the comparison is equal, the values in the second pair of registers are written to
 * memory. If the writes are performed, the reads and writes occur atomically such that no other modification of the memory location can take place
 * between the reads and writes.
 */
arminst CASP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    unsigned int sz = extract32(aFetchedOpcode.theOpcode, 30, 1);
    unsigned int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);


    bool sf = (sz == 1) ? true : false;

    if ((rt & 1) || (rs & 1)){
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsAtomic, codeCASP);

    unsigned int size = 32 << sz;
    unsigned int datasize = size*2;

    eAccType stacctype = o0 == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
    eAccType ldacctype = L == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;

    //obtain the loaded values
    predicated_dependant_action casp = caspAction( inst, size, kPD );
    inst->addDispatchEffect( allocateCASP( inst, datasize, casp.dependance, ldacctype) );
    multiply_dependant_action update_address = updateCASAddressAction( inst);
    inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );
    inst->addDispatchEffect( satisfy( inst, update_address.dependances[2] ) );

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

    multiply_dependant_action update_value = updateCASValueAction( inst);
    InternalDependance dep( inst->retirementDependance() );
    connectDependance( dep, update_value );

    //Read the value which will be stored   
    std::vector<std::list<InternalDependance> > uv_deps(2);
//    uv_deps.push_back( update_value.dependances[0]  );
//    addReadXRegister( inst, 3, rt, uv_deps , sf);
//    addReadXRegister( inst, 4, rt+1, uv_deps, sf );

//    //Read the comparison value
//    std::vector<std::list<InternalDependance>> cmp_deps(2);
//    cmp_deps.push_back( update_value.dependances[1]  );
//    addReadXRegister( inst, 1, rs, cmp_deps, sf );
//    addReadXRegister( inst, 2, rs+1, cmp_deps, sf );

    // address dependencies
//    std::list<InternalDependance> addr_dep;
//    addr_dep.push_back( update_address.dependances[0] );
//    addr_dep.push_back( update_value.dependances[2] );  // re-read the value as well b/c may have lost value on earlier read (w/ diff address)
//    addReadXRegister( inst, 1, operands.rs1(), addr_dep );

//    addDestination( inst, operands.rd(), cas);
  }

/*Compare and Swap byte in memory reads an 8-bit byte from memory, and compares it against the value held in a first register. If the comparison
 * is equal, the value in a second register is written to memory. If the write is performed, the read and write occur atomically such that no other
 * modification of the memory location can take place between the read and write.
 */
arminst CAS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    unsigned int sz = extract32(aFetchedOpcode.theOpcode, 30, 1);
    unsigned int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);

    inst->setClass(clsAtomic, codeCAS);

    bool sf = (sz == 1) ? true : false;

    unsigned int size = 8 << sz;

    eAccType stacctype = o0 == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
    eAccType ldacctype = L == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;

    //obtain the loaded values
    predicated_dependant_action cas = casAction( inst, size, kPD );
    inst->addDispatchEffect( allocateCAS( inst, size, cas.dependance, ldacctype) );
    multiply_dependant_action update_address = updateCASAddressAction( inst );
    inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core

    multiply_dependant_action update_value = updateCASValueAction( inst);
    InternalDependance dep( inst->retirementDependance() );
    connectDependance( dep, update_value );

    //Read the value which will be stored
    std::list<InternalDependance> uv_dep;
    uv_dep.push_back( update_value.dependances[0]  );
    addReadXRegister( inst, 3, rt, uv_dep , sf);

    //Read the comparison value
    std::list<InternalDependance> cmp_dep;
    cmp_dep.push_back( update_value.dependances[1]  );
    addReadXRegister( inst, 1, rs, cmp_dep, sf );
}

/*Store-Release Register Byte stores a byte from a 32-bit register to a memory location. The instruction also has memory ordering semantics as
 * described in Load-Acquire, Store-Release. For information about memory accesses, see Load/Store addressing modes.
 */
arminst STLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    unsigned int size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    unsigned int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5); // ignored by load/store single register
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector< std::list<InternalDependance> > rs_deps(2);
    addAddressCompute( inst, rs_deps ) ;

    addReadXRegister(inst, 1, rn, rs_deps[0]);
    addReadXRegister(inst, 2, rt, rs_deps[1]);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, size, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addUpdateData(inst, rt, rn );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, size, inst ) );
    inst->addCommitEffect( commitStore(inst) );

}

/*Load-Acquire Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The instruction also has memory ordering semantics as described in Load-Acquire, Store-Release. For information about memory accesses, see
 * Load/Store addressing modes.
 */
arminst LDAR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    unsigned int size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 1);
    unsigned int rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsLoad, codeLoad);
    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;

    addReadXRegister(inst, 1, rn, rs_deps[0], false);
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, size, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load);

}

/*Load Exclusive Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The memory access is atomic. The PE marks the physical address being accessed as an exclusive access. This exclusive access mark is checked
 * by Store Exclusive instructions. See Synchronization and semaphores. For information about memory accesses see Load/Store addressing
 * modes.
 */
arminst LDXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    eAccType acctype = (o0 == 1) ? kAccType_ORDERED : kAccType_ATOMIC;
    inst->setClass(clsLoad, codeLoadEX);

    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], false);


    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, size, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load);
}

/*Load Register (literal) calculates an address from the PC value and an immediate offset,
 * loads a word from memory, and writes it to a register.
 * For information about memory accesses, see Load/Store addressing modes.
 */
arminst LDR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    int rt = extract32(aFetchedOpcode.thePC, 0, 5);
    int64_t imm = sextract32(aFetchedOpcode.thePC, 5, 19) << 2;
    int opc = extract32(aFetchedOpcode.thePC, 30, 2);
    bool is_signed = false;
    int size = 2;

    eMemOp memop = kMemOp_LOAD;

    switch (opc) {
    case 0:
        size = 4;
        break;
    case 1:
        size = 8;
        break;
    case 2:
        size = 4;
        is_signed = true;
        break;
    case 3:
        memop = kMemOp_PREFETCH;
        break;
    default:
        DBG_Assert(false);
        break;
    }

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addAddressCompute(inst, rs_deps);
    inst->setOperand(kOperand1, aFetchedOpcode.thePC);
    inst->setOperand(kUopAddressOffset, imm);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, size, is_signed ? kSignExtend : kNoExtention, kPD );
    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance, kAccType_NORMAL ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load);

}

/*
 * Load SIMD&FP Register (PC-relative literal). This instruction loads a SIMD&FP register from memory.
 * The address that is used for the load is calculated from the PC value and an immediate offset.
 * Depending on the settings in the CPACR_EL1, CPTR_EL2, and CPTR_EL3 registers, and the current
 * Security state and Exception level, an attempt to execute the instruction might be trapped.
 */
arminst LDRF(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){


    unsigned int rt = extract32(aFetchedOpcode.thePC, 0, 5);
    int64_t imm = sextract32(aFetchedOpcode.thePC, 5, 19) << 2;
    unsigned int opc = extract32(aFetchedOpcode.thePC, 30, 2);
    int size = 2;

    switch (opc) {
    case 0:
        size = 4;
        break;
    case 1:
        size = 8;
        break;
    case 2:
        size = 16;
        break;
    case 3:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    default:
        DBG_Assert(false);
        break;
    }

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    inst->setClass(clsLoad, codeLoadFP);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    addAddressCompute(inst, rs_deps);
    inst->setOperand(kOperand1, aFetchedOpcode.thePC);
    inst->setOperand(kUopAddressOffset, imm);

    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );

    predicated_dependant_action load;
    load = loadFloatingAction( inst, size, kPFD0, kPFD1);

    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance, kAccType_VEC ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

//    addFDestination( inst, rt, size, load);

    return inst;

}
arminst LDRSW(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    
}
arminst PRFM(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);


arminst LDFP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst STFP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDPSW(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo);
arminst LDP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    unsigned int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    uint64_t imm7 = (uint64_t)sextract32(aFetchedOpcode.theOpcode, 15, 7);
    unsigned int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int scale = 2 + (opc & 0x2);
    bool is_signed = ( opc & 1 ) != 0;
    unsigned int size = 8 << scale;

    if (( ((opc & 1) == 1) && L == 0) || opc == 3 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    imm7 <<= scale;

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    eAccType acctype = kAccType_NORMAL;
    std::vector< std::list<InternalDependance> > addr_deps(1), data_deps(2);

    // calculate the address from rn
    std::vector<std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;

    if (index != kSingedOffset){
        predicated_action act = operandAction(inst, kAddress, kResult, kPD);
        addDestination(inst,rn,act);
    }

    addReadXRegister(inst, 1, rn, addr_deps[0], size/2 == 64 ? true : false);
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm7);
    }

    predicated_dependant_action load;
    load = ldpAction( inst, size, is_signed ? kSignExtend : kNoExtention, kPD, kPD1 );
    inst->addDispatchEffect( allocateLoad( inst, size, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addPairDestination( inst, rt, rt2, load);



}
arminst STP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){

    unsigned int opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    unsigned int L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    uint64_t imm7 = (uint64_t)sextract32(aFetchedOpcode.theOpcode, 15, 7);
    unsigned int rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int scale = 2 + (opc & 0x2);
    bool is_signed = ( opc & 1 ) != 0;
    unsigned int size = 8 << scale;

    if (((opc & 1 == 1) && L == 0) || opc == 3 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    imm7 <<= scale;

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    eAccType acctype = kAccType_STREAM;
    std::vector< std::list<InternalDependance> > addr_deps(1), data_deps(2);

    // calculate the address from rn
    std::vector<std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps) ;

    if (index != kSingedOffset){
        predicated_action act = operandAction(inst, kAddress, kResult, kPD);
        addDestination(inst,rn,act);
    }

    addReadXRegister(inst, 1, rn, addr_deps[0], size/2 == 64 ? true : false);
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm7);
    }

    // read data registers
    addReadXRegister(inst, 1, rt, data_deps[1], size/2 == 64 ? true : false);
    addReadXRegister(inst, 2, rt2, data_deps[1], size/2 == 64 ? true : false);

    std::unique_ptr<Operation> ptr =  operation(kCONCAT_);
    simple_action act = addExecute(inst, ptr,data_deps);
    inst->addDispatchAction(act);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, size, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    predicated_dependant_action update_value = updateStoreValueAction( inst, kResult );
    connectDependance( inst->retirementDependance(), update_value );
    inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
    inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );

    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, size, inst ) );
    inst->addCommitEffect( commitStore(inst) );
}



} // narmDecoder
