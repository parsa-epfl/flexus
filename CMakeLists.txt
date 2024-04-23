# DO-NOT-REMOVE begin-copyright-block
# QFlex consists of several software components that are governed by various
# licensing terms, in addition to software that was developed internally.
# Anyone interested in using QFlex needs to fully understand and abide by the
# licenses governing all the software components.
#
#### Software developed externally (not by the QFlex group)
#
#    * [NS-3](https://www.gnu.org/copyleft/gpl.html)
#    * [QEMU](http://wiki.qemu.org/License)
#    * [SimFlex] (http://parsa.epfl.ch/simflex/)
#
# Software developed internally (by the QFlex group)
#**QFlex License**
#
# QFlex
# Copyright (c) 2016, Parallel Systems Architecture Lab, EPFL
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#    notice,
#      this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
#      nor the names of its contributors may be used to endorse or promote
#      products derived from this software without specific prior written
#      permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE
# LABORATORY, EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. DO-NOT-REMOVE end-copyright-block
cmake_minimum_required(VERSION 3.10.2)
project(flexus)

# Setup configuration parameters
include(Configuration.cmake)

#Include simulator specific settings only for "real" simulators
include(${FLEXUS_ROOT}/simulators/${SIMULATOR}/${SIMULATOR}.cmake)

# Setup include paths
include_directories(${FLEXUS_ROOT})
include_directories(SYSTEM ${BOOST_INCLUDEDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FLEXUS_ROOT}/CMakeFiles)

# compile core
file(GLOB_RECURSE CORE_SOURCE ${FLEXUS_ROOT}/core/*.cpp)
file(GLOB_RECURSE QEMU_SOURCE ${FLEXUS_ROOT}/core/qemu/*.cpp)
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/test")
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/qemu")
add_library(core STATIC ${CORE_SOURCE})
add_library(qemu STATIC ${QEMU_SOURCE})
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
target_link_libraries(${SIMULATOR} ${GCC_LDFLAGS} "-Wl,--whole-archive" "-Wl,-export-dynamic" ${${SIMULATOR}_REQUIRED_COMPONENTS})
target_link_libraries(${SIMULATOR} ${GCC_LDFLAGS} "-Wl,--whole-archive" core qemu "-Wl,--no-whole-archive")
target_link_libraries(${SIMULATOR} ${GCC_LDFLAGS} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)

# clean for cmake
add_custom_target(clean_cmake
    COMMAND rm -rf *.o *.a *.so
    COMMAND rm -rf CMakeFiles
    COMMAND rm -rf stat-manager
    COMMAND rm -rf cmake_install.cmake
    COMMAND rm -rf CMakeCache.txt
    COMMAND rm -rf Makefile
    COMMAND echo "All cleared."
)

# Add subdirectory for stat manager
add_subdirectory(stat-manager)