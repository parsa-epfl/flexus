#ifndef FLEXUS_CORE_TARGET_HPP_INCLUDED
#define FLEXUS_CORE_TARGET_HPP_INCLUDED

#define CAT(x, y)   CAT_D(x, y)
#define CAT_D(x, y) x##y

#ifdef TARGET_PLATFORM

#define TARGET_PLATFORM_aarch64  0
#define TARGET_PLATFORM_riscv    1
#define DETECTED_TARGET_PLATFORM CAT(TARGET_PLATFORM_, TARGET_PLATFORM)

#if DETECTED_TARGET_PLATFORM == TARGET_PLATFORM_aarch64
#include <core/targets/AARCH64.hpp>
#elif DETECTED_TARGET_PLATFORM == TARGET_PLATFORM_riscv
#include <core/targets/RISCV.hpp>
#else
#error "No correct platform specified"
#endif

#else
#error "TARGET_PLATFORM was not passed in from the make command line"
#endif

#undef CAT
#undef CAT_D

#endif // FLEXUS_CORE_TARGET_HPP_INCLUDED
