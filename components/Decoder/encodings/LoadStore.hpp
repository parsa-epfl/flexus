
#ifndef FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED
#define FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

// Load/store exclusive
// archinst CASP(archcode const & aFetchedOpcode, uint32_t  aCPU, int64_t
// aSequenceNo);
archinst
CAS(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
STXR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
STRL(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDAQ(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDXR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Load register (literal)
archinst
LDR_lit(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Load/store pair (all forms)
archinst
LDP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
STP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

/* Load/store register (all forms) */
archinst
LDR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
STR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

/* atomic memory operations */ // TODO
archinst
LDADD(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDCLR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDEOR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDSET(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDSMAX(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDSMIN(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDUMAX(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
LDUMIN(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SWP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armLOADSTORE_HPP_INCLUDED