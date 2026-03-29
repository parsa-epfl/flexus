//

/* Basic goals:
 * - sub-class the CacheController with TLB specific functionality
 * - adds a large interface to the MMU component in core/qemu
 * - improve readability and usability
 */

#ifndef FLEXUS_TLB_CONTROLLER_HPP_INCLUDED
#define FLEXUS_TLB_CONTROLLER_HPP_INCLUDED

#include <components/Cache/CacheController.hpp>

#define DBG_DeclareCategories TLBCtrl
#define DBG_SetDefaultOps     AddCat(TLBCtrl)
#include DBG_Control()

namespace nTLB {

using namespace nMessageQueues;
using namespace nCache;
typedef Flexus::SharedTypes::MemoryTransport Transport;
typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

} // end namespace nTLB

#define DBG_Reset
#include DBG_Control()

#endif // FLEXUS_TLB_CONTROLLER_HPP_INCLUDED
