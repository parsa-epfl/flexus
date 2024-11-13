#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT NIC
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()



COMPONENT_PARAMETERS(
    PARAMETER( numberOfTXQueues,    size_t, "Number of Transmit Queues", "numberOfTXQueues", 1 )
    PARAMETER( numberOfRXQueues,    size_t, "Number of Receive Queues", "numberOfRXQueues", 1 )
);

COMPONENT_INTERFACE(

    PORT( PushInput, MemoryMessage, CPUMemoryRequest )              // This channel for the CPU to access the device MMIO registers
    PORT( PushOutput, MemoryMessage, CPUMemoryResponse )            // This channel sends the response to CPU for the MMIO register access

    PORT( PushOutput, MemoryMessage, DeviceMemoryRequest )          // This channel is used for sending translated or untranslated memory requests
                                                                    // In case of an untransated request, the SMMU translates it and forwards the
                                                                    // request to the memory or LLC
                                                                    // NOTE: For an IO device, there is no theAssociatedPC field
                                                                    // So we use this field for hilding the IOVA of the memory request
                                                                    // At this point, theAddress field will be 0. This allows us to 
                                                                    // distinguish between untranslated and translated request: If theAddress == 0
                                                                    // it is an untranslated request, otherwise it is translated
    
    PORT( PushInput, MemoryMessage, DeviceMemoryResponse )         // Memory response is received for reads and non-posted writes

    PORT ( PushOutput, TranslationPtr, TranslationRequestOut )      // This will be used in ATS for translation request
                                                                    // After the push function returns, the behavior of TranslationPtr
                                                                    // should be similar to as if we received a Translation Response
                                                                    // However, doing things this way does not reflect the actual PCIe
                                                                    // traffic as PCIe traffic would also have the Translation Response as
                                                                    // a separate message, but here the response is not sent as an explicit message.
                                                                    // Instead, it is iferred as being sent when the push function returns.

                                                                    // In conclusion, every kind of memory request and translation request goes through
                                                                    // the SMMU. The SMMU may decide to allow the request to bypass the permission checks
                                                                    // it deems it appropriate.

    DRIVE( UpdateNICState )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NIC
// clang-format on
