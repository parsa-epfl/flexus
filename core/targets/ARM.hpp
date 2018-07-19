#if defined(FLEXUS_TARGET) && (FLEXUS_TARGET == 0)
#error "Only a single target may be defined in a simulator"
#endif //FLEXUS_TARGET

#define FLEXUS_LITTLE_ENDIAN 1
#define FLEXUS_BIG_ENDIAN 0

#define FLEXUS_TARGET_x86 0
#define FLEXUS_TARGET_v9 0
#define FLEXUS_TARGET_ARM 1

#define FLEXUS_TARGET FLEXUS_TARGET_ARM
#define FLEXUS_TARGET_WORD_BITS 64
#define FLEXUS_TARGET_VA_BITS 64
#define FLEXUS_TARGET_PA_BITS 64
#define FLEXUS_TARGET_ENDIAN FLEXUS_LITTLE_ENDIAN

#define FLEXUS_TARGET_IS(target) ( FLEXUS_TARGET == FLEXUS_TARGET_ ## target )

#define TARGET_VA_BITS 64
#define TARGET_PA_BITS 64
#define TARGET_ARM

#define TARGET_MEM_TRANS arm_memory_transaction_t
