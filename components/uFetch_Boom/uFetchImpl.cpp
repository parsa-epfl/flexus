#include <fstream>
#include <set>

#include <boost/none.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/stats.hpp>

#include <components/uFetch_Boom/uFetch.hpp>
#include <components/uFetch/uFetchTypes.hpp> // IMPORTANT: Use global uFetch header
#include <components/MTManager/MTManager.hpp>

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>

#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories uFetch
#define DBG_SetDefaultOps AddCat(uFetch)
#include DBG_Control()

#include <core/qemu/mai_api.hpp>
#include <components/CommonQEMU/seq_map.hpp>

#include <components/CommonQEMU/Translation.hpp>

#define MAX_RESET_DISTANCE 40

#define LOG2(x)         \
  ((x)==1 ? 0 :         \
  ((x)==2 ? 1 :         \
  ((x)==4 ? 2 :         \
  ((x)==8 ? 3 :         \
  ((x)==16 ? 4 :        \
  ((x)==32 ? 5 :        \
  ((x)==64 ? 6 :        \
  ((x)==128 ? 7 :       \
  ((x)==256 ? 8 :       \
  ((x)==512 ? 9 :       \
  ((x)==1024 ? 10 :     \
  ((x)==2048 ? 11 :     \
  ((x)==4096 ? 12 :     \
  ((x)==8192 ? 13 :     \
  ((x)==16384 ? 14 :    \
  ((x)==32768 ? 15 :    \
  ((x)==65536 ? 16 :    \
  ((x)==131072 ? 17 :   \
  ((x)==262144 ? 18 :   \
  ((x)==524288 ? 19 :   \
  ((x)==1048576 ? 20 :  \
  ((x)==2097152 ? 21 :  \
  ((x)==4194304 ? 22 :  \
  ((x)==8388608 ? 23 :  \
  ((x)==16777216 ? 24 : \
  ((x)==33554432 ? 25 : \
  ((x)==67108864 ? 26 : -0xffff)))))))))))))))))))))))))))

namespace nuFetch {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
static const uint64_t kBadTag = 0xFFFFFFFFFFFFFFFFULL;

typedef flexus_boost_set_assoc<uint64_t, int> SimCacheArray;
typedef SimCacheArray::iterator SimCacheIter;
struct SimCache {
  SimCacheArray theCache;
  int32_t theCacheSize;
  int32_t theCacheAssoc;
  int32_t theCacheBlockShift;
  int32_t theBlockSize;
  std::string theName;

  void init(int32_t aCacheSize, int32_t aCacheAssoc, int32_t aBlockSize, const std::string & aName) {
    theCacheSize = aCacheSize;
    theCacheAssoc = aCacheAssoc;
    theBlockSize = aBlockSize;
    theCacheBlockShift = LOG2(theBlockSize);
    theCache.init( theCacheSize / theBlockSize, theCacheAssoc, 0 );
    theName = aName;
  }

  void loadState(std::string const & aDirName) {
    std::string fname(aDirName);
    DBG_(Dev, ( << "Loading state: " << fname << " for ufetch order L1i cache" ) );
    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( ! loadArray( ifs ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                      ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
      ifs.close();
    }
  }
  bool loadArray( std::istream & s ) {
    static const int32_t kSave_ValidBit = 1;
    int32_t tagShift = LOG2(theCache.sets());

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    for ( uint32_t i = 0; i < theCache.sets() ; i++ ) {
      s >> paren; // {
      if ( paren != '{' ) {
        DBG_ (Crit, ( << "Expected '{' when loading checkpoint" ) );
        return false;
      }
      for ( uint32_t j = 0; j < theCache.assoc(); j++ ) {
        s >> paren >> load_state >> load_tag >> paren;
        DBG_(Trace,  ( << theName << " Loading block " << std::hex << (((load_tag << tagShift) | i) << theCacheBlockShift) << " with state " << ((load_state & kSave_ValidBit) ? "Shared" : "Invalid" ) << " in way " << j ));
        if (load_state & kSave_ValidBit) {
          theCache.insert( std::make_pair(((load_tag << tagShift) | i), 0) );
          DBG_Assert(theCache.size() <= theCache.assoc());
        }
      }
      s >> paren; // }
      if ( paren != '}' ) {
        DBG_ (Crit, ( << "Expected '}' when loading checkpoint" ) );
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if ( paren != '<' ) {
        DBG_ (Crit, ( << "Expected '<' when loading checkpoint" ) );
        return false;
      }
      for ( uint32_t j = 0; j < theCache.assoc(); j++ ) {
        s >> dummy;
      }
      s >> paren; // >
      if ( paren != '>' ) {
        DBG_ (Crit, ( << "Expected '>' when loading checkpoint" ) );
        return false;
      }
    }
    return true;
  }

  uint64_t insert(uint64_t addr) {
    uint64_t ret_val = 0;
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return ret_val;  //already present
    }
    if ((int)theCache.size() >= theCacheAssoc) {
      ret_val = theCache.front_key() << theCacheBlockShift;
      theCache.pop_front();
    }
    theCache.insert( std::make_pair(addr, 0) );
//    if (ret_val) {
//    	DBG_ (Tmp, ( << "Evicting " << (ret_val) ));
//    } else {
//    	DBG_ (Tmp, ( << "Evicting nothing " ));
//    }
    return ret_val;
  }

  bool lookup(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return true;  // present
    }
    return false; //not present
  }

  bool inval(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.erase(iter);
      return true;  // invalidated
    }
    return false;  // not present
  }

};

class FLEXUS_COMPONENT(uFetch) {
  FLEXUS_COMPONENT_IMPL(uFetch);

  std::vector< std::list< FetchAddr > > theFAQ;
  std::vector< std::list< FetchAddr > > thePAQ; //Rakesh: Prefetch address queue
  std::vector< std::list< VirtualMemoryAddress > > thePBQ; //Rakesh: Prefetch block queue
  std::vector< std::set< PhysicalMemoryAddress > > theOutstandingMissQ; //Rakesh: queue for outstanding misses
  std::vector< std::list< VirtualMemoryAddress > > theOutstandingMissVQ; //Rakesh
  std::vector< std::vector< FetchAddr > > theLastFetches; //Rakesh
  std::vector< std::list< VirtualMemoryAddress > > theRecordedMissQ; //Rakesh

  //This opcode is used to signal an ITLB miss to the core, to force
  //a resync with Simics
  static const int32_t kITLBMiss = 0UL; //illtrap

  //Statistics on the ICache
  Stat::StatCounter theFetchAccesses;
  Stat::StatCounter theFetches;
  Stat::StatCounter thePrefetches;
  Stat::StatCounter theFailedTranslations;
  Stat::StatCounter theMisses;
  Stat::StatCounter theHits;
  Stat::StatCounter theTransientHits;
  Stat::StatCounter theMissCycles;
  Stat::StatCounter theAllocations;
  Stat::StatMax  theMaxOutstandingEvicts;

  Stat::StatCounter theSeqMiss;
  Stat::StatCounter theBranch;
  Stat::StatCounter theResyncMispred;
  Stat::StatCounter theResyncException;
  Stat::StatCounter theResyncInterrupt;
  Stat::StatCounter theResyncSpecFailed;
  Stat::StatCounter theResynchronization;
  Stat::StatCounter theResyncUnknown;
  Stat::StatCounter theOthers;

  Stat::StatCounter theSeqMissP;
  Stat::StatCounter theBranchP;
  Stat::StatCounter theResyncMispredP;
  Stat::StatCounter theResyncExceptionP;
  Stat::StatCounter theResyncInterruptP;
  Stat::StatCounter theResyncSpecFailedP;
  Stat::StatCounter theResynchronizationP;
  Stat::StatCounter theResyncUnknownP;
  Stat::StatCounter theOthersP;

  Stat::StatCounter * lastReset[MAX_RESET_DISTANCE];
  Stat::StatCounter * lastResetNP[MAX_RESET_DISTANCE];
  Stat::StatCounter * lastResetSeq[MAX_RESET_DISTANCE];
  Stat::StatCounter * lastResetSeqNP[MAX_RESET_DISTANCE];
  Stat::StatCounter * lastResetBranch[MAX_RESET_DISTANCE];
  Stat::StatCounter * lastResetBranchNP[MAX_RESET_DISTANCE];

  Stat::StatCounter * bimodalConfindence[4];
  Stat::StatCounter * tageConfindence[8];

  Stat::StatCounter * squashReason[6];
  Stat::StatCounter * squashReasonBranch[10];

  Stat::StatCounter returnUsedRAS;
  Stat::StatCounter returnUsedBTB;
  Stat::StatCounter returnUsedBTBZero;

  uint64_t lastResetCycle;
  eSquashCause lastSquashCause;
  boost::intrusive_ptr<BPredState> lastSquashedBPState; //Rakesh
  bool demandFetchIssued;
  bool waitingForRetryDone;

  //The I-cache
  SimCache theI;

  // Magic for checking TLB walk.
  std::vector<TranslationPtr> TranslationsFromTLB;
  pFetchBundle theBundle;
  int32_t theBundleCoreID;

  //Indicates whether a miss is outstanding, and what the physical address
  //of the miss is
  std::vector< boost::optional< PhysicalMemoryAddress > > theIcacheMiss;
  std::vector< boost::optional< VirtualMemoryAddress > > theIcacheVMiss;
  std::vector< boost::intrusive_ptr<TransactionTracker> > theFetchReplyTransactionTracker;
  std::vector< boost::optional< std::pair<PhysicalMemoryAddress, tFillLevel> > > theLastMiss;

  //Indicates whether a prefetch is outstanding, and what paddr was prefetched
  std::vector< boost::optional< PhysicalMemoryAddress > > theIcachePrefetch;
  //Indicates whether a prefetch is outstanding, and what paddr was prefetched
  std::vector< boost::optional< uint64_t > > theLastPrefetchVTagSet;

  // Set of outstanding evicts.
  std::set< uint64_t > theEvictSet;

  //Cache the last translation to avoid calling Simics
  uint64_t theLastVTagSet;
  PhysicalMemoryAddress theLastPhysical;

  uint32_t theIndexShift;
  uint64_t theBlockMask;
  uint32_t theMissQueueSize;
  uint32_t theMaxOutstandingFDIPMisses;
  uint8_t theMaxOutstandingRecMisses;

  std::list< MemoryTransport > theMissQueue;
  std::list< MemoryTransport > theSnoopQueue;
  std::list< MemoryTransport > theReplyQueue;
  std::list< MemoryTransport > theFDIPPrefetchQueue;
  std::list< MemoryTransport > theRecMissPrefetchQueue;
  std::list< MemoryTransport > theRecordedMissPrefetchQueue;

  std::vector<CPUState> theCPUState;

private:
  //I-Cache manipulation functions
  //=================================================================
  /*
      uint64_t index( PhysicalMemoryAddress const & anAddress ) {
        return ( static_cast<uint64_t>(anAddress) >> theIndexShift ) & theIndexMask;
      }

      uint64_t tag( PhysicalMemoryAddress const & anAddress ) {
        return ( static_cast<uint64_t>(anAddress) >> theTagShift );
      }
  */
  bool lookup( PhysicalMemoryAddress const & anAddress ) {
    if (theI.lookup(anAddress)) {
      DBG_( Verb, Comp(*this) ( << "Core[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] I-Lookup hit: " << anAddress ));
      return true;
    }
    DBG_( Verb, Comp(*this) ( << "Core[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] I-Lookup Miss addr: " << anAddress ));
    return false;
  }

  PhysicalMemoryAddress insert( PhysicalMemoryAddress const & anAddress ) {
    PhysicalMemoryAddress ret_val(0);
    if (! lookup(anAddress) ) {
      ++theAllocations;
      ret_val = PhysicalMemoryAddress( theI.insert(anAddress) );
    }
    return ret_val;
  }
  bool inval( PhysicalMemoryAddress const & anAddress ) {
    return theI.inval(anAddress);
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(uFetch)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theFetchAccesses( statName() + "-FetchAccess" )
    , theFetches( statName() + "-Fetches" )
    , thePrefetches( statName() + "-Prefetches" )
    , theFailedTranslations( statName() + "-FailedTranslations" )
    , theMisses( statName() + "-Misses" )
    , theHits( statName() + "-Hits" )
    , theTransientHits( statName() + "-TransientHits" )
    , theMissCycles( statName() + "-MissCycles" )
    , theAllocations( statName() + "-Allocations" )
    , theMaxOutstandingEvicts( statName() + "-MaxEvicts" )
    , theSeqMiss( statName() + "-Miss:Seq" )
	, theBranch( statName() + "-Miss:Branch" )
	, theResyncMispred( statName() + "-Miss:Resync:BranchMispredict" )
	, theResyncException( statName() + "-Miss:Resync:Exception" )
	, theResyncInterrupt( statName() + "-Miss:Resync:Interrupt" )
	, theResyncSpecFailed( statName() + "-Miss:Resync:SpecFailed" )
	, theResynchronization( statName() + "-Miss:Resync:Sync" )
	, theResyncUnknown( statName() + "-Miss:Resync:Unknown" )
	, theOthers( statName() + "-Miss:Others" )
	, theSeqMissP( statName() + "-Miss:Seq:NoPrefetch" )
	, theBranchP( statName() + "-Miss:Branch:NoPrefetch" )
	, theResyncMispredP( statName() + "-Miss:Resync:BranchMispredict:NoPrefetch" )
	, theResyncExceptionP( statName() + "-Miss:Resync:Exception:NoPrefetch" )
	, theResyncInterruptP( statName() + "-Miss:Resync:Interrupt:NoPrefetch" )
	, theResyncSpecFailedP( statName() + "-Miss:Resync:SpecFailed:NoPrefetch" )
	, theResynchronizationP( statName() + "-Miss:Resync:Sync:NoPrefetch" )
	, theResyncUnknownP( statName() + "-Miss:Resync:Unknown:NoPrefetch" )
	, theOthersP( statName() + "-Miss:Others:NoPrefetch" )
    , returnUsedRAS(statName() + "-squashReasonBranch:kReturn:RAS")
    , returnUsedBTB(statName() + "-squashReasonBranch:kReturn:BTB")
    , returnUsedBTBZero(statName() + "-squashReasonBranch:kReturn:Zero")
    , theLastVTagSet(0)
    , theLastPhysical(0)
  {
	    char stat_name[50];
		for (int i = 0; i <= MAX_RESET_DISTANCE; i++) {
			sprintf(stat_name, "-lastResetDistance%d", i);
			lastReset[i] = new Stat::StatCounter(statName() + stat_name);
			sprintf(stat_name, "-lastResetDistanceNP%d", i);
			lastResetNP[i] = new Stat::StatCounter(statName() + stat_name);
			sprintf(stat_name, "-lastResetDistance:Seq%d", i);
			lastResetSeq[i] = new Stat::StatCounter(statName() + stat_name);
			sprintf(stat_name, "-lastResetDistance:SeqNP%d", i);
			lastResetSeqNP[i] = new Stat::StatCounter(statName() + stat_name);
			sprintf(stat_name, "-lastResetDistance:Branch%d", i);
			lastResetBranch[i] = new Stat::StatCounter(statName() + stat_name);
			sprintf(stat_name, "-lastResetDistance:BranchNP%d", i);
			lastResetBranchNP[i] = new Stat::StatCounter(statName() + stat_name);
		}

		for (int i = 0; i <= 4; i++) {
			sprintf(stat_name, "-bimodConfidence%d", i);
			bimodalConfindence[i] = new Stat::StatCounter(statName() + stat_name);
		}
		for (int i = 0; i <= 8; i++) {
			sprintf(stat_name, "-tageConfidence%d", i);
			tageConfindence[i] = new Stat::StatCounter(statName() + stat_name);
		}


		sprintf(stat_name, "-squashReason:Unknown");
		squashReason[0] = new Stat::StatCounter(statName() + stat_name);
		sprintf(stat_name, "-squashReason:kResynchronize");
		squashReason[1] = new Stat::StatCounter(statName() + stat_name);
		sprintf(stat_name, "-squashReason:kBranchMispredict");
		squashReason[2] = new Stat::StatCounter(statName() + stat_name);
		sprintf(stat_name, "-squashReason:kException");
		squashReason[3] = new Stat::StatCounter(statName() + stat_name);
		sprintf(stat_name, "-squashReason:kInterrupt");
		squashReason[4] = new Stat::StatCounter(statName() + stat_name);
		sprintf(stat_name, "-squashReason:kFailedSpec");
		squashReason[5] = new Stat::StatCounter(statName() + stat_name);


		sprintf(stat_name, "-squashReasonBranch:kNonBranch");
		squashReasonBranch[0] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kConditional");
	    squashReasonBranch[1] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kUnconditional");
	    squashReasonBranch[2] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kCall");
	    squashReasonBranch[3] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kJmpl");
	    squashReasonBranch[4] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kReturn");
	    squashReasonBranch[5] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kJmplCall");
	    squashReasonBranch[6] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kRetry");
	    squashReasonBranch[7] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kDone");
	    squashReasonBranch[8] = new Stat::StatCounter(statName() + stat_name);
	    sprintf(stat_name, "-squashReasonBranch:kLastBranchType");
	    squashReasonBranch[9] = new Stat::StatCounter(statName() + stat_name);

	    waitingForRetryDone = false;
  }

  void initialize() {

    theI.init( cfg.Size, cfg.Associativity, cfg.ICacheLineSize, statName() );
    theIndexShift = LOG2( cfg.ICacheLineSize );
    theBlockMask = ~ (cfg.ICacheLineSize - 1);
    theBundleCoreID = flexusIndex();
    theBundle = new FetchBundle();
    theBundle->coreID = theBundleCoreID;

    theFAQ.resize(cfg.Threads);
    thePAQ.resize(cfg.Threads);		//Rakesh
    thePBQ.resize(cfg.Threads);		//Rakesh
    theRecordedMissQ.resize(cfg.Threads);
    theOutstandingMissQ.resize(cfg.Threads);		//Rakesh
    theOutstandingMissVQ.resize(cfg.Threads);		//Rakesh
    theLastFetches.resize(cfg.Threads);		//Rakesh
    theIcacheMiss.resize(cfg.Threads);
    theIcacheVMiss.resize(cfg.Threads);
    theFetchReplyTransactionTracker.resize(cfg.Threads);
    theLastMiss.resize(cfg.Threads);
    theIcachePrefetch.resize(cfg.Threads);
    theLastPrefetchVTagSet.resize(cfg.Threads);
    theCPUState.resize(cfg.Threads);

    theMissQueueSize = cfg.MissQueueSize;
    theMaxOutstandingFDIPMisses = 4; //Fixme: make it a parameter.
    theMaxOutstandingRecMisses = 2;

    uint64_t temp = 0;

    for (uint32_t i = 0; i < cfg.Threads; i++) {
//    	theLastFetches[i][0].theAddress = VirtualMemoryAddress(0);
//    	theLastFetches[i][1].theAddress = VirtualMemoryAddress(0);
    	theLastFetches[i].push_back(FetchAddr(VirtualMemoryAddress(0), temp));
    	theLastFetches[i].push_back(FetchAddr(VirtualMemoryAddress(0), temp));

    	for (uint32_t j = 0; j < (theMaxOutstandingFDIPMisses + theMissQueueSize) * 2 /*random stuff*/; j++) {
    		theOutstandingMissVQ[i].push_back(VirtualMemoryAddress(0));
    	}
    }

    lastResetCycle = 0;
    lastSquashCause = kResynchronize; //Some initialization
    lastSquashedBPState = 0;
    demandFetchIssued = false;
  }

  void finalize() {}

  bool isQuiesced() const {
    if (! theMissQueue.empty() || ! theSnoopQueue.empty() || !theReplyQueue.empty()) {
      return false;
    }
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (! theFAQ[i].empty() ) {
        return false;
      }
    }
    return true;
  }

  void saveState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + boost::padded_string_cast < 3, '0' > (flexusIndex()) + "-L1i";
    //Not supported
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    if (flexusWidth() == 1) {
      fname += "/sys-L1i";
    } else {
      fname += "/" + boost::padded_string_cast < 2, '0' > (flexusIndex()) + "-L1i";
    }
    theI.loadState(fname);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RASOpsIn);
  void push(interface::RASOpsIn const &, index_t anIndex, std::list< boost::intrusive_ptr<BPredState> > & theRASops) {
//	  DBG_( Tmp, ( << " reconstruct RAS uFetch" ));

	  for ( std::list< FetchAddr >::iterator it = theFAQ[anIndex].begin(); it != theFAQ[anIndex].end(); it++) {
		  boost::intrusive_ptr<BPredState> bpState = it->theBPState;
	      if (bpState->thePredictedType == kCall || bpState->thePredictedType == kJmplCall || bpState->thePredictedType == kReturn) {
//	    	  DBG_( Tmp, ( << " RAS Decode " << bpState->pc << " pred type " << bpState->thePredictedType << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(bpState->pc)));
	    	  theRASops.push_front(bpState);
	      }
	  }

	  FLEXUS_CHANNEL( RASOpsOut ) << theRASops;
  }

  //FetchAddressIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchAddressIn);
  void push( interface::FetchAddressIn const &, index_t anIndex, boost::intrusive_ptr<FetchCommand> & aCommand) {
    std::copy
    ( aCommand->theFetches.begin()
      , aCommand->theFetches.end()
      , std::back_inserter( theFAQ[anIndex] )
    );
  }

  //PrefetchAddressIn Rakesh
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(PrefetchAddressIn);
  void push( interface::PrefetchAddressIn const &, index_t anIndex, boost::intrusive_ptr<FetchCommand> & aCommand) {
    std::copy
    ( aCommand->theFetches.begin()
      , aCommand->theFetches.end()
      , std::back_inserter( thePAQ[anIndex] )
    );
    assert(thePAQ[anIndex].size() <= cfg.FAQSize);
  }

  //RecordedMissIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RecordedMissIn);
  void push( interface::RecordedMissIn const &, index_t anIndex, boost::intrusive_ptr<RecordedMisses> & rMisses) {
	  while ( rMisses->theMisses.size() > 0 ) {
		  VirtualMemoryAddress preFetch_addr = rMisses->theMisses.front();
		  std::list<VirtualMemoryAddress>::iterator findIter = std::find(theRecordedMissQ[anIndex].begin(), theRecordedMissQ[anIndex].end(), preFetch_addr);
		  if (findIter == theRecordedMissQ[anIndex].end()) {
			  theRecordedMissQ[anIndex].push_back(preFetch_addr);
		  }
		  rMisses->theMisses.pop_front();
//		  DBG_(Tmp, ( << "Received recoreded miss " << preFetch_addr));
	  }
  }

  //AvailableFAQOut
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(AvailableFAQOut);
  int32_t pull( interface::AvailableFAQOut const &, index_t anIndex) {
    return cfg.FAQSize - theFAQ[anIndex].size() ;
  }

  //SquashIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
  void push( interface::SquashIn const &, index_t anIndex, eSquashCause  & aReason) {
    DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] Fetch SQUASH: " << aReason));
//    DBG_( Tmp, Comp(*this) ( << std::endl << std::endl <<  "Squashing CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] Fetch SQUASH: " << aReason << std::endl << std::endl));
    theFAQ[anIndex].clear();
    thePAQ[anIndex].clear();	//Rakesh
    thePBQ[anIndex].clear();	//Rakesh
    theRecordedMissQ[anIndex].clear(); //Rakesh
    theIcacheMiss[anIndex] = boost::none;
    theIcacheVMiss[anIndex] = boost::none;
    theFetchReplyTransactionTracker[anIndex] = NULL;
    theLastMiss[anIndex] = boost::none;
    theIcachePrefetch[anIndex] = boost::none;
    theLastPrefetchVTagSet[anIndex] = 0;

    lastResetCycle = theFlexus->cycleCount();
    lastSquashCause = aReason;
    FLEXUS_CHANNEL(SquashOut) << aReason;
  }

  //BranchSquashIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashBranchIn);
  void push( interface::SquashBranchIn const &, index_t anIndex, boost::intrusive_ptr<BPredState> & aBPState) {
//	  DBG_( Tmp, Comp(*this) ( << std::endl << std::endl <<  "Squashing Mispredicted branch CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] Fetch SQUASH: " << std::endl << std::endl));
	  lastSquashedBPState = aBPState;
      FLEXUS_CHANNEL(SquashBranchOut) << aBPState;
  }

  //ChangeCPUState
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ChangeCPUState);
  void push( interface::ChangeCPUState const &, index_t anIndex, CPUState & aState) {
    DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] Change CPU State. PSTATE: " << std::hex << aState.thePSTATE << std::dec ));
    theCPUState[anIndex] = aState;
  }

  //FetchMissIn
  FLEXUS_PORT_ALWAYS_AVAILABLE(FetchMissIn);
  void push( interface::FetchMissIn const &, MemoryTransport & aTransport ) {
    DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] Fetch Miss Reply Received on Port FMI: " << *aTransport[MemoryMessageTag]));
    fetchReply( aTransport );
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(ICount);
  int32_t pull(ICount const &, index_t anIndex) {
    return theFAQ[anIndex].size();
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) {
    int32_t available_fiq = 0;
    bool isFIQEmpty = false;
    std::pair<int, bool> decode_state;
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFIQ, anIndex ).available() ) ;
    FLEXUS_CHANNEL_ARRAY( AvailableFIQ, anIndex ) >> decode_state;
    boost::tie(available_fiq, isFIQEmpty) = decode_state;
    return theFAQ[anIndex].empty() || available_fiq == 0 || theIcacheMiss[anIndex];
  }

  //   Msutherl: TLB in-out functions
  FLEXUS_PORT_ALWAYS_AVAILABLE(iTranslationIn);
  void push(interface::iTranslationIn const &, TranslationPtr &retdTranslations) {
    for (std::vector<TranslationPtr>::iterator it = TranslationsFromTLB.begin();
         it != TranslationsFromTLB.end(); ++it) {
      if ((*it)->theID == retdTranslations->theID) {
        TranslationsFromTLB.erase(it);
        TranslationsFromTLB.push_back(retdTranslations);
        getFetchResponse();
        break;
      }
    }
    DBG_(Iface, (<< "Set TranslationsFromTLB component...."));
  }

  bool available(interface::ResyncIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
    TranslationsFromTLB.clear();
    theBundle->clear();
    theBundleCoreID = aResync;
    theBundle->coreID = theBundleCoreID;
  }

  void getFetchResponse() {

    DBG_Assert(TranslationsFromTLB.back()->isDone() || TranslationsFromTLB.back()->isHit());

    DBG_(VVerb, (<< "Starting magic translation after sending to TLB...."));
    TranslationPtr tr = TranslationsFromTLB.back();
    TranslationsFromTLB.pop_back();
    DBG_(VVerb, (<< "poping entry out of fetch translation requests " << tr->theVaddr));
    PhysicalMemoryAddress magicTranslation =
        cpu(tr->theIndex)->translateVirtualAddress(tr->theVaddr);

    if (tr->thePaddr == magicTranslation || tr->isPagefault()) {
      DBG_(VVerb,
           (<< "Magic QEMU translation == MMU Translation. Vaddr = " << std::hex << tr->theVaddr
            << std::dec << ", Paddr = " << std::hex << tr->thePaddr << std::dec));
    } else {
      DBG_Assert(false, (<< "ERROR: Magic QEMU translation NOT EQUAL TO MMU "
                            "Translation. Vaddr = "
                         << std::hex << tr->theVaddr << std::dec << ", PADDR_MMU = " << std::hex
                         << tr->thePaddr << std::dec << ", PADDR_QEMU = " << std::hex
                         << magicTranslation << std::dec));
    }
    uint32_t opcode = 1;
    if (!tr->isPagefault()) {
      opcode = cpu(tr->theIndex)->fetchInstruction(tr->theVaddr);
      opcode += opcode ? 0 : 1;
      theBundle->updateOpcode(tr->theVaddr, opcode);
    } else {
      theBundle->updateOpcode(tr->theVaddr, opcode);
    }
  }

public:
  void drive( interface::uFetchDrive const & ) {
    bool garbage = true;
    FLEXUS_CHANNEL( ClockTickSeen ) << garbage;

    int32_t td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFThread(flexusIndex());
    }
    doFetch(td);
    doPrefetch(td);	//Rakesh
    sendMisses();
  }

  Qemu::Processor cpu(index_t anIndex) {
    return Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + anIndex);
  }

private:
  void addOustandingMiss(index_t anIndex, VirtualMemoryAddress addr) {
	  theOutstandingMissVQ[anIndex].pop_front();
	  theOutstandingMissVQ[anIndex].push_back(addr);
  }

  void prefetchNext(index_t anIndex) {
    // Limit the number of prefetches.  With some backpressure, the number of
    // outstanding prefetches can otherwise be unbounded.
    if ( theMissQueue.size() >= theMissQueueSize )
      return;

    if (cfg.PrefetchEnabled && theLastPrefetchVTagSet[anIndex]) {
      //Prefetch the line following theLastPrefetchVTagSet
      //(if it has a valid translation)

      ++ (*theLastPrefetchVTagSet[anIndex]);
      VirtualMemoryAddress vprefetch = VirtualMemoryAddress( *theLastPrefetchVTagSet[anIndex] << theIndexShift );
      Flexus::SharedTypes::Translation xlat;
      xlat.theVaddr = vprefetch;
      xlat.thePSTATE = theCPUState[anIndex].thePSTATE;
      xlat.theType = Flexus::SharedTypes::Translation::eFetch;
      xlat.thePaddr = cpu(anIndex)->translateVirtualAddress(xlat.theVaddr);
      if (! xlat.thePaddr ) {
        //Unable to translate for prefetch
//    	  DBG_(Tmp, ( << "NLP translation failed "));
        theLastPrefetchVTagSet[anIndex] = boost::none;
        return;
      } else {
    	if (theOutstandingMissQ[anIndex].find(xlat.thePaddr) != theOutstandingMissQ[anIndex].end()) {
//    		DBG_(Tmp, ( << "NLP: FDIP prefetch on way " << std::hex << vprefetch));
    	        //We have already sent a prefetch request out for this. No need
    	        //to request again.
    	} else if (lookup( xlat.thePaddr )) {
          //No need to prefetch, already in cache
          theLastPrefetchVTagSet[anIndex] = boost::none;
        } else {
          theIcachePrefetch[anIndex] = xlat.thePaddr;
          DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] L1I PREFETCH " << *theIcachePrefetch[anIndex] ));
          theOutstandingMissQ[anIndex].insert(xlat.thePaddr);
          addOustandingMiss(anIndex, vprefetch);
//          DBG_(Tmp, ( << "NLP: issue prefetch " << std::hex << vprefetch << " paddr " << xlat.thePaddr << " outreq " << theOutstandingMissQ[anIndex].size()));
          issueFetch( xlat.thePaddr , vprefetch);
          ++thePrefetches;
        }
      }
    }
  }

  bool icacheLookup( index_t anIndex, VirtualMemoryAddress vaddr ) {
    //Translate virtual address to physical.
    //First, see if it is our cached translation
    PhysicalMemoryAddress paddr;
    uint64_t tagset = vaddr >> theIndexShift;
    if ( tagset == theLastVTagSet ) {
      paddr = theLastPhysical;
      if (paddr == 0) {
        ++theFailedTranslations;
        return true; //Failed translations are treated as hits - they will cause an ITLB miss in the pipe.
      }
    } else {
      Flexus::SharedTypes::Translation xlat;
      xlat.theVaddr = vaddr;
      xlat.thePSTATE = theCPUState[anIndex].thePSTATE;
      xlat.theType = Flexus::SharedTypes::Translation::eFetch;
      xlat.thePaddr = cpu(anIndex)->translateVirtualAddress(xlat.theVaddr);
      paddr = xlat.thePaddr;
      if (paddr == 0) {
        ++theFailedTranslations;
        return true; //Failed translations are treated as hits - they will cause an ITLB miss in the pipe.
      }
      //Cache translation
      theLastPhysical = paddr;
      theLastVTagSet = tagset;
    }

    bool hit = lookup(paddr);
    ++theFetchAccesses;
    if (hit) {
      ++theHits;
      if (theLastPrefetchVTagSet[anIndex] && ( *theLastPrefetchVTagSet[anIndex] == tagset ) ) {
        prefetchNext(anIndex);
      }
      return true;
    }
    ++theMisses;

    //It is an error to get a second miss while one is outstanding
    DBG_Assert( ! theIcacheMiss[anIndex] );
    //Record the miss address, so we know when it comes back
    PhysicalMemoryAddress temp(paddr & theBlockMask);
    theIcacheMiss[anIndex] = temp;
    theIcacheVMiss[anIndex] = vaddr;
    theFetchReplyTransactionTracker[anIndex] = NULL;

    DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << anIndex << "] L1I MISS " << vaddr << " " << *theIcacheMiss[anIndex]));

    if ( theIcachePrefetch[anIndex] && *theIcacheMiss[anIndex] == *theIcachePrefetch[anIndex] ) {
      theIcachePrefetch[anIndex] = boost::none;
      //We have already sent a prefetch request out for this miss. No need
      //to request again.  However, we can advance the prefetcher to the
      //next miss
      theTransientHits++;
//      DBG_(Tmp, ( << "Miss: NLP prefetch on way " << std::hex << (tagset << theIndexShift)));
      prefetchNext(anIndex);
    } else if (theOutstandingMissQ[anIndex].find(*theIcacheMiss[anIndex]) != theOutstandingMissQ[anIndex].end()) {
        //We have already sent a prefetch request out for this miss. No need
        //to request again.
    	theTransientHits++;
//    	DBG_(Tmp, ( << "Miss: FDIP prefetch on way " << std::hex << (tagset << theIndexShift)));
    } else {
      theIcachePrefetch[anIndex] = boost::none;
      // Need to issue the miss.
      theOutstandingMissQ[anIndex].insert(*theIcacheMiss[anIndex]);
      addOustandingMiss(anIndex, *theIcacheVMiss[anIndex]);
//      DBG_(Tmp, ( << "Miss: issue fetch " << std::hex << *theIcacheMiss[anIndex] << " vaddr " << *theIcacheVMiss[anIndex] << " outreq " << theOutstandingMissQ[anIndex].size()));
      issueFetch(*theIcacheMiss[anIndex], *theIcacheVMiss[anIndex]);
      demandFetchIssued = true;

      //Also issue a new prefetch
      theLastPrefetchVTagSet[anIndex] = tagset;
      prefetchNext(anIndex);
    }
    return false;
  }

  void sendMisses() {
    while ( ! theMissQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available() ) {
      MemoryTransport trans = theMissQueue.front();

      if (!trans[MemoryMessageTag]->isEvictType()) {
        PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
        if (theEvictSet.find(temp) != theEvictSet.end()) {
          DBG_(Trace, Comp(*this) ( << "Trying to fetch block while evict in process, stalling miss: " << *trans[MemoryMessageTag] ));
          break;
        }
      }

      DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag] ));;

//      DBG_(Tmp, ( << "Sending L1i miss " << *trans[MemoryMessageTag]));
      trans[MemoryMessageTag]->coreIdx()=flexusIndex();
      FLEXUS_CHANNEL(FetchMissOut) << trans;
      theMissQueue.pop_front();
    }

    if ( ! theRecMissPrefetchQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available() ) {
      MemoryTransport trans = theRecMissPrefetchQueue.front();
      bool evictInProcess = false;

      if (!trans[MemoryMessageTag]->isEvictType()) {
        PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
        if (theEvictSet.find(temp) != theEvictSet.end()) {
          DBG_(Trace, Comp(*this) ( << "Trying to fetch block while evict in process, stalling miss: " << *trans[MemoryMessageTag] ));
          evictInProcess = true;
        }
      }

      if (!evictInProcess) {
          DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag] ));;

    //      DBG_(Tmp, ( << "Sending L1i miss " << *trans[MemoryMessageTag]));
          trans[MemoryMessageTag]->coreIdx()=flexusIndex();
          FLEXUS_CHANNEL(FetchMissOut) << trans;
          theRecMissPrefetchQueue.pop_front();
      }
    }

    if ( ! theFDIPPrefetchQueue.empty() && FLEXUS_CHANNEL(FetchMissOut).available() ) {
      MemoryTransport trans = theFDIPPrefetchQueue.front();
      bool evictInProcess = false;

      if (!trans[MemoryMessageTag]->isEvictType()) {
        PhysicalMemoryAddress temp(trans[MemoryMessageTag]->address() & theBlockMask);
        if (theEvictSet.find(temp) != theEvictSet.end()) {
          DBG_(Trace, Comp(*this) ( << "Trying to fetch block while evict in process, stalling miss: " << *trans[MemoryMessageTag] ));
          evictInProcess = true;
        }
      }

      if (!evictInProcess) {
          DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] L1I Sending Miss on Port FMO: " << *trans[MemoryMessageTag] ));;

    //      DBG_(Tmp, ( << "Sending L1i miss " << *trans[MemoryMessageTag]));
          trans[MemoryMessageTag]->coreIdx()=flexusIndex();
          FLEXUS_CHANNEL(FetchMissOut) << trans;
          theFDIPPrefetchQueue.pop_front();
      }
    }


    while ( ! theSnoopQueue.empty() && FLEXUS_CHANNEL(FetchSnoopOut).available() ) {
      MemoryTransport trans = theSnoopQueue.front();
      DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] L1I Sending Snoop on Port FSO: " << *trans[MemoryMessageTag] ));;
      trans[MemoryMessageTag]->coreIdx()=flexusIndex();
      FLEXUS_CHANNEL(FetchSnoopOut) << trans;
      theSnoopQueue.pop_front();
    }

    if (cfg.UseReplyChannel) {
      while ( ! theReplyQueue.empty() && FLEXUS_CHANNEL(FetchReplyOut).available() ) {
        MemoryTransport trans = theReplyQueue.front();
        DBG_( Trace, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "] L1I Sending Reply on Port FRO: " << *trans[MemoryMessageTag] ));;
        trans[MemoryMessageTag]->coreIdx()=flexusIndex();
        FLEXUS_CHANNEL(FetchReplyOut) << trans;
        theReplyQueue.pop_front();
      }
    }
  }

  void queueSnoopMessage ( MemoryTransport          &         aTransport,
                           MemoryMessage::MemoryMessageType   aType,
                           PhysicalMemoryAddress       &      anAddress,
                           bool                               aContainsData = false ) {
    boost::intrusive_ptr<MemoryMessage> msg = new MemoryMessage ( aType, anAddress );
    msg->reqSize() = cfg.ICacheLineSize;
    if (aType == MemoryMessage::FetchAck) {
      msg->ackRequiresData() = aContainsData;
    }
    aTransport.set ( MemoryMessageTag, msg );
    if (cfg.UseReplyChannel) {
      theReplyQueue.push_back ( aTransport );
    } else {
      theSnoopQueue.push_back ( aTransport );
    }
  }

  void fetchReply( MemoryTransport & aTransport) {
    boost::intrusive_ptr<MemoryMessage> reply = aTransport[MemoryMessageTag];
    boost::intrusive_ptr<TransactionTracker> tracker = aTransport[ TransactionTrackerTag ];

    //The fetch unit better only get load replies
    //DBG_Assert (reply->type() == MemoryMessage::FetchReply);

    switch ( reply->type() ) {
      case MemoryMessage::FwdReply:   // JZ: For new protoocl
      case MemoryMessage::FwdReplyOwned: // JZ: For new protoocl
      case MemoryMessage::MissReply:
      case MemoryMessage::FetchReply:
      case MemoryMessage::MissReplyWritable: {
        //Insert the address into the array
        PhysicalMemoryAddress replacement = insert( reply->address());

//        DBG_(Tmp, ( << "Miss reply: for " << reply->address() << " replacing " << replacement));
        issueEvict( replacement );

        //See if it is our outstanding miss or our outstanding prefetch
        for (uint32_t i = 0; i < cfg.Threads; ++i) {
          if (theIcacheMiss[i] && *theIcacheMiss[i] == reply->address()) {
            DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << i << "] L1I FILL " << reply->address()));
            theIcacheMiss[i] = boost::none;
            theIcacheVMiss[i] = boost::none;
            theFetchReplyTransactionTracker[i] = tracker;
            if (aTransport[TransactionTrackerTag] && aTransport[TransactionTrackerTag]->fillLevel()) {
              theLastMiss[i] = std::make_pair(PhysicalMemoryAddress(reply->address() & theBlockMask), *aTransport[TransactionTrackerTag]->fillLevel());
//              DBG_(Tmp, ( << "Fetchreply: for " << reply->address()));
            } else {
              DBG_(Dev, ( << "Received Fetch Reply with no TransactionTrackerTag" ));
            }
          }

          if (theIcachePrefetch[i] && *theIcachePrefetch[i] == reply->address()) {
            DBG_( Iface, Comp(*this) ( << "CPU[" << std::setfill('0') << std::setw(2) << flexusIndex() << "." << i << "] L1I PREFETCH-FILL " << reply->address()));
            theIcachePrefetch[i] = boost::none;
//            DBG_(Tmp, ( << "NLPreply: for " << reply->address()));
          }

          int32_t anIndex = 0;
          if (cfg.Threads > 1) {
        	  anIndex = nMTManager::MTManager::get()->scheduleFThread(flexusIndex());
          }
          std::set< PhysicalMemoryAddress >::iterator itr = theOutstandingMissQ[anIndex].find(reply->address());
          if (itr != theOutstandingMissQ[anIndex].end()) {
        	  theOutstandingMissQ[anIndex].erase(itr);
//        	  DBG_(Tmp, ( << "FDIPreply: for " << reply->address() << " outreq " << theOutstandingMissQ[anIndex].size() ));
          } else {
        	  assert(0);
          }
        }

        // Send an Ack if necessary
        if (cfg.SendAcks && reply->ackRequired()) {
          DBG_Assert( (aTransport[DestinationTag]) );
          aTransport[DestinationTag]->type = DestinationMessage::Directory;

          // We should include data in the ack iff:
          //   we received a reply directly from memory and we want to send a copy to the L2
          //   we received a reply from a peer cache (if the L2 had a copy, it would have replied instead of Fwding the request)
          MemoryMessage::MemoryMessageType ack_type = MemoryMessage::FetchAck;
          bool contains_data = reply->ackRequiresData();
          if (reply->type() == MemoryMessage::FwdReplyOwned) {
            contains_data = true;
            ack_type = MemoryMessage::FetchAckDirty;
          }
          queueSnoopMessage( aTransport, ack_type, reply->address(), contains_data );
        }
        break;
      }

      case MemoryMessage::Invalidate:

        inval( reply->address());
        if ( aTransport[DestinationTag] ) {
          aTransport[DestinationTag]->type = DestinationMessage::Requester;
        }
        queueSnoopMessage ( aTransport, MemoryMessage::InvalidateAck, reply->address() );
        break;

      case MemoryMessage::ReturnReq:
        queueSnoopMessage ( aTransport, MemoryMessage::ReturnReply, reply->address() );
        break;

      case MemoryMessage::Downgrade:
        // We can always reply to this message, regardless of the hit status
        queueSnoopMessage ( aTransport, MemoryMessage::DowngradeAck, reply->address() );
        break;

      case MemoryMessage::ReadFwd:
      case MemoryMessage::FetchFwd:
        // If we have the data, send a FwdReply
        // Otherwise send a FwdNAck
        DBG_Assert( aTransport[DestinationTag] );
        aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
        aTransport[TransactionTrackerTag]->setPreviousState(eShared);
        if (lookup(reply->address())) {
          aTransport[DestinationTag]->type = DestinationMessage::Requester;
          queueSnoopMessage( aTransport, MemoryMessage::FwdReply, reply->address());
        } else {
          std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
          if (iter != theEvictSet.end()) {
            aTransport[DestinationTag]->type = DestinationMessage::Requester;
            queueSnoopMessage( aTransport, MemoryMessage::FwdReply, reply->address());
          } else {
            aTransport[DestinationTag]->type = DestinationMessage::Directory;
            queueSnoopMessage( aTransport, MemoryMessage::FwdNAck, reply->address());
          }
        }
        break;

      case MemoryMessage::WriteFwd:
        // similar, but invalidate data
        DBG_Assert( aTransport[DestinationTag] );
        aTransport[TransactionTrackerTag]->setFillLevel(Flexus::SharedTypes::ePeerL1Cache);
        aTransport[TransactionTrackerTag]->setPreviousState(eShared);
        if (inval(reply->address())) {
          aTransport[DestinationTag]->type = DestinationMessage::Requester;
          queueSnoopMessage( aTransport, MemoryMessage::FwdReplyWritable, reply->address());
        } else {
          std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
          if (iter != theEvictSet.end()) {
            aTransport[DestinationTag]->type = DestinationMessage::Requester;
            queueSnoopMessage( aTransport, MemoryMessage::FwdReplyWritable, reply->address());
          } else {
            aTransport[DestinationTag]->type = DestinationMessage::Directory;
            queueSnoopMessage( aTransport, MemoryMessage::FwdNAck, reply->address());
          }
        }
        break;

      case MemoryMessage::BackInvalidate:
        // Same as invalidate
        aTransport[DestinationTag]->type = DestinationMessage::Directory;
#if 0
        // Only send an Ack if we actually have the block
        // If we don't have the block, then our InvalAck will race with an earlier Evict msg
        // let the Evict msg serve a dual purpose and skip the inval ack.
        if (inval( reply->address())) {
          queueSnoopMessage ( aTransport, MemoryMessage::InvalidateAck, reply->address() );
        } else {
          DBG_(Trace, Comp(*this) ( << "Received BackInvalidate for block not present in cache: " << *reply ));
        }
#endif

        // small protocol change - always send the InvalAck, whether we have the block or not
        inval(reply->address());
        queueSnoopMessage ( aTransport, MemoryMessage::InvalidateAck, reply->address() );
        break;

      case MemoryMessage::EvictAck: {
        std::set<uint64_t>::iterator iter = theEvictSet.find(reply->address());
        DBG_Assert(iter != theEvictSet.end(), Comp(*this) ( << "Block not found in EvictBuffer upon receipt of Ack: " << *reply ));
//        DBG_(Tmp, ( << "Removing from evict queue " << reply->address()));
        theEvictSet.erase(iter);
        break;
      }
      default:
        DBG_Assert ( false, Comp(*this) ( << "Unhandled message received: " << *reply ) );
    }
  }

  void issueEvict( PhysicalMemoryAddress anAddress) {
    if (cfg.CleanEvict && anAddress != 0) {
      MemoryTransport transport;
      boost::intrusive_ptr<MemoryMessage> operation( new MemoryMessage(MemoryMessage::EvictClean, anAddress ) );
      operation->reqSize() = 64;

      // Put in evict buffer and wait for Ack
      std::pair<std::set<uint64_t>::iterator, bool> ret = theEvictSet.insert(anAddress);
//      DBG_(Tmp, ( << "Moving to evict queue " << anAddress << " status " << ret.second));
      DBG_Assert(ret.second);
      theMaxOutstandingEvicts << theEvictSet.size();
      operation->ackRequired() = true;

      boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
      tracker->setAddress( anAddress );
      tracker->setInitiator(flexusIndex());
      transport.set(TransactionTrackerTag, tracker);
      transport.set(MemoryMessageTag, operation);

      if (cfg.EvictOnSnoop) {
        theSnoopQueue.push_back ( transport );
      } else {
        theMissQueue.push_back( transport );
      }
    }
  }

  void issueFetch( PhysicalMemoryAddress anAddress, VirtualMemoryAddress vPC) {
    DBG_Assert( anAddress != 0);
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( MemoryMessage::newFetch(anAddress, vPC));
    operation->reqSize() = 64;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( anAddress );
    tracker->setInitiator(flexusIndex());
    tracker->setFetch(true);
    tracker->setSource("uFetch");
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    theMissQueue.push_back(transport);
  }

  void issuePreFetch( PhysicalMemoryAddress anAddress, VirtualMemoryAddress vPC) {
    DBG_Assert( anAddress != 0);
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( MemoryMessage::newFetch(anAddress, vPC));
    operation->reqSize() = 64;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( anAddress );
    tracker->setInitiator(flexusIndex());
    tracker->setFetch(true);
    tracker->setSource("uFetch");
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    theFDIPPrefetchQueue.push_back(transport);
  }

  void issueRecMissPreFetch( PhysicalMemoryAddress anAddress, VirtualMemoryAddress vPC) {
    DBG_Assert( anAddress != 0);
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( MemoryMessage::newFetch(anAddress, vPC));
    operation->reqSize() = 64;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( anAddress );
    tracker->setInitiator(flexusIndex());
    tracker->setFetch(true);
    tracker->setSource("uFetch");
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    theRecMissPrefetchQueue.push_back(transport);
  }

  void missStats(index_t anIndex, FetchAddr fetch_addr) {

	/*Miss Distance Stats*/
	uint64_t lastResetDistance = theFlexus->cycleCount() - lastResetCycle;
	if (lastResetDistance >= MAX_RESET_DISTANCE - 4 && lastResetDistance < 50) {
		lastResetDistance = MAX_RESET_DISTANCE - 4;
	} else if (lastResetDistance >= 50 && lastResetDistance < 100) {
		lastResetDistance = MAX_RESET_DISTANCE - 3;
	} else if (lastResetDistance >= 100 && lastResetDistance < 150) {
		lastResetDistance = MAX_RESET_DISTANCE - 2;
	} else if (lastResetDistance >= 150) {
		lastResetDistance = MAX_RESET_DISTANCE - 1;
	}
//  	DBG_(Tmp, Comp(*this) ( << " Miss " << std::hex << fetch_addr.theAddress << " block " << std::hex << (fetch_addr.theAddress & theBlockMask) << " reset distance " << lastResetDistance << " prefetch on way " << !demandFetchIssued << " size " << theLastFetches[anIndex].size()));

  	(* lastReset[lastResetDistance])++;
	if (demandFetchIssued) {
		(* lastResetNP[lastResetDistance])++;
	}
    if (fetch_addr.theAddress == theLastFetches[anIndex][1].theAddress + 4) {
//            	  DBG_(Tmp, ( << "Seq Miss " << lastResetDistance));
  	  theSeqMiss++;
  	  (* lastResetSeq[lastResetDistance])++;
  	  if (demandFetchIssued) {
//  		  DBG_(Tmp, ( << "Seq Miss NP " << lastResetDistance));
  		  (* lastResetSeqNP[lastResetDistance])++;
  		  theSeqMissP++;
  	  }
    } else if (theLastFetches[anIndex][0].theBPState && theLastFetches[anIndex][0].theBPState->thePredictedType != kNonBranch && theLastFetches[anIndex][0].theBPState->thePrediction <= kTaken) {
  	  theBranch++;
  	  (* lastResetBranch[lastResetDistance])++;
  	  if (demandFetchIssued) {
//  		  DBG_(Tmp, ( << "Branch Miss "));
  		  (* lastResetBranchNP[lastResetDistance])++;
  		  theBranchP++;
  	  }
//            	  DBG_(Tmp, ( << "Branch Miss "));
    } else if (fetch_addr.wasRedirected) {
  	  switch (fetch_addr.redirectionCause) {
  	  case kResynchronize:
//            		  DBG_(Tmp, ( << "Resync Miss "));
  		  theResynchronization++;
      	  if (demandFetchIssued) {
      		  theResynchronizationP++;
//      		  DBG_(Tmp, ( << "Resync Miss "));
      	  }
  		  break;
        case kBranchMispredict:
//                	  DBG_(Tmp, ( << "BP Mispredict Miss "));
      	  theResyncMispred++;
      	  if (demandFetchIssued) {
      		  theResyncMispredP++;
//      		  DBG_(Tmp, ( << "BP Mispredict Miss "));
      	  }
      	  break;
        case kException:
//                	  DBG_(Tmp, ( << "Exception Miss "));
      	  theResyncException++;
      	  if (demandFetchIssued) {
      		  theResyncExceptionP++;
//      		  DBG_(Tmp, ( << "Exception Miss "));
      	  }
      	  break;
        case kInterrupt:
//                	  DBG_(Tmp, ( << "Interrupt Miss "));
      	  theResyncInterrupt++;
      	  if (demandFetchIssued) {
      		  theResyncInterruptP++;
//      		  DBG_(Tmp, ( << "Interrupt Miss "));
      	  }
      	  break;
        case kFailedSpec:
//                	  DBG_(Tmp, ( << "Spec failed Miss "));
      	  theResyncSpecFailed++;
      	  if (demandFetchIssued) {
      		  theResyncSpecFailedP++;
//      		  DBG_(Tmp, ( << "Spec failed Miss "));
      	  }
      	  break;
        default:
//                	  DBG_(Tmp, ( << "Unkonwn Miss "));
      	  theResyncUnknown++;
      	  if (demandFetchIssued) {
      		  theResyncUnknownP++;
//      		  DBG_(Tmp, ( << "Unkonwn Miss "));
      	  }
      	  break;
  	  }
    } else {
//            	  DBG_(Tmp, ( << "Othre Miss "));
  	  theOthers++;
  	  if (demandFetchIssued) {
  		  theOthersP++;
//  		  DBG_(Tmp, ( << "Othre Miss "));
  	  }
    }

    /*Fetch redirect reason stats*/
    assert(lastSquashCause < 6);
    if (lastResetDistance < 30) {

//		DBG_(Tmp, ( << "Miss squash reason " << lastSquashCause));
		(* squashReason[lastSquashCause])++;
		if (lastSquashCause == kBranchMispredict) {

	    	vaddr_pair missPair;
	    	missPair.first = lastSquashedBPState->pc;
	    	missPair.second = VirtualMemoryAddress(fetch_addr.theAddress & theBlockMask);
//	    	DBG_(Tmp, ( << "Sending miss pair pc " << lastSquashedBPState->pc << " missed block " << missPair.second << " predicted type " << lastSquashedBPState->thePredictedType << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(lastSquashedBPState->pc)));
	    	FLEXUS_CHANNEL(MissPairOut) << missPair;

			assert(lastSquashedBPState->thePredictedType < 10);
//			DBG_(Tmp, ( << "Miss squash Serial " << lastSquashedBPState->theSerial << " pc " << lastSquashedBPState->pc << " miss prediected " << lastSquashedBPState->thePredictedType << " actual type " << lastSquashedBPState->theActualType << " prefetch on way " << !demandFetchIssued << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(lastSquashedBPState->pc)));
			(* squashReasonBranch[lastSquashedBPState->thePredictedType])++;
			if (lastSquashedBPState->thePredictedType == kConditional) {
				if (lastSquashedBPState->bimodalPrediction) {
					assert(lastSquashedBPState->saturationCounter >= 0 && lastSquashedBPState->saturationCounter < 4);
					( *bimodalConfindence[lastSquashedBPState->saturationCounter])++;
				} else {
					assert(lastSquashedBPState->saturationCounter >= 0 && lastSquashedBPState->saturationCounter < 8);
					( *tageConfindence[lastSquashedBPState->saturationCounter])++;
				}
			} else if (lastSquashedBPState->thePredictedType == kReturn) {
				if (lastSquashedBPState->returnUsedRAS) {
//					DBG_(Tmp, ( << "Used RAS but wrong "));
					returnUsedRAS++;
				} else {
					if (lastSquashedBPState->thePredictedTarget == 0) {
						returnUsedBTBZero++;
					} else {
						returnUsedBTB++;
					}
				}
			}
		}
    }
    demandFetchIssued = false;
  }
  //Implementation of the FetchDrive drive interface
  void doFetch(index_t anIndex) {

//	  DBG_(Tmp, ( << std::endl<< "Entering uFetch " << theFlexus->cycleCount() << std::endl) );
    if (theIcacheMiss[anIndex]) {
//    	DBG_(Tmp, ( << "Handling I-cache Miss"));
      ++theMissCycles;
      return;
    }

    //Determine available FIQ this cycle
    int32_t available_fiq = 0;
    bool isFIQEmpty = false;
    std::pair<int, bool> decode_state;
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFIQ, anIndex ).available() ) ;
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( ROBEmptyIn, anIndex ).available() ) ;

    FLEXUS_CHANNEL_ARRAY( AvailableFIQ, anIndex ) >> decode_state;
    boost::tie(available_fiq, isFIQEmpty) = decode_state;

    bool isROBEmpty = false;
    FLEXUS_CHANNEL_ARRAY( ROBEmptyIn, anIndex ) >> isROBEmpty;

    if (isFIQEmpty && isROBEmpty) {
    	waitingForRetryDone = false;
    }

//    DBG_(Tmp, ( << "AvailableFIQ " << available_fiq << " FAQ Size " << theFAQ[anIndex].size()));
    if (available_fiq > 0 && ( theFAQ[anIndex].size() > 1 || theFlexus->quiescing()) ) {
      pFetchBundle bundle(new FetchBundle);
      bundle->coreID = theBundleCoreID;

      std::set< VirtualMemoryAddress> available_lines;
      int32_t remaining_fetch = cfg.MaxFetchInstructions;
      if (available_fiq < remaining_fetch) {
        remaining_fetch = available_fiq;
      }

//      DBG_(Tmp, ( << "AvailableFIQ " << available_fiq << " max addr " << remaining_fetch << " limit " << cfg.MaxFetchInstructions) );
      while ( remaining_fetch > 0 && ( theFAQ[anIndex].size() > 1 || theFlexus->quiescing()) && !waitingForRetryDone) {
        bool from_icache(false);

        FetchAddr fetch_addr = theFAQ[anIndex].front();
        VirtualMemoryAddress block_addr( fetch_addr.theAddress & theBlockMask);

//        DBG_(Tmp, ( << "Fetching " << fetch_addr.theAddress << " block " << block_addr));
        if ( available_lines.count( block_addr ) == 0) {
          //Line needs to be fetched from I-cache
          if (available_lines.size() >= cfg.MaxFetchLines) {
            //Reached limit of I-cache reads per cycle
//        	  DBG_(Tmp, ( << "Reached I-cache reads per cycle limit "));
            break;
          }

          // Notify the PowerTracker of Icache access
          bool garbage = true;
          FLEXUS_CHANNEL(InstructionFetchSeen) << garbage;

          if ( ! cfg.PerfectICache ) {
            //Do I-cache access here.
            if (! icacheLookup( anIndex, block_addr ) ) {
              missStats(anIndex, fetch_addr);
              break;
            }
            from_icache = true;
          }

          available_lines.insert( block_addr );
        }

        uint32_t op_code = fetchFromQEMU( anIndex, fetch_addr.theAddress );
        uint64_t aFetchSerial(theFAQ[anIndex].front().theSerial);

        theLastFetches[anIndex][0] = theLastFetches[anIndex][1];
        theLastFetches[anIndex][1] = fetch_addr;

        theFAQ[anIndex].pop_front();
//        DBG_(Tmp, ( << "Fetched " << fetch_addr.theAddress << " " <<std::hex << op_code << " " << Flexus::Simics::Processor::getProcessor(flexusIndex())->disassemble(fetch_addr.theAddress)));
        DBG_(Verb, ( << "Fetched " << fetch_addr.theAddress ) );
        bundle->theOpcodes.push_back( FetchedOpcode( fetch_addr.theAddress
                                      , theFAQ[anIndex].empty() ?  VirtualMemoryAddress(0) : theFAQ[anIndex].front().theAddress
                                      , op_code
                                      , fetch_addr.theBPState
                                      , theFetchReplyTransactionTracker[anIndex]
									  , aFetchSerial
                                      , nullptr // no InfoMissStats 
                                      )
                                    );
        if (from_icache && theLastMiss[anIndex] && theLastPhysical == theLastMiss[anIndex]->first) {
          bundle->theFillLevels.push_back(theLastMiss[anIndex]->second);
          theLastMiss[anIndex] = boost::none;
        } else {
          bundle->theFillLevels.push_back(eL1I);
        }
        ++theFetches;

        eBranchType thePredictedType = fetch_addr.theBPState->thePredictedType;
        if (thePredictedType == kRetry || thePredictedType == kDone) {
        	waitingForRetryDone = true;
        }
        if (op_code == kITLBMiss) {
          //stop fetch on an MMU exception
          break;
        }
        --remaining_fetch;
      }
      if (bundle->theOpcodes.size() > 0) {
        FLEXUS_CHANNEL_ARRAY( FetchBundleOut, anIndex ) << bundle;
      }
    }
//    DBG_(Tmp, ( << std::endl<< "Leaving uFetch " << std::endl) );
  }

  uint32_t fetchFromQEMU(index_t anIndex, VirtualMemoryAddress const & anAddress) {
    uint32_t op_code;
    Flexus::SharedTypes::Translation xlat;
    xlat.theVaddr = anAddress;
    xlat.thePSTATE = theCPUState[anIndex].thePSTATE;
    xlat.theType = Flexus::SharedTypes::Translation::eFetch;
    op_code = cpu(anIndex)->fetchInstruction(xlat.theVaddr);
    DBG_(Verb, Comp(*this) ( << "Fetch " << anAddress << " op: " << std::hex << std::setw(8) << op_code << std::dec ) );
    return op_code;
    /*
    } else {
      DBG_(Iface, Comp(*this) ( << "No translation for " << anAddress << "PSTATE: " << theCPUState[anIndex].thePSTATE << " MMU exception: " << xlat.theException << std::dec ) );
      return kITLBMiss // or other exception - OoO will figure it out
    }
  */
  }

  void doPrefetch(index_t anIndex) {
	  if (! cfg.FDIPEnabled) {
		  thePAQ[anIndex].clear();
		  return;
	  }

//	  DBG_(Tmp, ( << std::endl<< "Entering uPrefetch " << std::endl) );

	  std::set< VirtualMemoryAddress> available_lines;
	  while ( thePAQ[anIndex].size() > 0 ) {

		FetchAddr fetch_addr = thePAQ[anIndex].front();
		VirtualMemoryAddress block_addr( fetch_addr.theAddress & theBlockMask);

		if ( available_lines.find(block_addr) == available_lines.end()) {
			available_lines.insert( block_addr );
//			DBG_(Tmp, ( << "FDIP: potential prefetch " <<  block_addr));

			std::list<VirtualMemoryAddress>::iterator findIter = std::find(thePBQ[anIndex].begin(), thePBQ[anIndex].end(), block_addr);
//	    	std::list<VirtualMemoryAddress>::iterator itr = std::find(theOutstandingMissVQ[anIndex].begin(), theOutstandingMissVQ[anIndex].end(), block_addr);
	    	/*if (itr != theOutstandingMissVQ[anIndex].end()) {
	    		//Outstanding request, no need to prefetch
	    		DBG_(Tmp, ( << "FDIP: already outstanding " << std::hex << block_addr));
	    	} else*/ if ( findIter != thePBQ[anIndex].end()) {
//				DBG_(Tmp, ( << "FDIP: already in PBQ " << std::hex << block_addr));
			} else {
//				DBG_(Tmp, ( << "FDIP: push in PBQ " << std::hex << block_addr));
				thePBQ[anIndex].push_back(block_addr);
			}
		}
		thePAQ[anIndex].pop_front();
	  }

	  enqueuePrefetch(anIndex);

	  if (! cfg.RecMissEnabled) {
		  theRecordedMissQ[anIndex].clear();
		  return;
	  } else {
		  enqueueRecMissPrefetch(anIndex);
	  }

  }

  void enqueuePrefetch(index_t anIndex) {
	  if (thePBQ[anIndex].size()) {
		VirtualMemoryAddress prefetch_addr = thePBQ[anIndex].front();
//		DBG_(Tmp, ( << "FDIP: check block " << std::hex << prefetch_addr));

		PhysicalMemoryAddress paddr;
		Flexus::SharedTypes::Translation xlat;
		xlat.theVaddr = prefetch_addr;
		xlat.thePSTATE = theCPUState[anIndex].thePSTATE;
		xlat.theType = Flexus::SharedTypes::Translation::eFetch;
        xlat.thePaddr = cpu(anIndex)->translateVirtualAddress(xlat.theVaddr);
		paddr = xlat.thePaddr;
		if (! paddr ) {
//			DBG_(Tmp, ( << "NLP translation failed"));
			//Unable to translate for prefetch. Fixme: should we just move to the next request or keep trying translation
			return;
		} else {
	    	if (theOutstandingMissQ[anIndex].find(paddr) != theOutstandingMissQ[anIndex].end()) {
	    		//Fetch already sent
//	    		DBG_(Tmp, ( << "FDIP: Outstanding req " << std::hex << prefetch_addr));
	    		thePBQ[anIndex].pop_front();
//	    		assert(0);
	    	} else if ( theIcachePrefetch[anIndex] && paddr == *theIcachePrefetch[anIndex] ) {
				assert(0);
				thePBQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "FDIP: NLP request already sent " << std::hex << prefetch_addr));
				//Prefetch already sent
			} else if ( theIcacheMiss[anIndex] && paddr == *theIcacheMiss[anIndex] ) {
				assert(0);
				thePBQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "FDIP: demand fetch already sent " << std::hex << prefetch_addr));
				//Fetch already sent
			} else if (lookup( paddr )) {
				thePBQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "FDIP: already in cache " << std::hex << prefetch_addr << " paddr " << paddr));
			  //No need to prefetch, already in cache
			} else {
				if (theFDIPPrefetchQueue.size() < theMaxOutstandingFDIPMisses) {
				  theOutstandingMissQ[anIndex].insert(paddr);
				  addOustandingMiss(anIndex, prefetch_addr);
				  issuePreFetch( paddr , prefetch_addr);
//				  DBG_(Tmp, ( << "FDIP: issue prefech " << std::hex << paddr << " vaddr " << prefetch_addr << " outreq " << theOutstandingMissQ[anIndex].size()));
				  thePBQ[anIndex].pop_front();
				} else {
//					DBG_(Tmp, ( << "FDIP: max outstanding req " << prefetch_addr ));
				}
			}
		}
	  }
  }

  void enqueueRecMissPrefetch(index_t anIndex) {
	  if (theRecordedMissQ[anIndex].size()) {
		VirtualMemoryAddress prefetch_addr = theRecordedMissQ[anIndex].front();
//		DBG_(Tmp, ( << "Rec miss: check block " << std::hex << prefetch_addr));

		PhysicalMemoryAddress paddr;
		Flexus::SharedTypes::Translation xlat;
		xlat.theVaddr = prefetch_addr;
		xlat.thePSTATE = theCPUState[anIndex].thePSTATE;
		xlat.theType = Flexus::SharedTypes::Translation::eFetch;
        xlat.thePaddr = cpu(anIndex)->translateVirtualAddress(xlat.theVaddr);
		paddr = xlat.thePaddr;
		if (! paddr ) {
//			DBG_(Tmp, ( << "NLP translation failed"));
			//Unable to translate for prefetch. Fixme: should we just move to the next request or keep trying translation
			return;
		} else {
	    	if (theOutstandingMissQ[anIndex].find(paddr) != theOutstandingMissQ[anIndex].end()) {
	    		//Fetch already sent
//	    		DBG_(Tmp, ( << "recmiss: Outstanding req " << std::hex << prefetch_addr));
	    		theRecordedMissQ[anIndex].pop_front();
//	    		assert(0);
	    	} else if ( theIcachePrefetch[anIndex] && paddr == *theIcachePrefetch[anIndex] ) {
				assert(0);
				theRecordedMissQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "FDIP: NLP request already sent " << std::hex << prefetch_addr));
				//Prefetch already sent
			} else if ( theIcacheMiss[anIndex] && paddr == *theIcacheMiss[anIndex] ) {
				assert(0);
				theRecordedMissQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "FDIP: demand fetch already sent " << std::hex << prefetch_addr));
				//Fetch already sent
			} else if (lookup( paddr )) {
				theRecordedMissQ[anIndex].pop_front();
//				DBG_(Tmp, ( << "recmiss: already in cache " << std::hex << prefetch_addr << " paddr " << paddr));
			  //No need to prefetch, already in cache
			} else {
				if (theRecMissPrefetchQueue.size() < theMaxOutstandingRecMisses) {
				  theOutstandingMissQ[anIndex].insert(paddr);
				  addOustandingMiss(anIndex, prefetch_addr);
				  issueRecMissPreFetch( paddr , prefetch_addr);
//				  DBG_(Tmp, ( << "recmiss: issue prefech " << std::hex << paddr << " vaddr " << prefetch_addr << " outreq " << theOutstandingMissQ[anIndex].size()));
				  theRecordedMissQ[anIndex].pop_front();
				} else {
//					DBG_(Tmp, ( << "recmiss: max outstanding req " << prefetch_addr ));
				}
			}
		}
	  }
  }

};

}//End namespace nuFetch

FLEXUS_COMPONENT_INSTANTIATOR(uFetch, nuFetch);

FLEXUS_PORT_ARRAY_WIDTH( uFetch, FetchAddressIn )           {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, PrefetchAddressIn )           {//Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, RASOpsIn )           {//Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, RecordedMissIn )           {//Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, SquashIn )      {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, SquashBranchIn )      {	//Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, ChangeCPUState)   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, AvailableFAQOut )    {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, AvailableFIQ )   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, ROBEmptyIn )   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, FetchBundleOut)   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, ICount )   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( uFetch, Stalled )   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(uFetch, ResyncIn) {
  return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uFetch

#define DBG_Reset
#include DBG_Control()
