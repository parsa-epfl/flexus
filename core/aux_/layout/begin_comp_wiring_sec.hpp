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
#ifndef FLEXUS__CORE_TEST

#ifdef FLEXUS__LAYOUT_COMPONENTS_WIRED
#error "Wiring.cpp may contain only one component wiring section"
#endif // FLEXUS__LAYOUT_COMPONENTS_WIRED

#ifndef FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED
#error "The component wiring section of wiring.cpp must follow the component instantiation section"
#endif // FLEXUS__LAYOUT_COMPONENTS_INSTANTIATED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif // FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_COMPONENT_WIRING_SECTION

#endif // FLEXUS__CORE_TEST

#ifdef FLEXUS__CORE_TEST
#define nFLEXUS FLEXUS__CORE_TEST
#else
#define nFLEXUS Flexus
#endif // FLEXUS__CORE_TEST

namespace nFLEXUS {
namespace Wiring {

#define WIRE(FromInstance, FromPort, ToInstance, ToPort)                                                               \
    BOOST_PP_CAT(FromInstance, _instance).theJumpTable.BOOST_PP_CAT(wire_available_, FromPort) =                       \
      &resolve_channel<ToInstance,                                                                                     \
                       ToInstance::iface::ToPort,                                                                      \
                       ToInstance::iface::ToPort::port_type,                                                           \
                       ToInstance::iface::ToPort::is_array>::invoke_available;                                         \
    BOOST_PP_CAT(FromInstance, _instance).theJumpTable.BOOST_PP_CAT(wire_manip_, FromPort) =                           \
      &resolve_channel<ToInstance,                                                                                     \
                       ToInstance::iface::ToPort,                                                                      \
                       ToInstance::iface::ToPort::port_type,                                                           \
                       ToInstance::iface::ToPort::is_array>::invoke_manip; /**/

void
connectWiring()
{
