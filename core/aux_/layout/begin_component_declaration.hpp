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
#ifndef FLEXUS_IMPLEMENTATION_FILE

#ifndef FLEXUS__LAYOUT_DECLARATION_SECTION
#error "Component declaration files must be included in the component declaration section of wiring.cpp"
#endif // FLEXUS_LAYOUT_COMPONENT_DECLARATION_SECTION

#ifndef FLEXUS_BEGIN_COMPONENT
#error "This component never declared the FLEXUS_BEGIN_COMPONENT macro"
#endif // FLEXUS_COMPONENT

#ifndef FLEXUS_END_COMPONENT
#error "Previous component forget to set the FLEXUS_END_COMPONENT macro"
#endif // FLEXUS_END_COMPONENT

#endif // FLEXUS_IMPLEMENTATION_FILE

#define FLEXUS__LAYOUT_IN_COMPONENT_DECLARATION

using namespace Flexus::SharedTypes;

#define COMPONENT_NO_PARAMETERS FLEXUS_DECLARE_COMPONENT_NO_PARAMETERS(FLEXUS_BEGIN_COMPONENT)
#define COMPONENT_PARAMETERS(x) FLEXUS_DECLARE_COMPONENT_PARAMETERS(FLEXUS_BEGIN_COMPONENT, x)
#define PARAMETER               FLEXUS_PARAMETER

#define COMPONENT_EMPTY_INTERFACE   FLEXUS_COMPONENT_EMPTY_INTERFACE(FLEXUS_BEGIN_COMPONENT)
#define COMPONENT_INTERFACE(x)      FLEXUS_COMPONENT_INTERFACE(FLEXUS_BEGIN_COMPONENT, x)
#define PORT(x, y, z)               FLEXUS_IFACE_PORT(x, y, z)
#define PORT_ARRAY(x, y, z, w)      FLEXUS_IFACE_PORT_ARRAY(x, y, z, w)
#define DYNAMIC_PORT_ARRAY(x, y, z) FLEXUS_IFACE_DYNAMIC_PORT_ARRAY(x, y, z)
#define DRIVE(x)                    FLEXUS_IFACE_DRIVE(x)
