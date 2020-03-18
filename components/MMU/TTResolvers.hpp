//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

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
