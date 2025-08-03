#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <core/types.hpp>
#include <iostream>

namespace Flexus {
namespace SharedTypes {

eSize
dbSize(uint32_t aSize)
{
    switch (aSize) {
        case 8: return kByte;
        case 16: return kHalfWord;
        case 32: return kWord;
        case 64: return kDoubleWord;
        case 128: return kQuadWord;
        default: assert(false);
    }
    __builtin_unreachable();
}

std::ostream&
operator<<(std::ostream& anOstream, eSize op)
{
    std::string str;
    switch (op) {
        case kByte: str = "Byte"; break;
        case kHalfWord: str = "HalfWord"; break;
        case kWord: str = "Word"; break;
        case kDoubleWord: str = "DoubleWord"; break;
        case kQuadWord: str = "QuadWord"; break;
        case kIllegalSize:
        default: str = "Invalid size"; break;
    }
    anOstream << str;
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, eOperation op)
{
    const char* map_tables[] = { "PageWalkRequest",
                                 "Load",
                                 "Load Pair",
                                 "AtomicPreload",
                                 "RMW",
                                 "CAS",
                                 "CAS Pair",
                                 "StorePrefetch",
                                 "Store",
                                 "Invalidate",
                                 "Downgrade",
                                 "Probe",
                                 "Return",
                                 "LoadReply",
                                 "AtomicPreloadReply",
                                 "StoreReply",
                                 "StorePrefetchReply",
                                 "RMWReply",
                                 "CASReply",
                                 "DowngradeAck",
                                 "InvAck",
                                 "ProbeAck",
                                 "ReturnReply",
                                 "MEMBARMarker",
                                 "Invalid Operation",
                                 "LastOperation" };
    if (op >= kLastOperation) {
        anOstream << "Invalid Operation(" << static_cast<int>(op) << ")";
    } else {
        anOstream << map_tables[op];
    }
    return anOstream;
}

std::ostream&
operator<<(std::ostream& anOstream, MemOp const& aMemOp)
{
    anOstream
      // << "instr=[ " << aMemOp.theInstruction << " ] "
      << aMemOp.theOperation << "(" << aMemOp.theSize << ") " << aMemOp.theVAddr << aMemOp.thePAddr
      << " pc:" << aMemOp.thePC;
    if (aMemOp.theReverseEndian) { anOstream << " {reverse-endian}"; }
    if (aMemOp.theNonCacheable) { anOstream << " {non-cacheable}"; }
    if (aMemOp.theSideEffect) { anOstream << " {side-effect}"; }
    if (aMemOp.theNAW) { anOstream << " {non-allocate-write}"; }
    anOstream << " =" << std::hex << aMemOp.theValue;
    return anOstream;
}

} // namespace SharedTypes
} // namespace Flexus