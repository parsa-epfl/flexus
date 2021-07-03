#!/bin/bash

#  DO-NOT-REMOVE begin-copyright-block
# QFlex consists of several software components that are governed by various
# licensing terms, in addition to software that was developed internally.
# Anyone interested in using QFlex needs to fully understand and abide by the
# licenses governing all the software components.
#
# ### Software developed externally (not by the QFlex group)
#
#     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
#     * [QEMU] (http://wiki.qemu.org/License)
#     * [SimFlex] (http://parsa.epfl.ch/simflex/)
#     * [GNU PTH] (https://www.gnu.org/software/pth/)
#
# ### Software developed internally (by the QFlex group)
# **QFlex License**
#
# QFlex
# Copyright (c) 2021, Parallel Systems Architecture Lab, EPFL
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
#       nor the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
# EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  DO-NOT-REMOVE end-copyright-block

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

set -x
set -e

JOBS=$(($(getconf _NPROCESSORS_ONLN) + 1))
echo "=== Using ${JOBS} simultaneous jobs ==="

# Install a compatible version of gcc
GCC_VERSION="8"
sudo apt-get update -qq
sudo apt-get autoremove libboost-all-dev
sudo apt-get -y install gcc-${GCC_VERSION} g++-${GCC_VERSION}

# Set the recently installed version of gcc as default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 20
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 20
sudo update-alternatives --config gcc
sudo update-alternatives --config g++

# Install a compatible version of boost library
BOOST="boost_1_70_0"
BOOST_VERSION="1.70.0"
BOOST_SHA="882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9"
wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST}.tar.gz -O /tmp/${BOOST}.tar.gz
echo "${BOOST_SHA} /tmp/${BOOST}.tar.gz | sha256sum --check --status"
if [ $? -ne 0 ]; then
  echo "SHA256 for boost tarball failed!"
  exit 1
fi
tar -xf /tmp/${BOOST}.tar.gz
cd ./${BOOST}/
./bootstrap.sh --prefix=/usr/local
./b2 -j${JOBS} --with-system --with-regex --with-serialization --with-iostreams
sudo ./b2 --with-system --with-regex --with-serialization --with-iostreams install
