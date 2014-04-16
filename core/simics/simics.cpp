#include <core/target.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/event_api.hpp>

namespace Flexus {
namespace Simics {

const char * HAPs::Core_Initial_Configuration::hap_name = "Core_Initial_Configuration";
const char * HAPs::Core_Asynchronous_Trap::hap_name = "Core_Asynchronous_Trap";
const char * HAPs::Core_Exception_Return::hap_name = "Core_Exception_Return";
const char * HAPs::Core_Continuation::hap_name = "Core_Continuation";
const char * HAPs::Core_Simulation_Stopped::hap_name = "Core_Simulation_Stopped";
const char * HAPs::Core_Magic_Instruction::hap_name = "Core_Magic_Instruction";
const char * HAPs::Core_Periodic_Event::hap_name = "Core_Periodic_Event";
const char * HAPs::Ethernet_Network_Frame::hap_name = "Ethernet_Network_Frame";
const char * HAPs::Ethernet_Frame::hap_name = "Ethernet_Frame";
const char * HAPs::Xterm_Break_String::hap_name = "Xterm_Break_String";
const char * HAPs::Gfx_Break_String::hap_name = "Gfx_Break_String";

InitialConfigHapHandler * InitialConfigHapHandler::theStaticInitialConfigHapHandler = 0;

namespace Detail {
EventImpl::timing_seconds_tag EventImpl::timing_seconds;
EventImpl::timing_cycles_tag EventImpl::timing_cycles;
EventImpl::timing_steps_tag EventImpl::timing_steps;
EventImpl::timing_stacked_tag EventImpl::timing_stacked;
}

}  //End Namespace Simics
} //namespace Flexus

