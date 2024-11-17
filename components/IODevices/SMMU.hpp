#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/qemu/api_wrappers.hpp>
#include <core/simulator_layout.hpp>
#include "components/Decoder/BitManip.hpp"
#include "components/MMU/MMUUtil.hpp"
#include "components/MMU/pageWalk.hpp"
#include "components/MMU/mmuRegisters.hpp"

#include "SMMUTypes.hpp"
#include "IOTLB.hpp"

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SMMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()



COMPONENT_PARAMETERS(
    PARAMETER( IOTLBSize,    size_t, "Size of the IOTLB", "iotlbsize", 64 )
    PARAMETER( PerfectTLB,  bool, "IOTLB never misses", "perfect", true )
);

COMPONENT_INTERFACE(

    // CPU uses this interface to talk to the SMMU
    // CPU writes to MMIO registers of SMMU to insert commands
    // Eg. IOTLB Invalidation command
    // CPU uses this port to get the status of SMMU as well
    // Eg. Progress of IOTLB invalidation, Fault Occurrence 
    PORT( PushInput, MemoryMessage, CPUMemoryRequest )

    // SMMU uses this port to communicate to LLC or Memory
    // to read data like entries in Command, Event or PRI Queues
    // These queues are memory resident
    PORT( PushOutput, MemoryMessage, MemoryRequest )

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

    // This channel takes in memory request from the device
    // If required, it performs translation
    // Then it performs the memory operation and returns the 
    // message with the data to the sender
    PORT( PushInput, MemoryMessage, DeviceMemoryRequest )

    DRIVE(SMMUDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MMU
// clang-format on
