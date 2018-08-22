#ifndef _ARM_TT_RESOLVERS_DEFINED_HPP_
#define _ARM_TT_RESOLVERS_DEFINED_HPP_
#include "ARMTranslationGranules.hpp"

#include <stdint.h>
#include <memory>
namespace Flexus {
namespace Qemu {
namespace MMU {

typedef unsigned long long address_t;
typedef std::shared_ptr<TranslationGranule> _TTResolver_Shptr_T ;

/* Used for dealing with all varying address widths and granules, figures out
 * which bits to get and index and discard. 
 */
class TTResolver
{
    public:
        TTResolver(bool abro, _TTResolver_Shptr_T aGranule,address_t aTTBR,uint8_t PAddrWidth); 
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
        L0Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t tbr,uint8_t aPAW); 
        address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L1Resolver: public TTResolver {
    public:
        L1Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t attbr,uint8_t aPAW); 
        address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L2Resolver: public TTResolver {
    public:
        L2Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t attbr,uint8_t aPAW); 
        address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};
class L3Resolver: public TTResolver {
    public:
        L3Resolver(bool abro, _TTResolver_Shptr_T aGranule,address_t attbr,uint8_t aPAW); 
        address_t getBlockOutputBits(address_t rawTTEFromPhysMemory);
};


} // end MMU
} // end Qemu
} // end Flexus

#endif // _ARM_TT_RESOLVERS_DEFINED_HPP_
