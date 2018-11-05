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
#include "armSharedFunctions.hpp"

namespace narmDecoder {
using namespace nuArchARM;

// Load/store exclusive

/*Compare and Swap byte in memory reads an 8-bit byte from memory, and compares it against the value held in a first register. If the comparison
 * is equal, the value in a second register is written to memory. If the write is performed, the read and write occur atomically such that no other
 * modification of the memory location can take place between the read and write.
 */
arminst CAS(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
//    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    bool L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    bool sz = extract32(aFetchedOpcode.theOpcode, 30, 1);
    uint32_t rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);


    bool is_pair = ! extract32(aFetchedOpcode.theOpcode, 23, 1);



    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    bool sf = (sz == 1) ? true : false;
    eSize dbsize = (sz == 1) ? kQuadWord : kDoubleWord;


//    eAccType stacctype = o0 == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
    eAccType ldacctype = L == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;

    //obtain the loaded values
    std::vector<std::list<InternalDependance>> rs_deps(1);
    addReadXRegister(inst, 3, rn, rs_deps[0], sz);
    addAddressCompute(inst, rs_deps);

    predicated_dependant_action cas;
    if (! is_pair){
        cas = casAction( inst, dbsize, kPD );
    } else {
        cas = caspAction( inst, dbsize, kPD, kPD1 );
    }
    inst->addDispatchEffect( allocateCAS( inst, dbsize, cas.dependance, ldacctype) );
    multiply_dependant_action update_address = updateCASAddressAction( inst );
    inst->addDispatchEffect( satisfy( inst, update_address.dependances[1] ) );

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core


    if (! is_pair){
        multiply_dependant_action update_value = updateCASValueAction( inst, kOperand1, kOperand2);
        InternalDependance dep( inst->retirementDependance() );
        connectDependance( dep, update_value );

        //Read the value which will be stored
        std::vector<std::list<InternalDependance>> uv_dep(1);
        uv_dep[0].push_back( update_value.dependances[0]  );
        addReadXRegister( inst, 2, rt, uv_dep[0] , sf);

        //Read the comparison value
        std::vector<std::list<InternalDependance>> cmp_dep(1);
        cmp_dep[0].push_back( update_value.dependances[1]  );
        addReadXRegister( inst, 1, rs, cmp_dep[0], sf );
    } else {
        multiply_dependant_action update_value = updateCASPValueAction( inst, kOperand1, kOperand2, kOperand3, kOperand4);
        InternalDependance dep( inst->retirementDependance() );
        connectDependance( dep, update_value );

        //Read the value which will be stored
        std::vector<std::list<InternalDependance>> uv_dep(2);
        uv_dep[0].push_back( update_value.dependances[0]  );
        uv_dep[1].push_back( update_value.dependances[0]  );
        addReadXRegister( inst, 3, rt, uv_dep[0] , sf);
        addReadXRegister( inst, 4, rt, uv_dep[1] , sf);

        //Read the comparison value
        std::vector<std::list<InternalDependance>> cmp_dep(2);
        cmp_dep[0].push_back( update_value.dependances[1]  );
        cmp_dep[1].push_back( update_value.dependances[1]  );
        addReadXRegister( inst, 1, rs, cmp_dep[0], sf );
        addReadXRegister( inst, 2, rs, cmp_dep[1], sf );
    }

    return inst;
}

/* Store Exclusive Register Byte stores a byte from a register to memory
 * if the PE has exclusive access to the memory address, and returns a status
 * value of 0 if the store was successful, or of 1 if no store was performed.
 * See Synchronization and semaphores. The memory access is atomic.
 * For information about memory accesses see Load/Store addressing modes.
 */
arminst STXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1); // LA-SR
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5); // data
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5); // data


    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5); // address
    uint32_t rs = extract32(aFetchedOpcode.theOpcode, 16, 5); // memory status
    bool is_pair = extract32(aFetchedOpcode.theOpcode, 21, 1);


    eSize sz = dbSize(size);

    eAccType acctype = o0 == 1 ? kAccType_ORDERED : kAccType_ATOMIC;


    inst->setClass(clsAtomic, codeStore);
    inst->setExclusive();

    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], size == 64);

    inst->addCommitEffect(exclusiveMonitorPass(inst, kAddress, sz));

    std::vector<std::list<InternalDependance> > status_deps(1);

    predicated_action act = addExecute(inst, operation(kMOV_), status_deps );
    addDestination(inst, rs, act, size == 64);


    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addAnnulmentEffect(eraseLSQ(inst));


    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );


    predicated_dependant_action update_value = updateStoreValueAction( inst, kResult);
    inst->addAnnulmentEffect( satisfy( inst, update_value.predicate ) );
    inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );

    if (! is_pair) {
        setRD( inst, rt);
        inst->addDispatchEffect( mapSource( inst, kRD, kPD ) );
        simple_action read_value = readRegisterAction( inst, kPD, kResult, size == 64);
        inst->addDispatchAction( read_value );
        connectDependance( update_value.dependance, read_value );
        connectDependance( inst->retirementDependance(), update_value );
        inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
        inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );

    } else {
        std::vector<std::list<InternalDependance> > concat_deps(2);

        setRS( inst, kRS1 , rt );
        inst->addDispatchEffect( mapSource( inst, kRS1, kPS1 ) );
        simple_action act = readRegisterAction( inst, kPS1, kOperand1, is_pair ? size/2 == 64 : size == 64 );
        connect( concat_deps[0], act );
        inst->addDispatchAction( act );
    //    connectDependance( update_value.dependance, act );
        setRS( inst, kRS2 , rt2 );
        inst->addDispatchEffect( mapSource( inst, kRS2, kPS2 ) );
        simple_action act2 = readRegisterAction( inst, kPS2, kOperand2, is_pair ? size/2 == 64: size == 64 );
        connect( concat_deps[1], act2 );
        inst->addDispatchAction( act2 );

        connectDependance( inst->retirementDependance(), update_value );
        inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
        inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );

        predicated_action res = addExecute(inst, operation(size == 64 ? kCONCAT64_ : kCONCAT32_), concat_deps);
        connectDependance( update_value.dependance, res );
        inst->addDispatchAction( res );
    }

    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, sz, inst ) );
    inst->addCommitEffect( commitStore(inst) );


    return inst;
}

/*Store-Release Register Byte stores a byte from a 32-bit register to a memory location. The instruction also has memory ordering semantics as
 * described in Load-Acquire, Store-Release. For information about memory accesses, see Load/Store addressing modes.
 */
arminst STRL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
//    uint32_t L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
//    uint32_t rs = extract32(aFetchedOpcode.theOpcode, 16, 5);  // ignored by all loads and store-release
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
//    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5); // ignored by load/store single register
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);

    eSize sz = dbSize(size);

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    inst->setClass(clsStore, codeStore);
    inst->setExclusive();

    if (acctype == kAccType_ORDERED ) {
        MEMBAR(inst, /*kMO_ALL | */kBAR_STRL);
    }

    std::vector< std::list<InternalDependance> > rs_deps(2);
    addAddressCompute( inst, rs_deps ) ;

    addReadXRegister(inst, 1, rn, rs_deps[0], size == 64 ? true : false);
    addReadXRegister(inst, 2, rt, rs_deps[1], size == 64 ? true : false);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

//    addUpdateData(inst, rt, rn );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, sz, inst ) );
    inst->addCommitEffect( commitStore(inst) );

    return inst;

}

/*Load-Acquire Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The instruction also has memory ordering semantics as described in Load-Acquire, Store-Release. For information about memory accesses, see
 * Load/Store addressing modes.
 */
arminst LDAQ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 1);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    eSize sz = dbSize(size);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsLoad, codeLoad);
    inst->setExclusive();

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;

    addReadXRegister(inst, 1, rn, rs_deps[0], false);
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, sz, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load, size == 64);

    return inst;
}

/*Load Exclusive Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The memory access is atomic. The PE marks the physical address being accessed as an exclusive access. This exclusive access mark is checked
 * by Store Exclusive instructions. See Synchronization and semaphores. For information about memory accesses see Load/Store addressing
 * modes.
 */
arminst LDXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_pair = extract32(aFetchedOpcode.theOpcode, 21, 1);
    eSize sz = dbSize(size);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    eAccType acctype = (o0 == 1) ? kAccType_ORDERED : kAccType_ATOMIC;
    inst->setClass(clsAtomic, codeLoad);
    inst->setExclusive();

    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], size == 64);

    inst->addDispatchEffect(markExclusiveMonitor(inst, kRS1, sz));

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    if (!is_pair){
        load = loadAction( inst, sz, kZeroExtend, kPD );
    } else {
        load = ldpAction( inst, sz, kZeroExtend, kPD, kPD1 );
    }
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    if (!is_pair){
    addDestination( inst, rt, load, size == 64);
    } else {
        addPairDestination( inst, rt, rt2, load, size == 64);
    }
    
    return inst;
}

// Load register (literal)
/*Load Register (literal) calculates an address from the PC value and an immediate offset,
 * loads a word from memory, and writes it to a register.
 * For information about memory accesses, see Load/Store addressing modes.
 */
arminst LDR_lit(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;

    uint32_t rt = extract32(aFetchedOpcode.thePC, 0, 5);
    int64_t imm = sextract32(aFetchedOpcode.thePC, 5, 19) << 2;
    uint32_t opc = extract32(aFetchedOpcode.thePC, 30, 2);
    bool is_signed = false;
    uint32_t size = 2;


    int64_t offset = aFetchedOpcode.thePC + (uint64_t) imm - 4;


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

    if (memop == kMemOp_PREFETCH) {
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    }

    size *=8;
    eSize sz = dbSize(size);



    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );


    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addAddressCompute(inst, rs_deps);
    inst->setOperand(kUopAddressOffset, offset);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, sz, is_signed ? kSignExtend : kNoExtention, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, kAccType_NORMAL ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load, size == 64);

    return inst;

}
// Load/store pair (all forms)
arminst LDP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    uint64_t imm7 = (uint64_t)sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 + (opc & 0x2);
    bool is_signed = ( opc & 1 ) != 0;
    uint32_t size = 8 << scale;
    eSize sz = dbSize(size);


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
        predicated_action act = operandAction(inst, kAddress, kResult, (index==kPostIndex) ? imm7 : 0, kPD);
        addDestination(inst,rn,act, size/2 == 64);
    }

    addReadXRegister(inst, 1, rn, addr_deps[0], size/2 == 64);
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm7);
    }

    predicated_dependant_action load;
    load = ldpAction( inst, sz, is_signed ? kSignExtend : kNoExtention, kPD, kPD1 );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addPairDestination( inst, rt, rt2, load, size/2 == 64);

return inst;

}
arminst STP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    uint64_t imm7 = (uint64_t)sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 + (opc & 0x2);
//    bool is_signed = ( opc & 1 ) != 0;
    int size = 8 << scale;
    eSize sz = dbSize(size);


    if ((((opc & 1) == 1) && L == 0) || opc == 3 ) {
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
        predicated_action act = operandAction(inst, kAddress, kResult, (index==kPostIndex) ? imm7 : 0, kPD);
        addDestination(inst,rn,act, size/2 == 64);
    }

    addReadXRegister(inst, 1, rn, addr_deps[0], size/2 == 64);
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm7);
    }

    // read data registers
    addReadXRegister(inst, 1, rt, data_deps[1], size/2 == 64);
    addReadXRegister(inst, 2, rt2, data_deps[1], size/2 == 64);

    simple_action act = addExecute(inst, operation(size/2 == 64 ? kCONCAT64_ : kCONCAT32_),data_deps);
    inst->addDispatchAction(act);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    predicated_dependant_action update_value = updateStoreValueAction( inst, kResult );
    connectDependance( inst->retirementDependance(), update_value );
    inst->addAnnulmentEffect( squash( inst, update_value.predicate ) );
    inst->addReinstatementEffect( satisfy( inst, update_value.predicate ) );

    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, sz, inst ) );
    inst->addCommitEffect( commitStore(inst) );

    return inst;
}

arminst STR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t option = extract32(aFetchedOpcode.theOpcode, 13, 3);
    uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t imm = 0;
    uint32_t regsize;
    bool S = extract32(aFetchedOpcode.theOpcode, 12, 1);
    int shift = (S==1) ? size : 0;



    eIndex index = kNoOffset;
//    bool is_signed;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x4) ? 64 : 32;
    } else {
        if (size == 0x4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            if (size == 0x3 && (opc & 1) == 1) {
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
            regsize = ((opc & 1) == 0x1) ? 32 : 64;
//            is_signed = true;
        }
    }

    size = 8 << size;

    eSize sz = dbSize(size);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    if (extract32(aFetchedOpcode.theOpcode, 24, 1)) {
        index = kUnsingedOffset;
        imm = extract32(aFetchedOpcode.theOpcode, 10, 12);
    } else {
        switch (extract32(aFetchedOpcode.theOpcode, 10, 2)) {
        case 0x0:
            index = kNoOffset;
            break;
        case 0x1:
            index = kPostIndex;
            break;
        case 0x2:
            index = kRegOffset;
            break;
        case 0x3:
            index = kPreIndex;
            break;
        default:
            DBG_Assert(false);
            break;
        }
        if (index !=kRegOffset){
            imm = sextract32(aFetchedOpcode.theOpcode, 12, 9);
        } else if (index == kPostIndex || index == kPreIndex) {
            predicated_action act = operandAction(inst, kAddress, kResult, (index==kPostIndex) ? imm : 0, kPD);
            addDestination( inst, rt, act, size == 64);
        }
    }

    if (index == kRegOffset){
        if ((option & 2) == 0 ){
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        }
    }

    eAccType acctype = kAccType_NORMAL;
    inst->setClass(clsLoad, codeLoad);

    std::vector< std::list<InternalDependance> > rs_deps(1);


    addAddressCompute( inst, rs_deps ) ;
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm);
    }
    addReadXRegister(inst, 1, rn, rs_deps[0], regsize == 64);


    if (index == kRegOffset){
        std::vector< std::list<InternalDependance> > offset_deps(2);
        addReadXRegister(inst, 1, rm, offset_deps[0], regsize == 64);
        std::unique_ptr<Operation> op ;
        if (S) {
            op = operation(kLSL_);
            addReadConstant(inst,2,shift,offset_deps[1]);
        } else {
            op = extend(DecodeRegExtend(option));
        }
        simple_action act = addExecute(inst, std::move(op), offset_deps, kUopAddressOffset, boost::none );
        inst->addDispatchAction(act);
    }


    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    return inst;


}


arminst LDR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t imm;
    uint32_t regsize;
    eIndex index = kNoOffset;
//    bool is_signed;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x4) ? 64 : 32;
    } else {
        if (size == 0x4) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            if (size == 0x3 && (opc & 1) == 1) {
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
            regsize = ((opc & 1) == 0x1) ? 32 : 64;
//            is_signed = true;
        }
    }

    size = 8 << size;
    eSize sz = dbSize(size);


    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    if (extract32(aFetchedOpcode.theOpcode, 24, 1)) {  // Unsigned offset
        index = kUnsingedOffset;
        imm = extract32(aFetchedOpcode.theOpcode, 10, 12);
    } else {
        index = (extract32(aFetchedOpcode.theOpcode, 11, 1) == 1) ? kPreIndex : kPostIndex;
        imm = sextract32(aFetchedOpcode.theOpcode, 12, 9);
        predicated_action act = operandAction(inst, kAddress, kResult, (index==kPostIndex) ? imm : 0, kPD);
        addDestination( inst, rt, act, size == 64);
    }




    eAccType acctype = kAccType_NORMAL;
    inst->setClass(clsLoad, codeLoad);

    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    if (index != kPostIndex) {
        inst->setOperand(kUopAddressOffset, imm);
    }

    addReadXRegister(inst, 1, rn, rs_deps[0], regsize == 64);


    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, sz, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    return inst;
}


// Atomic add on byte in memory atomically loads an 8-bit byte from memory,
// adds the value held in a register to it, and stores the result back to
// memory. The value initially loaded from memory is returned in the destination register.

arminst LDADD(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDCLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDEOR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDSET(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDSMAX(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDSMIN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDUMAX(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst LDUMIN(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }
arminst SWP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)   {DECODER_TRACE; return blackBox(aFetchedOpcode, aCPU, aSequenceNo); DBG_Assert(false); }


} // narmDecoder
