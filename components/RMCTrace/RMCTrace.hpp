#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/RMCEntry.hpp>
#include <components/CommonQEMU/Slices/RMCPacket.hpp>
#include <components/CommonQEMU/rmc_breakpoints.hpp>
#include <components/CommonQEMU/RMC_SRAMS.hpp>

#include <son-common/libsonuma/flexus_include.h>

#include <stdlib.h>
#ifdef __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

#define NO_PAGE_WALKS 1

#define DATA_STORE_OP 	StoreDataInLLC	//StoreData
#define DATA_LOAD_OP 	LoadDataInLLC	//LoadData

#define RMC_DEBUG Verb

typedef enum RMCStatus
{
	INIT,
	READY
} RMCStatus;

#define FLEXUS_BEGIN_COMPONENT RMCTrace
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  //PARAMETER( Enable, bool, "Enable RMC", "enable", true )
  PARAMETER( TLBEntries, uint32_t, "Number of TLB entries", "TLBEntries", 10)
  
  PARAMETER( TextFlexpoints, bool, "Store flexpoints as text files", "text_flexpoints", true )
  PARAMETER( GZipFlexpoints, bool, "Compress flexpoints with gzip", "gzip_flexpoints", false )
  
  PARAMETER( ReadyCountTarget, uint32_t, "The number of threads to wait for their completion to start RMCs", "ready_count_target", 1 ) 
  PARAMETER( RGPPerTile, bool, "Put one RGP/RCP per tile. Default is one per row across the first column", "rgp_per_tile", false )
  PARAMETER( RRPPPerTile, bool, "Put one RRPP per tile. Default is one per row across the first column", "rrpp_per_tile", false )
  PARAMETER( RRPPWithMCs, bool, "Put the RRPPs on the MC tiles (last column). Default is on the first column", "rrpps_with_mcs", false )
  PARAMETER( SharedL1, bool, "RMC and core share the same L1 cache", "sharedL1", false )
  PARAMETER( CopyEmuRMC, bool, "Directly copy rmc_state_* from load dir to save dir", "copy_emu_rmc", false )
  PARAMETER( SingleRMCFlexpoint, bool, "All control info for RMCs are stored in a single file", "singleRMC_flexpoint", false )		//this allows the reuse of the same flexpoint to test different RMC
																																	//placements and QP assignments
  PARAMETER( MachineCount, uint32_t, "Number of machines", "MachineCount", 1 )
);

COMPONENT_INTERFACE(
  //DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, RequestOut )
  PORT( PushOutput, MemoryMessage, RequestOut )
  
  PORT( PushInput, RMCEntry, RequestIn )
  
  PORT( PushOutput, RMCPacket, MsgOut )
  PORT( PushInput,  RMCPacket, MsgIn )
  
  DRIVE( PollL1Cache )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT RMCTrace
