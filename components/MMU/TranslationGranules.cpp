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