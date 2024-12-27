#ifndef __SMMU_COMMAND_TYPES_HPP__
#define __SMMU_COMMAND_TYPES_HPP__

#include <stdint.h>
#include <sstream>

namespace Flexus {
namespace SharedTypes {

typedef struct SMMUInterfaceConfig {
	uint32_t SMMUEN			: 1;
	uint32_t PRIQEN			: 1;
	uint32_t EVENTQEN		: 1;
	uint32_t CMDQEN			: 1;
	uint32_t ATSCHK			: 1;
	uint32_t Res0			: 1;
	uint32_t VMW			: 3;
	uint32_t Res1			: 1;
	uint32_t DPT_WALK_EN	: 1;
	uint32_t Res3			: 21;
	
	// toString method that returns a formatted string
	std::string toString() const {
		std::ostringstream oss;
		oss << "SMMUEN(" << SMMUEN << ")\tPRIQEN(" << PRIQEN << ")\tEVENTQEN(" << EVENTQEN 
			<< ")\tCMDQEN(" << CMDQEN << ")\tATSCHK(" << ATSCHK << ")\tVMW(" << VMW << ")";
		return oss.str();
	}
} SMMUInterfaceConfig;


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

typedef struct InvalidationCommand {

	enum CommandType {
		// EL1 specific commands
		CMD_TLBI_NH_ALL		=	0x10,
		CMD_TLBI_NH_ASID	=	0x11,
		CMD_TLBI_NH_VAA		=	0x13,
		CMD_TLBI_NH_VA		=	0x12,
		CMD_TLBI_NSNH_ALL	=	0x30
	};

	// First 32-bits
	CommandType opcode	: 8;
	uint64_t Res0		: 4;
	uint64_t NUM		: 5;
	uint64_t Res1		: 3;
	uint64_t SCALE		: 6;
	uint64_t Res2		: 6;

	// 63-32 bits
	uint64_t VMID		: 16;
	uint64_t ASID		: 16;

	// 127-64 bits
	uint64_t Leaf		: 1;
	uint64_t Res3		: 6;
	uint64_t TTL128		: 1;
	uint64_t TTL		: 2;
	uint64_t TG			: 2;
	uint64_t Address	: 52;
	
	// toString method that returns a formatted string
	std::string toString() const {
		std::ostringstream oss;
		oss << "opcode("		<< opcode
			<< ")\tVMID("		<< VMID 
			<< ")\tASID("		<< ASID 
			<< ")\tAddress("	<< Address 
			<< ")";
		return oss.str();
	}

} InvalidationCommand;

}
}

#endif