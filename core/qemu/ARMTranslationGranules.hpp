#ifndef _ARM_TRANSLATION_GRANULES_DEFINED_HPP_
#define _ARM_TRANSLATION_GRANULES_DEFINED_HPP_
#include <stdint.h>

namespace Flexus {
namespace Qemu {
namespace MMU {

/* Msutherl - june'18
 * - added definitions for granules and varying sizes
 */
class TranslationGranule
{
    protected:
        unsigned KBSize;
        unsigned logKBSize;
        unsigned granuleShift;
        unsigned granuleEntries;

        uint8_t PARange_RawValue;
        uint8_t PAddrWidth;
        uint8_t IAddrWidth;

    public:
        TranslationGranule();
        TranslationGranule(unsigned ksize);
        TranslationGranule(unsigned ksize,unsigned ASize);
        TranslationGranule(unsigned ksize,unsigned PASize, unsigned IASize);
        unsigned getKBSize() const;
        unsigned getlogKBSize() const;
        unsigned getGranuleShift() const;
        unsigned getGranuleEntries() const;
        uint8_t getPARange_Raw() const;
        void setPARange_Raw(uint8_t aRawValue);
        uint8_t getPAddrWidth() const;
        void setPAddrWidth(uint8_t aSize);
        uint8_t getIAddrWidth() const;
        void setIAddrWidth(uint8_t aSize);
};

} // end MMU
} // end Qemu
} // end Flexus
#endif //_ARM_TRANSLATION_GRANULES_DEFINED_HPP_
