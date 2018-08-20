#ifndef _ARM_TRANSLATION_GRANULES_DEFINED_HPP_
#define _ARM_TRANSLATION_GRANULES_DEFINED_HPP_

#include <stdint.h>
#include <cmath>

namespace Flexus {
namespace Qemu {
namespace MMU {

/* Msutherl - june'18
 * - added definitions for granules and varying sizes
 */
class TranslationGranule
{
    private:
        unsigned KBSize;
        unsigned logKBSize;
        unsigned granuleShift;
        unsigned granuleEntries;

        uint8_t PARange_RawValue;
        uint8_t PAddrWidth;
        uint8_t IAddrWidth;

    public:
        TranslationGranule() : KBSize(4096), logKBSize(12), granuleShift(9), granuleEntries(512), PAddrWidth(48), IAddrWidth(48) { } // 4KB default
        TranslationGranule(unsigned ksize) : KBSize(ksize), logKBSize(log2(ksize)),
            granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(48), IAddrWidth(48) { } // -3 for all ARM specs (ARMv8 ref manual, D4-2021)
        TranslationGranule(unsigned ksize,unsigned ASize) : KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(ASize), IAddrWidth(ASize) { }
        TranslationGranule(unsigned ksize,unsigned PASize, unsigned IASize) : KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(PASize), IAddrWidth(IASize) { }

        unsigned getKBSize() const { return KBSize; }
        unsigned getlogKBSize() const { return logKBSize; }
        unsigned getGranuleShift() const { return granuleShift; }
        unsigned getGranuleEntries() const { return granuleEntries; }
        uint8_t getPARange_Raw() const { return PARange_RawValue; }
        void setPARange_Raw(uint8_t aRawValue) { PARange_RawValue = aRawValue; }
        uint8_t getPAddrWidth() const { return PAddrWidth; }
        void setPAddrWidth(uint8_t aSize) { PAddrWidth = aSize; }
        uint8_t getIAddrWidth() const { return IAddrWidth; }
        void setIAddrWidth(uint8_t aSize) { IAddrWidth = aSize; }
};

} // end MMU
} // end Qemu
} // end Flexus
#endif //_ARM_TRANSLATION_GRANULES_DEFINED_HPP_
