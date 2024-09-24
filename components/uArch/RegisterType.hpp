
#ifndef FLEXUS_UARCH_REGISTERTYPE_HPP_INCLUDED
#define FLEXUS_UARCH_REGISTERTYPE_HPP_INCLUDED

#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <functional>

namespace nuArch {

static const uint32_t kGlobalRegCount = 32;
static const uint32_t kGlobalRegSets  = 1;

static const uint32_t kNumWindows         = 0;
static const uint32_t kRegistersPerWindow = 0;
static const uint32_t kWindowRegCount     = kRegistersPerWindow * kNumWindows;
static const uint32_t kSpecialRegCount    = 3;
static const uint32_t kTotalRegs          = kGlobalRegSets * kGlobalRegCount + kWindowRegCount + kSpecialRegCount;
static const uint32_t kFirstSpecialReg    = kGlobalRegSets * kGlobalRegCount + kWindowRegCount;
static const uint32_t kRegY               = kFirstSpecialReg;
static const uint32_t kRegASI             = kRegY + 1;
static const uint32_t kRegGSR             = kRegASI + 1;

enum eRegisterType
{
    xRegisters,
    vRegisters,
    ccBits,
    kLastMapTableCode
};

std::ostream&
operator<<(std::ostream& anOstream, eRegisterType aCode);

} // namespace nuArch

#endif // FLEXUS_uARCH_REGISTERTYPE_HPP_INCLUDED