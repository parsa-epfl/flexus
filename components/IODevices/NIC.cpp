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

// This should not be of interest to us as the context descriptor 
// holds the control information for transmission
// This may also be wrong, so double check
typedef struct AdvancedTransmitContextDescriptor {
    struct Lo { 
        uint64_t reserved : 24;     
        uint64_t ipsec_sa_index : 8;
        uint64_t vlan : 16;         
        uint64_t maclen : 7;        
        uint64_t iplen : 9;         
    } lo;

    struct Hi {
        uint64_t mss : 16;         
        uint64_t l4len : 8;        
        uint64_t rsv1 : 1;         
        uint64_t idx : 3;          
        uint64_t reserved : 6;     
        uint64_t dext : 1;         
        uint64_t rsv2 : 5;         
        uint64_t dtyp : 4;         
        uint64_t tucmd : 12;       
        uint64_t ipsec_esp_len : 9;
    } hi;
} AdvancedTransmitContextDescriptor;  // Advanced Transmit Context Descriptor

typedef struct AdvancedTransmitDataDescriptor {
    uint64_t address;

    struct Fields {
        uint64_t dtalen : 16; 
        uint64_t rsv : 2;
        uint64_t mac : 2;  
        uint64_t dtyp : 4;  
        uint64_t dcmd : 8;    
        uint64_t sta : 4; 
        uint64_t idx : 3; 
        uint64_t cc : 1;   
        uint64_t popts : 6;   
        uint64_t paylen : 18; 
    } fields;
} AdvancedTransmitDataDescriptor; // Advanced Transmit Data Descriptor

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
      DBG_(Crit, (<< "Initializing NIC registers from QEMU..." << std::endl
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

    void initializeBARs () {    // TODO: This should be a member of PCIeDevice class
      BAR0 = Flexus::Qemu::API::qemu_api.get_pcie_config(BDF, 0x10);
      BAR1 = Flexus::Qemu::API::qemu_api.get_pcie_config(BDF, 0x14);
      BAR2 = Flexus::Qemu::API::qemu_api.get_pcie_config(BDF, 0x18);
      BAR3 = Flexus::Qemu::API::qemu_api.get_pcie_config(BDF, 0x1C);
    }

    void initialize(void)
    {

      cpu = Flexus::Qemu::Processor::getProcessor(0); // Use CPU 0 for now

      initializeBARs();
      
      RXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RXPBS_offset), 4);
      RCTL  = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RCTL_offset), 4);
      RDBAL = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDBAL_offset), 4);
      RDBAH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDBAH_offset), 4);
      RDLEN = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDLEN_offset), 4);
      RDH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDH_offset), 4);
      RDT = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDT_offset), 4);

      TXPBS = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TXPBS_offset), 4);
      TCTL  = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TCTL_offset), 4);
      TDBAL = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDBAL_offset), 4);
      TDBAH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDBAH_offset), 4);
      TDLEN = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDLEN_offset), 4);
      TDH = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDH_offset), 4);
      TDT = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDT_offset), 4);

      receiveQueue.resize (RDLEN / sizeof (AdvancedReceiveDescriptor));
      
      receiveHeadCurrent = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDH_offset), 4);
      receiveTailCurrent = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDT_offset), 4);
      receiveHeadPrevious = receiveHeadCurrent;
      receiveTailPrevious = receiveTailCurrent;

      printNICRegisters();
    }

    // Single ports do not need index
    // DYNAMIC_PORT_ARRAY need index in this function signature as well
    // This push function is called every time a message is pushed into the PushInput port
    // There is no complementary function for PushOutput port
    // For PushOutput, the driver simply pushes an output into the port and the 
    // component connected to the PushOutput then receives the message at its 
    // PushInput port and handles the message in its push function
    FLEXUS_PORT_ALWAYS_AVAILABLE(CPUMemoryRequest);
    void push(interface::CPUMemoryRequest const&, MemoryMessage& aMessage)
    {
      DBG_(VVerb, (<< "NIC Memory Message: Address: " << std::hex << (uint64_t)aMessage.address() << std::dec));

      if ((uint64_t)aMessage.address() >= BAR0 && (uint64_t)aMessage.address() < (BAR0 + BAR0_size)) {
        if ((uint64_t)aMessage.address() == (BAR0 + TDT_offset) ) {
          processTxTailUpdate();
        }
        else if ((uint64_t)aMessage.address() == (BAR0 + RDT_offset) ) {
          DBG_(Crit, ( << "Rx Tail Access: Write(" << (aMessage.type() == MemoryMessage::IOStoreReq) << ")" ));
          processRxTailUpdate();
        } else if ((uint64_t)aMessage.address() == (BAR0 + RDH_offset) ) {
          DBG_(Crit, ( << "Rx Head Access: Write(" << (aMessage.type() == MemoryMessage::IOStoreReq) << ")\t Head Prev: 0x" << std::hex << receiveHeadPrevious << "\tHead Curr: 0x" << receiveHeadCurrent << std::dec ));
          processRxHeadUpdate(*((uint32_t *)aMessage.getDataPtr()));
        }
      }
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(DeviceMemoryResponse);
    void push(interface::DeviceMemoryResponse const&, MemoryMessage& aMessage)
    {
      // TODO
    }

    void drive(interface::UpdateNICState const&) {}

  private:

    // TODO: Overload this function for Receive Descriptor
    // This function takes a descriptor as input and chops it up into smaller
    // TLP sized messages to be sent to the SMMU
    std::vector <MemoryMessage> generateMemoryMessageFromDescriptor (AdvancedTransmitDataDescriptor transmitDescriptor) {

      std::vector <MemoryMessage> ret;

      uint32_t bytesLeft = transmitDescriptor.fields.dtalen;  //! Use dtalen instead of paylen. When TSO is enabled, dtalen holds the length of data to be transmitted
                                                              // !!!! I need confirmation for this!!!! I am not sure if I have implemented correct functionality for TSO
      uint32_t numberOfMessages = ceil(bytesLeft / (maxTLPSize * 1.0));

      for (uint32_t i = 0; i < numberOfMessages; i++) {

        VirtualMemoryAddress txBufferIOVA = VirtualMemoryAddress( transmitDescriptor.address + i * maxTLPSize );

        uint32_t payloadLength = bytesLeft > maxTLPSize ? maxTLPSize : bytesLeft;

        MemoryMessage txDataReadMessage (MemoryMessage::IOLoadReq, PhysicalMemoryAddress(0), txBufferIOVA, BDF);
        txDataReadMessage.reqSize() = payloadLength;
        // Data is not actually required so dataRequired field is not set here

        ret.push_back(txDataReadMessage);

        bytesLeft -= payloadLength;
      }

      return ret;
    }

    void processTxDescriptor (AdvancedTransmitDataDescriptor transmitDescriptor) {

      /**
       * Read the IOVA of Tx Buffer from the buffer descriptor
       * Create memory message*s* with the appropriate IOVA and message size
       * Send the memory message to the SMMU
       * SMMU will then process that message, do translation and read and
       * return data if necessary
       * SMMU will be responsible for injecting memory accesses into the cache
       * hierarchy
       * So the NIC only sends MemoryMessages and not inject memory operations
       * directly into the memory or LLC
       */

      std::vector <MemoryMessage> txDataReadMessages = generateMemoryMessageFromDescriptor (transmitDescriptor);

      for (auto& txDataReadMessage : txDataReadMessages)
        FLEXUS_CHANNEL(DeviceMemoryRequest) << txDataReadMessage;  // Sending to SMMU for processing
                                                                   // Processing entails both translation and memory access

      // By this point, the SMMU should have injected read requests into the cache hierarchy

      printDescriptor(transmitDescriptor);
    }

    void processRxDescriptor (AdvancedReceiveDescriptor receiveDescriptor) {

      // /**
      //  * Read the IOVA of Rx data and header Buffer from the buffer descriptor
      //  * Create memory message*s* with the appropriate IOVA and message size
      //  * Send the memory message to the SMMU
      //  * SMMU will then process that message, do translation and read and
      //  * return data if necessary
      //  * SMMU will be responsible for injecting memory accesses into the cache
      //  * hierarchy
      //  * So the NIC only sends MemoryMessages and not inject memory operations
      //  * directly into the memory or LLC
      //  */

      // std::vector <MemoryMessage> rxReadReadMessages = generateMemoryMessageFromDescriptor (transmitDescriptor);

      // for (auto& txDataReadMessage : txDataReadMessages)
      //   FLEXUS_CHANNEL(DeviceMemoryRequest) << txDataReadMessage;  // Sending to SMMU for processing
      //                                                              // Processing entails both translation and memory access

      // // By this point, the SMMU should have injected read requests into the cache hierarchy

      printDescriptor(receiveDescriptor);
    }

    // Use pointer arithmetic to conveniently read descriptors that are bigger than 64 bits
    AdvancedTransmitDataDescriptor readTransmitDataDescriptor (VirtualMemoryAddress tailDescriptorIOVA) {

      AdvancedTransmitDataDescriptor transmitDescriptor;

      // !!!!!! Assuming that this message is always < maxTLP
      // !!!!!! Otherwise this message also needs to be chopped
      MemoryMessage txDataDescriptorReadMessage (MemoryMessage::IOLoadReq, PhysicalMemoryAddress(0), tailDescriptorIOVA, BDF);
      txDataDescriptorReadMessage.reqSize() = sizeof (transmitDescriptor);   // Descriptor is 16-bytes long
      txDataDescriptorReadMessage.setDataRequired();  // NIC requires the descriptor data

      FLEXUS_CHANNEL(DeviceMemoryRequest) << txDataDescriptorReadMessage;  // Sending to SMMU for processing
                                                                 // Processing entails both translation and memory access

      transmitDescriptor = *((AdvancedTransmitDataDescriptor *) txDataDescriptorReadMessage.getDataPtr());  // Copy data from memory message to transmitDescriptor

      free((void *)txDataDescriptorReadMessage.getDataPtr()); // Free the memory allocated for holding the data

      return transmitDescriptor;
    }

    // Use pointer arithmetic to conveniently read descriptors that are bigger than 64 bits
    AdvancedReceiveDescriptor readReceiveDescriptor (VirtualMemoryAddress descriptorIOVA) {

      AdvancedReceiveDescriptor receiveDescriptor;

      // !!!!!! Assuming that this message is always < maxTLP
      // !!!!!! Otherwise this message also needs to be chopped
      MemoryMessage rxDescriptorReadMessage (MemoryMessage::IOLoadReq, PhysicalMemoryAddress(0), descriptorIOVA, BDF);
      rxDescriptorReadMessage.reqSize() = sizeof (receiveDescriptor.advancedReceiveReadDescriptor);   // Descriptor is 16-bytes long
      rxDescriptorReadMessage.setDataRequired();  // NIC requires the descriptor data

      FLEXUS_CHANNEL(DeviceMemoryRequest) << rxDescriptorReadMessage;  // Sending to SMMU for processing
                                                                       // Processing entails both translation and memory access

      receiveDescriptor.advancedReceiveReadDescriptor = *((AdvancedReceiveDescriptor::AdvancedReceiveReadDescriptor *) rxDescriptorReadMessage.getDataPtr());  // Copy data from memory message to receiveDescriptor

      free((void *)rxDescriptorReadMessage.getDataPtr()); // Free the memory allocated for holding the data

      return receiveDescriptor;
    }

    /**
     * Handle update to TDT
     * TDT update means that the Driver wrote
     * a new TX descriptor to be processed
     * processTxTailUpdate handles the update 
     * by getting the PA of the buffer descriptor
     * then processing the descriptor
     */
    void processTxTailUpdate () {

      // Current position of the tail in the TX Ring Buffer
      // This points to the next empty slot in ring buffer, meaning 
      // the slot before this one has the descriptor address written
      // by the OS
      uint64_t tailPosition = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + TDT_offset), 4);
      tailPosition--;

      VirtualMemoryAddress tailDescriptorIOVA ( TDBAL + (tailPosition << 4) );

      // This step requires one translation
      AdvancedTransmitDataDescriptor transmitDescriptor = readTransmitDataDescriptor(tailDescriptorIOVA);

      // This will require as many translations as the data is big
      if (transmitDescriptor.fields.dtyp == 0x3) { // 0x3 means that this is a data descriptor
        processTxDescriptor(transmitDescriptor);
      }
    }

    /**
     * Handle update to RDT
     * RDT update means that the Driver wrote
     * a new RX descriptor to be processed
     * processRxTailUpdate handles the update 
     * by getting the PA of the buffer descriptor
     * then processing the descriptor
     */
    void processRxTailUpdate () {

      // Current position of the tail in the RX Ring Buffer
      // Tail update implies that the SW wrote a new empty descriptor
      // at tail-1 and updated the pointer to tail
      // So we should obtain the Tail Descriptor and store it in
      // the internal receive descriptor queue (receiveQueue)

      // At the same time, we should read the Head Pointer too in
      // order to know the progress of the hardware

      receiveTailCurrent = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(BAR0 + RDT_offset), 4);

      // TODO: Wrap around logic
      // Everything between PreviousTail and Current Tail has been added by the Software
      // for the hardware to process
      for (uint32_t i = receiveTailPrevious; i < receiveTailCurrent; i++) {
        VirtualMemoryAddress tailDescriptorIOVA ( RDBAL + (i << 4) );

      // This step requires one translation
      AdvancedReceiveDescriptor receiveDescriptor = readReceiveDescriptor(tailDescriptorIOVA);
        receiveDescriptor.advancedReceiveReadDescriptor.DD = 0; // For some reason QEMU gives wrong DD so we fix the DD here
                                                                // Tail update implies that new descriptors have been added to
                                                                // to the ring. Thus, these descriptors are unprocessed

        receiveQueue[i] = receiveDescriptor;

        // This will require as many translations as the data is big
        processRxDescriptor(receiveDescriptor);
      }

      // Everything between PreviousHead and CurrentHead was processed by the hardware 
      // and can be taken by the software

      receiveTailPrevious = receiveTailCurrent; // Update the Previous pointer to the current pointer
                                                // Current pointer gets updated st the next invocation of this function
    }

    /**
     * Handle update to RDH
     * Update to RDH implies that 
     * NIC processed reception of data
     * and has written the ReceiveWriteBack
     * Descriptor to the Rx Ring. This WriteBack
     * descriptor can be used to determine how much
     * data was written
     */
    void processRxHeadUpdate (uint32_t headPosition) {

      receiveHeadCurrent = headPosition;

      // TODO: Wrap around logic
      // Everything between PreviousTail and Current Tail has been added by the Software
      // for the hardware to process
      for (uint32_t i = receiveHeadPrevious; i < receiveHeadCurrent; i++) {
        VirtualMemoryAddress headDescriptorIOVA ( RDBAL + (i << 4) );

        // This step requires one translation
        AdvancedReceiveDescriptor receiveDescriptor = readReceiveDescriptor(headDescriptorIOVA);
        receiveDescriptor.advancedReceiveReadDescriptor.DD = 1; // For some reason QEMU gives wrong DD so we fix the DD here
                                                                // Head update implies that descriptors have been processed
                                                                // by hardware

        receiveQueue[i] = receiveDescriptor;

      // This will require as many translations as the data is big
      processRxDescriptor(receiveDescriptor);
      }

      // Everything between PreviousHead and CurrentHead was processed by the hardware 
      // and can be taken by the software

      receiveHeadPrevious = receiveHeadCurrent;
    }

  private:  // These are helpers for debugging purposes and can be safely removed in release
    void printDescriptor(const AdvancedTransmitDataDescriptor& descriptor) {
      DBG_(VVerb, ( << "Advanced Transmit Data Descriptor Contents:"
      
      // Print the 64-bit address field in hex
      << "\n  Buffer IOVA:      0x" << std::hex << std::setw(16) << std::setfill('0') 
      << descriptor.address 
      
      // Print fields in the 'Fields' struct
      << "\n  PAYLEN:           0x" << std::hex << std::setw(5) << std::setfill('0') 
      << descriptor.fields.paylen 
      << "\n  POPTS:            0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.popts 
      << "\n  CC:               0x" << std::hex << std::setw(2) << std::setfill('0') 
      << descriptor.fields.cc 
      << "\n  IDX:              0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.idx 
      << "\n  STA:              0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.sta 
      << "\n  DCMD:             0x" << std::hex << std::setw(2) << std::setfill('0') 
      << descriptor.fields.dcmd 
      << "\n  DTYP:             0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.dtyp 
      << "\n  MAC:              0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.mac 
      << "\n  RSV:              0x" << std::hex << std::setw(1) << std::setfill('0') 
      << descriptor.fields.rsv 
      << "\n  DTA_LEN:          0x" << std::hex << std::setw(5) << std::setfill('0') 
      << descriptor.fields.dtalen ));
    }

    void printDescriptor(const AdvancedReceiveDescriptor& descriptor) {

      bool isProcessed = descriptor.advancedReceiveWriteDescriptor.DD == 1;

      if (!isProcessed) {
        DBG_(Crit, ( << "Advanced Receive Read Descriptor Contents: HeadPrev: " << receiveHeadPrevious << "\tHeadCurr: " << receiveHeadCurrent << "\tTailPrev: " << receiveTailPrevious << "\tTailCurr: " << receiveTailCurrent
        
        // Print the 64-bit address field in hex
        << "\n  Packet Buffer IOVA: " << std::hex << std::setw(16) << std::setfill('0') 
        << descriptor.advancedReceiveReadDescriptor.packetBufferAddress
        
        << "\n  Header Buffer IOVA: " << std::hex << std::setw(16) << std::setfill('0') 
        << (descriptor.advancedReceiveReadDescriptor.headerBufferAddress << 1)
        << "\n  Descriptor Done: " << (uint64_t) descriptor.advancedReceiveWriteDescriptor.DD
        ));
      } else {
        DBG_(Crit, ( << "Advanced Receive Writeback Descriptor Contents: HeadPrev: " << receiveHeadPrevious << "\tHeadCurr: " << receiveHeadCurrent << "\tTailPrev: " << receiveTailPrevious << "\tTailCurr: " << receiveTailCurrent
        
        << "\n  Packet Length: " << std::hex << (uint64_t)descriptor.advancedReceiveWriteDescriptor.PKT_LEN 
        << "\n  Header Length: " << std::hex << descriptor.advancedReceiveWriteDescriptor.HDR_LEN
        << "\n  End Of Packet: " << (uint64_t)descriptor.advancedReceiveWriteDescriptor.EOP ));
      }
    }

  private:
    Flexus::Qemu::Processor cpu;  // TODO: Technically NIC should not be tied to a CPU but this exposes convenient functions that we want to use

  private:

    // These are MMIO registers
    uint32_t BAR0;    // BAR of NIC                             // TODO: Every IO device connected to PCIe has
    uint32_t BAR1;                                              // various BARs. So This field should preferably
    uint32_t BAR2;                                              // be in a Base class PCIeDevice which gets inherited
    uint32_t BAR3;                                              // by the various kinds of PCIe devices

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
    const uint16_t maxTLPSize   = 4 * 1024;   // As per PCIe 6 documentation, the maximum size of a TLP can be 4KB
    const uint16_t BDF          = 0x8;        // TODO: This BDF should be dynamically determined and should also be a member of PCIeDevice class
    const uint32_t BAR0_size    = 128 * 1024; // TODO: BAR Sizes are device specific and specified in the Device documentation, so they need to be
                                              // initialized by the device constructor
    const uint32_t BAR1_size    = 64 * 1024;
    const uint32_t BAR2_size    = 32;
    const uint32_t BAR3_size    = 16 * 1024;

    const uint32_t RXPBS_offset = 0x2404;
    const uint32_t RCTL_offset  = 0x100;
    const uint32_t RDBAL_offset = 0xC000;
    const uint32_t RDBAH_offset = 0xC004;
    const uint32_t RDLEN_offset = 0xC008;
    const uint32_t RDH_offset   = 0x2810;
    const uint32_t RDT_offset   = 0x2818;

    const uint32_t TXPBS_offset = 0x3404;
    const uint32_t TCTL_offset  = 0x400;
    const uint32_t TDBAL_offset = 0xE000;
    const uint32_t TDBAH_offset = 0xE004;
    const uint32_t TDLEN_offset = 0xE008;
    const uint32_t TDH_offset   = 0xE010;
    const uint32_t TDT_offset   = 0x3818;

  private:
    // These are different from the MMIO registers and 
    // are maintained inside the NIC for its internal processing
    uint32_t receiveHeadCurrent;
    uint32_t receiveHeadPrevious;
    uint32_t receiveTailCurrent;
    uint32_t receiveTailPrevious;
    std::vector <AdvancedReceiveDescriptor> receiveQueue;
    

}; // end class NIC

} // end Namespace nNIC

FLEXUS_COMPONENT_INSTANTIATOR(NIC, nNIC);


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NIC

#define DBG_Reset
#include DBG_Control()