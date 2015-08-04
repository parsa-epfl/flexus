#ifndef FLEXUS_CORE_TARGET_HPP_INCLUDED
#define FLEXUS_CORE_TARGET_HPP_INCLUDED

#define FLEXUS_TARGET_ID_x86()                                    <core/targets/x86.hpp>
#define FLEXUS_TARGET_ID_v9()                                     <core/targets/v9.hpp>
#define FLEXUS_TARGET_ID_ARM()                                    <core/targets/ARM.hpp>

#define CAT(x,y) CAT_D(x,y)
#define CAT_D(x,y) x##y

#ifdef TARGET_PLATFORM
#define TARGET_PLATFORM_x86 0
#define TARGET_PLATFORM_v9 1
#define TARGET_PLATFORM_arm 2
#define DETECTED_TARGET_PLATFORM CAT(TARGET_PLATFORM_, TARGET_PLATFORM)

#if DETECTED_TARGET_PLATFORM == TARGET_PLATFORM_x86
#include <core/targets/x86.hpp>
#elif DETECTED_TARGET_PLATFORM == TARGET_PLATFORM_v9
#include <core/targets/v9.hpp>
#elif DETECTED_TARGET_PLATFORM == TARGET_PLATFORM_arm
#include <core/targets/ARM.hpp>
#else
#error "No correct platform specified"
#endif

//#include CAT(FLEXUS_TARGET_ID_,TARGET_PLATFORM)()
#else
#error "TARGET_PLATFORM was not passed in from the make command line"
#endif

#undef CAT
#undef CAT_D

/*
#include <iostream>

struct StaticPrint {
  StaticPrint(char * aMessage) { std::cerr << aMessage; }
};
*/

#endif //FLEXUS_CORE_TARGET_HPP_INCLUDED
