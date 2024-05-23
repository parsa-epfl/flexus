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
