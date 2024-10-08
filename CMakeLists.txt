cmake_minimum_required(VERSION 3.25)
project(flexus)

# Add crucial definiton for compilating Flexus
# TARGET_PLATFORM = Only platform supported so far, so kind of useless
add_compile_definitions(FLEXUS)
add_compile_definitions(TARGET_PLATFORM=aarch64)
add_compile_definitions(BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS)
add_compile_definitions(BOOST_MPL_LIMIT_VECTOR_SIZE=50)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#Include simulator specific settings only for "real" simulators
include(./target/${SIMULATOR}/${SIMULATOR}.cmake)

# Setup include paths
include_directories(.)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-error=maybe-uninitialized -Wno-error=dangling-pointer -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -g3 -fno-eliminate-unused-debug-symbols -fsanitize=address -fno-omit-frame-pointer -funroll-loops")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# Find boost
set(Boost_USE_DEBUG_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS system iostreams regex serialization)
include_directories(${Boost_INCLUDE_DIRS})

# compile core
file(GLOB_RECURSE CORE_SOURCE ./core/*.cpp)
add_library(core STATIC ${CORE_SOURCE})
target_compile_options(core PRIVATE)

# compile components
foreach(COMPONENT ${REQUIRED_COMPONENTS})
    file(GLOB_RECURSE COMPONENT_${COMPONENT}_SOURCE "./components/${COMPONENT}/*.cpp")
    add_library(${COMPONENT} STATIC ${COMPONENT_${COMPONENT}_SOURCE})
    target_compile_options(${COMPONENT} PRIVATE)
endforeach()

# compile simulators
add_library(${SIMULATOR} SHARED ./target/${SIMULATOR}/wiring.cpp)
target_link_libraries(${SIMULATOR} "-Wl,--whole-archive" "-Wl,--no-undefined" core ${REQUIRED_COMPONENTS})
target_link_libraries(${SIMULATOR} "-Wl,--no-whole-archive" ${Boost_LIBRARIES} boost_system boost_regex boost_serialization boost_iostreams)
