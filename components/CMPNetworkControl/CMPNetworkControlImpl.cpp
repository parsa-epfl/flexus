#include <components/CMPNetworkControl/CMPNetworkControl.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/CachePlacementPolicyDefn.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

using namespace boost::multi_index;

#include <boost/bind.hpp>
#define _BACKWARD_BACKWARD_WARNING_H
#include <ext/hash_map>

#include <fstream>

#include <stdlib.h> // for random()

#define FLEXUS_BEGIN_COMPONENT CMPNetworkControl
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories CMPNetworkControl
#define DBG_SetDefaultOps AddCat(CMPNetworkControl)
#include DBG_Control()

#define L2_TILE     0
#define CORE_TILE   1
#define MAX_CORES  64

namespace nCMPNetworkControl {

using namespace Flexus;
using namespace Flexus::SharedTypes;
using namespace Core;

typedef boost::intrusive_ptr<MemoryMessage>      MemoryMessage_p;
typedef boost::intrusive_ptr<NetworkMessage>     NetworkMessage_p;
typedef boost::intrusive_ptr<TransactionTracker> TransactionTracker_p;

typedef uint64_t tAddress;

static const uint32_t TILE_ID_UNDEF = ( 1ULL << (sizeof(uint32_t) * 8) ) - 1;

static uint32_t thePageSizeLog2;       // log2 (page size)
static uint32_t theCacheLineSizeLog2;  // log2 (cache line size)

class FLEXUS_COMPONENT(CMPNetworkControl) {
  FLEXUS_COMPONENT_IMPL(CMPNetworkControl);

private:
  uint32_t theNumCores;           // # cores
  uint32_t theNumL2Slices;        // # L2 Slices
  uint32_t theNumL2SlicesLog2;    // log2(# L2 Slices)
  uint32_t theNumCoresPerL2Slice; // # cores per L2 slice (for hierarchical designs)
  uint32_t theNumMemControllers;  // # memory controllers

  uint32_t theNumVChannels;       // # VCs

  tAddress     theCacheLineSize;      // cache line size
  tAddress     theCacheLineMask;      // mask out the cache line offset
  tAddress     thePageSize;           // page size
  tAddress     thePageMask;           // mask out the page offset

  uint32_t theInterleavingBlockShiftBits; // interleaving granularity

  uint32_t theCoreNodeNumber[MAX_CORES];  // node numbers for the network -- cores
  uint32_t theL2NodeNumber[MAX_CORES];    // node numbers for the network -- L2 slices
  uint32_t theTileNumber[MAX_CORES*2];    // each tile has a core and an L2
  bool         isL2Node[MAX_CORES*2];         // true if a network node is an L2; false if it is a core

  // 2D torus topology
  uint32_t theNumCoresPerTorusRow;  // the number of cores per torus row
  std::vector<uint32_t> theNTileID; // north tile neighbor in torus
  std::vector<uint32_t> theSTileID; // south tile neighbor in torus
  std::vector<uint32_t> theWTileID; // west tile neighbor in torus
  std::vector<uint32_t> theETileID; // east tile neighbor in torus

  tPlacement thePlacement; // L2 design

  // member function pointer for processing core requests
  typedef boost::function<int32_t ( const index_t anIndex, MemoryTransport & aTransport ) > tGetL2IDForCoreReq_func;
  tGetL2IDForCoreReq_func getL2IDForCoreReq;

  /* CMU-ONLY-BLOCK-BEGIN */
  // R-NUCA design with instruction storage replication
  uint32_t theSizeOfInstrCluster;                     // the cluster size for instruction replication
  uint32_t theSizeOfInstrClusterLog2;
  std::vector<uint32_t> theCoreInstrInterleavingID;   // interleaving IDs for indexing instruction replicas

  uint32_t theSizeOfPrivCluster;                     // the cluster size for private data
  uint32_t theSizeOfPrivClusterLog2;
  std::vector<uint32_t> theCorePrivInterleavingID;   // interleaving IDs for indexing private data
  /* CMU-ONLY-BLOCK-END */

  int32_t theIdxExtraOffsetBitsInstr;
  int32_t theIdxExtraOffsetBitsShared;
  int32_t theIdxExtraOffsetBitsPrivate;

  /* CMU-ONLY-BLOCK-BEGIN */
  std::list< MemoryTransport > theSendQueue;              // queue to send purges from
  std::list< MemoryTransport > theCustomerServiceQueue;   // queue to send requests waiting for purges

  // page hashing function (for use in hash maps)
  struct PageHash {
    std::size_t operator() ( const tAddress anAddr ) const {
      return (anAddr >> thePageSizeLog2);
    }
  };

  // waiting for replies on purges to update the page state
  typedef struct tPendingPurgesMapEntry {
    MemoryTransport trans;
    uint32_t acount;
  } tPendingPurgesMapEntry;

  typedef __gnu_cxx::hash_map <  const tAddress  // block address
  , tPendingPurgesMapEntry  // entry pointer
  , PageHash         // page hashing
  > tPendingPurgesMap;
  tPendingPurgesMap thePendingPurgesMap;
  /* CMU-ONLY-BLOCK-END */

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMPNetworkControl)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) { }

  bool isQuiesced() const {
    return ( true
             && theSendQueue.empty() && thePendingPurgesMap.empty() && theCustomerServiceQueue.empty() /* CMU-ONLY */
           );
  }

  // Initialization
  void initialize() {
    // Confirm there are enough bits for the NumCores and NumL2Slices
    DBG_Assert(sizeof(index_t) * 8 >= log_base2(cfg.NumCores * cfg.NumL2Tiles));
    DBG_Assert(sizeof(uint32_t) * 8 >= log_base2(cfg.NumCores));
    DBG_Assert(sizeof(uint32_t) * 8 >= log_base2(cfg.NumL2Tiles));

    theNumCores = (cfg.NumCores ? cfg.NumCores : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    theNumL2Slices = (cfg.NumL2Tiles ? cfg.NumL2Tiles : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    theNumCoresPerL2Slice = theNumCores / theNumL2Slices;
    theNumL2SlicesLog2 = log_base2(theNumL2Slices);
    theNumMemControllers = cfg.NumMemControllers;

    theNumVChannels = cfg.VChannels;

    // At most 64 cores or L2 slices
    DBG_Assert ( theNumCores <= 64, ( << "This implementation only supports up to 64 cores" ) );
    DBG_Assert ( theNumL2Slices <= 64, ( << "This implementation only supports up to 64 L2 slices" ) );

    // Confirm that PageSize is a power of 2
    DBG_Assert( (cfg.PageSize & (cfg.PageSize - 1)) == 0);
    DBG_Assert( cfg.PageSize  >= 4);

    // Calculate shifts and masks
    thePageSize = cfg.PageSize;
    thePageSizeLog2 = log_base2(cfg.PageSize);
    thePageMask = ~(cfg.PageSize - 1);

    // Confirm that CacheLineSize is a power of 2
    DBG_Assert( (cfg.CacheLineSize & (cfg.CacheLineSize - 1)) == 0);
    DBG_Assert( cfg.CacheLineSize  >= 4);

    // Calculate shifts and masks
    theCacheLineSize = cfg.CacheLineSize;
    theCacheLineSizeLog2 = log_base2(cfg.CacheLineSize);
    theCacheLineMask = ~(cfg.CacheLineSize - 1);

    // L2 design
    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.Placement == "R-NUCA") {
      thePlacement = kRNUCACache;
      getL2IDForCoreReq = boost::bind( &CMPNetworkControlComponent::getL2IDForCoreReq_RNUCA, this, _1, _2);
    } else
      /* CMU-ONLY-BLOCK-END */
      if (cfg.Placement == "private") {
        thePlacement = kPrivateCache;
        getL2IDForCoreReq = boost::bind( &CMPNetworkControlComponent::getL2IDForCoreReq_private, this, _1, _2);
      } else if (cfg.Placement == "shared") {
        thePlacement = kSharedCache;
        getL2IDForCoreReq = boost::bind( &CMPNetworkControlComponent::getL2IDForCoreReq_shared, this, _1, _2);
      } else {
        DBG_Assert(false, ( << "Unknown L2 design") );
      }
    DBG_Assert(theNumCores == theNumL2Slices, ( << "I have no idea what will break if this is not true, but something probably will") );

    // interleaving granularity
    theInterleavingBlockShiftBits = log_base2( cfg.L2InterleavingGranularity );

    DBG_( Crit, ( << "Running with "
                  << theNumCores << " cores"
                  << ", " << theNumL2Slices
                  << " " << cfg.Placement
                  << " L2 slices"
                  << " in " << cfg.Floorplan << " topology"
                  << ", " << cfg.PageSize << "-byte page"
                  << ", " << cfg.CacheLineSize << "-byte cache line"
                  << ", " << cfg.L2InterleavingGranularity << "-byte interleaving"
                  << ", " << cfg.NumMemControllers << " mem controllers"
                  // << ", " << cfg.FramesPerL2Slice << " frames per slice for ASR" /* CMU-ONLY */
                ));

    ///////////////// tiled torus
    if (cfg.Floorplan == "tiled-torus") {
      DBG_Assert(theNumCores == theNumL2Slices);

      // assign L2 numbers
      for (uint32_t i = 0; i < theNumL2Slices; ++i) {
        theL2NodeNumber[i] = i;
      }

      // assign core numbers
      for (uint32_t i = 0; i < theNumCores; ++i) {
        theCoreNodeNumber[i] = theNumL2Slices + i;
      }

      // 2D torus connectivity
      theNumCoresPerTorusRow = cfg.NumCoresPerTorusRow;

      theNTileID.reserve(theNumCores);
      uint32_t currID = theNumCores - theNumCoresPerTorusRow;
      for (uint32_t i = 0; i < theNumCores; i++) {
        theNTileID [i] = currID;
        currID ++;
        if (currID == theNumCores) {
          currID = 0;
        }
      }

      theSTileID.reserve(theNumCores);
      currID = theNumCoresPerTorusRow;
      for (uint32_t i = 0; i < theNumCores; i++) {
        theSTileID [i] = currID;
        currID ++;
        if (currID == theNumCores) {
          currID = 0;
        }
      }

      theWTileID.reserve(theNumCores);
      for (uint32_t row = 0; row < theNumCores / theNumCoresPerTorusRow;  row++) {
        uint32_t currID = (row + 1) * theNumCoresPerTorusRow - 1;
        for (uint32_t i = 0; i < theNumCoresPerTorusRow; i++) {
          theWTileID [row * theNumCoresPerTorusRow + i] = currID;
          currID ++;
          if (currID == (row + 1) * theNumCoresPerTorusRow) {
            currID -= theNumCoresPerTorusRow;
          }
        }
      }

      theETileID.reserve(theNumCores);
      for (uint32_t row = 0; row < theNumCores / theNumCoresPerTorusRow;  row++) {
        uint32_t currID = row * theNumCoresPerTorusRow + 1;
        for (uint32_t i = 0; i < theNumCoresPerTorusRow; i++) {
          theETileID [row * theNumCoresPerTorusRow + i] = currID;
          currID ++;
          if (currID == (row + 1) * theNumCoresPerTorusRow) {
            currID -= theNumCoresPerTorusRow;
          }
        }
      }

      // debugging
      for (uint32_t i = 0; i < theNumCores; i++) {
        DBG_Assert(i == theSTileID[theNTileID[i]]);
        DBG_Assert(i == theETileID[theWTileID[i]]);
      }

      /*
      DBG_( Crit, ( << "Torus Interconnect" ));
      DBG_( Crit, ( << "North cores" ));
      for (uint32_t i=0; i < theNumCores; i++) {
        std::cout << " " << theNTileID [i];
        if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
      DBG_( Crit, ( << "South cores" ));
      for (uint32_t i=0; i < theNumCores; i++) {
        std::cout << " " << theSTileID [i];
        if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
      DBG_( Crit, ( << "West cores" ));
      for (uint32_t i=0; i < theNumCores; i++) {
        std::cout << " " << theWTileID [i];
        if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
      DBG_( Crit, ( << "East cores" ));
      for (uint32_t i=0; i < theNumCores; i++) {
        std::cout << " " << theETileID [i];
        if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
      */

      /* CMU-ONLY-BLOCK-BEGIN */
      if (thePlacement == kRNUCACache) {
        // assign instruction interleaving IDs to the cores
        theSizeOfInstrCluster = cfg.SizeOfInstrCluster;
        theSizeOfInstrClusterLog2 = log_base2(cfg.SizeOfInstrCluster);
        theCoreInstrInterleavingID.reserve(theNumCores);

        for (uint32_t row = 0; row < theNumCores / theNumCoresPerTorusRow;  row++) {

          uint32_t theCurrentInstrInterleavingID;
          if ((theSizeOfInstrCluster < theNumCores / 2) || (theSizeOfInstrCluster == 4 && theNumCores == 8))
            theCurrentInstrInterleavingID = (row * theSizeOfInstrClusterLog2) % theSizeOfInstrCluster;
          else
            theCurrentInstrInterleavingID = (row * theNumCoresPerTorusRow) % theSizeOfInstrCluster;

          for (uint32_t i = 0; i < theNumCoresPerTorusRow; i++) {
            theCoreInstrInterleavingID [ row * theNumCoresPerTorusRow + i ] = theCurrentInstrInterleavingID;
            theCurrentInstrInterleavingID = (theCurrentInstrInterleavingID + 1) % theSizeOfInstrCluster;
          }
        }

        DBG_( Crit, ( << "Torus Instruction Interleaving IDs" ));
        for (uint32_t i = 0; i < theNumCores; i++) {
          std::cout << "  Core" << i << "[ " << theCoreInstrInterleavingID [i] << " ]";
          if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
            std::cout << std::endl;
          }
        }
        std::cout << std::endl;

        // make sure the rotational interleaving function works
        /*
        tCoreID center_RID = 4;
        for (tAddress addr=0; addr < theSizeOfInstrCluster; addr++) {
          int32_t dest_RID = (addr + ~center_RID + 1) & (theSizeOfInstrCluster-1);
          DBG_(Crit, ( << "addr=" << addr << " center=" << center_RID << " dest=" << dest_RID ));
        }
        */

        // assign private data interleaving IDs to the cores
        theSizeOfPrivCluster = cfg.SizeOfPrivCluster;
        theSizeOfPrivClusterLog2 = log_base2(cfg.SizeOfPrivCluster);
        theCorePrivInterleavingID.reserve(theNumCores);

        for (uint32_t row = 0; row < theNumCores / theNumCoresPerTorusRow;  row++) {

          uint32_t theCurrentPrivInterleavingID;
          if ((theSizeOfPrivCluster < theNumCores / 2) || (theSizeOfPrivCluster == 4 && theNumCores == 8))
            theCurrentPrivInterleavingID = (row * theSizeOfPrivClusterLog2) % theSizeOfPrivCluster;
          else
            theCurrentPrivInterleavingID = (row * theNumCoresPerTorusRow) % theSizeOfPrivCluster;

          for (uint32_t i = 0; i < theNumCoresPerTorusRow; i++) {
            theCorePrivInterleavingID [ row * theNumCoresPerTorusRow + i ] = theCurrentPrivInterleavingID;
            theCurrentPrivInterleavingID = (theCurrentPrivInterleavingID + 1) % theSizeOfPrivCluster;
          }
        }

        DBG_( Crit, ( << "Torus Private Data Interleaving IDs" ));
        for (uint32_t i = 0; i < theNumCores; i++) {
          std::cout << "  Core" << i << "[ " << theCorePrivInterleavingID [i] << " ]";
          if ( (i % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
            std::cout << std::endl;
          }
        }
        std::cout << std::endl;

        // make sure the rotational interleaving function works
        /*
        tCoreID center_RID = 4;
        for (tAddress addr=0; addr < theSizeOfPrivCluster; addr++) {
          int32_t dest_RID = (addr + ~center_RID + 1) & (theSizeOfPrivCluster-1);
          DBG_(Crit, ( << "addr=" << addr << " center=" << center_RID << " dest=" << dest_RID ));
        }
        */

        theIdxExtraOffsetBitsInstr   = theSizeOfInstrClusterLog2;
        theIdxExtraOffsetBitsShared  = theNumL2SlicesLog2;
        theIdxExtraOffsetBitsPrivate = theSizeOfPrivClusterLog2;
      } else
        /* CMU-ONLY-BLOCK-END */
        if (thePlacement == kPrivateCache) {
          /* FIXME
          // intialize the directory for the private cache
          // sharers bitmask size for up to 64 nodes
          uint32_t theSharersBitmaskComponentSize = sizeof(tSharers) * 8;
          theSharersBitmaskComponents = 64 * 2 / theSharersBitmaskComponentSize;
          theSharersBitmaskComponentSizeLog2 = log_base2(theSharersBitmaskComponentSize);
          theSharersBitmaskLowMask = (theSharersBitmaskComponentSize-1);
          DBG_Assert(theSharersBitmaskComponents <= 2, ( << "The current implementation supports at most 64 nodes."));

          // inval dir entry
          clearDirEntry(anInvalChipDirEntry);

          // intialize the directory FSM
          fillChipDirFSM();
          */
        }
    }

    ///////////////// woven torus
    else if (cfg.Floorplan == "woven-torus") {
      for (uint32_t i = 0; i < theNumCores; ++i) {
        theCoreNodeNumber[i] = 2 * i + (i % 4 < 2);
      }
      for (uint32_t i = 0; i < theNumL2Slices; ++i) {
        theL2NodeNumber[i] = 2 * i + (i % 4 >= 2);
      }
    }

    ///////////////// mesh
    else if (cfg.Floorplan == "mesh") {
      for (uint32_t i = 0; i < theNumCores; ++i) {
        theCoreNodeNumber[i] = 2 * i + (i % 4 >= 2);
      }
      for (uint32_t i = 0; i < theNumL2Slices; ++i) {
        theL2NodeNumber[i] = 2 * i + (i % 4 < 2);
      }
    }

    ///////////////// Cores on one side, L2 banks on the other
    else if (cfg.Floorplan == "1-1") {
      theCoreNodeNumber[0] = 0;
      theCoreNodeNumber[1] = 3;
      theCoreNodeNumber[2] = 1;
      theCoreNodeNumber[3] = 2;
      theCoreNodeNumber[4] = 28;
      theCoreNodeNumber[5] = 31;
      theCoreNodeNumber[6] = 29;
      theCoreNodeNumber[7] = 30;
      theCoreNodeNumber[8] = 4;
      theCoreNodeNumber[9] = 7;
      theCoreNodeNumber[10] = 5;
      theCoreNodeNumber[11] = 6;
      theCoreNodeNumber[12] = 24;
      theCoreNodeNumber[13] = 27;
      theCoreNodeNumber[14] = 25;
      theCoreNodeNumber[15] = 26;

      theL2NodeNumber[0] = 8;
      theL2NodeNumber[1] = 11;
      theL2NodeNumber[2] = 9;
      theL2NodeNumber[3] = 10;
      theL2NodeNumber[4] = 20;
      theL2NodeNumber[5] = 23;
      theL2NodeNumber[6] = 21;
      theL2NodeNumber[7] = 22;
      theL2NodeNumber[8] = 12;
      theL2NodeNumber[9] = 15;
      theL2NodeNumber[10] = 13;
      theL2NodeNumber[11] = 14;
      theL2NodeNumber[12] = 16;
      theL2NodeNumber[13] = 19;
      theL2NodeNumber[14] = 17;
      theL2NodeNumber[15] = 18;
    } else if (cfg.Floorplan == "4-4-line") {
      for (uint32_t i = 0; i < theNumCores; ++i) {
        theCoreNodeNumber[i] = i;
      }
      for (uint32_t i = 0; i < theNumL2Slices; ++i) {
        theL2NodeNumber[i] = theNumCores + i;
      }
    } else if (cfg.Floorplan == "4-4-block") {
      theCoreNodeNumber[0] = 0;
      theCoreNodeNumber[1] = 3;
      theCoreNodeNumber[2] = 28;
      theCoreNodeNumber[3] = 31;
      theCoreNodeNumber[4] = 5;
      theCoreNodeNumber[5] = 6;
      theCoreNodeNumber[6] = 25;
      theCoreNodeNumber[7] = 26;
      theCoreNodeNumber[8] = 8;
      theCoreNodeNumber[9] = 11;
      theCoreNodeNumber[10] = 20;
      theCoreNodeNumber[11] = 23;
      theCoreNodeNumber[12] = 13;
      theCoreNodeNumber[13] = 14;
      theCoreNodeNumber[14] = 17;
      theCoreNodeNumber[15] = 18;

      theL2NodeNumber[0] = 1;
      theL2NodeNumber[1] = 2;
      theL2NodeNumber[2] = 29;
      theL2NodeNumber[3] = 30;
      theL2NodeNumber[4] = 4;
      theL2NodeNumber[5] = 7;
      theL2NodeNumber[6] = 24;
      theL2NodeNumber[7] = 27;
      theL2NodeNumber[8] = 9;
      theL2NodeNumber[9] = 10;
      theL2NodeNumber[10] = 21;
      theL2NodeNumber[11] = 22;
      theL2NodeNumber[12] = 12;
      theL2NodeNumber[13] = 15;
      theL2NodeNumber[14] = 16;
      theL2NodeNumber[15] = 19;
    }

    ///////////////// The connectivity of these two is the same, one is just rotated 180 degrees
    else if (cfg.Floorplan == "cores-top-mesh" || cfg.Floorplan == "cores-bottom-mesh") {
      for (uint32_t i = 0; i < theNumCores; ++i) {
        theCoreNodeNumber[i] = i;
      }
      for (uint32_t i = 0; i < theNumL2Slices; ++i) {
        theL2NodeNumber[i] = theNumCores + i;
      }
    } else {
      DBG_Assert(false, ( << "CMPNetworkControl does not support floorplan \"" << cfg.Floorplan << "\""));
    }

    // Set the reverse mappings
    for (uint32_t i = 0; i < theNumCores; ++i) {
      isL2Node[theCoreNodeNumber[i]] = false;
      theTileNumber[theCoreNodeNumber[i]] = i;
    }
    for (uint32_t i = 0; i < theNumL2Slices; ++i) {
      isL2Node[theL2NodeNumber[i]] = true;
      theTileNumber[theL2NodeNumber[i]] = i;
    }
  }

  // Ports

  /////////////////////////////////////
  ///////////////////////////////////// from core
  bool available( interface::FromCore const &
                  , index_t anIndex
                ) {
    const uint32_t aCoreID = anIndex / theNumVChannels;
    const uint32_t aVC = anIndex % theNumVChannels;
    const uint32_t coreNodeNumber = theCoreNodeNumber[ aCoreID ];
    const uint32_t networkIndex = getIndex( coreNodeNumber, aVC );

    DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );
    return(FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex).available() );
  }

  void push( interface::FromCore const &
             , index_t anIndex
             , MemoryTransport & aTransport
           ) {
    // set up source network node
    const uint32_t aCoreID = anIndex / theNumVChannels;
    DBG_Assert( static_cast<int>(aCoreID) == aTransport[MemoryMessageTag]->coreNumber() ); // memory message already has the correct core number
    const uint32_t coreNodeNumber = theCoreNodeNumber[ aCoreID ];
    aTransport[ NetworkMessageTag ]->src = coreNodeNumber;

    // Virtual channel was set in NIC
    const uint32_t aVC = anIndex % theNumVChannels;
    DBG_Assert(static_cast<int>(aVC) == aTransport[ NetworkMessageTag ]->vc);
    const uint32_t networkIndex = getIndex( coreNodeNumber, aVC );

    aTransport[ NetworkMessageTag ]->size = aTransport[ MemoryMessageTag ]->carriesData() ? 1 : 0;
    aTransport[ NetworkMessageTag ]->src_port = 0;
    aTransport[ NetworkMessageTag ]->dst_port = 0;

    int32_t anL2ID;

    // get destination L2 slice
    anL2ID = getL2IDForCoreReq( aCoreID, aTransport );
    if (anL2ID < 0) {
      return; // if the request is held back,
      // or the request sent purges,    /* CMU-ONLY */
      // just return
    }

    // set up destination network node
    const uint32_t l2NodeNumber = theL2NodeNumber[ anL2ID ];
    aTransport[ NetworkMessageTag ]->dest = l2NodeNumber;
    aTransport[ MemoryMessageTag ]->l2Number() = anL2ID; // setup the L2 number in the memory message

    DBG_(Iface,
         Address(aTransport[ MemoryMessageTag ]->address())
         ( << statName()
           << " sending Core message " << *aTransport[ MemoryMessageTag ]
           << " into network from " << aTransport[ NetworkMessageTag ]->src
           << ":" << aTransport[ NetworkMessageTag ]->src_port
           << " (core[" << aCoreID << "])"
           << " to " << aTransport[ NetworkMessageTag ]->dest
           << ":" << aTransport[ NetworkMessageTag ]->dst_port
           << " (L2[" << anL2ID << "])"
           << " on vc[" << aTransport[ NetworkMessageTag ]->vc << "]"
           << " via idx " << networkIndex
         ));
    DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );
    FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex ) << aTransport;
  }

  /////////////////////////////////////
  ///////////////////////////////////// from L2
  bool available( interface::FromL2 const &
                  , index_t anIndex
                ) {
    const uint32_t anL2ID = anIndex / theNumVChannels;
    const uint32_t aVC = anIndex % theNumVChannels;
    const uint32_t srcNodeNumber = theL2NodeNumber[ anL2ID ];
    const uint32_t networkIndex = getIndex( srcNodeNumber, aVC );
    DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );
    return(FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex).available() );
  }

  void push( interface::FromL2 const &
             , index_t anIndex
             , MemoryTransport & aTransport
           ) {
    const tAddress anAddr = aTransport[ MemoryMessageTag ]->address();

    // src network node
    const uint32_t anL2ID = anIndex / theNumVChannels;
    const uint32_t srcNodeNumber = theL2NodeNumber[ anL2ID ];

    // src network idx
    const uint32_t aVC = anIndex % theNumVChannels;
    const uint32_t networkIndex = getIndex( srcNodeNumber, aVC );

    DBG_( Iface, Addr(anAddr) ( << statName()
                                << " received from L2[" << anL2ID << "]"
                                << " vc[" << aVC << "]"
                                << " msg " << *aTransport[ MemoryMessageTag ]
                              ));

    // dest network node
    uint32_t aCoreID = aTransport[ MemoryMessageTag ]->coreNumber(); // memory message already has the correct core number
    uint32_t dstNodeNumber;
    uint32_t dstNodeID;

    ///////////// shared cache
    if (thePlacement == kSharedCache) {
      // from L2 to core
      aTransport[ MemoryMessageTag ]->dstMC() = -1;
      dstNodeNumber = theCoreNodeNumber[ aCoreID ];
      dstNodeID = aCoreID;
      DBG_( VVerb, Addr(anAddr) ( << statName()
                                  << " sending L2 message to core[" << dstNodeID << "]"
                                  << " message " << *aTransport[ MemoryMessageTag ]
                                ));
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    ///////////// R-NUCA cache
    else if (thePlacement == kRNUCACache) {
      if (aTransport[ MemoryMessageTag ]->directionToBack()) {
        // from L2 backside
        const uint32_t aDstL2ID = getDstL2ID(anAddr);
        const uint32_t aDstMC = getDstMC(aDstL2ID);
        aTransport[ MemoryMessageTag ]->dstMC() = aDstMC;
        dstNodeNumber = theL2NodeNumber[ aDstL2ID ];
        dstNodeID = aDstL2ID;
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to MC[" << aDstMC << "]"
                                    << " through L2[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      } else if ( aTransport[ MemoryMessageTag ]->dstMC() != -1 ) {
        // from remote MC (attached to an L2 iface) to L2
        aTransport[ MemoryMessageTag ]->dstMC() = -1;
        dstNodeNumber = theL2NodeNumber[ aTransport[ MemoryMessageTag ]->l2Number() ];
        dstNodeID = aTransport[ MemoryMessageTag ]->l2Number();
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to L2[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      } else {
        // from L2 to core
        aTransport[ MemoryMessageTag ]->dstMC() = -1;
        dstNodeNumber = theCoreNodeNumber[ aCoreID ];
        dstNodeID = aCoreID;
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to core[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      }
    }
    /* CMU-ONLY-BLOCK-END */

    ///////////// private cache
    else {
      if (aTransport[ MemoryMessageTag ]->directionToBack()) {
        // from L2 backside
        const uint32_t aDstL2ID = getDstL2ID(anAddr);
        const uint32_t aDstMC = getDstMC(aDstL2ID);
        aTransport[ MemoryMessageTag ]->dstMC() = aDstMC;
        dstNodeNumber = theL2NodeNumber[ aDstL2ID ];
        dstNodeID = aDstL2ID;
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to MC[" << aDstMC << "]"
                                    << " through L2[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      } else if (aTransport[ MemoryMessageTag ]->isRequest()) {
        dstNodeID = aCoreID;
        if ( aTransport[ MemoryMessageTag ]->dstMC() == -1 // from remote MC (attached to an L2 iface) via this L2 to core
             || dstNodeID == anL2ID) {                     // or from this L2 which IS the directory owner
          dstNodeNumber = theCoreNodeNumber[ dstNodeID ];
          DBG_( VVerb, Addr(anAddr) ( << statName()
                                      << " sending L2 message to core[" << dstNodeID << "]"
                                      << " message " << *aTransport[ MemoryMessageTag ]
                                    ));
        } else {
          // from remote MC (attached to an L2 iface) to L2
          dstNodeNumber = theL2NodeNumber[ dstNodeID ];
          DBG_( VVerb, Addr(anAddr) ( << statName()
                                      << " sending L2 message to L2[" << dstNodeID << "]"
                                      << " message " << *aTransport[ MemoryMessageTag ]
                                    ));
        }
        aTransport[ MemoryMessageTag ]->dstMC() = -1;
      } else if ( aTransport[ MemoryMessageTag ]->dstMC() != -1 ) {
        // from remote MC (attached to an L2 iface) to L2
        aTransport[ MemoryMessageTag ]->dstMC() = -1;
        dstNodeNumber = theL2NodeNumber[ aTransport[ MemoryMessageTag ]->l2Number() ];
        dstNodeID = aTransport[ MemoryMessageTag ]->l2Number();
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to L2[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      } else {
        // from L2 to core
        aTransport[ MemoryMessageTag ]->dstMC() = -1;
        dstNodeNumber = theCoreNodeNumber[ aCoreID ];
        dstNodeID = aCoreID;
        DBG_( VVerb, Addr(anAddr) ( << statName()
                                    << " sending L2 message to core[" << dstNodeID << "]"
                                    << " message " << *aTransport[ MemoryMessageTag ]
                                  ));
      }
    }

    // Virtual channel was set in NIC
    aTransport[ NetworkMessageTag ]->src = srcNodeNumber;
    aTransport[ NetworkMessageTag ]->dest = dstNodeNumber;
    aTransport[ NetworkMessageTag ]->size = aTransport[ MemoryMessageTag ]->carriesData() ? 1 : 0;
    aTransport[ NetworkMessageTag ]->src_port = 0;
    aTransport[ NetworkMessageTag ]->dst_port = 0;

    aTransport[ MemoryMessageTag ]->l2Number() = anL2ID;    // setup the L2 number in the memory message

    DBG_(Iface, Addr(anAddr) ( << statName()
                               << " sending L2 message " << *aTransport[ MemoryMessageTag ]
                               << " into network from " << aTransport[ NetworkMessageTag ]->src
                               << ":" << aTransport[ NetworkMessageTag ]->src_port
                               << " (L2[" << anL2ID << "])"
                               << " to " << aTransport[ NetworkMessageTag ]->dest
                               << ":" << aTransport[ NetworkMessageTag ]->dst_port
                               << (isL2Node[ dstNodeNumber ] ? " (L2[" : " (core[" ) << dstNodeID << "])"
                               << " on vc[" << aTransport[ NetworkMessageTag ]->vc << "]"
                               << " via idx " << networkIndex
                             ));
    DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );
    FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex ) << aTransport;
  }

  /////////////////////////////////////
  ///////////////////////////////////// from network
  bool available(interface::FromNetwork const &
                 , index_t anIndex
                ) {
    const uint32_t nodeNumber = anIndex / theNumVChannels;
    const uint32_t aVC = anIndex % theNumVChannels;
    const uint32_t tileNumber = theTileNumber[ nodeNumber ];
    const uint32_t index = getIndex( tileNumber, aVC );

    // Going to an L2
    if ( isL2Node[ nodeNumber ] ) {
      DBG_Assert( index < theNumL2Slices * theNumVChannels );
      return( FLEXUS_CHANNEL_ARRAY( ToL2, index ).available() );
    }

    // Going to a core
    else {
      DBG_Assert( index < theNumCores * theNumVChannels );
      return( FLEXUS_CHANNEL_ARRAY( ToCore, index ).available() );
    }
  }

  void push( interface::FromNetwork const &
             , index_t anIndex
             , MemoryTransport & aTransport
           ) {
    DBG_(Iface,
         Addr( aTransport[ MemoryMessageTag ]->address() )
         ( << statName()
           << " sending Net message " << *aTransport[ MemoryMessageTag ]
           << " from network node " << aTransport[ NetworkMessageTag ]->src
           << ":" << aTransport[ NetworkMessageTag ]->src_port
           << " to " << aTransport[ NetworkMessageTag ]->dest
           << ":" << aTransport[ NetworkMessageTag ]->dst_port
           << (isL2Node[ anIndex / theNumVChannels ] ? " (L2[" : " (Core[" ) << theTileNumber[ anIndex / theNumVChannels ] << "])"
           << " on vc[" << aTransport[ NetworkMessageTag ]->vc << "]"
           << " via idx " << anIndex
         ));

    const uint32_t nodeNumber = anIndex / theNumVChannels;
    const uint32_t aVC = anIndex % theNumVChannels;
    const uint32_t tileNumber = theTileNumber[ nodeNumber ];
    const uint32_t index = getIndex( tileNumber, aVC );

    // Going to an L2
    if ( isL2Node[ nodeNumber ] ) {
      DBG_Assert( index < theNumL2Slices * theNumVChannels );
      FLEXUS_CHANNEL_ARRAY( ToL2, index ) << aTransport;
    }

    // Going to a core
    else {
      DBG_Assert( ( aTransport[ MemoryMessageTag ]->coreNumber() == static_cast<int> ( theTileNumber[ nodeNumber ] ) ),
                  ( << "Message was destined for cpu[" << aTransport[ MemoryMessageTag ]->coreNumber()
                    << "] but arrived at cpu[" << (theTileNumber[ nodeNumber ]) << "] "
                    << *aTransport[ MemoryMessageTag ]
                  ));

      DBG_Assert( index < theNumCores * theNumVChannels );
      FLEXUS_CHANNEL_ARRAY( ToCore, index ) << aTransport;
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  bool available(interface::PurgeAckIn const &
                 , index_t anIndex
                ) {
    return true;
  }

  void push( interface::PurgeAckIn const &
             , index_t anIndex
             , MemoryTransport & aTransport
           ) {
    // finalize purge when receiving all replies
    DBG_Assert (aTransport[ MemoryMessageTag ]->type() == MemoryMessage::PurgeAck);

    const tAddress anAddr = aTransport[ MemoryMessageTag ]->address();
    const tAddress aPageAddr = pageAddress(anAddr);
    tPendingPurgesMap::iterator iterPendingPurgesMap = thePendingPurgesMap.find(aPageAddr);
    DBG_Assert( iterPendingPurgesMap != thePendingPurgesMap.end() );
    tPendingPurgesMapEntry & e = iterPendingPurgesMap->second;

    DBG_(VVerb,
         Addr(aTransport[ MemoryMessageTag ]->address())
         ( << "O/S Received Purge Ack with acount: " << e.acount
           << " msg: " << *aTransport[ MemoryMessageTag ]
         ));

    e.acount --;
    if (e.acount == 0) {
      finalizePurge_RNUCA( aTransport );
      thePendingPurgesMap.erase(aPageAddr);
    }

    return;
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////
  ///////////////////////////////////// Drive Interfaces
  void drive( interface::CMPNetworkControlDrive const & ) {
    /* CMU-ONLY-BLOCK-BEGIN */
    // Revive any requests that have been waiting on purges
    reviveCustomerServiceQueue();

    // Handle any pending purges in the queue
    sendPurgeMsgFromQueue();
    /* CMU-ONLY-BLOCK-END */
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
private:
  /* The woven torus interonnect logically looks like this:
  Node  0 =   L2[0]  Node  1 = Core[0]   Node  2 =   L2[1]  Node  3 = Core[1]
  Node  4 = Core[2]  Node  5 =   L2[2]   Node  6 = Core[3]  Node  7 =   L2[3]
  Node  8 =   L2[4]  Node  9 = Core[4]   Node 10 =   L2[5]  Node 11 = Core[5]
  Node 12 = Core[6]  Node 13 =   L2[6]   Node 14 = Core[7]  Node 15 =   L2[7]
  Node 16 =   L2[8]  Node 17 = Core[8]   Node 18 =   L2[9]  Node 19 = Core[9]
  Node 20 = Core[10]  Node 21 =   L2[10]   Node 22 = Core[11]  Node 23 =   L2[11]
  Node 24 =   L2[12]  Node 25 = Core[12]   Node 26 =   L2[13]  Node 27 = Core[13]
  Node 28 = Core[14]  Node 29 =   L2[14]   Node 30 = Core[15]  Node 31 =   L2[15]
  */

  /* The cores-top torus interonnect logically looks like this:
  Node  0 = Core[0]  Node  1 = Core[2]   Node  2 = Core[3]  Node  3 = Core[1]
  Node  4 = Core[8]  Node  5 = Core[10]   Node  6 = Core[11]  Node  7 = Core[9]
  Node  8 =   L2[0]  Node  9 =   L2[2]   Node 10 =   L2[3]  Node 11 =   L2[1]
  Node 12 =   L2[8]  Node 13 =   L2[10]   Node 14 =   L2[11]  Node 15 =   L2[9]
  Node 16 =   L2[12]  Node 17 =   L2[14]   Node 18 =   L2[15]  Node 19 =   L2[13]
  Node 20 =   L2[4]  Node 21 =   L2[6]   Node 22 =   L2[7]  Node 23 =   L2[5]
  Node 24 = Core[12]  Node 25 = Core[14]   Node 26 = Core[15]  Node 27 = Core[13]
  Node 28 = Core[4]  Node 29 = Core[6]   Node 30 = Core[7]  Node 31 = Core[5]
  */

  ////////////////////// Helper functions
  uint32_t log_base2( const uint32_t num ) const {
    uint32_t x = num;
    uint32_t ii = 0;
    while (x > 1) {
      ii++;
      x >>= 1;
    }
    return ii;
  }

  uint32_t getDstL2ID ( const uint64_t anAddr ) const {
    return ((anAddr >> theInterleavingBlockShiftBits ) % theNumL2Slices ); //fixme: respect floorplan
  }

  int32_t getDstMC( const uint32_t aDstL2ID ) const {
    return (aDstL2ID  / (theNumL2Slices / theNumMemControllers)); //fixme : respect floorplan
  }

  inline uint64_t pageAddress( const uint64_t anAddr ) const {
    return (anAddr & thePageMask);
  }

  inline tAddress blockAddress( const tAddress addr ) const {
    return (addr & theCacheLineMask);
  }

  ////////////////////// indexing
  index_t getIndex( const uint32_t aNodeNumber
                    , const uint32_t aVC
                  ) const {
    return (aNodeNumber * theNumVChannels + aVC);
  }

  // translate a coreID to a tile ID
  uint32_t getTileID( const uint32_t aCoreID ) const {
    const uint32_t ret = (aCoreID / theNumCoresPerL2Slice);
    DBG_(VVerb, ( << "Core[" << aCoreID << "] is in tile " << ret ));
    return ret;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// facilitate snoop requests

  void sendPurge ( MemoryMessage::MemoryAddress       anAddr
                   , MemoryMessage::MemoryMessageType type
                   , uint32_t                     aCoreID
                   , uint32_t                     anL2ID
                   , uint32_t                     anIdxExtraOffsetBits
                   , MemoryTransport         &        origTransport
                 ) {
    uint32_t purges_sent = 0;
    const tAddress aPageAddr = pageAddress(anAddr);
    MemoryTransport aTransport;

    for (uint64_t addr = aPageAddr; addr < aPageAddr + thePageSize; addr += theCacheLineSize) {

      MemoryMessage::MemoryAddress purgeAddr( addr );
      uint32_t l2ID = getL2SliceIDPrivate_RNUCA( purgeAddr, aCoreID );
      uint32_t l2NodeNumber = theL2NodeNumber[ l2ID ];

      // memory tag
      MemoryMessage_p MemMsg = new MemoryMessage(type, purgeAddr, true, aCoreID, l2ID, anIdxExtraOffsetBits);
      aTransport.set( MemoryMessageTag, MemMsg );

      // network tag
      const uint32_t coreNodeNumber = theCoreNodeNumber[ aCoreID ]; // src network node
      const uint32_t aVC            = 0;
      NetworkMessage_p NetMsg = new NetworkMessage();
      NetMsg->src = coreNodeNumber;
      NetMsg->dest = l2NodeNumber;
      NetMsg->vc = aVC;
      NetMsg->size = 0;
      NetMsg->src_port = 0;
      NetMsg->dst_port = 0;
      aTransport.set( NetworkMessageTag, NetMsg );

      // transaction tracker tag
      TransactionTracker_p aTracker = new TransactionTracker();
      aTracker->setAddress(purgeAddr);
      aTracker->setInitiator(aCoreID);
      aTracker->setSource(statName() + " Purge");
      aTracker->setDelayCause(statName(), "Purge");
      aTransport.set( TransactionTrackerTag, aTracker );

      const uint32_t networkIndex = getIndex( coreNodeNumber, aVC );
      DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );

      if (FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aCoreID ).available()) {
        DBG_(VVerb ,
             Address(purgeAddr)
             ( << statName()
               << " send purge for addr: 0x" << std::hex << purgeAddr
               << " to core[" << std::dec << aCoreID << std::hex << "]"
               << " msg: " << *aTransport[MemoryMessageTag]
             ));
        FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aCoreID ) << aTransport;
      } else {
        // queue up
        theSendQueue.push_back ( aTransport );
      }

      purges_sent ++;
    }

    // remember the outstanding purge
    DBG_Assert (thePendingPurgesMap.find(aPageAddr) == thePendingPurgesMap.end());
    tPendingPurgesMapEntry e = { aTransport, purges_sent };
    thePendingPurgesMap.insert(std::make_pair(aPageAddr, e));
  }

  void sendPurgeMsgFromQueue( void ) {
    // try to give a message to the network
    std::list<MemoryTransport>::iterator i = theSendQueue.begin();
    while (i != theSendQueue.end()) {
      MemoryTransport aTransport         = *i;
      const uint32_t aCoreID         = aTransport[MemoryMessageTag]->coreNumber();

      if (FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aCoreID ).available()) {
        DBG_(VVerb,
             Address(aTransport[ MemoryMessageTag ]->address())
             ( << statName()
               << " sending Purge message " << *aTransport[ MemoryMessageTag ]
               << " msg:" << (*aTransport[ MemoryMessageTag ])
               << " into network from " << aTransport[ NetworkMessageTag ]->src
               << ":" << aTransport[ NetworkMessageTag ]->src_port
               << " (core[" << aCoreID << "])"
               << " to " << aTransport[ NetworkMessageTag ]->dest
               << ":" << aTransport[ NetworkMessageTag ]->dst_port
               << " (L2[" << aTransport[ MemoryMessageTag ]->l2Number() << "])"
               << " on vc[" << aTransport[ NetworkMessageTag ]->vc << "]"
               << " via idx " << getIndex( theCoreNodeNumber[ aCoreID ], aTransport[NetworkMessageTag]->vc )
             ));

        FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aCoreID ) << aTransport;

        // removing a list element invalidates only the iterators that point to it
        std::list<MemoryTransport>::iterator sent_element = i;
        i++;
        theSendQueue.erase(sent_element);
      }

      else {
        i++;
      }
    }
  }

  void reviveCustomerServiceQueue( void ) {
    // try to give a message to the network
    std::list<MemoryTransport>::iterator i = theCustomerServiceQueue.begin();
    while (i != theCustomerServiceQueue.end()) {
      MemoryTransport aTransport = *i;
      const tAddress anAddr = aTransport[ MemoryMessageTag ]->address();

      // get the entry
      tPageMap::iterator iterPageMap = thePageMap.find(pageAddress(anAddr));
      DBG_Assert (iterPageMap != thePageMap.end());
      tPageMapEntry & thePageMapEntry = iterPageMap->second;
      if (isInCustomerServiceState(thePageMapEntry)) {
        i++;
        continue;
      }

      const uint32_t aCoreID         = aTransport[MemoryMessageTag]->coreNumber();
      const uint32_t aCoreNodeNumber = theCoreNodeNumber[ aCoreID ];
      const uint32_t aVC             = aTransport[NetworkMessageTag]->vc;
      const uint32_t networkIndex    = getIndex( aCoreNodeNumber, aVC );
      const int32_t          anL2ID          = getL2IDForCoreReq( aCoreID, aTransport );
      DBG_Assert(anL2ID >= 0);

      // set up destination network node
      const uint32_t l2NodeNumber = theL2NodeNumber[ anL2ID ];
      aTransport[ NetworkMessageTag ]->dest = l2NodeNumber;

      aTransport[ MemoryMessageTag ]->l2Number() = anL2ID; // setup the L2 number in the memory message
      aTransport[ MemoryMessageTag ]->pageState() = kPageStateShared; // for stats: all requests held during a purge are shared when released

      if (FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex ).available()) {
        DBG_(VVerb, Address(anAddr) ( << statName()
                                      << " reviving & sending suspended Core message #" << aTransport[ MemoryMessageTag ]->serial()
                                      << " for 0x" << std::hex << anAddr << std::dec
                                      << " into network from " << aTransport[ NetworkMessageTag ]->src
                                      << ":" << aTransport[ NetworkMessageTag ]->src_port
                                      << " (core[" << aCoreID << "])"
                                      << " to " << aTransport[ NetworkMessageTag ]->dest
                                      << ":" << aTransport[ NetworkMessageTag ]->dst_port
                                      << " (L2[" << anL2ID << "])"
                                      << " on vc[" << aTransport[ NetworkMessageTag ]->vc << "]"
                                      << " via idx " << networkIndex
                                    ));
        DBG_Assert(aTransport[ TransactionTrackerTag ]->purgeDelayCycles());
        uint64_t thePurgeDelayCycles = Flexus::Core::theFlexus->cycleCount() - *aTransport[ TransactionTrackerTag ]->purgeDelayCycles();
        DBG_(Crit, ( << " Finished purge of page 0x" << std::hex << pageAddress(anAddr) << std::dec
                     << " and delayed request from core[" << aCoreID << "]"
                     << " for " << thePurgeDelayCycles << " cycles"
                   ));
        aTransport[ TransactionTrackerTag ]->setPurgeDelayCycles(thePurgeDelayCycles); // record the delay in the tracker for later time bkd accounting

        DBG_Assert( networkIndex < theNumVChannels * (theNumCores + theNumL2Slices) );
        FLEXUS_CHANNEL_ARRAY( ToNetwork, networkIndex ) << aTransport;

        // removing a list element invalidates only the iterators that point to it
        std::list<MemoryTransport>::iterator sent_element = i;
        i++;
        theCustomerServiceQueue.erase(sent_element);
      }

      else {
        i++;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// RNUCA placement

  ////////////////////// the page map
  typedef struct {
    //classification
    tPageState state;
    uint32_t privateTileID;   // owner of private data

    bool pageHasInstr; // not a real page map entry. Used only for stats and for I/D decoupling by page shootdown

  } tPageMapEntry;

  typedef __gnu_cxx::hash_map <  const tAddress // block address
  , tPageMapEntry  // entry pointer
  , PageHash       // cache-line hashing
  > tPageMap;

  tPageMap thePageMap;

  // get page type
  bool isDataClassInvalid( const tPageMapEntry & thePageMapEntry ) const {
    return (thePageMapEntry.state == kPageStateInvalid);
  }
  bool isShared( const tPageMapEntry & thePageMapEntry ) const {
    return (thePageMapEntry.state == kPageStateShared);
  }
  bool isPrivate( const uint32_t aTileID
                  , const tPageMapEntry & thePageMapEntry
                ) const {
    return (thePageMapEntry.state == kPageStatePrivate && (thePageMapEntry.privateTileID == aTileID));
  }
  bool isInCustomerServiceState( const tPageMapEntry & thePageMapEntry ) const {
    return (thePageMapEntry.state == kPageStateCustomerService);
  }
  tPageState getPageState( const tPageMapEntry & thePageMapEntry ) const {
    return thePageMapEntry.state;
  }

  // classify page
  void classifyAsInvalidDataClass( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.state = kPageStateInvalid;
  }
  void classifyAsShared( const uint32_t oldOwnerTileID
                         , tPageMapEntry & thePageMapEntry
                       ) {
    thePageMapEntry.state = kPageStateShared;
    thePageMapEntry.privateTileID = oldOwnerTileID;
  }
  void classifyAsPrivate( const uint32_t aTileID
                          , tPageMapEntry & thePageMapEntry
                        ) {
    thePageMapEntry.state = kPageStatePrivate;
    thePageMapEntry.privateTileID = aTileID;
  }
  void classifyAsCustomerService( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.state = kPageStateCustomerService;
  }

  void clearPageMapEntry( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.state = kPageStateInvalid;
    thePageMapEntry.privateTileID = TILE_ID_UNDEF;
    thePageMapEntry.pageHasInstr = false;
  }

  // get owner id
  uint32_t getPrivateL2ID( const tPageMapEntry & thePageMapEntry ) const {
    return thePageMapEntry.privateTileID;
  }

  // helper funcs for stats and for I/D decoupling by page shootdown
  bool pageHasData( const tPageMapEntry & thePageMapEntry ) const {
    return !isDataClassInvalid( thePageMapEntry );
  }
  bool pageHasInstr( const tPageMapEntry & thePageMapEntry ) const {
    return thePageMapEntry.pageHasInstr;
  }
  void classifyAsPageWithInstr( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.pageHasInstr = true;
  }
  void classifyAsPageWithoutInstr( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.pageHasInstr = false;
  }

  // helper funcs for debugging
  void print ( const tPageMapEntry & thePageMapEntry ) const {
    std::cerr << " state:         " << thePageMapEntry.state << std::endl;
    std::cerr << " privateTileID: " << thePageMapEntry.privateTileID << std::endl;
    std::cerr << " pageHasInstr:  " << thePageMapEntry.pageHasInstr << std::endl;
  }

  ////////////////////// indexing

  // get the L2 slice local to the interleaved core within the instruction storage cluster
  uint32_t getL2SliceIDInstr_RNUCA( const tAddress anAddr
                                    , const uint32_t aCoreID
                                  ) {
    // interleaving function      IL: addr ---> dest_RID
    const uint32_t theAddrInterleaveBits = (anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfInstrCluster - 1);
    const uint32_t dest_RID = theAddrInterleaveBits;

    // relative-vector function   RV: <center_RID,dest_RID> ---> D
    const uint32_t center_RID = theCoreInstrInterleavingID [ aCoreID ];
    const int32_t D = (dest_RID + ~center_RID + 1) & (theSizeOfInstrCluster - 1);

    // global mapping function    GM: <center_RID,D> ---> dest_CID
    DBG_Assert(theNumCores == 16 || (theNumCores == 8 && theNumCoresPerTorusRow == 4)); // works only for 4x4 and 4x2 toruses
    uint32_t dest_CID;
    if (theSizeOfInstrCluster <= 4) {
      switch (D) {
        case 0:
          dest_CID = getTileID(aCoreID);
          break;               // stay put (local L2 slice)
        case 1:
          dest_CID = theETileID[ getTileID(aCoreID) ];
          break; // go right
        case 2:
          dest_CID = theNTileID[ getTileID(aCoreID) ];
          break; // go up
        case 3:
          dest_CID = theWTileID[ getTileID(aCoreID) ];
          break; // go left
        default:
          DBG_Assert( false );
          dest_CID = 0; // compiler happy
      }
    } else if (theSizeOfInstrCluster == 8) {
      switch (D) {
        case 0:
          dest_CID = getTileID(aCoreID);
          break;                                           // stay put (local L2 slice)
        case 1:
          dest_CID = theETileID[ getTileID(aCoreID) ];
          break;                             // right
        case 2:
          dest_CID = theWTileID[ theWTileID[ getTileID(aCoreID) ] ];
          break;               // left-left
        case 3:
          dest_CID = theWTileID[ getTileID(aCoreID) ];
          break;                             // left
        case 4:
          dest_CID = theSTileID[ getTileID(aCoreID) ];
          break;                             // down
        case 5:
          dest_CID = theETileID[ theSTileID[ getTileID(aCoreID) ] ];
          break;               // down-right
        case 6:
          dest_CID = theWTileID[ theWTileID[ theSTileID[ getTileID(aCoreID) ] ] ];
          break; // down-left-left
        case 7:
          dest_CID = theSTileID[ theWTileID[ getTileID(aCoreID) ] ];
          break;               // left-down
        default:
          DBG_Assert( false );
          dest_CID = 0; // compiler happy
      }
    } else if (theSizeOfInstrCluster == 16) {
      dest_CID = theAddrInterleaveBits;
    } else {
      DBG_Assert( false );
      dest_CID = 0; // compiler happy
    }

    DBG_( VVerb, Addr(anAddr) ( << "Instruction " << std::hex << anAddr << std::dec
                                << " with interleave bits " << ((anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfInstrCluster - 1))
                                << " from core " << aCoreID
                                << " of tile " << getTileID(aCoreID)
                                << " to L2[" << dest_CID << "]"
                              ));
    return dest_CID;
  }

  // index to address-interleaved L2 slice
  uint32_t getL2SliceIDShared_RNUCA( const tAddress anAddr ) {
    const uint32_t ret = (anAddr >> theCacheLineSizeLog2) % theNumL2Slices;
    DBG_(VVerb, Addr(anAddr) ( << "Shared " << std::hex << anAddr << std::dec
                               << " to L2[" << ret << "]"
                             ));
    return ret;
  }

  // index to the private L2 slice
  uint32_t getL2SliceIDPrivate_RNUCA( const tAddress anAddr
                                      , const uint32_t  aCoreID
                                    ) {
    // interleaving function      IL: addr ---> dest_RID
    const uint32_t theAddrInterleaveBits = (anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfPrivCluster - 1);
    const uint32_t dest_RID = theAddrInterleaveBits;

    // relative-vector function   RV: <center_RID,dest_RID> ---> D
    const uint32_t center_RID = theCorePrivInterleavingID [ aCoreID ];
    const int32_t D = (dest_RID + ~center_RID + 1) & (theSizeOfPrivCluster - 1);

    // global mapping function    GM: <center_RID,D> ---> dest_CID
    DBG_Assert(theNumCores == 16 || (theNumCores == 8 && theNumCoresPerTorusRow == 4)); // works only for 4x4 and 4x2 toruses
    uint32_t dest_CID;
    if (theSizeOfPrivCluster <= 4) {
      switch (D) {
        case 0:
          dest_CID = getTileID(aCoreID);
          break;               // stay put (local L2 slice)
        case 1:
          dest_CID = theETileID[ getTileID(aCoreID) ];
          break; // go right
        case 2:
          dest_CID = theNTileID[ getTileID(aCoreID) ];
          break; // go up
        case 3:
          dest_CID = theWTileID[ getTileID(aCoreID) ];
          break; // go left
        default:
          DBG_Assert( false );
          dest_CID = 0; // compiler happy
      }
    } else if (theSizeOfPrivCluster == 8) {
      switch (D) {
        case 0:
          dest_CID = getTileID(aCoreID);
          break;                                           // stay put (local L2 slice)
        case 1:
          dest_CID = theETileID[ getTileID(aCoreID) ];
          break;                             // right
        case 2:
          dest_CID = theWTileID[ theWTileID[ getTileID(aCoreID) ] ];
          break;               // left-left
        case 3:
          dest_CID = theWTileID[ getTileID(aCoreID) ];
          break;                             // left
        case 4:
          dest_CID = theSTileID[ getTileID(aCoreID) ];
          break;                             // down
        case 5:
          dest_CID = theETileID[ theSTileID[ getTileID(aCoreID) ] ];
          break;               // down-right
        case 6:
          dest_CID = theWTileID[ theWTileID[ theSTileID[ getTileID(aCoreID) ] ] ];
          break; // down-left-left
        case 7:
          dest_CID = theSTileID[ theWTileID[ getTileID(aCoreID) ] ];
          break;               // left-down
        default:
          DBG_Assert( false );
          dest_CID = 0; // compiler happy
      }
    } else if (theSizeOfPrivCluster == 16) {
      dest_CID = theAddrInterleaveBits;
    } else {
      DBG_Assert( false );
      dest_CID = 0; // compiler happy
    }

    DBG_( VVerb, Addr(anAddr) ( << "Private " << std::hex << anAddr << std::dec
                                << " with interleave bits " << ((anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfPrivCluster - 1))
                                << " from core " << aCoreID
                                << " of tile " << getTileID(aCoreID)
                                << " to L2[" << dest_CID << "]"
                              ));
    return dest_CID;
  }

  ////////////////////// categorization, indexing and placement
  //
  // page classification state machine (only valid transitions shown)
  //  #   <current DataClassValid, P/S, owner, mapping>  |  <I/D stream request>  <core>  |  <next state DataClassValid, P/S, owner, mapping>
  //
  //  1*                     x      x     x     no       |           I              x     |                        0      x     x     yes
  //  2*                     x      x     x     no       |           D              c     |                        1      P     c     yes
  //
  //  3                      0      x     x     yes      |           I              x     |                        0      x     x     yes
  //  4*                     0      x     x     yes      |           D              c     |                        1      P     c     yes
  //  5.                     0      x     x     yes      |       shootdown          x     |                        x      x     x     no
  //
  //  6                      1      P     c     yes      |           I              x     |                        1      P     c     yes
  //  7                      1      P     c     yes      |           D              c     |                        1      P     c     yes
  //  8*                     1      P     c     yes      |           D            !=c     |                        1      S     x     yes
  //  9.                     1      P     c     yes      |       shootdown          x     |                        x      x     x     no
  //
  // 10                      1      S     x     yes      |           I              x     |                        1      S     x     yes
  // 11                      1      S     x     yes      |           D              x     |                        1      S     x     yes
  // 12.                     1      S     x     yes      |       shootdown          x     |                        x      x     x     no
  //
  // The "*" transitions (1, 2, 4, 8) are the only ones, other than shootdowns, that change the state of the page. They are implemented below.
  // The unmarked transitions 3, 6, 7, 10, 11 do not change any state. Count only stats for them.
  //
  // TODO: implement page (TLB) shootdown to transition back to invalid mapping stage ("." transitions -- 5, 9, 12)
  // This may be used, for example, by the loader when a program ends and another one starts and reuses the same page frames.
  //

  // Return a page map iterator for the requested entry.
  // If the page is not found, insert and classify it.
  tPageMap::iterator
  findOrInsertPage( const tAddress anAddr
                    , const uint32_t aCoreID
                    , const bool fromDCache
                  ) {
    const tAddress aPageAddr = pageAddress(anAddr);

    // get the entry
    tPageMap::iterator iterPageMap = thePageMap.find(aPageAddr);
    if (iterPageMap == thePageMap.end()) {
      tPageMapEntry aPageMapEntry;
      clearPageMapEntry( aPageMapEntry );
      if (!fromDCache) {
        // transition 1
        classifyAsInvalidDataClass(aPageMapEntry);
        classifyAsPageWithInstr(aPageMapEntry);
        DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr first touch addr=0x" << std::hex << anAddr << std::dec ));
      } else {
        // transition 2
        classifyAsPrivate( getTileID(aCoreID), aPageMapEntry );
        classifyAsPageWithoutInstr(aPageMapEntry);
        DBG_( VVerb, Addr(anAddr) ( << "Classify as Priv first touch addr=0x" << std::hex << anAddr << std::dec
                                    << " owner=" << getPrivateL2ID(aPageMapEntry)
                                  ));
      }
      thePageMap.insert(std::make_pair(aPageAddr, aPageMapEntry));
      iterPageMap = thePageMap.find(aPageAddr);
    }
    return iterPageMap;
  }

  // Classify a page. Return false if re-classification priv->shared is necessary
  bool
  pageClassification( const uint32_t aCoreID
                      , tPageMap::iterator iterPageMap
                      , MemoryTransport & aTransport
                    ) {
    const tAddress  anAddr          = aTransport[ MemoryMessageTag ]->address();
    const bool      fromDCache      = aTransport[ MemoryMessageTag ]->isDstream(); // differentiate icache from dcache
    tPageMapEntry & thePageMapEntry = iterPageMap->second;
    bool            compiler_happy  = false;

    if (   aTransport[ MemoryMessageTag ]->isRequest()
           && !aTransport[ MemoryMessageTag ]->isEvict()
           && !aTransport[ MemoryMessageTag ]->isPurge()
       ) {
      // istream ref on page that until now had none
      // transition 6, 10 (first time only)
      if ( !fromDCache
           && !pageHasInstr( thePageMapEntry )
         ) {
        classifyAsPageWithInstr( thePageMapEntry );
        if ( isShared(thePageMapEntry) ) {
          DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Shared upon Istream ref for addr=0x" << std::hex << anAddr << std::dec ));
        } else {
          DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Priv upon Istream ref for addr=0x" << std::hex << anAddr << std::dec
                                      << " owner=" << getPrivateL2ID(thePageMapEntry)
                                    ));
        }
      }

      // data ref in page that until now had none
      // transition 4
      else if ( fromDCache
                && isDataClassInvalid( thePageMapEntry )
              ) {
        classifyAsPrivate( getTileID(aCoreID), thePageMapEntry );
        DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Priv upon data ref for addr=0x" << std::hex << anAddr << std::dec
                                    << " owner=" << getPrivateL2ID(thePageMapEntry)
                                  ));
      }

      // reclassify priv -> shared
      // transition 8
      else if ( fromDCache
                && !isShared(thePageMapEntry)
                && !isPrivate( getTileID(aCoreID), thePageMapEntry )
                && !isInCustomerServiceState(thePageMapEntry) // if a purge is already in progress, allow reqs from old owner to finish
              ) {
        return false;
      }
    }

    return true;

    compiler_happy = anAddr == 0;
  }

  uint32_t getL2IDForCoreReq_RNUCA( const uint32_t aCoreID
                                    , MemoryTransport & aTransport
                                  ) {
    const tAddress anAddr     = aTransport[ MemoryMessageTag ]->address();
    const bool     fromDCache = aTransport[ MemoryMessageTag ]->isDstream(); // differentiate icache from dcache

    // get the entry
    // if no entry is found, a new one is created and classified initially based on the request
    tPageMap::iterator iterPageMap = findOrInsertPage(anAddr, aCoreID, fromDCache);
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    // your call is important to us...
    if (
      isInCustomerServiceState(thePageMapEntry)
      && !aTransport[ MemoryMessageTag ]->isPurge()
      && !aTransport[ MemoryMessageTag ]->isEvict()
    ) {
      // if a purge is already in progress, allow reqs from old owner to finish
      if (thePageMapEntry.privateTileID == getTileID(aCoreID)) {
        DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                    << " Class = Private"
                                    << " O/S allows request from old owner " << *aTransport[ MemoryMessageTag ] )
            );

        // indexing and placement
        aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
        aTransport[ MemoryMessageTag ]->pageState() = kPageStatePrivate; // for stats: reqs from old owner are private
        uint32_t theProbedL2 = getL2SliceIDPrivate_RNUCA(anAddr, aCoreID);
        // print(thePageMapEntry);

        return theProbedL2;
      } else {
        // your call is important to us. Please hold for the next available representative...
        aTransport[ TransactionTrackerTag ]->setDelayCause(statName(), "Purge");
        aTransport[ TransactionTrackerTag ]->setPurgeDelayCycles(Flexus::Core::theFlexus->cycleCount()); // remember, remember, the time of the purger
        theCustomerServiceQueue.push_back( aTransport );
        DBG_( VVerb, Addr(anAddr) ( << "Hold in customer service " << *aTransport[ MemoryMessageTag ] ));

        return -1;
      }
    }

    // Classify the page
    if (!pageClassification( aCoreID, iterPageMap, aTransport )) {
      // reclassify priv -> shared
      // transition 8
      DBG_(Verb, Addr(anAddr) ( << "Reclassify page 0xp:" << std::hex << pageAddress(anAddr) << std::dec
                                << " priv L2[" << getPrivateL2ID(thePageMapEntry) << "]"
                                << " -> shared"
                              ));
      // print(thePageMapEntry);

      // Shoot down the page
      // Only the private data will be shot down. Instr (if they exist) will be left mostly untouched.
      uint32_t reqDestL2 = getL2SliceIDShared_RNUCA(anAddr);
      uint32_t purgeDest = getPrivateL2ID(thePageMapEntry);
      DBG_Assert( reqDestL2 < theNumL2Slices );

      classifyAsCustomerService(thePageMapEntry);
      DBG_( VVerb, Addr(anAddr) ( << "Classify as CustomerService addr=0x" << std::hex << anAddr << std::dec
                                  << " owner=" << getPrivateL2ID(thePageMapEntry)
                                ));

      MemoryMessage::MemoryMessageType reqType;
      reqType = MemoryMessage::PurgeDReq;
      sendPurge(MemoryMessage::MemoryAddress(anAddr)
                , reqType
                , purgeDest
                , reqDestL2
                , theIdxExtraOffsetBitsPrivate
                , aTransport
               );

      // your call is important to us. Please hold for the next available representative...
      aTransport[ TransactionTrackerTag ]->setDelayCause(statName(), "Purge");
      aTransport[ TransactionTrackerTag ]->setPurgeDelayCycles(Flexus::Core::theFlexus->cycleCount()); // remember, remember, the time of the purger
      theCustomerServiceQueue.push_back( aTransport );
      DBG_( VVerb, Addr(anAddr) ( << "Hold in customer service " << *aTransport[ MemoryMessageTag ] ));

      return -2;
    }

    // indexing and placement
    uint32_t theProbedL2;
    if ( !fromDCache ) {
      aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theIdxExtraOffsetBitsInstr;
      theProbedL2 = getL2SliceIDInstr_RNUCA(anAddr, aCoreID);
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Instr" ));
    } else if ( isShared(thePageMapEntry) ) {
      aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theIdxExtraOffsetBitsShared;
      theProbedL2 = getL2SliceIDShared_RNUCA(anAddr);
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Shared" ));
    } else {
      aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
      theProbedL2 = getL2SliceIDPrivate_RNUCA(anAddr, aCoreID);
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Private" ));
    }

    aTransport[ MemoryMessageTag ]->pageState() = getPageState( thePageMapEntry ); // for stats

    // print(thePageMapEntry);

    return theProbedL2;
  }

  int32_t finalizePurge_RNUCA ( MemoryTransport & aTransport ) {
    // get the entry
    const tAddress anAddr = aTransport[ MemoryMessageTag ]->address();
    tPageMap::iterator iterPageMap = thePageMap.find(pageAddress(anAddr));
    DBG_Assert (iterPageMap != thePageMap.end());
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    //////////////// unlock the cache mafs
    uint32_t aPurgeFinalizeDest = getPrivateL2ID(thePageMapEntry);
    MemoryTransport aFinalizeTransport;

    MemoryMessage_p MemMsg = new MemoryMessage(MemoryMessage::FinalizePurge,
        PhysicalMemoryAddress(anAddr),
        true,
        aPurgeFinalizeDest,
        getTileID(aPurgeFinalizeDest),
        theIdxExtraOffsetBitsPrivate
                                              );
    aFinalizeTransport.set( MemoryMessageTag, MemMsg );

    TransactionTracker_p aTracker = new TransactionTracker();
    aTracker->setAddress(PhysicalMemoryAddress(anAddr));
    aTracker->setInitiator(aPurgeFinalizeDest);
    aTracker->setSource(statName() + " Purge");
    aTracker->setDelayCause(statName(), "Purge");
    aFinalizeTransport.set( TransactionTrackerTag, aTracker );

    if (FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aPurgeFinalizeDest ).available()) {
      DBG_(VVerb ,
           Address(anAddr)
           ( << statName()
             << " send finalize purge for addr: 0x" << std::hex << anAddr
             << " to core[" << std::dec << aPurgeFinalizeDest << std::hex << "]"
             << " msg: " << *aFinalizeTransport[ MemoryMessageTag ]
           ));
      FLEXUS_CHANNEL_ARRAY( PurgeAddrOut, aPurgeFinalizeDest ) << aFinalizeTransport;
    } else {
      // queue up
      theSendQueue.push_back ( aFinalizeTransport );
    }

    ///////////////////// finish
    classifyAsShared(getPrivateL2ID(thePageMapEntry), thePageMapEntry);
    aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theIdxExtraOffsetBitsShared;
    DBG_( VVerb, Addr(anAddr) ( << " finalize purge for Addr: 0x" << std::hex << anAddr << std::dec
                                << " Class = Shared" ));
    aTransport[ MemoryMessageTag ]->pageState() = getPageState( thePageMapEntry ); // for stats
    // print(thePageMapEntry);

    return getL2SliceIDShared_RNUCA(anAddr);
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// private L2 placement

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////// stats
  void
  pageBkdStats( const uint32_t aCoreID
                , MemoryTransport & aTransport
              ) {
    const tAddress anAddr     = aTransport[ MemoryMessageTag ]->address();
    const bool     fromDCache = aTransport[ MemoryMessageTag ]->isDstream(); // differentiate icache from dcache

    // get the entry
    // if no entry is found, a new one is created and classified initially based on the request
    tPageMap::iterator iterPageMap = findOrInsertPage(anAddr, aCoreID, fromDCache);
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    // Classify the page
    if (!pageClassification( aCoreID, iterPageMap, aTransport )) {
      // must reclassify page priv->shared
      classifyAsShared(getPrivateL2ID(thePageMapEntry), thePageMapEntry);
      DBG_( VVerb, Addr(anAddr) ( << "Classify as Shared addr=0x" << std::hex << anAddr << std::dec
                                  << " owner=" << getPrivateL2ID(thePageMapEntry)
                                ));
    }

    aTransport[ MemoryMessageTag ]->pageState() = getPageState( thePageMapEntry ); // for stats
  }
  /* CMU-ONLY-BLOCK-END */

  ////////////////////// indexing and placement
  uint32_t getL2IDForCoreReq_private( const uint32_t aCoreID
                                      , MemoryTransport & aTransport
                                    ) {
    pageBkdStats( aCoreID, aTransport );  /*  CMU-ONLY */

    return aCoreID; // coreID and L2ID are the same in the private scheme
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// distributed shared cache placement (statically address-interleaved)

  ////////////////////// indexing and placement
  uint32_t getL2IDForCoreReq_shared( const uint32_t aCoreID
                                     , MemoryTransport & aTransport
                                   ) {
    pageBkdStats( aCoreID, aTransport );  /*  CMU-ONLY */

    const uint64_t anAddr(aTransport[MemoryMessageTag]->address());
    uint32_t anL2ID = ((anAddr >> theInterleavingBlockShiftBits) % theNumL2Slices);
    aTransport[ MemoryMessageTag ]->idxExtraOffsetBits() = theNumL2SlicesLog2;
    DBG_( VVerb, Addr(anAddr) ( << "Core[" << aCoreID << "]"
                                << " req for 0x" << anAddr
                                << " routed to " << anL2ID
                              ));
    return anL2ID;
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// checkpoint save restore
  void saveState( std::string const & aDirName ) {
    DBG_Assert( false );
  }

  void loadState( std::string const & aDirName ) {
    /* CMU-ONLY-BLOCK-BEGIN */
    std::string fname ( aDirName );
    fname += "/" + statName() + "-RNUCAPageMap";
    std::ifstream ifs ( fname.c_str() );

    thePageMap.clear(); // empty the page map

    if (! ifs.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty R-NUCA page table. " )  );
    } else {
      ifs >> std::skipws; // skip white space

      // Read the R-NUCA page map info
      int32_t page_map_size = 0;
      uint32_t aPlacement;
      ifs >> aPlacement
          >> theNumCoresPerL2Slice
          >> theCacheLineSize
          >> thePageSize
          >> theSizeOfInstrCluster
          >> page_map_size
          ;

      // make sure the configurations match
      DBG_Assert(theCacheLineSizeLog2 == log_base2(theCacheLineSize));
      DBG_Assert(thePageSizeLog2 == log_base2(thePageSize));
      // if simulating R-NUCA, the page map should be non-empty
      DBG_Assert ( thePlacement != kRNUCACache || page_map_size >= 0 );

      // Read in each entry
      for ( int32_t i = 0; i < page_map_size; i++ ) {
        char paren1;
        uint64_t theAddress;
        bool isDataClassValid;
        bool isShared;
        uint32_t privateTileID;                // owner of private data
        bool pageHasInstr;                         // not a real page map entry. Used only for stats and for I/D decoupling
        bool HasFetchRef;                          // for whitebox debugging
        uint64_t theCachedLineBitmask_1; // not a real page map entry. Used only to optimize the simflex code
        uint64_t theCachedLineBitmask_0; // not a real page map entry. Used only to optimize the simflex code
        char paren2;

        ifs >> paren1
            >> theAddress
            >> isDataClassValid
            >> isShared
            >> privateTileID           // owner of private data
            >> pageHasInstr            // not a real page map entry. Used only for stats and for I/D decoupling
            >> HasFetchRef             // for whitebox debugging
            >> theCachedLineBitmask_1  // not a real page map entry. Used only to optimize the simflex code
            >> theCachedLineBitmask_0  // not a real page map entry. Used only to optimize the simflex code
            >> paren2
            ;
        DBG_Assert ( (paren1 == '{'), ( << "Expected '{' when loading thePageTable from checkpoint" ) );
        DBG_Assert ( (paren2 == '}'), ( << "Expected '}' when loading thePageTable from checkpoint" ) );

        DBG_ ( VVerb,
               Addr(theAddress)
               ( << std::hex
                 << " Address: "              << theAddress
                 << " isDataClassValid: "     << isDataClassValid
                 << " isShared: "             << isShared
                 << " privateTileID: "        << privateTileID
                 << " pageHasInstr: "         << pageHasInstr
                 << " HasFetchRef: "          << HasFetchRef
                 << " theCachedLineBitmask: " << theCachedLineBitmask_1 << "-" << theCachedLineBitmask_0
               ) );

        tPageMapEntry thePageMapEntry;
        if (isShared) {
          thePageMapEntry.state = kPageStateShared;
          DBG_Assert( isDataClassValid );
        } else if (isDataClassValid) {
          thePageMapEntry.state = kPageStatePrivate;
          DBG_Assert( isDataClassValid );
        } else {
          thePageMapEntry.state = kPageStateInvalid;
          DBG_Assert( pageHasInstr );
        }
        thePageMapEntry.privateTileID = privateTileID;
        thePageMapEntry.pageHasInstr = pageHasInstr;

        tPageMap::iterator iter;
        bool is_new;
        boost::tie(iter, is_new) = thePageMap.insert( std::make_pair( theAddress, thePageMapEntry ) );
        DBG_Assert(is_new);
      }
      ifs.close();
      DBG_( Dev, ( << "R-NUCA PageMap loaded." ));
    }
    /* CMU-ONLY-BLOCK-END */
  }

};

} //End Namespace nCMPNetworkControl

FLEXUS_COMPONENT_INSTANTIATOR( CMPNetworkControl, nCMPNetworkControl );

FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, ToCore) {
  return cfg.VChannels * Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, FromCore) {
  return cfg.VChannels * Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, ToL2) {
  return cfg.VChannels * cfg.NumL2Tiles;
}
FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, FromL2) {
  return cfg.VChannels * cfg.NumL2Tiles;
}

FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, ToNetwork) {
  return cfg.VChannels * (cfg.NumL2Tiles + Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, FromNetwork) {
  return cfg.VChannels * (cfg.NumL2Tiles + Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}

FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, PurgeAddrOut) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();  /* CMU-ONLY */
}
FLEXUS_PORT_ARRAY_WIDTH( CMPNetworkControl, PurgeAckIn)   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();  /* CMU-ONLY */
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPNetworkControl

#define DBG_Reset
#include DBG_Control()
