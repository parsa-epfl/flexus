#include "TTResolvers.hpp"
#include "bitUtilities.hpp"

#include <iostream>

namespace Flexus {
namespace Qemu {
namespace MMU {

/*
 * Functions for TTResolver, carried around by each Translation& object
 * and interacting with MMU classes in granules etc
 */
TTResolver::TTResolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    isBR0(abro), RawTTBRReg(aTTBR), regimeTG(aGranule) 
{ 
    this->IAWidth = regimeTG.getIAddrWidth();
}

address_t
TTResolver::maskAndShiftInputAddress(address_t anAddr)
{
    address_t output = extractBitsWithRange(anAddr,offset_LSB,offset_NumBits);
    return (output << 3);
}

L0Resolver::L0Resolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    TTResolver(abro,aGranule,aTTBR)
{
    TTBR_LSB = (IAWidth-1)-35;
    TTBR_NumBits = 47 - TTBR_LSB+1; // 47 since L0 applies to 39<=IAWidth<=47
    offset_LSB = 39;
    offset_NumBits = (IAWidth-39)+1;
}

address_t
L0Resolver::resolve(address_t inputAddress)
{
    address_t output = extractBitsWithRange(RawTTBRReg,TTBR_LSB,TTBR_NumBits);
    std::cout << "TTBR Base Bits: " << std::hex << output << std::dec << std::endl;
    output |= maskAndShiftInputAddress(inputAddress);
    std::cout << "TTE To Read Bits: " << std::hex << output << std::dec << std::endl;
    return output;
}

L1Resolver::L1Resolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    TTResolver(abro,aGranule,aTTBR)
{
    // calculate i/o bits, sizes, and masks for resolve()
}

L2Resolver::L2Resolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    TTResolver(abro,aGranule,aTTBR)
{
    // calculate i/o bits, sizes, and masks for resolve()
}

L3Resolver::L3Resolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    TTResolver(abro,aGranule,aTTBR)
{
    // calculate i/o bits, sizes, and masks for resolve()
}

} // end MMU
} // end Qemu
} // end Flexus
