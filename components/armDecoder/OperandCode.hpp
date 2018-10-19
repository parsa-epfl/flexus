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


#ifndef FLEXUS_armDECODER_OPERANDCODE_HPP_INCLUDED
#define FLEXUS_armDECODER_OPERANDCODE_HPP_INCLUDED

namespace narmDecoder {


#define SIGNED_UPPER_BOUND_B 0x000000FF00000000ULL
#define SIGNED_UPPER_BOUND_H 0x0000FFFF00000000ULL
#define SIGNED_UPPER_BOUND_W 0x00FFFFFF00000000ULL
#define SIGNED_UPPER_BOUND_X 0xFFFFFFFF00000000ULL



enum eOpType {
    kADD_,
    kADDS_,
    kCONCAT32_,
    kCONCAT64_,
    kAND_,
    kANDS_,
    kORR_,
    kXOR_,

    kSUB_,
    kSUBS_,
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
};

enum eArchState {
    kAARCH64,
    kAARCH32,
    KLASTARCH,
};

enum eOperandCode {
    kRS1 = 0
  , kRS2
  , kRS3
  , kRS4
  , kRS5
  , kFS1_0
  , kFS1_1
  , kFS2_0
  , kFS2_1
  , kCCs
  , kRD
  , kRD1
  , kXTRAr
  , kFD0
  , kFD1
  , kCCd
  , kPS1
  , kPS2
  , kPS3
  , kPS4
  , kPS5
  , kPFS1_0
  , kPFS1_1
  , kPFS2_0
  , kPFS2_1
  , kCCps
  , kPD
  , kPD1
  , kXTRApd
  , kPFD0
  , kPFD1
  , kCCpd
  , kPPD
  , kPPD1
  , kXTRAppd
  , kPPFD0
  , kPPFD1
  , kPCCpd
  , kOperand1
  , kOperand2
  , kOperand3
  , kOperand4
  , kOperand5
  , kFOperand1_0
  , kFOperand1_1
  , kFOperand2_0
  , kFOperand2_1
  , kCC
  , kResult
  , kResult1
  , kfResult0
  , kfResult1
  , kXTRAout
  , kResultCC
  , kAddress
  , kStatus
  , kCondition
  , kStoredValue
  , kocFPSR
  , kocFPCR
  , kUopAddressOffset
  , kLastOperandCode
};

std::ostream & operator << ( std::ostream & anOstream, eOperandCode aCode);

} //narmDecoder

#endif //FLEXUS_armDECODER_OPERANDCODE_HPP_INCLUDED
