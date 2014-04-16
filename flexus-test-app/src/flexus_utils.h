#ifndef _flexus_utils_h_
#define _flexus_utils_h_

#ifndef INLINE
#ifdef __cplusplus
#define INLINE inline
#else
#define INLINE extern inline
#endif
#endif

//////////////// Magic instruction for Simics as defined by Virtutech
#define __MAGIC_CASSERT(p) do {                                    \
        typedef int __check_magic_argument[(p) ? 1 : -1];          \
} while (0)

#if defined(__sparc)
#    if defined(__GNUC__)
#        define MAGIC(n) do {                                      \
                    __MAGIC_CASSERT((n) > 0 && (n) < (1U << 22));  \
                    __asm__ __volatile__ ("sethi " #n ", %g0");    \
                } while (0)
#        define MAGIC_BREAKPOINT MAGIC(0x40000)

#    else  // !__GNUC__
#        define MAGIC(n)
#        define MAGIC_BREAKPOINT
#        warning "Unable to support magic breakpoints!"
#    endif

#elif defined(__i386) || defined(__x86_64__) || defined(__INTEL_COMPILER)
#	 if defined(__GNUC__)
#        define MAGIC(n) do {                              \
	               __MAGIC_CASSERT(!(n));                  \
                   __asm__ __volatile__ ("xchg %bx,%bx");  \
                } while (0)

#        define MAGIC_BREAKPOINT MAGIC(0)
#    else
#        define MAGIC(n)
#        define MAGIC_BREAKPOINT
#        warning "Unable to support magic breakpoints!"
#    endif
#else // !__sparc
#        define MAGIC(n)
#        define MAGIC_BREAKPOINT
#        warning "Unable to support magic breakpoints!"
#        define NO_FLEXUS
#endif

#include "flexprint.h"

//////////////// Magic breakpoints to signal Simics and Flexus
INLINE
void flexBkp_StartWarmup(void)
{
  // fprintf(stderr, "Start Warmup\n");
  #if defined(__sparc)
    MAGIC(1);
  #elif defined(__i386) || defined(__x86_64__)
    //magic instruction on x86 accepts only '0' as argument. This is another trick for 
	//the $FLEXUS_ROOT/components/MagicComponent/breakpoint_tracker.cpp to differentiate
	//between flexus breakpoints
    __asm__ __volatile__ (
                      "xchg %bx, %bx\n"
                      "jmp 1f\n"
                      ".long 0xDEAD0001\n"
                      "1:\n"
                      );
  #endif

  return;
}

INLINE
void flexBkp_StartParallel(void)
{
  // fprintf(stderr, "Start Parallel\n");
  #if defined(__sparc)
    MAGIC(2);
  #elif defined(__i386) || defined(__x86_64__)
    //magic instruction on x86 accepts only '0' as argument. This is another trick for 
	//the $FLEXUS_ROOT/components/MagicComponent/breakpoint_tracker.cpp to differentiate
	//between flexus breakpoints
    __asm__ __volatile__ (
                      "xchg %bx, %bx\n"
                      "jmp 1f\n"
                      ".long 0xDEAD0002\n"
                      "1:\n"
                      );
  #endif
  
  return;
}

INLINE
void flexBkp_EndParallel(void)
{
  // fprintf(stderr, "End Parallel\n");
  #if defined(__sparc)
    MAGIC(3);
  #elif defined(__i386) || defined(__x86_64__)
    //magic instruction on x86 accepts only '0' as argument. This is another trick for 
	//the $FLEXUS_ROOT/components/MagicComponent/breakpoint_tracker.cpp to differentiate
	//between flexus breakpoints
    __asm__ __volatile__ (
                      "xchg %bx, %bx\n"
                      "jmp 1f\n"
                      ".long 0xDEAD0003\n"
                      "1:\n"
                      );
  #endif


  return;
}

INLINE
void flexBkp_Iteration(void)
{
  // fprintf(stderr, "New iteration\n");
  #if defined(__sparc)
    MAGIC(4);
  #elif defined(__i386) || defined(__x86_64__)
    //magic instruction on x86 accepts only '0' as argument. This is another trick for 
	//the $FLEXUS_ROOT/components/MagicComponent/breakpoint_tracker.cpp to differentiate
	//between flexus breakpoints
    __asm__ __volatile__ (
                      "xchg %bx, %bx\n"
                      "jmp 1f\n"
                      ".long 0xDEAD0004\n"
                      "1:\n"
                      );
  #endif


  return;
}

#endif // _flexus_utils_h_
