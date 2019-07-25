cmake_minimum_required(VERSION 3.10.2)
project(flexus)

include(Configuration.cmake)
include(${FLEXUS_ROOT}/simulators/${SIMULATOR}/${SIMULATOR}.cmake)
include_directories(${FLEXUS_ROOT})
include_directories(SYSTEM ${BOOST_INCLUDEDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FLEXUS_ROOT}/CMakeFiles)

# compile core
file(GLOB_RECURSE CORE_SOURCE ${FLEXUS_ROOT}/core/*.cpp)
file(GLOB_RECURSE QEMU_SOUECE ${FLEXUS_ROOT}/core/qemu/*.cpp)
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/test")
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/qemu")
add_library(core STATIC ${CORE_SOURCE})
add_library(qemu STATIC ${QEMU_SOUECE})
target_compile_options(core PRIVATE -c ${GCC_FLAGS})
target_compile_options(qemu PRIVATE -c ${GCC_FLAGS})

# compile components
foreach(COMPONENT ${${SIMULATOR}_REQUIRED_COMPONENTS})
    file(GLOB_RECURSE COMPONENT_${COMPONENT}_SOURCE "${FLEXUS_ROOT}/components/${COMPONENT}/*.cpp")
    if (${COMPONENT} STREQUAL "NetShim")
        list(FILTER COMPONENT_NetShim_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/components/NetShim/testing")
    endif()
    add_library(${COMPONENT} STATIC ${COMPONENT_${COMPONENT}_SOURCE})
    target_compile_options(${COMPONENT} PRIVATE -c ${GCC_FLAGS})
endforeach()

# compile simulators
add_library(wiring OBJECT ${FLEXUS_ROOT}/simulators/${SIMULATOR}/wiring.cpp)
target_compile_options(wiring PRIVATE -c ${GCC_FLAGS})
add_library(${SIMULATOR} SHARED $<TARGET_OBJECTS:wiring>)
target_link_libraries(${SIMULATOR} "-Wl,--whole-archive" "-Wl,-export-dynamic" ${${SIMULATOR}_REQUIRED_COMPONENTS})
target_link_libraries(${SIMULATOR} "-Wl,--whole-archive" core qemu "-Wl,--no-whole-archive")
target_link_libraries(${SIMULATOR} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)

# clean for cmake
add_custom_target(clean_cmake
    COMMAND rm -rf *.o *.a *.so
    COMMAND rm -rf CMakeFiles
    COMMAND rm -rf cmake_install.cmake
    COMMAND rm -rf CMakeCache.txt
    COMMAND rm -rf Makefile
    COMMAND echo "All cleared."
)
