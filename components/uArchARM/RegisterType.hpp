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


#ifndef FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED
#define FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED

#include <functional>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>

namespace nuArchARM {

//static const uint32_t kRegASI = 1;
//static const uint32_t kGlobalRegSets = 31;
//static const uint32_t kGlobalRegCount = 31;
//static const uint32_t kFirstSpecialReg = kGlobalRegSets;
//static const uint32_t kTotalRegs = kGlobalRegSets;

static const uint32_t kGlobalRegCount = 32;
static const uint32_t kGlobalRegSets = 4;

static const uint32_t kNumWindows = 8;
static const uint32_t kRegistersPerWindow = 16;
static const uint32_t kWindowRegCount = 16 * kNumWindows;
static const uint32_t kSpecialRegCount = 3; //Y, ASI, GSR
static const uint32_t kTotalRegs = kGlobalRegSets * kGlobalRegCount + kWindowRegCount + kSpecialRegCount; //Y and ASI
static const uint32_t kFirstSpecialReg = kGlobalRegSets * kGlobalRegCount + kWindowRegCount;
static const uint32_t kRegY = kFirstSpecialReg;
static const uint32_t kRegASI = kRegY + 1;
static const uint32_t kRegGSR = kRegASI + 1;

enum eRegisterType {
     xRegisters
   , vRegisters
   , ccBits
   , kLastMapTableCode
};

enum condBits {
    N  // negative
  , Z  // zero
  , C  // carry - for unsinged overflow
  , V  // signed overflow
  , kLastcondBit
};

enum fccVals {
  fccE
  , fccL
  , fccG
  , fccU
  , kLastfccVal
};

std::ostream & operator <<( std::ostream & anOstream, eRegisterType aCode);
std::ostream & operator <<( std::ostream & anOstream, condBits aBits);
std::ostream & operator <<( std::ostream & anOstream, fccVals aVal);

} //nuArchARM

#endif //FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED
