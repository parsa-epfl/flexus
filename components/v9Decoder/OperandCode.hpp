#ifndef FLEXUS_v9DECODER_OPERANDCODE_HPP_INCLUDED
#define FLEXUS_v9DECODER_OPERANDCODE_HPP_INCLUDED

namespace nv9Decoder {

enum eOperandCode {
  kRS1
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
  , kCondition
  , kStoredValue
  , kocCWP
  , kocCANSAVE
  , kocCANRESTORE
  , kocOTHERWIN
  , kocCLEANWIN
  , kocTL
  , kocFPRS
  , kocFSR
  , kUopAddressOffset
  , kLastOperandCode
};

std::ostream & operator << ( std::ostream & anOstream, eOperandCode aCode);

} //nv9Decoder

#endif //FLEXUS_v9DECODER_OPERANDCODE_HPP_INCLUDED
