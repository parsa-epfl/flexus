
#include "Encodings.hpp"

#include "Unallocated.hpp"

namespace nDecoder {

/* C3.1 A64 instruction index by encoding */
archinst
disas_a64_insn(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop, bool &aLastUop)
{
    if (aFetchedOpcode.theOpcode == 1) { // instruction fetch page fault
        return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    }
    DECODER_DBG("#" << aSequenceNo << ": opcode = " << std::hex << aFetchedOpcode.theOpcode << std::dec);

    switch (extract32(aFetchedOpcode.theOpcode, 25, 4)) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3: /* UNALLOCATED */ return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x8:
        case 0x9: /* Data processing - immediate */ return disas_data_proc_imm(aFetchedOpcode, aCPU, aSequenceNo);
        case 0xa:
        case 0xb: /* Branch, exception generation and system insns */
            return disas_b_exc_sys(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x4:
        case 0x6:
        case 0xc:
        case 0xe: /* Loads and stores */ return disas_ldst(aFetchedOpcode, aCPU, aSequenceNo, aUop, aLastUop);
        case 0x5:
        case 0xd: /* Data processing - register */ return disas_data_proc_reg(aFetchedOpcode, aCPU, aSequenceNo);
        case 0x7:
        case 0xf: /* Data processing - SIMD and floating point */
            return disas_data_proc_simd_fp(aFetchedOpcode, aCPU, aSequenceNo);
        default:
            DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should
                                                                            be handled above */
            break;
    }
    DBG_Assert(false, (<< "DECODER: unhandled decoding case!")); /* all 15 cases should
                                                                    be handled above */
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
}

} // namespace nDecoder