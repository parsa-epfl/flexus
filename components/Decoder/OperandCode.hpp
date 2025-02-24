
#ifndef FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED
#define FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED

#include <iostream>

namespace nDecoder {

#define SIGNED_UPPER_BOUND_B 0xFFFFFFFFFFFFFF00ULL
#define SIGNED_UPPER_BOUND_H 0xFFFFFFFFFFFF0000ULL
#define SIGNED_UPPER_BOUND_W 0xFFFFFFFF00000000ULL
#define SIGNED_UPPER_BOUND_X 0x0000000000000000ULL

enum eOpType
{
    kADD_,
    kADDS32_,
    kADDS64_,
    kCONCAT32_,
    kCONCAT64_,
    kAND_,
    kANDS_,
    kANDSN_,
    kORR_,
    kXOR_,

    kSUB_,
    kSUBS64_,
    kSUBS32_,
    kAndN_,
    kOrN_,
    kEoN_,

    kXnor_,
    kMulX_,
    kUMul_,
    kUMulH_,
    kUMulL_,
    kSMul_,
    kSMulH_,
    kSMulL_,
    kUDivX_,
    kUDiv_,
    kSDiv_,
    kSDivX_,
    kMovCC_,
    kMOV_,
    kMOVN_,
    kMOVK_,

    kSextB_,
    kSextH_,
    kSextW_,
    kSextX_,
    kZextB_,
    kZextH_,
    kZextW_,
    kZextX_,

    kNot_,
    kTCC_,
    kAlign_,
    kAlignLittle_,
    kULONG2ICC_,
    kICC2ULONG_,
    kAddTrunc_,

    kROR_,
    kASR_,
    kLSR_,
    kLSL_,
    kOVERWRITE_,
};

enum eOperandCode
{
    kRS1 = 0,
    kRS2,
    kRS3,
    kRS4,
    kRS5,
    kFS1_0,
    kFS1_1,
    kFS2_0,
    kFS2_1,
    kCCs,
    kRD,
    kRD1,
    kRD2,
    kXTRAr,
    kFD0,
    kFD1,
    kCCd,
    kPS1,
    kPS2,
    kPS3,
    kPS4,
    kPS5,
    kPFS1_0,
    kPFS1_1,
    kPFS2_0,
    kPFS2_1,
    kCCps,
    kPD,
    kPD1,
    kPD2,
    kXTRApd,
    kPFD0,
    kPFD1,
    kCCpd,
    kPPD,
    kPPD1,
    kPPD2,
    kXTRAppd,
    kPPFD0,
    kPPFD1,
    kPCCpd,
    kOperand1,
    kOperand2,
    kOperand3,
    kOperand4,
    kOperand5,
    kFOperand1_0,
    kFOperand1_1,
    kFOperand2_0,
    kFOperand2_1,
    kCC,
    kResult,
    kResult1,
    kResult2,
    kfResult0,
    kfResult1,
    kXTRAout,
    kResultCC,
    kAddress,
    kStatus,
    kCondition,
    kStoredValue,
    kocFPSR,
    kocFPCR,
    kUopAddressOffset,
    kSopAddressOffset,
    kLastOperandCode = 0xDEAD
};

std::ostream&
operator<<(std::ostream& anOstream, eOperandCode aCode);

} // namespace nDecoder

#endif // FLEXUS_DECODER_OPERANDCODE_HPP_INCLUDED
