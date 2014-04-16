#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

namespace nProtocolEngine {

#ifdef DIGITAL_UNIX_CC
#    define INLINE __inline  // #pragma inline(func)
#else
#    define INLINE inline
#endif

#define NO_DBG                           0x00000000UL

// microcode loader   0x0000000fUL
#define MCD_LOADER_DBG                   0x00000001UL
#define MCD_LOADER_MCD_DUMP_DBG          0x00000002UL
#define MCD_EXECUTE_DBG                  0x00000004UL
#define MCD_INSTR_DBG                    0x00000008UL

// service providers  0x000000f0UL
#define SRVC_PROVIDER_DBG                0x00000010UL

// TSRF               0x00000f00UL
#define TSRF_DBG                         0x00000100UL

// home+remote engine 0x0000f000UL
#define HE_INSTR_DBG                     0x00001000UL
#define HE_EXECUTE_DBG                   0x00002000UL
#define RE_INSTR_DBG                     0x00004000UL
#define RE_EXECUTE_DBG                   0x00008000UL

// protocol engine    0x000f0000UL
#define PROT_ENGINE_DBG                  0x00010000UL

// input queue        0x00f00000UL
#define INPUT_Q_DBG                      0x00100000UL

// thread driver      0x0f000000UL
#define THREAD_DRIVER_DBG                0x01000000UL

// generic            0xf0000000UL
#define FILE_LINE_INFO_DBG               0x10000000UL
#define DEBUGF_STOP_AT_DBG               0x20000000UL   /* -d param defines debug message# to stop */
#define CACHE_INDEX_DBG                  0x40000000UL   /* compile with ___DEBUG_INDEX=xxxx where xxxx is the L2 idx to output */
#define ASSERT_DBG                       0x80000000UL

#ifndef _DEBUG_LEVEL
#   define _DEBUG_LEVEL  (  NO_DBG                              \
                          | ASSERT_DBG                          \
                          /* | HE_EXECUTE_DBG                      */ \
                          /* | RE_EXECUTE_DBG                      */ \
                          /* | SRVC_PROVIDER_DBG                   */ \
                          /* | TSRF_DBG                            */ \
                          /* | INPUT_Q_DBG                         */ \
                          /* | THREAD_DRIVER_DBG                   */ \
                          /* | PROT_ENGINE_DBG                     */ \
                          /* | HE_INSTR_DBG                        */ \
                          /* | RE_INSTR_DBG                        */ \
                          /* | MCD_INSTR_DBG                       */ \
                          /* | MCD_LOADER_DBG                      */ \
                          /* | MCD_EXECUTE_DBG                     */ \
                          /* | MCD_LOADER_MCD_DUMP_DBG             */ \
                          /* | FILE_LINE_INFO_DBG                  */ \
                          /* | DEBUGF_STOP_AT_DBG                  */ \
                          /* | CACHE_INDEX_DBG                     */ \
                         )
#endif

#define NIL_INDEX                 0xffffffffUL
#ifndef ___DEBUG_INDEX
#    define ___DEBUG_INDEX        NIL_INDEX
#endif

#ifndef LOOP_ON_ASSERT_FAIL
#    define LOOP_ON_ASSERT_FAIL   1UL
#endif

#define IS_DEBUG_LEVEL(_x)        (_DEBUG_LEVEL & (_x))

typedef void (* _tExitFuncPtr) (int32_t val);
typedef void (* _tAbortFuncPtr) (void);

extern void (* ___exit) (int32_t val);
extern void (* ___abort) (void);
extern void ___debug_init(int32_t argc, char * argv[], _tExitFuncPtr _exit_fnp, _tAbortFuncPtr _abort_fnp);

extern FILE * _fp_stdin;
extern FILE * _fp_stdout;
extern FILE * _fp_stderr;

extern volatile uint32_t ___debugf_stop_at_num;
extern uint32_t          ___debugf_line;
extern char         *         ___debugf_file;

#define ___MAX(_a, _b)        (((_a) > (_b)) ? (_a) : (_b))
#define ___MIN(_a, _b)        (((_a) < (_b)) ? (_a) : (_b))
#define ___CEIL(_a, _b)       ((((_a) % (_b)) == 0) ? ((_a) / (_b)) : (((_a) / (_b)) + 1))
#define SAFE_DIV(x, y)        ((y) ? ((double)(x) / (double)(y)) : 0.0)
#define SAFE_PERCENT(x, y)    (100.0 * SAFE_DIV((x), (y)))

#define LOG2(x)          ((x)==1 ? 0 :   \
                           ((x)==2 ? 1 :   \
                             ((x)==4 ? 2 :   \
                               ((x)==8 ? 3 :   \
                                 ((x)==16 ? 4 :   \
                                   ((x)==32 ? 5 :   \
                                     ((x)==64 ? 6 :   \
                                       ((x)==128 ? 7 :   \
                                         ((x)==256 ? 8 :   \
                                           ((x)==512 ? 9 :   \
                                             ((x)==1024 ? 10 :   \
                                               ((x)==2048 ? 11 :   \
                                                 ((x)==4096 ? 12 :   \
                                                   ((x)==8192 ? 13 :   \
                                                     ((x)==16384 ? 14 :   \
                                                       ((x)==32768 ? 15 :   \
                                                         ((x)==65536 ? 16 :   \
                                                           ((x)==131072 ? 17 :   \
                                                             ((x)==262144 ? 18 :   \
                                                               ((x)==524288 ? 19 :   \
                                                                 ((x)==1048576 ? 20 :  \
                                                                   ((x)==2097152 ? 21 :  \
                                                                     ((x)==4194304 ? 22 :  \
                                                                       ((x)==8388608 ? 23 : \
                                                                         ((x)==16777216 ? 24 : \
                                                                           ((x)==33554432 ? 25 : \
                                                                             ((x)==67108864 ? 26 : -0xffff)))))))))))))))))))))))))))

#define SCALE_DOWN_HIST(_x, _step)       ((uint32_t) ((_x) / (_step)))
#define RESUME_HIST_SCALE(_x, _step)     ((_x) * (_step))

extern uint32_t GetLog2(uint32_t x);
extern char    *    get_date_string (time_t time_num);  // Time to parse, or 0 for current time

extern FILE * ___stdin_from_file(char * name);
extern FILE * ___stdout_to_file(char * name);
extern FILE * ___stderr_to_file(char * name);
extern void ___stdxxx_close(void);
extern void ___printf(const char * fmt, ...);

extern void ___warning(const char * fmt, ...);
extern void ___error(const char * fmt, ...);
extern void ___fatal(const char * fmt, ...);
extern void ___memfatal(const char * fmt, ...);
extern void ___assert_failed(const char * fmt, ...);

extern void ___debugf(const char * fmt, ...);

extern void          ___debug_switch_off(void);
extern void          ___debug_switch_on(void);
extern uint8_t ___debug_is_trigger_on(void);

#define NIKOS_UNUSED(_x)         (_x) = 0

#define ASSERT_ALWAYS(x)                                     \
 {                                                           \
    static volatile char ___loop___ = LOOP_ON_ASSERT_FAIL;   \
    if (!(x)) {                                              \
        fflush(_fp_stdout);                                  \
        fflush(_fp_stderr);                                  \
        ___assert_failed("(%s) in %s, line %d.\n",           \
                          #x, __FILE__, __LINE__);           \
        fflush(_fp_stdout);                                  \
        fflush(_fp_stderr);                                  \
        while (___loop___)                                   \
            usleep(10);                                      \
        (*___abort)();                                       \
    }                                                        \
 }

#define ASSERT_ALWAYS_MSG(x, y)                              \
 {                                                           \
    static volatile char ___loop___ = LOOP_ON_ASSERT_FAIL;   \
    if (!(x)) {                                              \
        fflush(_fp_stdout);                                  \
        fflush(_fp_stderr);                                  \
        ___assert_failed("(%s) in %s, line %d.\n",           \
                          #x, __FILE__, __LINE__);           \
        ___error y ;                                         \
        fflush(_fp_stdout);                                  \
        fflush(_fp_stderr);                                  \
        while (___loop___)                                   \
            usleep(10);                                      \
        (*___abort)();                                       \
    }                                                        \
 }

#define LOOP_ON_COND_MSG(x, y)                   \
 {                                               \
    static volatile char ___loop___ = 1;         \
    if (x) {                                     \
        fflush(_fp_stdout);                      \
        fflush(_fp_stderr);                      \
        ___error("(%s) in %s, line %d.\n",       \
                  #x, __FILE__, __LINE__);       \
        ___error y ;                             \
        fflush(_fp_stdout);                      \
        fflush(_fp_stderr);                      \
        while (___loop___)                       \
            usleep(10);                          \
    }                                            \
 }

#ifdef ASSERT
#   undef ASSERT
#endif

#ifdef ASSERT_MSG
#   undef ASSERT_MSG
#endif

#if (IS_DEBUG_LEVEL(ASSERT_DBG))
#   define ASSERT(_a)           ASSERT_ALWAYS(_a)
#   define ASSERT_MSG(_a, _b)   ASSERT_ALWAYS_MSG(_a, _b)

#else // _DEBUG_LEVEL != ASSERT_DBG
#   define ASSERT(_a)
#   define ASSERT_MSG(_a, _b)
#endif // _DEBUG_LEVEL

#if (_DEBUG_LEVEL != NO_DBG)
#   if (IS_DEBUG_LEVEL(FILE_LINE_INFO_DBG))
#       define DEBUG(_a, _idx, _b)                                                    \
        {                                                                             \
           if (   IS_DEBUG_LEVEL(_a)                                                  \
               && ___debug_is_trigger_on()                                            \
               && (   !IS_DEBUG_LEVEL(CACHE_INDEX_DBG)                                \
                   || (IS_DEBUG_LEVEL(CACHE_INDEX_DBG) && (   ___DEBUG_INDEX==(_idx)  \
                                                           || NIL_INDEX==(_idx)))))   \
          {                                                                           \
               ___debugf_line = __LINE__;                                             \
               ___debugf_file = __FILE__;                                             \
               ___debugf _b ;                                                         \
          }                                                                           \
        }
#   else // ~FILE_LINE_INFO_DBG
#       define DEBUG(_a, _idx, _b)                                                    \
        {                                                                             \
           if (   IS_DEBUG_LEVEL(_a)                                                  \
               && ___debug_is_trigger_on()                                            \
               && (   !IS_DEBUG_LEVEL(CACHE_INDEX_DBG)                                \
                   || (IS_DEBUG_LEVEL(CACHE_INDEX_DBG) && (   ___DEBUG_INDEX==(_idx)  \
                                                           || NIL_INDEX==(_idx)))))   \
          {                                                                           \
               ___debugf _b ;                                                         \
          }                                                                           \
        }
#   endif // FILE_LINE_INFO_DBG
#else // _DEBUG_LEVEL == NO_DBG
#   define DEBUG(_a, _idx, _b)
#endif // _DEBUG_LEVEL

}  // namespace nProtocolEngine

#endif // UTIL_H
