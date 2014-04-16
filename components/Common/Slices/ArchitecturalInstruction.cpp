#include <components/Common/Slices/ArchitecturalInstruction.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator <<(std::ostream & anOstream, const ArchitecturalInstruction & aMemOp) {
  anOstream << "SimicsMemoryOp: " << aMemOp.opName() << " [PC=" << &std::hex
            << aMemOp.physicalInstructionAddress() << "] ";
  if (aMemOp.isMemory()) {
    anOstream << "addr=" << aMemOp.physicalMemoryAddress();
  }
  return anOstream;
}

bool operator == (const ArchitecturalInstruction & a, const ArchitecturalInstruction & b) {
  if ( a.physicalInstructionAddress() != b.physicalInstructionAddress() ) {
    return false;
  }
  if ( a.opName() != b.opName() ) {
    return false;
  }
  if ( a.isMemory() ) {
    if ( a.physicalMemoryAddress() != b.physicalMemoryAddress() ) {
      return false;
    }
  }
  return true;
}

} //namespace SharedTypes
} //namespace Flexus

