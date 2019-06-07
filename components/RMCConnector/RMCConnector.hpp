/* In this version of the simulator (singleFatNode), we have one RMCConnector per RMC
 * and since we have one RMC per NI, an RMCConnector is an NI
 * The RMCConnector also generates the responses to the outgoing requests, plus synthetic
 * requests from remote nodes
 */

#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/RMCEntry.hpp>
#include <components/CommonQEMU/Slices/RMCPacket.hpp>
#include <components/CommonQEMU/rmc_breakpoints.hpp>

#include <stdlib.h>
#ifdef __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

#define CONNECTOR_DEBUG		Verb
#define CONNECTOR_VVERB 	Verb
#define QUEUES_DEBUG 			Verb	//Tmp

#define FLEXUS_BEGIN_COMPONENT RMCConnector
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define REPIN 	0
#define REQIN 	1
#define REPOUT 	0
#define REQOUT 	1

//#define TRACE_LATENCY

typedef struct queuePacket {
	RMCPacket thePacket;
	int32_t cycleCounter;
} queuePacket;

typedef struct queueReplyPacket {
	RMCPacket thePacket;
	int32_t cycleCounter;
	uint32_t theRMCId;	//Quick n Dirty solution. Should be able to find the correct return port just by the packet header
	uint8_t msgType; //Request or Reply. Only used in Loopback mode
} queueReplyPacket;

COMPONENT_PARAMETERS(
  PARAMETER( networkLatency, uint32_t, "The network latency in cycles (one way)", "network_latency", 3)

  PARAMETER( bandwidth, uint32_t, "Aggregate Node Bandwidth (Gbps)", "bandwidth", 100)
  PARAMETER( cpu_freq, uint32_t, "cpu_freq", "cpu_freq", 2000)
  PARAMETER( buffers, uint32_t, "Buffers per VC", "buffers", 20)
  PARAMETER( NodeDegree, uint32_t, "Node degree (related to topology)", "node_degree", 6)	//default: 3D torus

  PARAMETER( RGPCount, int, "Number of RGPs", "RGPCount", 8)
  PARAMETER( RRPPCount, int, "Number of RRPPs", "RRPPCount", 8)
  PARAMETER( RGPLocation, std::string, "RGP locations (ex: '0,8,16,24,32,40,48,56')", "RGPLocation", "0,8,16,24,32,40,48,56")	//currently unused
  PARAMETER( RRPPLocation, std::string, "RRPP locations (ex: '0,8,16,24,32,40,48,56')", "RRPPLocation", "0,8,16,24,32,40,48,56")	//currently unused

  PARAMETER( RRPPDistribution, std::string, "Distribute incoming traffic to RRPPs (random, address-LLC, address-mem)", "RRPPDistribution", "random" )
  PARAMETER( DirInterleaving, int, "Interleaving between directories (in bytes)", "DirInterleaving", 64)
  PARAMETER( MemInterleaving, int, "Interleaving between memory controllers (in bytes)", "MemInterleaving", 4096)

  PARAMETER( Loopback, bool, "Requests will be serviced by the requester itself", "Loopback", false )
					//if false, the component models incoming requests by mirroring outgoing requests and replies from a supposed remote node.
					//In practice, this means that the data carried by replies is garbage.
					//if true, the request is sent back to the same requesting node, where it is serviced as a
					//normal incoming remote request. Similarly, the reply is sent out and looped back in.
);

COMPONENT_INTERFACE(
  //PORT( PushInput, RMCPacket, CheckLinkAvail )
  //PORT( PushOutput, uint32_t, LinkAvailRep )

  DYNAMIC_PORT_ARRAY( PushInput, uint64_t, RRPPLatency )

  DYNAMIC_PORT_ARRAY( PushInput, RMCPacket, MessageIn )
  DYNAMIC_PORT_ARRAY( PushOutput, RMCPacket, MessageOut )

  DRIVE( ProcessMessages )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT RMCConnector
