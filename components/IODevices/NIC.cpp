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

    void printNICRegisters() {
      DBG_(VVerb, (<< "Initializing NIC registers from QEMU..." << std::endl
                    << std::hex

                    // Print Receive Side Registers
                    << "\t" << "RXPBS: "  << RXPBS  << std::endl
                    << "\t" << "RCTL: "   << RCTL   << std::endl
                    << "\t" << "RDBAL: "  << RDBAL  << std::endl
                    << "\t" << "RDBAH: "  << RDBAH  << std::endl
                    << "\t" << "RDLEN: "  << RDLEN  << std::endl
                    << "\t" << "RDH: "    << RDH    << std::endl
                    << "\t" << "RDT: "    << RDT    << std::endl

                    << std::endl

                    // Print Transmit Side Registers
                    << "\t" << "TXPBS: "  << TXPBS  << std::endl
                    << "\t" << "TCTL: "   << TCTL   << std::endl
                    << "\t" << "TDBAL: "  << TDBAL  << std::endl
                    << "\t" << "TDBAH: "  << TDBAH  << std::endl
                    << "\t" << "TDLEN: "  << TDLEN  << std::endl
                    << "\t" << "TDH: "    << TDH    << std::endl
                    << "\t" << "TDT: "    << TDT    << std::endl

                    << std::dec
                    << std::endl));
    }

    void finalize(void) {  }

    void initialize(void)
    {
      Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(0); // Use CPU 0 for now
      
      RXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RXPBS_offset), 4);
      RCTL  = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RCTL_offset), 4);
      RDBAL = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RDBAL_offset), 4);
      RDBAH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RDBAH_offset), 4);
      RDLEN = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RDLEN_offset), 4);
      RDH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RDH_offset), 4);
      RDT = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + RDT_offset), 4);

      TXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TXPBS_offset), 4);
      TCTL  = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TCTL_offset), 4);
      TDBAL = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TDBAL_offset), 4);
      TDBAH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TDBAH_offset), 4);
      TDLEN = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TDLEN_offset), 4);
      TDH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TDH_offset), 4);
      TDT = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BASE_ADDRESS + TDT_offset), 4);

      printNICRegisters();
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
    uint32_t RXPBS;   // RX Packet Buffer Size
    uint32_t RCTL;    // Receive Control Register
    uint32_t RDBAL;   // Receive Descriptor Base Address Low    // TODO: This has aliases
    uint32_t RDBAH;   // Receive Descriptor Base Address High   // TODO: This has aliases
    uint32_t RDLEN;   // Receive Descriptor Ring Length         // TODO: This has aliases
    uint32_t RDH;     // Receive Descriptor Head Register       // TODO: This has aliases
    uint32_t RDT;     // Receive Descriptor Tail Register       // TODO: This has aliases

    uint32_t TXPBS;   // TX Packet Buffer Size
    uint32_t TCTL;    // Transmit Control Register
    uint32_t TDBAL;   // Transmit Descriptor Base Address Low    // TODO: This has aliases
    uint32_t TDBAH;   // Transmit Descriptor Base Address High   // TODO: This has aliases
    uint32_t TDLEN;   // Transmit Descriptor Ring Length         // TODO: This has aliases
    uint32_t TDH;     // Transmit Descriptor Head Register       // TODO: This has aliases
    uint32_t TDT;     // Transmit Descriptor Tail Register       // TODO: This has aliases


  private:
    const uint64_t BASE_ADDRESS = 0x10000000; // TODO: This should not be hardcoded. It should be determined by reading the PCIe BAR register
    
    const uint32_t RXPBS_offset = 0x2404;
    const uint32_t RCTL_offset  = 0x100;
    const uint32_t RDBAL_offset = 0xC000;
    const uint32_t RDBAH_offset = 0xC004;
    const uint32_t RDLEN_offset = 0xC008;
    const uint32_t RDH_offset   = 0xC010;
    const uint32_t RDT_offset   = 0xC018;

    const uint32_t TXPBS_offset = 0x3404;
    const uint32_t TCTL_offset  = 0x400;
    const uint32_t TDBAL_offset = 0xE000;
    const uint32_t TDBAH_offset = 0xE004;
    const uint32_t TDLEN_offset = 0xE008;
    const uint32_t TDH_offset   = 0xE010;
    const uint32_t TDT_offset   = 0xE018;
    

}; // end class NIC

} // end Namespace nNIC

FLEXUS_COMPONENT_INSTANTIATOR(NIC, nNIC);


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NIC

#define DBG_Reset
#include DBG_Control()