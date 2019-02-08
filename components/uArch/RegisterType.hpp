#ifndef FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED
#define FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED

#include <boost/function.hpp>

#include <components/Common/Slices/AbstractInstruction.hpp>

namespace nuArch {

static const uint32_t kGlobalRegCount = 8;
static const uint32_t kGlobalRegSets = 4;

static const uint32_t kNumWindows = 8;
static const uint32_t kRegistersPerWindow = 16;
static const uint32_t kWindowRegCount = 16 * kNumWindows;
static const uint32_t kSpecialRegCount = 3; //Y, ASI, GSR
static const uint32_t kTotalRegs = kGlobalRegSets * kGlobalRegCount + kWindowRegCount + kSpecialRegCount; //Y and ASI
static const uint32_t kFirstSpecialReg = kGlobalRegSets * kGlobalRegCount + kWindowRegCount;
static const uint32_t kRegY = kFirstSpecialReg;
static const uint32_t kRegASI = kRegY + 1;
static const uint32_t kRegGSR = kRegASI + 1;

enum eRegisterType {
  rRegisters
  , fRegisters
  , ccBits
  , kLastMapTableCode
};

enum eccBits {
  iccC
  , iccV
  , iccZ
  , iccN
  , xccC
  , xccV
  , xccZ
  , xccN
  , kLastccBit
};

enum fccVals {
  fccE
  , fccL
  , fccG
  , fccU
  , kLastfccVal
};

std::ostream & operator <<( std::ostream & anOstream, eRegisterType aCode);
std::ostream & operator <<( std::ostream & anOstream, eccBits aBits);
std::ostream & operator <<( std::ostream & anOstream, fccVals aVal);

} //nuArch

#endif //FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED
