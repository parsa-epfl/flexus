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

    bool sf = sz;
    eSize dbsize = (sz == 1) ? kQuadWord : kDoubleWord;

//    eAccType stacctype = o0 == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
    eAccType ldacctype = L == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;

    //obtain the loaded values
    std::vector<std::list<InternalDependance>> rs_deps(1);
    addAddressCompute(inst, rs_deps);
    addReadXRegister(inst, 3, rn, rs_deps[0], true);

    predicated_dependant_action cas;
    if (! is_pair){
        cas = casAction( inst, dbsize, kNoExtention, kPD );
    } else {
        cas = caspAction( inst, dbsize, kNoExtention, kPD, kPD1 );
    }
    inst->addDispatchEffect( allocateCAS( inst, dbsize, cas.dependance, ldacctype) );
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core


    if (! is_pair){
        multiply_dependant_action update_value = updateCASValueAction( inst, kOperand1, kOperand2);

        InternalDependance dep( inst->retirementDependance() );
        connectDependance( dep, update_value );

        //Read the value which will be stored
        std::vector<std::list<InternalDependance>> str_dep(1);
        str_dep[0].push_back( update_value.dependances[0]  );
        addReadXRegister( inst, 2, rt, str_dep[0] , sf);

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
        uv_dep[1].push_back( update_value.dependances[1]  );
        addReadXRegister( inst, 3, rt, uv_dep[0] , sf);
        addReadXRegister( inst, 4, rt, uv_dep[1] , sf);

        //Read the comparison value
        std::vector<std::list<InternalDependance>> cmp_dep(2);
        cmp_dep[0].push_back( update_value.dependances[2]  );
        cmp_dep[1].push_back( update_value.dependances[3]  );
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

    bool o0 = extract32(aFetchedOpcode.theOpcode, 15, 1); // LA-SR
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5); // data
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5); // data
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5); // address
    uint32_t rs = extract32(aFetchedOpcode.theOpcode, 16, 5); // memory status
    bool is_pair = extract32(aFetchedOpcode.theOpcode, 21, 1);
    eSize sz = dbSize(size);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsAtomic, codeStore);
    inst->setExclusive();

    // calculate the address from rn
    eAccType acctype = o0 == 1 ? kAccType_ORDERED : kAccType_ATOMIC;
    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], true);


//    std::vector<std::list<InternalDependance> > status_deps(1);
//    predicated_action act = addExecute(inst, operation(kMOV_), status_deps );
//    addDestination(inst, rs, act, size == 64);

    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( commitStore(inst) );

//    inst->addCommitEffect(exclusiveMonitorPass(inst, kAddress, sz));





    if (! is_pair) {
        std::vector< std::list<InternalDependance> > data_deps(1);
        // read data registers
        simple_action act = addExecute(inst, operation(kMOV_), {kOperand2}, data_deps);
        addReadXRegister(inst, 2, rt, data_deps[0], size/2 == 64);

        predicated_dependant_action update_value = updateStoreValueAction( inst, kResult);
        connectDependance( update_value.dependance, act );
        connectDependance( inst->retirementDependance(), update_value );
//        inst->addPostvalidation( validateMemory( kAddress, kResult, sz, inst ) );

    } else {
        std::vector< std::list<InternalDependance> > data_deps(2);

        // read data registers
        simple_action act = addExecute(inst, operation(size/2 == 64 ? kCONCAT64_ : kCONCAT32_), {kOperand2, kOperand3}, data_deps);
        addReadXRegister(inst, 2, rt2, data_deps[0], size/2 == 64);
        addReadXRegister(inst, 3, rt, data_deps[1], size/2 == 64);

        multiply_dependant_action update_value = updateSTPValueAction( inst, kResult );
        inst->addDispatchEffect( satisfy( inst, update_value.dependances[1]) );
        connectDependance( update_value.dependances[0], act );
        connectDependance( inst->retirementDependance(), update_value );
//        inst->addPostvalidation( validateMemory( kAddress, kResult, sz, inst ) );
    }

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
    uint32_t regsize;
    regsize = (size == 0x3) ? 64 : 32;

    eSize sz = dbSize(size);

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    inst->setClass(clsStore, codeStore);
    inst->setExclusive();

    if (acctype == kAccType_ORDERED ) {
        MEMBAR(inst, /*kMO_ALL | */kBAR_STRL);
    }

    std::vector< std::list<InternalDependance> > rs_deps(1), data_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], true);

    simple_action act = addExecute(inst, operation(kMOV_), data_deps);
    addReadXRegister(inst, 2, rt, data_deps[0], regsize == 64);

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    predicated_dependant_action update_value = updateStoreValueAction(inst, kResult );
    connectDependance( update_value.dependance, act );
    connectDependance( inst->retirementDependance(), update_value );

    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addPostvalidation( validateMemory( kAddress, kResult, sz, inst ) );

    return inst;

}

/*Load-Acquire Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The instruction also has memory ordering semantics as described in Load-Acquire, Store-Release. For information about memory accesses, see
 * Load/Store addressing modes.
 */
arminst LDAQ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t s = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t regsize;
    regsize = (size == 0x3) ? 64 : 32;
    eSize sz = dbSize(size);

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsLoad, codeLoad);
    inst->setExclusive();

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;

    addReadXRegister(inst, 1, rn, rs_deps[0], true);
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    DBG_(Dev, (<< "Loading with size " << sz));

    predicated_dependant_action load;
    load = loadAction( inst, sz, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load, regsize == 64);

    return inst;
}

/*Load Exclusive Register Byte derives an address from a base register value, loads a byte from memory, zero-extends it and writes it to a register.
 * The memory access is atomic. The PE marks the physical address being accessed as an exclusive access. This exclusive access mark is checked
 * by Store Exclusive instructions. See Synchronization and semaphores. For information about memory accesses see Load/Store addressing
 * modes.
 */
arminst LDXR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_pair = extract32(aFetchedOpcode.theOpcode, 21, 1);
    uint32_t regsize;
    regsize = (size == 0x3) ? 64 : 32;
    eSize sz = dbSize(size);


    DECODER_DBG("Loading a " << sz << " " << ((is_pair) ? "as pair" : " ") );

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    eAccType acctype = (o0 == 1) ? kAccType_ORDERED : kAccType_ATOMIC;
    inst->setClass(clsAtomic, codeLoad);
    inst->setExclusive();

    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, rn, rs_deps[0], true);



    predicated_dependant_action load;
    if (!is_pair){
        load = loadAction( inst, sz, kZeroExtend, kPD );
    } else {
        load = ldpAction( inst, sz, kZeroExtend, kPD, kPD1 );
    }

    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

//    inst->addDispatchEffect(markExclusiveMonitor(inst, kRS1, sz));

    if (!is_pair){
        addDestination( inst, rt, load, regsize == 64);
    } else {
        addPairDestination( inst, rt, rt2, load, regsize == 64);
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

    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int64_t imm = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_signed = false;
    uint32_t size = 2;


    int64_t offset = (uint64_t)aFetchedOpcode.thePC + imm - 4;


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
    addReadConstant(inst, 1, 0, rs_deps[0]);
    inst->setOperand(kSopAddressOffset, offset );

    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
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
    int64_t imm7 = sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 +  extract32(opc, 1, 1);
    bool is_signed = ( opc & 1 ) != 0;
    uint32_t size = 8 << (scale+1);
    eSize sz = dbSize(size);

    if (( ((opc & 1) == 1) && L == 0) || opc == 3 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    imm7 <<= scale;

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsStore, codeLDP);

    eAccType acctype = kAccType_NORMAL;
    std::vector< std::list<InternalDependance> > addr_deps(1);

    // calculate the address from rn
    simple_action act = addAddressCompute( inst, addr_deps ) ;
    addReadXRegister(inst, 1, rn, addr_deps[0], true);

    if (index != kSignedOffset){
        std::vector< std::list<InternalDependance> > wb_deps(1);
        predicated_action wback = addExecute(inst, operation(kADD_), {kAddress, kOperand4}, wb_deps, kResult2);
        addReadConstant(inst, 4, ((index==kPostIndex) ? imm7 : 0), wb_deps[0]);
//        wback.action->addDependance(wback.action->dependance(1));
        connectDependance(wback.action->dependance(1), act);
//        wb_deps[0].push_back(act.action->dependance(0));
//        connectDependance(wback.predicate, act);
//        inst->addDispatchEffect( satisfy( inst, wback.action->dependance(1) ) );

        addDestination2(inst, rn, wback, size/2 == 64);

    }

    if (index != kPostIndex) {
        if (index == kUnsignedOffset){
            inst->setOperand(kUopAddressOffset, (uint64_t)imm7 );
            DBG_(Dev, (<<"setting unsigned offset #" << (uint64_t)imm7 ));

        } else {
            inst->setOperand(kSopAddressOffset, imm7 );
            DBG_(Dev, (<<"setting signed offset #" << imm7 ));
        }
    }

    predicated_dependant_action load;
    load = ldpAction( inst, sz, is_signed ? kSignExtend : kNoExtention, kPD, kPD1 );

    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );


    addPairDestination( inst, rt, rt2, load, size/2 == 64);

    return inst;
}
arminst STP(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t L = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    int64_t imm7 = sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2 = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 +  extract32(opc, 1, 1);
//    bool is_signed = ( opc & 1 ) != 0;
    int size = 8 << (scale+1);
    eSize sz = dbSize(size);


    if ((((opc & 1) == 1) && L == 0) || opc == 3 ) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    imm7 <<= scale;

    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsStore, codeStore);

    // calculate the address from rn
    eAccType acctype = kAccType_STREAM;
    std::vector< std::list<InternalDependance> > addr_deps(1), data_deps(2);
    addAddressCompute( inst, addr_deps );

    addReadXRegister(inst, 1, rn, addr_deps[0], true);


    if (index != kSignedOffset){
        std::vector< std::list<InternalDependance> > wb_deps(1);
        predicated_action wback = addExecute(inst, operation(kADD_), {kAddress, kOperand4}, wb_deps, kResult1);
        addReadConstant(inst, 4, ((index==kPostIndex) ? imm7 : 0), wb_deps[0]);
        addDestination1(inst, rn, wback, size/2 == 64);
        inst->addDispatchEffect( satisfy( inst, wback.action->dependance(1) ) );
    }

    if (index != kPostIndex) {
        if (index == kUnsignedOffset){
            inst->setOperand(kUopAddressOffset, (uint64_t)imm7 );
            DBG_(Dev, (<<"setting unsigned offset #" << (uint64_t)imm7 ));

        } else {
            inst->setOperand(kSopAddressOffset, imm7 );
            DBG_(Dev, (<<"setting signed offset #" << imm7 ));
        }
    }


    // read data registers
    simple_action act = addExecute(inst, operation(size/2 == 64 ? kCONCAT64_ : kCONCAT32_), {kOperand2, kOperand3}, data_deps);
    addReadXRegister(inst, 2, rt2, data_deps[0], size/2 == 64);
    addReadXRegister(inst, 3, rt, data_deps[1], size/2 == 64);


    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    multiply_dependant_action update_value = updateSTPValueAction( inst, kResult );
    inst->addDispatchEffect( satisfy( inst, update_value.dependances[1]) );
    connectDependance( update_value.dependances[0], act );
    connectDependance( inst->retirementDependance(), update_value );


    inst->addRetirementEffect( retireMem(inst) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addPostvalidation( validateMemory( kAddress, kResult, sz, inst ) );

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
    int64_t imm = 0;
    uint32_t regsize;
    bool S = extract32(aFetchedOpcode.theOpcode, 12, 1);
    int shift = (S==1) ? size : 0;



    eIndex index = kNoOffset;
//    bool is_signed;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else {
        if (size == 0x3) {
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
        index = kUnsignedOffset;
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
            addDestination( inst, rt, act, regsize == 64);
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
        if (index == kUnsignedOffset){
            inst->setOperand(kUopAddressOffset, (uint64_t)imm );
            DBG_(Dev, (<<"setting unsigned offset " << (uint64_t)imm));

        } else {
            inst->setOperand(kSopAddressOffset, imm );
            DBG_(Dev, (<<"setting signed offset " << imm));
        }
    }

    addReadXRegister(inst, 1, rn, rs_deps[0], true);


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


    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    return inst;


}

/*
 * Load Register (immediate) loads a word or doubleword from memory and writes it to a register. The address that is used for the load is
 * calculated from a base register and an immediate offset. For information about memory accesses, see Load/Store addressing modes. The
 * Unsigned offset variant scales the immediate offset value by the size of the value accessed before adding it to the base register value.
*/
arminst LDR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
    int64_t imm;
    uint32_t regsize;
    eIndex index = kNoOffset;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else {
        if (size == 0x3) {
            return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            if (size == 0x3 && (opc & 1) == 1) {
                return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            }
            regsize = ((opc & 1) == 0x1) ? 32 : 64;
        }
    }

    uint32_t scale = 8 << size;
    eSize sz = dbSize(scale);
    DBG_(Dev, (<<"Size " << sz));


    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsLoad, codeLoad);

    if (extract32(aFetchedOpcode.theOpcode, 24, 1)) {  // Unsigned offset
        index = kUnsignedOffset;
        imm = extract32(aFetchedOpcode.theOpcode, 10, 12) & 0xfff;
        imm <<= size;
    } else {
        index = (extract32(aFetchedOpcode.theOpcode, 11, 1) == 1) ? kPreIndex : kPostIndex;
        imm = sextract32(aFetchedOpcode.theOpcode, 12, 9);
    }

    if (index != kPostIndex){
        if (index == kUnsignedOffset){
            inst->setOperand(kUopAddressOffset, (uint64_t)imm );
            DBG_(Dev, (<<"setting unsigned offset #" << (uint64_t)imm));

        } else {
            inst->setOperand(kSopAddressOffset, imm );
            DBG_(Dev, (<<"setting signed offset #" << imm));
        }
    }

    // write-back to x[n]


    std::vector< std::list<InternalDependance> > rs_deps(1);
    addAddressCompute( inst, rs_deps ) ;

//    if (index != kUnsignedOffset){
//        if (index == kPostIndex){
//            uint64_t offset = ((imm < 0) ? uint64_t(~imm) : uint64_t(imm));
//            rs_deps.resize(2);
//            predicated_action wb = addExecute(inst, operation(kADD_), {kOperand1, kOperand3}, rs_deps, kResult1);
//            addReadConstant(inst, 2, offset, rs_deps[1]);
//            addDestination1(inst, rn, wb, true);
//        } else {
//            predicated_action wb = addExecute(inst, operation(kMOV_), rs_deps, kResult1);
//            addDestination1(inst, rn, wb, true);
//        }
//    }

    addReadXRegister(inst, 1, rn, rs_deps[0], true);


    eAccType acctype = kAccType_NORMAL;
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, sz, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load, regsize == 64);

    return inst;
}

arminst STR_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t option = extract32(aFetchedOpcode.theOpcode, 13, 3);
    uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
    int64_t imm = 0;
    uint32_t regsize;
    bool S = extract32(aFetchedOpcode.theOpcode, 12, 1);
    int shift = (S==1) ? size : 0;



    eIndex index = kNoOffset;
//    bool is_signed;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else {
        if (size == 0x3) {
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
        index = kUnsignedOffset;
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
            addDestination( inst, rt, act, regsize == 64);
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
        if (index == kUnsignedOffset){
            inst->setOperand(kUopAddressOffset, (uint64_t)imm );
            DBG_(Dev, (<<"setting unsigned offset " << (uint64_t)imm));

        } else {
            inst->setOperand(kSopAddressOffset, imm );
            DBG_(Dev, (<<"setting signed offset " << imm));
        }
    }

    addReadXRegister(inst, 1, rn, rs_deps[0], true);


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


    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, sz, false, acctype ) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    return inst;

}

/*
 * Load Register (immediate) loads a word or doubleword from memory and writes it to a register. The address that is used for the load is
 * calculated from a base register and an immediate offset. For information about memory accesses, see Load/Store addressing modes. The
 * Unsigned offset variant scales the immediate offset value by the size of the value accessed before adding it to the base register value.
*/
arminst LDR_reg(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo){
    DECODER_TRACE;
    uint32_t rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rm = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t option = extract32(aFetchedOpcode.theOpcode, 13, 2);
    eExtendType extend_type = DecodeRegExtend(option);
    uint32_t S = extract32(aFetchedOpcode.theOpcode, 12, 1);
    uint32_t shift_amount = (S) ? size : 0;
    uint32_t regsize;
    eIndex index = kNoOffset;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else if (size != 0x3) {
        regsize = (opc & 1) ? 32 : 64;
    }

    uint32_t scale = 8 << size;
    eSize sz = dbSize(scale);
    DBG_(Dev, (<<"Size " << sz));


    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    inst->setClass(clsLoad, codeLoad);

    std::vector< std::list<InternalDependance> > rs_deps(1);
    predicated_action ex = addExecute(inst, extend(extend_type),{kOperand3}, rs_deps, kOperand2 );
    rs_deps.resize(2);
    if(shift_amount && option == 3){
        inst->setOperand(kOperand4, (uint64_t) shift_amount);
        inst->setOperand(kOperand5, (uint64_t) regsize);
        predicated_action sh = addExecute(inst, shift(0/*kLSL_*/),{kOperand2, kOperand4, kOperand5}, rs_deps, kOperand2 );
        inst->addDispatchEffect( satisfy( inst, sh.action->dependance(2)) );
    }

    addAddressCompute( inst, rs_deps ) ;

    if(rn == 31)
        addReadXRegister(inst, 1, rn, rs_deps[0], true);
    else
        readRegister(inst, 1, rn, rs_deps[0], true);
    readRegister(inst, 3, rm, rs_deps[1], true);


    eAccType acctype = kAccType_NORMAL;
    inst->addCheckTrapEffect( mmuPageFaultCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, sz, kZeroExtend, kPD );
    inst->addDispatchEffect( allocateLoad( inst, sz, load.dependance, acctype ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, rt, load, regsize == 64);

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
