
#include "OperandCode.hpp"

namespace nDecoder {

std::ostream&
operator<<(std::ostream& anOstream, eOperandCode aCode)
{
    const char* operand_codes[] = { "rs1",
                                    "rs2",
                                    "rs3" // Y for div
                                    ,
                                    "rs4",
                                    "rs5",
                                    "fs1_0",
                                    "fs1_1",
                                    "fs2_0",
                                    "fs2_1",
                                    "CCs",
                                    "rd",
                                    "rd1",
                                    "rd2",
                                    "XTRA",
                                    "fd0",
                                    "fd1",
                                    "CCd",
                                    "ps1",
                                    "ps2",
                                    "ps3",
                                    "ps4",
                                    "ps5",
                                    "pfs1_0",
                                    "pfs1_1",
                                    "pfs2_0",
                                    "pfs2_1",
                                    "CCps",
                                    "pd",
                                    "pd1",
                                    "pd2",
                                    "XTRApd",
                                    "pfd0",
                                    "pfd1",
                                    "CCpd",
                                    "ppd",
                                    "ppd1",
                                    "ppd2",
                                    "XTRAppd",
                                    "ppfd0",
                                    "ppfd1",
                                    "pCCpd",
                                    "operand1",
                                    "operand2",
                                    "operand3",
                                    "operand4",
                                    "operand5",
                                    "foperand1_0",
                                    "foperand1_1",
                                    "foperand2_0",
                                    "foperand2_1",
                                    "cc",
                                    "result",
                                    "result1",
                                    "result2",
                                    "fresult0",
                                    "fresult1",
                                    "XTRAout",
                                    "resultcc",
                                    "address",
                                    "status",
                                    "condition",
                                    "storedvalue",
                                    "fpsr",
                                    "fpcr",
                                    "uop_address_offset",
                                    "sop_address_offset" };
    if (aCode >= kLastOperandCode) {
        anOstream << "InvalidOperandCode(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << operand_codes[aCode];
    }
    return anOstream;
}

} // namespace nDecoder
