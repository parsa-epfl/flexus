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

#include "armEncodings.hpp"
#include "armBranch.hpp"
#include "armUnallocated.hpp"
#include "armMagic.hpp"

namespace narmDecoder {

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
        if (opc == 1 || op2 == 2) { // BLR and RET
            return BLR(aFetchedOpcode, aCPU, aSequenceNo);
        } else {
            return BR(aFetchedOpcode, aCPU, aSequenceNo);
        }

        break;
    case 4: /* ERET */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register) : ERET \033[0m"));
        inst->addPrevalidation( validateLegalReturn(inst) );
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

    case 5: /* DRPS */
        DBG_(Tmp,(<< "\033[1;31m DECODER: Unconditional branch (register) : DRPS \033[0m"));
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);

    default:
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }
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

//            handle_hint(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        case 3: /* CLREX, DSB, DMB, ISB */
            DBG_(Tmp,(<< "\033[1;31m DECODER: CLREX, DSB, DMB, ISB \033[0m"));

//            handle_sync(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        case 4: /* MSR (immediate) */
            DBG_(Tmp,(<< "\033[1;31m DECODER: MSR (immediate)  \033[0m"));

//            handle_msr_i(aFetchedOpcode.theOpcode, op1, op2, crm);
            break;
        default:
            DBG_(Tmp,(<< "\033[1;31m DECODER: unallocated_encoding  \033[0m"));

            unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
            break;
        }
        //return inst;
    }
//    handle_sys(aFetchedOpcode.theOpcode,  l, op0, op1, op2, crn, crm, rt);
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
    if ((aFetchedOpcode.theOpcode & (1 << 4)) || (aFetchedOpcode.theOpcode & (1 << 24))) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    return BCOND(aFetchedOpcode, aCPU, aSequenceNo);
}

/* Test and branch (immediate)
 *   31  30         25  24  23   19 18          5 4    0
 * +----+-------------+----+-------+-------------+------+
 * | b5 | 0 1 1 0 1 1 | op |  b40  |    imm14    |  Rt  |
 * +----+-------------+----+-------+-------------+------+
 */
arminst disas_test_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned op = extract32(aFetchedOpcode.theOpcode, 24, 1); /* 0: TBZ; 1: TBNZ */

    if (op == 0) {
        return TBZ(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        return TBNZ(aFetchedOpcode, aCPU, aSequenceNo);
    }

}

/* Compare and branch (immediate)
 *   31  30         25  24  23                  5 4      0
 * +----+-------------+----+---------------------+--------+
 * | sf | 0 1 1 0 1 0 | op |         imm19       |   Rt   |
 * +----+-------------+----+---------------------+--------+
 */
arminst disas_comp_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    unsigned op = extract32(aFetchedOpcode.theOpcode, 24, 1); /* 0: CBZ; 1: CBNZ */

    if (op == 0) {
        return CBZ(aFetchedOpcode, aCPU, aSequenceNo);
    } else {
        return CBNZ(aFetchedOpcode, aCPU, aSequenceNo);
    }
}

/* Unconditional branch (immediate)
 *   31  30       26 25                                  0
 * +----+-----------+-------------------------------------+
 * | op | 0 0 1 0 1 |                 imm26               |
 * +----+-----------+-------------------------------------+
 */
arminst disas_uncond_b_imm(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    if (aFetchedOpcode.theOpcode & (1U << 31)) {  // BL
        /* Branch with Link branches to a PC-relative offset,
         * setting the register X30 to PC+4.
         * It provides a hint that this is a subroutine call.*/

        return BL(aFetchedOpcode, aCPU, aSequenceNo);
    } else { // B
        return B(aFetchedOpcode, aCPU, aSequenceNo);
    }

}

/* Branches, exception generating and system instructions

 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
+--------+--------+-----------+------------------------------------------------------
|  op0   |   101  |    op1    |
+--------+--------+-----------+-------------------------------------------------------


+--------+-------+
|  op0   |  op1  |
+--------+-------+
|  010   | 0xxx  | Conditional branch (immediate)
|  010   | 1xxx  | UNALLOCATED
|  110   | 00xx  | Exception generation
|  110   | 0100  | System
|  110   | 0101  | UNALLOCATED
|  110   | 011x  | UNALLOCATED
|  110   | 1xxx  | Unconditional branch (register)
|  x00   |       | Unconditional branch (Immediate)
|  x01   | 0xxx  | Compare and branch (immediate)
|  x01   | 1xxx  | Test and branch (immediate)
|  x11   |       | UNALLOCATED

*/

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

} // narmDecoder
