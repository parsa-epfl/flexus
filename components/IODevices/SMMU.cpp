#include "components/CommonQEMU/Slices/MemoryMessage.hpp"
#include "components/IODevices/SMMUTypes.hpp"
#include "core/debug/debug.hpp"
#include "core/types.hpp"
#include <cstdint>
#include <vector>
#include "SMMU.hpp"

#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT NIC
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nSMMU {

using namespace Flexus;
using namespace Qemu;
using namespace Core;
using namespace SharedTypes;
using namespace nMMU;
uint64_t PAGEMASK;

// These two defines have been taken from QEMU: smmu-internal.h
#define TBI0(tbi) ((tbi) & 0x1)
#define TBI1(tbi) ((tbi) & 0x2 >> 1)

class FLEXUS_COMPONENT(SMMU)
{
	FLEXUS_COMPONENT_IMPL(SMMU);
	
private:
	Flexus::Qemu::Processor cpu;  // TODO: Technically SMMU should not be tied to a CPU but this exposes convenient functions that we want to use

private:
	const uint64_t CacheMessageSize		=	8;				  // SMMU sends cache messages of size 8 Bytes to the LLC
    const uint32_t configBaseAddress	=	0x09050000;       // TODO: Should be determined from QEMU. Base address of SMMU config space
    const uint32_t configSize			=	0x00020000;       // Size of SMMU config space

private:
    StreamTableBaseConfig streamTableBaseConfig;
    uint64_t streamTableSize;
    uint64_t streamTableBase;
	SMMUInterfaceConfig smmuInterfaceConfig;				// SMMU programming interface control and configuration register

	uint64_t commandQueueBase;
	uint32_t commandQueueLog2Size;
	uint32_t commandQueueProd;
	uint32_t commandQueueCons;

private:
    const uint32_t SMMU_STRTAB_BASE_CFG     =   0x0088;       // Stream Table Base Configuration register offset
    const uint32_t SMMU_STRTAB_BASE         =   0x0080;       // Stream Table Base register offset
	const uint32_t SMMU_IDR0				=	0x0000;
	const uint32_t SMMU_CR0					=	0x0020;		  // SMMU programming interface control and configuration register
	const uint32_t SMMU_CMDQ_BASE			=	0x0090;
	const uint32_t SMMU_CMDQ_PROD			=	0x0098;
	const uint32_t SMMU_CMDQ_CONS			=	0x009c;

// These are from core MMU of QFlex
private:
	bit_configs_t aarch64_bit_configs;
	std::unique_ptr<PageWalk> thePageWalker;
	std::shared_ptr<mmu_t> theMMU;

private:
	 std::shared_ptr<IOTLB> theIOTLB;

public:
	FLEXUS_COMPONENT_CONSTRUCTOR(SMMU)
	  : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
	{
	}

	// Initialization
	void initialize()
	{
		cpu = Flexus::Qemu::Processor::getProcessor(0); // Use CPU 0 for now

		*((uint32_t *)(&smmuInterfaceConfig)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CR0), 4);

		DBG_Assert(smmuInterfaceConfig.SMMUEN == 1, (<< "QEMU does not have an SMMU"));
		DBG_Assert(smmuInterfaceConfig.CMDQEN == 1, (<< "QEMU does not have an SMMU Command Queue Enabled"));
		DBG_Assert(smmuInterfaceConfig.EVENTQEN == 1, (<< "QEMU does not have an SMMU Event Queue Enabled"));

		*((uint64_t *)(&commandQueueBase)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CMDQ_BASE), 8);
		commandQueueLog2Size = commandQueueBase & ((1UL << 5) - 1);
		commandQueueBase >>= 5;
		commandQueueBase &= ((1UL << 51) - 1);
		commandQueueBase <<= 5;

		*((uint32_t *)(&commandQueueProd)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CMDQ_PROD), 4);
		*((uint32_t *)(&commandQueueCons)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CMDQ_CONS), 4);

		*((uint32_t *)(&streamTableBaseConfig)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_STRTAB_BASE_CFG), 4);
        streamTableBase         = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_STRTAB_BASE), 8);
        streamTableBase &= 0xffffffffffffff;
        streamTableBase = ((streamTableBase >> 6) << 6);

        DBG_Assert(streamTableBaseConfig.fmt == 1, (<< "Invalid Stream Table Format. Only 2 level tables are supported"));

        streamTableSize = 1ULL << streamTableBaseConfig.log2size;

		printSMMUConfig();

		thePageWalker.reset(new PageWalk(flexusIndex()));
		thePageWalker->setMMU(theMMU);

		theIOTLB.reset (new IOTLB (12, 1024, 8));	// TODO: Use Config file to get the config for IOTLB
	}

	PhysicalMemoryAddress translateIOVA (TranslationPtr& aTranslate, MemoryMessage& aMemoryMessage) {

		uint16_t bdf = aTranslate->bdf;

		DBG_(VVerb, (<< "Translating: BDF: " << std::hex << bdf << "\tIOVA: " << (uint64_t) aTranslate->theVaddr << std::dec ));

		// Configuration lookup happens for every IOTLB access as well
		// TODO: Add caching for configuration

		StreamTableEntry streamTableEntry = getStreamTableEntry (bdf);
		ContextDescriptor contextDescriptor = getContextDescriptor (streamTableEntry);
		uint64_t translationTableBase = getTranslationTableBase(aTranslate->theVaddr, contextDescriptor);

		uint16_t ASID = contextDescriptor.ASID;

		if (theIOTLB->contains(ASID, aTranslate->theVaddr)) {	// Hit in IOTLB
			aTranslate->thePaddr = theIOTLB->access(ASID, aTranslate->theVaddr);

			DBG_(VVerb, (	
						<< "Translation Hit in IOTLB: BDF: " << std::hex << bdf 
						<< "\tIOVA: " << (uint64_t) aTranslate->theVaddr 
						<< "\tPA: " << (uint64_t)  aTranslate->thePaddr
						<< std::dec
						));

			return aTranslate->thePaddr;
		}

		// Miss in IOTLB, do Page Table Walk and insert the entry into the IOTLB
		cfg_smmu(0, contextDescriptor, aTranslate->theVaddr);	// TODO: Passing 0 for now as we have only 1 device

		thePageWalker->setMMU(theMMU);

		thePageWalker->push_back_trace(aTranslate, Flexus::Qemu::Processor::getProcessor(flexusIndex()));

		// Injecting PTW memory accesses into the cache hierarchy
		MemoryMessage mmu_message(aMemoryMessage);
        mmu_message.setPageWalk();

		while (aTranslate->trace_addresses.size()) {
            mmu_message.type()    = MemoryMessage::ReadReq;		// As this request doesn't go to L1 cache, the memory type must be ReadReq instead of LoadReq so that coherence protocol doesn't mess up
																// We use fromSMMU field in the memory message to tell the directory that the address need not be added to the sharers list. 
																// Therefore every message coming from the SMMU **MUST** have fromSMMU field set
            mmu_message.address() = aTranslate->trace_addresses.front();
			mmu_message.setFromSMMU();
            aTranslate->trace_addresses.pop();

			FLEXUS_CHANNEL(MemoryRequest) << mmu_message;
		}

		// Insert the translation into the IOTLB

		theIOTLB->update(ASID, aTranslate->theVaddr, aTranslate->thePaddr);

		DBG_(VVerb, (	<< "Translation Miss in IOTLB: BDF: " << std::hex << bdf 
						<< "\tIOVA: " << (uint64_t) aTranslate->theVaddr 
						<< "\tPA: " << (uint64_t)  aTranslate->thePaddr
						<< std::dec));

		return aTranslate->thePaddr;
    }

	void finalize() {}

	// SMMUDrive
	//----------
	void drive(interface::SMMUDrive const&)
	{
	}

	FLEXUS_PORT_ALWAYS_AVAILABLE(CPUMemoryRequest);
	void push(interface::CPUMemoryRequest const&, MemoryMessage& aMessage)
	{
		if ((uint64_t)aMessage.address() == (configBaseAddress + SMMU_CMDQ_PROD)) {	// CPU produced a command
			processCommandQueue ();
		}
	}

	// !This needs to be implemented
	bool available(interface::TranslationRequestIn const&) { return true; }
	void push(interface::TranslationRequestIn const&, TranslationPtr& aTranslate)
	{
		// PhysicalMemoryAddress qflexPA = translateIOVA (aTranslate,);
		// PhysicalMemoryAddress qemuPA (Flexus::Qemu::API::qemu_api.translate_iova2pa (aTranslate->bdf, (uint64_t)aTranslate->theVaddr));

		// DBG_Assert(((uint64_t)qflexPA) == ((uint64_t)qemuPA), (<< "Mismatch between QFlex Translation and QEMU Translation"));
	}

	bool available(interface::DeviceMemoryRequest const&) { return true; }
	void push(interface::DeviceMemoryRequest const&, MemoryMessage& aMemoryMessage)
	{

		// NOTE: it should be guaranteed by the Device that the memory message it sends
		// will not exceede the maximum TLP size. THus we need not worry if a memory message 
		// may span more than one page as the max TLP size is 4KB as per PCIe 6 Specification

		TranslationPtr tr(new Translation);
		  tr->setData();
		  tr->theType     = aMemoryMessage.type() == MemoryMessage::IOLoadReq ? Translation::eLoad : Translation::eStore;
		  tr->theVaddr    = aMemoryMessage.pc();	// holds the IOVA for IO devices
		  tr->thePaddr    = PhysicalMemoryAddress(0);   // This will hold the PA of IOVA after the PTW
		  tr->inTraceMode = true;
		  tr->setIO(aMemoryMessage.getBDF());

		PhysicalMemoryAddress qflexPA = translateIOVA (tr, aMemoryMessage);	// SMMU translation happens here

		// Validation
		PhysicalMemoryAddress qemuPA (Flexus::Qemu::API::qemu_api.translate_iova2pa (tr->bdf, (uint64_t)tr->theVaddr));
		DBG_Assert(((uint64_t)qflexPA) == ((uint64_t)qemuPA), (<< "Mismatch between QFlex Translation(0x" << std::hex << ((uint64_t)qflexPA) << ") and QEMU Translation(0x" << ((uint64_t)qemuPA) << ")"));

		aMemoryMessage.address() = tr->thePaddr;

		// aMemoryMessage is ready to perform the memory operation as it now has the physical address
		if (aMemoryMessage.getDataRequired()) {
			char * data = (char *) malloc ((aMemoryMessage.reqSize() / sizeof(uint32_t) + 1) * sizeof(uint32_t));	// Allocate a little bit more memory than actually needed

			for (int i = 0; i < aMemoryMessage.reqSize() / 4; i++) {	// Always read 4 bytes at a time
				*(((uint32_t *) (data)) + i) = (uint32_t)cpu.read_pa(aMemoryMessage.address() + 4 * i, 4);
			}

			aMemoryMessage.setDataPtr((uint64_t) data);
		}

		// TODO: Send reply

		// Inject memory message into the LLC after translation
		injectMemoryMessageIntoLLC(aMemoryMessage);

	}

private:
    // Reads Stream Table from memory, so it should be hooked to LLC or memory
    // There should also be a cache for Stream Table Entries
    StreamTableEntry getStreamTableEntry (uint16_t bdf) {
        L1StreamTableDescriptor l1StreamTableDescriptor;
		StreamTableEntry streamTableEntry;

        uint16_t SID = bdf;     // SMMU Stream ID is the same as BDF of PCIe device
		uint32_t split = streamTableBaseConfig.split;	// This is he index where the stream id needs to be split in order to get the two indices for the two stream tables
		uint16_t l1Index = SID >> split;				// SID[:split]
		uint16_t l2Index = SID & ((1U << split) - 1);	// SID[split-1:]

        uint64_t l1StreamTablePointer = streamTableBase + (l1Index * sizeof(l1StreamTableDescriptor));

		*((uint64_t *)(&l1StreamTableDescriptor)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l1StreamTablePointer), 8);	// Read with data
		injectMemoryMessageIntoLLC(PhysicalMemoryAddress(l1StreamTablePointer), sizeof(l1StreamTableDescriptor), false);		// Inject read into LLC

		DBG_Assert(l1StreamTableDescriptor.Span != 0, (<< "Invalid Stream Table Descriptor"));
		DBG_Assert((1 << (l1StreamTableDescriptor.Span - 1)) >= l2Index , (<< "l2 index is beyond the range of l2 table"));

		// Bits L2Ptr[N:0] are treated as 0 by the SMMU, where N == 5 + (Span - 1).
		// uint16_t N = 5 + (l1StreamTableDescriptor.Span - 1);
		// l1StreamTableDescriptor.L2Ptr = (l1StreamTableDescriptor.L2Ptr & ~((1 << (N + 1)) - 1));

		uint64_t l2StreamTablePointer = (l1StreamTableDescriptor.L2Ptr << 6) + (l2Index * sizeof(streamTableEntry));

        *((uint64_t *)(&streamTableEntry)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l2StreamTablePointer), 8);
		injectMemoryMessageIntoLLC(PhysicalMemoryAddress(l2StreamTablePointer), sizeof(StreamTableEntry), false);		// Inject read into LLC

		DBG_Assert(streamTableEntry.V == 1, (<< "Stream Table Entry is invalid. STE: " << streamTableEntry.toString() 
			<< "Others: L1Index: " << l1Index << "\tl2Index: " << l2Index << "\tSpan: " << l1StreamTableDescriptor.Span 
			<< "\tl1StreamTablePointer: " << std::hex << l1StreamTablePointer << "\tl2StreamTablePointer: " << l2StreamTablePointer << std::dec));

		DBG_Assert(streamTableEntry.Config == 5, (<< "Only Stage 1 translation and Stage 2 bypass is supported"));

		DBG_(VVerb, (<< "Stream Table Pointer (" << std::hex << l2StreamTablePointer << ")" ));

        return streamTableEntry;
    }

    ContextDescriptor getContextDescriptor (StreamTableEntry streamTableEntry) {
        ContextDescriptor contextDescriptor;

		DBG_Assert(streamTableEntry.S1Fmt == 0, (<< "Only Linear Context Descriptor Tables supported"));
		DBG_Assert(streamTableEntry.S1CDMax == 0, (<< "Only single entry in Context Descriptor Table supported. \
														We do not have SubstreamIDs to index into CD Table as PCIe BDF can only be converted to Stream ID"));

        uint64_t contextPointer = streamTableEntry.S1ContextPtr << 6;   // S1ContextPtr holds bits 55:6

		DBG_(VVerb, (<< "Context Descriptor Pointer (" << std::hex << contextPointer << ")" ));
        
        // Reading only the first 192 bits of context descriptor as the other fields are pretty useless
        *((uint64_t *)(&contextDescriptor))     = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer), 8);
        *((uint64_t *)(&contextDescriptor) + 1) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer + 8), 8);
        *((uint64_t *)(&contextDescriptor) + 2) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer + 16), 8);

		injectMemoryMessageIntoLLC(PhysicalMemoryAddress(contextPointer), sizeof(ContextDescriptor), false);

		DBG_Assert(contextDescriptor.V == 1, (<< "Context Descriptor is invalid" ));

        return contextDescriptor;
    }

    // This function has been copied from QEMU: smmu-common.c as I couldn't understand how to select the correct TTB 
    uint64_t getTranslationTableBase (VirtualMemoryAddress iova, ContextDescriptor contextDescriptor) {
        bool tbi = extract64((uint64_t)iova, 55, 1) ? TBI1(contextDescriptor.TBI0) : TBI0(contextDescriptor.TBI1);
        uint8_t tbi_byte = tbi * 8;

        if (contextDescriptor.T0SZ &&
            !extract64(iova, 64 - contextDescriptor.T0SZ, contextDescriptor.T0SZ - tbi_byte)) {
            /* there is a ttbr0 region and we are in it (high bits all zero) */
            return contextDescriptor.TTB0 << 4;

        } else if (contextDescriptor.T1SZ &&
            sextract64(iova, 64 - contextDescriptor.T1SZ, contextDescriptor.T1SZ - tbi_byte) == -1) {
            /* there is a ttbr1 region and we are in it (high bits all one) */
            return contextDescriptor.TTB1 << 4;

        } else if (!contextDescriptor.T0SZ) {
            /* ttbr0 region is "everything not in the ttbr1 region" */
            return contextDescriptor.TTB0 << 4;

        } else if (!contextDescriptor.T1SZ) {
            /* ttbr1 region is "everything not in the ttbr0 region" */
            return contextDescriptor.TTB1 << 4;
        }

        DBG_(VVerb, ( << "SMMU Error: Translation Table Base not found" ));

        /* in the gap between the two regions, this is a Translation fault */
        return 0;
    }

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	/**
	 * Currently, the command queue is processed
	 * whenever there is a write to the CMDQ_PROD register
	 * The entire queue is processed all at once
	 * However, the real hardware may not process all the 
	 * commands at once. It may process some commands and
	 * update the CMDQ_CONS register such that the CMDQ is
	 * not empty.
	 * But right now, we are implementing it such that the
	 * entire wueue is processed all at once and CMDQ_CONS
	 * and CMDQ_PROD become equal (empty queue)
	 * This may lead to incorrect or subpar behavior
	 * If we optimistically process more commands than are done
	 * by the HW, worst case scenario, we end up reexecuting
	 * those commands. For IOTLB invalidation, this may lead to 
	 * more invalidations than are intended. But that does not
	 * imply incorrect behavior, just more IOPTW walks
	 */
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	void processCommandQueue () {
		*((uint32_t *)(&commandQueueProd)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CMDQ_PROD), 4);
		
		uint32_t QS = commandQueueLog2Size;

		uint32_t prodWrBit = (commandQueueProd >> QS) & 1U;
		uint32_t consWrBit = (commandQueueCons >> QS) & 1U;

		commandQueueProd &= ((1UL << QS) - 1);
		commandQueueCons &= ((1UL << QS) - 1);

		DBG_(VVerb, ( 	<< "Processing Command Queue: CMDQ_PROD(" << commandQueueProd 
						<< ")\tCMDQ_CONS(" << commandQueueCons 
						<< ")\tPROD_WR(" << prodWrBit 
						<< ")\tCONS_WR(" << consWrBit 
						<< ")" ));

		while (!(commandQueueProd == commandQueueCons && prodWrBit == consWrBit)) {	// Empty Queue Condition
			processCommand(commandQueueCons);
			
			commandQueueCons++;
			consWrBit++;
			consWrBit &= 1UL;
		}

		/** QEMU also processes all the commands in the queue at once
		 * So if commandQueueCons is also initialized at the beginning of this function 
		 * like commandQueueProd, both will have the same value and appear to be empty
		 * So updating the commandQueueCons at the end of this function causes the next 
		 * invocation of this function to see the old value of commandQueueCons and the 
		 * queue appears to be non-empty
		 */
		*((uint32_t *)(&commandQueueCons)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_CMDQ_CONS), 4);
	}

	void processCommand (uint32_t index) {

		// TODO: Implement other non-invalidation commands as well
		// TODO: Implement Config Cache invalidations
		// TODO: Invalidation Broadcast
		InvalidationCommand invalidationCommand;

		*((uint64_t *)(&invalidationCommand)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(commandQueueBase + sizeof(invalidationCommand) * index), 8);
		*((uint64_t *)(&invalidationCommand) + 1) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(commandQueueBase + sizeof(invalidationCommand) * index + 8), 8);
		injectMemoryMessageIntoLLC(PhysicalMemoryAddress(commandQueueBase + sizeof(invalidationCommand) * index), sizeof(InvalidationCommand), false);

		switch (invalidationCommand.opcode) {
            case InvalidationCommand::CMD_TLBI_NH_ALL:
				theIOTLB->invalidate();
				DBG_(VVerb, (<< "Invalidated Entire IOTLB" 	<< std::endl ));
				break;
			case InvalidationCommand::CMD_TLBI_NH_VAA:
            case InvalidationCommand::CMD_TLBI_NH_ASID:
				theIOTLB->invalidate(invalidationCommand.ASID);
				DBG_(VVerb, (<< "Invalidated IOTLB ASID(" << invalidationCommand.ASID << ")"	<< std::endl ));
				break;
            case InvalidationCommand::CMD_TLBI_NH_VA:
				theIOTLB->invalidate(invalidationCommand.ASID,  VirtualMemoryAddress(invalidationCommand.Address << 12));	// TODO: Should not hardcode 12 here. 
																																				// ? What to do when we have hugepages?
				DBG_(VVerb, (<< "Invalidated IOTLB ASID(" << invalidationCommand.ASID << ")\tIOVA(" << std::hex << (invalidationCommand.Address << 12)	<< ")" << std::endl ));
				break;
			default:
				DBG_(VVerb, (<< "Not IOTLB Invalidation Command" 	<< std::endl ));
        }
	}

	std::vector <MemoryMessage> generateCacheMessages (MemoryMessage & memoryMessage) {
		std::vector <MemoryMessage> ret;

		uint32_t bytesLeft = memoryMessage.reqSize();
      	uint32_t numberOfMessages = ceil(bytesLeft / (CacheMessageSize * 1.0));

		for (uint32_t i = 0; i < numberOfMessages; i++) {

			uint32_t cacheMessageSize = bytesLeft > CacheMessageSize ? CacheMessageSize : bytesLeft;

			MemoryMessage cacheMessage (memoryMessage);

			DBG_Assert(memoryMessage.type() == MemoryMessage::IOLoadReq || memoryMessage.type() == MemoryMessage::IOStoreReq, (<< "Invalid IO Request Type" ));

			cacheMessage.type() = memoryMessage.type() == MemoryMessage::IOLoadReq ? MemoryMessage::ReadReq : MemoryMessage::WriteReq;
			cacheMessage.reqSize() = cacheMessageSize;
			cacheMessage.pc() = VirtualMemoryAddress( (uint64_t)memoryMessage.pc() + i * CacheMessageSize );
			cacheMessage.address() = PhysicalMemoryAddress( (uint64_t)memoryMessage.address() + i * CacheMessageSize );

			// Data is not actually required so dataRequired field is not set here

			ret.push_back(cacheMessage);

			bytesLeft -= cacheMessageSize;
		}

      return ret;
	}

	void injectMemoryMessageIntoLLC (MemoryMessage & memoryMessage) {	// Inject memory message
		std::vector <MemoryMessage> cacheMessages = generateCacheMessages (memoryMessage);
		
		for (auto & cacheMessage : cacheMessages)
			FLEXUS_CHANNEL(MemoryRequest) << cacheMessage;
	}

	void injectMemoryMessageIntoLLC (PhysicalMemoryAddress pa, uint64_t size, bool isWrite) {	// Prepare the memory message and then inject
		MemoryMessage memoryMessage (isWrite ? MemoryMessage::IOStoreReq : MemoryMessage::IOLoadReq, pa);
		memoryMessage.reqSize() = size;
		memoryMessage.setFromSMMU();

		injectMemoryMessageIntoLLC(memoryMessage);
	}

	void printSMMUConfig() {
		DBG_(Crit, (<< "Initializing SMMU from QEMU..." 								<< std::endl
                << std::hex

                // Print Receive Side Registers
                << "\t" << "Config Base Address: "  		<< configBaseAddress  		<< std::endl
                << "\t" << "Config Size: "   				<< configSize  				<< std::endl
				<< "\t" << "Interface Config: "   			<< smmuInterfaceConfig.toString()  		<< std::endl
				<< "\t" << "Command Queue Base Address: "  	<< commandQueueBase  		<< std::endl
				<< "\t" << "Command Queue Log2Size: "  		<< commandQueueLog2Size  		<< std::endl
				<< "\t" << "Command Queue Prod: "  			<< commandQueueProd  		<< std::endl
				<< "\t" << "Command Queue Cons: "  			<< commandQueueCons  		<< std::endl
                << "\t" << "Stream Table Base Config: " 	<< streamTableBaseConfig.toString()  	<< std::endl
                << "\t" << "Stream Table Base Address: "  	<< streamTableBase  		<< std::endl
                << "\t" << "Stream Table Size: "  			<< streamTableSize  		<< std::endl

                << std::dec
                << std::endl));
	}

private:
    bool cfg_smmu(index_t anIndex, ContextDescriptor contextDescriptor, VirtualMemoryAddress iova)
    {
        bool ret = false;
        theMMU.reset(new mmu_t());

		theMMU->setupBitConfigs();

        if (init_smmu_regs(anIndex, theMMU, contextDescriptor, iova)) {
            theMMU->setupAddressSpaceSizesAndGranules();
            DBG_Assert(theMMU->Gran0->getlogKBSize() == 12, (<< "TG0 has non-4KB size - unsupported"));
            DBG_Assert(theMMU->Gran1->getlogKBSize() == 12, (<< "TG1 has non-4KB size - unsupported"));
            PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
            ret      = true;
        }

        return ret;
    }

	void setupBitConfigs()
	{
		// enable and endian-ness
		// - in SCTLR_ELx
		aarch64_bit_configs.M_Bit   = 0;  // 0b1 = MMU enabled
		aarch64_bit_configs.EE_Bit  = 25; // endianness of EL1, 0b0 is little-endian
		aarch64_bit_configs.EoE_Bit = 24; // endianness of EL0, 0b0 is little-endian

		// Granule that this MMU supports
		// - in ID_AA64MMFR0_EL1
		aarch64_bit_configs.TGran4_Base  = 28;  // = 0b0000 IF supported, = 0b1111 if not
		aarch64_bit_configs.TGran16_Base = 20;  // = 0b0000 if NOT supported, 0b0001 IF
												// supported (yes, this is correct)
		aarch64_bit_configs.TGran64_Base  = 24; // = 0b0000 IF supported, = 0b1111 if not
		aarch64_bit_configs.TGran_NumBits = 4;

		// Granule configured for EL1 (TODO: add others if we ever care about them in
		// the future)
		// - in TCR_ELx
		aarch64_bit_configs.TG0_Base = 14; 	/* 	00 = 4KB
											01 = 64KB
											11 = 16KB */
		aarch64_bit_configs.TG0_NumBits = 2;
		aarch64_bit_configs.TG1_Base    = 30; /* 01 = 16KB
												10 = 4KB
												11 = 64KB (yes, this is different than TG0) */
		aarch64_bit_configs.TG1_NumBits = 2;

		// Physical, output, and input address sizes. ( in ID_AA64MMFR0_EL1 )
		aarch64_bit_configs.PARange_Base = 0; /* 0b0000 = 32b
												0b0001 = 36b
												0b0010 = 40b
												0b0011 = 42b
												0b0100 = 44b
												0b0101 = 48b
												0b0110 = 52b */
		aarch64_bit_configs.PARange_NumBits = 4;

		// translation range sizes, chooses which VA's map to TTBR0 and TTBR1
		// - in TCR_ELx
		aarch64_bit_configs.TG0_SZ_Base    = 0;
		aarch64_bit_configs.TG1_SZ_Base    = 16;
		aarch64_bit_configs.TGn_SZ_NumBits = 6;
	}

	bool init_smmu_regs(std::size_t core_index, std::shared_ptr<mmu_t> theMMU, ContextDescriptor contextDescriptor, VirtualMemoryAddress iova)	// SMMU Config changes per context
	{
		using namespace Flexus::Qemu;

		setupBitConfigs(); // initialize AARCH64 bit locations for masking

		//? sctlr_el0 does not exist
		theMMU->mmu_regs.SCTLR[EL1] = cpu.read_register(Qemu::API::SCTLR, EL1);

		//? tcr_el0 does not exist
		theMMU->mmu_regs.TCR[EL1] = 0;
		theMMU->mmu_regs.TCR[EL1] |= contextDescriptor.T0SZ << aarch64_bit_configs.TG0_SZ_Base;
		theMMU->mmu_regs.TCR[EL1] |= contextDescriptor.TG0 << aarch64_bit_configs.TG0_Base;

		// When EPD1 is 1, TT1 is disabled. To let the core side MMU still work with the TT1, we need to pass the appropriate
		// values. In this case, the values are the same as TT0 config but TG1 has to be 2 as that corresponds to 4KB page
		theMMU->mmu_regs.TCR[EL1] |= (contextDescriptor.EPD1 ? contextDescriptor.T0SZ : contextDescriptor.T1SZ) << aarch64_bit_configs.TG1_SZ_Base;
		theMMU->mmu_regs.TCR[EL1] |= (contextDescriptor.EPD1 ? 2 : contextDescriptor.TG1) << aarch64_bit_configs.TG1_Base;

		DBG_(VVerb, ( << "SMMU TCR: 0x" << std::hex << theMMU->mmu_regs.TCR[EL1] << std::dec ));

		// In SMMU, the translation table base is selected based on the IOVA
		// As we are using the Core side MMU, core MMU does not have the ability to choose
		// between TTB0 and TTB1 of SMMU. So we pass the same TTB to both TTB0 and TTB2.
		// No matter which TTB core side MMU chooses, it will always use the correct TTB
		theMMU->mmu_regs.TTBR0[EL1] = getTranslationTableBase(iova, contextDescriptor);
		theMMU->mmu_regs.TTBR1[EL1] = getTranslationTableBase(iova, contextDescriptor);

		//? Section D23.2.74 - AArch64 Memory Model Feature Register 0
		theMMU->mmu_regs.ID_AA64MMFR0_EL1 = cpu.read_register(Qemu::API::ID_AA64MMFR0, EL1);

		return (theMMU->mmu_regs.TCR[EL1] != 0);
	}
};

} // End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR(SMMU, nSMMU);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()
