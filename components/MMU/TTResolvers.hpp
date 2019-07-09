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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

#ifndef _ARM_TT_RESOLVERS_DEFINED_HPP_
#define _ARM_TT_RESOLVERS_DEFINED_HPP_
#include "ARMTranslationGranules.hpp"
#include "MMUUtil.hpp"

#include <memory>
#include <stdint.h>

namespace nMMU {

typedef std::shared_ptr<TranslationGranule> _TTResolver_Shptr_T;
typedef uint64_t address_t;

/* Used for dealing with all varying address widths and granules, figures out
 * which bits to get and index and discard.
 */
class TTResolver {
public:
  TTResolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR, uint8_t PAddrWidth);
  virtual address_t resolve(address_t inputAddress);
  virtual address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
  void updateRawBaseRegister(address_t newTTBR);

protected:
  // for going through the TT
  bool isBR0;
  address_t RawTTBRReg;
  uint8_t TTBR_LSB;
  uint8_t TTBR_MSB;
  uint8_t offset_LSB;
  uint8_t offset_MSB;
  uint8_t IAddressWidth;
  uint8_t PAddressWidth;
  uint8_t TnSz;
  address_t descriptorIndex;

  // for setting input-output bits dependent on TG size
  _TTResolver_Shptr_T regimeTG;

  // utility
  address_t maskAndShiftInputAddress(address_t anAddr);
};

class L0Resolver : public TTResolver {
public:
  L0Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t tbr, uint8_t aPAW);
  address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L1Resolver : public TTResolver {
public:
  L1Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t attbr, uint8_t aPAW);
  address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L2Resolver : public TTResolver {
public:
  L2Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t attbr, uint8_t aPAW);
  address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L3Resolver : public TTResolver {
public:
  L3Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t attbr, uint8_t aPAW);
  address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};

} // namespace nMMU
#endif // _ARM_TT_RESOLVERS_DEFINED_HPP_
