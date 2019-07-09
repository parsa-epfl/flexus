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

#include <core/qemu/bitUtilities.hpp>

#include "TTResolvers.hpp"
#include <assert.h>
#include <iostream>

namespace nMMU {

/*
 * Functions for TTResolver, carried around by each Translation& object
 * and interacting with MMU classes in granules etc
 */
TTResolver::TTResolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR, uint8_t PAddrWidth)
    : isBR0(abro), RawTTBRReg(aTTBR), PAddressWidth(PAddrWidth), regimeTG(aGranule) {
  TnSz = regimeTG->getIAddrOffset();
  IAddressWidth = 64 - regimeTG->getIAddrSize();
}

// All fields must be set before calling this (in constructors)
address_t TTResolver::resolve(address_t inputAddress) {
  DBG_(VVerb, (<< "TTBR RAW: " << std::hex << RawTTBRReg << std::dec));
  address_t output = extractBitsWithBounds(RawTTBRReg, TTBR_MSB, TTBR_LSB);
  DBG_(VVerb, (<< "TTBR EXTRACT: " << std::hex << output << std::dec));
  output = output << TTBR_LSB;
  DBG_(VVerb, (<< "TTBR SHIFT : " << std::hex << output << std::dec));
  output |= maskAndShiftInputAddress(inputAddress);
  DBG_(VVerb, (<< "TTE Indexed: " << std::hex << output << std::dec));
  return output;
}

/* Return: Shifted and Masked bits of output address (physical or intermediate)
 * given the rawTTE which has been read from QEMUs physical memory
 *  - Depending on Granule Size, and Translation Level....
 */
address_t TTResolver::getBlockOutputBits(address_t rawTTEFromPhysMemory) {
  return 0; // always a subclass
}

address_t TTResolver::maskAndShiftInputAddress(address_t anAddr) {
  address_t output = extractBitsWithBounds(anAddr, offset_MSB, offset_LSB);
  return (output << 3);
}

void TTResolver::updateRawBaseRegister(address_t newTTBR) {
  RawTTBRReg = newTTBR;
}

/* Msutherl:
 * - ALL MAGIC NUMBERS COME FROM : ARMv8 Manual, page D4-2050
 * - If need to edit or change this, BE INCREDIBLY CAREFUL.
 *   These algorithms must work for ALL IA and PA sizes.
 */
// FIXME: FIXME:
//- Right now, this ONLY WORKS FOR 4KB GRANULES.
//- MOVE ALL CALCULATION BOUNDARIES INTO TRANSLATIONGRANULE CLASS,POLYMORPHIZE
// IT
L0Resolver::L0Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR,
                       uint8_t aPhysAddrWidth)
    : TTResolver(abro, aGranule, aTTBR, aPhysAddrWidth) {
  assert(TnSz >= 16 && TnSz <= 24); // Constraints to be in level 0 TT access
                                    // (otherwise we don't need to resolve L0)
  unsigned int x = 28 - TnSz;
  unsigned int y = x + 35;
  TTBR_LSB = x;
  TTBR_MSB = PAddressWidth - 1;
  offset_LSB = 39; // Least significant bit of index
  offset_MSB = y;
  DBG_(VVerb,
       (std::dec << "Setting TTBR_LSB: " << (int)TTBR_LSB << ", TTBR_MSB: " << (int)TTBR_MSB
                 << ", offset_LSB: " << (int)offset_LSB << ", offset_MSB: " << (int)offset_MSB));
}

/* Return: Shifted and Masked bits of output address (physical or intermediate)
 * given the rawTTE which has been read from QEMUs physical memory
 */
address_t L0Resolver::getBlockOutputBits(address_t rawTTEFromPhysMemory) {
  return 0; /* Level 0 not allowed to be a block descriptor in 4k Granules*/
  // FIXME: when you do this for other granule sizes, L0 is possible afaik
}

L1Resolver::L1Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR,
                       uint8_t aPhysAddrWidth)
    : TTResolver(abro, aGranule, aTTBR, aPhysAddrWidth) {
  unsigned int x;
  if (TnSz >= 25 && TnSz <= 33)
    x = 37 - TnSz; /* If L0 was skipped due to IASize */
  else
    x = 12; // second level
  unsigned int y = x + 26;
  TTBR_LSB = x;
  TTBR_MSB = PAddressWidth - 1;
  offset_LSB = 30; // (bit 30 since previous was 39, 4K granules resolve 9 bits/level
  offset_MSB = y;
}

/* Return: Shifted and Masked bits of output address (physical or intermediate)
 * given the rawTTE which has been read from QEMUs physical memory
 */
address_t L1Resolver::getBlockOutputBits(address_t rawTTEFromPhysMemory) {
  unsigned int blockLSB = 30, blockMSB = 47;
  address_t output = extractBitsWithBounds(rawTTEFromPhysMemory, blockMSB, blockLSB);
  return (output << blockLSB);
}

L2Resolver::L2Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR,
                       uint8_t aPhysAddrWidth)
    : TTResolver(abro, aGranule, aTTBR, aPhysAddrWidth) {
  unsigned int x;
  if (TnSz >= 34 && TnSz <= 39)
    x = 46 - TnSz; /* If L0/L1 skipped b/c of IASize */
  else
    x = 12; // third level
  unsigned int y = x + 17;
  TTBR_LSB = x;
  TTBR_MSB = PAddressWidth - 1;
  offset_LSB = 21; // (bit 21 since previous was 30, 4K granules resolve 9 bits/level
  offset_MSB = y;
}

/* Return: Shifted and Masked bits of output address (physical or intermediate)
 * given the rawTTE which has been read from QEMUs physical memory
 */
address_t L2Resolver::getBlockOutputBits(address_t rawTTEFromPhysMemory) {
  unsigned int blockLSB = 21, blockMSB = 47;
  address_t output = extractBitsWithBounds(rawTTEFromPhysMemory, blockMSB, blockLSB);
  return (output << blockLSB);
}

L3Resolver::L3Resolver(bool abro, _TTResolver_Shptr_T aGranule, address_t aTTBR,
                       uint8_t aPhysAddrWidth)
    : TTResolver(abro, aGranule, aTTBR, aPhysAddrWidth) {
  // L3 can never be the first level of a 4KB granule walk
  TTBR_LSB = 12;
  TTBR_MSB = PAddressWidth - 1;
  offset_LSB = 12; // (bit 12 since previous was 21, 4K granules resolve 9 bits/level
  offset_MSB = 20;
}

/* Return: Shifted and Masked bits of output address (physical or intermediate)
 * given the rawTTE which has been read from QEMUs physical memory
 */
address_t L3Resolver::getBlockOutputBits(address_t rawTTEFromPhysMemory) {
  unsigned int blockLSB = 12, blockMSB = 47;
  address_t output = extractBitsWithBounds(rawTTEFromPhysMemory, blockMSB, blockLSB);
  return (output << blockLSB);
}

} // namespace nMMU
