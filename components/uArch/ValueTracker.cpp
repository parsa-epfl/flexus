#include "ValueTracker.hpp"

namespace nuArch {

  ValueTracker ** ValueTracker::theGlobalTracker = nullptr;
  int ValueTracker::theNumTrackers = 0;

  API::cycles_t DMATracerImpl::dma_mem_hier_operate(API::conf_object_t * space, API::map_list_t * map, API::generic_transaction_t * aMemTrans) {
  Flexus::Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Flexus::Simics::APIFwd::memory_transaction_t *>(aMemTrans);

  const int32_t k_no_stall = 0;

  //debugTransaction(mem_trans);
  mem_trans->s.block_STC = 1;

  if (API::SIM_mem_op_is_write(&mem_trans->s)) {
    DBG_( Iface, ( << "DMA To Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
    ValueTracker::valueTracker(theVM).invalidate( (PhysicalMemoryAddress)mem_trans->s.physical_address, (eSize)mem_trans->s.size);
  } else {
    DBG_( Iface, ( << "DMA From Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
  }

  return k_no_stall; //Never stalls
}

} //nuArch
