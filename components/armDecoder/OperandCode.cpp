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


#include <bitset>
#include <iostream>
#include <components/uArchARM/RegisterType.hpp>
#include "OperandCode.hpp"
#include <core/types.hpp>

namespace narmDecoder {

using namespace nuArchARM;

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
    , "status"
    , "condition"
    , "storedvalue"
    , "fpsr"
    , "fpcr"
    , "uop_address_offset"
    , "sop_address_offset"
  };
  if (aCode >= kLastOperandCode) {
    anOstream << "InvalidOperandCode(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << operand_codes[aCode];
  }
  return anOstream;
}

//std::ostream & operator << ( std::ostream & anOstream, Flexus::Core::bits const & aCC) {
//  anOstream
//      << aCC;
//  return anOstream;
//}

} //narmDecoder

