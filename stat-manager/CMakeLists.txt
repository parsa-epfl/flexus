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

# Check if top level or invoked from the parent
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Top level, invoked only here
    # TODO
else()
    # Called from parent (e.g., during a flexus build)
    set(REL_PATH ${CMAKE_SOURCE_DIR}/build)
    set(STAT_MANAGER_PATH ${FLEXUS_ROOT}/stat-manager)
endif()

# Get sources from Flexus core
set(CORE_STATS_SRCS
    ${FLEXUS_ROOT}/core/stats/stats.cpp
    ${FLEXUS_ROOT}/core/stats/stats_calc.cpp
    ${FLEXUS_ROOT}/core/stats/measurement.cpp
    )
file(GLOB_RECURSE CORE_DEBUG_SRCS ${FLEXUS_ROOT}/core/debug/*.cpp)

# Add a static library for the core stats srcs and core dbg srcs
add_library(stats_flexcore STATIC ${CORE_STATS_SRCS}
    ${CORE_DEBUG_SRCS})
target_compile_options(stats_flexcore PRIVATE -c ${GCC_FLAGS})
target_link_libraries(stats_flexcore ${GCC_LDFLAGS} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)

# Add 3 executable targets:
# stat manager, stat sample, stat collapse
add_executable(stat-manager ${STAT_MANAGER_PATH}/stat-manager.cpp)
target_link_libraries(stat-manager pthread stats_flexcore)
add_executable(stat-sample ${STAT_MANAGER_PATH}/stat-sample.cpp)
target_link_libraries(stat-sample pthread stats_flexcore)
add_executable(stat-collapse ${STAT_MANAGER_PATH}/stat-collapse.cpp)
target_link_libraries(stat-collapse pthread stats_flexcore)
