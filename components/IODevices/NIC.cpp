#include "core/debug/debug.hpp"

#include <components/IODevices/NIC.hpp>
#include <cstdint>
#include <fstream>
#include <string>

#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT NIC
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nNIC {

using namespace Flexus;

class FLEXUS_COMPONENT(NIC)
{
    FLEXUS_COMPONENT_IMPL(NIC);

    int32_t theIndex;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(NIC)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    void finalize(void) {  }

    void initialize(void)
    {
      Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(0); // Use CPU 0 for now
      
      RXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RXPBS_offset), 8);
      TXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TXPBS_offset), 8);

      DBG_(VVerb,
             (<< "NIC Initialization: RXPBS(" << RXPBS << ")\tTXPBS(" << TXPBS << ")" << std::dec));
    }

    // Single ports do not need index
    // DYNAMIC_PORT_ARRAY need index in this function signature as well
    // This push function is called every time a message is pushed into the PushInput port
    // There is no complementary function for PushOutput port
    // For PushOutput, the driver simply pushes an output into the port and the 
    // component connected to the PushOutput then receives the message at its 
    // PushInput port and handles the message in its push function
    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryRequest);
    void push(interface::MemoryRequest const&, MemoryMessage& aMessage)
    {
    }

    void drive(interface::UpdateNICState const&) {}


  private:
    uint32_t RXPBS;
    uint32_t TXPBS;

  private:
    const uint64_t BASE_ADDRESS = 0x10000000; // TODO: This should not be hardcoded. It should be determined by reading the PCIe BAR register
    const uint32_t RXPBS_offset = 0x2404;
    const uint32_t TXPBS_offset = 0x3404;

}; // end class NIC

} // end Namespace nNIC

FLEXUS_COMPONENT_INSTANTIATOR(NIC, nNIC);


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NIC

#define DBG_Reset
#include DBG_Control()