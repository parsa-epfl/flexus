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

set(FLEXUS_ROOT ${CMAKE_SOURCE_DIR})
# add option -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# add option --std=c++11
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET_PLATFORM)
  message( STATUS "TARGET_PLATFORM was not specified; using default TARGET_PLATFORM=riscv" )
  set(TARGET_PLATFORM riscv)
endif()

if(NOT SELECTED_DEBUG)
  message( STATUS "SELECTED_DEBUG was not specified; using default SELECTED_DEBUG=vverb" )
  set(SELECTED_DEBUG vverb)
endif()

if(NOT SIMULATOR)
  message( STATUS "SIMULATOR was not specified; using default SIMULATOR=TanglyKraken" )
  set(SIMULATOR TanglyKraken)
elseif(SIMULATOR STREQUAL "Harness")
  message( STATUS "SIMULATOR was defined as Harness. Building testing executable" )
endif()

if(NOT BOOST_INCLUDEDIR)
  message( STATUS "BOOST_INCLUDEDIR was not specified; using default BOOST_INCLUDEDIR=/usr/local/include" )
  set(BOOST_INCLUDEDIR /usr/local/include)
endif()

if(NOT CMAKE_C_COMPILER)
  message( STATUS "CMAKE_C_COMPILER was not specified; using default CMAKE_C_COMPILER=/usr/bin/gcc" )
  set(CMAKE_C_COMPILER /usr/bin/gcc)
endif()

if(NOT CMAKE_CXX_COMPILER)
  message( STATUS "CMAKE_CXX_COMPILER was not specified; using default CMAKE_CXX_COMPILER=/usr/bin/g++" )
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif()

if(NOT BOOST_LIBRARYDIR)
  message( STATUS "BOOST_LIBRARYDIR was not specified; using default BOOST_LIBRARYDIR=/usr/local/lib" )
  set(BOOST_LIBRARYDIR /usr/local/lib)
endif()

if(NOT BUILD_DEBUG)
    message( STATUS "Building in default optimized configuration" )
    if(NOT GCC_FLAGS)
        message( STATUS "GCC_FLAGS was not specified; using default" )
        set(GCC_FLAGS -O3 -funroll-loops -fno-omit-frame-pointer -g3 -Wall -Werror -fmessage-length=160 -x c++)
    endif()
else()
    message( STATUS "Building in slow debugging configuration (ASAN enabled)" )
    set(GCC_FLAGS -O0 -g3 -fsanitize=address -Wall -Werror -fmessage-length=160 -x c++) 
    set(GCC_LDFLAGS -fsanitize=address)
endif()

add_definitions(
  -DTARGET_PLATFORM=${TARGET_PLATFORM}
  -DSELECTED_DEBUG=${SELECTED_DEBUG}
  -DSIMULATOR=${SIMULATOR}
  -DBOOST_MPL_LIMIT_VECTOR_SIZE=50
  -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
)
