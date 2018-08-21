#include "TTResolvers.hpp"
#include "bitUtilities.hpp"
#include <iostream>
#include <assert.h>

namespace Flexus {
namespace Qemu {
namespace MMU {

/*
 * Functions for TTResolver, carried around by each Translation& object
 * and interacting with MMU classes in granules etc
 */
TTResolver::TTResolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t PAddrWidth) :
    isBR0(abro), RawTTBRReg(aTTBR), PAddressWidth(PAddrWidth), regimeTG(aGranule) 
{ 
    TnSz = regimeTG->getIAddrSize();
    IAddressWidth = 64 - regimeTG->getIAddrSize();
}

// All fields must be set before calling this (in constructors)
address_t
TTResolver::resolve(address_t inputAddress)
{
    address_t output = extractBitsWithBounds(RawTTBRReg,TTBR_LSB,TTBR_MSB);
    std::cout << "TTBR Base Bits: " << std::hex << output << std::dec << std::endl;
    output |= maskAndShiftInputAddress(inputAddress);
    std::cout << "TTE To Read Bits: " << std::hex << output << std::dec << std::endl;
    return output;
}

address_t
TTResolver::maskAndShiftInputAddress(address_t anAddr)
{
    address_t output = extractBitsWithBounds(anAddr,offset_LSB,offset_MSB);
    return (output << 3);
}

void
TTResolver::updateRawBaseRegister(address_t newTTBR)
{
    RawTTBRReg = newTTBR;
}

/* Msutherl:
 * - ALL MAGIC NUMBERS COME FROM : ARMv8 Manual, page D4-2050
 * - If need to edit or change this, BE INCREDIBLY CAREFUL.
 *   These algorithms must work for ALL IA and PA sizes.
 */
//FIXME: FIXME:
//- Right now, this ONLY WORKS FOR 4KB GRANULES.
//- MOVE ALL CALCULATION BOUNDARIES INTO TRANSLATIONGRANULE CLASS,POLYMORPHIZE IT
L0Resolver::L0Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t aPhysAddrWidth) : 
    TTResolver(abro,aGranule,aTTBR,aPhysAddrWidth)
{
    assert( TnSz >= 16 && TnSz <= 24 ); // Constraints to be in level 0 TT access
                                        // (otherwise we don't need to resolve L0)
    unsigned int x = 28 - TnSz;
    unsigned int y = x + 35;
    TTBR_LSB = x;
    TTBR_MSB = PAddressWidth - 1;
    offset_LSB = 39; // Least significant bit of index
    offset_MSB = y; 
}

L1Resolver::L1Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t aPhysAddrWidth) :
    TTResolver(abro,aGranule,aTTBR,aPhysAddrWidth)
{
    unsigned int x;
    if( TnSz >= 25 && TnSz <= 33 ) x = 37 - TnSz; /* If L0 was skipped due to IASize */
    else x = 12; // second level
    unsigned int y = x + 26;
    TTBR_LSB = x;
    TTBR_MSB = PAddressWidth - 1;
    offset_LSB = 30; // (bit 30 since previous was 39, 4K granules resolve 9 bits/level
    offset_MSB = y; 
}

L2Resolver::L2Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t aPhysAddrWidth) :
    TTResolver(abro,aGranule,aTTBR,aPhysAddrWidth)
{
    unsigned int x;
    if( TnSz >= 34 && TnSz <= 39 ) x = 46 - TnSz; /* If L0/L1 skipped b/c of IASize */
    else x = 12; // third level
    unsigned int y = x + 17;
    TTBR_LSB = x;
    TTBR_MSB = PAddressWidth - 1;
    offset_LSB = 21; // (bit 21 since previous was 30, 4K granules resolve 9 bits/level
    offset_MSB = y; 
}

L3Resolver::L3Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t aPhysAddrWidth) :
    TTResolver(abro,aGranule,aTTBR,aPhysAddrWidth)
{
    // L3 can never be the first level of a 4KB granule walk
    TTBR_LSB = 12;
    TTBR_MSB = PAddressWidth - 1;
    offset_LSB = 12; // (bit 12 since previous was 21, 4K granules resolve 9 bits/level
    offset_MSB = 20; 
}

} // end MMU
} // end Qemu
} // end Flexus
