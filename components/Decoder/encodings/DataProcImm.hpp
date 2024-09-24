
/*
 * The instruction disassembly implemented here matches
 * the instruction encoding classifications documented in
 * the 00bet3.1 release of the A64 ISA XML for ARMv8.2.
 * classification names and decode diagrams here should generally
 * match up with those in the manual.
 */

#ifndef FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED
#define FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED

#include "SharedFunctions.hpp"

namespace nDecoder {

// TODO
archinst
ADR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
EXTR(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
BFM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
MOVE(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

archinst
LOGICALIMM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);
archinst
ALUIMM(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo);

} // namespace nDecoder
#endif // FLEXUS_armDECODER_armDATAPROCIMM_HPP_INCLUDED