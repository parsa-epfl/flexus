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
