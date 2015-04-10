#include <components/CacheTraceMemory/CacheTraceMemory.hpp>

#define DBG_DefineCategories CacheTrace, Memory
#define DBG_SetDefaultOps AddCat(CacheTrace) AddCat(Memory)
#include DBG_Control()

#include <fstream>
#include <zlib.h>
#include <list>

#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <components/Common/MemoryMap.hpp>

#include <core/simics/configuration_api.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>

#define FLEXUS_BEGIN_COMPONENT CacheTraceMemory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nCacheTraceMemory {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

using boost::intrusive_ptr;

/*
class CacheTraceMemorySimicsIface {
public:
  virtual void flushFiles() {
    DBG_Assert(false);
  }
};

class CacheTraceMemory_SimicsObject_Impl  {
    CacheTraceMemorySimicsIface * theComponentInterface; //Non-owning pointer
  public:
    CacheTraceMemory_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * ) : theComponentInterface(0) {}

    void setComponentInterface(CacheTraceMemorySimicsIface * aComponentInterface) {
      theComponentInterface = aComponentInterface;
    }

    void flushTraceFiles() {
      theComponentInterface->flushFiles();
    }

};

class CacheTraceMemory_SimicsObject : public Simics::AddInObject <CacheTraceMemory_SimicsObject_Impl> {
    typedef Simics::AddInObject<CacheTraceMemory_SimicsObject_Impl> base;
   public:
    static const Simics::Persistence  class_persistence = Simics::Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "CacheTraceMemory"; }
    static std::string classDescription() { return "CacheTraceMemory object"; }

    CacheTraceMemory_SimicsObject() : base() { }
    CacheTraceMemory_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
    CacheTraceMemory_SimicsObject(CacheTraceMemory_SimicsObject_Impl * anImpl) : base(anImpl) {}

    template <class Class>
    static void defineClass(Class & aClass) {

      aClass.addCommand
        ( & CacheTraceMemory_SimicsObject_Impl::flushTraceFiles
        , "flush-trace-files"
        , "flush the current trace files and switch to next"
        );

    }

};

Simics::Factory<CacheTraceMemory_SimicsObject> theCacheTraceMemoryFactory;
*/

typedef FILE  *  FILE_p;
typedef FILE_p * FILE_pp;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;
typedef uint64_t Time;

enum BlockStateEnum {
  StateShared,
  StateExclusive
};
struct BlockState : boost::counted_base {
  BlockState(int32_t node)
    : state(StateShared)
    , producer(-1)
    , time(0)
    , sharers(1 << node)
    , pc(0)
  {}
  BlockState(int32_t node, Time t, MemoryAddress aPc, bool aPriv)
    : state(StateExclusive)
    , producer(node)
    , time(t)
    , sharers(0)
    , pc(aPc)
    , priv(false)
  {}

  bool isSharer(int32_t node) {
    return( sharers & (1 << node) );
  }
  void setSharer(int32_t node) {
    sharers |= (1 << node);
  }
  void newShareList(int32_t node, int32_t oldOwner) {
    sharers = (1 << node);
    sharers |= (1 << oldOwner);
  }

  BlockStateEnum state;
  int32_t producer;
  Time time;
  uint32_t sharers;
  MemoryAddress pc;
  bool priv;
};
typedef intrusive_ptr<BlockState> BlockState_p;

class MemoryStateTable {
  typedef std::map<MemoryAddress, BlockState_p> StateMap;
  typedef StateMap::iterator Iterator;
  typedef std::pair<MemoryAddress, BlockState_p> BlockPair;
  typedef std::pair<Iterator, bool> InsertPair;

public:
  MemoryStateTable()
  {}

  BlockState_p find(MemoryAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if (iter != theBlockStates.end()) {
      return iter->second;
    }
    return BlockState_p(0);
  }

  void remove(MemoryAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if (iter != theBlockStates.end()) {
      theBlockStates.erase(iter);
    }
  }

  int64_t size() {
    return theBlockStates.size();
  }

  void addEntry(MemoryAddress anAddr, BlockState_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    BlockPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = theBlockStates.insert(insert);
    DBG_Assert(result.second);
  }

private:
  StateMap theBlockStates;

};  // end class MemoryStateTable

class FLEXUS_COMPONENT(CacheTraceMemory) {
  FLEXUS_COMPONENT_IMPL( CacheTraceMemory );

  //CacheTraceMemory_SimicsObject theCacheTraceMemoryObject;

  struct Xact {
    int32_t theNode;
    MemoryAddress theAddr;
    MemoryAddress thePC;
    bool thePriv;
    uint32_t theOpcode;
    MemoryTransport theTransport;
    Xact(int32_t aNode, MemoryAddress anAddr, MemoryAddress aPC, bool aPriv, uint32_t anOpcode, MemoryTransport aTrans)
      : theNode(aNode)
      , theAddr(anAddr)
      , thePC(aPC)
      , thePriv(aPriv)
      , theOpcode(anOpcode)
      , theTransport(aTrans)
    {}
  };

  std::list< Xact > theRequests;
  std::list< Xact > theSnoops;

  struct PendStruct {
    int32_t node;
    int32_t count;
    MemoryTransport transport;
    std::deque<Xact> waiting;
    PendStruct(int32_t aNode, int32_t aCount, MemoryTransport aTrans)
      : node(aNode)
      , count(aCount)
      , transport(aTrans)
    {}
  };

  std::map<MemoryAddress, PendStruct> thePending;
  typedef std::map<MemoryAddress, PendStruct>::iterator PendIter;

  // state for all active blocks
  MemoryStateTable myStateTable;

  // output files
  struct OutfileRecord {
    std::string theBaseName;
    int32_t theNumProcs;
    gzFile * theFiles;
    int32_t   *  theFileNo;
    int64_t  *  theFileLengths;

    static const int32_t K = 1024;
    static const int32_t M = 1024 * K;
    static const int32_t kMaxFileSize = 512 * M;

    OutfileRecord(char * aName)
      : theBaseName(aName)
      , theNumProcs(-1)
      , theFiles(0)
    {}
    ~OutfileRecord() {
      if (theFiles != 0) {
        int32_t ii;
        for (ii = 0; ii < theNumProcs; ii++) {
          gzclose(theFiles[ii]);
        }
        delete [] theFiles;
      }
    }

    void init(int32_t numProcs) {
      int32_t ii;
      theNumProcs = numProcs;
      theFiles = new gzFile[theNumProcs];
      theFileNo = new int[theNumProcs];
      theFileLengths = new long[theNumProcs];
      for (ii = 0; ii < theNumProcs; ii++) {
        theFiles[ii] = gzopen( nextFile(ii).c_str(), "w" );
        theFileNo[ii] = 0;
        theFileLengths[ii] = 0;
      }
    }
    void flush() {
      int32_t ii;
      for (ii = 0; ii < theNumProcs; ii++) {
        switchFile(ii);
      }
    }
    std::string nextFile(int32_t aCpu) {
      return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".sordtrace64." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
    }
    void switchFile(int32_t aCpu) {
      DBG_Assert(theFiles);
      gzclose(theFiles[aCpu]);
      std::string new_file = nextFile(aCpu);
      theFiles[aCpu] = gzopen( new_file.c_str(), "w" );
      theFileLengths[aCpu] = 0;
      DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    }
    void writeFile(uint8_t * aBuffer, int32_t aLength, int32_t aCpu) {
      DBG_Assert(theFiles);
      gzwrite(theFiles[aCpu], aBuffer, aLength);

      theFileLengths[aCpu] += aLength;
      if (theFileLengths[aCpu] > kMaxFileSize) {
        switchFile(aCpu);
      }
    }
  };
  OutfileRecord OffchipReads;
  OutfileRecord OffchipWrites;
  OutfileRecord OffchipUpgrades;
  OutfileRecord CohReads;
  OutfileRecord CohWrites;
  OutfileRecord CohUpgrades;

  // file I/O buffer
  uint8_t theBuffer[64];

  std::ofstream theMemoryOut;

  std::vector< boost::intrusive_ptr<MemoryMap> > theMemoryMaps;

  std::vector< std::deque<MemoryTransport> > outQueue;

  //Statistics

  Stat::StatCounter statReads;
  Stat::StatCounter statOSReads;
  Stat::StatCounter statWrites;
  Stat::StatCounter statOSWrites;
  Stat::StatCounter statUpgrades;
  Stat::StatCounter statOSUpgrades;
  Stat::StatCounter statProductions;
  Stat::StatCounter statOSProductions;
  Stat::StatCounter statConsumptions;
  Stat::StatCounter statOSConsumptions;
  Stat::StatCounter statColdReads;
  Stat::StatCounter statColdWrites;
  Stat::StatCounter statColdUpgrades;
  Stat::StatCounter statColdConsumptions;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueAddresses;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueOSAddresses;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueUserAddresses;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueSharedAddresses;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueSharedOSAddresses;
  Stat::StatUniqueCounter<MemoryAddress> statUniqueSharedUserAddresses;
  //Stat::StatInstanceCounter<MemoryAddress> statSharedAddressInstances;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CacheTraceMemory)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )

    , OffchipReads("offchip_read")
    , OffchipWrites("offchip_write")
    , OffchipUpgrades("offchip_upgrade")
    , CohReads("coh_read")
    , CohWrites("coh_write")
    , CohUpgrades("coh_upgrade")

    //Stat initialization
    , statReads(statName() + "-Reads")
    , statOSReads(statName() + "-Reads:OS")
    , statWrites( statName() + "-Writes")
    , statOSWrites(statName() + "-Writes:OS")
    , statUpgrades( statName() + "-Upgrades")
    , statOSUpgrades(statName() + "-Upgrades:OS")
    , statProductions(statName() + "-Productions")
    , statOSProductions(statName() + "-Productions:OS")
    , statConsumptions(statName() + "-Consumptions")
    , statOSConsumptions(statName() + "-Consumptions:OS")
    , statColdReads(statName() + "-Reads:cold")
    , statColdWrites(statName() + "-Writes:cold")
    , statColdUpgrades(statName() + "-Upgrades:cold")
    , statColdConsumptions(statName() + "Consumptions:cold")
    , statUniqueAddresses(statName() + "-UniqueAddresses")
    , statUniqueOSAddresses(statName() + "-UniqueAddresses:OS")
    , statUniqueUserAddresses(statName() + "-UniqueAddresses:User")
    , statUniqueSharedAddresses(statName() + "-UniqueAddresses:Shared")
    , statUniqueSharedOSAddresses(statName() + "-UniqueAddresses:Shared:OS")
    , statUniqueSharedUserAddresses(statName() + "-UniqueAddresses:Shared:User")
    //, statSharedAddressInstances(statName() + "-SharedAddressInstances")

  {
    //theCacheTraceMemoryObject = theCacheTraceMemoryFactory.create("cache-trace-mem");
    //theCacheTraceMemoryObject->setComponentInterface(this);
  }

  ~CacheTraceMemoryComponent() {
    // the OutfileRecords will close everything
  }

  bool isQuiesced() const {
    bool ok =  theRequests.empty() && theSnoops.empty();
    return ok;
  }

  // Initialization
  void initialize() {
    if (cfg.TraceEnabled) {
      OffchipReads.init(cfg.NumProcs);
      OffchipWrites.init(cfg.NumProcs);
      OffchipUpgrades.init(cfg.NumProcs);
      CohReads.init(cfg.NumProcs);
      CohWrites.init(cfg.NumProcs);
      CohUpgrades.init(cfg.NumProcs);
    }

    if (cfg.CurvesEnabled) {
      theMemoryOut.open("memory.out");
      Stat::getStatManager()->openLoggedPeriodicMeasurement("Memory over Time", 1000000, Stat::Accumulate, theMemoryOut , statName() + "-(?!SharedAddressInstances).*");
    }

    for (int32_t ii = 0; ii < cfg.NumProcs; ii++) {
      theMemoryMaps.push_back(MemoryMap::getMemoryMap(ii));
      outQueue.push_back(std::deque<MemoryTransport>());
    }
  }

//Need to check this implementation!!!!
  void finalize() override {

  }

  // Ports
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNode_Snoop);
  void push(interface::FromNode_Snoop const &, index_t anIndex, MemoryTransport & transport) {
    DBG_(Iface, Comp(*this) Addr(transport[MemoryMessageTag]->address())
         ( << "snoop from node " << anIndex << " received: " << *transport[MemoryMessageTag] ) );

    MemoryAddress blockAddr = blockAddress(transport[MemoryMessageTag]->address());
    //Allocate the page in the MemoryMap, if not already done
    theMemoryMaps[anIndex]->node(blockAddr);

    switch (transport[MemoryMessageTag]->type()) {
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::DownUpdateAck:
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
        theSnoops.push_back( Xact( static_cast<int>(anIndex), blockAddr, MemoryAddress(0), 0, 0, transport ) );
        break;
      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
        // do nothing
        break;
      default:
        DBG_Assert(false, Component(*this) ( << "Don't know how to handle snoop message: " << *transport[MemoryMessageTag]) );
        return;
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNode_Req);
  void push(interface::FromNode_Req const &, index_t anIndex, MemoryTransport & transport) {
    DBG_(Iface, Comp(*this) Addr(transport[MemoryMessageTag]->address())
         ( << "request from node " << anIndex << " received: " << *transport[MemoryMessageTag] ) );

    MemoryAddress blockAddr = blockAddress(transport[MemoryMessageTag]->address());
    //Allocate the page in the MemoryMap, if not already done
    theMemoryMaps[anIndex]->node(blockAddr);
    MemoryAddress pc(0);
    bool priv = false;
    uint32_t opcode = 0;
    if (transport[ExecuteStateTag]) {
      /* doesn't seem to work any more - twenisch will fix this
      SharedTypes::ExecuteState & ex( static_cast<SharedTypes::ExecuteState &>(*transport[ExecuteStateTag]) );
      pc = ex.instruction().physicalInstructionAddress();
      priv = ex.instruction().isPriv();
      opcode = ex.instruction().opcode();
      */
    }

    switch (transport[MemoryMessageTag]->type()) {
      case MemoryMessage::ReadReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::UpgradeReq:
        theRequests.push_back( Xact( static_cast<int>(anIndex), blockAddr, pc, priv, opcode, transport ) );
        break;
      default:
        DBG_Assert(false, Component(*this) ( << "Don't know how to handle req message: " << *transport[MemoryMessageTag]) );
        return;
    }
  }

  void drive( interface::MemoryDrive const & ) {
    DBG_( Verb, Comp(*this) ( << "MemoryDrive" ));
    doMemoryDrive();

    int32_t ii;
    for (ii = 0; ii < cfg.NumProcs; ii++) {
      while (!outQueue[ii].empty()) {
        if ( FLEXUS_CHANNEL_ARRAY( ToNode, ii).available() ) {
          DBG_(Iface, Comp(*this) Addr(outQueue[ii].front()[MemoryMessageTag]->address())
               ( << "msg for node " << ii << ": " << *outQueue[ii].front()[MemoryMessageTag] ) );
          FLEXUS_CHANNEL_ARRAY( ToNode, ii) << outQueue[ii].front();
          outQueue[ii].pop_front();
        } else {
          break;
        }
      }
    }
  }

private:
  void doMemoryDrive() {
    while (! theSnoops.empty() ) {
      // this snoop ack must match a pending operation
      PendIter iter = thePending.find(theSnoops.front().theAddr);
      DBG_Assert(iter != thePending.end(), Comp(*this));

      if (theSnoops.front().theTransport[MemoryMessageTag]->type() == MemoryMessage::InvUpdateAck ||
          theSnoops.front().theTransport[MemoryMessageTag]->type() == MemoryMessage::DownUpdateAck ) {
        // change upgrade reqeusts to write requests
        if (iter->second.transport[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq) {
          iter->second.transport[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
        }
      }

      bool stillPending = true;

      // check if this is the last ack expected
      --(iter->second.count);
      if (iter->second.count == 0) {
        // send the reply and wake a subsequent operation
        reply(iter->second.node, iter->second.transport);
        stillPending = false;

        while (!iter->second.waiting.empty()) {
          Xact woken = iter->second.waiting.front();
          iter->second.waiting.pop_front();
          int32_t pending = performOperation(woken);
          if (pending > 0) {
            iter->second.node = woken.theNode;
            iter->second.count = pending;
            iter->second.transport = woken.theTransport;
            stillPending = true;
            break;
          }
          reply(woken.theNode, woken.theTransport);
        }
      }

      // clean up if no longer pending
      if (!stillPending) {
        thePending.erase(iter);
      }

      theSnoops.pop_front();
    }

    while (! theRequests.empty() ) {
      // first check for a pending operation for this address
      PendIter iter = thePending.find(theRequests.front().theAddr);
      if (iter != thePending.end()) {
        // attach to the pending operation
        iter->second.waiting.push_back(theRequests.front());
      } else {
        // perform the memory operation
        int32_t pending = performOperation(theRequests.front());
        if (pending == 0) {
          reply(theRequests.front().theNode, theRequests.front().theTransport);
        } else {
          PendStruct waiting(theRequests.front().theNode,
                             pending,
                             theRequests.front().theTransport);
          thePending.insert( std::make_pair(theRequests.front().theAddr, waiting) );
        }
      }

      theRequests.pop_front();
    }
  }  // doMemoryDrive()

  int32_t performOperation(Xact req) {
    int32_t pending = 0;
    switch (req.theTransport[MemoryMessageTag]->type()) {
      case MemoryMessage::WriteReq:
        pending = recvWrite( req.theNode,
                             req.theAddr,
                             req.thePC,
                             req.thePriv,
                             req.theOpcode );
        break;
      case MemoryMessage::UpgradeReq:
        pending = recvUpgrade( req.theNode,
                               req.theAddr,
                               req.thePC,
                               req.thePriv,
                               req.theOpcode );
        break;
      case MemoryMessage::ReadReq:
        pending = recvRead( req.theNode,
                            req.theAddr,
                            req.thePC,
                            req.thePriv,
                            req.theOpcode );
        break;
      default:
        DBG_Assert(false, Component(*this) ( << "Don't know how to handle message: " << *req.theTransport[MemoryMessageTag]) );
    }
    return pending;
  }

  void reply(int32_t node, MemoryTransport transport) {
    intrusive_ptr<MemoryMessage> reply;
    switch (transport[MemoryMessageTag]->type()) {
      case MemoryMessage::ReadReq:
        reply = new MemoryMessage(MemoryMessage::MissReply, transport[MemoryMessageTag]->address());
        break;
      case MemoryMessage::WriteReq:
        reply = new MemoryMessage(MemoryMessage::MissReplyWritable, transport[MemoryMessageTag]->address());
        break;
      case MemoryMessage::UpgradeReq:
        reply = new MemoryMessage(MemoryMessage::UpgradeReply, transport[MemoryMessageTag]->address());
        break;
      default:
        DBG_Assert(false, Component(*this) ( << "Don't know how to handle req message.") );
    }

    reply->reqSize() = cfg.BlockSize;
    transport.set(MemoryMessageTag, reply);
    outQueue[node].push_back(transport);
  }

  int32_t doDowngrade(MemoryAddress addr, int32_t node) {
    DBG_Assert( (node >= 0) && (node < cfg.NumProcs) );
    intrusive_ptr<MemoryMessage> snoop(new MemoryMessage(MemoryMessage::Downgrade, addr));
    MemoryTransport transport;
    transport.set(MemoryMessageTag, snoop);
    outQueue[node].push_back(transport);
    return 1;
  }

  int32_t doInvalidate(MemoryAddress addr, int32_t node) {
    DBG_Assert( (node >= 0) && (node < cfg.NumProcs) );
    intrusive_ptr<MemoryMessage> snoop(new MemoryMessage(MemoryMessage::Invalidate, addr));
    MemoryTransport transport;
    transport.set(MemoryMessageTag, snoop);
    outQueue[node].push_back(transport);
    return 1;
  }

  int32_t doInvalidate(MemoryAddress addr, uint32_t sharers, int32_t node) {
    int32_t count = 0;
    int32_t ii;

    for (ii = 0; ii < cfg.NumProcs; ii++) {
      if (sharers & (1 << ii)) {
        // don't invalidate the upgrader
        if (ii != node) {
          count += doInvalidate(addr, ii);
        }
      }
    }
    return count;
  }

  //--- Base functions for maintaining state -----------------------------------
  int32_t recvRead(int32_t node, MemoryAddress addr, MemoryAddress pc, bool priv, uint32_t opcode) {
    DBG_(Verb, ( << "got read from node " << node << " for addr " << addr ) );
    int32_t pending = 0;

    ++statReads;
    if (priv) ++statOSReads;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if (block) {
      DBG_(Verb, ( << "retrieved block state" ) );

      if (block->state == StateShared) {
        DBG_(Verb, ( << "state shared" ) );

        if (!block->isSharer(node)) {
          DBG_(Verb, ( << "new sharer" ) );

          // new sharer - record first access to this block by this node
          //consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
          block->setSharer(node);
        }
      } else { // StateExclusive
        // check if the reader is already the node with exclusive access
        if (block->producer == node) {
          DBG_(Verb, ( << "state exclusive - read by owner" ) );
        } else {
          DBG_(Verb, ( << "state exclusive - read by another" ) );

          pending = doDowngrade(addr, block->producer);

          // record the last store time of this address to file
          //production(block->producer, addr, block->pc, block->time, block->priv);

          // transition to shared and record this first read time
          block->state = StateShared;
          block->newShareList(node, block->producer);
          //consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
        }
      }
    } else {
      DBG_(Verb, ( << "no existing block state" ) );

      ++statColdReads;

      // no current state for this block
      block = new BlockState(node);
      myStateTable.addEntry(addr, block);

      // record this first read time
      //consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
    }

    DBG_(Verb, ( << "done read" ) );
    return pending;
  }

  int32_t recvWrite(int32_t node, MemoryAddress addr, MemoryAddress pc, bool priv, uint32_t opcode) {
    DBG_(Verb, ( << "got write from node " << node << " for addr " << addr ) );
    int32_t pending = 0;

    ++statWrites;
    if (priv) ++statOSWrites;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if (block) {
      DBG_(Verb, ( << "retrieved block state" ) );

      if (block->state == StateShared) {
        DBG_(Verb, ( << "state shared" ) );

        pending = doInvalidate(addr, block->sharers, node);

        // transition the block to exclusive
        block->state = StateExclusive;
        block->producer = node;
        block->time = currentTime();
        block->pc = pc;
        block->priv = priv;
      } else { // StateExclusive
        DBG_(Verb, ( << "state exclusive" ) );

        if (node == block->producer) {
          DBG_(Verb, ( << "same producer" ) );

          // same node - just update production time
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        } else {
          DBG_(Verb, ( << "new producer" ) );

          pending = doInvalidate(addr, block->producer);

          // different node - record the last production for that node
          //production(block->producer, addr, block->pc, block->time, block->priv);

          // transition to new producer
          block->producer = node;
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        }
      }
    } else {
      DBG_(Verb, ( << "no existing block state" ) );

      ++statColdWrites;

      // no current state for this block
      block = new BlockState(node, currentTime(), pc, priv);
      myStateTable.addEntry(addr, block);
    }

    DBG_(Verb, ( << "done write" ) );
    return pending;
  }

  int32_t recvUpgrade(int32_t node, MemoryAddress addr, MemoryAddress pc, bool priv, uint32_t opcode) {
    DBG_(Verb, ( << "got upgrade from node " << node << " for addr " << addr ) );
    int32_t pending = 0;

    ++statUpgrades;
    if (priv) ++statOSUpgrades;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if (block) {
      DBG_(Verb, ( << "retrieved block state" ) );

      if (block->state == StateShared) {
        DBG_(Verb, ( << "state shared" ) );

        pending = doInvalidate(addr, block->sharers, node);

        // transition the block to exclusive
        block->state = StateExclusive;
        block->producer = node;
        block->time = currentTime();
        block->pc = pc;
        block->priv = priv;
      } else { // StateExclusive
        DBG_(Verb, ( << "state exclusive" ) );

        if (node == block->producer) {
          DBG_(Verb, ( << "same producer" ) );

          // same node - just update production time
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        } else {
          DBG_(Verb, ( << "new producer" ) );

          pending = doInvalidate(addr, block->producer);

          // different node - record the last production for that node
          //production(block->producer, addr, block->pc, block->time, block->priv);

          // transition to new producer
          block->producer = node;
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        }
      }
    } else {
      DBG_(Verb, ( << "no existing block state" ) );

      ++statColdUpgrades;

      // no current state for this block
      block = new BlockState(node, currentTime(), pc, priv);
      myStateTable.addEntry(addr, block);
    }

    DBG_(Verb, ( << "done upgrade" ) );
    return pending;
  }

  //--- Functions for trace generation -----------------------------------------
  void flushFiles() {
    if (cfg.TraceEnabled) {
      DBG_(Dev, Comp(*this) ( << "Switching all output files" ) );
      OffchipReads.flush();
      OffchipWrites.flush();
      OffchipUpgrades.flush();
      CohReads.flush();
      CohWrites.flush();
      CohUpgrades.flush();
    }
  }

  //--- Utility functions ------------------------------------------------------
  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(cfg.BlockSize - 1) );
  }

  Time currentTime() {
    return theFlexus->cycleCount();
  }

};

}//End namespace nCacheTraceMemory

FLEXUS_COMPONENT_INSTANTIATOR( CacheTraceMemory, nCacheTraceMemory );
FLEXUS_PORT_ARRAY_WIDTH( CacheTraceMemory, ToNode ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CacheTraceMemory, FromNode_Snoop ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( CacheTraceMemory, FromNode_Req ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CacheTraceMemory

#define DBG_Reset
#include DBG_Control()
