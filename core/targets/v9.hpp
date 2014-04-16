#ifdef FLEXUS_TARGET
#error "Only a single target may be defined in a simulator"
#endif //FLEXUS_TARGET

#define FLEXUS_LITTLE_ENDIAN 0
#define FLEXUS_BIG_ENDIAN 1

#define FLEXUS_TARGET_x86 0
#define FLEXUS_TARGET_v9 1

#define FLEXUS_TARGET FLEXUS_TARGET_v9
#define FLEXUS_TARGET_WORD_BITS 64
#define FLEXUS_TARGET_VA_BITS 64
#define FLEXUS_TARGET_PA_BITS 64
#define FLEXUS_TARGET_ENDIAN FLEXUS_BIG_ENDIAN

#define FLEXUS_TARGET_IS(target) ( FLEXUS_TARGET == FLEXUS_TARGET_ ## target )

#define TARGET_VA_BITS 64
#define TARGET_PA_BITS 64
#define TARGET_SPARC_V9
#define TARGET_ULTRA

#define TARGET_MEM_TRANS v9_memory_transaction_t

#define FLEXUS_SIMICS_TARGET_REGISTER_ID() <core/simics/v9/register_id.hpp>
#define FLEXUS_SIMICS_TARGET_INSTRUCTION_EXTENSTIONS() <core/simics/v9/instructions.hpp>

#include <simics/global.h>
#if SIM_VERSION > 1300
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/core/HEADER.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/arch/sparc.h>
#else
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/HEADER##_api.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/sparc_api.h>
#endif

