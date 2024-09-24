
// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()



COMPONENT_PARAMETERS(
    PARAMETER( cores,       size_t, "Number of cores", "cores",  1 )
    PARAMETER( iTLBSize,    size_t, "Size of the Instruction TLB", "itlbsize", 64 )
    PARAMETER( dTLBSize,    size_t, "Size of the Data TLB", "dtlbsize", 64 )
    PARAMETER( PerfectTLB,  bool, "TLB never misses", "perfect", true )
);

COMPONENT_INTERFACE(
    DYNAMIC_PORT_ARRAY( PushInput,  TranslationPtr, iRequestIn )
    DYNAMIC_PORT_ARRAY( PushInput,  TranslationPtr, dRequestIn )
    DYNAMIC_PORT_ARRAY( PushInput,  int, ResyncIn)

    DYNAMIC_PORT_ARRAY( PushOutput, TranslationPtr, iTranslationReply )
    DYNAMIC_PORT_ARRAY( PushOutput, TranslationPtr, dTranslationReply )
    DYNAMIC_PORT_ARRAY( PushOutput, TranslationPtr, MemoryRequestOut )

    DYNAMIC_PORT_ARRAY( PushInput, TranslationPtr, TLBReqIn ) // this is for trace

    DYNAMIC_PORT_ARRAY( PushOutput, int, ResyncOut )


    DRIVE(MMUDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MMU
// clang-format on
