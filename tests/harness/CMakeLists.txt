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

# Include cmake helper macros
include(cmake_helpers.cmake)

# Configure paths and options
include(Configuration.cmake)

# Specify harness root
set(HARNESS_ROOT ${FLEXUS_ROOT}/tests/harness)

# Collect all external component dependencies
set(EXTRA_DEPS "")	# Stores all external dependencies
subdirlist(ALL_TESTS ${HARNESS_ROOT}/tests)
foreach(TEST_DIR ${ALL_TESTS})
	if(EXISTS "${HARNESS_ROOT}/tests/${TEST_DIR}/extra_deps.cmake" )
		include(${HARNESS_ROOT}/tests/${TEST_DIR}/extra_deps.cmake)
	else()
		set(REQUIRED_COMPONENTS "")	# By default extra dependencies are empty
		set(ENABLE_TEST YES)	# By defaut tests are enabled
	endif()
	if(ENABLE_TEST)
		list(APPEND EXTRA_DEPS ${REQUIRED_COMPONENTS})	# Add dependencies to a common list
	endif()
endforeach()
list(REMOVE_DUPLICATES EXTRA_DEPS)	# Remove duplicate dependencies

# Setup include paths
include_directories(${FLEXUS_ROOT})
include_directories(SYSTEM ${BOOST_INCLUDEDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FLEXUS_ROOT}/CMakeFiles)

# include google test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()

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
foreach(COMPONENT ${EXTRA_DEPS})
    file(GLOB_RECURSE COMPONENT_${COMPONENT}_SOURCE "${FLEXUS_ROOT}/components/${COMPONENT}/*.cpp")
    if (${COMPONENT} STREQUAL "NetShim")
        list(FILTER COMPONENT_NetShim_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/components/NetShim/testing")
    endif()
    add_library(${COMPONENT} STATIC ${COMPONENT_${COMPONENT}_SOURCE})
    target_compile_options(${COMPONENT} PRIVATE -c ${GCC_FLAGS})
endforeach()

# compile wiring
add_library(wiring OBJECT ${HARNESS_ROOT}/harness_base/wiring.cpp)
target_compile_options(wiring PRIVATE -c ${GCC_FLAGS})

# compile tests
set(ENABLED_TESTS "")
foreach(TEST_DIR ${ALL_TESTS})
	if(EXISTS "${HARNESS_ROOT}/tests/${TEST_DIR}/extra_deps.cmake" )
		include(${HARNESS_ROOT}/tests/${TEST_DIR}/extra_deps.cmake)
	else()
		set(REQUIRED_COMPONENTS "")	# By default extra dependencies are empty
		set(ENABLE_TEST YES)	# By defaut tests are enabled
	endif()
	if(ENABLE_TEST)
    		file(GLOB_RECURSE TEST_${TEST_DIR}_SOURCE "${HARNESS_ROOT}/tests/${TEST_DIR}/*.cpp")	# Fetch all sources for the test
		add_library(${TEST_DIR} STATIC ${TEST_${TEST_DIR}_SOURCE})	# Create a target library for the test
    		target_compile_options(${TEST_DIR} PRIVATE -c ${GCC_FLAGS})
		target_link_libraries(${TEST_DIR} ${GCC_LDFLAGS} "-Wl,--whole-archive" ${REQUIRED_COMPONENTS} "-Wl,--no-whole-archive")
		target_link_libraries(${TEST_DIR} gtest)
		list(APPEND ENABLED_TESTS ${TEST_DIR})
		message(STATUS "Found test: ${TEST_DIR}")
	endif()
endforeach()

# Compile harness
set(TESTING_HARNESS testing_harness)
add_executable(${TESTING_HARNESS} ${HARNESS_ROOT}/harness_base/all_tests.cpp)
target_link_libraries(${TESTING_HARNESS} "-Wl,--whole-archive" ${ENABLED_TESTS} "-Wl,--no-whole-archive") 
target_link_libraries(${TESTING_HARNESS} gtest_main)
target_link_libraries(${TESTING_HARNESS} $<TARGET_OBJECTS:wiring>)
target_link_libraries(${TESTING_HARNESS} ${GCC_LDFLAGS} "-Wl,--whole-archive" core qemu "-Wl,--no-whole-archive")
target_link_libraries(${TESTING_HARNESS} ${GCC_LDFLAGS} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)

include(GoogleTest)
gtest_discover_tests(${TESTING_HARNESS})

# clean for cmake
add_custom_target(clean_cmake
    COMMAND rm -rf *.o *.a *.so
    COMMAND rm -rf CMakeFiles
    COMMAND rm -rf cmake_install.cmake
    COMMAND rm -rf CMakeCache.txt
    COMMAND rm -rf Makefile
    COMMAND rm -rf ${TESTING_HARNESS}*
    COMMAND rm -rf _deps
    COMMAND rm -rf CTestTestfile.cmake 
    COMMAND rm -rf bin
    COMMAND rm -rf lib
    COMMAND echo "All cleared."
    )
