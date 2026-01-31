//

/* Basic goals:
 * - sub-class the CacheController with TLB specific functionality
 * - adds a large interface to the MMU component in core/qemu
 * - improve readability and usability
 */
#include "TLBController.hpp"

using namespace Flexus;

#define DBG_DeclareCategories TLBCtrl
#define DBG_SetDefaultOps     AddCat(TLBCtrl) Set((CompName) << theName) Set((CompIdx) << theNodeId)
#include DBG_Control()

namespace nTLB {
} // end namespace nTLB

#define DBG_Reset
#include DBG_Control()
// EOF
