/*
  V9 Memory Op
*/

#include <core/types.hpp>
#include <core/flexus.hpp>

#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include "StoreBuffer.hpp"

#include <core/simics/mai_api.hpp>

namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"

} //namespace API
} //namespace Simics
} //namespace Flexus

#include "DebugTrace.hpp"
#include "TraceConsumer.hpp"

using namespace Flexus::Simics::API;

namespace Flexus {
namespace SharedTypes {

void ArchitecturalInstruction::perform() {

  thePerformed = true;

  if (!isShadow() && !isTrace()) {

    DBG_Assert(! ( isStore() && !isSync() && (theStoreBuffer == 0) && FLEXUS_TARGET_IS(v9) ) );

    if (theStoreBuffer) {

      PhysicalMemoryAddress aligned_addr(thePhysicalAddress & ~7LL);
      DBG_(VVerb, Addr( thePhysicalAddress ) ( << "Performing Store @" << &std::hex << thePhysicalAddress << " aligned: " << aligned_addr << &std::dec << " value: " << theData) );

      //Perform the store
      //The correct value is in the store buffer
      switch ( theSize ) {
        case 1: {
          SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getByte(thePhysicalAddress & 7), theSize);
          DBG_(VVerb, ( << "    Wrote: " << &std::hex << (uint32_t) theData.getByte(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
        }
        break;
        case 2: {
          SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getHalfWord(thePhysicalAddress & 7), theSize);
          DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getHalfWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );

        }
        break;
        case 4: {
          SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getWord(thePhysicalAddress & 7), theSize);
          DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
        }
        break;
        case 8: {
          SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getDoubleWord(thePhysicalAddress & 7), theSize);
          DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getDoubleWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
        }
        break;
        default:
          DBG_Assert( false, ( << "Store has invalid size: " << theSize) );
      }

      StoreBuffer::iterator iter = theStoreBuffer->find(aligned_addr);
      DBG_Assert(iter != theStoreBuffer->end());

      //Decrement the outstanding store count
      if (--(iter->second) == 0) {
        DBG_(VVerb, ( << "  Last store to addr.  Freeing SB entry." ) );

        //Remove the sb_entry if the count reaches zero
        theStoreBuffer->erase(iter);
      }

    } //if (theStoreBuffer)
  } //if (!isShadow() && !isTrace())

  if (isTrace() && !isShadow() && !isNOP()) {
    DBG_(Crit, ( << "Transaction completed in "
                 << Flexus::Core::theFlexus->cycleCount() - theStartTime
                 << " cycles"));
  }

}

void ArchitecturalInstruction::release() {
  DBG_Assert( (theReleased == false) );
  theReleased = true;
  if (! isShadow() && theConsumer) {
    theConsumer->releaseInstruction(*this);
  }
}

} //namespace SharedTypes
} //namespace Flexus

