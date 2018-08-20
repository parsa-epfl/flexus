#ifndef _ARM_TT_RESOLVERS_DEFINED_HPP_
#define _ARM_TT_RESOLVERS_DEFINED_HPP_
#include "ARMTranslationGranules.hpp"

namespace Flexus {
namespace Qemu {
namespace MMU {

typedef unsigned long long address_t;

/* Used for dealing with all varying address widths and granules, figures out
 * which bits to get and index and discard. 
 */
class TTResolver
{
    public:
        TTResolver(bool abro, TranslationGranule& aGranule,address_t aTTBR); 
        virtual address_t resolve() { return TTBase; }
    private:
        // for going through the TT
        bool isBR0;
        address_t TTBase;
        address_t RawTTBRReg;
        uint8_t TTBR_LSB;
        uint8_t TTBR_NumBits;

        // for setting input-output bits dependent on TG size
        TranslationGranule regimeTG;
};

class L0Resolver : public TTResolver {
    public:
    L0Resolver(bool abro, TranslationGranule& aGranule,address_t tbr); 
    virtual address_t resolve();
};
class L1Resolver: public TTResolver {
    public:
    L1Resolver(bool abro, TranslationGranule& aGranule,address_t attbr); 
    virtual address_t resolve();
};
class L2Resolver: public TTResolver {
    public:
    L2Resolver(bool abro, TranslationGranule& aGranule,address_t attbr); 
    virtual address_t resolve();
};
class L3Resolver: public TTResolver {
    public:
    L3Resolver(bool abro, TranslationGranule& aGranule,address_t attbr); 
    virtual address_t resolve();
};


} // end MMU
} // end Qemu
} // end Flexus

#endif // _ARM_TT_RESOLVERS_DEFINED_HPP_
