
#include "LoadStore.hpp"

#include "SharedFunctions.hpp"
#include "Unallocated.hpp"
#include "boost/none.hpp"
namespace nDecoder {
using namespace nuArch;

// Load/store exclusive

/*Compare and Swap byte in memory reads an 8-bit byte from memory, and compares
 * it against the value held in a first register. If the comparison is equal,
 * the value in a second register is written to memory. If the write is
 * performed, the read and write occur atomically such that no other
 * modification of the memory location can take place between the read and
 * write.
 */
archinst
CAS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    [[maybe_unused]] SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                                       aFetchedOpcode.theOpcode,
                                                                       aFetchedOpcode.theBPState,
                                                                       aCPU,
                                                                       aSequenceNo));

    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}
//
//    //    uint32_t o0 = extract32(aFetchedOpcode.theOpcode, 15, 1);
//    bool L       = extract32(aFetchedOpcode.theOpcode, 22, 1);
//    bool sz      = extract32(aFetchedOpcode.theOpcode, 30, 1);
//    uint32_t rs  = extract32(aFetchedOpcode.theOpcode, 16,
//                            5); // ignored by all loads and store-release
//    uint32_t rt  = extract32(aFetchedOpcode.theOpcode, 0, 5);
//    uint32_t rn  = extract32(aFetchedOpcode.theOpcode, 5, 5);
//    bool is_pair = !extract32(aFetchedOpcode.theOpcode, 23, 1);
//
//    inst->setClass(clsAtomic, codeCAS);
//    inst->setExclusive();
//
//    bool sf      = sz;
//    eSize dbsize = (sz == 1) ? kQuadWord : kDoubleWord;
//
//    //    eAccType stacctype = o0 == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
//    eAccType ldacctype = L == 1 ? kAccType_ORDEREDRW : kAccType_ATOMICRW;
//
//    // obtain the loaded values
//    std::vector<std::list<InternalDependance>> rs_deps(1);
//    addAddressCompute(inst, rs_deps);
//    addReadXRegister(inst, 3, rn, rs_deps[0], true);
//
//    predicated_dependant_action cas;
//    if (!is_pair) {
//        cas = casAction(inst, dbsize, kNoExtension, kPD);
//    } else {
//        cas = caspAction(inst, dbsize, kNoExtension, kPD, kPD1);
//    }
//    inst->addDispatchEffect(allocateCAS(inst, dbsize, cas.dependance, ldacctype));
//    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
//    inst->addRetirementEffect(retireMem(inst));
//    inst->addSquashEffect(eraseLSQ(inst));
//    inst->addCommitEffect(accessMem(inst));
//    inst->setMayCommit(false); // Can't commit till memory-order speculation is
//                               // resolved by the core
//
//    if (!is_pair) {
//        multiply_dependant_action update_value = updateCASValueAction(inst, kOperand1, kOperand2);
//
//        InternalDependance dep(inst->retirementDependance());
//        connectDependance(dep, update_value);
//
//        // Read the value which will be stored
//        std::vector<std::list<InternalDependance>> str_dep(1);
//        str_dep[0].push_back(update_value.dependances[0]);
//        addReadXRegister(inst, 2, rt, str_dep[0], sf);
//
//        // Read the comparison value
//        std::vector<std::list<InternalDependance>> cmp_dep(1);
//        cmp_dep[0].push_back(update_value.dependances[1]);
//        addReadXRegister(inst, 1, rs, cmp_dep[0], sf);
//
//    } else {
//        multiply_dependant_action update_value =
//          updateCASPValueAction(inst, kOperand1, kOperand2, kOperand3, kOperand4);
//        InternalDependance dep(inst->retirementDependance());
//        connectDependance(dep, update_value);
//
//        // Read the value which will be stored
//        std::vector<std::list<InternalDependance>> uv_dep(2);
//        uv_dep[0].push_back(update_value.dependances[0]);
//        uv_dep[1].push_back(update_value.dependances[1]);
//        addReadXRegister(inst, 3, rt, uv_dep[0], sf);
//        addReadXRegister(inst, 4, rt, uv_dep[1], sf);
//
//        // Read the comparison value
//        std::vector<std::list<InternalDependance>> cmp_dep(2);
//        cmp_dep[0].push_back(update_value.dependances[2]);
//        cmp_dep[1].push_back(update_value.dependances[3]);
//        addReadXRegister(inst, 1, rs, cmp_dep[0], sf);
//        addReadXRegister(inst, 2, rs, cmp_dep[1], sf);
//    }
//
//    return inst;
//}

/* Store Exclusive Register Byte stores a byte from a register to memory
 * if the PE has exclusive access to the memory address, and returns a status
 * value of 0 if the store was successful, or of 1 if no store was performed.
 * See Synchronization and semaphores. The memory access is atomic.
 * For information about memory accesses see Load/Store addressing modes.
 */
archinst
STXR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    bool o0       = extract32(aFetchedOpcode.theOpcode, 15, 1); // LA-SR
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt   = extract32(aFetchedOpcode.theOpcode, 0, 5);  // data
    uint32_t rt2  = extract32(aFetchedOpcode.theOpcode, 10, 5); // data
    uint32_t rn   = extract32(aFetchedOpcode.theOpcode, 5, 5);  // address
    uint32_t rs   = extract32(aFetchedOpcode.theOpcode, 16, 5); // memory status
    bool is_pair  = extract32(aFetchedOpcode.theOpcode, 21, 1);
    eSize sz      = dbSize(size);

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    // calculate the address from rn
    eAccType acctype = o0 == 1 ? kAccType_ORDERED : kAccType_ATOMIC;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    simple_action addr = addAddressCompute(inst, rs_deps);
    addReadXRegister(inst, 1, rn, rs_deps[0], true);

    predicated_action monitor = exclusiveMonitorAction(inst, kAddress, sz, kPD);
    connectDependance(monitor.action->dependance(0), addr);
    addDestination(inst, rs, monitor, size == 64);

    inst->addSquashEffect(eraseLSQ(inst));
    // inst->addDispatchEffect(allocateStore(inst, sz, false, acctype));
    inst->addDispatchEffect(allocateCAS(inst, sz, inst->retirementDependance(), acctype));
    // inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    // inst->addRetirementConstraint(sideEffectStoreConstraint(inst));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(commitStore(inst));
    inst->addCommitEffect(clearExclusiveMonitor(inst));

    if (!is_pair) {
        std::vector<std::list<InternalDependance>> data_deps(1);
        // read data registers
        simple_action act = addExecute(inst, operation(kMOV_), { kOperand2 }, data_deps, kOperand5);
        readRegister(inst, 2, rt, data_deps[0], size == 64);

        inst->setOperand(kOperand4, (uint64_t)kCheckAndStore);
        // predicated_dependant_action update_value = updateStoreValueAction(inst, kOperand5);
        multiply_dependant_action update_value = updateCASValueAction(inst, kOperand4, kOperand5);
        connectDependance(update_value.dependances[0], addr);
        connectDependance(update_value.dependances[1], monitor);
        connectDependance(update_value.dependances[2], act);
        connectDependance(inst->retirementDependance(), update_value);
        inst->addPostvalidation(validateMemory(kAddress, kOperand5, sz, inst));

    } else {
        std::vector<std::list<InternalDependance>> data_deps(2);

        // read data registers
        simple_action act =
          addExecute(inst, operation(size == 64 ? kCONCAT64_ : kCONCAT32_), { kOperand2, kOperand3 }, data_deps);
        readRegister(inst, 2, rt2, data_deps[0], size == 64);
        readRegister(inst, 3, rt, data_deps[1], size == 64);

        multiply_dependant_action update_value = updateSTPValueAction(inst, kResult);
        inst->addDispatchEffect(satisfy(inst, update_value.dependances[1]));
        connectDependance(update_value.dependances[0], act);
        connectDependance(inst->retirementDependance(), update_value);
        inst->addPostvalidation(validateMemory(kAddress, kResult, sz, inst));
    }

    return inst;
}

/*Store-Release Register Byte stores a byte from a 32-bit register to a memory
 * location. The instruction also has memory ordering semantics as described in
 * Load-Acquire, Store-Release. For information about memory accesses, see
 * Load/Store addressing modes.
 */
archinst
STRL(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    bool o0          = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t size    = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt      = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn      = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t regsize = (size == 0x3) ? 64 : 32;

    eSize sz = dbSize(8 << size);

    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector<std::list<InternalDependance>> rs_deps(1), data_deps(1);
    simple_action addr = addAddressCompute(inst, rs_deps);
    readRegister(inst, 1, rn, rs_deps[0], true);

    inst->setOperand(kOperand4, (uint64_t)kAlwaysStore);
    simple_action act = addExecute(inst, operation(kMOV_), { kOperand2 }, data_deps, kOperand5);
    readRegister(inst, 2, rt, data_deps[0], regsize == 64);

    // predicated_dependant_action update_value = updateStoreValueAction(inst, kOperand5);
    multiply_dependant_action update_value = updateCASValueAction(inst, kOperand4, kOperand5);
    connectDependance(update_value.dependances[0], addr);
    connectDependance(update_value.dependances[1], act);
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[2]));
    connectDependance(inst->retirementDependance(), update_value);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(commitStore(inst));
    // inst->setMayCommit(false);
    // Can't commit till memory-order speculation is resolved by the core

    // inst->addDispatchEffect(allocateStore(inst, sz, false, acctype));
    inst->addDispatchEffect(allocateCAS(inst, sz, inst->retirementDependance(), acctype));
    // inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    // inst->addRetirementConstraint(sideEffectStoreConstraint(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOperand5, sz, inst));
    return inst;
}

/*Load-Acquire Register Byte derives an address from a base register value,
 * loads a byte from memory, zero-extends it and writes it to a register. The
 * instruction also has memory ordering semantics as described in Load-Acquire,
 * Store-Release. For information about memory accesses, see Load/Store
 * addressing modes.
 */
archinst
LDAQ(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t o0   = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t rt   = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rn   = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t regsize;
    regsize  = (size == 0x3) ? 64 : 32;
    eSize sz = dbSize(size);

    DBG_(VVerb, (<< "Loading with size " << sz));

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    eAccType acctype = o0 == 0 ? kAccType_LIMITEDORDERED : kAccType_ORDERED;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    simple_action addr = addAddressCompute(inst, rs_deps);
    addReadXRegister(inst, 1, rn, rs_deps[0], true);
    predicated_dependant_action load = loadAction(inst, sz, kZeroExtend, kPD);

    // SID: treating as CAS because making it a barrier is harder
    inst->setOperand(kOperand4, (uint64_t)kNeverStore);
    multiply_dependant_action update_value = updateCASValueAction(inst, kOperand4, kOperand4);
    connectDependance(update_value.dependances[0], addr);
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[1]));
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[2]));
    connectDependance(inst->retirementDependance(), update_value);

    // inst->addDispatchEffect(allocateLoad(inst, sz, load.dependance, acctype));
    inst->addDispatchEffect(allocateCAS(inst, sz, load.dependance, acctype));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(accessMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    addDestination(inst, rt, load, regsize == 64);

    return inst;
}

/*Load Exclusive Register Byte derives an address from a base register value,
 * loads a byte from memory, zero-extends it and writes it to a register. The
 * memory access is atomic. The PE marks the physical address being accessed as
 * an exclusive access. This exclusive access mark is checked by Store Exclusive
 * instructions. See Synchronization and semaphores. For information about
 * memory accesses see Load/Store addressing modes.
 */
archinst
LDXR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t rt   = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t rt2  = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t o0   = extract32(aFetchedOpcode.theOpcode, 15, 1);
    uint32_t rn   = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t size = 8 << extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_pair  = extract32(aFetchedOpcode.theOpcode, 21, 1);
    eSize sz      = dbSize(size);

    DECODER_DBG("Loading a " << sz << " " << ((is_pair) ? "as pair" : " "));

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsAtomic, codeCAS);
    inst->setExclusive();

    eAccType acctype = (o0 == 1) ? kAccType_ORDERED : kAccType_ATOMIC;

    std::vector<std::list<InternalDependance>> rs_deps(1);
    simple_action addr = addAddressCompute(inst, rs_deps);
    addReadXRegister(inst, 1, rn, rs_deps[0], true);

    predicated_dependant_action load;
    if (!is_pair) {
        load = loadAction(inst, sz, kZeroExtend, kPD);
    } else {
        load = ldpAction(inst, sz, kZeroExtend, kPD, kPD1);
    }

    inst->setOperand(kOperand4, (uint64_t)kNeverStore);
    multiply_dependant_action update_value = updateCASValueAction(inst, kOperand4, kOperand4);
    connectDependance(update_value.dependances[0], addr);
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[1]));
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[2]));
    connectDependance(inst->retirementDependance(), update_value);

    // inst->addDispatchEffect(allocateLoad(inst, sz, load.dependance, acctype));
    inst->addDispatchEffect(allocateCAS(inst, sz, load.dependance, acctype));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(accessMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));
    inst->addRetirementEffect(markExclusiveMonitor(inst, kAddress, sz));

    if (!is_pair) {
        addDestination(inst, rt, load, size == 64);
    } else {
        addPairDestination(inst, rt, rt2, load, size == 64);
    }

    return inst;
}

// Load register (literal)
/*Load Register (literal) calculates an address from the PC value and an
 * immediate offset, loads a word from memory, and writes it to a register. For
 * information about memory accesses, see Load/Store addressing modes.
 */
archinst
LDR_lit(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;

    uint32_t rt    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    int64_t imm    = sextract32(aFetchedOpcode.theOpcode, 5, 19) << 2;
    uint32_t opc   = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool is_signed = false;
    uint32_t size  = 2;

    int64_t offset = (uint64_t)aFetchedOpcode.thePC + imm;

    eMemOp memop = kMemOp_LOAD;

    switch (opc) {
        case 0: size = 4; break;
        case 1: size = 8; break;
        case 2:
            size      = 4;
            is_signed = true;
            break;
        case 3: memop = kMemOp_PREFETCH; break;
        default: DBG_Assert(false); break;
    }

    if (memop == kMemOp_PREFETCH) { return blackBox(aFetchedOpcode, aCPU, aSequenceNo); }

    size *= 8;
    eSize sz = dbSize(size);

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsLoad, codeLoad);

    std::vector<std::list<InternalDependance>> rs_deps(1);
    addAddressCompute(inst, rs_deps);
    addReadConstant(inst, 1, 0, rs_deps[0]);
    inst->setOperand(kSopAddressOffset, offset);

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));

    predicated_dependant_action load;
    load = loadAction(inst, sz, is_signed ? kSignExtend : kNoExtension, kPD);
    inst->addDispatchEffect(allocateLoad(inst, sz, load.dependance, kAccType_NORMAL));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    addDestination(inst, rt, load, size == 64);

    return inst;
}
// Load/store pair (all forms)
archinst
LDP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t opc   = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool L         = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index   = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    int64_t imm7   = sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2   = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn    = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 + extract32(opc, 1, 1);
    /* Msutherl:
     * Sections C6.2.117 and C6.2.118 distinguish between LDP and LDPSW by the following:
     *    - all LDPs have opc = x0
     *    - all LDPSW has opc = 01
     */
    bool is_signed = (opc & 0x1) == 0x1;
    uint32_t size  = 8 << (scale + 1);
    eSize sz       = dbSize(size);

    if ((((opc & 1) == 1) && L == 0) || opc == 3 || (is_signed && index == kNoOffset)) {
        /* Msutherl: if signed, only valid encodings are preindex, postindex, imm-index */
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
    if (rt == rt2 || ((index == kPreIndex || index == kPostIndex) && (rn == rt || rn == rt2))) {
        // Constrain unpredictable: C6.2.129 LDP Operation
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    imm7 <<= scale;

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsLoad, codeLDP);

    eAccType acctype = kAccType_NORMAL;
    std::vector<std::list<InternalDependance>> addr_deps(1);

    // calculate the address from rn
    simple_action act = addAddressCompute(inst, addr_deps);
    addReadXRegister(inst, 1, rn, addr_deps[0], true);

    if (index == kPreIndex || index == kPostIndex) {
        // C6.2.129 LDP PostIndex and PreIndex cause writeback
        std::vector<std::list<InternalDependance>> wb_deps(1);
        predicated_action wback = addExecute(inst, operation(kADD_), { kAddress, kOperand4 }, wb_deps, kResult2);
        addReadConstant(inst, 4, ((index == kPostIndex) ? imm7 : 0), wb_deps[0]);
        //        wback.action->addDependance(wback.action->dependance(1));
        connectDependance(wback.action->dependance(1), act);
        //        wb_deps[0].push_back(act.action->dependance(0));
        //        connectDependance(wback.predicate, act);
        //        inst->addDispatchEffect( satisfy( inst,
        //        wback.action->dependance(1) ) );

        addDestination2(inst, rn, wback, size / 2 == 64);
    }

    if (index != kPostIndex) {
        if (index == kUnsignedOffset) {
            inst->setOperand(kUopAddressOffset, (uint64_t)imm7);
            DBG_(VVerb, (<< "setting unsigned offset #" << (uint64_t)imm7));

        } else {
            inst->setOperand(kSopAddressOffset, imm7);
            DBG_(VVerb, (<< "setting signed offset #" << imm7));
        }
    }

    predicated_dependant_action load;
    load = ldpAction(inst, sz, is_signed ? kSignExtend : kNoExtension, kPD, kPD1);

    inst->addDispatchEffect(allocateLoad(inst, sz, load.dependance, acctype));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(accessMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    addPairDestination(inst, rt, rt2, load, size / 2 == 64);

    return inst;
}
archinst
STP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t opc   = extract32(aFetchedOpcode.theOpcode, 30, 2);
    uint32_t L     = extract32(aFetchedOpcode.theOpcode, 22, 1);
    eIndex index   = getIndex(extract32(aFetchedOpcode.theOpcode, 23, 3));
    int64_t imm7   = sextract32(aFetchedOpcode.theOpcode, 15, 7);
    uint32_t rt2   = extract32(aFetchedOpcode.theOpcode, 10, 5);
    uint32_t rn    = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rt    = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t scale = 2 + extract32(opc, 1, 1);
    //    bool is_signed = ( opc & 1 ) != 0;
    int size = 8 << (scale + 1);
    eSize sz = dbSize(size);

    if ((((opc & 1) == 1) && L == 0) || opc == 3) { return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo); }

    imm7 <<= scale;

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));

    inst->setClass(clsStore, codeStore);

    // calculate the address from rn
    eAccType acctype = kAccType_STREAM;
    std::vector<std::list<InternalDependance>> addr_deps(1), data_deps(2);
    simple_action addr = addAddressCompute(inst, addr_deps);

    addReadXRegister(inst, 1, rn, addr_deps[0], true);

    if (index == kPreIndex || index == kPostIndex) {
        // C6.2.273 STP PostIndex and PreIndex cause writeback
        std::vector<std::list<InternalDependance>> wb_deps(2);
        predicated_action wback = addExecute(inst, operation(kADD_), { kAddress, kOperand4 }, wb_deps, kResult1);
        connect(wb_deps[1], addr);
        addReadConstant(inst, 4, ((index == kPostIndex) ? imm7 : 0), wb_deps[0]);
        addDestination1(inst, rn, wback, size / 2 == 64);
    }

    if (index != kPostIndex) {
        if (index == kUnsignedOffset) {
            inst->setOperand(kUopAddressOffset, (uint64_t)imm7);
            DBG_(VVerb, (<< "setting unsigned offset #" << (uint64_t)imm7));

        } else {
            inst->setOperand(kSopAddressOffset, imm7);
            DBG_(VVerb, (<< "setting signed offset #" << imm7));
        }
    }

    // read data registers
    simple_action act =
      addExecute(inst, operation(size / 2 == 64 ? kCONCAT64_ : kCONCAT32_), { kOperand2, kOperand3 }, data_deps);
    readRegister(inst, 2, rt2, data_deps[0], size / 2 == 64);
    readRegister(inst, 3, rt, data_deps[1], size / 2 == 64);

    inst->addDispatchEffect(allocateStore(inst, sz, false, acctype));
    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstraint(inst));

    multiply_dependant_action update_value = updateSTPValueAction(inst, kResult);
    inst->addDispatchEffect(satisfy(inst, update_value.dependances[1]));
    connectDependance(update_value.dependances[0], act);
    connectDependance(inst->retirementDependance(), update_value);

    inst->addRetirementEffect(retireMem(inst));
    inst->addCommitEffect(commitStore(inst));
    inst->addSquashEffect(eraseLSQ(inst));

    inst->addPostvalidation(validateMemory(kAddress, kResult, sz, inst));

    return inst;
}

archinst
STR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t rt           = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc          = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn           = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rm           = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t option       = extract32(aFetchedOpcode.theOpcode, 13, 3);
    uint32_t size         = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool S                = extract32(aFetchedOpcode.theOpcode, 12, 1);
    uint32_t shift_amount = (S) ? size : 0;
    bool is_unsigned      = extract32(aFetchedOpcode.theOpcode, 24, 1);
    uint64_t imm = 0, regsize = 0;
    eIndex index = kNoOffset;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else if (size != 0x3) {
        regsize = (opc & 1) ? 32 : 64;
    }

    eSize sz = dbSize(8 << size);
    DBG_(VVerb, (<< "Size " << sz));

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    inst->setClass(clsStore, codeStore);
    eAccType acctype = kAccType_NORMAL;

    if (is_unsigned) {
        index = kUnsignedOffset;
        imm   = extract32(aFetchedOpcode.theOpcode, 10, 12);
        imm <<= size;
    } else {
        switch (extract32(aFetchedOpcode.theOpcode, 10, 2)) {
            case 0x0: index = kNoOffset; break;
            case 0x1: index = kPostIndex; break;
            case 0x2: index = kRegOffset; break;
            case 0x3: index = kPreIndex; break;
        }
        if (index != kRegOffset) { imm = (int64_t)sextract32(aFetchedOpcode.theOpcode, 12, 9); }
    }

    std::vector<std::list<InternalDependance>> rs_deps(1), rs2_deps(1), rs3_deps(1);
    predicated_action ex, sh, wb;

    if (index == kRegOffset) {
        ex = addExecute(inst, extend(DecodeRegExtend(option)), { kOperand3 }, rs_deps, kOperand2);
        rs_deps.resize(2);
        rs2_deps.resize(2);
        if (shift_amount) {
            inst->setOperand(kResult2, (uint64_t)shift_amount);
            inst->setOperand(kOperand4, (uint64_t)regsize);
            sh = addExecute(inst, operation(kLSL_), { kOperand2, kResult2, kOperand4 }, rs_deps, kOperand2);
            connect(rs_deps[1], ex);
            inst->addDispatchEffect(satisfy(inst, sh.action->dependance(2)));
        }
    }

    simple_action act = addAddressCompute(inst, rs2_deps);
    if (index == kRegOffset) {
        if (shift_amount)
            connect(rs2_deps[1], sh);
        else
            connect(rs2_deps[1], ex);
    }

    if (index == kUnsignedOffset) {
        inst->setOperand(kUopAddressOffset, imm);
        DBG_(VVerb, (<< "setting unsigned offset " << std::hex << imm));
    } else if (index == kNoOffset) {
        inst->setOperand(kSopAddressOffset, (int64_t)imm);
        DBG_(VVerb, (<< "setting signed offset no offset " << std::hex << imm));
    } else if (index == kPreIndex) {
        inst->setOperand(kSopAddressOffset, (int64_t)imm);
        DBG_(VVerb, (<< "setting signed offset preindex " << std::hex << imm));
        wb = operandAction(inst, kAddress, kResult1, imm, kPD1);
        connectDependance(wb.action->dependance(0), act);
    } else if (index == kPostIndex) {
        inst->setOperand(kOperand3, imm);
        DBG_(VVerb, (<< "setting signed offset postindex " << std::hex << imm));
        wb = addExecute(inst, operation(kADD_), { kOperand1, kOperand3 }, rs2_deps, kResult1);
        inst->addDispatchEffect(satisfy(inst, wb.action->dependance(1)));
    }

    predicated_dependant_action update_value = updateStoreValueAction(inst, kOperand5);
    rs3_deps[0].push_back(update_value.dependance);
    connectDependance(inst->retirementDependance(), update_value);

    if (rn == 31)
        addReadXRegister(inst, 1, rn, rs2_deps[0], true);
    else
        readRegister(inst, 1, rn, rs2_deps[0], true);
    readRegister(inst, 5, rt, rs3_deps[0], regsize == 64);

    if (index == kRegOffset) readRegister(inst, 3, rm, rs_deps[0], true);

    if (index == kPostIndex || index == kPreIndex) { addDestination1(inst, rn, wb, true); }

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));
    inst->addCommitEffect(commitStore(inst));

    inst->addDispatchEffect(allocateStore(inst, sz, false, acctype));
    inst->addRetirementConstraint(storeQueueAvailableConstraint(inst));
    inst->addRetirementConstraint(sideEffectStoreConstraint(inst));
    inst->addPostvalidation(validateMemory(kAddress, kOperand5, sz, inst));

    return inst;
}

/*
 * Load Register (immediate) loads a word or doubleword from memory and writes
 * it to a register. The address that is used for the load is calculated from a
 * base register and an immediate offset. For information about memory accesses,
 * see Load/Store addressing modes. The Unsigned offset variant scales the
 * immediate offset value by the size of the value accessed before adding it to
 * the base register value.
 */
archinst
LDR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    uint32_t rt           = extract32(aFetchedOpcode.theOpcode, 0, 5);
    uint32_t opc          = extract32(aFetchedOpcode.theOpcode, 22, 2);
    uint32_t rn           = extract32(aFetchedOpcode.theOpcode, 5, 5);
    uint32_t rm           = extract32(aFetchedOpcode.theOpcode, 16, 5);
    uint32_t option       = extract32(aFetchedOpcode.theOpcode, 13, 3);
    uint32_t size         = extract32(aFetchedOpcode.theOpcode, 30, 2);
    bool S                = extract32(aFetchedOpcode.theOpcode, 12, 1);
    uint32_t shift_amount = (S) ? size : 0;
    bool is_unsigned      = extract32(aFetchedOpcode.theOpcode, 24, 1);
    uint64_t imm = 0, regsize = 0;
    eIndex index = kNoOffset;

    if ((opc & 0x2) == 0) {
        regsize = (size == 0x3) ? 64 : 32;
    } else if (size != 0x3) {
        regsize = (opc & 1) ? 32 : 64;
    }

    eSize sz = dbSize(8 << size);
    DBG_(VVerb, (<< "Size " << sz));

    if (is_unsigned) {
        index = kUnsignedOffset;
        imm   = extract32(aFetchedOpcode.theOpcode, 10, 12);
        imm <<= size;
    } else {
        switch (extract32(aFetchedOpcode.theOpcode, 10, 2)) {
            case 0x0: index = kNoOffset; break;
            case 0x1: index = kPostIndex; break;
            case 0x2: index = kRegOffset; break;
            case 0x3: index = kPreIndex; break;
        }
        if (index != kRegOffset) { imm = (int64_t)sextract32(aFetchedOpcode.theOpcode, 12, 9); }
    }

    if ((index == kPreIndex || index == kPostIndex) && (rn == rt)) {
        // Constrain unpredictable: C6.2.131 LDR Operation
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    inst->setClass(clsLoad, codeLoad);
    eAccType acctype = kAccType_NORMAL;

    std::vector<std::list<InternalDependance>> rs_deps(1), rs2_deps(1);
    predicated_action ex, sh, wb;

    if (index == kRegOffset) {
        ex = addExecute(inst, extend(DecodeRegExtend(option)), { kOperand3 }, rs_deps, kOperand2);
        rs_deps.resize(2);
        rs2_deps.resize(2);
        if (shift_amount) {
            inst->setOperand(kResult2, (uint64_t)shift_amount);
            inst->setOperand(kOperand4, (uint64_t)regsize);
            sh = addExecute(inst, operation(kLSL_), { kOperand2, kResult2, kOperand4 }, rs_deps, kOperand2);
            connect(rs_deps[1], ex);
            inst->addDispatchEffect(satisfy(inst, sh.action->dependance(2)));
        }
    }

    simple_action act = addAddressCompute(inst, rs2_deps);
    if (index == kRegOffset) {
        if (shift_amount)
            connect(rs2_deps[1], sh);
        else
            connect(rs2_deps[1], ex);
    }

    if (index == kUnsignedOffset) {
        inst->setOperand(kUopAddressOffset, imm);
        DBG_(VVerb, (<< "setting unsigned offset " << std::hex << imm));
    } else if (index == kNoOffset) {
        inst->setOperand(kSopAddressOffset, (int64_t)imm);
        DBG_(VVerb, (<< "setting signed offset no offset " << std::hex << imm));
    } else if (index == kPreIndex) {
        inst->setOperand(kSopAddressOffset, (int64_t)imm);
        DBG_(VVerb, (<< "setting signed offset preindex " << std::hex << imm));
        wb = operandAction(inst, kAddress, kResult1, imm, kPD1);
        connectDependance(wb.action->dependance(0), act);
    } else if (index == kPostIndex) {
        inst->setOperand(kOperand3, imm);
        DBG_(VVerb, (<< "setting signed offset postindex " << std::hex << imm));
        wb = addExecute(inst, operation(kADD_), { kOperand1, kOperand3 }, rs2_deps, kResult1);
        inst->addDispatchEffect(satisfy(inst, wb.action->dependance(1)));
    }

    if (rn == 31)
        addReadXRegister(inst, 1, rn, rs2_deps[0], true);
    else
        readRegister(inst, 1, rn, rs2_deps[0], true);

    if (index == kRegOffset) readRegister(inst, 3, rm, rs_deps[0], true);

    if (index == kPostIndex || index == kPreIndex) { addDestination1(inst, rn, wb, true); }

    inst->addCheckTrapEffect(mmuPageFaultCheck(inst));
    inst->addRetirementEffect(retireMem(inst));
    inst->addSquashEffect(eraseLSQ(inst));

    predicated_dependant_action load;
    // rt == 31 means LDR XZR, which has no destination. Don't have a bypass register for it.
    if (rt != 31)
        load = loadAction(inst, sz, extract32(opc, 1, 1) ? kSignExtend : kZeroExtend, kPD);
    else
        load = loadAction(inst, sz, extract32(opc, 1, 1) ? kSignExtend : kZeroExtend, boost::none);
    inst->addDispatchEffect(allocateLoad(inst, sz, load.dependance, acctype));
    inst->addCommitEffect(accessMem(inst));
    inst->addRetirementConstraint(loadMemoryConstraint(inst));

    // destination register == 31 means LDR XZR, which is a NOP
    // Normally register 31 is SP, but it cannot be used as a destination of LDR
    if (rt != 31)
        addDestination(inst, rt, load, regsize == 64);

    return inst;
}

// Atomic add on byte in memory atomically loads an 8-bit byte from memory,
// adds the value held in a register to it, and stores the result back to
// memory. The value initially loaded from memory is returned in the destination
// register.

archinst
LDADD(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDCLR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDEOR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDSET(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDSMAX(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDSMIN(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDUMAX(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
LDUMIN(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}
archinst
SWP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    DBG_Assert(false);
}

} // namespace nDecoder
