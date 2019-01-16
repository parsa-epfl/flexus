#include <iostream>

#include "RegisterType.hpp"
#include "uArchInterfaces.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

std::ostream & operator <<( std::ostream & anOstream, eRegisterType aCode) {
  const char * map_tables[] = {
    "r-Registers"
    , "f-Registers"
    , "cc-Bits"
  };
  if (aCode >= kLastMapTableCode) {
    anOstream << "Invalid Map Table(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << map_tables[aCode];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, eResourceStatus aCode) {
  const char * statuses[] = {
    "Ready"
    , "Unmapped"
    , "NotReady"
  };
  if (aCode >= kLastStatus) {
    anOstream << "Invalid Status(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << statuses[aCode];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, eccBits aBit) {
  const char * cc_bits[] = {
    "iccN"
    , "iccZ"
    , "iccV"
    , "iccC"
    , "xccN"
    , "xccZ"
    , "xccV"
    , "xccC"
  };
  if (aBit >= kLastccBit) {
    anOstream << "Invalid ccBit(" << static_cast<int>(aBit) << ")";
  } else {
    anOstream << cc_bits[aBit];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, fccVals aVal) {
  const char * fcc_vals[] = {
    "fccE"
    , "fccL"
    , "fccG"
    , "fccU"
  };
  if (aVal >= kLastfccVal) {
    anOstream << "Invalid fccVal(" << static_cast<int>(aVal) << ")";
  } else {
    anOstream << fcc_vals[aVal];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, eInstructionCode aCode) {
  const char * insn_codes[] =
    //Special cases
  {
    "BlackBox"
    , "ITLBMiss"
    , "NOP"
    , "MAGIC"
    //ALU
    , "SETHI"
    , "ALU"
    , "Mul"
    , "Div"
    , "RDPR"
    , "WRPR"
    //Register Window Manipulation
    , "Restore"
    , "Save"
    , "Restored"
    , "Saved"
    , "FLUSHW"
    //FP
    , "FP"
    , "ALIGN"
    //Memory
    , "Load"
    , "LoadFP"
    , "LDD"
    , "Store"
    , "StoreFP"
    , "STD"
    //Atomics
    , "CAS"
    , "SWAP"
    , "LDSTUB"
    //Branches
    , "BranchUnconditional"
    , "BranchConditional"
    , "BranchFPConditional"
    , "CALL"
    , "RETURN"
    //MEMBARs
    , "MEMBARSync"
    , "MEMBARStLd"
    , "MEMBARStSt"
    //Unsupported Instructions
    , "RDPRUnsupported"
    , "WRPRUnsupported"
    , "POPCUnsupported"
    , "RETRYorDONE"
    , "ExceptionUnsupported"
    , "Exception"
    , "SideEffectLoad"
    , "SideEffectStore"
    , "SideEffectAtomic"
    , "DeviceAccess"
    , "MMUAccess"
    , "Tcc"
  };
  if (aCode >= codeLastCode) {
    anOstream << "Invalid code(" << static_cast<int>(aCode) << ")";
  } else {
    anOstream << insn_codes[aCode];
  }
  return anOstream;

}

} //nuArch
