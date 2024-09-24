
#ifndef FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED
#define FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

// Unconditional branch (immediate)
archinst
UNCONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Unconditional branch (register)
archinst
BR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
BLR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
RET(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
ERET(archcode const& aFetchedOpcode,
     uint32_t aCPU,
     int64_t aSequenceNo); // TODO
archinst
DPRS(archcode const& aFetchedOpcode,
     uint32_t aCPU,
     int64_t aSequenceNo); // TODO

// Compare and branch (immediate)
archinst
CMPBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Test and branch (immediate)
archinst
TSTBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional branch (immediate)
archinst
CONDBR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// System TODO
archinst
HINT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SYNC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
MSR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SYS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Exception generation TODO
archinst
SVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
HVC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SMC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
BRK(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
HLT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
DCPS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder
#endif // FLEXUS_armDECODER_armBRANCH_HPP_INCLUDED