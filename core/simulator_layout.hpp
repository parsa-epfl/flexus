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
#ifndef FLEXUS__SIMULATOR_LAYOUT_INCLUDED

#ifdef FLEXUS_WIRING_FILE
// We are being included from wiring.cpp

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>

namespace Flexus {
namespace SharedTypes {
}
} // namespace Flexus

#define FLEXUS_BEGIN_DECLARATION_SECTION() <core/aux_/layout/begin_declaration_section.hpp>
#define FLEXUS_END_DECLARATION_SECTION()   <core/aux_/layout/end_declaration_section.hpp>

#define FLEXUS_BEGIN_GROUP_DEFINITION_SECTION()        <core/aux_/layout/begin_group_definition_section.hpp>
#define FLEXUS_END_GROUP_DEFINITION_SECTION()          <core/aux_/layout/end_group_definition_section.hpp>
#define FLEXUS_BEGIN_COMPONENT_CONFIGURATION_SECTION() <core/aux_/layout/begin_comp_configuration_sec.hpp>
#define FLEXUS_END_COMPONENT_CONFIGURATION_SECTION()   <core/aux_/layout/end_comp_configuration_sec.hpp>
#define FLEXUS_BEGIN_COMPONENT_INSTANTIATION_SECTION() <core/aux_/layout/begin_comp_instantiation_sec.hpp>
#define FLEXUS_END_COMPONENT_INSTANTIATION_SECTION()   <core/aux_/layout/end_comp_instantiation_sec.hpp>
#define FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()        <core/aux_/layout/begin_comp_wiring_sec.hpp>
#define FLEXUS_END_COMPONENT_WIRING_SECTION()          <core/aux_/layout/end_comp_wiring_sec.hpp>
#define FLEXUS_BEGIN_DRIVE_ORDER_SECTION()             <core/aux_/layout/begin_drive_section.hpp>
#define FLEXUS_END_DRIVE_ORDER_SECTION()               <core/aux_/layout/end_drive_section.hpp>

#define FLEXUS_BEGIN_COMPONENT_DECLARATION() <core/aux_/layout/begin_component_declaration.hpp>
#define FLEXUS_END_COMPONENT_DECLARATION()   <core/aux_/layout/end_component_declaration.hpp>

#else //! FLEXUS_WIRING_FILE
// We are being included from a component declaration file within its
// implementation file

#define FLEXUS_IMPLEMENTATION_FILE

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {
}
} // namespace Flexus

#define FLEXUS_BEGIN_COMPONENT_DECLARATION() <core/aux_/layout/begin_component_declaration.hpp>
#define FLEXUS_END_COMPONENT_DECLARATION()   <core/aux_/layout/end_component_declaration.hpp>

#define FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION() <core/aux_/layout/begin_component_implementation.hpp>
#define FLEXUS_END_COMPONENT_IMPLEMENTATION()   <core/aux_/layout/end_component_implementation.hpp>

#endif // FLEXUS_WIRING_FILE

#endif // FLEXUS__SIMULATOR_LAYOUT_INCLUDED
