#include "ARMTranslationGranules.hpp"
#include <cmath>

namespace Flexus {
namespace Qemu {
namespace MMU {

TranslationGranule::TranslationGranule() : KBSize(4096), logKBSize(12), granuleShift(9), granuleEntries(512), PAddrWidth(48), IAddrWidth(48) { } // 4KB default

TranslationGranule::TranslationGranule(unsigned ksize) : KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(48), IAddrWidth(48) { } // -3 for all ARM specs (ARMv8 ref manual, D4-2021)

TranslationGranule::TranslationGranule(unsigned ksize,unsigned ASize) : KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(ASize), IAddrWidth(ASize) { }

TranslationGranule::TranslationGranule(unsigned ksize,unsigned PASize, unsigned IASize) : KBSize(ksize), logKBSize(log2(ksize)), granuleShift(logKBSize-3), granuleEntries(exp2(granuleShift)), PAddrWidth(PASize), IAddrWidth(IASize) { }

unsigned TranslationGranule::getKBSize() const { return KBSize; }
unsigned TranslationGranule::getlogKBSize() const { return logKBSize; }
unsigned TranslationGranule::getGranuleShift() const { return granuleShift; }
unsigned TranslationGranule::getGranuleEntries() const { return granuleEntries; }
uint8_t TranslationGranule::getPARange_Raw() const { return PARange_RawValue; }
void TranslationGranule::setPARange_Raw(uint8_t aRawValue) { PARange_RawValue = aRawValue; }
uint8_t TranslationGranule::getPAddrWidth() const { return PAddrWidth; }
void TranslationGranule::setPAddrWidth(uint8_t aSize) { PAddrWidth = aSize; }
uint8_t TranslationGranule::getIAddrWidth() const { return IAddrWidth; }
void TranslationGranule::setIAddrWidth(uint8_t aSize) { IAddrWidth = aSize; }

} // end MMU
} // end Qemu
} // end Flexus
