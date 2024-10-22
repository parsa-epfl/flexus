
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
    PARAMETER( iTLBSet,    size_t,  "Set count of the Instruction TLB",  "itlb_set", 1 )
    PARAMETER( iTLBAssoc,  size_t,  "Associativity of the Instruction TLB", "itlb_assoc", 64 )
    PARAMETER( dTLBSet,    size_t,  "Set count of the Data TLB",         "dtlb_set", 64 )
    PARAMETER( dTLBAssoc,  size_t,  "Associativity of the Data TLB",     "dtlb_assoc", 64 )
    PARAMETER( sTLBlat,     int,    "sTLB lookup latency",          "stlblat",  2)
    PARAMETER( sTLBSet,    size_t,  "Set count of the Second-level TLB", "stlb_sete", 2048 )
    PARAMETER( sTLBAssoc,  size_t,  "Associativity of the Second-level TLB", "stlb_assoc", 4 )
    PARAMETER( PerfectTLB,  bool,   "TLB never misses",             "perfect",  true )
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
