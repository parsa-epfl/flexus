#ifndef _FLEXPRINT_H_
#define _FLEXPRINT_H_

#ifndef FLEXPRINT_ENABLED

#define FLEXPRINT_MARKER(aThread, aValue) while (0) /**/

static inline
void
flexprint_sci(int    aThread,
              char * aFile,
              int    aLine,
              int    aValue)
{ } /* No effect */


#else /* FLEXPRINT_ENABLED */

#include <unistd.h>

#ifdef NO_FLEXUS
#include <stdlib.h>
#include <stdio.h>
#endif

#if defined(__sparc)
static void
flexprint_int(long aVal)
{
  long long g1back;
  __asm__ __volatile__ (
     "mov  %%g1, %0     \n"
     "mov  %1, %%g1     \n"
     "sethi 0x666, %%g0 \n"
     "mov  %0, %%g1     \n"
   : "=r"  (g1back)
   : "r"  (aVal)
   );
}

static void
flexprint_str(char const * aStr)
{
  long long g1back;
  __asm__ __volatile__ (
     "mov  %%g1, %0     \n"
     "mov  %1, %%g1     \n"
     "sethi 0x667, %%g0 \n"
     "mov  %0, %%g1     \n"
   : "=r"  (g1back)
   : "r"  (aStr)
   );
}
#endif

#define FLEXPRINT_MARKER(aThread, aValue) flexprint_sci( aThread, __FUNCTION__, __FILE__, __LINE__, aValue )    /**/

#ifdef NO_FLEXUS

static inline
void
flexprint_sci(int    aThread,
              char * aFunction,
              char * aFile,
              int    aLine,
              int    aValue)
{
  fprintf(stderr, "Td[%d]\t\t%lld\t\t%s:%d %s - %d\n", aThread, gethrtime(), aFile, aLine, aFunction, aValue);
  fflush(stderr);
}

#else /* Normal version */

static inline
void
flexprint_sci(int    aThread,
              char * aFunction,
              char * aFile,
              int    aLine,
              int    aValue)
{
#if defined(__sparc)
  long long g1back;
  long long g2back;
  long long g3back;
  long long g4back;
  long long g5back;
  volatile int td = aThread;
  volatile char * fn = aFunction;
  volatile char * fl = aFile;
  volatile int ln = aLine;
  volatile int val = aValue;

  __asm__ __volatile__ (
     "mov  %%g1, %0     \n"
     "mov  %%g2, %1     \n"
     "mov  %%g3, %2     \n"
     "mov  %%g4, %3     \n"
     "mov  %%g5, %4     \n"
     "mov  %5, %%g1     \n"
     "mov  %6, %%g2     \n"
     "mov  %7, %%g3     \n"
     "mov  %8, %%g4     \n"
     "mov  %9, %%g5     \n"
     "sethi 0x669, %%g0 \n"
     "mov  %0, %%g1     \n"
     "mov  %1, %%g2     \n"
     "mov  %2, %%g3     \n"
     "mov  %3, %%g4     \n"
     "mov  %4, %%g5     \n"
   : "=r"  (g1back)
   , "=r"  (g2back)
   , "=r"  (g3back)
   , "=r"  (g4back)
   , "=r"  (g5back)
   :  "r"  (td)
   ,  "r"  (fn)
   ,  "r"  (fl)
   ,  "r"  (ln)
   ,  "r"  (val)
   );

#else

  volatile int td = aThread;
  volatile const char * fn = aFunction;
  volatile const char * fl = aFile;
  volatile int ln = aLine;
  volatile int val = aValue;

  __asm__ __volatile__ (
     "movl  %0, %%eax     \n"
     "movl  %1, %%ebx     \n"
     "movl  %2, %%ecx     \n"
     "movl  %3, %%edx     \n"
     "movl  %4, %%esi     \n"
     "xchg  %%bx, %%bx    \n"
   :
   :  "m"  (td)
   ,  "m"  (fn)
   ,  "m"  (fl)
   ,  "m"  (ln)
   ,  "m"  (val)
   :  "eax", "ebx", "ecx", "edx", "esi"
   );

#endif

}

#endif /* NO_FLEXUS */

#endif /* FLEXPRINT_ENABLED */

#endif /* _FLEXPRINT_H_ */
