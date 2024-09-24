
#ifndef FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED
#define FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED

#include "components/Decoder/Instruction.hpp"

using namespace nuArch;

namespace nDecoder {

// Logical (shifted register)
archinst
LOGICAL(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Add/subtract (shifted register / extended register)
archinst
ADDSUB_SHIFTED(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
ADDSUB_EXTENDED(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (3 source)
archinst
DP_1_SRC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
DP_2_SRC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
DP_3_SRC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Add/subtract (with carry)
archinst
ADDSUB_CARRY(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional compare (immediate / register)
archinst
CCMP(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Conditional select
archinst
CSEL(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (1 source)
archinst
RBIT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
REV(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
CL(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

// Data-processing (2 sources)
archinst
DIV(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
SHIFT(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
CRC(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armDATAPROCREG_HPP_INCLUDED
