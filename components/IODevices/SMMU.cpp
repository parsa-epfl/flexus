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
		uint64_t V              : 1;
		uint64_t Config         : 3;
		uint64_t S1Fmt          : 2;
		uint64_t S1ContextPtr   : 50;
		uint64_t Res0           : 3;
		uint64_t S1CDMax        : 5;

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
		uint64_t EPD1           : 1;
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

		DBG_(VVerb, (<< "SMMU IDR0: " << std::hex << (uint32_t)cpu.read_pa(PhysicalMemoryAddress(configBaseAddress + SMMU_IDR0), 4) << std::dec));
	}

	PhysicalMemoryAddress translateIOVA (uint16_t bdf, VirtualMemoryAddress iova) {

		DBG_(VVerb, (<< "Translating: BDF: " << std::hex << bdf << "\tIOVA: " << (uint64_t) iova << std::dec ));

        StreamTableEntry streamTableEntry = getStreamTableEntry (bdf);
        ContextDescriptor contextDescriptor = getContextDescriptor (streamTableEntry);
        uint64_t translationTableBase = getTranslationTableBase(iova, contextDescriptor);

		DBG_(VVerb, (<< "Translation: BDF: " << std::hex << bdf << "\tIOVA: " << (uint64_t) iova << "\tTranslation Table Base: " << translationTableBase << std::dec));

		return PhysicalMemoryAddress(-1);
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
		translateIOVA (0x8, aTranslate->theVaddr);	// TODO: BDF is hardcoded right now. It should come from the translation message
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

        uint64_t l1StreamTablePointer = streamTableBase + (l1Index * 8);

		*((uint64_t *)(&l1StreamTableDescriptor)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l1StreamTablePointer), 8);

		DBG_Assert(l1StreamTableDescriptor.Span != 0, (<< "Invalid Stream Table Descriptor"));
		DBG_Assert((1 << (l1StreamTableDescriptor.Span - 1)) >= l2Index , (<< "l2 index is beyond the range of l2 table"));

		// Bits L2Ptr[N:0] are treated as 0 by the SMMU, where N == 5 + (Span - 1).
		// uint16_t N = 5 + (l1StreamTableDescriptor.Span - 1);
		// l1StreamTableDescriptor.L2Ptr = (l1StreamTableDescriptor.L2Ptr & ~((1 << (N + 1)) - 1));

		uint64_t l2StreamTablePointer = (l1StreamTableDescriptor.L2Ptr << 6) + (l2Index * 8);

        *((uint64_t *)(&streamTableEntry)) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(l2StreamTablePointer), 8);

		DBG_Assert(streamTableEntry.V == 1 , (<< "Stream Table Entry is invalid. STE: " << streamTableEntry.toString() 
			<< "Others: L1Index: " << l1Index << "\tl2Index: " << l2Index << "\tSpan: " << l1StreamTableDescriptor.Span << "\tl2StreamTablePointer: " << std::hex << l2StreamTablePointer << std::dec));

		DBG_(VVerb, (<< "Stream Table Entry: " << streamTableEntry.toString() ));

        return streamTableEntry;
    }

    ContextDescriptor getContextDescriptor (StreamTableEntry streamTableEntry) {
        ContextDescriptor contextDescriptor;

        uint64_t contextPointer = streamTableEntry.S1ContextPtr << 6;   // S1ContextPtr holds bits 55:6
        
        // Reading only the first 192 bits of context descriptor as the other fields are pretty useless
        *((uint64_t *)(&contextDescriptor))     = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer), 8);
        *((uint64_t *)(&contextDescriptor) + 1) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer + 8), 8);
        *((uint64_t *)(&contextDescriptor) + 2) = (uint64_t)cpu.read_pa(PhysicalMemoryAddress(contextPointer + 16), 8);

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

};

} // End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR(SMMU, nSMMU);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()
