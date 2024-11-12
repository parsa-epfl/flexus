#include "core/debug/debug.hpp"
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
	typedef struct StreamTableBaseConfig {
		uint64_t log2size   : 6;        // Table size as log2 (entries).
		uint64_t split      : 5;        // StreamID split point for multi-level table. 6->4KB leaf, 8->16KB leaf, 10->64KB leaf
		uint64_t res0       : 5;        
		uint64_t fmt        : 2;        // Format of Stream table: 0->Linear, 1->2-level
		uint64_t res1       : 14;

		// toString method that returns a formatted string
		std::string toString() const {
			std::ostringstream oss;
			oss << "Log2Size(" << log2size << ")\tSplit(" << split << ")\tFmt(" << fmt << ")";
			return oss.str();
		}
	} StreamTableBaseConfig;

	typedef struct L1StreamTableDescriptor {
		uint64_t Span			: 5;
		uint64_t Res0         	: 1;
		uint64_t L2Ptr          : 50;
		uint64_t Res1		    : 8;
	} L1StreamTableDescriptor;

	typedef struct StreamTableEntry {
		// First 64-bits
		uint64_t V              : 1;
		uint64_t Config         : 3;
		uint64_t S1Fmt          : 2;
		uint64_t S1ContextPtr   : 50;
		uint64_t Res0           : 3;
		uint64_t S1CDMax        : 5;

		// Rest of the bits are garbage
		uint64_t g0[7];

		// toString method that returns a formatted string
		std::string toString() const {
			std::ostringstream oss;
			oss << "V(" << V << ")\tConfig(" << Config << ")\tS1Fmt(" << S1Fmt 
				<< ")\tS1ContextPtr(" << S1ContextPtr << ")\tS1CDMax(" << S1CDMax << ")";
			return oss.str();
		}
	} StreamTableEntry;

	typedef struct ContextDescriptor {
		// 0-31 bits
		uint64_t T0SZ           : 6;
		uint64_t TG0            : 2;
		uint64_t IR0            : 2;
		uint64_t OR0            : 2;
		uint64_t SH0            : 2;
		uint64_t EPD0           : 1;
		uint64_t ENDI           : 1;
		uint64_t T1SZ           : 6;
		uint64_t TG1            : 2;
		uint64_t IR1            : 2;
		uint64_t OR1            : 2;
		uint64_t SH1            : 2;
		uint64_t EPD1           : 1;	// 1 if TT1 is Disabled
		uint64_t V              : 1;

		// 31-63 bits
		uint64_t IPS            : 3;
		uint64_t AFFD           : 1;
		uint64_t WXN            : 1;
		uint64_t UWXN           : 1;
		uint64_t TBI0           : 1;
		uint64_t TBI1           : 1;
		uint64_t PAN            : 1;
		uint64_t AA64           : 1;
		uint64_t HD             : 1;
		uint64_t HA             : 1;
		uint64_t S              : 1;
		uint64_t R              : 1;
		uint64_t A              : 1;
		uint64_t ASET           : 1;
		uint64_t ASID           : 16;

		// 64-127 bits
		uint64_t NSCFG0         : 1;
		uint64_t DisCH0         : 1;
		uint64_t E0PD0          : 1;
		uint64_t HAFT           : 1;
		uint64_t TTB0           : 52;
		uint64_t Res0           : 2;
		uint64_t PnCH           : 1;
		uint64_t EPAN           : 1;
		uint64_t HWU059         : 1;
		uint64_t HWU060         : 1;
		uint64_t SKL0           : 2;

		// 128-191 bits
		uint64_t NSCFG1         : 1;
		uint64_t DisCH1         : 1;
		uint64_t E0PD1          : 1;
		uint64_t AIE            : 1;
		uint64_t TTB1           : 52;
		uint64_t Res1           : 2;
		uint64_t DS             : 1;
		uint64_t PIE            : 1;
		uint64_t HWU159         : 1;
		uint64_t HWU160         : 1;
		uint64_t SKL1           : 2;

		// 192-511 are garbage
		uint64_t garbage[5];

		// toString method that returns a formatted string
		std::string toString() const {
			std::ostringstream oss;
			oss << "T0SZ(" << T0SZ << ")\tTG0(" << TG0 << ")\tT1SZ(" << T1SZ 
				<< ")\tTG1(" << TG1 << ")\tEPD0(" << EPD0 << ")\tEPD1(" << EPD1 << ")";
			return oss.str();
		}

	} ContextDescriptor;

private:
    const uint32_t configBaseAddress	=	0x09050000;       // TODO: Should be determined from QEMU. Base address of SMMU config space
    const uint32_t configSize			=	0x00020000;       // Size of SMMU config space

private:
    StreamTableBaseConfig streamTableBaseConfig;
    uint64_t streamTableSize;
    uint64_t streamTableBase;

private:
    const uint32_t SMMU_STRTAB_BASE_CFG     =   0x0088;       // Stream Table Base Configuration register offset
    const uint32_t SMMU_STRTAB_BASE         =   0x0080;       // Stream Table Base register offset
	const uint32_t SMMU_IDR0				=	0x0000;

// These are from core MMU of QFlex
private:
	bit_configs_t aarch64_bit_configs;
	std::unique_ptr<PageWalk> thePageWalker;
	std::shared_ptr<mmu_t> theMMU;

  public:
	FLEXUS_COMPONENT_CONSTRUCTOR(SMMU)
	  : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
	{
	}

	// Initialization
	void initialize()
	{
		cpu = Flexus::Qemu::Processor::getProcessor(0); // Use CPU 0 for now

		*((uint32_t *)(&streamTableBaseConfig)) = (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_STRTAB_BASE_CFG), 4);
        streamTableBase         = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_STRTAB_BASE), 8);
        streamTableBase &= 0xffffffffffffff;
        streamTableBase = ((streamTableBase >> 6) << 6);

        DBG_Assert(streamTableBaseConfig.fmt == 1, (<< "Invalid Stream Table Format. Only 2 level tables are supported"));

        streamTableSize = 1ULL << streamTableBaseConfig.log2size;

		printSMMUConfig();

		thePageWalker.reset(new PageWalk(flexusIndex()));
		thePageWalker->setMMU(theMMU);
	}

	PhysicalMemoryAddress translateIOVA (uint16_t bdf, TranslationPtr& aTranslate) {

		DBG_(VVerb, (<< "Translating: BDF: " << std::hex << bdf << "\tIOVA: " << (uint64_t) aTranslate->theVaddr << std::dec ));

        StreamTableEntry streamTableEntry = getStreamTableEntry (bdf);
        ContextDescriptor contextDescriptor = getContextDescriptor (streamTableEntry);
        uint64_t translationTableBase = getTranslationTableBase(aTranslate->theVaddr, contextDescriptor);

		cfg_smmu(0, contextDescriptor, aTranslate->theVaddr);	// TODO: Passing 0 for now as we have only 1 device

		thePageWalker->setMMU(theMMU);

		thePageWalker->push_back_trace(aTranslate, Flexus::Qemu::Processor::getProcessor(flexusIndex()));

		PhysicalMemoryAddress pma[4];	// Max 4 levels 
		uint64_t traceSize = aTranslate->trace_addresses.size();
		for (uint64_t i = 0; i < traceSize; i++) {
			pma[i] = aTranslate->trace_addresses.front();
			aTranslate->trace_addresses.pop();
		}

		DBG_(VVerb, (	<< "Translation: BDF: " << std::hex << bdf 
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

	bool available(interface::TranslationRequestIn const&) { return true; }
	void push(interface::TranslationRequestIn const&, TranslationPtr& aTranslate)
	{
		PhysicalMemoryAddress qflexPA = translateIOVA (0x8, aTranslate);	// TODO: BDF is hardcoded right now. It should come from the translation message
		PhysicalMemoryAddress qemuPA (Flexus::Qemu::API::qemu_api.translate_iova2pa (0x8, (uint64_t)aTranslate->theVaddr));

		DBG_Assert(((uint64_t)qflexPA) == ((uint64_t)qemuPA), (<< "Mismatch between QFlex Translation and QEMU Translation"));
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

		*((uint64_t *)(&l1StreamTableDescriptor)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l1StreamTablePointer), 8);

		DBG_Assert(l1StreamTableDescriptor.Span != 0, (<< "Invalid Stream Table Descriptor"));
		DBG_Assert((1 << (l1StreamTableDescriptor.Span - 1)) >= l2Index , (<< "l2 index is beyond the range of l2 table"));

		// Bits L2Ptr[N:0] are treated as 0 by the SMMU, where N == 5 + (Span - 1).
		// uint16_t N = 5 + (l1StreamTableDescriptor.Span - 1);
		// l1StreamTableDescriptor.L2Ptr = (l1StreamTableDescriptor.L2Ptr & ~((1 << (N + 1)) - 1));

		uint64_t l2StreamTablePointer = (l1StreamTableDescriptor.L2Ptr << 6) + (l2Index * sizeof(streamTableEntry));

        *((uint64_t *)(&streamTableEntry)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l2StreamTablePointer), 8);

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

	void printSMMUConfig() {
		DBG_(VVerb, (<< "Initializing SMMU from QEMU..." 								<< std::endl
                << std::hex

                // Print Receive Side Registers
                << "\t" << "Config Base Address: "  		<< configBaseAddress  		<< std::endl
                << "\t" << "Config Size: "   				<< configSize  				<< std::endl
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
