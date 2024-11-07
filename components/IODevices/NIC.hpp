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
    PORT( PushInput, MemoryMessage, MemoryRequest )
    PORT( PushOutput, MemoryMessage, MemoryResponse )

    PORT ( PushOutput, TranslationPtr, TranslationRequestOut )

    DRIVE( UpdateNICState )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NIC
// clang-format on
