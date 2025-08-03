#include "TranslationGranules.hpp"

#include <cmath>
#include <core/debug/debug.hpp>

namespace nMMU {

/*
 * Parent class
 */
TranslationGranule::TranslationGranule()
  : KBSize(4096)
  , logKBSize(12)
  , granuleShift(9)
  , granuleEntries(512)
  , PAddrWidth(48)
  , IAddrOffset(48)
{
} // 4KB default

/*
TranslationGranule::TranslationGranule(unsigned ksize) : KBSize(ksize),
logKBSize(log2(ksize)), granuleShift(logKBSize-3),
granuleEntries(exp2(granuleShift)), PAddrWidth(48), IAddrOffset(48) { } // -3
for all ARM specs (ARMv8 ref manual, D4-2021)

TranslationGranule::TranslationGranule(unsigned ksize,unsigned ASize) :
KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3),
granuleEntries(exp2(granuleShift)), PAddrWidth(ASize), IAddrOffset(ASize) { }
*/

TranslationGranule::TranslationGranule(unsigned ksize, unsigned PASize, unsigned IASize)
  : KBSize(ksize)
  , logKBSize(log2(ksize))
  , granuleShift(logKBSize - 3)
  , granuleEntries(exp2(granuleShift))
  , PAddrWidth(PASize)
  , IAddrOffset(IASize)
{
}

unsigned
TranslationGranule::getKBSize() const
{
    return KBSize;
}
unsigned
TranslationGranule::getlogKBSize() const
{
    return logKBSize;
}
unsigned
TranslationGranule::getGranuleShift() const
{
    return granuleShift;
}
unsigned
TranslationGranule::getGranuleEntries() const
{
    return granuleEntries;
}
uint8_t
TranslationGranule::getPARange_Raw() const
{
    return PARange_RawValue;
}
void
TranslationGranule::setPARange_Raw(uint8_t aRawValue)
{
    PARange_RawValue = aRawValue;
}
uint8_t
TranslationGranule::getPAddrWidth() const
{
    return PAddrWidth;
}
void
TranslationGranule::setPAddrWidth(uint8_t aSize)
{
    PAddrWidth = aSize;
}
uint8_t
TranslationGranule::getIAddrOffset() const
{
    return IAddrOffset;
}
void
TranslationGranule::setIAddrOffset(uint8_t aSize)
{
    IAddrOffset = aSize;
}

uint8_t
TranslationGranule::NumBitsResolvedPerTTAccess() const
{
    return (logKBSize - 3);
}

uint8_t
TranslationGranule::getIAddrSize() const
{
    return (64 - IAddrOffset);
}

/* Magic numbers here ALL come from:
 * ARMv8 Ref. Manual, Section D4.2.6, pages D4-2030
 *  (for all granule sizes)
 */
uint8_t
TranslationGranule::getInitialLookupLevel() const
{
    uint8_t TnSz = getIAddrOffset();
    switch (KBSize) {
        case 1 << 12: // 4k
        {
            if (TnSz >= 34) {
                return 2;
            } else if (TnSz >= 25) {
                return 1;
            } else
                return 0;
        }
        case 1 << 14: // 16k
        {
            if (TnSz >= 39) {
                return 3;
            } else if (TnSz >= 28) {
                return 2;
            } else if (TnSz >= 17) {
                return 1;
            } else
                return 0;
        }
        case 1 << 16: // 64k
        {
            if (TnSz >= 35) {
                return 3;
            } else if (TnSz >= 22) {
                return 2;
            } else if (TnSz >= 16) {
                return 1;
            } else
                return 1; // l0 not possible, even w. ext addressing (LVA)
        }
        default: return 0; // FAIL
    }
}

// should never use this, should always be a specific lower level class
uint64_t
TranslationGranule::GetLowerAddressRangeLimit() const
{
    DBG_Assert(false);
    return 0;
}
uint64_t
TranslationGranule::GetUpperAddressRangeLimit() const
{
    DBG_Assert(false);
    return 0;
}

/*
 * TG0 subclass - for behaviour specific to address range translated by TTBR0
 */
TG0_Granule::TG0_Granule()
  : TranslationGranule()
{
}
TG0_Granule::TG0_Granule(unsigned granuleSize, unsigned PAddrSize, unsigned IAOffset)
  : TranslationGranule(granuleSize, PAddrSize, IAOffset)
{
}

/*
 * TG0 lower bound is ALWAYS 0x0
 * ARMv8 Ref. Manual, page D4-2045
 */
uint64_t
TG0_Granule::GetLowerAddressRangeLimit() const
{
    return 0;
}

/*
 * Get TnSz value, calculate upper bound
 * 2^(64 - TnSz) -1
 * Armv8 Ref Manual, page D4-2046
 */
uint64_t
TG0_Granule::GetUpperAddressRangeLimit() const
{
    uint8_t TnSz = getIAddrOffset();
    uint64_t val = pow(2, 64 - TnSz) - 1;
    return val;
}

/*
 * TG1 subclass - for behaviour specific to address range translated by TTBR1
 */
TG1_Granule::TG1_Granule()
  : TranslationGranule()
{
}
TG1_Granule::TG1_Granule(unsigned granuleSize, unsigned PAddrSize, unsigned IAOffset)
  : TranslationGranule(granuleSize, PAddrSize, IAOffset)
{
}

/*
 * Get TnSz value, calculate lower bound
 * 2^64 - 2^(64 - TnSz)
 * Armv8 Ref Manual, page D4-2046
 */
uint64_t
TG1_Granule::GetLowerAddressRangeLimit() const
{
    uint8_t TnSz = getIAddrOffset();
    uint64_t val = pow(2, 64) - pow(2, (64 - TnSz));
    return val;
}

/*
 * TG1 upper bound is ALWAYS 0xFFFF FFFF FFFF FFFF
 * ARMv8 Ref. Manual, page D4-2045
 */
uint64_t
TG1_Granule::GetUpperAddressRangeLimit() const
{
    return 0xffffffffffffffff;
}

} // namespace nMMU