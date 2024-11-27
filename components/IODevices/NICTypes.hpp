#ifndef __NIC_TYPES_HPP__
#define __NIC_TYPES_HPP__

#include <cstdint>
#include <stdint.h>
#include <sstream>

namespace Flexus {
namespace SharedTypes {

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


typedef union AdvancedReceiveDescriptor {
	
	struct AdvancedReceiveReadDescriptor {
		uint64_t packetBufferAddress	: 64;
		uint64_t DD						: 1;	// Descriptor Done: Indicates that the hardware has processed this descriptor
		uint64_t headerBufferAddress 	: 63;
	} advancedReceiveReadDescriptor; // Advanced Receive Read Descriptor for reading the Rx data and header buffer

	struct AdvancedReceiveWriteDescriptor {
		uint64_t RSS_Type					: 4;
		uint64_t Packet_Type				: 13;
		uint64_t RSV						: 4;
		uint64_t HDR_LEN					: 10;
		uint64_t SPH						: 1;
		uint64_t RSSHash_FragChecksum_IPid	: 32;

		uint64_t DD							: 1;
		uint64_t EOP						: 1;		// End-Of-Packet: Indicates whether it this was the last buffer holding a packet as a packet may span multiple buffer descriptors
		uint64_t Res0						: 18;		// When EOP is 1, these fields hold some valid data but that is currently irrelevant to us

		uint64_t Extended_Error				: 12;
		uint64_t PKT_LEN					: 16;
		uint64_t VLAN_Tag					: 16;
	} advancedReceiveWriteDescriptor;	// Advanced Receive Write Descriptor for writing back the information about the packet received and/or processed by the NIC

} AdvancedReceiveDescriptor;

}
}

#endif