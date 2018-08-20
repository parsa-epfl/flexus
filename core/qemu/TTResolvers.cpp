#include "TTResolvers.hpp"

namespace Flexus {
namespace Qemu {
namespace MMU {

/*
 * Functions for TTResolver, carried around by each Translation& object
 * and interacting with MMU classes in granules etc
 */
TTResolver::TTResolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    isBR0(abro), regimeTG(aGranule), RawTTBRReg(aTTBR)
{ 
    // default behaviour?
}

L0Resolver::L0Resolver(bool abro, TranslationGranule& aGranule,address_t aTTBR) :
    TTResolver(abro,aGranule,aTTBR)
{
    uint8_t IAWidth = regimeTG.getIAWidth();
    TTBR_LSB = (IAWidth-1)-35;
    TTBR_NumBits = 47 - TTBR_LSB+1; // 47 since L0 applies to 39<=IAWidth<=47
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
