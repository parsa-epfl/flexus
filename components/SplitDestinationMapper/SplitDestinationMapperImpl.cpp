#include <components/SplitDestinationMapper/SplitDestinationMapper.hpp>

#include <core/types.hpp>
#include <core/stats.hpp>

#include <core/qemu/mai_api.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include <fstream>

#include <components/CommonQEMU/Util.hpp>

using nCommonUtil::log_base2;

#include <stdlib.h> // for random()
#include <math.h>

#define DBG_DefineCategories SplitDestinationMapper
#define DBG_SetDefaultOps AddCat(SplitDestinationMapper) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT SplitDestinationMapper
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define MACHINE_BITS 8

namespace nSplitDestinationMapper {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

#define NUM_VC 3
#define NUM_PORTS 3

#define REPLY_VC 0
#define SNOOP_VC 1
#define REQUEST_VC 2

//ALEX
//#define REPLY_VC 1
//#define SNOOP_VC 2
//#define REQUEST_VC 0

#define CACHE_PORT 0
#define DIR_PORT 1
#define MEM_PORT 2

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(SplitDestinationMapper) {
  FLEXUS_COMPONENT_IMPL( SplitDestinationMapper );

  std::vector<int> theDirIndexMap;
  std::vector<int> theMemIndexMap;

  std::vector<int> theDirReverseMap;
  std::vector<int> theMemReverseMap;

  std::vector<int> theCPUMap;
  std::vector<int> theReverseMap;

  int32_t theTotalNumCores;
  int32_t theMemShift;
  int32_t theMemXORShift;
  int32_t theMemMask;
  int32_t theDirShift;
  int32_t theDirXORShift;
  int32_t theDirMask;

  uint32_t MCsPerMachine, theMemMaskMultinode, theDirMaskMultinode, tilesPerMachine;	//for multinode

  bool the2PhaseWB;

  uint32_t numCols, numRows, coordX, coordY, tileCount;	//ALEX

  enum DirLocation_t {
    eDistributed,
    eAtMemory
  } theDirLoc;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(SplitDestinationMapper)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void finalize( void ) {
  }

  void initialize(void) {
    theTotalNumCores = (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth());

    DBG_Assert( (cfg.MemControllers & (cfg.MemControllers - 1)) == 0);
    DBG_Assert( (cfg.Directories & (cfg.Directories - 1)) == 0);
    DBG_Assert( (cfg.MemInterleaving & (cfg.MemInterleaving - 1)) == 0);
    DBG_Assert( (cfg.DirInterleaving & (cfg.DirInterleaving - 1)) == 0);

    theMemShift = log_base2(cfg.MemInterleaving);
    theMemXORShift = cfg.MemXORShift;
    theMemMask = cfg.MemControllers - 1;

    theDirShift = log_base2(cfg.DirInterleaving);
    theDirXORShift = cfg.DirXORShift;
    theDirMask = cfg.Directories - 1;

    DBG_(Crit, ( << "Creating SplitDestinationMapper with " << theTotalNumCores << " cores, " << cfg.Directories << " directories, and " << cfg.MemControllers << " memory controllers." ));
    DBG_Assert(cfg.Banks <= cfg.Directories, ( << "Number of banks = " << cfg.Banks << " Did you initialize the number of banks in the specs file???"));

	numCols = (uint32_t)ceil(sqrt(theTotalNumCores/cfg.MachineCount));
	numRows = numCols;
	tileCount = theTotalNumCores;
	DBG_Assert(theTotalNumCores == cfg.Directories, ( << "ALEX: For YX routing, assumed equal number of tiles and directories"));

	//WARNING: Alternative routing schemes other than default XY not verified with multinode
	DBG_Assert(numRows * numCols * cfg.MachineCount == tileCount, ( << "ALEX: For YX routing, assumed chip with perfect square of tiles per machine."));

	tilesPerMachine = tileCount / cfg.MachineCount;
	DBG_Assert(tilesPerMachine == numRows * numCols);
	theDirMaskMultinode = (cfg.Directories/cfg.MachineCount) - 1;
	theMemMaskMultinode = (cfg.MemControllers/cfg.MachineCount) - 1;
	MCsPerMachine = cfg.MemControllers / cfg.MachineCount;
	DBG_Assert( MCsPerMachine * cfg.MachineCount == (uint32_t)cfg.MemControllers);

    // Determine where directory is located
    if (strcasecmp(cfg.DirLocation.c_str(), "distributed") == 0) {
      theDirLoc = eDistributed;
    } else if (strcasecmp(cfg.DirLocation.c_str(), "AtMemory") == 0) {
      theDirLoc = eAtMemory;
    } else {
      DBG_Assert(false, ( << "Unknown Directory Location '" << cfg.DirLocation << "'" ));
    }

    // Map Memory Controllers to nodes
    std::string mem_loc_str = cfg.MemLocation;
    std::list<int> mem_loc_list;
    std::string::size_type loc;
    do {
      loc = mem_loc_str.find(',', 0);
      if (loc != std::string::npos) {
        std::string cur_loc = mem_loc_str.substr(0, loc);
        mem_loc_str = mem_loc_str.substr(loc + 1);
        mem_loc_list.push_back(boost::lexical_cast<int>(cur_loc));
      }
    } while (loc != std::string::npos);
    if (mem_loc_str.length() > 0) {
      mem_loc_list.push_back(boost::lexical_cast<int>(mem_loc_str));
    }
    DBG_Assert((int)mem_loc_list.size() == cfg.MemControllers,
               ( << "Configuration specifies " << cfg.MemControllers
                 << " memory controllers, but locations given for " << mem_loc_list.size() ));

    theMemIndexMap.resize(cfg.MemControllers, -1);
    theMemReverseMap.resize(theTotalNumCores, -1);
    theDirIndexMap.resize(cfg.Directories, -1);
    theDirReverseMap.resize(theTotalNumCores, -1);

    // Mem Index Map: flexus index of memory controller -> absolute network port index
    // Dir Index Map: flexus index of memory controller -> absolute network port index

    for (int32_t i = 0; i < cfg.MemControllers; i++) {
      int32_t loc = mem_loc_list.front();
      mem_loc_list.pop_front();
      theMemIndexMap[i] = loc + (MEM_PORT * theTotalNumCores);
      theMemReverseMap[loc] = i;
      if (theDirLoc == eAtMemory) {
        theDirIndexMap[i] = loc + (DIR_PORT * theTotalNumCores);
        theDirReverseMap[loc] = i;
      }
    }

    if (theDirLoc == eDistributed) {
      DBG_Assert(cfg.Directories == theTotalNumCores);
      for (int32_t i = 0; i < cfg.Directories; i++) {
        theDirIndexMap[i] = i + (DIR_PORT * theTotalNumCores);
        theDirReverseMap[i] = i;
      }
    }

    the2PhaseWB = cfg.TwoPhaseWB;
  }

//ALEX - begin
  bool isOnFirstCol(uint32_t nodeID) {
	 uint32_t tileID = nodeID % tilesPerMachine;
	 if (tileID % numCols == 0) return true;
	 return false;
  }

  bool isOnLastCol(uint32_t nodeID) {
	 uint32_t tileID = nodeID % tilesPerMachine;
	 if (tileID % numCols == (numCols-1)) return true;
	 return false;
  }

  bool isOnFirstRow(uint32_t nodeID) {
	  uint32_t tileID = nodeID % tilesPerMachine;
	 if (tileID / numRows == 0) return true;
	 return false;
  }

  bool isOnLastRow(uint32_t nodeID) {
	  uint32_t tileID = nodeID % tilesPerMachine;
	 if (tileID / numRows == (numRows-1)) return true;
	 return false;
  }
//ALEX - end

  bool available( interface::ICacheRequestIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index).available();
  }

  void push( interface::ICacheRequestIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
      aMessage[DestinationTag]->requester = getICacheId(anIndex);
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    } else if (aMessage[DestinationTag]->directory == -1) {
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->requester == -1) {
      aMessage[DestinationTag]->requester = getICacheId(anIndex);
    }
    aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from ICacheRequest!" << *aMessage[DestinationTag]  ));

    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REQUEST_VC));
    aMessage[NetworkMessageTag]->src_port = 1;

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Request from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index) << aMessage;
  }

  bool available( interface::ICacheSnoopIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index).available();
  }

  void push( interface::ICacheSnoopIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
      aMessage[DestinationTag]->requester = getICacheId(anIndex);
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
      aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
    } else if (aMessage[DestinationTag]->memory == -1) {
      // Make sure we have the right memory location
      aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->directory == -1) {
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->requester == -1) {
      aMessage[DestinationTag]->requester = getICacheId(anIndex);
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from ICacheSnoop!" << *aMessage[DestinationTag]  ));
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, SNOOP_VC));
    aMessage[NetworkMessageTag]->src_port = 1;

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Snoop from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index) << aMessage;
  }

  bool available( interface::ICacheReplyIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index).available();
  }

  void push( interface::ICacheReplyIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      DBG_Assert(aMessage[MemoryMessageTag]->isEvictType() || aMessage[MemoryMessageTag]->isAckType() || aMessage[MemoryMessageTag]->type() == MemoryMessage::ProtocolMessage, ( << "ICache sent reply with no DestinationMessage: " << *aMessage[MemoryMessageTag] ));
      aMessage.set(DestinationTag, buildReplyDestination(aMessage[MemoryMessageTag], getICacheId(anIndex)));
      DBG_(Verb, ( << "Received cache reply: " << *aMessage[MemoryMessageTag] << " - NEW Destination: " << *aMessage[DestinationTag] ));
    } else {
      if (aMessage[DestinationTag]->directory == -1) {
        aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
      }

      adjustReplyDestination(aMessage);
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from ICacheReply!"  << *aMessage[DestinationTag] ));
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REPLY_VC));
    aMessage[NetworkMessageTag]->src_port = 1;

    DBG_(Verb, ( << "Sending cache reply: " << *aMessage[MemoryMessageTag] << " - " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Reply from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC1, nic_index) << aMessage;
  }

  bool available( interface::CacheRequestIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::CacheRequestIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
      aMessage[DestinationTag]->requester = getDCacheId(anIndex);
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    } else if (aMessage[DestinationTag]->directory == -1) {
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->requester == -1) {
      aMessage[DestinationTag]->requester = getDCacheId(anIndex);
    }
    aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from CacheRequest!" << *aMessage[DestinationTag]  ));

    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REQUEST_VC));
    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_Assert(aMessage[NetworkMessageTag]->dest < 2*cfg.Cores);	//ALEX
    DBG_(Trace, ( << "Received Request from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::CacheSnoopIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::CacheSnoopIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
      aMessage[DestinationTag]->requester = getDCacheId(anIndex);
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
      aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
    } else if (aMessage[DestinationTag]->memory == -1) {
      // Make sure we have the right memory location
      aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->directory == -1) {
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    }
    if (aMessage[DestinationTag]->requester == -1) {
      aMessage[DestinationTag]->requester = getDCacheId(anIndex);
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from CacheSnoop!" << *aMessage[DestinationTag]  ));
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, SNOOP_VC));

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Snoop from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::CacheReplyIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::CacheReplyIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      DBG_Assert(aMessage[MemoryMessageTag]->isEvictType() || aMessage[MemoryMessageTag]->isAckType() || aMessage[MemoryMessageTag]->type() == MemoryMessage::ProtocolMessage, ( << "Cache sent reply with no DestinationMessage: " << *aMessage[MemoryMessageTag] ));
      aMessage.set(DestinationTag, buildReplyDestination(aMessage[MemoryMessageTag], getDCacheId(anIndex)));
      DBG_(Verb, ( << "Received cache reply: " << *aMessage[MemoryMessageTag] << " - NEW Destination: " << *aMessage[DestinationTag] ));
    } else {
      if (aMessage[DestinationTag]->directory == -1) {
        aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
      }

      adjustReplyDestination(aMessage);
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from CacheReply!"  << *aMessage[DestinationTag] ));
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REPLY_VC));

    DBG_(Verb, ( << "Sending cache reply: " << *aMessage[MemoryMessageTag] << " - " << *aMessage[DestinationTag] ));

    DBG_(Trace, ( << "Received Reply from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::DirRequestIn const &,
                  index_t anIndex ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + REQUEST_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::DirRequestIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + REQUEST_VC;

    DBG_Assert(aMessage[DestinationTag] != nullptr);

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from DirRequest!"  << *aMessage[DestinationTag] ));
    // Setup the network message
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, dirIndex2NodeIndex(anIndex), REQUEST_VC));
    //ALEX - begin
    aMessage[NetworkMessageTag]->routeYX = true;
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || isOnLastRow(aMessage[NetworkMessageTag]->dest)) && !(isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
		DBG_(Verb, ( << "Dir sending msg to dest " << aMessage[NetworkMessageTag]->dest << ". Setting routeYX to false"));
		aMessage[NetworkMessageTag]->routeYX = false;
	}
	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || (isOnLastRow(aMessage[NetworkMessageTag]->dest) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		}
	#endif
	//ALEX - end

	/*if (aMessage[NetworkMessageTag]->dest >= 2*cfg.Cores || aMessage[NetworkMessageTag]->src >= 2*cfg.Cores) {	//ALEX
			DBG_(Tmp, ( << "!!! Received Request from Dir " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag]
						<< ". Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
	}*/

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Request from Directory " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::DirSnoopIn const &,
                  index_t anIndex ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + SNOOP_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::DirSnoopIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + SNOOP_VC;

    DBG_Assert(aMessage[DestinationTag] != nullptr);

    fixupMemoryLocation(aMessage, anIndex);
    fixupDirectoryLocation(aMessage, anIndex);

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from DirSnoop!"  << *aMessage[DestinationTag] ));
    // Setup the network message
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, dirIndex2NodeIndex(anIndex), SNOOP_VC));
	 //ALEX - begin
    aMessage[NetworkMessageTag]->routeYX = true;
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || isOnLastRow(aMessage[NetworkMessageTag]->dest)) && !(isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
		DBG_(Verb, ( << "Dir sending msg to dest " << aMessage[NetworkMessageTag]->dest << ". Setting routeYX to false"));
		aMessage[NetworkMessageTag]->routeYX = false;
	}
	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || (isOnLastRow(aMessage[NetworkMessageTag]->dest) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		}
	#endif

	//ALEX - end
   // DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));

    /*if (aMessage[NetworkMessageTag]->dest >= 2*cfg.Cores || aMessage[NetworkMessageTag]->src >= 2*cfg.Cores) {	//ALEX
			DBG_(Tmp, ( << "!!! Received Snoop from Dir " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag]
						<< ". Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
	}*/

    DBG_(Trace, ( << "Received Snoop from Directory " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    DBG_(Trace, ( << "\tSerial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::DirReplyIn const &,
                  index_t anIndex ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + REPLY_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::DirReplyIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (dirIndex2NodeIndex(anIndex) * NUM_VC) + REPLY_VC;

    DBG_Assert(aMessage[DestinationTag] != nullptr, ( << "Receive Msg with no destination, MemMsg = " << *(aMessage[MemoryMessageTag]) ));

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from DirReply!"  << *aMessage[DestinationTag] ));
    // Setup the network message
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, dirIndex2NodeIndex(anIndex), REPLY_VC));

	 //ALEX - begin
    aMessage[NetworkMessageTag]->routeYX = true;
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || isOnLastRow(aMessage[NetworkMessageTag]->dest)) && !(isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
		DBG_(Trace, ( << "Dir sending msg to dest " << aMessage[NetworkMessageTag]->dest << ". Setting routeYX to false"));
		aMessage[NetworkMessageTag]->routeYX = false;
	}

	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->dest) || isOnLastCol(aMessage[NetworkMessageTag]->dest))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->dest) || (isOnLastRow(aMessage[NetworkMessageTag]->dest) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		}
	#endif
	//ALEX - end
	/*if (aMessage[NetworkMessageTag]->dest >= 2*cfg.Cores || aMessage[NetworkMessageTag]->src >= 2*cfg.Cores) {	//ALEX
			DBG_(Tmp, ( << "!!! Received Reply from Dir " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag]
						<< ". Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
	}*/
    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Reply from Directory " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::MemoryIn const &,
                  index_t anIndex ) {
    index_t nic_index = (memIndex2NodeIndex(anIndex) * NUM_VC) + REPLY_VC;
    DBG_(Verb, ( << "Checking if MemoryIn[" << anIndex << "] = " << memIndex2NodeIndex(anIndex) << "*" << NUM_VC << "+" << REPLY_VC << " = " << nic_index << " is available" ));
    return FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index).available();
  }

  void push( interface::MemoryIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (memIndex2NodeIndex(anIndex) * NUM_VC) + REPLY_VC;

    DBG_Assert(aMessage[DestinationTag] != nullptr);

    // Need to send the reply either to the Requester or Directory
    if (cfg.MemReplyToDir) {
      aMessage[DestinationTag]->type = DestinationMessage::Directory;
    } else {
      aMessage[DestinationTag]->type = DestinationMessage::Requester;
      if (aMessage[MemoryMessageTag] && cfg.MemAcksNeedData) {
        aMessage[MemoryMessageTag]->ackRequired() = true;
        aMessage[MemoryMessageTag]->ackRequiresData() = true;
      }
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from Memory!"  << *aMessage[DestinationTag] ));
    // Setup the network message
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, memIndex2NodeIndex(anIndex), REPLY_VC));
     //ALEX - begin
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || isOnLastRow(aMessage[NetworkMessageTag]->src)) && !isOnLastCol(aMessage[NetworkMessageTag]->src)) {
		DBG_(Verb, ( << "Memory " << aMessage[NetworkMessageTag]->src << " sending msg. Setting routeYX to true"));
		aMessage[NetworkMessageTag]->routeYX = true;
		//DBG_Assert(!isOnLastCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
	}
	if (!cfg.MachineCount) { 	//ROUTING NOT APPLICABLE TO MULTINODE CURRENTLY
		DBG_Assert(!isOnFirstCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
	}

	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnLastCol(aMessage[NetworkMessageTag]->src))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || (isOnLastRow(aMessage[NetworkMessageTag]->src) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		}
	#endif

	//ALEX - end

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Reply from Memory " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC0, nic_index) << aMessage;
  }

  bool available( interface::FromNIC0 const &,
                  index_t anIndex ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(CacheRequestOut, node).available();
            //DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!"));
            // always return true, we'll catch the error below where we can see what the actual message was
            return true;
            break;
          case SNOOP_VC:
            return FLEXUS_CHANNEL_ARRAY(CacheSnoopOut, node).available();
          case REPLY_VC:
            return FLEXUS_CHANNEL_ARRAY(CacheReplyOut, node).available();
          default:
            DBG_Assert( false, ( << "ERROR! Nic port maps to invalid VC?" ));
            break;

        }
        break;
      }
      case DIR_PORT: {
        int32_t dir_index = theDirReverseMap[node] % cfg.Banks;
        DBG_Assert( dir_index >= 0);
        switch (vc) {
          case REQUEST_VC:
            return FLEXUS_CHANNEL_ARRAY(DirRequestOut, dir_index).available();
          case SNOOP_VC:
            return FLEXUS_CHANNEL_ARRAY(DirSnoopOut, dir_index).available();
          case REPLY_VC:
            return FLEXUS_CHANNEL_ARRAY(DirReplyOut, dir_index).available();
          default:
            DBG_Assert( false, ( << "ERROR! Nic port maps to invalid VC?" ));
            break;

        }
        break;
      }
      case MEM_PORT: {
        int32_t mem_index = theMemReverseMap[node];
        DBG_Assert( mem_index >= 0);
        return FLEXUS_CHANNEL_ARRAY(MemoryOut, mem_index).available();
      }
      default:
        DBG_Assert( false, ( << "ERROR! Nic port maps to invalid port?" ));
        break;
    }

    DBG_Assert( false, ( << "Tried to push message into non-existant port"));
    return false;
  }

  void push( interface::FromNIC0 const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(CacheRequestOut, node).available();
            DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!" << *aMessage[MemoryMessageTag] ));
            break;
          case SNOOP_VC:
            DBG_(Trace, ( << "Received Snoop for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(CacheSnoopOut, node) << aMessage;
            break;
          case REPLY_VC:
            DBG_(Trace, ( << "Received Reply for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(CacheReplyOut, node) << aMessage;
            break;
          default:
            DBG_Assert( false, ( << "Tried to push message into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
            break;

        }
        break;
      }
      case DIR_PORT: {
        int32_t dir_index = theDirReverseMap[node] % cfg.Banks;
        DBG_Assert( dir_index >= 0);
        DBG_Assert( dir_index >= 0, ( << "Tried to send msg to non-existant dir: " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
        switch (vc) {
          case REQUEST_VC:
            DBG_(Trace, ( << "Received Request for Directory " << dir_index << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(DirRequestOut, dir_index) << aMessage;
            break;
          case SNOOP_VC:
            DBG_(Trace, ( << "Received Snoop for Directory " << dir_index << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(DirSnoopOut, dir_index) << aMessage;
            break;
          case REPLY_VC:
            DBG_(Trace, ( << "Received Reply for Directory " << dir_index << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(DirReplyOut, dir_index) << aMessage;
            break;
          default:
            DBG_Assert( false, ( << "Tried to push message into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
            break;

        }
        break;
      }
      case MEM_PORT: {
        int32_t mem_index = theMemReverseMap[node];
        DBG_Assert( mem_index >= 0);
        DBG_(Trace, ( << "Received Request for Memory " << mem_index << " - " << *aMessage[MemoryMessageTag] ));
        FLEXUS_CHANNEL_ARRAY(MemoryOut, mem_index) << aMessage;
        break;
      }
      default:
        DBG_Assert( false, ( << "Tried to push message into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
        break;
    }
  }

  bool available( interface::FromNIC1 const &,
                  index_t anIndex ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(CacheRequestOut, node).available();
            //DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!"));
            // always return true, we'll catch the error below where we can see what the actual message was
            return true;
            break;
          case SNOOP_VC:
            return FLEXUS_CHANNEL_ARRAY(ICacheSnoopOut, node).available();
          case REPLY_VC:
            return FLEXUS_CHANNEL_ARRAY(ICacheReplyOut, node).available();
          default:
            DBG_Assert( false, ( << "ERROR! Nic port maps to invalid VC?" ));
            break;

        }
        break;
      }
      default:
        DBG_Assert( false, ( << "ERROR! Nic 1 port maps to port other than CACHE_PORT" ));
        break;
    }

    DBG_Assert( false, ( << "Tried to push message into non-existant port"));
    return false;
  }

  void push( interface::FromNIC1 const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(ICacheRequestOut, node).available();
            DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!" << *aMessage[MemoryMessageTag] ));
            break;
          case SNOOP_VC:
            DBG_(Trace, ( << "Received Snoop for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(ICacheSnoopOut, node) << aMessage;
            break;
          case REPLY_VC:
            DBG_(Trace, ( << "Received Reply for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(ICacheReplyOut, node) << aMessage;
            break;
          default:
            DBG_Assert( false, ( << "Tried to push message into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
            break;

        }
        break;
      }
      default:
        DBG_Assert( false, ( << "Tried to push message from Nic1 into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
        break;
    }
  }

 //ALEX - begin

  bool available( interface::FromNIC2 const &,
                  index_t anIndex ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(CacheRequestOut, node).available();
            //DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!"));
            // always return true, we'll catch the error below where we can see what the actual message was
            return true;
            break;
          case SNOOP_VC:
            return FLEXUS_CHANNEL_ARRAY(RMCCacheSnoopOut, node).available();
          case REPLY_VC:
            return FLEXUS_CHANNEL_ARRAY(RMCCacheReplyOut, node).available();
          default:
            DBG_Assert( false, ( << "ERROR! Nic port maps to invalid VC?" ));
            break;

        }
        break;
      }
      default:
        DBG_Assert( false, ( << "ERROR! Nic 2 port maps to port other than CACHE_PORT" ));
        break;
    }

    DBG_Assert( false, ( << "Tried to push message into non-existant port"));
    return false;
  }

  void push( interface::FromNIC2 const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    // This NIC can be connected to either a Cache, a Directory, or a MemoryController
    // Each node has 3 ports (for Cache, Dir, Mem)
    // Each port has 3 vc (Request, Reply, Snoop)
    int32_t port = anIndex / (theTotalNumCores * NUM_VC);
    int32_t node = (anIndex / NUM_VC) % theTotalNumCores;
    int32_t vc = anIndex % NUM_VC;

    switch (port) {
      case CACHE_PORT: {
        switch (vc) {
          case REQUEST_VC:
            //return FLEXUS_CHANNEL_ARRAY(ICacheRequestOut, node).available();
            //DBG_Assert( false, ( << "ERROR! Cache received message on Request Virtual Channel!" << *aMessage[MemoryMessageTag] ));
            DBG_Assert( aMessage[MemoryMessageTag]->type() == MemoryMessage::WQMessage, ( << "ERROR! Cache received message on Request Virtual Channel!" << *aMessage[MemoryMessageTag] ));	//ALEX
       		DBG_(SPLIT_RMC_MESSAGES_DEBUG, ( << "RGP backend " << anIndex << " (node " << node << ") received a WQMessage from " << aMessage[NetworkMessageTag]->src
												<< ": " << *aMessage[MemoryMessageTag]));

            FLEXUS_CHANNEL_ARRAY(RMCCacheSnoopOut, node) << aMessage;	//ALEX
            break;
          case SNOOP_VC:
            DBG_(Trace, ( << "Received Snoop for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(RMCCacheSnoopOut, node) << aMessage;
            break;
          case REPLY_VC:
            DBG_(Trace, ( << "Received Reply for cache " << node << " - " << *aMessage[MemoryMessageTag] ));
            FLEXUS_CHANNEL_ARRAY(RMCCacheReplyOut, node) << aMessage;
            break;
          default:
            DBG_Assert( false, ( << "Tried to push message into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
            break;

        }
        break;
      }
      default:
        DBG_Assert( false, ( << "Tried to push message from Nic2 into non-existant port " << anIndex << " (" << port << ", " << node << ", " << vc << ") " << *(aMessage[DestinationTag])));
        break;
    }
  }

  bool available( interface::RMCCacheRequestIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index).available();
  }

  void push( interface::RMCCacheRequestIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REQUEST_VC;

    //ALEX - Need special handling for the case of WQMessages
    if (aMessage[MemoryMessageTag]->type() == MemoryMessage::WQMessage) {
		DBG_Assert( (!aMessage[DestinationTag]) );
		NetworkMessage_p ret( new NetworkMessage());
		ret->src = anIndex;
		ret->src_port = 2;
		ret->dst_port = 2;
		ret->vc = REQUEST_VC;
		ret->size = aMessage[MemoryMessageTag]->messageSize();
	//README: This is the most important part. SDM decides which backend to send the WQMessage to.
	//Here I assume that the RGP backends sit at the first column, and all the  frontends of each row
	//are mapped to that row's backend.
	    if (aMessage[MemoryMessageTag]->target_backend != -1) //special case for specific FE to BE steering specified in RMC code
            ret->dest = aMessage[MemoryMessageTag]->target_backend;
        else
		    ret->dest = (anIndex/numRows) * numRows;

		DBG_(SPLIT_RMC_MESSAGES_DEBUG, ( << "Received a WQMessage from RGP frontend " << anIndex << ". Sending to RGP backend " << ret->dest
											<< ": " << *aMessage[MemoryMessageTag]));

		aMessage.set( NetworkMessageTag, ret);
	} else {
		//Need to setup the destination message if not there, as well as the network message
		if (!aMessage[DestinationTag]) {
		  aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
		  aMessage[DestinationTag]->requester = getRMCCacheId(anIndex);
		  aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
		} else if (aMessage[DestinationTag]->directory == -1) {
		  aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
		}
		if (aMessage[DestinationTag]->requester == -1) {
		  aMessage[DestinationTag]->requester = getRMCCacheId(anIndex);
		}
		aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());

		DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from RMCCacheRequest!" << *aMessage[DestinationTag]  ));

		aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REQUEST_VC));
		aMessage[NetworkMessageTag]->src_port = 2;
    }
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || isOnLastRow(aMessage[NetworkMessageTag]->src)) && !isOnFirstCol(aMessage[NetworkMessageTag]->src)) {
		DBG_(Verb, ( << "RMC " << aMessage[NetworkMessageTag]->src << " sending msg. Setting routeYX to true"));
		aMessage[NetworkMessageTag]->routeYX = true;
		//DBG_Assert(!isOnFirstCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
	}
	//DBG_Assert(!isOnLastCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement

	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->src))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || (isOnLastRow(aMessage[NetworkMessageTag]->src) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		}
	#endif

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Request from RMCcache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));
    FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index) << aMessage;
  }

  bool available( interface::RMCCacheSnoopIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index).available();
  }

  void push( interface::RMCCacheSnoopIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + SNOOP_VC;

	//ALEX - Need special handling for the case of CQMessages
	 if (aMessage[MemoryMessageTag]->type() == MemoryMessage::CQMessage || aMessage[MemoryMessageTag]->type() == MemoryMessage::RRPPFwd) {
		DBG_Assert( (!aMessage[DestinationTag]) );
		NetworkMessage_p ret( new NetworkMessage());
		ret->src = anIndex;
		ret->src_port = 2;
		ret->dst_port = 2;
		ret->vc = SNOOP_VC;
		ret->size = aMessage[MemoryMessageTag]->messageSize();
	//README: This is the most important part. SDM decides which RCP backend to send the WQMessage to.
	//Here I assume that there is an RCP backend per tile, and the QP id of the MemoryMessage matches the core (tile) id
		ret->dest = aMessage[MemoryMessageTag]->QP_id;

		aMessage.set( NetworkMessageTag, ret);
	} else {
    //Need to setup the destination message if not there, as well as the network message
		if (!aMessage[DestinationTag]) {
		  aMessage.set(DestinationTag, new DestinationMessage(DestinationMessage::Directory));
		  aMessage[DestinationTag]->requester = getRMCCacheId(anIndex);
		  aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
		  aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
		} else if (aMessage[DestinationTag]->memory == -1) {
		  // Make sure we have the right memory location
		  aMessage[DestinationTag]->memory    = getMemoryLocation(aMessage[MemoryMessageTag]->address());
		}
		if (aMessage[DestinationTag]->directory == -1) {
		  aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
		}
		if (aMessage[DestinationTag]->requester == -1) {
		  aMessage[DestinationTag]->requester = getRMCCacheId(anIndex);
		}
		DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from RMCCacheSnoop!" << *aMessage[DestinationTag]  ));
	    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, SNOOP_VC));
		aMessage[NetworkMessageTag]->src_port = 2;
    }
    if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || isOnLastRow(aMessage[NetworkMessageTag]->src)) && !isOnFirstCol(aMessage[NetworkMessageTag]->src)) {
		DBG_(Verb, ( << "RMC " << aMessage[NetworkMessageTag]->src << " sending msg. Setting routeYX to true"));
		aMessage[NetworkMessageTag]->routeYX = true;
		//DBG_Assert(!isOnFirstCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
	}
	DBG_Assert(!isOnLastCol(aMessage[NetworkMessageTag]->src) || isOnFirstCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
                    //second condition needed for single-core chips

	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->src))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || (isOnLastRow(aMessage[NetworkMessageTag]->src) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		}
	#endif

    DBG_(Verb, ( << "Serial: " << aMessage[MemoryMessageTag]->serial() << ", DEST: " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Snoop from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index) << aMessage;
  }

  bool available( interface::RMCCacheReplyIn const &,
                  index_t anIndex ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;
    return FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index).available();
  }

  void push( interface::RMCCacheReplyIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    index_t nic_index = (anIndex * NUM_VC) + REPLY_VC;

    //Need to setup the destination message if not there, as well as the network message
    if (!aMessage[DestinationTag]) {
      DBG_Assert(aMessage[MemoryMessageTag]->isEvictType() || aMessage[MemoryMessageTag]->isAckType() || aMessage[MemoryMessageTag]->type() == MemoryMessage::ProtocolMessage, ( << "RMCCache sent reply with no DestinationMessage: " << *aMessage[MemoryMessageTag] ));
      aMessage.set(DestinationTag, buildReplyDestination(aMessage[MemoryMessageTag], getRMCCacheId(anIndex)));
      DBG_(Verb, ( << "Received cache reply: " << *aMessage[MemoryMessageTag] << " - NEW Destination: " << *aMessage[DestinationTag] ));
    } else {
      if (aMessage[DestinationTag]->directory == -1) {
        aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
      }

      adjustReplyDestination(aMessage);
    }

    DBG_Assert(aMessage[DestinationTag]->type != DestinationMessage::Multicast, ( << "Received Multicast message from ICacheReply!"  << *aMessage[DestinationTag] ));
    aMessage.set( NetworkMessageTag, buildNetworkMessage(aMessage, anIndex, REPLY_VC));
    aMessage[NetworkMessageTag]->src_port = 2;

    if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || isOnLastRow(aMessage[NetworkMessageTag]->src)) && !isOnFirstCol(aMessage[NetworkMessageTag]->src)) {
		DBG_(Verb, ( << "RMC " << aMessage[NetworkMessageTag]->src << " sending msg. Setting routeYX to true"));
		aMessage[NetworkMessageTag]->routeYX = true;
		//DBG_Assert(!isOnFirstCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement
	}
	//DBG_Assert(!isOnLastCol(aMessage[NetworkMessageTag]->src));	//Sanity check for current placement

	#ifdef WEIGHTED_ROUTING
		int prob_edge = rand()%cfg.EdgeInterfaces;
		if ((isOnFirstCol(aMessage[NetworkMessageTag]->src))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = true;
			} else {
				aMessage[NetworkMessageTag]->routeYX = false;
			}
		} else if ((isOnFirstRow(aMessage[NetworkMessageTag]->src) || (isOnLastRow(aMessage[NetworkMessageTag]->src) ))) {
			if (!prob_edge) {
				aMessage[NetworkMessageTag]->routeYX = false;
			} else {
				aMessage[NetworkMessageTag]->routeYX = true;
			}
		}
	#endif

    DBG_(Verb, ( << "Sending cache reply: " << *aMessage[MemoryMessageTag] << " - " << *aMessage[DestinationTag] ));
    DBG_(Trace, ( << "Received Reply from cache " << anIndex << " sending to network port " << nic_index << " - " << *aMessage[MemoryMessageTag] ));

    FLEXUS_CHANNEL_ARRAY(ToNIC2, nic_index) << aMessage;
  }

 //ALEX - end

private:

  void saveState(std::string const & aDirName) { }

  void loadState( std::string const & aDirName ) { }

  inline int32_t dirIndex2NodeIndex(int32_t index) {
    return theDirIndexMap[index];
  }

  inline int32_t memIndex2NodeIndex(int32_t index) {
    return theMemIndexMap[index];
  }

  inline int32_t getDCacheId(index_t i) {
	return (i << 1);
  }

  inline int32_t getICacheId(index_t i) {
	return ((i << 1) + 1);
  }

  inline int32_t getRMCCacheId(index_t i) {
	  DBG_(Trace, ( << "getRMCCacheId called with index_t = " << i << ". Return value = " << ((cfg.Cores + i) << 1) ));
	return ((cfg.Cores + i) << 1);		//ALEX - This looks weird, but it's like this because of the Trace simulation, where each RMC is attached to
										//a common L1d cache, i.e., to L1d caches with ids Cores+i. So the mapping for the directory is similar to the
										//normal L1d caches, we just need to add the "Cores" offset first
  }

  inline int32_t getDirectoryLocation(const PhysicalMemoryAddress & anAddress) {
	if (cfg.MachineCount > 1) { //for multinode
		uint32_t machine_id = (anAddress >> (64 - MACHINE_BITS));
		DBG_Assert(machine_id < cfg.MachineCount);
        uint32_t dir_base = machine_id * tilesPerMachine;
		uint32_t dir_offset = (anAddress >> theDirShift) & theDirMaskMultinode;
		DBG_Assert(dir_offset < tilesPerMachine);
		uint32_t target_directory = dir_base + dir_offset;
		return (int32_t)target_directory;
	} else {   //for single node
		if (theDirXORShift > 0) {
		  return ((anAddress >> theDirShift) ^ (anAddress >> theDirXORShift)) & theDirMask;
		} else {
		  return ((anAddress >> theDirShift) & theDirMask) % cfg.Banks;
		}
	}
  }

  inline int32_t getMemoryLocation(const PhysicalMemoryAddress & anAddress) {
	if (cfg.MachineCount > 1) { //for multinode
		uint32_t MCbase = (anAddress >> (64 - MACHINE_BITS)) * MCsPerMachine;
		DBG_Assert(MCbase < (uint32_t)cfg.MemControllers);
		uint32_t MC_offset = (anAddress >> theMemShift) & theMemMaskMultinode;
		DBG_Assert(MC_offset < MCsPerMachine);
		uint32_t target_MC = MCbase + MC_offset;
		return (int32_t)target_MC;
	} else {   //for single node
		if (theMemXORShift > 0) {
		  return ((anAddress >> theMemShift) ^ (anAddress >> theMemXORShift)) & theMemMask;
		} else {
		  return (anAddress >> theMemShift) & theMemMask;
		}
	}
  }

  typedef boost::intrusive_ptr<NetworkMessage>  NetworkMessage_p;
  typedef boost::intrusive_ptr<MemoryMessage>   MemoryMessage_p;
  typedef boost::intrusive_ptr<DestinationMessage> DestinationMessage_p;

  NetworkMessage_p buildNetworkMessage(MemoryTransport & aMessage, int32_t aSrc, int32_t aVC) {
    NetworkMessage_p ret( new NetworkMessage());
    ret->src = aSrc;
    ret->src_port = 0;
    ret->dst_port = 0;
    ret->vc = aVC;
    ret->size = aMessage[MemoryMessageTag]->messageSize();
    if (aMessage[TaglessDirMsgTag] != NULL) {
      ret->size += aMessage[TaglessDirMsgTag]->messageSize();
    }
    DBG_Assert(ret->size >= 0, ( << "Couldn't calculate size for message: " << *(aMessage[MemoryMessageTag]) ));

    switch (aMessage[DestinationTag]->type) {
      case DestinationMessage::Requester:
		//DBG_(Tmp, ( << "\tmsgType: DestinationMessage::Requester. Dest = " << aMessage[DestinationTag]->requester << " (requester):"  << *(aMessage[MemoryMessageTag])));
        ret->dest = aMessage[DestinationTag]->requester >> 1;
        ret->dst_port = aMessage[DestinationTag]->requester & 1;
        //ALEX - begin
        if (aMessage[DestinationTag]->requester >= cfg.Cores * 2) {
			ret->dest -= cfg.Cores;
			ret->dst_port = 2;
			DBG_(Trace, ( << "Received packet with requester id " << aMessage[DestinationTag]->requester << ". Changed to " << ret->dest << " with dst_port " << ret->dst_port));
		}
        //ALEX - end
        DBG_(Trace, ( << "Dest = " << ret->dest << " (requester):"  << *(aMessage[MemoryMessageTag])));
        //DBG_(Tmp, ( << "\tNEW: Dest = " << ret->dest << " (requester):"  << *(aMessage[MemoryMessageTag])));
        break;
      case DestinationMessage::Directory:
        if (cfg.LocalDir && aMessage[DestinationTag]->requester >= 0) {
          ret->dest = dirIndex2NodeIndex(aMessage[DestinationTag]->requester >> 1);
          DBG_(Trace, ( << "Dest = " << aMessage[DestinationTag]->requester << " (dirIndex2NodeIndex(" << aMessage[DestinationTag]->requester << ")):"  << *(aMessage[MemoryMessageTag])));
        } else {
		  //DBG_(Tmp, ( << "\tmsgType: DestinationMessage::Directory. Dest = " << ret->dest << " (requester):"  << *(aMessage[MemoryMessageTag])));
          DBG_Assert(aMessage[DestinationTag]->directory >= 0);
          ret->dest = dirIndex2NodeIndex(aMessage[DestinationTag]->directory);
          DBG_(Trace, ( << "Dest = " << ret->dest << " (dirIndex2NodeIndex(" << aMessage[DestinationTag]->directory << ")):"  << *(aMessage[MemoryMessageTag])));
          //DBG_(Tmp, ( << "\tNEW: Dest = " << ret->dest << " (dirIndex2NodeIndex(" << aMessage[DestinationTag]->directory << ")):"  << *(aMessage[MemoryMessageTag])));
        }
        break;
      case DestinationMessage::RemoteDirectory:
        ret->dest = dirIndex2NodeIndex(aMessage[DestinationTag]->remote_directory);
        break;
      case DestinationMessage::Memory:
        ret->dest = memIndex2NodeIndex(aMessage[DestinationTag]->memory);
        break;
      case DestinationMessage::Source:
        ret->dest = aMessage[DestinationTag]->source >> 1;
        ret->dst_port = aMessage[DestinationTag]->source & 1;
        //ALEX - begin
        if (aMessage[DestinationTag]->source >= cfg.Cores * 2) {
			ret->dest -= cfg.Cores;
			ret->dst_port = 2;
			DBG_(Trace, ( << "Received packet with source id " << aMessage[DestinationTag]->source << ". Set dest to " << ret->dest << " and dst_port to " << ret->dst_port));
		}
        //ALEX - end
        break;
      case DestinationMessage::Other:
        // Multicast uses "Other" for individual destinations
        // Multicast is set up by the directory which tracks I&D caches separately
        ret->dest = aMessage[DestinationTag]->other >> 1;
        ret->dst_port = aMessage[DestinationTag]->other & 1;
        //ALEX - begin
        if (aMessage[DestinationTag]->other >= cfg.Cores * 2) {
			ret->dest -= cfg.Cores;
			ret->dst_port = 2;
			DBG_(Trace, ( << "Received packet with \"other\" id " << aMessage[DestinationTag]->other << ". Set dest to " << ret->dest << " and dst_port to " << ret->dst_port));
		}
        //ALEX - end
        break;
      default:
        DBG_Assert( false, ( << "Invalid DestinationType " << aMessage[DestinationTag]->type ));
        break;
    }

    return ret;
  }

  DestinationMessage_p buildReplyDestination(MemoryMessage_p msg, index_t node) {
    // We got a cache reply without a Destination Message
    // This must be an Evict message, send it to the appropriate directory
    DestinationMessage_p ret(new DestinationMessage(DestinationMessage::Directory));
    ret->requester = node;
    ret->directory = getDirectoryLocation(msg->address());
    ret->memory = getMemoryLocation(msg->address());
    return ret;
  }

  void adjustReplyDestination(MemoryTransport & aMessage) {
    // This is a reply message from the cache
    // FwdAck, FwdNAck, Evict go to Dir
    // FwdReply goes to requester
    // Inval Ack -> goes to either Dir or Requester
    switch (aMessage[MemoryMessageTag]->type()) {
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::ReadAck:
      case MemoryMessage::FetchAck:
      case MemoryMessage::ReadAckDirty:
      case MemoryMessage::FetchAckDirty:
      case MemoryMessage::WriteAck:
      case MemoryMessage::UpgradeAck:
      case MemoryMessage::NASAck:
      case MemoryMessage::WriteRetry:
      case MemoryMessage::StoreDataACK:	//ALEX
      case MemoryMessage::LoadDataACK:	//ALEX
        aMessage[DestinationTag]->type = DestinationMessage::Directory;
        break;
      case MemoryMessage::FwdNAck:
        if (!the2PhaseWB) {
          aMessage[DestinationTag]->type = DestinationMessage::Directory;
        }
        break;
      case MemoryMessage::FwdReply:
      case MemoryMessage::FwdReplyOwned:
      case MemoryMessage::FwdReplyWritable:
      case MemoryMessage::FwdReplyDirty:
      case MemoryMessage::LoadDataReply:	//ALEX
        aMessage[DestinationTag]->type = DestinationMessage::Requester;
        break;
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvalidateNAck:
      case MemoryMessage::InvUpdateAck:
        if (aMessage[DestinationTag]->requester == -1) {
          aMessage[DestinationTag]->type = DestinationMessage::Directory;
        } else {
          aMessage[DestinationTag]->type = DestinationMessage::Requester;
        }
        break;
      case MemoryMessage::ProtocolMessage:
        // Pass-through component should have determined the correct destination
        if (aMessage[DestinationTag]->type == DestinationMessage::Requester) {
          aMessage[DestinationTag]->type = DestinationMessage::Directory;
        }
        break;
      default:
        DBG_Assert(false, ( << "Unexpected type of reply message: " << aMessage[MemoryMessageTag]->type() ));
        break;
    }
  }

  void fixupMemoryLocation(MemoryTransport & aMessage, index_t anIndex) {
    aMessage[DestinationTag]->memory = getMemoryLocation(aMessage[MemoryMessageTag]->address());
  }
  void fixupDirectoryLocation(MemoryTransport & aMessage, index_t anIndex) {
    if (aMessage[DestinationTag]->directory == -1) {
      aMessage[DestinationTag]->directory = getDirectoryLocation(aMessage[MemoryMessageTag]->address());
    }
  }

};  // end class SplitDestinationMapper

}  // end Namespace nSplitDestinationMapper

FLEXUS_COMPONENT_INSTANTIATOR( SplitDestinationMapper, nSplitDestinationMapper);

FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirReplyIn )   {
  return cfg.Directories;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirSnoopIn )   {
  return cfg.Directories;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirRequestIn )  {
  return cfg.Directories;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirReplyOut )  {
  return cfg.Directories;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirSnoopOut )  {
  return cfg.Directories;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, DirRequestOut )  {
  return cfg.Directories;
}

FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, CacheSnoopOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, CacheReplyOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, CacheRequestIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, CacheSnoopIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, CacheReplyIn )  {
  return cfg.Cores;
}
//ALEX - begin
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, RMCCacheSnoopOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, RMCCacheReplyOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, RMCCacheRequestIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, RMCCacheSnoopIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, RMCCacheReplyIn )  {
  return cfg.Cores;
}
//ALEX- end

FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ICacheSnoopOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ICacheReplyOut )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ICacheRequestIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ICacheSnoopIn )  {
  return cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ICacheReplyIn )  {
  return cfg.Cores;
}

FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, FromNIC0 )   {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ToNIC0 )    {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, FromNIC1 )   {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ToNIC1 )    {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}

//ALEX -begin
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, FromNIC2 )   {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, ToNIC2 )    {
  return NUM_VC * NUM_PORTS * cfg.Cores;
}
//ALEX - end


FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, MemoryIn )   {
  return cfg.MemControllers;
}
FLEXUS_PORT_ARRAY_WIDTH( SplitDestinationMapper, MemoryOut )   {
  return cfg.MemControllers;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SplitDestinationMapper

#define DBG_Reset
#include DBG_Control()
