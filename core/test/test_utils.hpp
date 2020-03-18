//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED
#define FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/test/unit_test.hpp>

#include <core/debug/debug.hpp>

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/metaprogram.hpp>
#include <core/ports.hpp>
#include <core/simulator_layout.hpp>
#include <core/wiring.hpp>

using namespace boost::unit_test_framework;
using namespace Flexus::Core;

// V2 versions of macros
#define FLEXUS_TEST_IO_LIST(COUNT, ...) mpl::vector<__VA_ARGS__>
#define FLEXUS_TEST_IO_LIST_EMPTY mpl::vector<>
#define FLEXUS_TEST_WIRING template <class Handle, class Wiring>
#define FLEXUS_TEST_CHANNEL(COMPONENT, PORT)                                                       \
  Wiring::channel(static_cast<Handle *>(0), static_cast<PORT *>(0), COMPONENT.flexusIndex())
#define FLEXUS_TEST_CHANNEL_ARRAY(COMPONENT, PORT, IDX)                                            \
  Wiring::channel_array(static_cast<Handle *>(0), static_cast<PORT *>(0),                          \
                        (COMPONENT).flexusIndex(), IDX)
#define FLEXUS_TEST_DRIVE_LIST(COUNT, ...) mpl::vector<__VA_ARGS__>
#define FLEXUS_TEST_DRIVE_LIST_EMPTY mpl::vector<>

#endif // FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED
