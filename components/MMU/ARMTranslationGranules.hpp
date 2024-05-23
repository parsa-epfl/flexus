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
#ifndef _ARM_TRANSLATION_GRANULES_DEFINED_HPP_
#define _ARM_TRANSLATION_GRANULES_DEFINED_HPP_
#include <stdint.h>

namespace nMMU {

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
    uint8_t IAddrOffset;

  public:
    TranslationGranule();
    TranslationGranule(unsigned ksize, unsigned PASize, unsigned IASize);
    unsigned getKBSize() const;
    unsigned getlogKBSize() const;
    unsigned getGranuleShift() const;
    unsigned getGranuleEntries() const;
    uint8_t getPARange_Raw() const;
    void setPARange_Raw(uint8_t aRawValue);
    uint8_t getPAddrWidth() const;
    void setPAddrWidth(uint8_t aSize);
    uint8_t getIAddrOffset() const;
    void setIAddrOffset(uint8_t aSize);

    uint8_t NumBitsResolvedPerTTAccess() const;
    uint8_t getIAddrSize() const;
    uint8_t getInitialLookupLevel() const;
    virtual uint64_t GetLowerAddressRangeLimit() const;
    virtual uint64_t GetUpperAddressRangeLimit() const;
};

class TG0_Granule : public TranslationGranule
{
  public:
    TG0_Granule();
    TG0_Granule(unsigned granuleSize, unsigned PAddrSize, unsigned IAOffset);
    uint64_t GetLowerAddressRangeLimit() const;
    uint64_t GetUpperAddressRangeLimit() const;
};

class TG1_Granule : public TranslationGranule
{
  public:
    TG1_Granule();
    TG1_Granule(unsigned granuleSize, unsigned PAddrSize, unsigned IAOffset);
    uint64_t GetLowerAddressRangeLimit() const;
    uint64_t GetUpperAddressRangeLimit() const;
};

} // namespace nMMU

#endif //_ARM_TRANSLATION_GRANULES_DEFINED_HPP_
