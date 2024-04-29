cmake_minimum_required(VERSION 3.25)
project(flexus)


#Include simulator specific settings only for "real" simulators
include(${FLEXUS_ROOT}/target/${SIMULATOR}/${SIMULATOR}.cmake)

# Setup include paths
include_directories(${FLEXUS_ROOT})
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-error=maybe-uninitialized -Wno-error=dangling-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -g3 -fno-eliminate-unused-debug-symbols -fsanitize=address -fno-omit-frame-pointer -funroll-loops")
# compile core
file(GLOB_RECURSE CORE_SOURCE ${FLEXUS_ROOT}/core/*.cpp)
file(GLOB_RECURSE QEMU_SOURCE ${FLEXUS_ROOT}/core/qemu/*.cpp)
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/test")
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/qemu")
add_library(core STATIC ${CORE_SOURCE})
add_library(qemu STATIC ${QEMU_SOURCE})
target_compile_options(core PRIVATE)
target_compile_options(qemu PRIVATE)

# compile components
foreach(COMPONENT ${${SIMULATOR}_REQUIRED_COMPONENTS})
    file(GLOB_RECURSE COMPONENT_${COMPONENT}_SOURCE "${FLEXUS_ROOT}/components/${COMPONENT}/*.cpp")
    if (${COMPONENT} STREQUAL "NetShim")
        list(FILTER COMPONENT_NetShim_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/components/NetShim/testing")
    endif()
    add_library(${COMPONENT} STATIC ${COMPONENT_${COMPONENT}_SOURCE})
    target_compile_options(${COMPONENT} PRIVATE)
endforeach()

# Find boost
set(Boost_USE_DEBUG_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS system iostreams regex serialization)

# compile simulators
add_library(wiring OBJECT ${FLEXUS_ROOT}/target/${SIMULATOR}/wiring.cpp)
target_compile_options(wiring PRIVATE)
add_library(${SIMULATOR} SHARED $<TARGET_OBJECTS:wiring>)
target_link_libraries(${SIMULATOR} "-Wl,--whole-archive" ${${SIMULATOR}_REQUIRED_COMPONENTS})
target_link_libraries(${SIMULATOR} "-Wl,--whole-archive" core qemu)
target_link_libraries(${SIMULATOR} "-Wl,--no-whole-archive" ${Boost_LIBRARIES} boost_system boost_regex boost_serialization boost_iostreams)