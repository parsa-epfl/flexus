#include <components/FastCMPNetworkController/FastCMPNetworkController.hpp>
#include <core/stats.hpp>

#include <components/WhiteBox/WhiteBoxIface.hpp>
#include <components/Common/CachePlacementPolicyDefn.hpp>

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
#include <math.h>   // for trunc()

#define DBG_DefineCategories CMPNetworkController
#define DBG_SetDefaultOps AddCat(CMPNetworkController) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastCMPNetworkController
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define VTBDbg VVerb

namespace nFastCMPNetworkController {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

namespace Stat = Flexus::Stat;
using boost::counted_base;
using boost::intrusive_ptr;

typedef uint64_t tAddress;
typedef uint32_t tTileID;
typedef uint32_t tCoreID;
typedef uint32_t tL2SliceID;

typedef boost::function<tL2SliceID( const index_t anIndex, MemoryMessage & aMessage ) > tRouteCoreReq_func;
typedef boost::function<void( index_t anIndex, MemoryMessage & aMessage ) > tPropagateSnoopUp_func;
typedef boost::function<void( index_t anIndex, MemoryMessage & aMessage ) > tUpdateOffChipStats_func; /* CMU-ONLY */
typedef boost::function<void( const tL2SliceID anL2ID, const tCoreID aCoreID ) > tUpdatePeerL1HopsStats_func; /* CMU-ONLY */
typedef boost::function<void( const tL2SliceID anL2ID, const tCoreID aCoreID ) > tUpdateReplyHopsStats_func; /* CMU-ONLY */
typedef boost::function<void( const int32_t reqRoundTripHops ) > tUpdateReqRoundTripHopsStats_func; /* CMU-ONLY */

static const tTileID TILE_ID_UNDEF = ( 1ULL << (sizeof(tTileID) * 8) ) - 1;

static uint32_t thePageSizeLog2;       // log2 (page size)
static uint32_t theCacheLineSizeLog2;  // log2 (cache line size)

class FLEXUS_COMPONENT(FastCMPNetworkController) {
  FLEXUS_COMPONENT_IMPL( FastCMPNetworkController );

  tCoreID      theNumCores;           // # cores
  tL2SliceID   theNumL2Slices;        // # L2 Slices
  uint32_t theNumL2SlicesLog2;

  tAddress     theCacheLineSize;      // cache line size
  tAddress     theCacheLineMask;      // mask out the cache line offset
  tAddress     thePageSize;           // page size
  tAddress     thePageMask;           // mask out the page offset

  tCoreID      theNumCoresPerL2Slice; // # cores per L2 slice

  // 2D torus topology
  uint32_t theNumCoresPerTorusRow;  // the number of cores per torus row
  std::vector<uint32_t> theNTileID; // north tile neighbor in torus
  std::vector<uint32_t> theSTileID; // south tile neighbor in torus
  std::vector<uint32_t> theWTileID; // west tile neighbor in torus
  std::vector<uint32_t> theETileID; // east tile neighbor in torus

  int32_t theHops[64][64];                  // array of hop distances in 2D torus

  /* CMU-ONLY-BLOCK-BEGIN */
  int32_t coreToL2Hops;                     // hops for coreRequestor -> homeL2
  int32_t dirHops;                          // hops for homeL2 -> dir (if L2 and dir on different slices -- applicable only to private designs for now)
  int32_t snoopedL2Hops;                    // hops for dir -> snoopedL2 (applicable only to private designs for now)
  int32_t peerL1Hops;                       // additional hops for home L2 -> peerL1
  int32_t replyHops;                        // additional hops for PeerL1 -> coreRequestor or homeL2 -> coreRequestor
  /* CMU-ONLY-BLOCK-END */

  int32_t theProbedL2;                      // the L2 slice that is probed for an I/D request
  int32_t theDir;                           // the L2 slice that also hosts the directory for this line (applicably only to private designs for now)
  int32_t theSnoopedL2;                     // the L2 slice that was snooped for data (applicable only to private designs for now)
  int32_t thePeerL1;                        // the peer L1 replying to the I/D request

  bool theDstream;                             // help with stats: is the original request a dstream or an istream request?
  MemoryMessage::MemoryMessageType theType;    // help with stats: the type of the original message
  bool theL2Dstream;                           // help with stats: is the request off of L2 a dstream or an istream request? -- for private L2s
  MemoryMessage::MemoryMessageType theL2Type;  // help with stats: the type of the message out of L2 -- for private L2s

  tPlacement thePlacement; // L2 design
  bool thePrivateWithASR; /* CMU-ONLY */

  tRouteCoreReq_func     routeCoreReq;       // member function pointer for processing core requests
  tPropagateSnoopUp_func propagateSnoopUp;   // member function pointer for processing bus snoops

  tUpdatePeerL1HopsStats_func       updatePeerL1HopsStats;       // member function pointer for design-specific peerL1 stats updater /* CMU-ONLY */
  tUpdateReplyHopsStats_func        updateReplyHopsStats;        // member function pointer for design-specific reply hops stats updater /* CMU-ONLY */
  tUpdateReqRoundTripHopsStats_func updateReqRoundTripHopsStats; // member function pointer for design-specific request round-trip hops updater /* CMU-ONLY */
  tUpdateOffChipStats_func          updateOffChipStats;          // member function pointer for design-specific miss stat updater /* CMU-ONLY */

  // ASRCache structures /* CMU-ONLY */
  // PrivateCache structures
  uint64_t theOffChipRequest;

  /* CMU-ONLY-BLOCK-BEGIN */
  // ASR structures
  unsigned theASRRandomContext;
  std::vector<int> theASR_ReplicationLevel;

  std::vector<int> theBenefitOfIncreasingReplication;
  std::vector<int> theCostOfIncreasingReplication;
  std::vector<int> theBenefitOfDecreasingReplication;
  std::vector<int> theCostOfDecreasingReplication;

  bool theRemoteL2Hit;
  bool theASR_InsertInVTB;
  bool theASR_LocalAllocation;
  bool theASR_NLHBInsert;

  struct by_LRU {};
  struct by_address;
  typedef boost::multi_index_container
  < tAddress
  , indexed_by
  < sequenced< tag<by_LRU> >
  , ordered_unique
  < tag<by_address>
  , identity<tAddress>
  >
  >
  > tOrderedAddrList;
  typedef tOrderedAddrList::index<by_LRU>::type::iterator tOrderedAddrList_lru_iter;
  typedef tOrderedAddrList::index<by_address>::type::iterator tOrderedAddrList_addr_iter;

  std::vector<tOrderedAddrList> theNLHB;
  std::vector<tOrderedAddrList> theSoonToBeEvicted;
  std::vector<tOrderedAddrList> theVTB;
  std::vector<std::set<tAddress> > theCurrentReplicationOnly;

  std::vector<int> theASR_LocalReplicationCount;
  std::vector<int> theASR_NLHBAllocationCount;

  std::vector<std::vector<int> > theASR_LastEvaluations;

  int32_t theASR_ReplicationThresholds[8];

  void saveVector( std::ofstream & ofs, std::string const & aName, std::vector<int> const & aVector ) {
    ofs << aName << " " << aVector.size() << std::endl;
    std::vector<int>::const_iterator iter( aVector.begin() );
    for (; iter != aVector.end(); iter++)
      ofs << *iter << " ";
    ofs << std::endl;
    ofs << '-' << std::endl;
    DBG_(Dev, ( << "Saved " << aName));
  }

  void saveVector( std::ofstream & ofs, std::string const & aName, std::vector<tOrderedAddrList> const & aVector ) {
    ofs << aName << " " << aVector.size() << std::endl;
    std::vector<tOrderedAddrList>::const_iterator iter( aVector.begin() );
    for (; iter != aVector.end(); iter++) {
      ofs << iter->size() << " " << std::endl;
      tOrderedAddrList_lru_iter listiter(iter->begin());
      tOrderedAddrList_lru_iter listiter_end(iter->end());
      for (; listiter != listiter_end; listiter++) {
        ofs << std::hex << *listiter << " ";
      }
      ofs << std::dec << std::endl;
    }
    ofs << std::endl;
    ofs << '-' << std::endl;
    DBG_(Dev, ( << "Saved " << aName));
  }

  void loadVector( std::ifstream & ifs, std::string const & aName, std::vector<tOrderedAddrList> & aVector ) {
    std::string theName;
    ifs >> theName;
    DBG_Assert(aName == theName, ( << "Attempted to load state for " << theName << " while expecting " << aName));
    uint32_t aSize;
    tAddress temp;
    ifs >> aSize;
    DBG_Assert(aSize == aVector.size(), ( << "Vector " << aName << " size in file " << aSize << " does not match configured size " << aVector.size()));
    int32_t anIndex(0);
    while (aSize--) {
      int32_t aListSize;
      ifs >> aListSize;
      DBG_(Dev, ( << "  loading index " << aSize << " of size " << aListSize ));
      aVector[anIndex].clear();
      while (aListSize--) {
        ifs >> std::hex >> temp >> std::dec;
        aVector[anIndex].push_back(temp);
      }
      ++anIndex;
    }
    char tmpc;
    ifs >> tmpc;
    DBG_Assert(tmpc == '-', ( << "End of list marker not found, found: " << tmpc));
    DBG_(Dev, ( << "Loaded " << aName));
  }

  void saveVector( std::ofstream & ofs, std::string const & aName, std::vector<std::vector<int> > const & aVector ) {
    ofs << aName << " " << aVector.size() << std::endl;
    std::vector<std::vector<int> >::const_iterator iter( aVector.begin() );
    for (; iter != aVector.end(); iter++) {
      ofs << iter->size() << " " << std::endl;
      std::vector<int>::const_iterator listiter(iter->begin());
      std::vector<int>::const_iterator listiter_end(iter->end());
      for (; listiter != listiter_end; listiter++) {
        ofs << std::hex << *listiter << " ";
      }
      ofs << std::dec << std::endl;
    }
    ofs << std::endl;
    ofs << '-' << std::endl;
    DBG_(Dev, ( << "Saved " << aName));
  }

  void loadVector( std::ifstream & ifs, std::string const & aName, std::vector<std::vector<int> > & aVector ) {
    std::string theName;
    ifs >> theName;
    DBG_Assert(aName == theName, ( << "Attempted to load state for " << theName << " while expecting " << aName));
    uint32_t aSize;
    tAddress temp;
    ifs >> aSize;
    DBG_Assert(aSize == aVector.size(), ( << "Vector " << aName << " size in file " << aSize << " does not match configured size " << aVector.size()));
    int32_t anIndex(0);
    while (aSize--) {
      int32_t aListSize;
      ifs >> aListSize;
      aVector[anIndex].clear();
      while (aListSize--) {
        ifs >> std::hex >> temp >> std::dec;
        aVector[anIndex].push_back(temp);
      }
      ++anIndex;
    }
    char tmpc;
    ifs >> tmpc;
    DBG_Assert(tmpc == '-', ( << "End of list marker not found, found: " << tmpc));
    DBG_(Dev, ( << "Loaded " << aName));
  }

  void saveVector( std::ofstream & ofs, std::string const & aName, std::vector<std::set<tAddress> > const & aVector ) {
    ofs << aName << " " << aVector.size() << std::endl;
    std::vector<std::set<tAddress> >::const_iterator iter( aVector.begin() );
    for (; iter != aVector.end(); iter++) {
      ofs << iter->size() << " " << std::endl;
      std::set<tAddress>::const_iterator listiter(iter->begin());
      std::set<tAddress>::const_iterator listiter_end(iter->end());
      for (; listiter != listiter_end; listiter++) {
        ofs << std::hex << *listiter << " ";
      }
      ofs << std::dec << std::endl;
    }
    ofs << std::endl;
    ofs << '-' << std::endl;
    DBG_(Dev, ( << "Saved " << aName));
  }

  void loadVector( std::ifstream & ifs, std::string const & aName, std::vector<std::set<tAddress> > & aVector ) {
    std::string theName;
    ifs >> theName;
    DBG_Assert(aName == theName, ( << "Attempted to load state for " << theName << " while expecting " << aName));
    uint32_t aSize;
    tAddress temp;
    ifs >> aSize;
    DBG_Assert(aSize == aVector.size(), ( << "Vector " << aName << " size in file " << aSize << " does not match configured size " << aVector.size()));
    int32_t anIndex(0);
    while (aSize--) {
      int32_t aListSize;
      ifs >> aListSize;
      aVector[anIndex].clear();
      while (aListSize--) {
        ifs >> std::hex >> temp >> std::dec;
        aVector[anIndex].insert(temp);
      }
      ++anIndex;
    }
    char tmpc;
    ifs >> tmpc;
    DBG_Assert(tmpc == '-', ( << "End of list marker not found, found: " << tmpc));
    DBG_(Dev, ( << "Loaded " << aName));
  }

  template<typename _t> void loadVector( std::ifstream & ifs, std::string const & aName, std::vector<_t> & aVector ) {
    std::string theName;
    ifs >> theName;
    DBG_Assert(aName == theName, ( << "Attempted to load state for " << theName << " while expecting " << aName));
    int32_t aSize;
    _t temp;
    ifs >> aSize;
    aVector.clear();
    while (aSize--) {
      ifs >> temp;
      aVector.push_back(temp);
    }
    char tmpc;
    ifs >> tmpc;
    DBG_Assert(tmpc == '-', ( << "End of list marker not found, found: " << tmpc));
    DBG_(Dev, ( << "Loaded " << aName));
  }

  // RNUCA design with instruction storage replication
  uint32_t theSizeOfInstrCluster;                     // the cluster size for instruction replication
  uint32_t theSizeOfInstrClusterLog2;
  std::vector<uint32_t> theCoreInstrInterleavingID;   // interleaving IDs for indexing instruction replicas

  uint32_t theSizeOfPrivCluster;                      // the cluster size for private data
  uint32_t theSizeOfPrivClusterLog2;
  std::vector<uint32_t> theCorePrivInterleavingID;    // interleaving IDs for indexing private data
  /* CMU-ONLY-BLOCK-END */

  int32_t theIdxExtraOffsetBitsInstr;
  int32_t theIdxExtraOffsetBitsShared;
  int32_t theIdxExtraOffsetBitsPrivate;

  enum tAccessClass {
    kInstruction
    , kPrivateData
    , kSharedData
    , kDMA
  };
  tAccessClass pendingL2AccessClass;

  /* CMU-ONLY-BLOCK-BEGIN */
  bool theReclassifyInProgress;  // indicate the type of the reclassification in progress or kNONE
  int32_t theReclassifyHops;         // additional round-trip hops for reclassifying data to instructions or private data to shared

  uint32_t theCachedLineBitmaskComponentSizeLog2;
  uint32_t theCachedLineBitmaskComponents;
  uint64_t theCachedLineBitmaskLowMask;

  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_I_Fetch;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_P_Read;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_P_Write;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_P_Upg;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_P_Atomic;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_S_Read;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_S_Write;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_S_Upg;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_S_Atomic;

  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_I_Fetch;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_P_Read;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_P_Write;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_P_Upg;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_P_Atomic;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_S_Read;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_S_Write;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_S_Upg;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_S_Atomic;

  Stat::StatInstanceCounter<int64_t> theReplyHops_I_Fetch;
  Stat::StatInstanceCounter<int64_t> theReplyHops_P_Read;
  Stat::StatInstanceCounter<int64_t> theReplyHops_P_Write;
  Stat::StatInstanceCounter<int64_t> theReplyHops_P_Upg;
  Stat::StatInstanceCounter<int64_t> theReplyHops_P_Atomic;
  Stat::StatInstanceCounter<int64_t> theReplyHops_S_Read;
  Stat::StatInstanceCounter<int64_t> theReplyHops_S_Write;
  Stat::StatInstanceCounter<int64_t> theReplyHops_S_Upg;
  Stat::StatInstanceCounter<int64_t> theReplyHops_S_Atomic;

  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_I_Fetch;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_P_Read;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_P_Write;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_P_Upg;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_P_Atomic;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_S_Read;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_S_Write;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_S_Upg;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_S_Atomic;

  Stat::StatCounter theOffChip_I_Fetch;
  Stat::StatCounter theOffChip_I_Evict;
  Stat::StatCounter theOffChip_P_Read;
  Stat::StatCounter theOffChip_P_Write;
  Stat::StatCounter theOffChip_P_Upg;
  Stat::StatCounter theOffChip_P_Atomic;
  Stat::StatCounter theOffChip_P_EvictClean;
  Stat::StatCounter theOffChip_P_EvictDirty;
  Stat::StatCounter theOffChip_P_EvictWritable;
  Stat::StatCounter theOffChip_S_Read;
  Stat::StatCounter theOffChip_S_Write;
  Stat::StatCounter theOffChip_S_Upg;
  Stat::StatCounter theOffChip_S_Atomic;
  Stat::StatCounter theOffChip_S_EvictClean;
  Stat::StatCounter theOffChip_S_EvictDirty;
  Stat::StatCounter theOffChip_S_EvictWritable;

  Stat::StatCounter theReclassifiedPages_PrivateDataWithInstr;
  Stat::StatCounter theReclassifiedPages_SharedDataWithInstr;
  Stat::StatCounter theReclassifiedPages_PrivateToShared;
  Stat::StatInstanceCounter<int64_t> theReclassifiedLines_PrivateToShared;
  Stat::StatInstanceCounter<int64_t> theReclassifyPrivateToSharedHops;
  Stat::StatCounter theDataRefsInInstrDataPage;
  Stat::StatCounter theInstrRefsInInstrDataPage;

  Stat::StatCounter theDecoupledLines_Data;
  Stat::StatCounter theDecoupledLines_Instr;

  // Private L2 design or Shared L2 design
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_D_Read;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_D_Write;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_D_Upg;
  Stat::StatInstanceCounter<int64_t> theCoreToL2Hops_D_Atomic;

  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_D_Read;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_D_Write;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_D_Upg;
  Stat::StatInstanceCounter<int64_t> thePeerL1Hops_D_Atomic;

  Stat::StatInstanceCounter<int64_t> theReplyHops_D_Read;
  Stat::StatInstanceCounter<int64_t> theReplyHops_D_Write;
  Stat::StatInstanceCounter<int64_t> theReplyHops_D_Upg;
  Stat::StatInstanceCounter<int64_t> theReplyHops_D_Atomic;

  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_D_Read;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_D_Write;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_D_Upg;
  Stat::StatInstanceCounter<int64_t> theReqRoundTripHops_D_Atomic;

  Stat::StatCounter theOffChip_D_Read;
  Stat::StatCounter theOffChip_D_Write;
  Stat::StatCounter theOffChip_D_Upg;
  Stat::StatCounter theOffChip_D_Atomic;
  Stat::StatCounter theOffChip_D_EvictClean;
  Stat::StatCounter theOffChip_D_EvictDirty;
  Stat::StatCounter theOffChip_D_EvictWritable;

  // Private L2 designs only
  Stat::StatInstanceCounter<int64_t> theDirHops_I_Fetch;

  Stat::StatInstanceCounter<int64_t> theDirHops_D_Read;
  Stat::StatInstanceCounter<int64_t> theDirHops_D_Write;
  Stat::StatInstanceCounter<int64_t> theDirHops_D_Upg;
  Stat::StatInstanceCounter<int64_t> theDirHops_D_Atomic;

  Stat::StatInstanceCounter<int64_t> theSnoopedL2Hops_I_Fetch;

  Stat::StatInstanceCounter<int64_t> theSnoopedL2Hops_D_Read;
  Stat::StatInstanceCounter<int64_t> theSnoopedL2Hops_D_Write;
  Stat::StatInstanceCounter<int64_t> theSnoopedL2Hops_D_Upg;
  Stat::StatInstanceCounter<int64_t> theSnoopedL2Hops_D_Atomic;
  /* CMU-ONLY-BLOCK-END */

  // message handlers for snoops generated at this level and evictions
  MemoryMessage theSnoopMessage;
  MemoryMessage theDirSnoopMessage;

  /* CMU-ONLY-BLOCK-BEGIN */
  // XJ:
  Stat::StatInstanceCounter<int64_t> theDirFoundOffset;
  /* CMU-ONLY-BLOCK-END */

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastCMPNetworkController)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )

/* CMU-ONLY-BLOCK-BEGIN */
    , theReclassifyInProgress(false)

    , theCoreToL2Hops_I_Fetch(statName() + "-CoreToL2Hops:Instr:Fetch")
    , theCoreToL2Hops_P_Read(statName() + "-CoreToL2Hops:Priv:Read")
    , theCoreToL2Hops_P_Write(statName() + "-CoreToL2Hops:Priv:Write")
    , theCoreToL2Hops_P_Upg(statName() + "-CoreToL2Hops:Priv:Upg")
    , theCoreToL2Hops_P_Atomic(statName() + "-CoreToL2Hops:Priv:Atomic")
    , theCoreToL2Hops_S_Read(statName() + "-CoreToL2Hops:Shared:Read")
    , theCoreToL2Hops_S_Write(statName() + "-CoreToL2Hops:Shared:Write")
    , theCoreToL2Hops_S_Upg(statName() + "-CoreToL2Hops:Shared:Upg")
    , theCoreToL2Hops_S_Atomic(statName() + "-CoreToL2Hops:Shared:Atomic")

    , thePeerL1Hops_I_Fetch(statName() + "-PeerL1Hops:Instr:Fetch")
    , thePeerL1Hops_P_Read(statName() + "-PeerL1Hops:Priv:Read")
    , thePeerL1Hops_P_Write(statName() + "-PeerL1Hops:Priv:Write")
    , thePeerL1Hops_P_Upg(statName() + "-PeerL1Hops:Priv:Upg")
    , thePeerL1Hops_P_Atomic(statName() + "-PeerL1Hops:Priv:Atomic")
    , thePeerL1Hops_S_Read(statName() + "-PeerL1Hops:Shared:Read")
    , thePeerL1Hops_S_Write(statName() + "-PeerL1Hops:Shared:Write")
    , thePeerL1Hops_S_Upg(statName() + "-PeerL1Hops:Shared:Upg")
    , thePeerL1Hops_S_Atomic(statName() + "-PeerL1Hops:Shared:Atomic")

    , theReplyHops_I_Fetch(statName() + "-ReplyHops:Instr:Fetch")
    , theReplyHops_P_Read(statName() + "-ReplyHops:Priv:Read")
    , theReplyHops_P_Write(statName() + "-ReplyHops:Priv:Write")
    , theReplyHops_P_Upg(statName() + "-ReplyHops:Priv:Upg")
    , theReplyHops_P_Atomic(statName() + "-ReplyHops:Priv:Atomic")
    , theReplyHops_S_Read(statName() + "-ReplyHops:Shared:Read")
    , theReplyHops_S_Write(statName() + "-ReplyHops:Shared:Write")
    , theReplyHops_S_Upg(statName() + "-ReplyHops:Shared:Upg")
    , theReplyHops_S_Atomic(statName() + "-ReplyHops:Shared:Atomic")

    , theReqRoundTripHops_I_Fetch(statName() + "-ReqRoundTripHops:Instr:Fetch")
    , theReqRoundTripHops_P_Read(statName() + "-ReqRoundTripHops:Priv:Read")
    , theReqRoundTripHops_P_Write(statName() + "-ReqRoundTripHops:Priv:Write")
    , theReqRoundTripHops_P_Upg(statName() + "-ReqRoundTripHops:Priv:Upg")
    , theReqRoundTripHops_P_Atomic(statName() + "-ReqRoundTripHops:Priv:Atomic")
    , theReqRoundTripHops_S_Read(statName() + "-ReqRoundTripHops:Shared:Read")
    , theReqRoundTripHops_S_Write(statName() + "-ReqRoundTripHops:Shared:Write")
    , theReqRoundTripHops_S_Upg(statName() + "-ReqRoundTripHops:Shared:Upg")
    , theReqRoundTripHops_S_Atomic(statName() + "-ReqRoundTripHops:Shared:Atomic")

    , theOffChip_I_Fetch(statName() + "-OffChip:Instr:Fetch")
    , theOffChip_I_Evict(statName() + "-OffChip:Instr:Evict")
    , theOffChip_P_Read(statName() + "-OffChip:Priv:Read")
    , theOffChip_P_Write(statName() + "-OffChip:Priv:Write")
    , theOffChip_P_Upg(statName() + "-OffChip:Priv:Upg")
    , theOffChip_P_Atomic(statName() + "-OffChip:Priv:Atomic")
    , theOffChip_P_EvictClean(statName() + "-OffChip:Priv:EvictClean")
    , theOffChip_P_EvictDirty(statName() + "-OffChip:Priv:EvictDirty")
    , theOffChip_P_EvictWritable(statName() + "-OffChip:Priv:EvictWritable")
    , theOffChip_S_Read(statName() + "-OffChip:Shared:Read")
    , theOffChip_S_Write(statName() + "-OffChip:Shared:Write")
    , theOffChip_S_Upg(statName() + "-OffChip:Shared:Upg")
    , theOffChip_S_Atomic(statName() + "-OffChip:Shared:Atomic")
    , theOffChip_S_EvictClean(statName() + "-OffChip:Shared:EvictClean")
    , theOffChip_S_EvictDirty(statName() + "-OffChip:Shared:EvictDirty")
    , theOffChip_S_EvictWritable(statName() + "-OffChip:Shared:EvictWritable")

    , theReclassifiedPages_PrivateDataWithInstr(statName() + "-ReclassifiedPages:PI")
    , theReclassifiedPages_SharedDataWithInstr(statName() + "-ReclassifiedPages:SI")
    , theReclassifiedPages_PrivateToShared(statName() + "-ReclassifiedPages:P2S")
    , theReclassifiedLines_PrivateToShared(statName() + "-ReclassifiedLines:P2S")
    , theReclassifyPrivateToSharedHops(statName() + "-ReclassifyHops:PrivToShared")

    , theDataRefsInInstrDataPage(statName() + "-RefsInInstrDataPage:Data")
    , theInstrRefsInInstrDataPage(statName() + "-RefsInInstrDataPage:Instr")

    , theDecoupledLines_Data(statName() + "-DecoupledLines:Data")
    , theDecoupledLines_Instr(statName() + "-DecoupledLines:Instr")

    , theCoreToL2Hops_D_Read(statName() + "-CoreToL2Hops:Data:Read")
    , theCoreToL2Hops_D_Write(statName() + "-CoreToL2Hops:Data:Write")
    , theCoreToL2Hops_D_Upg(statName() + "-CoreToL2Hops:Data:Upg")
    , theCoreToL2Hops_D_Atomic(statName() + "-CoreToL2Hops:Data:Atomic")

    , thePeerL1Hops_D_Read(statName() + "-PeerL1Hops:Data:Read")
    , thePeerL1Hops_D_Write(statName() + "-PeerL1Hops:Data:Write")
    , thePeerL1Hops_D_Upg(statName() + "-PeerL1Hops:Data:Upg")
    , thePeerL1Hops_D_Atomic(statName() + "-PeerL1Hops:Data:Atomic")

    , theReplyHops_D_Read(statName() + "-ReplyHops:Data:Read")
    , theReplyHops_D_Write(statName() + "-ReplyHops:Data:Write")
    , theReplyHops_D_Upg(statName() + "-ReplyHops:Data:Upg")
    , theReplyHops_D_Atomic(statName() + "-ReplyHops:Data:Atomic")

    , theReqRoundTripHops_D_Read(statName() + "-ReqRoundTripHops:Data:Read")
    , theReqRoundTripHops_D_Write(statName() + "-ReqRoundTripHops:Data:Write")
    , theReqRoundTripHops_D_Upg(statName() + "-ReqRoundTripHops:Data:Upg")
    , theReqRoundTripHops_D_Atomic(statName() + "-ReqRoundTripHops:Data:Atomic")

    , theOffChip_D_Read(statName() + "-OffChip:Data:Read")
    , theOffChip_D_Write(statName() + "-OffChip:Data:Write")
    , theOffChip_D_Upg(statName() + "-OffChip:Data:Upg")
    , theOffChip_D_Atomic(statName() + "-OffChip:Data:Atomic")
    , theOffChip_D_EvictClean(statName() + "-OffChip:Data:EvictClean")
    , theOffChip_D_EvictDirty(statName() + "-OffChip:Data:EvictDirty")
    , theOffChip_D_EvictWritable(statName() + "-OffChip:Data:EvictWritable")

    , theDirHops_I_Fetch(statName() + "-DirHops:Instr:Fetch")

    , theDirHops_D_Read(statName() + "-DirHops:Data:Read")
    , theDirHops_D_Write(statName() + "-DirHops:Data:Write")
    , theDirHops_D_Upg(statName() + "-DirHops:Data:Upg")
    , theDirHops_D_Atomic(statName() + "-DirHops:Data:Atomic")

    , theSnoopedL2Hops_I_Fetch(statName() + "-SnoopedL2Hops:Instr:Fetch")

    , theSnoopedL2Hops_D_Read(statName() + "-SnoopedL2Hops:Data:Read")
    , theSnoopedL2Hops_D_Write(statName() + "-SnoopedL2Hops:Data:Write")
    , theSnoopedL2Hops_D_Upg(statName() + "-SnoopedL2Hops:Data:Upg")
    , theSnoopedL2Hops_D_Atomic(statName() + "-SnoopedL2Hops:Data:Atomic")
/* CMU-ONLY-BLOCK-END */

    , theSnoopMessage(MemoryMessage::Invalidate)
    , theDirSnoopMessage(MemoryMessage::Invalidate)

/* CMU-ONLY-BLOCK-BEGIN */
    // XJ:
    , theDirFoundOffset(statName() + "DirFoundOffset")
/* CMU-ONLY-BLOCK-END */

  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void finalize( void ) {
    // theStats->update();
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// init

  void initialize(void) {
    // Confirm there are enough bits for the NumCores and NumL2Slices
    DBG_Assert(sizeof(index_t) * 8 >= log_base2(cfg.NumCores * cfg.NumL2Slices));
    DBG_Assert(sizeof(tCoreID) * 8 >= log_base2(cfg.NumCores));
    DBG_Assert(sizeof(tL2SliceID) * 8 >= log_base2(cfg.NumL2Slices));

    theNumCores = (cfg.NumCores ? cfg.NumCores : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    theNumL2Slices = (cfg.NumL2Slices ? cfg.NumL2Slices : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    theNumCoresPerL2Slice = theNumCores / theNumL2Slices;
    theNumL2SlicesLog2 = log_base2(theNumL2Slices);

    // At most 64 cores or L2 slices
    DBG_Assert ( theNumCores <= 64, ( << "This implementation only supports up to 64 cores" ) );
    DBG_Assert ( theNumL2Slices <= 64, ( << "This implementation only supports up to 64 L2 slices" ) );

    /* CMU-ONLY-BLOCK-BEGIN */
    // ASR
    theBenefitOfIncreasingReplication.resize(theNumL2Slices);
    theCostOfIncreasingReplication.resize(theNumL2Slices);
    theBenefitOfDecreasingReplication.resize(theNumL2Slices);
    theCostOfDecreasingReplication.resize(theNumL2Slices);
    theNLHB.resize(theNumL2Slices);
    theSoonToBeEvicted.resize(theNumL2Slices);
    theVTB.resize(theNumL2Slices);
    theCurrentReplicationOnly.resize(theNumL2Slices);
    theASR_ReplicationLevel.resize(theNumL2Slices);
    theASR_LocalReplicationCount.resize(theNumL2Slices);
    theASR_NLHBAllocationCount.resize(theNumL2Slices);
    theASR_LastEvaluations.resize(theNumL2Slices);
    for (uint32_t i = 0; i < theNumL2Slices; ++i) {
      theASR_LastEvaluations[i].resize(4);
    }

    for (std::vector<int>::iterator i = theASR_ReplicationLevel.begin(); i != theASR_ReplicationLevel.end(); ++i) {
      *i = 5;
    }
    // assign values  64: allocate 25%
    // assign values 128: allocate 50%
    // assign values 192: allocate 75%
    theASR_ReplicationThresholds[0] =   0 /*   0 */; // these levels are 1 higher than in the paper to avoid corner cases
    theASR_ReplicationThresholds[1] =   0 /*   0 */;
    theASR_ReplicationThresholds[2] =   4 /*   4 */;
    theASR_ReplicationThresholds[3] =  16 /*  16 */;
    theASR_ReplicationThresholds[4] =  64 /*  64 */;
    theASR_ReplicationThresholds[5] = 128 /* 128 */;
    theASR_ReplicationThresholds[6] = 256 /* 256 */;
    theASR_ReplicationThresholds[7] = 256 /* 256 */;
    /* CMU-ONLY-BLOCK-END */

    // Confirm that PageSize is a power of 2
    DBG_Assert( (cfg.PageSize & (cfg.PageSize - 1)) == 0);
    DBG_Assert( cfg.PageSize  >= 4);

    // Calculate shifts and masks for pages
    thePageSize = cfg.PageSize;
    thePageMask = ~(cfg.PageSize - 1);
    thePageSizeLog2 = log_base2(cfg.PageSize);

    // Confirm that CacheLineSize is a power of 2
    DBG_Assert( (cfg.CacheLineSize & (cfg.CacheLineSize - 1)) == 0);
    DBG_Assert( cfg.CacheLineSize  >= 4);

    // Calculate shifts and masks for cache lines
    theCacheLineSize = cfg.CacheLineSize;
    theCacheLineMask = ~(cfg.CacheLineSize - 1);
    theCacheLineSizeLog2 = log_base2(cfg.CacheLineSize);

    // L2 design
    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.Placement == "R-NUCA") {
      thePlacement = kRNUCACache;
    } else
      /* CMU-ONLY-BLOCK-END */
      if (cfg.Placement == "private") {
        thePlacement = kPrivateCache;
      } else if (cfg.Placement == "shared") {
        thePlacement = kSharedCache;
      } else {
        DBG_Assert(false, ( << "Unknown L2 design") );
      }

    /* CMU-ONLY-BLOCK-BEGIN */
    thePrivateWithASR = cfg.PrivateWithASR;
    DBG_Assert(!thePrivateWithASR || thePlacement == kPrivateCache); // turn on ASR only if private-nuca
    /* CMU-ONLY-BLOCK-END */

    DBG_Assert(theNumCores == theNumL2Slices, ( << "I have no idea what will break if this is not true, but something probably will") );

    /* CMU-ONLY-BLOCK-BEGIN */
    if (thePlacement == kRNUCACache) {
      routeCoreReq = boost::bind( &FastCMPNetworkControllerComponent::routeCoreReq_RNUCA, this, _1, _2);
      propagateSnoopUp = boost::bind( &FastCMPNetworkControllerComponent::propagateSnoopUp_RNUCA, this, _1, _2);
      updatePeerL1HopsStats = boost::bind( &FastCMPNetworkControllerComponent::updatePeerL1HopsStats_RNUCA, this, _1, _2);
      updateReplyHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReplyHopsStats_RNUCA, this, _1, _2);
      updateReqRoundTripHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReqRoundTripHopsStats_RNUCA, this, _1);
      updateOffChipStats = boost::bind( &FastCMPNetworkControllerComponent::updateOffChipStats_RNUCA, this, _1, _2);
    } else
      /* CMU-ONLY-BLOCK-END */
      if (thePlacement == kPrivateCache) {

        /* CMU-ONLY-BLOCK-BEGIN */
        if (thePrivateWithASR)
          routeCoreReq = boost::bind( &FastCMPNetworkControllerComponent::routeCoreReq_ASR, this, _1, _2);
        else
          /* CMU-ONLY-BLOCK-END */
          routeCoreReq = boost::bind( &FastCMPNetworkControllerComponent::routeCoreReq_Private, this, _1, _2);

        propagateSnoopUp = boost::bind( &FastCMPNetworkControllerComponent::propagateSnoopUp_Private, this, _1, _2);
        updatePeerL1HopsStats = boost::bind( &FastCMPNetworkControllerComponent::updatePeerL1HopsStats_Private, this, _1, _2); /* CMU-ONLY */
        updateReplyHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReplyHopsStats_Private, this, _1, _2); /* CMU-ONLY */
        updateReqRoundTripHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReqRoundTripHopsStats_Private, this, _1); /* CMU-ONLY */
        updateOffChipStats = NULL; /* CMU-ONLY */
      } else if (thePlacement == kSharedCache) {
        routeCoreReq = boost::bind( &FastCMPNetworkControllerComponent::routeCoreReq_Shared, this, _1, _2);
        propagateSnoopUp = boost::bind( &FastCMPNetworkControllerComponent::propagateSnoopUp_Shared, this, _1, _2);
        updatePeerL1HopsStats = boost::bind( &FastCMPNetworkControllerComponent::updatePeerL1HopsStats_Shared, this, _1, _2); /* CMU-ONLY */
        updateReplyHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReplyHopsStats_Shared, this, _1, _2); /* CMU-ONLY */
        updateReqRoundTripHopsStats = boost::bind( &FastCMPNetworkControllerComponent::updateReqRoundTripHopsStats_Shared, this, _1); /* CMU-ONLY */
        updateOffChipStats = boost::bind( &FastCMPNetworkControllerComponent::updateOffChipStats_Shared, this, _1, _2); /* CMU-ONLY */
      } else {
        DBG_Assert( false );
      }
    theASRRandomContext = 0; /* CMU-ONLY */

    DBG_( Crit, ( << "Running with "
                  << theNumCores << " cores"
                  << ", " << theNumL2Slices
                  << (
                    thePlacement == kRNUCACache ? " R-NUCA" /* CMU-ONLY */
                    : thePrivateWithASR ? " ASR"            /* CMU-ONLY */
                    :                                       /* CMU-ONLY */
                    thePlacement == kPrivateCache ? " private"
                    : " shared"
                  )
                  << " L2 slices"
                  << ", " << cfg.PageSize << "-byte page"
                  << ", " << cfg.CacheLineSize << "-byte cache line"
                  << ", " << cfg.FramesPerL2Slice << " frames per slice" /* CMU-ONLY */
                ) );

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

    // calculate hop array for core0 with BFS
    DBG_Assert( theNumCores <= 64 );
    theHops[ 0 ][ 0 ] = 0;
    for (uint32_t i = 1; i < theNumCores; i++) {
      theHops[ 0 ][ i ] = -1;
    }
    std::deque< uint32_t > stack;
    stack.push_back(0);
    for (uint32_t i = 1; i < theNumCores; i++) {
      uint32_t idx = stack.front();
      stack.pop_front();
      if (theHops[ 0 ][ theNTileID[idx] ] == -1) {
        theHops[ 0 ][ theNTileID[idx] ] = theHops[ 0 ][ idx ] + 1;
        stack.push_back( theNTileID[idx] );
      }
      if (theHops[ 0 ][ theSTileID[idx] ] == -1) {
        theHops[ 0 ][ theSTileID[idx] ] = theHops[ 0 ][ idx ] + 1;
        stack.push_back( theSTileID[idx] );
      }
      if (theHops[ 0 ][ theETileID[idx] ] == -1) {
        theHops[ 0 ][ theETileID[idx] ] = theHops[ 0 ][ idx ] + 1;
        stack.push_back( theETileID[idx] );
      }
      if (theHops[ 0 ][ theWTileID[idx] ] == -1) {
        theHops[ 0 ][ theWTileID[idx] ] = theHops[ 0 ][ idx ] + 1;
        stack.push_back( theWTileID[idx] );
      }
    }

    // calculate hop array for each core through permutations of core0 hop array
    for (uint32_t i = 1; i < theNumCores; i++) {
      const uint32_t Xdiff = i % theNumCoresPerTorusRow;
      const uint32_t Ydiff = i / theNumCoresPerTorusRow;
      for (uint32_t row = 0; row < theNumCores / theNumCoresPerTorusRow; row++) {
        uint32_t src  = row * theNumCoresPerTorusRow;
        uint32_t dest = ( (row + Ydiff) * theNumCoresPerTorusRow + Xdiff ) % theNumCores;
        for (uint32_t j = 0; j < theNumCoresPerTorusRow; j++) {
          theHops[ i ][ dest ] = theHops[ 0 ][ src ];
          dest ++;
          if (dest % theNumCoresPerTorusRow == 0) {
            dest -= theNumCoresPerTorusRow;
          }
          src ++;
        }
      }
    }

    /*
    for (uint32_t i=0; i < theNumCores; i++) {
      DBG_( Crit, ( << "Torus distances for core " << i));
      for (uint32_t j=0; j < theNumCores; j++) {
        std::cout << " " << theHops[ i ][ j ];
        if ( (j % theNumCoresPerTorusRow) == theNumCoresPerTorusRow - 1 ) {
          std::cout << std::endl;
        }
      }
      std::cout << std::endl;
    }
    */

    /* CMU-ONLY-BLOCK-BEGIN */
    if (thePlacement == kRNUCACache) {

      uint64_t theCachedLineBitmaskComponentSize = sizeof(uint64_t) * 8;
      theCachedLineBitmaskComponents = cfg.PageSize / cfg.CacheLineSize / theCachedLineBitmaskComponentSize;
      theCachedLineBitmaskComponentSizeLog2 = log_base2(theCachedLineBitmaskComponentSize);
      theCachedLineBitmaskLowMask = (theCachedLineBitmaskComponentSize - 1);
      DBG_Assert(theCachedLineBitmaskComponents <= 2, ( << "The current implementation supports at most 128 cache lines per page."));

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

      // make sure the rotational interleaving function works for data
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
    }
    /* CMU-ONLY-BLOCK-END */

    // intialize the directory for the private cache
    // sharers bitmask size for up to 64 nodes
    uint32_t theSharersBitmaskComponentSize = sizeof(tSharers) * 8;
    theSharersBitmaskComponents = 64 * 2 / theSharersBitmaskComponentSize;
    theSharersBitmaskComponentSizeLog2 = log_base2(theSharersBitmaskComponentSize);
    theSharersBitmaskLowMask = (theSharersBitmaskComponentSize - 1);
    DBG_Assert(theSharersBitmaskComponents <= 2, ( << "The current implementation supports at most 64 nodes."));

    // inval dir entry
    clearDirEntry(anInvalChipDirEntry);

    // intialize the directory FSM
    fillChipDirFSM();

    /* CMU-ONLY-BLOCK-BEGIN */
    // XJ:
    // if DirEntries == 0 we have an unbounded directory
    if (cfg.DirEntries != 0) {
      // Dir Entries must be greater than 0 and a power of 2
      DBG_Assert( (cfg.DirEntries & (cfg.DirEntries - 1)) == 0);
      DBG_Assert( cfg.DirEntries  >= 1);

      // Dir Ways number must be greater than 0
      DBG_Assert( cfg.DirWays  >= 1);

      // Initializing the bounded directory
      theDirNumSets = theNumL2Slices * cfg.DirEntries / cfg.DirWays;
      theDirSetMask = (theDirNumSets - 1);
      theDirEntrySets = (tCMPBoundedDirEntry *) malloc(sizeof(tCMPBoundedDirEntry) * theDirNumSets * cfg.DirWays);
    }
    /* CMU-ONLY-BLOCK-END */

    ///////////////////////////
    // HACK: initialize values here to get rid of annoying DMA asserts.
    // Must fix this at some point.
    // It only affects stats, and DMAs are rare, so it should be fine even if I leave it as is.
    //
    /* CMU-ONLY-BLOCK-BEGIN */
    coreToL2Hops = 0;
    dirHops = 0;
    snoopedL2Hops = 0;
    peerL1Hops = 0;
    replyHops = 0;
    /* CMU-ONLY-BLOCK-END */

    theProbedL2 = -1;
    theSnoopedL2 = -1;
    theDir = -1;
    thePeerL1 = -1;

    theOffChipRequest = kNoOffChipReq;
    theRemoteL2Hit = false; /* CMU-ONLY */
    theASR_InsertInVTB = false; /* CMU-ONLY */
    theASR_NLHBInsert = false; /* CMU-ONLY */

    theType = MemoryMessage::ReadReq;

    theL2Dstream = true;
    theL2Type = MemoryMessage::ReadReq;
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// ports

  void processRequestInMessage(  index_t         anIndex
                                 , MemoryMessage & aMessage
                              ) {
    //keep a snapshot of the system for reclassifications
    /* CMU-ONLY-BLOCK-BEGIN */
    int32_t tmp_coreToL2Hops = coreToL2Hops;
    int32_t tmp_dirHops = dirHops;
    int32_t tmp_snoopedL2Hops = snoopedL2Hops;
    int32_t tmp_peerL1Hops = peerL1Hops;
    int32_t tmp_replyHops = replyHops;

    int32_t tmp_theProbedL2 = theProbedL2;
    int32_t tmp_theSnoopedL2 = theSnoopedL2;
    int32_t tmp_theDir = theDir;
    int32_t tmp_thePeerL1 = thePeerL1;
    bool tmp_theDstream = theDstream;
    MemoryMessage::MemoryMessageType tmp_theType = theType;

    if (theReclassifyInProgress) {
      // page reclassification causes only cache block evicts.
      DBG_Assert(   aMessage.type() == MemoryMessage::EvictClean
                    || aMessage.type() == MemoryMessage::EvictWritable
                    || aMessage.type() == MemoryMessage::EvictDirty
                );
    }

    coreToL2Hops = 0;
    dirHops = 0;
    snoopedL2Hops = 0;
    peerL1Hops = 0;
    replyHops = 0;
    /* CMU-ONLY-BLOCK-END */

    theProbedL2 = -1;
    theSnoopedL2 = -1;
    theDir = -1;
    thePeerL1 = -1;

    theOffChipRequest = kNoOffChipReq;
    theRemoteL2Hit = false; /* CMU-ONLY */
    theASR_InsertInVTB = false; /* CMU-ONLY */
    theASR_NLHBInsert = false; /* CMU-ONLY */

    theType = aMessage.type();

    /* CMU-ONLY-BLOCK-BEGIN */
    bool anASR_WasAlreadyAllocatedLocally(false);
    if (thePlacement == kPrivateCache && thePrivateWithASR && theType == MemoryMessage::EvictClean) {
      theSnoopMessage.address() = PhysicalMemoryAddress(blockAddress(aMessage.address()));
      theSnoopMessage.type() = MemoryMessage::Probe;
      FLEXUS_CHANNEL_ARRAY( ToL2Snoops, anIndex ) << theSnoopMessage;
      anASR_WasAlreadyAllocatedLocally = (
                                           theSnoopMessage.type() == MemoryMessage::ProbedClean
                                           || theSnoopMessage.type() == MemoryMessage::ProbedDirty
                                           || theSnoopMessage.type() == MemoryMessage::ProbedWritable
                                         );
    }
    /* CMU-ONLY-BLOCK-END */

    theDstream = aMessage.dstream();
    tL2SliceID theL2SlicePort = routeCoreReq(anIndex, aMessage);
    DBG_Assert(theL2SlicePort < theNumCores * theNumL2Slices, ( << "theL2SlicePort=" << theL2SlicePort
               << " theNumCores*theNumL2Slices=" << theNumCores * theNumL2Slices
                                                              ));
    if (aMessage.dstream()) {
      FLEXUS_CHANNEL_ARRAY( RequestOutD, theL2SlicePort ) << aMessage;
    } else {
      FLEXUS_CHANNEL_ARRAY( RequestOutI, theL2SlicePort ) << aMessage;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    // ASR bookkeeping after the request finishes
    const tAddress anAddr     = aMessage.address();
    const tAddress aBlockAddr = blockAddress(anAddr);
    if (thePrivateWithASR) {
      switch (theType) {
        case MemoryMessage::FetchReq:
        case MemoryMessage::LoadReq:
        case MemoryMessage::ReadReq:
        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:     // FIXME: should upgrades be included here? They may access the dir or cause an off-chip request
        case MemoryMessage::UpgradeAllocate:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq: {
          // remote on-chip L2 hit
          if (theRemoteL2Hit) {
            // check local NLHB to see if block would have been available (would have been replicated) if replication level was higher
            tOrderedAddrList & aNLHB(theNLHB[anIndex]);
            tOrderedAddrList_addr_iter aNLHB_entry(aNLHB.get<by_address>().find(aBlockAddr));
            if (aNLHB_entry != aNLHB.get<by_address>().end()) {
              int32_t Hc(cfg.ASRRemoteHitLatency); // Hit latency at Current replication level
              int32_t Hh(cfg.ASRLocalHitLatency); // Hit latency at Higher replication level
              theBenefitOfIncreasingReplication[anIndex] += Hc - Hh;
              aNLHB.get<by_address>().erase(aNLHB_entry);
            }
          }

          // local on-chip L2 hit
          else if (theDir == -1) {
            // track to-be-evicted blocks to see the cost of kicking them out earlier by increasing replication level
            tOrderedAddrList & aSoonToBeEvicted(theSoonToBeEvicted[anIndex]);
            tOrderedAddrList_addr_iter aSoonToBeEvicted_entry(aSoonToBeEvicted.get<by_address>().find(aBlockAddr));
            // update recency list
            if (aSoonToBeEvicted_entry != aSoonToBeEvicted.get<by_address>().end()) {
              // check recency list (to do cost calculations) before modifying the list
              if (std::distance(aSoonToBeEvicted.get<by_LRU>().begin(), aSoonToBeEvicted.project<by_LRU>(aSoonToBeEvicted_entry)) < cfg.MonitorSize) {
                int32_t aOnChipLocation( probeForBlock( anIndex, aMessage.address(), false ) );
                bool aIsBlockPresentSomewhereElseOnChip( aOnChipLocation != -1 );
                DBG_Assert( aOnChipLocation != static_cast<int>(anIndex) ); // FIXME: this will be true if !aIsBlockPresentSomewhereElseOnChip, right?
                if (!aIsBlockPresentSomewhereElseOnChip) {
                  theCostOfIncreasingReplication[anIndex] += cfg.ASROffChipLatency; /* Mh-Mc */
                } // FIXME: else increment by On-chip latency?  looks like ASR does not do this...
              }
              aSoonToBeEvicted.relocate(aSoonToBeEvicted.get<by_LRU>().end(), aSoonToBeEvicted.project<by_LRU>(aSoonToBeEvicted_entry));
            } else {
              aSoonToBeEvicted.get<by_LRU>().push_back(aBlockAddr);
              if (aSoonToBeEvicted.size() == static_cast<uint32_t>(cfg.FramesPerL2Slice)) {
                aSoonToBeEvicted.get<by_LRU>().pop_front();
              }
            }

            // if the block would not be available at lower replication level, compute additional cost of accessing it
            std::set<tAddress>& aCurrentReplicationOnly(theCurrentReplicationOnly[anIndex]);
            std::set<tAddress>::iterator aCurrentReplicationOnly_entry(aCurrentReplicationOnly.find(aBlockAddr));
            if (aCurrentReplicationOnly_entry != aCurrentReplicationOnly.end()) {
              DBG_( VVerb, ( << "L2[" << anIndex << "] Current replication only! " << std::hex << aBlockAddr << std::dec << " CRO size " << aCurrentReplicationOnly.size() ));
              theCostOfDecreasingReplication[anIndex] += cfg.ASRRemoteHitLatency - cfg.ASRLocalHitLatency; /* Hl-Hc */
              aCurrentReplicationOnly.erase(aCurrentReplicationOnly_entry);
            }
          }

          // off-chip access, might have been avoided if only we decreased replication level (this is a hit on a recent victim of replicated block)
          else if (theOffChipRequest != kNoOffChipReq) {
            //DBG_Assert(theOffChipRequest != kNoOffChipReq, (<< "Must be an off-chip request"));
            tOrderedAddrList & aVTB(theVTB[anIndex]);
            tOrderedAddrList_addr_iter aVTB_entry(aVTB.get<by_address>().find(aBlockAddr));
            //DBG_( VTBDbg, ( << "L2[" << anIndex << "] VTB search for " << std::hex << aBlockAddr << std::dec ));
            if (aVTB_entry != aVTB.get<by_address>().end()) {
              theBenefitOfDecreasingReplication[anIndex] += cfg.ASROffChipLatency ; /* Mc-Ml */
              DBG_( VTBDbg, ( << "L2[" << anIndex << "] VTB found! " << std::hex << aBlockAddr << std::dec << " VTB size " << aVTB.size() << " new benefit=" << theBenefitOfDecreasingReplication[anIndex]));
              //aVTB.get<by_address>().erase(aVTB_entry);

            }
          } else {
            // upgrade and write requests that are resolved on chip.
            // ASR doesn't care about them anyway, it only cares about shared RO blocks.
          }

          break;
        }

        case MemoryMessage::EvictClean:

        {

          if (aMessage.type() == MemoryMessage::EvictCleanNonAllocating) {
            if (theASR_NLHBInsert) {
              tOrderedAddrList & aNLHB(theNLHB[theProbedL2]);
              DBG_(Verb, ( << "rand > thresholdHL (" << theASR_ReplicationThresholds[theProbedL2] << ") for L2[" << theProbedL2 << "] NLHB size=" << aNLHB.size() ));
              aNLHB.get<by_LRU>().push_back(aBlockAddr);
              if (aNLHB.size() == static_cast<uint32_t>(cfg.NLHBSize)) {
                aNLHB.get<by_LRU>().pop_front();
              }
              ++theASR_NLHBAllocationCount[anIndex];
            }
          } else {
            DBG_Assert(!theASR_NLHBInsert, ( << "Regular clean evict allocated at this level, so can't request to allocate at 'only' higher level"));
            if (!anASR_WasAlreadyAllocatedLocally) {
              ++theASR_LocalReplicationCount[anIndex];
            }
          }

          if (theASR_LocalReplicationCount[anIndex] == cfg.MonitorSize
              || theASR_NLHBAllocationCount[anIndex] == cfg.MonitorSize) {
            // evaluate replication level change
            int32_t aDeltaIncrease(theBenefitOfIncreasingReplication[anIndex] - theCostOfIncreasingReplication[anIndex]);
            int32_t aDeltaDecrease(theBenefitOfDecreasingReplication[anIndex] - theCostOfDecreasingReplication[anIndex]);
            std::vector<int>& aLastEvals(theASR_LastEvaluations[anIndex]);
            aLastEvals[0] = aLastEvals[1];
            aLastEvals[1] = aLastEvals[2];
            aLastEvals[2] = aLastEvals[3];

            if (aDeltaIncrease > 0 || aDeltaDecrease > 0) {
              if (aDeltaIncrease > 0 && aDeltaDecrease <= 0) {
                // top right box
                aLastEvals[3] = 1;
              } else if (aDeltaIncrease <= 0 && aDeltaDecrease > 0) {
                // bottom left box
                aLastEvals[3] = -1;
              } else {
                // top left box
                if (aDeltaIncrease > aDeltaDecrease) {
                  aLastEvals[3] = 1;
                } else {
                  aLastEvals[3] = -1;
                }
              }
            } else {
              aLastEvals[3] = 0;
              DBG_(VVerb, ( << "do nothing with replication level L2[" << anIndex << "]" ));
            }

            DBG_(Crit, ( << "Outcome for L2[" << anIndex << "]:"
                         << " " << aLastEvals[0]
                         << " " << aLastEvals[1]
                         << " " << aLastEvals[2]
                         << " " << aLastEvals[3]
                         << " aDeltaIncrease=" << aDeltaIncrease
                         << "(" << theBenefitOfIncreasingReplication[anIndex] << "-" << theCostOfIncreasingReplication[anIndex] << ")"
                         << " aDeltaDecrease=" << aDeltaDecrease
                         << "(" << theBenefitOfDecreasingReplication[anIndex] << "-" << theCostOfDecreasingReplication[anIndex] << ")"
                         << ", new lvl = " << theASR_ReplicationLevel[anIndex]
                       ));

            if (1
                && aLastEvals[0] == aLastEvals[1]
                && aLastEvals[1] == aLastEvals[2]
                && aLastEvals[2] == aLastEvals[3]
               ) {
              theASR_ReplicationLevel[anIndex] += aLastEvals[3];
              if (aLastEvals[3] < 0) {
                DBG_(Crit, ( << "DECREASING replication for L2[" << anIndex << "] to " << theASR_ReplicationLevel[anIndex] ));
                theVTB[anIndex].clear(); // must clear VTB when reducing replication level
              }
              if (aLastEvals[3] > 0) {
                DBG_(Crit, ( << "INCREASING replication for L2[" << anIndex << "] to " << theASR_ReplicationLevel[anIndex] ));
                theCurrentReplicationOnly[anIndex].clear(); // must clear "current replication" bits when increasing replication level
              }
              aLastEvals[3] = 0; // effectively reset history to make sure 4 new change decisions are needed for next level change
              if (theASR_ReplicationLevel[anIndex] == 7) theASR_ReplicationLevel[anIndex] = 6; // picking up random garbage from theNLHB and theSoonToBeEvicted, ASR paper does not say to clear these structures
              if (theASR_ReplicationLevel[anIndex] == 0) theASR_ReplicationLevel[anIndex] = 1; // should never happen... right?
            }
            // reset evaluation counters
            theBenefitOfIncreasingReplication[anIndex] = 0;
            theCostOfIncreasingReplication[anIndex] = 0;
            theBenefitOfDecreasingReplication[anIndex] = 0;
            theCostOfDecreasingReplication[anIndex] = 0;
            theASR_LocalReplicationCount[anIndex] = 0;
            theASR_NLHBAllocationCount[anIndex] = 0;
          }
          theASR_NLHBInsert = false;
        }

        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
          break;

        case MemoryMessage::EvictCleanNonAllocating:
          DBG_Assert( false, ( << "theType here is before routing_req, so it couldn't have been made non-allocating" ));
          break;

        default:
          DBG_Assert( (false), ( << "Message type: " << theType ) ); //Unhandled message type
      }
    }
    DBG_Assert(!theASR_NLHBInsert, ( << "Make sure the only path with theASR_NLHBInsert is covered"));

    if (thePeerL1 != -1) {
      updateReplyHopsStats(thePeerL1, anIndex);
    } else {
      // if this is a RNUCA design, it is always true
      // if this is a shared design, it is always true
      // if this is a private L2 design, then it denotes an L2 hit (no directory accessed)
      if (theDir == -1) {
        DBG_Assert(theProbedL2 >= 0 && theProbedL2 < static_cast<int>(theNumL2Slices));
        updateReplyHopsStats(theProbedL2, anIndex);
      }
      // for private caches, this denotes an on-chip hit that doesn't involve remote L1s (ProbedL2 -> dir -> SnoopedL2)
      else if (theSnoopedL2 != -1) {
        updateReplyHopsStats(theSnoopedL2, anIndex);
      }
    }
    updateReqRoundTripHopsStats(coreToL2Hops + dirHops + snoopedL2Hops + peerL1Hops + replyHops);
    /* CMU-ONLY-BLOCK-END */

    theProbedL2 = -1;
    theSnoopedL2 = -1;
    theDir = -1;
    thePeerL1 = -1;

    theOffChipRequest = kNoOffChipReq;
    theRemoteL2Hit = false; /* CMU-ONLY */

    /* CMU-ONLY-BLOCK-BEGIN */
    //restore the snapshot of the system for reclassifications
    if (theReclassifyInProgress) {
      coreToL2Hops = tmp_coreToL2Hops;
      dirHops = tmp_dirHops;
      snoopedL2Hops = tmp_snoopedL2Hops;
      peerL1Hops = tmp_peerL1Hops;
      replyHops = tmp_replyHops;
      theProbedL2 = tmp_theProbedL2;
      theSnoopedL2 = tmp_theSnoopedL2;
      theDir = tmp_theDir;
      thePeerL1 = tmp_thePeerL1;
      theType = tmp_theType;
      theDstream = tmp_theDstream;
      theType = tmp_theType;
    }
    /* CMU-ONLY-BLOCK-END */
  }

  ////////////////////// from ICache
  bool available(  interface::RequestInI const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::RequestInI const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port RequestInI[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " page: " << std::hex << pageAddress(aMessage.address()) << std::dec ));
    aMessage.dstream() = false;
    processRequestInMessage(anIndex, aMessage);
  }

  ////////////////////// from DCache
  bool available(  interface::RequestInD const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::RequestInD const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port RequestInD[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " page: " << std::hex << pageAddress(aMessage.address()) << std::dec ));
    aMessage.dstream() = true;
    processRequestInMessage(anIndex, aMessage);
  }

  ////////////////////// from L2
  bool available(  interface::RequestFromL2 const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::RequestFromL2 const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port RequestFromL2[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " page: " << std::hex << pageAddress(aMessage.address()) << std::dec ));

    theL2Dstream = aMessage.dstream();
    theL2Type = aMessage.type();

    // for RNUCA or shared designs /* CMU-ONLY */
    if (
      thePlacement == kRNUCACache || /* CMU-ONLY */
      thePlacement == kSharedCache
    ) {
      /* CMU-ONLY-BLOCK-BEGIN */
      // Page reclassification causes cache block evicts.
      if (theReclassifyInProgress) {
        DBG_Assert(   theL2Type == MemoryMessage::EvictClean
                      || theL2Type == MemoryMessage::EvictWritable
                      || theL2Type == MemoryMessage::EvictDirty
                  );
      }
      updateOffChipStats(anIndex, aMessage);
      /* CMU-ONLY-BLOCK-END */

      FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
    }

    // for private and ASR design /* CMU-ONLY */
    else {

      /* CMU-ONLY-BLOCK-BEGIN */
      // hack to generate EvictL2CleanNonAllocating messages
      if (cfg.CleanSingletonRingWritebacks && theL2Type == MemoryMessage::EvictClean) {
        // get the chip-wide entry
        tChipDir::iterator iterChipDir = theChipDir.find(blockAddress(aMessage.address()));
        DBG_Assert (iterChipDir != theChipDir.end()) ;
        tChipDirEntry & theChipDirEntry = iterChipDir->second;
        tChipDirEntry tmpChipDirEntry = theChipDirEntry;
        rmSharer(tmpChipDirEntry, anIndex);
        if (noSharersLeft( tmpChipDirEntry )) {
          aMessage.type() = MemoryMessage::EvictL2CleanNonAllocating;
          theL2Type = aMessage.type();
        }
      }
      /* CMU-ONLY-BLOCK-END */

      processRequestFromL2_Private( anIndex, aMessage );
    }
  }

  ////////////////////// snoop port I
  void processSnoopToL1(   index_t         anIndex
                           , MemoryMessage & aMessage
                           , const bool      isDstream
                       ) {
    const tCoreID aCoreID = getCoreIDForSnoop(anIndex);

    // There is a pending L2 request,
    // and it is not a page reclassification, /* CMU-ONLY */
    // so this must be a PeerL1 request
    if (
      theProbedL2 != -1
      && !theReclassifyInProgress /* CMU-ONLY */
    ) {
      thePeerL1 = aCoreID;
      if (
        thePlacement == kRNUCACache || /* CMU-ONLY */
        thePlacement == kSharedCache
      ) {
        updatePeerL1HopsStats(theProbedL2, thePeerL1); /* CMU-ONLY */
        DBG_( Verb, Addr(aMessage.address()) ( << " PeerL1 for 0x" << std::hex << aMessage.address() << std::dec
                                               << " L2[" << theProbedL2 << "]"
                                               << " -> L1[" << thePeerL1 << "]" ));
      } else {
        if (theSnoopedL2 != -1) {
          updatePeerL1HopsStats(theSnoopedL2, thePeerL1); /* CMU-ONLY */
          DBG_( Verb, Addr(aMessage.address()) ( << " PeerL1 for 0x" << std::hex << aMessage.address() << std::dec
                                                 << " L2[" << theSnoopedL2 << "]"
                                                 << " -> L1[" << thePeerL1 << "]" ));
        } else {
          updatePeerL1HopsStats(theProbedL2, thePeerL1); /* CMU-ONLY */
          DBG_( Verb, Addr(aMessage.address()) ( << " PeerL1 for 0x" << std::hex << aMessage.address() << std::dec
                                                 << " L2[" << theProbedL2 << "]"
                                                 << " -> L1[" << thePeerL1 << "]" ));
        }
      }
    }
    /* CMU-ONLY-BLOCK-BEGIN */
    else if (theReclassifyInProgress) {
      DBG_Assert(theProbedL2 != -1);
      theReclassifyHops += theHops[ theProbedL2 ][ aCoreID ];
      DBG_Assert(theHops[ theProbedL2 ][ aCoreID ] <= 8);
      DBG_( Verb, Addr(aMessage.address()) ( << " Reclassify for 0x" << std::hex << aMessage.address() << std::dec
                                             << " L2[" << theProbedL2 << "]"
                                             << " -> L1[" << aCoreID << "]" ));
    }
    /* CMU-ONLY-BLOCK-END */
    DBG_Assert(aCoreID < theNumCores );
    if (isDstream) {
      FLEXUS_CHANNEL_ARRAY( SnoopOutD, aCoreID ) << aMessage;
    } else {
      FLEXUS_CHANNEL_ARRAY( SnoopOutI, aCoreID ) << aMessage;
    }
  }

  bool available(  interface::SnoopInI const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::SnoopInI const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port SnoopInI[" << anIndex << "]"
                                            << " coreID: " << getCoreIDForSnoop(anIndex)
                                            << " Request: " << aMessage
                                            << " page: 0x" << std::hex << pageAddress(aMessage.address()) << std::dec ));
    processSnoopToL1( anIndex, aMessage, false );
  }

  ////////////////////// snoop port D
  bool available(  interface::SnoopInD const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::SnoopInD const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port SnoopInD[" << anIndex << "]"
                                            << " coreID: " << getCoreIDForSnoop(anIndex)
                                            << " Request: " << aMessage
                                            << " page: 0x" << std::hex << pageAddress(aMessage.address()) << std::dec ));
    processSnoopToL1( anIndex, aMessage, true );
  }

  ////////////////////// snoop from bus
  bool available(  interface::BusSnoopIn const &
                   , index_t anIndex
                ) {
    return true;
  }

  void push(  interface::BusSnoopIn const &
              , index_t         anIndex
              , MemoryMessage & aMessage
           ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port BusSnoopIn[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " page: " << std::hex << pageAddress(aMessage.address()) << std::dec
                                          ));

    // DMA operation:
    // 1. a device issues a DMA request
    // 2. the device driver traps into the OS which finds the page table entry and pins it
    // 3. the OS issues purge requests to the L2 slices with a (possible) copy of the data
    // 4. DMA in progress
    // 5. DMA finishes, traps into the OS again, and the page is unpinned.
    //
    // However, #3 can be done automatically if the IO is treated as another chip in the system and the coherence protocol takes over.
    // The current implementation follows that.
    pendingL2AccessClass = kDMA; /* CMU-ONLY */
    propagateSnoopUp( anIndex, aMessage );
  }

  ////////////////////// drives
  void drive( interface::UpdateStatsDrive const & ) {
    // theStats->update();
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
private:

  ////////////////////// Helper functions
  uint32_t log_base2( uint32_t num ) {
    uint32_t ii = 0;
    while (num > 1) {
      ii++;
      num >>= 1;
    }
    return ii;
  }

  inline tAddress pageAddress( const tAddress addr ) const {
    return (addr & thePageMask);
  }

  inline tAddress blockAddress( const tAddress addr ) const {
    return (addr & theCacheLineMask);
  }

  ////////////////////// indexing

  // translate a coreID to a tile ID
  tTileID getTileID( const tCoreID aCoreID ) {
    tTileID ret = (aCoreID / theNumCoresPerL2Slice);
    DBG_(VVerb, ( << "Core " << aCoreID << " is in tile " << ret ));
    return ret;
  }

  // get L2 port for a core request
  tL2SliceID getL2SliceRequestOutPort( const tL2SliceID anL2ID
                                       , const tCoreID aCoreID
                                     ) {
    tL2SliceID ret = anL2ID * theNumCores + aCoreID;
    DBG_(VVerb, ( << "From core " << aCoreID
                  << " to L2[" << anL2ID << "]"
                  << " port " << ret
                ));
    return ret;
  }

  // L2ID from L2 port
  tL2SliceID getL2IDFromSnoopInPort ( const tL2SliceID aPort ) {
    return (tL2SliceID) (aPort / theNumCores);
  }

  // index to the core dest for a snoop
  tCoreID getCoreIDForSnoop( const index_t anIndex ) {
    tCoreID ret = (anIndex % theNumCores);
    DBG_(VVerb, ( << "Snoop from L2[" << getL2IDFromSnoopInPort(anIndex) << "]"
                  << " port " << anIndex
                  << " to core port " << ret ));
    return ret;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// RNUCA with instruction storage replacement and private data migration

  ////////////////////// the page map
  struct PageHash {
    std::size_t operator() ( tAddress anAddress ) const {
      anAddress = (anAddress >> thePageSizeLog2);
      return anAddress;
    }
  };

  typedef struct {
    //classification
    bool isDataClassValid;
    bool isShared;
    tTileID privateTileID;   // owner of private data

    bool pageHasInstr; // not a real page map entry. Used only for stats and for I/D decoupling

    bool HasFetchRef; // for whitebox debugging

    uint64_t theCachedLineBitmask[2]; // not a real page map entry. Used only to optimize the simflex code

  } tPageMapEntry;

  typedef __gnu_cxx::hash_map <  const tAddress // block address
  , tPageMapEntry  // entry pointer
  , PageHash       // cache-line hashing
  > tPageMap;

  tPageMap thePageMap;

  // Remove the functions below from the tPageMapEntry class to reduce each entry's footprint and improve performance
  uint32_t getCachedLineBitmaskPos( const tAddress anAddr
                                    , const tPageMapEntry & thePageMapEntry
                                  ) const {
    /*
      std::cout  << std::hex
                 << "anAddr " << anAddr
                 << " aPageMask " << thePageMask
                 << " aCacheLineSizeLog2 " << theCacheLineSizeLog2
                 << " (anAddr & ~aPageMask) " << (anAddr & ~thePageMask)
                 << " CachedLineBitmaskPos " << ((anAddr & ~thePageMask) >> theCacheLineSizeLog2)
                 << std::dec << std::endl;
    */
    return ((anAddr & ~thePageMask) >> theCacheLineSizeLog2);
  }

  // get page type
  bool isDataClassValid( const tPageMapEntry & thePageMapEntry ) const {
    return thePageMapEntry.isDataClassValid;
  }
  bool isShared( const tPageMapEntry & thePageMapEntry ) const {
    return (isDataClassValid(thePageMapEntry) && thePageMapEntry.isShared);
  }
  bool isPrivate( const tTileID aTileID
                  , const tPageMapEntry & thePageMapEntry
                ) const {
    return (isDataClassValid(thePageMapEntry) && !thePageMapEntry.isShared && (thePageMapEntry.privateTileID == aTileID));
  }

  // classify page
  void classifyAsInvalidDataClass( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.isDataClassValid = false;
  }
  void classifyAsShared( const tTileID oldOwnerTileID
                         , tPageMapEntry & thePageMapEntry
                       ) {
    thePageMapEntry.isDataClassValid = true;
    thePageMapEntry.isShared = true;
    thePageMapEntry.privateTileID = oldOwnerTileID;
  }
  void classifyAsPrivate( const tTileID aTileID
                          , tPageMapEntry & thePageMapEntry
                        ) {
    thePageMapEntry.isDataClassValid = true;
    thePageMapEntry.isShared = false;
    thePageMapEntry.privateTileID = aTileID;
  }

  // get owner id
  tTileID getPrivateL2ID( const tPageMapEntry & thePageMapEntry ) const {
    return thePageMapEntry.privateTileID;
  }

  // helper funcs for stats and for I/D decoupling
  bool pageHasData( const tPageMapEntry & thePageMapEntry ) const {
    return isDataClassValid( thePageMapEntry );
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

  // helper funcs for faster code
  void clearPageMapEntry( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.isDataClassValid = false;
    thePageMapEntry.isShared = false;
    thePageMapEntry.privateTileID = TILE_ID_UNDEF;
    thePageMapEntry.pageHasInstr = false;
    clearCachedLineBitmask( thePageMapEntry );
  }

  void clearCachedLineBitmask( tPageMapEntry & thePageMapEntry ) {
    thePageMapEntry.theCachedLineBitmask[0] = 0ULL;
    thePageMapEntry.theCachedLineBitmask[1] = 0ULL;
  }

  void addCachedLineBitmask( const tAddress anAddr
                             , tPageMapEntry & thePageMapEntry
                           ) {
    uint32_t real_pos = getCachedLineBitmaskPos(anAddr, thePageMapEntry);
    uint32_t idx = real_pos >> theCachedLineBitmaskComponentSizeLog2;
    uint64_t pos = real_pos & theCachedLineBitmaskLowMask;

    thePageMapEntry.theCachedLineBitmask[idx] |= (1ULL << pos);
  }

  void rmCachedLineBitmask( const tAddress anAddr
                            , tPageMapEntry & thePageMapEntry
                          ) {
    uint32_t real_pos = getCachedLineBitmaskPos(anAddr, thePageMapEntry);
    uint32_t idx = real_pos >> theCachedLineBitmaskComponentSizeLog2;
    uint64_t pos = real_pos & theCachedLineBitmaskLowMask;

    thePageMapEntry.theCachedLineBitmask[idx] &= ~(1ULL << pos);
  }

  bool isLineCached( const tAddress anAddr
                     , const tPageMapEntry & thePageMapEntry
                   ) const {
    uint32_t real_pos = getCachedLineBitmaskPos(anAddr, thePageMapEntry);
    uint32_t idx = real_pos >> theCachedLineBitmaskComponentSizeLog2;
    uint64_t pos = real_pos & theCachedLineBitmaskLowMask;

    return ((thePageMapEntry.theCachedLineBitmask[idx] & (1ULL << pos)) != 0ULL);
  }

  // helper funcs for debugging
  void print ( const tPageMapEntry & thePageMapEntry ) const {
    std::cout << " isDataClassValid:  " << thePageMapEntry.isDataClassValid ;
    std::cout << " isShared:          " << thePageMapEntry.isShared;
    std::cout << " privateTileID:     " << thePageMapEntry.privateTileID ;
    std::cout << " pageHasInstr:      " << thePageMapEntry.pageHasInstr ;
    std::cout << " CachedLineBitmask: " << std::hex << thePageMapEntry.theCachedLineBitmask[1] << thePageMapEntry.theCachedLineBitmask[0] << std::dec << std::endl;
  }

  ////////////////////// indexing

  // get the L2 slice local to the interleaved core within the instruction storage cluster
  tL2SliceID getL2SliceIDInstr_RNUCA( const tAddress anAddr
                                      , const tCoreID  aCoreID
                                    ) {
    // interleaving function      IL: addr ---> dest_RID
    const uint32_t theAddrInterleaveBits = (anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfInstrCluster - 1);
    const uint32_t dest_RID = theAddrInterleaveBits;

    // relative-vector function   RV: <center_RID,dest_RID> ---> D
    const uint32_t center_RID = theCoreInstrInterleavingID [ aCoreID ];
    const int32_t D = (dest_RID + ~center_RID + 1) & (theSizeOfInstrCluster - 1);

    // global mapping function    GM: <center_RID,D> ---> dest_CID
    DBG_Assert(theNumCores == 16 || (theNumCores == 8 && theNumCoresPerTorusRow == 4)); // works only for 4x4 and 4x2 toruses
    tL2SliceID dest_CID;
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
                                << " port " << getL2SliceRequestOutPort(dest_CID, aCoreID)
                              ));
    return dest_CID;
  }

  // index to address-interleaved L2 slice
  tL2SliceID getL2SliceIDShared_RNUCA( const tAddress anAddr ) {
    tL2SliceID ret = (anAddr >> theCacheLineSizeLog2) % theNumL2Slices;
    DBG_(VVerb, Addr(anAddr) ( << "Shared " << std::hex << anAddr << std::dec
                               << " to L2[" << ret << "]"
                             ));
    return ret;
  }

  // index to the private L2 slice
  tL2SliceID getL2SliceIDPrivate_RNUCA( const tAddress anAddr
                                        , const tCoreID  aCoreID
                                      ) {
    // interleaving function      IL: addr ---> dest_RID
    const uint32_t theAddrInterleaveBits = (anAddr >> theCacheLineSizeLog2) & ((tAddress) theSizeOfPrivCluster - 1);
    const uint32_t dest_RID = theAddrInterleaveBits;

    // relative-vector function   RV: <center_RID,dest_RID> ---> D
    const uint32_t center_RID = theCorePrivInterleavingID [ aCoreID ];
    const int32_t D = (dest_RID + ~center_RID + 1) & (theSizeOfPrivCluster - 1);

    // global mapping function    GM: <center_RID,D> ---> dest_CID
    DBG_Assert(theNumCores == 16 || (theNumCores == 8 && theNumCoresPerTorusRow == 4)); // works only for 4x4 and 4x2 toruses
    tL2SliceID dest_CID;
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
                                << " port " << getL2SliceRequestOutPort(dest_CID, aCoreID)
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
        aPageMapEntry.HasFetchRef = true; // for WhiteBox debugging
        DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr first touch addr=0x" << std::hex << anAddr << std::dec ));
      } else {
        // transition 2
        classifyAsPrivate( getTileID(aCoreID), aPageMapEntry );
        classifyAsPageWithoutInstr(aPageMapEntry);
        aPageMapEntry.HasFetchRef = false; // for WhiteBox debugging
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
                      , MemoryMessage & aMessage
                    ) {
    const tAddress  anAddr          = aMessage.address();
    const bool      fromDCache      = aMessage.isDstream(); // differentiate icache from dcache
    tPageMapEntry & thePageMapEntry = iterPageMap->second;
    bool            compiler_happy  = false;

    // reclassify only on actual requests
    if (!theReclassifyInProgress) {

      // istream ref on page that until now had none
      // transition 6, 10 (first time only)
      if ( !fromDCache
           && !pageHasInstr( thePageMapEntry )
         ) {
        classifyAsPageWithInstr( thePageMapEntry );

        // stats
        if ( isShared(thePageMapEntry) ) {
          theReclassifiedPages_SharedDataWithInstr ++; // was shared data
          DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Shared upon Istream ref for addr=0x" << std::hex << anAddr << std::dec ));
        } else {
          theReclassifiedPages_PrivateDataWithInstr ++; // was private data
          DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Priv upon Istream ref for addr=0x" << std::hex << anAddr << std::dec
                                      << " owner=" << getPrivateL2ID(thePageMapEntry)
                                    ));
        }
      }

      // data ref in page that until now had none
      // transition 4
      else if ( fromDCache
                && !pageHasData( thePageMapEntry )
              ) {
        classifyAsPrivate( getTileID(aCoreID), thePageMapEntry );
        DBG_( VVerb, Addr(anAddr) ( << "Classify as Instr+Priv upon data ref for addr=0x" << std::hex << anAddr << std::dec
                                    << " owner=" << getPrivateL2ID(thePageMapEntry)
                                  ));

        // stats
        theReclassifiedPages_PrivateDataWithInstr ++; // was private data
      }

      // reclassify priv -> shared
      // transition 8
      else if ( fromDCache
                && !isShared(thePageMapEntry)
                && !isPrivate( getTileID(aCoreID), thePageMapEntry )
              ) {
        return false;
      }
    }

    return true;

    compiler_happy = anAddr == 0;
  }

  tL2SliceID routeCoreReq_RNUCA( const index_t   anIndex
                                 , MemoryMessage & aMessage
                               ) {
    const tCoreID  theCoreID  = anIndex;
    const tAddress anAddr     = aMessage.address();
    const tAddress aPageAddr  = pageAddress(anAddr);
    const bool     fromDCache = aMessage.isDstream(); // differentiate icache from dcache

    const MemoryMessage::MemoryMessageType anL2Type = aMessage.type();

    // get the entry
    tPageMap::iterator iterPageMap = findOrInsertPage( anAddr
                                     , theCoreID
                                     , fromDCache
                                                     );
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    // I/D decoupling
    if (!cfg.DecoupleInstrDataSpaces) {
      // if it is a fetch req on a DataLine
      if (   (    (    theType == MemoryMessage::FetchReq
                       && pageHasData(thePageMapEntry)
                  )
                  // or it is a data req on an InstrLine
                  || (    pageHasInstr(thePageMapEntry)
                          && theType != MemoryMessage::EvictClean
                          && theType != MemoryMessage::EvictWritable
                          && theType != MemoryMessage::EvictDirty
                     )
             )
             // and the data are shared and the data and Istream lines are the same
             && (    (    isShared(thePageMapEntry)
                          && (getL2SliceIDInstr_RNUCA(anAddr, theCoreID) == getL2SliceIDShared_RNUCA(anAddr)))
                     // or the data are private and the data and Istream lines are the same
                     || (getL2SliceIDInstr_RNUCA(anAddr, theCoreID) == getPrivateL2ID(thePageMapEntry))
                )
         ) {
        theReclassifyInProgress = true;

        if (    theType == MemoryMessage::FetchReq
                && pageHasData(thePageMapEntry)
           ) {
          pendingL2AccessClass = kInstruction;
          theSnoopMessage.type() = MemoryMessage::PurgeDReq;
          if ( isShared(thePageMapEntry) ) {
            theSnoopMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsShared;
            theProbedL2 = getL2SliceIDShared_RNUCA(anAddr);
          } else {
            theSnoopMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
            theProbedL2 = getPrivateL2ID(thePageMapEntry);
          }
        } else {
          theSnoopMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsInstr;
          theProbedL2 = getL2SliceIDInstr_RNUCA(anAddr, theCoreID);
          theSnoopMessage.type() = MemoryMessage::PurgeIReq;
          if ( isShared(thePageMapEntry) ) {
            pendingL2AccessClass = kSharedData;
          } else {
            pendingL2AccessClass = kPrivateData;
          }
        }

        DBG_(Verb, Addr(anAddr) ( << "Decouple I/D for 0xp:" << std::hex << anAddr << std::dec
                                  << " Istream L2[" << getL2SliceIDInstr_RNUCA(anAddr, theCoreID) << "]"
                                  << " priv L2[" << getPrivateL2ID(thePageMapEntry) << "]"
                                  << " shared L2[" << getL2SliceIDShared_RNUCA(anAddr) << "]"
                                  << " purge L2[" << theProbedL2 << "]"
                                  << " extraOffsetBits:" << theSnoopMessage.idxExtraOffsetBits()
                                  << " message: " << theSnoopMessage
                                ));

        // Shoot down the line
        theSnoopMessage.address() = PhysicalMemoryAddress(anAddr);

        FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theProbedL2 ) << theSnoopMessage;
        if (theSnoopMessage.type() == MemoryMessage::PurgeAck) {
          if (pendingL2AccessClass == kInstruction) {
            theDecoupledLines_Data ++;
          } else {
            theDecoupledLines_Instr ++;
          }
        }

        theReclassifyInProgress = false;
      }
    }

    // WhiteBox debugging
    if (theType == MemoryMessage::FetchReq) {
      thePageMapEntry.HasFetchRef = true;
    } else if (    thePageMapEntry.HasFetchRef
                   && theType != MemoryMessage::EvictClean
                   && theType != MemoryMessage::EvictWritable
                   && theType != MemoryMessage::EvictDirty
              ) {
      thePageMapEntry.HasFetchRef = false;
      if (cfg.WhiteBoxDebug) {
        nWhiteBox::CPUState state;
        nWhiteBox::WhiteBox::getWhiteBox()->getState(theCoreID, state);
        DBG_(VVerb, ( << " Data ref" << (isShared(thePageMapEntry) ? " shared" : " private") << " after fetch " << state ));
      }
    }

    // Classify the page
    if (!pageClassification( theCoreID, iterPageMap, aMessage )) {

      // The only reclassification is Priv -> Shared.
      // Account for all evicts as private data evicts.
      theReclassifyInProgress = true;
      theReclassifyHops = 0;

      pendingL2AccessClass = kPrivateData;

      DBG_(Verb, Addr(anAddr) ( << "Reclassify page 0xp:" << std::hex << aPageAddr << std::dec
                                << " priv L2[" << getPrivateL2ID(thePageMapEntry) << "]"
                                << " -> shared"
                                << " with CachedLineBitmask:" << std::hex
                                << thePageMapEntry.theCachedLineBitmask[1]
                                << thePageMapEntry.theCachedLineBitmask[0]
                                << std::dec
                              ));
      // print(thePageMapEntry);

      // Shoot down the page
      // Only the private data will be shot down. Instr (if they exist) will be left mostly untouched.
      uint32_t numLinesFound = 0;
      for (tAddress iterAddr = aPageAddr; iterAddr < aPageAddr + thePageSize; iterAddr += theCacheLineSize) {

        if (isLineCached( iterAddr, thePageMapEntry )) {
          theSnoopMessage.address() = PhysicalMemoryAddress(iterAddr);
          theSnoopMessage.type() = MemoryMessage::PurgeDReq;
          int32_t thePrivClusterCenterL2 = getPrivateL2ID(thePageMapEntry);
          theProbedL2 = getL2SliceIDPrivate_RNUCA(iterAddr, thePrivClusterCenterL2);

          DBG_Assert( theProbedL2 < static_cast<int>(theNumL2Slices) );
          theSnoopMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theProbedL2 ) << theSnoopMessage;

          if (theSnoopMessage.type() == MemoryMessage::PurgeAck) {
            numLinesFound ++;
            theReclassifyHops += theHops[ theCoreID ][ theProbedL2 ];
            DBG_Assert(theHops[ theCoreID ][ theProbedL2 ] <= 8, ( << "theHops[" << theCoreID << "][" << theProbedL2 << "] == " << theHops[ theCoreID ][ theProbedL2 ] ));
          }
        }
      }
      classifyAsShared(theProbedL2, thePageMapEntry);
      DBG_( VVerb, Addr(anAddr) ( << "Classify as Shared addr=0x" << std::hex << anAddr << std::dec
                                  << " owner=" << getPrivateL2ID(thePageMapEntry)
                                ));
      clearCachedLineBitmask(thePageMapEntry) ;

      // stats
      theReclassifiedPages_PrivateToShared ++;
      theReclassifiedLines_PrivateToShared << std::make_pair(numLinesFound, 1);

      theReclassifyPrivateToSharedHops << std::make_pair(theReclassifyHops * 2, 1);
      theReclassifyInProgress = false;
      theReclassifyHops = 0;
    }

    // count fetches on data pages, and data refs on instr pages
    if ( pageHasData(thePageMapEntry)
         && pageHasInstr(thePageMapEntry)
         && anL2Type != MemoryMessage::EvictClean
         && anL2Type != MemoryMessage::EvictWritable
         && anL2Type != MemoryMessage::EvictDirty
       ) {
      if ( fromDCache ) {
        theDataRefsInInstrDataPage ++;
      }

      else {
        theInstrRefsInInstrDataPage ++;
      }
    }

    // indexing and placement
    if ( !fromDCache ) {
      aMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsInstr;
      theProbedL2 = getL2SliceIDInstr_RNUCA(anAddr, theCoreID);
      pendingL2AccessClass = kInstruction;
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Instr" ));
    } else if ( isShared(thePageMapEntry) ) {
      aMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsShared;
      theProbedL2 = getL2SliceIDShared_RNUCA(anAddr);
      pendingL2AccessClass = kSharedData;
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Shared" ));
    } else {
      aMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
      theProbedL2 = getL2SliceIDPrivate_RNUCA(anAddr, theCoreID);
      pendingL2AccessClass = kPrivateData;
      DBG_( VVerb, Addr(anAddr) ( << " Addr: 0x" << std::hex << anAddr << std::dec
                                  << " Class = Private" ));
    }

    // stats
    updateCoreToL2HopsStats_RNUCA(anIndex, theProbedL2);

    // update cached lines bitmask
    addCachedLineBitmask( anAddr, thePageMapEntry );

    // print(thePageMapEntry);

    return getL2SliceRequestOutPort(theProbedL2, theCoreID);
  }

  // send snoop from Bus up in the cache hierarchy
  void propagateSnoopUp_RNUCA( index_t anIndex
                               , MemoryMessage & aMessage
                             ) {
    const tAddress anAddr    = aMessage.address();
    const tAddress aPageAddr = pageAddress(anAddr);

    // get the entry
    tPageMap::iterator iterPageMap = thePageMap.find(aPageAddr);
    DBG_Assert (iterPageMap != thePageMap.end());
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    DBG_( Verb, Addr(anAddr) ( << " Propagate snoop up for " << std::hex << anAddr << std::dec ));
    // print(thePageMapEntry);

    // snoop instructions
    if (aMessage.type() != MemoryMessage::Downgrade) {
      // Istream lines are never written.
      // Thus, send the snoop to the Istream nodes only when the snoop is one of:
      //   MemoryMessage::ReturnReq
      //   MemoryMessage::Invalidate
      //   MemoryMessage::PurgeReq
      theSnoopMessage = aMessage;
      uint32_t theAddrInterleavingBits = (anAddr >> theCacheLineSizeLog2) & 2;
      for (uint32_t i = 0; i < theNumCores; i++) {
        if (theCoreInstrInterleavingID [i] == theAddrInterleavingBits) {
          MemoryMessage tmpMessage(aMessage);
          DBG_Assert( getTileID(i) < theNumL2Slices );
          tmpMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsInstr;
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, getTileID(i) ) << tmpMessage;

          // if a reply is received, data have been found
          if (tmpMessage.type() != aMessage.type()) {
            theSnoopMessage = tmpMessage;
          }
        }
      }
    }

    // snoop data
    MemoryMessage tmpMessage(aMessage);
    if ( isShared(thePageMapEntry) ) {
      tL2SliceID theL2SlicePort = ((anAddr >> theCacheLineSizeLog2) % theNumL2Slices);
      DBG_Assert( theL2SlicePort < theNumL2Slices );
      tmpMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsShared;
      FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theL2SlicePort ) << tmpMessage;
    } else if ( isDataClassValid(thePageMapEntry) ) {
      int32_t thePrivClusterCenterL2 = getPrivateL2ID(thePageMapEntry);
      tL2SliceID theL2SlicePort = getL2SliceIDPrivate_RNUCA(anAddr, thePrivClusterCenterL2);
      DBG_Assert( theL2SlicePort < theNumL2Slices );
      tmpMessage.idxExtraOffsetBits() = theIdxExtraOffsetBitsPrivate;
      FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theL2SlicePort ) << tmpMessage;
    }

    // if a reply is received, data have been found
    if (tmpMessage.type() != aMessage.type()) {
      theSnoopMessage = tmpMessage;
    }

    // Send the correct reply.
    // If data are found during the instr snooping, the reply is a plain xxxAck.
    // if data are found during the data snooping, the reply is either a plain xxxAck, or an xxxUpdAck.
    // Thus, the type of data snoop reply takes precedence over the type of the instr snoop reply.
    aMessage = theSnoopMessage;
  }

  ////////////////////// stats

  // hop counts
  void updateCoreToL2HopsStats_RNUCA( const tCoreID aCoreID
                                      , const tL2SliceID anL2ID
                                    ) {
    switch (pendingL2AccessClass) {
      case kInstruction: {
        switch (theType) {
          case MemoryMessage::FetchReq:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_I_Fetch << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType << " Fetch: " << MemoryMessage::FetchReq) ); //Unhandled message type
        }
        break;
      }
      case kPrivateData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_P_Read << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_P_Write << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_P_Upg << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_P_Atomic << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType << " Fetch: " << MemoryMessage::FetchReq) ); //Unhandled message type
        }
        break;
      }
      case kSharedData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_S_Read << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_S_Write << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_S_Upg << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
            theCoreToL2Hops_S_Atomic << std::make_pair(coreToL2Hops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kDMA:
        break;
      default:
        DBG_Assert( false );
    }
  }

  void updatePeerL1HopsStats_RNUCA( const tL2SliceID anL2ID
                                    , const tCoreID aCoreID
                                  ) {
    switch (pendingL2AccessClass) {
      case kInstruction: {
        switch (theType) {
          case MemoryMessage::FetchReq:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_I_Fetch << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType << " Fetch: " << MemoryMessage::FetchReq) ); //Unhandled message type
        }
        break;
      }
      case kPrivateData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_P_Read << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_P_Write << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_P_Upg << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_P_Atomic << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kSharedData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_S_Read << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_S_Write << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_S_Upg << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            peerL1Hops = theHops[ anL2ID ][ aCoreID ];
            thePeerL1Hops_S_Atomic << std::make_pair(peerL1Hops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kDMA:
        break;
      default:
        DBG_Assert( false );
    }
  }

  void updateReplyHopsStats_RNUCA( const tL2SliceID anL2ID         // or a PeerL1. NOTE: make all these TileID types.
                                   , const tCoreID aCoreID
                                 ) {
    switch (pendingL2AccessClass) {
      case kInstruction: {
        switch (theType) {
          case MemoryMessage::FetchReq:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_I_Fetch << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType << " Fetch: " << MemoryMessage::FetchReq) ); //Unhandled message type
        }
        break;
      }
      case kPrivateData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_P_Read << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_P_Write << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_P_Upg << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_P_Atomic << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kSharedData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_S_Read << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_S_Write << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_S_Upg << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            replyHops = theHops[ anL2ID ][ aCoreID ];
            theReplyHops_S_Atomic << std::make_pair(replyHops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kDMA:
        break;
      default:
        DBG_Assert( false );
    }
  }

  void updateReqRoundTripHopsStats_RNUCA( const int32_t reqRoundTripHops ) {
    switch (pendingL2AccessClass) {
      case kInstruction: {
        switch (theType) {
          case MemoryMessage::FetchReq:
            theReqRoundTripHops_I_Fetch << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType << " Fetch: " << MemoryMessage::FetchReq) ); //Unhandled message type
        }
        break;
      }
      case kPrivateData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            theReqRoundTripHops_P_Read << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            theReqRoundTripHops_P_Write << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            theReqRoundTripHops_P_Upg << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            theReqRoundTripHops_P_Atomic << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kSharedData: {
        switch (theType) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            theReqRoundTripHops_S_Read << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            theReqRoundTripHops_S_Write << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            theReqRoundTripHops_S_Upg << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            theReqRoundTripHops_S_Atomic << std::make_pair(reqRoundTripHops, 1);
            break;

          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
            break;

          default:
            DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
        }
        break;
      }
      case kDMA:
        break;
      default:
        DBG_Assert( false );
    }
  }

  // off-chip misses
  void updateOffChipStats_RNUCA( index_t         anIndex
                                 , MemoryMessage & aMessage
                               ) {
    // evictions are categorized based on the access type that displaced them, rather than the category of the evited block itself
    switch (pendingL2AccessClass) {
      case kInstruction: {
        switch (aMessage.type()) {
          case MemoryMessage::FetchReq:
            theOffChip_I_Fetch ++;
            break;

          case MemoryMessage::EvictClean:
          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
            theOffChip_I_Evict ++;
            break;

          default:
            DBG_Assert( (false), ( << "Reclassify: " << (theReclassifyInProgress ? "yes" : "no") << " Message: " << aMessage ) ); //Unhandled message type
        }
        break;
      }
      case kPrivateData: {
        switch (aMessage.type()) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            theOffChip_P_Read ++;
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            theOffChip_P_Write ++;
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            theOffChip_P_Upg ++;
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            theOffChip_P_Atomic ++;
            break;

          case MemoryMessage::EvictClean:
            theOffChip_P_EvictClean ++;
            break;

          case MemoryMessage::EvictDirty:
            theOffChip_P_EvictDirty ++;
            break;

          case MemoryMessage::EvictWritable:
            theOffChip_P_EvictWritable ++;
            break;

          default:
            DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
        }
        break;
      }
      case kSharedData: {
        switch (aMessage.type()) {
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
            theOffChip_S_Read ++;
            break;

          case MemoryMessage::StoreReq:
          case MemoryMessage::StorePrefetchReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
            theOffChip_S_Write ++;
            break;

          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
            theOffChip_S_Upg ++;
            break;

          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            theOffChip_S_Atomic ++;
            break;

          case MemoryMessage::EvictClean:
            theOffChip_S_EvictClean ++;
            break;

          case MemoryMessage::EvictDirty:
            theOffChip_S_EvictDirty ++;
            break;

          case MemoryMessage::EvictWritable:
            theOffChip_S_EvictWritable ++;
            break;

          default:
            DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
        }
        break;
      }
      default:
        DBG_Assert( false );
    }

    // clear up cached lines bitmask if it is an eviction
    switch (aMessage.type()) {
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable: {
        const tAddress anAddr    = aMessage.address();
        const tAddress aPageAddr = pageAddress(anAddr);

        // get the entry
        tPageMap::iterator iterPageMap = thePageMap.find(aPageAddr);
        DBG_Assert (iterPageMap != thePageMap.end(), ( << " pageAddr: " << std::hex << aPageAddr << std::dec ) );
        tPageMapEntry & thePageMapEntry = iterPageMap->second;

        if (   (isShared(thePageMapEntry) && anIndex == getL2SliceIDShared_RNUCA(anAddr))  // shared data line evicted by the data home
               || (isPrivate(anIndex, thePageMapEntry))                                      // private data line evicted by the private owner
           ) {
          rmCachedLineBitmask( anAddr, thePageMapEntry );
          DBG_( VVerb, Addr(anAddr) ( << " Addr: " << std::hex << anAddr
                                      << " new CachedLineBitmask: " << thePageMapEntry.theCachedLineBitmask[1] << thePageMapEntry.theCachedLineBitmask[0] << std::dec
                                    ));
        }
        break;
      }

      default:
        break;
    }

  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// private L2
  ////////////////////// or ASR              /* CMU-ONLY */

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////// stats
  void
  pageBkdStats( const uint32_t aCoreID
                , MemoryMessage & aMessage
              ) {
    const tAddress anAddr     = aMessage.address();
    const bool     fromDCache = aMessage.isDstream(); // differentiate icache from dcache

    const MemoryMessage::MemoryMessageType anL2Type = aMessage.type();

    // get the entry
    // if no entry is found, a new one is created and classified initially based on the request
    tPageMap::iterator iterPageMap = findOrInsertPage(anAddr, aCoreID, fromDCache);
    tPageMapEntry & thePageMapEntry = iterPageMap->second;

    // Classify the page
    if (!pageClassification( aCoreID, iterPageMap, aMessage )) {
      // must reclassify page priv->shared
      classifyAsShared(getPrivateL2ID(thePageMapEntry), thePageMapEntry);
      DBG_( VVerb, Addr(anAddr) ( << "Classify as Shared addr=0x" << std::hex << anAddr << std::dec
                                  << " owner=" << getPrivateL2ID(thePageMapEntry)
                                ));

      // stats
      theReclassifiedPages_PrivateToShared ++;
    }

    // count fetches on data pages, and data refs on instr pages
    if ( pageHasData(thePageMapEntry)
         && pageHasInstr(thePageMapEntry)
         && anL2Type != MemoryMessage::EvictClean
         && anL2Type != MemoryMessage::EvictWritable
         && anL2Type != MemoryMessage::EvictDirty
       ) {
      if ( fromDCache ) {
        theDataRefsInInstrDataPage ++;
      }

      else {
        theInstrRefsInInstrDataPage ++;
      }
    }
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////// chip-wide distributed directory
  typedef uint16_t tChipDirState;

  // The state bits are the same as the ones in the FSM for the basic MOSI states.
  // The size changes to only 2 bits so the final state fits in the cache block offset.
  // The additional states in the FSM are not final states, only transient ones, so they are not needed here.
  static const tChipDirState kDirStateMask = 0x03; // ---- --11
  // static const tChipDirState kInvalid =   0x00; // ---- --00
  // static const tChipDirState kShared =    0x01; // ---- --01
  // static const tChipDirState kOwned =     0x02; // ---- --10
  // static const tChipDirState kModified =  0x03; // ---- --11

  static const tChipDirState kHasFetchRef = 0x80;  //    Used for whitebox debugging

  typedef uint64_t tSharers;                       // type definition
  static const tSharers kOwnerMask = 0x000000000000007FULL;  //    Current owner: At most 64 nodes (64 L1i + 64 L1d)

  // the chip-wide directory structure
  struct CacheLineHash {
    std::size_t operator() ( tAddress anAddress ) const {
      anAddress = (anAddress >> theCacheLineSizeLog2);
      return anAddress;
    }
  };

  typedef struct tChipDirEntry {
    tChipDirState theState;
    tSharers      theSharers[2];
  } tChipDirEntry;

  uint32_t theSharersBitmaskComponentSizeLog2;
  uint32_t theSharersBitmaskComponents;
  tSharers theSharersBitmaskLowMask;

  tChipDirEntry anInvalChipDirEntry;

  typedef __gnu_cxx::hash_map <  const tAddress // block address
  , tChipDirEntry   // entry pointer
  , CacheLineHash   // cache-line hashing
  > tChipDir;

  tChipDir theChipDir;

  /* CMU-ONLY-BLOCK-BEGIN */
  //XJ:
  uint32_t theDirSetMask;     // Mask to get the set. (!!!aligned to the BLOCK address!!!)
  uint32_t theDirNumSets;     // total number of sets

  typedef struct tCMPBoundedDirEntry {
    tAddress theAddress;
    bool     isValid;

    tCMPBoundedDirEntry() {
      isValid = false;
    }

  } tCMPBoundedDirEntry;
  tCMPBoundedDirEntry * theDirEntrySets;
  /* CMU-ONLY-BLOCK-END */

  ////// directory state maintenance
  void clearDirEntry( tChipDirEntry & aChipDirEntry ) {
    aChipDirEntry.theState = 0;
    aChipDirEntry.theSharers[0] = 0ULL;
    aChipDirEntry.theSharers[1] = 0ULL;
  }

  void clearSharers( tChipDirEntry & aChipDirEntry ) {
    aChipDirEntry.theSharers[0] = 0ULL;
    aChipDirEntry.theSharers[1] = 0ULL;
  }

  void addSharer( tChipDirEntry & aChipDirEntry
                  , const tL2SliceID aNode
                ) {
    uint32_t idx = aNode >> theSharersBitmaskComponentSizeLog2;
    uint32_t pos = aNode & theSharersBitmaskLowMask;
    aChipDirEntry.theSharers[idx] |= (1ULL << pos);
  }

  void rmSharer( tChipDirEntry & aChipDirEntry
                 , const tL2SliceID aNode
               ) {
    uint32_t idx = aNode >> theSharersBitmaskComponentSizeLog2;
    uint32_t pos = aNode & theSharersBitmaskLowMask;
    aChipDirEntry.theSharers[idx] &= ~(1ULL << pos);
  }

  bool isSharer( const tChipDirEntry & aChipDirEntry
                 , const tL2SliceID aNode
               ) const {
    uint32_t idx = aNode >> theSharersBitmaskComponentSizeLog2;
    uint32_t pos = aNode & theSharersBitmaskLowMask;
    return ((aChipDirEntry.theSharers[idx] & (1ULL << pos)) != 0ULL);
  }

  bool someSharers( const tChipDirEntry & aChipDirEntry ) const {
    return (aChipDirEntry.theSharers[0] != 0ULL || aChipDirEntry.theSharers[1] != 0ULL);
  }

  bool noSharersLeft( const tChipDirEntry & aChipDirEntry ) const {
    return !someSharers(aChipDirEntry);
  }

  tL2SliceID getOwner( const tChipDirEntry & aChipDirEntry ) const {
    return (aChipDirEntry.theSharers[0]);
  }

  void setOwner( tChipDirEntry & aChipDirEntry
                 , const tL2SliceID aNode
               ) const {
    aChipDirEntry.theSharers[0] = aNode;
  }

  void clearSharersAndOwner( tChipDirEntry & aChipDirEntry ) {
    aChipDirEntry.theSharers[0] = aChipDirEntry.theSharers[1] = 0ULL;
  }

  tChipDirState getState( const tChipDirEntry aChipDirEntry ) const {
    return (aChipDirEntry.theState & kDirStateMask);
  }

  void setState( tChipDirEntry & aChipDirEntry
                 , const tChipDirState newState ) {
    aChipDirEntry.theState = ((aChipDirEntry.theState & ~kDirStateMask) | newState);
  }

  bool isShared( const tChipDirEntry & aChipDirEntry ) const {
    return( (aChipDirEntry.theState & kDirStateMask) == kShared );
  }

  bool isModified( const tChipDirEntry & aChipDirEntry ) const {
    return( (aChipDirEntry.theState & kDirStateMask) == kModified );
  }

  bool isOwned( const tChipDirEntry & aChipDirEntry ) const {
    return( (aChipDirEntry.theState & kDirStateMask) == kOwned );
  }

  bool isInvalid( const tChipDirEntry & aChipDirEntry ) const {
    return ( (aChipDirEntry.theState & kDirStateMask) == kInvalid );
  }

  int32_t getRandomSharer( const tChipDirEntry & aChipDirEntry
                           , const tL2SliceID aSliceID
                         ) {
    if (!someSharers(aChipDirEntry)) {
      return -1;
    }

    int32_t theSharersCount = 0;
    for (uint32_t i = 0; i < theNumL2Slices; i++) {
      // DBG_( VVerb, ( << "i= " << std::hex << i << " ShrList=" << aChipDirEntry.theSharers[1] << aChipDirEntry.theSharers[0] << " i-1=" << i-1 << " L2=" << aSliceID ));
      if (isSharer(aChipDirEntry, i) && i != aSliceID) {
        theSharersCount++;
      }
    }
    if (theSharersCount == 0) {
      return -1;
    }

    int32_t someSharer = (random() % theSharersCount) + 1;
    int32_t j = 0;
    for (uint32_t i = 0; i < theNumL2Slices; i++) {
      if (isSharer(aChipDirEntry, i) && i != aSliceID) {
        someSharer--;
        if (someSharer == 0) {
          return j;
        }
      }
      j++;
    }
    DBG_Assert(false, ( << std::hex << "SharersList=" << aChipDirEntry.theSharers[1] << aChipDirEntry.theSharers[1] << std::dec << " some=" << someSharer ));

    return -1;
  }

  // chip-wide directory FSM
  static const uint64_t kStateMask =                     0x0007ULL; // ----  ----  ----  -111
  static const uint64_t kInvalid =                       0x0000ULL; // ----  ----  ----  -000
  static const uint64_t kShared =                        0x0001ULL; // ----  ----  ----  -001
  static const uint64_t kOwned =                         0x0002ULL; // ----  ----  ----  -010
  static const uint64_t kModified =                      0x0003ULL; // ----  ----  ----  -011
  static const uint64_t kSharersDependantState =         0x0004ULL; // ----  ----  ----  -100
  static const uint64_t kSharersDirtyDependantState =    0x0005ULL; // ----  ----  ----  -101
  static const uint64_t kReplyDependantState =           0x0006ULL; // ----  ----  ----  -110

  static const uint64_t kAccessMask =                    0x03F8ULL; // ----  --11  1111  1---
  static const uint64_t kNoAccess =                      0x0000ULL; // ----  --00  0000  0---
  static const uint64_t kFetchAccess =                   0x0018ULL; // ----  --00  0001  1---
  static const uint64_t kReadAccess =                    0x0010ULL; // ----  --00  0001  0---
  static const uint64_t kWriteAccess =                   0x0030ULL; // ----  --00  0011  0---
  static const uint64_t kUpgradeAccess =                 0x0020ULL; // ----  --00  0010  0---
  static const uint64_t kCleanEvictAccess =              0x0040ULL; // ----  --00  0100  0---
  static const uint64_t kDirtyEvictAccess =              0x0060ULL; // ----  --00  0110  0---
  static const uint64_t kWritableEvictAccess =           0x0070ULL; // ----  --00  0111  0---
  static const uint64_t kCleanEvictNonAllocatingAccess = 0x0050ULL; // ----  --00  0101  0---
  static const uint64_t kDirtyEvictNonAllocatingAccess = 0x00F0ULL; // ----  --00  1111  0---
  static const uint64_t kL2CleanEvictNonAllocatingAccess = 0x00D0ULL; // ----  --00  1101  0---
  static const uint64_t kIsEvictMask =                   0x0040ULL; // ----  --00  0100  0---

  static const uint64_t kInvalidateAccess =              0x0300ULL; // ----  --11  0000  0---
  static const uint64_t kReturnReqAccess =               0x0200ULL; // ----  --10  0000  0---
  static const uint64_t kDowngradeAccess =               0x0280ULL; // ----  --10  1000  0---

  static const uint64_t kSnoopMask =                     0x1C00ULL; // ---1  11--  ----  ----
  static const uint64_t kNoSnoop =                       0x0000ULL; // ---0  00--  ----  ----
  static const uint64_t kReturnReq =                     0x0400ULL; // ---0  01--  ----  ----
  static const uint64_t kInvalidate =                    0x0800ULL; // ---0  10--  ----  ----
  static const uint64_t kDowngrade =                     0x0C00ULL; // ---0  11--  ----  ----
  static const uint64_t kProbe =                         0x1000ULL; // ---1  00--  ----  ----
  static const uint64_t kProbeDirty =                    0x1400ULL; // ---1  01--  ----  ----

  static const uint64_t kOffChipReqMask =                0x00F8ULL; // ----  ----  1111  1---
  static const uint64_t kNoOffChipReq =                  0x0000ULL; // ----  ----  0000  0---
  static const uint64_t kFetchOCR =                      0x0018ULL; // ----  ----  0001  1---
  static const uint64_t kReadOCR =                       0x0010ULL; // ----  ----  0001  0---
  static const uint64_t kWriteOCR =                      0x0030ULL; // ----  ----  0011  0---
  static const uint64_t kUpgradeOCR =                    0x0020ULL; // ----  ----  0010  0---
  static const uint64_t kSharersDependantOCR =           0x0040ULL; // ----  ----  0100  0---
  static const uint64_t kDirtyEvictOCR =                 0x0060ULL; // ----  ----  0110  0---
  static const uint64_t kWritableEvictOCR =              0x0070ULL; // ----  ----  0111  0---
  static const uint64_t kIsEvictOCRMask =                0x0040ULL; // ----  ----  0100  0---
  static const uint64_t kDownUpdateAckOCR =              0x0080ULL; // ----  ----  1000  0---
  static const uint64_t kInvUpdateAckOCR =               0x0088ULL; // ----  ----  1000  1---
  static const uint64_t kCleanEvictNonAllocatingOCR =    0x0050ULL; // ----  ----  0101  0---
  static const uint64_t kDirtyEvictNonAllocatingOCR =    0x00D0ULL; // ----  ----  1101  0---

  static const uint64_t kPoison =                        0x1FFFULL; // ---1  1111  1111  1111

  static const uint64_t kLargestIndex = kStateMask | kAccessMask;
  uint64_t theChipDirFSM[ kLargestIndex ];

  void fillChipDirFSM( void ) {
    // poison the FSM
    for (uint32_t i = 0; i < kLargestIndex; i++) {
      theChipDirFSM[i] = kPoison;
    }

    // valid state transitions (all others go to poison)
    //
    // kSharersDependantState:
    // no sharers left   -> kInvalid
    // some sharers left -> state remains unchanged (kShared or KOwned)
    //
    // kSharersDirtyDependantState:
    // no remote allocation -> kInvalid
    // remotely allocated   -> unchanged (kDirty)
    //
    // kSharersDependantOCR:
    // no sharers left   -> kCleanEvictOCR (if oldState==kShared) or kDirtyEvictOCR (if oldState==kOwned)
    // some sharers left -> no OCR
    //
    //             state        access                          ->  next state               snoop         off-chip request(OCR)
    theChipDirFSM[ kInvalid   | kFetchAccess                   ] =  kReplyDependantState                 | kFetchOCR            ;
    theChipDirFSM[ kInvalid   | kReadAccess                    ] =  kReplyDependantState                 | kReadOCR             ;
    theChipDirFSM[ kInvalid   | kWriteAccess                   ] =  kModified                            | kWriteOCR            ;
    // off-chip accesses
    theChipDirFSM[ kInvalid   | kInvalidateAccess              ] =  kInvalid                                                    ;
    theChipDirFSM[ kInvalid   | kReturnReqAccess               ] =  kInvalid                                                    ;

    theChipDirFSM[ kShared    | kFetchAccess                   ] =  kShared                | kReturnReq                         ;
    theChipDirFSM[ kShared    | kReadAccess                    ] =  kShared                | kReturnReq                         ;
    theChipDirFSM[ kShared    | kWriteAccess                   ] =  kModified              | kInvalidate | kUpgradeOCR          ;
    theChipDirFSM[ kShared    | kUpgradeAccess                 ] =  kModified              | kInvalidate | kUpgradeOCR          ;
    theChipDirFSM[ kShared    | kCleanEvictAccess              ] =  kSharersDependantState               | kSharersDependantOCR ;
    theChipDirFSM[ kShared    | kCleanEvictNonAllocatingAccess ] =  kSharersDependantState | kProbe      | kCleanEvictNonAllocatingOCR ;
    theChipDirFSM[ kShared    | kL2CleanEvictNonAllocatingAccess ] =  kSharersDependantState | kProbe      | kCleanEvictNonAllocatingOCR ;
    // off-chip accesses
    theChipDirFSM[ kShared    | kInvalidateAccess              ] =  kInvalid               | kInvalidate                        ;
    theChipDirFSM[ kShared    | kReturnReqAccess               ] =  kShared                | kReturnReq                         ;

    theChipDirFSM[ kModified  | kFetchAccess                   ] =  kOwned                 | kDowngrade                         ;
    theChipDirFSM[ kModified  | kReadAccess                    ] =  kOwned                 | kDowngrade                         ;
    theChipDirFSM[ kModified  | kWriteAccess                   ] =  kModified              | kInvalidate                        ;
    theChipDirFSM[ kModified  | kDirtyEvictAccess              ] =  kInvalid                             | kDirtyEvictOCR       ;
    theChipDirFSM[ kModified  | kWritableEvictAccess           ] =  kInvalid                             | kWritableEvictOCR    ;
    theChipDirFSM[ kModified  | kDirtyEvictNonAllocatingAccess ] =  kSharersDirtyDependantState | kProbeDirty | kDirtyEvictNonAllocatingOCR ;
    // off-chip accesses
    theChipDirFSM[ kModified  | kInvalidateAccess              ] =  kInvalid               | kInvalidate                        ;
    theChipDirFSM[ kModified  | kDowngradeAccess               ] =  kShared                | kDowngrade                         ;

    theChipDirFSM[ kOwned     | kFetchAccess                   ] =  kOwned                 | kReturnReq                         ;
    theChipDirFSM[ kOwned     | kReadAccess                    ] =  kOwned                 | kReturnReq                         ;
    theChipDirFSM[ kOwned     | kWriteAccess                   ] =  kModified              | kInvalidate                        ;
    theChipDirFSM[ kOwned     | kUpgradeAccess                 ] =  kModified              | kInvalidate                        ;
    theChipDirFSM[ kOwned     | kCleanEvictAccess              ] =  kSharersDependantState               | kSharersDependantOCR ;
    theChipDirFSM[ kOwned     | kDirtyEvictAccess              ] =  kInvalid                             | kDirtyEvictOCR       ;
    theChipDirFSM[ kOwned     | kWritableEvictAccess           ] =  kInvalid                             | kWritableEvictOCR    ;
    theChipDirFSM[ kOwned     | kCleanEvictNonAllocatingAccess ] =  kSharersDependantState | kProbe      | kCleanEvictNonAllocatingOCR ;
    theChipDirFSM[ kOwned     | kL2CleanEvictNonAllocatingAccess ] =  kSharersDependantState | kProbe      | kCleanEvictNonAllocatingOCR ;
    theChipDirFSM[ kOwned     | kDirtyEvictNonAllocatingAccess ] =  kSharersDirtyDependantState | kProbeDirty | kDirtyEvictNonAllocatingOCR ;
    // off-chip accesses
    theChipDirFSM[ kOwned     | kInvalidateAccess              ] =  kInvalid               | kInvalidate | kInvUpdateAckOCR     ;
    theChipDirFSM[ kOwned     | kDowngradeAccess               ] =  kShared                | kReturnReq  | kDownUpdateAckOCR    ;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  // XJ: evict from or insert in the directory
  void countBoundedDirEntries ( tAddress blockAddr
                                , bool   newEntry
                                , bool   eraseEntry
                              ) {
    // Idea:
    // - Use a smaller structure in parallel with the unbounded directory.
    // - Invalidate when necessary
    if (cfg.DirEntries > 0) {
      int32_t  theDirSet = ( blockAddr >> theCacheLineSizeLog2 ) & theDirSetMask;
      int32_t  offset    = theDirSet * cfg.DirWays;
      int32_t  foundAt   = cfg.DirWays + (newEntry ? 100 : 10);

      // If the entry has been removed from the directory, we should remove it from here as well
      if (eraseEntry) {
        DBG_( VVerb, ( << std::hex << "Erasing 0x" << blockAddr << " from the Directory... " ));

        // find entry to erase
        for (int32_t i = 0; i < cfg.DirWays; i++) {
          tCMPBoundedDirEntry tmp = theDirEntrySets[offset+i];
          if (tmp.isValid && tmp.theAddress == blockAddr) {
            foundAt = i;
            break;
          }
        }
        // move the entries below one step up
        for (int32_t k = foundAt + 1; k < cfg.DirWays; k++) {
          if (theDirEntrySets[offset+k].isValid) {
            theDirEntrySets[offset+k-1] = theDirEntrySets[offset+k];
          } else {
            // until we reach an invalid entry
            break;
          }
        }
        // make sure last entry is invalid
        theDirEntrySets[offset+cfg.DirWays-1].isValid = false;
        DBG_( VVerb, ( << std::hex << "Erasing 0x" << blockAddr << " from the Directory at pos=" << foundAt << " ... Done! "));
      }

      else {
        DBG_( VVerb, ( << std::hex << "Inserting 0x" << blockAddr << " in directory set 0x" << theDirSet ));

        // setup the directory entry
        tCMPBoundedDirEntry evicted;
        evicted.theAddress = blockAddr;
        evicted.isValid    = true;

        // Search for the entry, and reorder the LRU list along the way
        for (int32_t i = 0; i < cfg.DirWays; i++) {
          tCMPBoundedDirEntry tmp = theDirEntrySets[offset+i];
          theDirEntrySets[offset+i] = evicted;
          evicted = tmp;

          if (!tmp.isValid) {
            /*
            // We reached the end. All other entries in the set are guaranteed to be invalid.
            for (int32_t k=i+1; k < cfg.DirWays; k++) {
              DBG_Assert( !theDirEntrySets[offset+k].isValid );
            }
            // And we still didn't find the entry.
            DBG_Assert( foundAt > cfg.DirWays );
            */
            // So, terminate the loop
            break;
          }

          // Is this the entry we were looking for?
          if (tmp.theAddress == blockAddr) {
            foundAt = i;
            // The ordering is done we can go out
            break;
          }
        }

        // count count count
        theDirFoundOffset << std::make_pair(foundAt, 1);
      }
    }
  }
  /* CMU-ONLY-BLOCK-END */

  ////////////////////// indexing and placement
  tL2SliceID routeCoreReq_Private( const index_t   anIndex
                                   , MemoryMessage & aMessage
                                 ) {
    pageBkdStats( anIndex, aMessage ); /* CMU-ONLY */

    updateCoreToL2HopsStats_Private( anIndex, getTileID(anIndex) ); /* CMU-ONLY */
    theProbedL2 = anIndex;
    return getL2SliceRequestOutPort( anIndex, anIndex );
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  // Go around a ring and return the tile id that has a copy of the block, or an empty slot, based on the flag.
  // The id of the first cache that either has the block or has en empty slot is returned.
  // The search starts from idx+1
  tL2SliceID probeForBlock(   const index_t  anIndex
                              , const tAddress anAddr
                              , const bool     stopAtEmpty
                          ) {
    uint32_t srcL2Slice = anIndex;
    uint32_t destL2Slice = getTileID((srcL2Slice + 1) % theNumL2Slices);
    do {
      theSnoopedL2 = destL2Slice;

      // send the snoop to the next cache in the ring
      theSnoopMessage.address() = PhysicalMemoryAddress(anAddr);
      theSnoopMessage.type() = MemoryMessage::Probe;
      FLEXUS_CHANNEL_ARRAY( ToL2Snoops, destL2Slice ) << theSnoopMessage;

      // break if the line is in the probed cache, or it has en empty slot
      if (   (   theSnoopMessage.type() == MemoryMessage::ProbedClean
                 || theSnoopMessage.type() == MemoryMessage::ProbedDirty
                 || theSnoopMessage.type() == MemoryMessage::ProbedWritable
             )
             || (   stopAtEmpty
                    && theSnoopMessage.type() == MemoryMessage::ProbedNotPresentSetNotFull
                )
         ) {
        break;
      }

      // move src and dest by one step in the ring
      srcL2Slice = destL2Slice;
      destL2Slice = (destL2Slice + 1) % theNumL2Slices;

    } while (destL2Slice != anIndex);

    // return the slice id with the block or an empty spot
    if (destL2Slice != anIndex) {
      return destL2Slice;
    }
    return -1;
  }

  tL2SliceID routeCoreReq_ASR( const index_t   anIndex
                               , MemoryMessage & aMessage
                             ) {
    pageBkdStats( anIndex, aMessage );

    const tCoreID  theCoreID  = anIndex;
    const tAddress anAddr     = aMessage.address();
    const tAddress aBlockAddr = blockAddress(anAddr);

    theProbedL2 = anIndex;

    switch (theType) {
      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
        // We have only copy of the block.
        // -> allocate locally to make sure block is retained on chip
        // Same action as private L2s.
        break;

      case MemoryMessage::EvictClean: {
        // if block is private, perform same action as private L2s
        // FIXME: The current implementation uses the on-chip directory to identify shared-RO data.
        // ASR uses a non-home-node protocol and extra bits in the cache line instead.
        tChipDir::iterator iterChipDir = theChipDir.find(aBlockAddr);
        if (iterChipDir != theChipDir.end()) {
          tChipDirEntry aChipDirEntry = iterChipDir->second;

          DBG_( VVerb, Addr(anAddr) ( << " addr: 0x" << std::hex << anAddr << std::dec
                                      << std::hex
                                      << " State: 0x" << getState(aChipDirEntry)
                                      << " Sharers: 0x" << aChipDirEntry.theSharers[1] << aChipDirEntry.theSharers[0]
                                      << " access: 0x" << theType
                                      << std::dec
                                    ));

          if (!isShared(aChipDirEntry)) {
            // if the block has been modified in the past, or it is the first time we see it, do nothing special.
            break;
          }
          rmSharer(aChipDirEntry, theCoreID);
          if (noSharersLeft(aChipDirEntry)) {
            // if this is a private block, do nothing special
            break;
          }
        }

        // this is a shared block (we are not the only ones on chip with a copy)
        // -> do probabilistic ASR allocation in local slice
        int32_t aThresholdOfAllocation(theASR_ReplicationThresholds[theASR_ReplicationLevel[theProbedL2]]);
        int32_t aThresholdOfAllocationAtNextHigherLevel(theASR_ReplicationThresholds[theASR_ReplicationLevel[theProbedL2] + 1]);
        int32_t aThresholdOfAllocationAtNextLowerLevel(theASR_ReplicationThresholds[theASR_ReplicationLevel[theProbedL2] - 1]);

        int32_t aRandomNumber( (rand_r(&theASRRandomContext) >> 6) % 256 );
        // aRandomNumber = -1;  // Hardcode ASR to always allocate locally
        // aRandomNumber = 255; // Hardcode ASR to never allocate locally
        if (aRandomNumber >= aThresholdOfAllocation) {
          // Let the block go through the local L2. If it doesn't find it there, it will go through the ring.
          aMessage.type() = MemoryMessage::EvictCleanNonAllocating;
        }

        do {
          // update the NLHB for probed(local) L2 slice (these "would be" replicated if replication level was higher)
          tOrderedAddrList & aNLHB(theNLHB[theProbedL2]);
          tOrderedAddrList_addr_iter aNLHB_entry(aNLHB.get<by_address>().find(aBlockAddr));
          if (aNLHB_entry != aNLHB.get<by_address>().end()) {
            aNLHB.relocate(aNLHB.get<by_LRU>().end(), aNLHB.project<by_LRU>(aNLHB_entry));
          } else if (aRandomNumber >= aThresholdOfAllocation && aRandomNumber < aThresholdOfAllocationAtNextHigherLevel) {
            theASR_NLHBInsert = true;
          }

          // if this allocation would not have taken place at lower replication level then...
          if (aRandomNumber < aThresholdOfAllocation && aRandomNumber >= aThresholdOfAllocationAtNextLowerLevel) {
            // -> ... add the victim to VTB as evidence that replication level should be lowered
            // (VTB entries are actually added on *Evict from L2)
            theASR_InsertInVTB = true; // warning: also used bellow for managing theCurrentReplicationOnly
            DBG_(VVerb, ( << "This alloc NOT in lower RL for L2[" << theCoreID << "]" ));
          }
        } while (0);
      }
      break;

      case MemoryMessage::FetchReq:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        // same action as private L2s
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }

    // ASR bookkeeping
    switch (theType) {
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable: {
        std::set<tAddress>& aCurrentReplicationOnly(theCurrentReplicationOnly[anIndex]);
        // this block will be allocated only in current replication level and not in the next lower one
        if (theASR_InsertInVTB /* using flag for information purposes only, not related to VTB here */) {
          DBG_Assert((uint32_t)theProbedL2 == anIndex, ( << "Must be a local allocation" ) );
          // set the current-replication-only bit on local allocation
          aCurrentReplicationOnly.insert(aBlockAddr);
        } else if ((uint32_t)theProbedL2 == anIndex) {
          // this block will be allocated locally no matter what, (even at lower replication level)
          // -> clear current-replication-only bit if it is set
          aCurrentReplicationOnly.erase(aBlockAddr);
        }
      }
      break;

      case MemoryMessage::FetchReq:
      case MemoryMessage::EvictCleanNonAllocating: // do nothing, no effect on theCurrentReplicationOnly
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        // do nothing
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }

    updateCoreToL2HopsStats_Private( theCoreID, getTileID(theProbedL2) );
    return getL2SliceRequestOutPort( theProbedL2, theCoreID );
  }
  /* CMU-ONLY-BLOCK-END */

  // send snoop from Bus up in the cache hierarchy
  void propagateSnoopUp_Private( index_t anIndex
                                 , MemoryMessage & aMessage
                               ) {
    DBG_Assert( anIndex == 0 );

    const tAddress anAddr     = aMessage.address();
    const tAddress aBlockAddr = blockAddress(anAddr);

    uint64_t theAccessType;
    switch (aMessage.type()) {
      case MemoryMessage::Downgrade:
        theAccessType = kDowngradeAccess;
        break;
      case MemoryMessage::Invalidate:
        theAccessType = kInvalidateAccess;
        break;
      case MemoryMessage::ReturnReq:
        theAccessType = kReturnReqAccess;
        break;
      default:
        DBG_Assert( false );
        theAccessType = kInvalidateAccess; // compiler happy
    }

    // get the chip-wide entry
    tChipDir::iterator iterChipDir = theChipDir.find(aBlockAddr);
    bool is_new = false;
    if (iterChipDir == theChipDir.end()) {
      // make new directory entry
      tChipDirEntry newChipDirEntry = anInvalChipDirEntry;
      theChipDir.insert(std::make_pair(aBlockAddr, newChipDirEntry));
      iterChipDir = theChipDir.find(aBlockAddr);
      is_new = true;
    }
    tChipDirEntry & theChipDirEntry = iterChipDir->second;

    /* CMU-ONLY-BLOCK-BEGIN */
    // XJ:
    countBoundedDirEntries ( aBlockAddr
                             , is_new
                             , false  // eraseEntry
                           );
    /* CMU-ONLY-BLOCK-END */

    // compute the state transition & actions to take
    uint64_t theOldState = getState(theChipDirEntry);
    const uint64_t theActions = theChipDirFSM[ theOldState | theAccessType ];
    DBG_Assert( theActions != kPoison, ( << " addr: 0x" << std::hex << anAddr << std::dec
                                         << std::hex
                                         << " oldState: 0x" << theOldState
                                         << " oldSharers: 0x" << theChipDirEntry.theSharers[1] << theChipDirEntry.theSharers[0]
                                         << " access: 0x" << theAccessType
                                         << std::dec
                                       ));

    const uint64_t theSnoop = theActions & kSnoopMask;
    const uint64_t theOffChipReply = theActions & kOffChipReqMask;
    const uint64_t theNewState = theActions & kStateMask;

    DBG_( VVerb, Addr(anAddr) ( << " addr: 0x" << std::hex << anAddr << std::dec
                                << " " << aMessage
                                << std::hex
                                << " oldState: 0x" << theOldState
                                << " oldSharers: 0x" << theChipDirEntry.theSharers[1] << theChipDirEntry.theSharers[0]
                                << " access: 0x" << theAccessType
                                << " newSnoop: 0x" << theSnoop
                                << " newOCR: 0x" << theOffChipReply
                                << " newState: 0x" << theNewState
                                << std::dec
                              ));

    // send snoops
    switch (theSnoop) {
      case kNoSnoop:
        break;

      case kReturnReq: {
        const int32_t aSharerCoreID = getRandomSharer( theChipDirEntry, -1 );
        DBG_Assert ( aSharerCoreID != -1 );
        aMessage.type() = MemoryMessage::ReturnReq;
        FLEXUS_CHANNEL_ARRAY( ToL2Snoops, aSharerCoreID ) << aMessage;
        break;
      }

      case kInvalidate: {
        if (theOldState != kModified) {
          for (uint32_t i = 0; i < theNumL2Slices; i++) {
            if (isSharer( theChipDirEntry, i )) {
              aMessage.type() = MemoryMessage::Invalidate;
              FLEXUS_CHANNEL_ARRAY( ToL2Snoops, i ) << aMessage;
            }
          }
        } else {
          const int32_t theOwner = getOwner( theChipDirEntry );
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theOwner ) << aMessage;
          DBG_Assert ( aMessage.type() == MemoryMessage::InvUpdateAck, ( << aMessage ));
        }
        break;
      }

      case kDowngrade: {
        const int32_t theOwner = getOwner( theChipDirEntry );
        FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theOwner ) << aMessage;
        clearSharersAndOwner(theChipDirEntry);
        addSharer(theChipDirEntry, theOwner);
        break;
      }

      default:
        DBG_Assert( false, ( << std::hex
                             << " oldState: 0x" << theOldState
                             << " access: 0x" << theAccessType
                             << " snoop: 0x" << theSnoop
                             << " OCR: 0x" << theOffChipReply
                             << " newState: 0x" << theNewState
                             << std::dec
                           ));
    }

    // make sure the off-chip reply type is the appropriate one
    switch (theOffChipReply) {
      case kNoOffChipReq:
        // just use the reply type returned from the L2s
        break;

      case kDownUpdateAckOCR:
        aMessage.type() = MemoryMessage::DownUpdateAck;
        break;

      case kInvUpdateAckOCR:
        aMessage.type() = MemoryMessage::InvUpdateAck;
        break;

      default:
        DBG_Assert( false );
    }

    // fix the on-chip state
    bool theDirEntryErased = false;
    switch (theNewState) {
      case kInvalid:
        // clearSharersAndOwner(theChipDirEntry);
        // setState(theChipDirEntry, kInvalid);
        theChipDir.erase( iterChipDir );
        theDirEntryErased = true;
        /* CMU-ONLY-BLOCK-BEGIN */
        // XJ:
        countBoundedDirEntries ( aBlockAddr
                                 , false  // is_new
                                 , true   // eraseEntry
                               );
        /* CMU-ONLY-BLOCK-END */
        break;

      case kShared:
        setState(theChipDirEntry, kShared);
        break;

      default:
        DBG_Assert( false );
    }

    DBG_( VVerb, Addr(anAddr) ( << " addr: 0x" << std::hex << anAddr << std::dec
                                << std::hex
                                << " oldState: 0x" << theOldState
                                << " access: 0x" << theAccessType
                                << " newSnoop: 0x" << theSnoop
                                << " newOCR: 0x" << theOffChipReply
                                << " newState: 0x" << (theDirEntryErased ? 0xffffffffffffffffULL : getState(theChipDirEntry))
                                << " newSharers: 0x" << (theDirEntryErased ? 0xffffffffffffffffULL : theChipDirEntry.theSharers[1])
                                << (theDirEntryErased ? 0xffffffffffffffffULL : theChipDirEntry.theSharers[0])
                                << std::dec
                              ));
  }

  // Process requests coming from L2.
  // A chip-wide directory keeps coherence information on a per-block basis
  // and snoops the other L2 slices for data to minimize the number of off-chip requests.
  // This is used for ASR too.              /* CMU-ONLY */
  void processRequestFromL2_Private( index_t anIndex
                                     , MemoryMessage & aMessage
                                   ) {
    //keep a snapshot of the system for reclassifications and evicts
    const int32_t tmp_theProbedL2 = theProbedL2;
    const int32_t tmp_theSnoopedL2 = theSnoopedL2;
    const int32_t tmp_theDir = theDir;
    const int32_t tmp_thePeerL1 = thePeerL1;
    const bool tmp_theDstream = theDstream;
    const MemoryMessage::MemoryMessageType tmp_theType = theType;

    const tCoreID  theCoreID  = anIndex;
    const tAddress anAddr     = aMessage.address();
    const tAddress aBlockAddr = blockAddress(anAddr);

    // Find the Dir and calculate the hops to reach it.
    // Assume a static address-interleaved distributed directory, as a centralized one will be a bottleneck.
    theDir = ((anAddr >> theCacheLineSizeLog2) % theNumL2Slices);
    DBG_Assert( theDir < static_cast<int>(theNumL2Slices) );

    /* CMU-ONLY-BLOCK-BEGIN */
    updateDirHopStats_Private(theProbedL2, theDir);

    if (   theL2Type == MemoryMessage::EvictClean
           || theL2Type == MemoryMessage::EvictWritable
           || theL2Type == MemoryMessage::EvictDirty
           || theL2Type == MemoryMessage::EvictDirtyNonAllocating
           || theL2Type == MemoryMessage::EvictL2CleanNonAllocating
           // || theL2Type == MemoryMessage::EvictCleanNonAllocating these do not cause victims, so can't insert anything into VTB
       ) {
      // update VTB for cases where blocks are evicted from(local) L2 slice (these "would not be" evicted if replication was lower)
      if (thePrivateWithASR) {
        tAddress aEvictedAddress(aMessage.address());
        tOrderedAddrList & aVTB(theVTB[anIndex]);
        tOrderedAddrList_addr_iter aVTB_entry(aVTB.get<by_address>().find(aEvictedAddress));
        if (aVTB_entry != aVTB.get<by_address>().end()) {
          DBG_( VTBDbg, ( << "L2[" << theCoreID << "] VTB relocate to tail " << std::hex << aEvictedAddress << std::dec ));
          aVTB.relocate(aVTB.get<by_LRU>().end(), aVTB.project<by_LRU>(aVTB_entry));
        } else if (theASR_InsertInVTB) {
          aVTB.get<by_LRU>().push_back(aEvictedAddress);
          DBG_( VTBDbg, ( << "L2[" << theCoreID << "] VTB put in " << std::hex << aEvictedAddress << std::dec << " size=" << aVTB.size() ));
          if (aVTB.size() == static_cast<uint32_t>(cfg.VTBSize)) {
            DBG_( VTBDbg, ( << "L2[" << theCoreID << "] VTB pop out " << std::hex << aVTB.get<by_LRU>().front() << std::dec ));
            aVTB.get<by_LRU>().pop_front();
          }
        }
        theCurrentReplicationOnly[anIndex].erase(aBlockAddr);
      }
    }
    /* CMU-ONLY-BLOCK-END */

    // get the access type
    uint64_t theAccessType;
    switch (theL2Type) {
      case MemoryMessage::FetchReq:
        theAccessType = kFetchAccess;
        break;

      case MemoryMessage::ReadReq:
        theAccessType = kReadAccess;
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        theAccessType = kWriteAccess;
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        theAccessType = kUpgradeAccess;
        break;

      case MemoryMessage::EvictDirty:
        theAccessType = kDirtyEvictAccess;
        break;

      case MemoryMessage::EvictWritable:
        theAccessType = kWritableEvictAccess;
        break;

      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
        theAccessType = kCleanEvictAccess;
        break;

        /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::EvictCleanNonAllocating:
        theAccessType = kCleanEvictNonAllocatingAccess;
        break;

      case MemoryMessage::EvictDirtyNonAllocating:
        theAccessType = kDirtyEvictNonAllocatingAccess;
        break;

      case MemoryMessage::EvictL2CleanNonAllocating:
        theAccessType = kL2CleanEvictNonAllocatingAccess;
        break;
        /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
        theAccessType = kFetchAccess; // compiler happy
        break;
    }

    // get the chip-wide entry
    tChipDir::iterator iterChipDir = theChipDir.find(aBlockAddr);
    bool isChipDirEntryNew = false;
    if (iterChipDir == theChipDir.end()) {
      // make new directory entry
      tChipDirEntry newChipDirEntry = anInvalChipDirEntry;
      if (theL2Type == MemoryMessage::FetchReq) {
        setState(newChipDirEntry, (newChipDirEntry.theState | kHasFetchRef));
      }
      theChipDir.insert(std::make_pair(aBlockAddr, newChipDirEntry));
      iterChipDir = theChipDir.find(aBlockAddr);
      isChipDirEntryNew = true;
    }
    tChipDirEntry & theChipDirEntry = iterChipDir->second;

    /* CMU-ONLY-BLOCK-BEGIN */
    // XJ:
    countBoundedDirEntries ( aBlockAddr
                             , isChipDirEntryNew
                             , false  // eraseEntry
                           );
    /* CMU-ONLY-BLOCK-END */

    // WhiteBox debugging
    if (theL2Type == MemoryMessage::FetchReq) {
      theChipDirEntry.theState |= kHasFetchRef;
    } else if (    (theChipDirEntry.theState & kHasFetchRef)
                   && theType != MemoryMessage::EvictClean
                   && theType != MemoryMessage::EvictWritable
                   && theType != MemoryMessage::EvictDirty
              ) {
      theChipDirEntry.theState &= ~kHasFetchRef;
      if (cfg.WhiteBoxDebug) {
        nWhiteBox::CPUState state;
        nWhiteBox::WhiteBox::getWhiteBox()->getState(theCoreID, state);
        DBG_(VVerb, ( << " Data ref after fetch " << state ));
      }
    }

    // compute the state transition & actions to take
    uint64_t theOldState = getState(theChipDirEntry);
    const uint64_t theActions = theChipDirFSM[ theOldState | theAccessType ];
    DBG_Assert( theActions != kPoison, ( << " addr: 0x" << std::hex << anAddr << std::dec
                                         << " core: " << theCoreID
                                         << std::hex
                                         << " oldState: 0x" << theOldState
                                         << " oldSharers: 0x" << theChipDirEntry.theSharers[1] << theChipDirEntry.theSharers[0]
                                         << " access: 0x" << theAccessType
                                         << std::dec
                                       ));

    const uint64_t theSnoop = theActions & kSnoopMask;
    const uint64_t theNewState = theActions & kStateMask;
    theOffChipRequest = theActions & kOffChipReqMask;

    DBG_( VVerb, Addr(anAddr) ( << " addr: 0x" << std::hex << anAddr << std::dec
                                << " core: " << theCoreID
                                << " " << aMessage
                                << std::hex
                                << " oldState: 0x" << theOldState
                                << " oldSharers: 0x" << theChipDirEntry.theSharers[1] << theChipDirEntry.theSharers[0]
                                << " access: 0x" << theAccessType
                                << " newSnoop: 0x" << theSnoop
                                << " newOCR: 0x" << theOffChipRequest
                                << " newState: 0x" << theNewState
                                << std::dec
                              ));

    // send snoops
    switch (theSnoop) {
      case kNoSnoop:
        break;

      case kReturnReq: {
        int32_t aSharerCoreID =  getRandomSharer( theChipDirEntry, theCoreID );
        DBG_Assert ( aSharerCoreID != -1, ( << " addr: 0x" << std::hex << anAddr << std::dec
                                            << " core: " << theCoreID
                                            << std::hex
                                            << " oldState: 0x" << theOldState
                                            << " oldSharers: 0x" << theChipDirEntry.theSharers[1] << theChipDirEntry.theSharers[0]
                                            << " access: 0x" << theAccessType
                                            << " newSnoop: 0x" << theSnoop
                                            << " newOCR: 0x" << theOffChipRequest
                                            << " newState: 0x" << theNewState
                                            << std::dec
                                          ));

        theSnoopedL2 = aSharerCoreID;
        updateSnoopedL2HopStats_Private(theDir, theSnoopedL2 ); /* CMU-ONLY */

        theDirSnoopMessage = aMessage;
        theDirSnoopMessage.type() = MemoryMessage::ReturnReq;
        FLEXUS_CHANNEL_ARRAY( ToL2Snoops, aSharerCoreID ) << theDirSnoopMessage;
        theRemoteL2Hit = true; /* CMU-ONLY */
        aMessage.type() = MemoryMessage::MissReply;
        break;
      }

      case kInvalidate: {
        if (theOldState != kModified) {
          MemoryMessage::MemoryMessageType theReturnType = MemoryMessage::InvalidateAck;
          for (uint32_t i = 0; i < theNumL2Slices; i++) {
            if (i != theCoreID && isSharer( theChipDirEntry, i )) {

              theSnoopedL2 = i;
              updateSnoopedL2HopStats_Private(theDir, theSnoopedL2 ); /* CMU-ONLY */

              theDirSnoopMessage = aMessage;
              theDirSnoopMessage.type() = MemoryMessage::Invalidate;
              FLEXUS_CHANNEL_ARRAY( ToL2Snoops, i ) << theDirSnoopMessage;
              if (theDirSnoopMessage.type() == MemoryMessage::InvUpdateAck) {
                theReturnType = MemoryMessage::InvUpdateAck;
              }
            }
          }
          theDirSnoopMessage.type() = theReturnType;
        } else {
          const int32_t theOwner = getOwner( theChipDirEntry );

          theSnoopedL2 = theOwner;
          updateSnoopedL2HopStats_Private(theDir, theSnoopedL2 ); /* CMU-ONLY */

          theDirSnoopMessage = aMessage;
          theDirSnoopMessage.type() = MemoryMessage::Invalidate;
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theOwner ) << theDirSnoopMessage;
        }
        clearSharersAndOwner(theChipDirEntry);
        aMessage.type() = MemoryMessage::MissReplyDirty;
        break;
      }

      case kDowngrade: {
        const int32_t theOwner = getOwner( theChipDirEntry );

        theSnoopedL2 = theOwner;
        updateSnoopedL2HopStats_Private(theDir, theSnoopedL2 ); /* CMU-ONLY */

        theDirSnoopMessage = aMessage;
        theDirSnoopMessage.type() = MemoryMessage::Downgrade;
        FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theOwner ) << theDirSnoopMessage;
        theRemoteL2Hit = true; /* CMU-ONLY */
        clearSharersAndOwner(theChipDirEntry);
        addSharer(theChipDirEntry, theOwner);
        aMessage.type() = MemoryMessage::MissReply;
        break;
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      case kProbe: {
        // Allocate the block somewhere on the ring (either a sharer or at a cache with an empty block frame)
        // FIXME: is it correct to return the first empty cache frame found? Also, search dir for sharers first.
        // FIXME: probes should also go to L1 ?
        // FIXME: should ring start from dir ?
        theSnoopedL2 = probeForBlock( theCoreID, anAddr, true );
        if (theSnoopedL2 != -1) {
          theDirSnoopMessage = aMessage;
          theDirSnoopMessage.type() = MemoryMessage::EvictCleanNonAllocating ;
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theSnoopedL2 ) << theDirSnoopMessage;
          addSharer(theChipDirEntry, theSnoopedL2);
        }
        rmSharer(theChipDirEntry, theCoreID);
        break;
      }

      case kProbeDirty: {
        // Allocate the block somewhere on the ring (either a sharer or at a cache with an empty block frame)
        // FIXME: is it correct to return the first empty cache frame found? Also, search dir for sharers first.
        // FIXME: probes should also go to L1 ?
        // FIXME: should ring start from dir ?
        theSnoopedL2 = probeForBlock( theCoreID, anAddr, true );
        if (theSnoopedL2 != -1) {
          theDirSnoopMessage = aMessage;
          theDirSnoopMessage.type() = MemoryMessage::EvictDirtyNonAllocating;
          FLEXUS_CHANNEL_ARRAY( ToL2Snoops, theSnoopedL2 ) << theDirSnoopMessage;
          setOwner(theChipDirEntry, theSnoopedL2);
        }
        break;
      }
      /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert( false, ( << std::hex
                             << " oldState: 0x" << theOldState
                             << " access: 0x" << theAccessType
                             << " snoop: 0x" << theSnoop
                             << " OCR: 0x" << theOffChipRequest
                             << " newState: 0x" << theNewState
                             << std::dec
                           ));
    }

    // send off-chip request
    switch (theOffChipRequest) {
      case kNoOffChipReq:
        break;

      case kFetchOCR:
        DBG_Assert(theL2Type == aMessage.type() && theL2Type == MemoryMessage::FetchReq);
        goto sendOCR;
      case kReadOCR:
        DBG_Assert(theL2Type == aMessage.type() && theL2Type == MemoryMessage::ReadReq);
        goto sendOCR;
      case kWriteOCR:
        DBG_Assert(theL2Type == aMessage.type() && theL2Type == MemoryMessage::WriteReq);
        goto sendOCR;
      case kDirtyEvictOCR:
        DBG_Assert(theL2Type == aMessage.type() && theL2Type == MemoryMessage::EvictDirty);
        goto sendOCR;
      case kWritableEvictOCR:
        DBG_Assert(theL2Type == aMessage.type() && theL2Type == MemoryMessage::EvictWritable);
        {
sendOCR:
          updateOffChipStats_Private(theCoreID, aMessage); /* CMU-ONLY */
          updateReplyHopsStats(theDir, theCoreID); /* CMU-ONLY */
          FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
          break;
        }

      case kUpgradeOCR: {
        const bool fromUpg = (aMessage.type() == MemoryMessage::UpgradeReq) ? true : false;
        aMessage.type() = MemoryMessage::UpgradeReq;
        updateOffChipStats_Private(theCoreID, aMessage); /* CMU-ONLY */
        updateReplyHopsStats(theDir, theCoreID); /* CMU-ONLY */
        FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
        if (!fromUpg && aMessage.type() == MemoryMessage::UpgradeReply) {
          aMessage.type() = MemoryMessage::MissReplyDirty;
        }
        break;
      }

      case kSharersDependantOCR: {
        // no sharers left   -> kCleanEvictOCR (if oldState==kShared) or kDirtyEvictOCR (if oldState==kOwned)
        // some sharers left -> no OCR

        // If this core is really the last sharer of the block, send the evict off chip.
        // The actual sharers bitmask cleanup happens on the next switch statement.
        tChipDirEntry tmpChipDirEntry = theChipDirEntry;
        rmSharer(tmpChipDirEntry, theCoreID);
        if (noSharersLeft( tmpChipDirEntry )) {
          updateOffChipStats_Private(theCoreID, aMessage); /* CMU-ONLY */
          updateReplyHopsStats(theDir, theCoreID); /* CMU-ONLY */
          if (theOldState == kOwned) {
            aMessage.type() = MemoryMessage::EvictDirty;
          }
          FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
        }
        break;
      }

      case kCleanEvictNonAllocatingOCR: {
        if (theSnoopedL2 == -1) {
          if (noSharersLeft( theChipDirEntry )) {
            // nobody on the ring had space for the block. Send it off chip.
            aMessage.type() = MemoryMessage::EvictClean;
            updateOffChipStats_Private(theCoreID, aMessage); /* CMU-ONLY */
            updateReplyHopsStats(theDir, theCoreID); /* CMU-ONLY */
            if (theOldState == kOwned) {
              aMessage.type() = MemoryMessage::EvictDirty;
            }
            FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
          }
        }
        break;
      }

      case kDirtyEvictNonAllocatingOCR: {
        if (theSnoopedL2 == -1) {
          // nobody on the ring had space for the block. Send it off chip.
          aMessage.type() = MemoryMessage::EvictDirty;
          updateOffChipStats_Private(theCoreID, aMessage); /* CMU-ONLY */
          updateReplyHopsStats(theDir, theCoreID); /* CMU-ONLY */
          FLEXUS_CHANNEL_ARRAY( RequestOutMem, 0 ) << aMessage;
        }
        break;
      }

      default:
        DBG_Assert( false );
    }

    // transition to the new state
    bool theDirEntryErased = false;
    switch (theNewState) {
      case kInvalid:
        // clearSharersAndOwner(theChipDirEntry);
        // setState( theChipDirEntry, theNewState );
        theChipDir.erase( iterChipDir );
        theDirEntryErased = true;
        break;

      case kShared:
      case kOwned:
        addSharer(theChipDirEntry, theCoreID);
        setState( theChipDirEntry, theNewState );
        break;

      case kModified:
        setOwner(theChipDirEntry, theCoreID);
        setState( theChipDirEntry, theNewState );
        break;

      case kReplyDependantState:
        if (   aMessage.type() == MemoryMessage::MissReplyWritable
               || aMessage.type() == MemoryMessage::MissReplyDirty
           ) {
          DBG_Assert(theOldState == kInvalid);
          addSharer(theChipDirEntry, theCoreID);
          setState( theChipDirEntry, kOwned );
          aMessage.type() = MemoryMessage::MissReply;
        } else {
          addSharer(theChipDirEntry, theCoreID);
          setState( theChipDirEntry, kShared );
        }
        break;

      case kSharersDependantState:
        // no sharers left   -> kInvalid
        // some sharers left -> state remains unchanged (kShared or KOwned)
        rmSharer(theChipDirEntry, theCoreID);
        if ( noSharersLeft( theChipDirEntry ) ) {
          theChipDir.erase( iterChipDir );
          theDirEntryErased = true;
        }
        break;

      case kSharersDirtyDependantState:
        // if there was no free frame to allocate the dirty evict, remove the dir entry
        if (theSnoopedL2 == -1) {
          theChipDir.erase( iterChipDir );
          theDirEntryErased = true;
        }
        break;

      default:
        DBG_Assert( false );
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    // XJ:
    if (theDirEntryErased) {
      countBoundedDirEntries ( aBlockAddr
                               , false  // is_new
                               , true   // eraseEntry
                             );
    }
    /* CMU-ONLY-BLOCK-END */

    DBG_( VVerb, Addr(anAddr) ( << " addr: 0x" << std::hex << anAddr << std::dec
                                << " core: " << theCoreID
                                << " " << aMessage
                                << std::hex
                                << " oldState: 0x" << theOldState
                                << " access: 0x" << theAccessType
                                << " newSnoop: 0x" << theSnoop
                                << " newOCR: 0x" << theOffChipRequest
                                << " newState: 0x" << (theDirEntryErased ? 0xffffffffffffffffULL : getState(theChipDirEntry))
                                << " newSharers: 0x" << (theDirEntryErased ? 0xffffffffffffffffULL : theChipDirEntry.theSharers[1])
                                << (theDirEntryErased ? 0xffffffffffffffffULL : theChipDirEntry.theSharers[0])
                                << std::dec
                              ));

    //restore the snapshot of the system for reclassifications and evicts
    if (   theL2Type == MemoryMessage::EvictDirty
           || theL2Type == MemoryMessage::EvictWritable
           || theL2Type == MemoryMessage::EvictClean
           /* CMU-ONLY-BLOCK-BEGIN */
           || theL2Type == MemoryMessage::EvictCleanNonAllocating
           || theL2Type == MemoryMessage::EvictDirtyNonAllocating
           || theL2Type == MemoryMessage::EvictL2CleanNonAllocating
           /* CMU-ONLY-BLOCK-END */
           || theL2Type == MemoryMessage::PrefetchInsert
       ) {
      theProbedL2 = tmp_theProbedL2;
      theSnoopedL2 = tmp_theSnoopedL2;
      theDir = tmp_theDir;
      thePeerL1 = tmp_thePeerL1;
      theDstream = tmp_theDstream;
      theType = tmp_theType;
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////// stats
  // hop counts
  void updateCoreToL2HopsStats_Private( const tCoreID aCoreID
                                        , const tL2SliceID anL2ID
                                      ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
        theCoreToL2Hops_I_Fetch << std::make_pair(coreToL2Hops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
        theCoreToL2Hops_D_Read << std::make_pair(coreToL2Hops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
        theCoreToL2Hops_D_Write << std::make_pair(coreToL2Hops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
        theCoreToL2Hops_D_Upg << std::make_pair(coreToL2Hops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        coreToL2Hops = theHops[ aCoreID ][ anL2ID ];
        theCoreToL2Hops_D_Atomic << std::make_pair(coreToL2Hops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  void updateDirHopStats_Private( const tL2SliceID anL2ID
                                  , const tL2SliceID aDirID
                                ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        dirHops = theHops[ anL2ID ][ aDirID ];
        theDirHops_I_Fetch << std::make_pair(dirHops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        dirHops = theHops[ anL2ID ][ aDirID ];
        theDirHops_D_Read << std::make_pair(dirHops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        dirHops = theHops[ anL2ID ][ aDirID ];
        theDirHops_D_Write << std::make_pair(dirHops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        dirHops = theHops[ anL2ID ][ aDirID ];
        theDirHops_D_Upg << std::make_pair(dirHops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        dirHops = theHops[ anL2ID ][ aDirID ];
        theDirHops_D_Atomic << std::make_pair(dirHops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  void updateSnoopedL2HopStats_Private( const tL2SliceID aDirID
                                        , const tL2SliceID anL2ID
                                      ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        snoopedL2Hops = theHops[ aDirID ][ anL2ID ];
        theSnoopedL2Hops_I_Fetch << std::make_pair(snoopedL2Hops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        snoopedL2Hops = theHops[ aDirID ][ anL2ID ];
        theSnoopedL2Hops_D_Read << std::make_pair(snoopedL2Hops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        snoopedL2Hops = theHops[ aDirID ][ anL2ID ];
        theSnoopedL2Hops_D_Write << std::make_pair(snoopedL2Hops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        snoopedL2Hops = theHops[ aDirID ][ anL2ID ];
        theSnoopedL2Hops_D_Upg << std::make_pair(snoopedL2Hops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        snoopedL2Hops = theHops[ aDirID ][ anL2ID ];
        theSnoopedL2Hops_D_Atomic << std::make_pair(snoopedL2Hops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  void updatePeerL1HopsStats_Private( const tL2SliceID anL2ID
                                      , const tCoreID aCoreID
                                    ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        peerL1Hops = theHops[ anL2ID ][ aCoreID ];
        thePeerL1Hops_I_Fetch << std::make_pair(peerL1Hops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        peerL1Hops = theHops[ anL2ID ][ aCoreID ];
        thePeerL1Hops_D_Read << std::make_pair(peerL1Hops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        peerL1Hops = theHops[ anL2ID ][ aCoreID ];
        thePeerL1Hops_D_Write << std::make_pair(peerL1Hops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        peerL1Hops = theHops[ anL2ID ][ aCoreID ];
        thePeerL1Hops_D_Upg << std::make_pair(peerL1Hops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        peerL1Hops = theHops[ anL2ID ][ aCoreID ];
        thePeerL1Hops_D_Atomic << std::make_pair(peerL1Hops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  void updateReplyHopsStats_Private( const tL2SliceID anL2ID
                                     , const tCoreID aCoreID
                                   ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        replyHops = theHops[ anL2ID ][ aCoreID ];
        theReplyHops_I_Fetch << std::make_pair(replyHops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        replyHops = theHops[ anL2ID ][ aCoreID ];
        theReplyHops_D_Read << std::make_pair(replyHops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        replyHops = theHops[ anL2ID ][ aCoreID ];
        theReplyHops_D_Write << std::make_pair(replyHops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        replyHops = theHops[ anL2ID ][ aCoreID ];
        theReplyHops_D_Upg << std::make_pair(replyHops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        replyHops = theHops[ anL2ID ][ aCoreID ];
        theReplyHops_D_Atomic << std::make_pair(replyHops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  void updateReqRoundTripHopsStats_Private( const int32_t reqRoundTripHops ) {
    switch (theType) {
      case MemoryMessage::FetchReq:
        theReqRoundTripHops_I_Fetch << std::make_pair(reqRoundTripHops, 1);
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        theReqRoundTripHops_D_Read << std::make_pair(reqRoundTripHops, 1);
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        theReqRoundTripHops_D_Write << std::make_pair(reqRoundTripHops, 1);
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        theReqRoundTripHops_D_Upg << std::make_pair(reqRoundTripHops, 1);
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        theReqRoundTripHops_D_Atomic << std::make_pair(reqRoundTripHops, 1);
        break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictCleanNonAllocating:
      case MemoryMessage::EvictL2CleanNonAllocating:
      case MemoryMessage::EvictDirtyNonAllocating:
        break;

      default:
        DBG_Assert( (false), ( << "Type: " << theType ) ); //Unhandled message type
    }
  }

  // off-chip misses
  void updateOffChipStats_Private( index_t anIndex
                                   , MemoryMessage & aMessage
                                 ) {
    switch (aMessage.type()) {
      case MemoryMessage::FetchReq:
        theOffChip_I_Fetch ++;
        break;

      case MemoryMessage::LoadReq:
      case MemoryMessage::ReadReq:
        theOffChip_D_Read ++;
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        theOffChip_D_Write ++;
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        theOffChip_D_Upg ++;
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        theOffChip_D_Atomic ++;
        break;

      case MemoryMessage::EvictClean:
        if (aMessage.dstream()) {
          theOffChip_D_EvictClean ++;
        } else {
          theOffChip_I_Evict ++;
        }
        break;

      case MemoryMessage::EvictDirty:
        theOffChip_D_EvictDirty ++;
        break;

      case MemoryMessage::EvictWritable:
        theOffChip_D_EvictWritable ++;
        break;

      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
    }
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////// shared address-interleaved L2

  ////////////////////// indexing and placement
  tL2SliceID routeCoreReq_Shared( const index_t   anIndex
                                  , MemoryMessage & aMessage
                                ) {
    pageBkdStats( anIndex, aMessage ); /* CMU-ONLY */

    aMessage.idxExtraOffsetBits() = theNumL2SlicesLog2;
    tL2SliceID anL2ID = ((aMessage.address() >> theCacheLineSizeLog2) % theNumL2Slices);
    tL2SliceID anL2Port = anL2ID * theNumCores + anIndex;
    theProbedL2 = anL2ID;
    updateCoreToL2HopsStats_Shared( anIndex, getTileID(anL2ID) );  /* CMU-ONLY */
    return anL2Port;
  }

  // send snoop from Bus up in the cache hierarchy
  void propagateSnoopUp_Shared( index_t anIndex
                                , MemoryMessage & aMessage
                              ) {
    tL2SliceID L2SliceID = ((aMessage.address() >> theCacheLineSizeLog2) % theNumL2Slices);
    DBG_Assert( L2SliceID < theNumL2Slices );
    aMessage.idxExtraOffsetBits() = theNumL2SlicesLog2;
    FLEXUS_CHANNEL_ARRAY( ToL2Snoops, L2SliceID ) << aMessage;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  ////////////////////// stats
  // hop counts
  void updateCoreToL2HopsStats_Shared( const tCoreID aCoreID
                                       , const tL2SliceID anL2ID
                                     ) {
    updateCoreToL2HopsStats_Private( aCoreID, anL2ID );
  }

  void updatePeerL1HopsStats_Shared( const tL2SliceID anL2ID
                                     , const tCoreID aCoreID
                                   ) {
    updatePeerL1HopsStats_Private( aCoreID, anL2ID );
  }

  void updateReplyHopsStats_Shared( const tL2SliceID anL2ID
                                    , const tCoreID aCoreID
                                  ) {
    updateReplyHopsStats_Private( aCoreID, anL2ID );
  }

  void updateReqRoundTripHopsStats_Shared( const int32_t reqRoundTripHops ) {
    updateReqRoundTripHopsStats_Private( reqRoundTripHops );
  }

  // off-chip misses
  void updateOffChipStats_Shared( index_t         anIndex
                                  , MemoryMessage & aMessage
                                ) {
    updateOffChipStats_Private( anIndex, aMessage );
  }
  /* CMU-ONLY-BLOCK-END */

  //////////////////// checkpoint save/restore
  void saveState(std::string const & aDirName) {
    savePageMapState( aDirName ); /* CMU-ONLY */
    saveChipDirState( aDirName );
    saveASRState( aDirName ); /* CMU-ONLY */
  }

  void loadState( std::string const & aDirName ) {
    loadPageMapState( aDirName ); /* CMU-ONLY */
    loadChipDirState( aDirName );
    loadASRState ( aDirName ); /* CMU-ONLY */
    DBG_( Dev, ( << "Checkpoint loaded." ));
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  void saveASRState(std::string const & aDirName) {
    std::string fnameASR( aDirName );
    fnameASR += "/" + statName() + "-ASR";
    std::ofstream ofsASR( fnameASR.c_str() );

    ofsASR << "seed " << theASRRandomContext << std::endl;

    saveVector(ofsASR, "theASR_ReplicationLevel", theASR_ReplicationLevel);

    saveVector(ofsASR, "theBenefitOfIncreasingReplication", theBenefitOfIncreasingReplication);
    saveVector(ofsASR, "theCostOfIncreasingReplication", theCostOfIncreasingReplication);
    saveVector(ofsASR, "theBenefitOfDecreasingReplication", theBenefitOfDecreasingReplication);
    saveVector(ofsASR, "theCostOfDecreasingReplication", theCostOfDecreasingReplication);

    saveVector(ofsASR, "theNLHB", theNLHB);
    saveVector(ofsASR, "theSoonToBeEvicted", theSoonToBeEvicted);
    saveVector(ofsASR, "theVTB", theVTB);
    saveVector(ofsASR, "theCurrentReplicationOnly", theCurrentReplicationOnly);

    saveVector(ofsASR, "theASR_LocalReplicationCount", theASR_LocalReplicationCount);
    saveVector(ofsASR, "theASR_NLHBAllocationCount", theASR_NLHBAllocationCount);

    saveVector(ofsASR, "theASR_LastEvaluations", theASR_LastEvaluations);
  }

  void loadASRState(std::string const & aDirName) {
    std::string fnameASR( aDirName );
    fnameASR += "/" + statName() + "-ASR";
    std::ifstream ifsASR( fnameASR.c_str() );

    if (! ifsASR.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fnameASR << " not found.  Resetting to empty ASR structures. " )  );
    } else {

      ifsASR >> fnameASR;
      ifsASR >> theASRRandomContext;

      loadVector<int>(ifsASR, "theASR_ReplicationLevel", theASR_ReplicationLevel);

      loadVector<int>(ifsASR, "theBenefitOfIncreasingReplication", theBenefitOfIncreasingReplication);
      loadVector<int>(ifsASR, "theCostOfIncreasingReplication", theCostOfIncreasingReplication);
      loadVector<int>(ifsASR, "theBenefitOfDecreasingReplication", theBenefitOfDecreasingReplication);
      loadVector<int>(ifsASR, "theCostOfDecreasingReplication", theCostOfDecreasingReplication);

      loadVector(ifsASR, "theNLHB", theNLHB);
      loadVector(ifsASR, "theSoonToBeEvicted", theSoonToBeEvicted);
      loadVector(ifsASR, "theVTB", theVTB);
      loadVector(ifsASR, "theCurrentReplicationOnly", theCurrentReplicationOnly);

      loadVector(ifsASR, "theASR_LocalReplicationCount", theASR_LocalReplicationCount);
      loadVector(ifsASR, "theASR_NLHBAllocationCount", theASR_NLHBAllocationCount);

      loadVector(ifsASR, "theASR_LastEvaluations", theASR_LastEvaluations);
    }
  }

  void savePageMapState(std::string const & aDirName) {
    // save the R-NUCA page map
    std::string fnameRNUCAPageMap( aDirName );
    fnameRNUCAPageMap += "/" + statName() + "-RNUCAPageMap";
    std::ofstream ofsRNUCAPageMap( fnameRNUCAPageMap.c_str() );

    // Write R-NUCA page map info and the number of entries
    ofsRNUCAPageMap << (uint32_t)thePlacement
                    << " " << theNumCoresPerL2Slice
                    << " " << theCacheLineSize
                    << " " << thePageSize
                    << " " << theSizeOfInstrCluster
                    << " " << thePageMap.size()
                    << std::endl;

    // Write out each page map entry
    for ( tPageMap::iterator iter = thePageMap.begin();
          iter != thePageMap.end();
          iter++
        ) {
      uint64_t theAddress = iter->first;

      tPageMapEntry & thePageMapEntry = iter->second;

      DBG_ ( VVerb,
             Addr(theAddress)
             ( << std::hex
               << " Address: "              << theAddress
               << " isDataClassValid: "     << thePageMapEntry.isDataClassValid
               << " isShared: "             << thePageMapEntry.isShared
               << " privateTileID: "        << thePageMapEntry.privateTileID
               << " pageHasInstr: "         << thePageMapEntry.pageHasInstr
               << " HasFetchRef: "          << thePageMapEntry.HasFetchRef
               << " theCachedLineBitmask: " << thePageMapEntry.theCachedLineBitmask[1] << "-" << thePageMapEntry.theCachedLineBitmask[0]
             ) );

      ofsRNUCAPageMap << "{"
                      << " " << theAddress
                      << " " << thePageMapEntry.isDataClassValid
                      << " " << thePageMapEntry.isShared
                      << " " << thePageMapEntry.privateTileID           // owner of private data
                      << " " << thePageMapEntry.pageHasInstr            // not a real page map entry. Used only for stats and for I/D decoupling
                      << " " << thePageMapEntry.HasFetchRef             // for whitebox debugging
                      << " " << thePageMapEntry.theCachedLineBitmask[1] // not a real page map entry. Used only to optimize the simflex code
                      << " " << thePageMapEntry.theCachedLineBitmask[0] // not a real page map entry. Used only to optimize the simflex code
                      << " " << "}"
                      << std::endl;
    }
    ofsRNUCAPageMap.close();
  }

  void loadPageMapState( std::string const & aDirName ) {
    std::string fnameRNUCAPageMap ( aDirName );
    fnameRNUCAPageMap += "/" + statName() + "-RNUCAPageMap";
    std::ifstream ifsRNUCAPageMap ( fnameRNUCAPageMap.c_str() );

    thePageMap.clear(); // empty the page map

    if (! ifsRNUCAPageMap.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fnameRNUCAPageMap << " not found.  Resetting to empty R-NUCA page table. " )  );
    } else {
      ifsRNUCAPageMap >> std::skipws; // skip white space

      // Read the R-NUCA page map info
      int32_t page_map_size = 0;
      uint32_t aPlacement;
      ifsRNUCAPageMap >> aPlacement
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
        tPageMapEntry thePageMapEntry;
        char paren2;

        ifsRNUCAPageMap >> paren1
                        >> theAddress
                        >> thePageMapEntry.isDataClassValid
                        >> thePageMapEntry.isShared
                        >> thePageMapEntry.privateTileID           // owner of private data
                        >> thePageMapEntry.pageHasInstr            // not a real page map entry. Used only for stats and for I/D decoupling
                        >> thePageMapEntry.HasFetchRef             // for whitebox debugging
                        >> thePageMapEntry.theCachedLineBitmask[1] // not a real page map entry. Used only to optimize the simflex code
                        >> thePageMapEntry.theCachedLineBitmask[0] // not a real page map entry. Used only to optimize the simflex code
                        >> paren2
                        ;
        DBG_Assert ( (paren1 == '{'), ( << "Expected '{' when loading thePageTable from checkpoint" ) );
        DBG_Assert ( (paren2 == '}'), ( << "Expected '}' when loading thePageTable from checkpoint" ) );

        DBG_ ( VVerb,
               Addr(theAddress)
               ( << std::hex
                 << " Address: "              << theAddress
                 << " isDataClassValid: "     << thePageMapEntry.isDataClassValid
                 << " isShared: "             << thePageMapEntry.isShared
                 << " privateTileID: "        << thePageMapEntry.privateTileID
                 << " pageHasInstr: "         << thePageMapEntry.pageHasInstr
                 << " HasFetchRef: "          << thePageMapEntry.HasFetchRef
                 << " theCachedLineBitmask: " << thePageMapEntry.theCachedLineBitmask[1] << "-" << thePageMapEntry.theCachedLineBitmask[0]
               ) );

        tPageMap::iterator iter;
        bool is_new;
        boost::tie(iter, is_new) = thePageMap.insert( std::make_pair( theAddress, thePageMapEntry ) );
        DBG_Assert(is_new);
      }
      ifsRNUCAPageMap.close();
      DBG_( Dev, ( << "R-NUCA PageMap loaded." ));
    }
  }
  /* CMU-ONLY-BLOCK-END */

  void saveChipDirState(std::string const & aDirName) {
    std::string fname( aDirName );
    fname += "/" + statName() + "-ChipDir";
    std::ofstream ofs( fname.c_str() );

    // Write ChipDir info and the number of entries
    ofs << (uint32_t)thePlacement
        << " " << theSharersBitmaskComponents
        << " " << theChipDir.size()
        << std::endl;

    // Write out each ChipDir entry
    for ( tChipDir::iterator iter = theChipDir.begin();
          iter != theChipDir.end();
          iter++
        ) {
      uint64_t theAddress = iter->first;
      tChipDirEntry & theChipDirEntry = iter->second;

      DBG_ ( VVerb,
             Addr(theAddress)
             ( << std::hex
               << " Address: "    << theAddress
               << " theState: "   << theChipDirEntry.theState
               << " theSharers: " << theChipDirEntry.theSharers[1] << "-" << theChipDirEntry.theSharers[0]
             ) );

      ofs << "{"
          << " " << theAddress
          << " " << theChipDirEntry.theState
          ;
      for (uint32_t i = 0; i < theSharersBitmaskComponents; ++i) {
        ofs << " " << theChipDirEntry.theSharers[i];
      }
      ofs << " " << "}"
          << std::endl;
    }
    ofs.close();
  }

  void loadChipDirState( std::string const & aDirName ) {
    std::string fname ( aDirName );
    fname += "/" + statName() + "-ChipDir";
    std::ifstream ifs ( fname.c_str() );

    theChipDir.clear(); // empty the ChipDir

    if (! ifs.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty ChipDir for private-nuca. " )  );
    } else {
      ifs >> std::skipws; // skip white space

      // Read the ChipDir map info
      int32_t chip_dir_size = 0;
      uint32_t aPlacement;
      uint32_t aSharersBitmaskComponents;
      ifs >> aPlacement
          >> aSharersBitmaskComponents
          >> chip_dir_size
          ;

      // make sure the configurations match
      DBG_Assert(   ( thePlacement == (tPlacement) aPlacement )
                    || ( thePlacement == kPrivateCache && aPlacement == kASRCache ) /* CMU-ONLY */
                );
      DBG_Assert(aSharersBitmaskComponents == theSharersBitmaskComponents);
      // if simulating private, the page map should be non-empty
      DBG_Assert ( (thePlacement != kPrivateCache) || (chip_dir_size >= 0) );

      // Read in each entry
      for ( int32_t i = 0; i < chip_dir_size; i++ ) {
        char paren1;
        uint64_t theAddress;
        tChipDirEntry theChipDirEntry;
        char paren2;

        ifs >> paren1;
        DBG_Assert ( (paren1 == '{'), ( << "Expected '{' when loading thePageTable from checkpoint" ) );

        ifs >> theAddress
            >> theChipDirEntry.theState
            ;
        for (uint32_t i = 0; i < theSharersBitmaskComponents; ++i) {
          ifs >> theChipDirEntry.theSharers[i];
        }

        ifs >> paren2;
        DBG_Assert ( (paren2 == '}'), ( << "Expected '}' when loading thePageTable from checkpoint" ) );

        DBG_ ( VVerb,
               Addr(theAddress)
               ( << std::hex
                 << " Address: "    << theAddress
                 << " theState: "   << theChipDirEntry.theState
                 << " theSharers: " << theChipDirEntry.theSharers[1] << "-" << theChipDirEntry.theSharers[0]
               ) );

        tChipDir::iterator iter;
        bool is_new;
        boost::tie(iter, is_new) = theChipDir.insert( std::make_pair( theAddress, theChipDirEntry ) );
        DBG_Assert(is_new);
      }
      ifs.close();
      DBG_( Dev, ( << "ChipDir loaded." ));
    }
  }

};  // end class FastCMPNetworkController

}  // end Namespace nFastCMPNetworkController

FLEXUS_COMPONENT_INSTANTIATOR( FastCMPNetworkController, nFastCMPNetworkController );

FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestInD )     {
  return (cfg.NumCores) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestInI )     {
  return (cfg.NumCores) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, SnoopOutD )      {
  return (cfg.NumCores) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, SnoopOutI )      {
  return (cfg.NumCores) ;
}

FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestOutD )    {
  return (cfg.NumCores * cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestOutI )    {
  return (cfg.NumCores * cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, SnoopInD )       {
  return (cfg.NumCores * cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, SnoopInI )       {
  return (cfg.NumCores * cfg.NumL2Slices) ;
}

FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestFromL2 )  {
  return (cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, ToL2Snoops )     {
  return (cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, RequestOutMem )  {
  return (cfg.NumL2Slices) ;
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPNetworkController, BusSnoopIn )     {
  return (cfg.NumL2Slices) ;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCMPNetworkController

#define DBG_Reset
#include DBG_Control()
