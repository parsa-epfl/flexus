
#include "RegisterType.hpp"

#include "uArchInterfaces.hpp"

#include <iostream>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps     AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

std::ostream&
operator<<(std::ostream& anOstream, eRegisterType aCode)
{
    const char* map_tables[] = { "x-Registers", "v-Registers", "cc-Bits" };
    //  if (aCode >= kLastMapTableCode) {
    //    anOstream << "Invalid Map Table(" << static_cast<int>(aCode) << ")";
    //  } else {
    anOstream << map_tables[aCode];
    //  }
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, eResourceStatus aCode)
{
    const char* statuses[] = { "Ready", "Unmapped", "NotReady" };
    if (aCode >= kLastStatus) {
        anOstream << "Invalid Status(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << statuses[aCode];
    }
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, eInstructionCode aCode)
{
    const char* insn_codes[] =
      // Special cases
      { "BlackBox",
        "NOP",
        "MAGIC",
        // ALU
        "ALU",
        "Reverse",
        "Mul",
        "Div",
        "RDPR",
        "WRPR",
        // FP
        "FP",
        "ALIGN",
        // Memory
        "Load",
        "LoadEX",
        "LoadAQ",
        "LDP",
        "LoadFP",
        "LDD",
        "LDALU",
        "Store",
        "StoreEX",
        "STP",
        "STRL",
        "StoreFP",
        "STD",
        // Atomics
        "CAS",
        "CASP",
        // Branches
        "BranchUnconditional",
        "BranchConditional",
        "BranchFPConditional",
        "CALL",
        "RETURN",
        "IndirectToRegister",
        "IndirectCall",
        // MEMBARs
        "MEMBARSync",
        "MEMBARStLd",
        "MEMBARStSt",
        "MEMBARLdSt",
        "MEMBARLdLd",
        // Unsupported Instructions
        "RDPRUnsupported",
        "WRPRUnsupported",
        "RETRYorDONE",
        "ExceptionUnsupported",
        "Exception",
        "SideEffectLoad",
        "SideEffectStore",
        "SideEffectAtomic",
        "DeviceAccess",
        "MMUAccess",
        "ITLBMiss",
        "CLREX",
        "HaltCode" };
    if (aCode >= codeLastCode) {
        anOstream << "Invalid code(" << static_cast<int>(aCode) << ")";
    } else {
        anOstream << insn_codes[aCode];
    }
    return anOstream;
}

} // namespace nuArchARM
