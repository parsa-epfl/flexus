#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/qemu/api_wrappers.hpp>
#include <core/simulator_layout.hpp>
#include "components/Decoder/BitManip.hpp"
#include "components/MMU/MMUUtil.hpp"
#include "components/MMU/pageWalk.hpp"
#include "components/MMU/mmuRegisters.hpp"

#include "IOTLB.hpp"

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SMMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()



COMPONENT_PARAMETERS(
    PARAMETER( IOTLBSize,    size_t, "Size of the IOTLB", "iotlbsize", 64 )
    PARAMETER( PerfectTLB,  bool, "IOTLB never misses", "perfect", true )
);

COMPONENT_INTERFACE(
    // this is for trace
    // IO devices send TranslationPtr to SMMU
    // SMMU looks up the IOTLB and performs PTW if required
    // TODO: Right now, it does not send traffic to cache hierarchy
    // TODO: Instead, it just tells the IO device how many
    // memory accesses, if any, are required to perform the 
    // translation. 
    // TODO: Ideally, there should be a port array connecting 
    // SMMU directly to the LLC.
    PORT( PushInput, TranslationPtr, TranslationRequestIn )

    DRIVE(SMMUDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MMU
// clang-format on
