#ifndef _BASECACHECONTROLLERIMPL_HPP
#define _BASECACHECONTROLLERIMPL_HPP

#include <list>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/AbstractFactory.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#include "AbstractArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"

namespace nNewCache {

using namespace Flexus::SharedTypes;
typedef MemoryMessage::MemoryAddress MemoryAddress;

// For the love of god, please simplify type names.
typedef boost::intrusive_ptr<MemoryMessage>      MemoryMessage_p;
typedef boost::intrusive_ptr<TransactionTracker> TransactionTracker_p;

typedef boost::intrusive_ptr<const MemoryMessage>      const_MemoryMessage_p;

enum enumAction {
  kNoAction                   //Drop the message on the floor
  , kSend                       //Send the message on its way
  , kInsertMAF_WaitAddress      //Create a MAF entry for the message in WaitAddress state
  , kInsertMAF_WaitSnoop        //Create a MAF entry for the message in WaitSnoop state
  , kInsertMAF_WaitEvict        //Create a MAF entry for the message in WaitSnoop state
  , kInsertMAF_WaitResponse     //Create a MAF entry for the message in WaitResponse state
  , kInsertMAF_WaitProbe        //Create a MAF entry for the message in WaitProbe state
  , kReplyAndRemoveMAF          //Remove the MAF entry and send the stored message on its way
  , kReplyAndRemoveResponseMAF  // Remove the MAF entry for a response MAF, not probe MAF
  , kInsertMAF_WaitRegion        //Create a MAF entry for the message in WaitRegion state
  , kRetryRequest            // Transition to WaitAddress and immediately wake-up and start over
  , kReplyAndRetryMAF            // Transition to WaitAddress and immediately wake-up and start over
};

struct Action {
  enumAction theAction;
  TransactionTracker_p theOutputTracker;

  MemoryTransport theFrontTransport;
  MemoryTransport theBackTransport;

  bool theFrontMessage;
  bool theBackMessage;

  bool theFrontToDCache;
  bool theFrontToICache;

  int32_t theRequiresData;
  int32_t theRequiresTag;

  bool theWakeSnoops;
  bool theWakeEvicts;
  bool theWakeRegion;
  bool theRememberSnoopTransport;

  Action(enumAction anAction = kNoAction, int32_t aRequiresData = 0)
    : theAction(anAction)
    , theFrontMessage(false)
    , theBackMessage(false)
    , theFrontToDCache(true)
    , theFrontToICache(false)
    , theRequiresData(aRequiresData)
    , theRequiresTag(1)
    , theWakeSnoops(false)
    , theWakeEvicts(false)
    , theWakeRegion(false)
    , theRememberSnoopTransport(true)
  {}

  Action(enumAction anAction, TransactionTracker_p aTracker, int32_t aRequiresData = 0)
    : theAction(anAction)
    , theOutputTracker(aTracker)
    , theFrontMessage(false)
    , theBackMessage(false)
    , theFrontToDCache(true)
    , theFrontToICache(false)
    , theRequiresData(aRequiresData)
    , theRequiresTag(1)
    , theWakeSnoops(false)
    , theWakeEvicts(false)
    , theWakeRegion(false)
    , theRememberSnoopTransport(true)
  {}

};

std::ostream & operator << ( std::ostream & s, const enumAction eAction );
std::ostream & operator << ( std::ostream & s, const Action     action );

// A structure that consolidates the initial info for creating any cache
// Passing this structure is less error-prone than trying to pass all of the
// arguments piecemeal.
class CacheInitInfo {
public:

  CacheInitInfo ( std::string aName,
                  int32_t  aCores,
                  std::string const & anArrayConfiguration,
                  int32_t  aBlockSize,
                  int32_t  aNodeId,
                  tFillLevel aCacheLevel,
                  int32_t  anEBSize,
                  int32_t  anSBSize,
                  bool aProbeOnIfetchMiss,
                  bool aDoCleanEvictions,
                  bool anAllowOffChipStreamFetch
                )
    :    theName              ( aName )
    , theCores             ( aCores )
    , theArrayConfiguration( anArrayConfiguration )
    , theBlockSize         ( aBlockSize )
    , theNodeId               ( aNodeId )
    , theCacheLevel        ( aCacheLevel )
    , theEBSize            ( anEBSize )
    , theSBSize            ( anSBSize )
    , theProbeOnIfetchMiss ( aProbeOnIfetchMiss )
    , theDoCleanEvictions  ( aDoCleanEvictions )
    , theAllowOffChipStreamFetch( anAllowOffChipStreamFetch    )
  {}

  std::string theName;
  int32_t  theCores;
  std::string theArrayConfiguration;
  int32_t  theBlockSize;
  int32_t  theNodeId;
  tFillLevel theCacheLevel;
  int32_t  theEBSize; // EvictBuffer Size
  int32_t  theSBSize; // SnoopBuffer Size
  bool theProbeOnIfetchMiss;
  bool theDoCleanEvictions;
  bool theAllowOffChipStreamFetch;
};

struct BaseCacheControllerImpl;
};

#include "CacheController.hpp"

namespace nNewCache {

//The CacheControllerImpl owns the CacheArray, EvictBuffer, and SnoopBuffer.
//It understands the cache protocol and does all the work.  It does NOT know
//about transport object, just MemoryMessages and TransactionTrackers.  This
//allows it to compile separately from the Flexus wiring.
struct BaseCacheControllerImpl {

protected:
  CacheController * theController;

  typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;

  // Basic structures that every cache needs
  SnoopBuffer  theSnoopBuffer;
  RequestTracker theRequestTracker; // Tracks # of active requests per set

  virtual const AbstractEvictBuffer & const_EvictBuffer() const  = 0;
  virtual AbstractEvictBuffer & evictBuffer() = 0;

  CacheInitInfo * theInit;

  std::string theName;

  int32_t  theBlockSize;
  int32_t  theNodeId;
  bool theProbeOnIfetchMiss;

  MissTracker theReadTracker;
  tFillLevel theCacheLevel;
  tFillLevel thePeerLevel;
  bool theDoCleanEvictions;

  // Statistics
  Stat::StatCounter accesses;
  Stat::StatCounter accesses_user_I;
  Stat::StatCounter accesses_user_D;
  Stat::StatCounter accesses_system_I;
  Stat::StatCounter accesses_system_D;
  Stat::StatCounter requests;
  Stat::StatCounter hits;
  Stat::StatCounter hits_user_I;
  Stat::StatCounter hits_user_D;
  Stat::StatCounter hits_user_D_Read;
  Stat::StatCounter hits_user_D_Write;
  Stat::StatCounter hits_user_D_PrefetchRead;
  Stat::StatCounter hits_user_D_PrefetchWrite;
  Stat::StatCounter hits_system_I;
  Stat::StatCounter hits_system_D;
  Stat::StatCounter hits_system_D_Read;
  Stat::StatCounter hits_system_D_Write;
  Stat::StatCounter hits_system_D_PrefetchRead;
  Stat::StatCounter hits_system_D_PrefetchWrite;
  Stat::StatCounter hitsEvict;
  Stat::StatCounter misses;
  Stat::StatCounter misses_user_I;
  Stat::StatCounter misses_user_D;
  Stat::StatCounter misses_user_D_Read;
  Stat::StatCounter misses_user_D_Write;
  Stat::StatCounter misses_user_D_PrefetchRead;
  Stat::StatCounter misses_user_D_PrefetchWrite;
  Stat::StatCounter misses_system_I;
  Stat::StatCounter misses_system_D;
  Stat::StatCounter misses_system_D_Read;
  Stat::StatCounter misses_system_D_Write;
  Stat::StatCounter misses_system_D_PrefetchRead;
  Stat::StatCounter misses_system_D_PrefetchWrite;
  Stat::StatCounter misses_peerL1_system_I;
  Stat::StatCounter misses_peerL1_system_D;
  Stat::StatCounter misses_peerL1_user_I;
  Stat::StatCounter misses_peerL1_user_D;
  Stat::StatCounter upgrades;
  Stat::StatCounter fills;
  Stat::StatCounter upg_replies;
  Stat::StatCounter evicts_clean;
  Stat::StatCounter evicts_dirty;
  Stat::StatCounter evicts_dropped;
  Stat::StatCounter evicts_protocol;
  Stat::StatCounter snoops;
  Stat::StatCounter probes;
  Stat::StatCounter iprobes;
  Stat::StatCounter lockedAccesses;
  Stat::StatCounter tag_match_invalid;
  Stat::StatCounter prefetchReads;
  Stat::StatCounter prefetchHitsRead;
  Stat::StatCounter prefetchHitsPrefetch;
  Stat::StatCounter prefetchHitsWrite;
  Stat::StatCounter prefetchHitsButUpgrade;
  Stat::StatCounter prefetchEvicts;
  Stat::StatCounter prefetchInvals;
  Stat::StatCounter atomicPreloadWrites;

public:

  static BaseCacheControllerImpl * construct ( CacheController * aController,
      CacheInitInfo  * anInfo,
      const std::string & type );

  BaseCacheControllerImpl ( CacheController * aController,
                            CacheInitInfo    *    anInit );

  virtual ~BaseCacheControllerImpl() {}

  virtual uint32_t freeEvictBuffer() const {
    uint32_t eb_free = const_EvictBuffer().freeEntries();
    uint32_t er_free = arrayEvictResourcesFree();
    return ((eb_free < er_free) ? eb_free : er_free);
  }

  virtual bool emptyEvictBuffer() const {
    return const_EvictBuffer().empty();
  }

  virtual bool fullEvictBuffer() const {
//if (const_EvictBuffer().full() || (arrayEvictResourcesAvailable() == 0)) {
// DBG_(Trace, ( << theName << " Full EvictBuffer: " << std::boolalpha << const_EvictBuffer().full() << ", " << (bool)(arrayEvictResourcesAvailable() == 0) ));
// if (const_EvictBuffer().full()) {
//  dumpEvictBuffer();
// }
//}
    return (const_EvictBuffer().full() || (arrayEvictResourcesAvailable() == 0));
  }

  virtual void reserveEvictBuffer() {
    evictBuffer().reserve();
    reserveArrayEvictResource();
//DBG_(Trace, ( << theName << " reserveEvictBuffer, Free entries = " << freeEvictBuffer() ));
  }

  virtual void unreserveEvictBuffer() {
    evictBuffer().unreserve();
    unreserveArrayEvictResource();
//DBG_(Trace, ( << theName << " unreserveEvictBuffer, Free entries = " << freeEvictBuffer() ));
  }

  virtual bool evictableBlockExists(int32_t anIndex) const {
    return const_EvictBuffer().headEvictable(anIndex);
  }

  virtual bool fullSnoopBuffer() const {
    return theSnoopBuffer.full();
  }

  virtual void reserveSnoopBuffer() {
    theSnoopBuffer.reserve();
  }

  virtual void unreserveSnoopBuffer() {
    theSnoopBuffer.unreserve();
  }

  virtual bool isQuiesced() const {
    return theSnoopBuffer.empty();
  }

  virtual void dumpEvictBuffer() const {
    uint32_t eb_free = const_EvictBuffer().freeEntries();
    uint32_t eb_reserved = const_EvictBuffer().reservedEntries();
    uint32_t eb_used = const_EvictBuffer().usedEntries();
    uint32_t er_free = arrayEvictResourcesFree();
    DBG_(VVerb, ( << theName << " EB Free/Reserved/Used " << eb_free << "/" << eb_reserved << "/" << eb_used << ", ERB free " << er_free ));
  }

  virtual void dumpSnoopBuffer() const {
    //theSnoopBuffer.dump();
    uint32_t sb_free = theSnoopBuffer.freeEntries();
    uint32_t sb_size = theSnoopBuffer.size();
    uint32_t sb_reserve = theSnoopBuffer.reservedEntries();
    DBG_(VVerb, ( << theName << " SnoopBuffer Size/Reserve/Free = " << sb_size << "/" << sb_reserve << "/" << sb_free ));
  }

  virtual void saveState(std::string const & aDirName);
  virtual void loadState(std::string const & aDirName, bool aTextFlexpoint, bool aGZippedFlexpoint);

  virtual void saveArrayState(std::ostream & os) = 0;
  virtual bool loadArrayState(std::istream & is, bool aTextFlexpoint) = 0;

  virtual MemoryAddress getBlockAddress( MemoryAddress const & anAddress) const = 0;
  virtual BlockOffset getBlockOffset( MemoryAddress const & anAddress) const = 0;
  virtual std::function<bool (MemoryAddress a, MemoryAddress b)> setCompareFn() const = 0;

  virtual bool canStartRequest( MemoryAddress const & anAddress) const = 0;
  virtual void addPendingRequest( MemoryAddress const & anAddress) = 0;
  virtual void removePendingRequest( MemoryAddress const & anAddress) = 0;

protected:

  //Simple accessor functions for getting information about a memory
  //message

  // These are used for simple translations to various address types
  inline MemoryAddress getBlockAddress(MemoryMessage const & aMessage) {
    return getBlockAddress(aMessage.address());
  }
  inline BlockOffset getBlockOffset(MemoryMessage const & aMessage) {
    return getBlockOffset(aMessage.address());
  }

  ///////////////////////////
  // Eviction Processing

  // Remove an entry from the Evict Buffer and send the eviction message to the lower level (ie. Directory/Memory/Lower level cache)
public:
  virtual Action doEviction();

protected:
  virtual uint32_t arrayEvictResourcesFree() const = 0;
  virtual bool arrayEvictResourcesAvailable() const = 0;
  virtual void reserveArrayEvictResource() = 0;
  virtual void unreserveArrayEvictResource() = 0;

  ///////////////////////////////
  // Request Channel processing
public:
  virtual Action handleRequestTransport ( MemoryTransport      transport,
                                          bool waiting_for_response );

public:
  virtual Action wakeMaf ( MemoryTransport       transport,
                           TransactionTracker_p aWakingTracker);

protected:
  // Examine a request, given that the related set is not locked
  // Responsible for updating hit statistics
  // - returns the action to be taken by the CacheController
  virtual Action examineRequest ( MemoryTransport      transport,
                                  bool                 has_maf_entry,
                                  TransactionTracker_p aWakingTracker =  TransactionTracker_p() );

  // Perform lookup, select action and update cache state if necessary
  // Returns (was_hit, was_prefetch, Action)
  virtual std::tuple<bool, bool, Action> doRequest ( MemoryTransport        transport,
      bool                   has_maf_entry,
      TransactionTracker_p aWakingTracker =  TransactionTracker_p() ) = 0;

public:

  /////////////////////////////////////////
  // Back Channel Processing
  virtual Action handleBackMessage ( MemoryTransport      transport ) = 0;

public:
  /////////////////////////////////////////
  // Snoop Channel Processing
  virtual bool snoopResourcesAvailable(const const_MemoryMessage_p msg);

  virtual void reserveSnoopResources(MemoryMessage_p msg, ProcessEntry_p process);
  virtual void unreserveSnoopResources(MemoryMessage_p msg, ProcessEntry_p process);

  virtual Action handleSnoopMessage ( MemoryTransport transport ) = 0;

  virtual Action handleIprobe ( bool aHit, MemoryTransport transport ) = 0;

  /////////////////////////////////////////
  // Idle Work Processing -> additional controller specific work
  //

  // Do we have work available? Are the required resources available?
  virtual bool idleWorkAvailable() {
    return false;
  }

  // get a message decsribing the work to be done
  // also reserve any necessary controller resources
  virtual MemoryMessage_p getIdleWorkMessage(ProcessEntry_p process) {
    return nullptr;
  }

  virtual void removeIdleWorkReservations(ProcessEntry_p process, Action & action) { }

  // take any necessary actions to start the idle work
  virtual Action handleIdleWork( MemoryTransport transport) {
    return Action(kNoAction, transport[TransactionTrackerTag]);
  }

  // take any necessary actions to complete the idle work
  virtual void completeIdleWork( MemoryMessage_p msg ) {
    return;
  }

  virtual bool hasWakingSnoops() {
    return !theSnoopBuffer.wakeListEmpty();
  }

  virtual MemoryTransport getWakingSnoopTransport();

  virtual Action handleWakeSnoop ( MemoryTransport transport ) = 0;

  virtual std::list<MemoryAddress> getRegionBlockList(MemoryAddress addr) {
    std::list<MemoryAddress> ret;
    ret.push_back(addr);
    return ret;
  }

}; // class BaseCacheControllerImpl

typedef std::pair<CacheController *, CacheInitInfo *> ControllerParams;

// Specific Controller Implementations should declare the two following public members:
//    1. static std::string name
//    2. static BaseCacheControllerImpl* createInstance( std::list< std::pair<std::string, std::string> > &args, const ControllerParams &params)
//
// Each Controller Implementation should then use the following macro to "register" itself.
// This allows for automatic inclusion of any new controller types without modifying ANY other code
// (assuming the new controller uses the same interface correctly)

#define REGISTER_CONTROLLER_IMPLEMENTATION(type, n) const std::string type::name = n; static ConcreteFactory<BaseCacheControllerImpl,type,ControllerParams> type ## _Factory

}; // namespace nNewCache

#endif // _BASECACHECONTROLLERIMPL_HPP
