#include <bitset>
#include <iostream>
#include <components/uArch/RegisterType.hpp>
#include "OperandCode.hpp"

namespace nv9Decoder {

using namespace nuArch;

std::ostream & operator << ( std::ostream & anOstream, eOperandCode aCode) {
  const char * operand_codes[] = {
    "rs1"
    , "rs2"
    , "rs3" //Y for div
    , "rs4"
    , "rs5"
    , "fs1_0"
    , "fs1_1"
    , "fs2_0"
    , "fs2_1"
    , "CCs"
    , "rd"
    , "rd1"
    , "XTRA"
    , "fd0"
    , "fd1"
    , "CCd"
    , "ps1"
    , "ps2"
    , "ps3"
    , "ps4"
    , "ps5"
    , "pfs1_0"
    , "pfs1_1"
    , "pfs2_0"
    , "pfs2_1"
    , "CCps"
    , "pd"
    , "pd1"
    , "XTRApd"
    , "pfd0"
    , "pfd1"
    , "CCpd"
    , "ppd"
    , "ppd1"
    , "XTRAppd"
    , "ppfd0"
    , "ppfd1"
    , "pCCpd"
    , "operand1"
    , "operand2"
    , "operand3"
    , "operand4"
    , "operand5"
    , "foperand1_0"
    , "foperand1_1"
    , "foperand2_0"
    , "foperand2_1"
    , "cc"
    , "result"
    , "result1"
    , "fresult0"
    , "fresult1"
    , "XTRAout"
    , "resultcc"
    , "address"
    , "condition"
    , "storedvalue"
    , "cwp"
    , "cansave"
    , "canrestore"
    , "otherwin"
    , "cleanwin"
    , "tl"
    , "fprs"
    , "fsr"
    , "uop_address_offset"
  };
  if (aCode >= kLastOperandCode) {
    anOstream << "InvalidOperandCode(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << operand_codes[aCode];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, std::bitset<8> const & aCC) {
  anOstream
      << " x{ "
      << ( aCC[xccN] ? "" : "!" ) << "N "
      << ( aCC[xccZ] ? "" : "!" ) << "Z "
      << ( aCC[xccV] ? "" : "!" ) << "V "
      << ( aCC[xccC] ? "" : "!" ) << "C "
      << "} i{ "
      << ( aCC[iccN] ? "" : "!" ) << "N "
      << ( aCC[iccZ] ? "" : "!" ) << "Z "
      << ( aCC[iccV] ? "" : "!" ) << "V "
      << ( aCC[iccC] ? "" : "!" ) << "C "
      << "}";
  return anOstream;
}

} //nv9Decoder

