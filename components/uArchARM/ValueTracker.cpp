// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#include "ValueTracker.hpp"

namespace nuArchARM {

ValueTracker **ValueTracker::theGlobalTracker = nullptr;
int ValueTracker::theNumTrackers = 0;

// ALEX - WARNING: Disabled dma for now
/*
  API::cycles_t DMATracerImpl::dma_mem_hier_operate(API::conf_object_t * space,
API::map_list_t * map, API::generic_transaction_t * aMemTrans) {
  Flexus::Qemu::APIFwd::memory_transaction_t * mem_trans =
reinterpret_cast<Flexus::Qemu::APIFwd::memory_transaction_t *>(aMemTrans);

  const int32_t k_no_stall = 0;

  //debugTransaction(mem_trans);
  mem_trans->s.block_STC = 1;

  if (API::SIM_mem_op_is_write(&mem_trans->s)) {
    DBG_( Iface, ( << "DMA To Mem: " << std::hex <<
mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size )
); ValueTracker::valueTracker(theVM).invalidate(
(PhysicalMemoryAddress)mem_trans->s.physical_address, (eSize)mem_trans->s.size);
  } else {
    DBG_( Iface, ( << "DMA From Mem: " << std::hex <<
mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size )
);
  }

  return k_no_stall; //Never stalls
}
*/
} // namespace nuArchARM
