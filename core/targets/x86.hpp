#if defined(FLEXUS_TARGET) && (FLEXUS_TARGET == 1)
#error "Only a single target may be defined in a simulator"
#endif //FLEXUS_TARGET

#define FLEXUS_LITTLE_ENDIAN 0
#define FLEXUS_BIG_ENDIAN 1

#define FLEXUS_TARGET_x86 0
#define FLEXUS_TARGET_v9 1

#define FLEXUS_TARGET FLEXUS_TARGET_x86
#define FLEXUS_TARGET_WORD_BITS 32
#define FLEXUS_TARGET_VA_BITS 32
#define FLEXUS_TARGET_PA_BITS 32
#define FLEXUS_TARGET_ENDIAN FLEXUS_LITTLE_ENDIAN

#define FLEXUS_TARGET_IS(target) ( FLEXUS_TARGET == FLEXUS_TARGET_ ## target )

#define TARGET_VA_BITS 32
#define TARGET_PA_BITS 32
#define TARGET_X86

#define TARGET_MEM_TRANS x86_memory_transaction_t

#define FLEXUS_SIMICS_TARGET_REGISTER_ID() <core/simics/x86/register_id.hpp>
#define FLEXUS_SIMICS_TARGET_INSTRUCTION_EXTENSTIONS() <core/simics/x86/instructions.hpp>

#include <simics/global.h>
#if SIM_VERSION > 1300
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/core/HEADER.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/arch/x86.h>
#else
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/HEADER##_api.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/x86_api.h>
#endif
