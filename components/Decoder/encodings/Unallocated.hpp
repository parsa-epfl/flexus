
#ifndef FLEXUS_armDECODER_armUNALLOCATED_HPP_INCLUDED
#define FLEXUS_armDECODER_armUNALLOCATED_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

archinst
blackBox(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
grayBox(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, eInstructionCode aCode);
archinst
nop(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

archinst
unallocated_encoding(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
unsupported_encoding(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_armUNALLOCATED_HPP_INCLUDED