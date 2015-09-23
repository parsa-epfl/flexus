#include <components/LocalEngine/LocalEngine.hpp>

#define FLEXUS_BEGIN_COMPONENT LocalEngine
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <vector>
#include <list>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/Common/Slices/DirectoryMessage.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>
#include <components/Common/Slices/Mux.hpp>
#include <components/Common/MemoryMap.hpp>

#define DBG_DefineCategories LocalEngine
#define DBG_SetDefaultOps AddCat(LocalEngine)
#include DBG_Control()

namespace nLocalEngine {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

enum LocalEngineState {
  StateWaitLock,
  StateWaitDir,
  StateLocalWaitMem,
  StateWaitSnoop,
  StateWaitProbe,
  StateDropProbe,
  StateWaitWakeup
};

const char * LocalEngineStateStr[] = {
  "WaitLock",
  "WaitDir",
  "LocalWaitMem",
  "WaitSnoop",
  "WaitProbe",
  "DropSnoop",
  "WaitWakeup"
};

// this struct contains the elements for entries in the transaction list
template<class Transport, class MemoryAddress>
struct T_TxcnEntry : public boost::counted_base {
  T_TxcnEntry(LocalEngineState aState, Transport aTransport)
    : state(aState)
    , trans(aTransport)
    , disallowModify(false)
    , withData(false)
  { }
  MemoryAddress getAddress() {
    return trans[SharedTypes::MemoryMessageTag]->address();
  }
  LocalEngineState state;
  Transport trans;
  bool disallowModify;
  bool withData;
};

// the transaction list is currently implemented as a simple vector
template<class MemoryAddress, class EntryType>
class TransList {
  typedef intrusive_ptr<EntryType> EntryType_p;
  std::vector<EntryType_p> theList;

public:
  bool empty() const {
    return theList.empty();
  }
  void add(EntryType_p newEntry) {
    theList.push_back(newEntry);
  }
  void remove(const EntryType_p toRemove) {
    typename std::vector<EntryType_p>::iterator entries;
    for (entries = theList.begin(); entries != theList.end(); entries++) {
      if ( (*entries) == toRemove ) {
        theList.erase(entries);
        return;
      }
    }
    DBG_Assert(false);
  }
  EntryType_p findAndRemove(const MemoryAddress & anAddress) {
    return find(anAddress, true);
  }
  EntryType_p find(const MemoryAddress & anAddress, bool removeEntry = false) {
    EntryType_p ret = nullptr;
    typename std::vector<EntryType_p>::iterator entries;
    for (entries = theList.begin(); entries != theList.end(); entries++) {
      if ( anAddress == (*entries)->getAddress() ) {
        // found the entry
        ret = *entries;
        if (removeEntry) {
          theList.erase(entries);
        }
        break;
      }
    }
    return ret;
  }
  std::list<EntryType_p> * findAll(const MemoryAddress & anAddress) {
    std::list<EntryType_p> * ret = new std::list<EntryType_p>;
    typename std::vector<EntryType_p>::iterator entries;
    for (entries = theList.begin(); entries != theList.end(); entries++) {
      if ( anAddress == (*entries)->getAddress() ) {
        // found a matching entry - add it to the list
        ret->push_back(*entries);
      }
    }
    return ret;
  }
};  // end class TransList

class FLEXUS_COMPONENT(LocalEngine) {
  FLEXUS_COMPONENT_IMPL(LocalEngine);

  boost::intrusive_ptr<SharedTypes::MemoryMap> theMemoryMap;
  SharedTypes::node_id_t theNodeId;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(LocalEngine)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return txcnList.empty();
  }

  // Initialization
  void initialize() {
    theNodeId = flexusIndex();
    theMemoryMap = SharedTypes::MemoryMap::getMemoryMap(flexusIndex());
  }

  bool isLocal(SharedTypes::PhysicalMemoryAddress anAddress) {
    return theNodeId == theMemoryMap->node(anAddress);
  }

  // Ports

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCache_Request);
  void push( interface::FromCache_Request const &, MemoryTransport & transport) {
    if (transport[MemoryMessageTag]->type() == MemoryMessage::FetchReq) {
      transport[MemoryMessageTag]->type() = MemoryMessage::ReadReq;
    }

    if (isLocal(transport[MemoryMessageTag]->address())) {
      DBG_( Iface, ( << "FromCache (local): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // initiate a directory request so we can determine if the
      // request needs to be forwarded to the home engine
      fromCacheLocal(transport);
    } else {
      DBG_( Iface, ( << "FromCache (remote): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // complex logic is required for evicts/flushes/snoops/etc
      fromCacheRemote(transport);
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCache_Prefetch);
  void push( interface::FromCache_Prefetch const &, MemoryTransport & transport) {
    if (transport[MemoryMessageTag]->type() == MemoryMessage::FetchReq) {
      transport[MemoryMessageTag]->type() = MemoryMessage::ReadReq;
    }

    if (isLocal(transport[MemoryMessageTag]->address())) {
      DBG_( Iface, ( << "FromCache (local): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // initiate a directory request so we can determine if the
      // request needs to be forwarded to the home engine
      fromCacheLocal(transport);
    } else {
      DBG_( Iface, ( << "FromCache (remote): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // complex logic is required for evicts/flushes/snoops/etc
      fromCacheRemote(transport);
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCache_Snoop );
  void push( interface::FromCache_Snoop const &, MemoryTransport & transport) {
    if (isLocal(transport[MemoryMessageTag]->address())) {
      DBG_( Iface, ( << "FromCache (local): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // initiate a directory request so we can determine if the
      // request needs to be forwarded to the home engine
      fromCacheLocal(transport);
    } else {
      DBG_( Iface, ( << "FromCache (remote): " << *transport[MemoryMessageTag] )
            Comp(*this) );

      // complex logic is required for evicts/flushes/snoops/etc
      fromCacheRemote(transport);
    }
  }

  bool available( interface::FromMemory const &) {
    return FLEXUS_CHANNEL(ToCache).available();
  }
  void push( interface::FromMemory const &, MemoryTransport & transport) {
    if (isLocal(transport[MemoryMessageTag]->address())) {
      fromMemory(transport);
    } else {
      throw FlexusException("Memory should not talk about remote addresses");
    }
  }

  bool available( interface::FromProtEngines const &) {
    return FLEXUS_CHANNEL(ToCache).available();
  }
  void push( interface::FromProtEngines const &, MemoryTransport & transport) {
    DBG_( Iface, ( << "FromProtEngines (loc|rem): " << *transport[MemoryMessageTag]
                   <<  " -> ToCache"
                 ) Comp(*this)
        );

    // track remote blocks better since they have no locking mechanism
    if ( !isLocal(transport[MemoryMessageTag]->address()) ) {
      fromRemoteEngine(transport);
    }

    // regardless of whether this address is local or remote, the
    // message needs to be passed to the cache
    DBG_Assert( FLEXUS_CHANNEL(ToCache).available() );
    FLEXUS_CHANNEL(ToCache) << transport;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromDirectory);
  void push( interface::FromDirectory const &, DirectoryTransport & transport) {
    if (isLocal(transport[DirectoryMessageTag]->addr)) {
      fromDirectory(transport);
    } else {
      throw FlexusException("Directory should not talk about remote addresses");
    }
  }

private:
  // the data types and structure for saving transactions in progress
  typedef T_TxcnEntry<MemoryTransport, MemoryAddress> TxcnEntry;
  typedef intrusive_ptr<TxcnEntry> TxcnEntry_p;
  typedef TransList<MemoryAddress, TxcnEntry> TxcnList_t;
  TxcnList_t txcnList;

  // discards common to both local and remote addresses
  bool fromCacheCommon(MemoryTransport & trans) {
    if (trans[TransactionTrackerTag]) {
      trans[TransactionTrackerTag]->setDelayCause(name(), "Processing");
    }

    // we can simply ignore DropHints - no response is required
    if (trans[MemoryMessageTag]->type() == MemoryMessage::EvictClean ||
        trans[MemoryMessageTag]->type() == MemoryMessage::EvictWritable ) {
      DBG_(VVerb, ( << " Clean/writable eviction discarded" ) Comp(*this) );
      return true;
    }
    if (trans[MemoryMessageTag]->type() == MemoryMessage::EvictWritable ) {
      FLEXUS_CHANNEL(WritePermissionLost) << trans[MemoryMessageTag]->address();
      DBG_(VVerb, ( << " writable eviction discarded" ) Comp(*this) );
      return true;
    }

    // a FlushReq can simply be discarded (if it had real data to
    // commit, it would be a Flush, which is handled below)
    if (trans[MemoryMessageTag]->type() == MemoryMessage::FlushReq) {
      DBG_(VVerb, ( << " FlushReq discarded" ) Comp(*this) );
      return true;
    }

    // we can also discard prefetch inserts, since there is no cache
    // to allocate in
    if (trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchInsert ||
        trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchInsertWritable ) {
      DBG_(VVerb, ( << " PrefetchInsert/writable discarded" ) Comp(*this) );
      return true;
    }

    return false;
  }

  // handle a message from the cpu/cache for a local address
  void fromCacheLocal(MemoryTransport & trans) {
    if (fromCacheCommon(trans)) {
      return;
    }

    // if this is the completion of an invalidate or a downgrade, handle it
    // as a special case
    if ( (trans[MemoryMessageTag]->type() == MemoryMessage::InvalidateAck) ||
         (trans[MemoryMessageTag]->type() == MemoryMessage::InvUpdateAck) ||
         (trans[MemoryMessageTag]->type() == MemoryMessage::DowngradeAck) ||
         (trans[MemoryMessageTag]->type() == MemoryMessage::DownUpdateAck) ) {

      FLEXUS_CHANNEL(WritePermissionLost) << trans[MemoryMessageTag]->address();
      // take care of the side effects at the local engine
      handleSnoop(trans);

      // then send this message on to the protocol engine
      DBG_(VVerb, ( << " Snoop handled and passed to protocol engines" ) Comp(*this) );
      FLEXUS_CHANNEL(ToProtEngines) << trans;
      return;
    }

    DBG_(Iface, ( << " -> ToDir: lock request" ) Comp(*this) );

    // other requests require a lock - first create a new transaction
    txcnList.add( new TxcnEntry(StateWaitLock, trans) );

    if (trans[TransactionTrackerTag]) {
      trans[TransactionTrackerTag]->setDelayCause(name(), "Lock Acquire");
    }

    // now lock the cache line
    MemoryAddress addr( trans[MemoryMessageTag]->address() );
    intrusive_ptr<DirectoryMessage> msg ( new DirectoryMessage(DirectoryCommand::Lock, addr) );
    DirectoryTransport dirTransport;
    dirTransport.set(DirectoryMessageTag, msg);
    FLEXUS_CHANNEL(ToDirectory) << dirTransport;
  }  // fromCacheLocal()

  // handle a message from the cpu/cache for a remote address
  void fromCacheRemote(MemoryTransport & trans) {
    if (fromCacheCommon(trans)) {
      return;
    }

    // if this is a probe reply, it is a special case
    if ( trans[MemoryMessageTag]->isProbeType() ) {
      handleProbeReply(trans);
      return;
    }

    // if this is the completion of an invalidate or a downgrade, handle it
    // as a special case
    if ( trans[MemoryMessageTag]->isSnoopType() ) {
      // take care of the side effects at the local engine
      handleSnoop(trans);

      // then send this message on to the protocol engine
      DBG_(VVerb, ( << " Snoop handled and passed to protocol engines" ) Comp(*this) );
      FLEXUS_CHANNEL(ToProtEngines) << trans;
      return;
    }

    MemoryAddress addr( trans[MemoryMessageTag]->address() );
    intrusive_ptr<TxcnEntry> entry( txcnList.find(addr) );

    // now consider evictions and flushes
    if ( (trans[MemoryMessageTag]->type() == MemoryMessage::EvictDirty) ||
         (trans[MemoryMessageTag]->type() == MemoryMessage::Flush) ) {
      // check for an outstanding snoop transaction
      FLEXUS_CHANNEL(WritePermissionLost) << addr;

      entry = findSnoopTxcn(addr);
      if (entry) {
        // found one - merge our data into this existing transaction,
        // then we can discard the message
        // TODO: copy data to txcnEntry
        entry->withData = true;
        DBG_(VVerb, ( << " Evict/flush dropped because of outstanding snoop" ) Comp(*this) );
      } else {
        DBG_(VVerb, ( << " Evict/flush passed to protocol engines" ) Comp(*this) );
        FLEXUS_CHANNEL(ToProtEngines) << trans;

        /*********************************
        if(trans[MemoryMessageTag]->type() == MemoryMessage::Flush) {
          // no snoop, but there could be a probe outstanding
          if(entry) {
            DBG_Assert(entry->state == StateWaitProbe);
            // pass this Flush to the protocol engines - since the flush
            // occured before the probe, the probe will see that this
            // block is not dirty anywhere in the hierarchy... thus the
            // eviction will be sent to the PE unless we tell it not to
            entry->state = StateDropProbe;
            DBG_(VVerb, ( << " Flush passed to prot engines despite probe" ) Comp(*this) );
            FLEXUS_CHANNEL(ToProtEngines) << trans;
          }
          else {
            // this is the simple case - no conflicting/racing transactions
            DBG_(VVerb, ( << " Flush passed to protocol engines" ) Comp(*this) );
            FLEXUS_CHANNEL(ToProtEngines) << trans;
          }
        }
        else {  // MemoryMessage::EvictDirty
          // there is no snoop outstanding, and an eviction is incoming,
          // so there should not be any matching entries... allocate one
          // and send up a probe
          DBG_Assert(!entry);
          txcnList.add( new TxcnEntry(StateWaitProbe,trans) );

          if (trans[TransactionTrackerTag]) {
            trans[TransactionTrackerTag]->setDelayCause(name(), "Probe");
          }

          intrusive_ptr<MemoryMessage> msg ( new MemoryMessage(MemoryMessage::Probe,addr) );
          MemoryTransport memTrans;
          memTrans.set(MemoryMessageTag, msg);
          DBG_(VVerb, ( << " Probe sent to cpu/cache" ) Comp(*this) );
          FLEXUS_CHANNEL(ToCache) << memTrans;
        }
        ************************************/

      }

      return;
    }  // end if Flush||Evict

    // other requests can be passed along (regardless of snoops) unless
    // there is a probe (for which it must wait, so that memory messages
    // arrive at the directory in an order that makes sense)
    bool passRequest = true;
    if (entry) {
      if (entry->state == StateWaitProbe) {
        // the incoming request must be queued
        txcnList.add( new TxcnEntry(StateWaitWakeup, trans) );

        if (entry->trans[TransactionTrackerTag]) {
          entry->trans[TransactionTrackerTag]->setDelayCause(name(), "Probe Conflict");
        }

        DBG_(VVerb, ( << " Queuing request due to outstanding probe" ) Comp(*this) );
        passRequest = false;
      } else {
        DBG_(VVerb, ( << " Passing request despite txcn["
                      << LocalEngineStateStr[entry->state]
                      << "]: " << *(entry->trans[SharedTypes::MemoryMessageTag])
                    ) Comp(*this) );
      }
    }

    if (passRequest) {

      // pass to the remote engine
      DBG_( Iface, ( <<  " -> ToProtEngines" ) Comp(*this) );
      FLEXUS_CHANNEL(ToProtEngines) << trans;
    }
  }  // fromCacheRemote()

  // handle a response from the protocol engines (for a remote address)
  void fromRemoteEngine(MemoryTransport & trans) {
    // we only care about snoops here - everything will be passed
    // to the cache/cpu regardless
    if (trans[MemoryMessageTag]->isRequest()) {
      DBG_Assert( (trans[MemoryMessageTag]->type() == MemoryMessage::Invalidate) ||
                  (trans[MemoryMessageTag]->type() == MemoryMessage::Downgrade) );

      // look an outstanding probe (there can only be one)
      MemoryAddress addr( trans[MemoryMessageTag]->address() );
      intrusive_ptr<TxcnEntry> entry( findProbeTxcn(addr) );
      if (entry) {
        // check that it's WaitProbe and not already DropProbe
        if (entry->state == StateWaitProbe) {
          // extract the data from the eviction and move it the
          // transaction entry we will create for this snoop
          // TODO: move data
          entry->state = StateDropProbe;
        }
      }

      // make a record this now outstanding snoop
      txcnList.add( new TxcnEntry(StateWaitSnoop, trans) );

      if (trans[TransactionTrackerTag]) {
        trans[TransactionTrackerTag]->setDelayCause(name(), "Snoop");
      }

    }
  }

  void setTransactionType(MemoryMessage::MemoryMessageType aType, boost::intrusive_ptr<DirectoryEntry> aDirEntry, boost::intrusive_ptr<TransactionTracker> aTracker, PhysicalMemoryAddress address) {
    if (! aTracker) {
      return;
    }

    switch (aDirEntry->state()) {
      case DIR_STATE_INVALID:
        aTracker->setPreviousState(eInvalid);
        break;
      case DIR_STATE_SHARED:
        aTracker->setPreviousState(eShared);
        break;
      case DIR_STATE_MODIFIED:
        aTracker->setPreviousState(eModified);
        break;
    }

    //Determine if the transaction is a read or a write
    switch (aType) {
      case MemoryMessage::ReadReq:
      case MemoryMessage::PrefetchReadAllocReq:
      case MemoryMessage::PrefetchReadNoAllocReq:
        if (aDirEntry->wasModified() && (! aDirEntry->isSharer(flexusIndex())) )  {
          aTracker->setFillType(eCoherence);
        } else if (! aDirEntry->wasSharer(flexusIndex()) ) {
          aTracker->setFillType(eCold);
        } else {
          aTracker->setFillType(eReplacement);
        }
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::UpgradeReq:
        if (! aDirEntry->wasModified() ) {
          aTracker->setFillType(eCold);
        } else  {
          aTracker->setFillType(eReplacement);
        }
        break;

      default:
        //Don't mark the transaction type
        break;
    }
  }

  // handle a response from the directory (must be for a local address)
  void fromDirectory(DirectoryTransport trans) {
    DBG_(Iface, ( << "FromDir (local): " << *trans[DirectoryMessageTag] ) Comp(*this) );

    // find the corresponding entry from the transaction list
    MemoryAddress addr(trans[DirectoryMessageTag]->addr);
    intrusive_ptr<TxcnEntry> entry( txcnList.find(addr) );
    DBG_Assert(entry, Comp(*this));
    DBG_(VVerb, Comp(*this) ( << " entry->state=" << LocalEngineStateStr[entry->state] ) );

    // base our actions on the current state
    switch (entry->state) {
      case StateWaitLock:
        DBG_Assert(trans[DirectoryMessageTag]->op == DirectoryCommand::Acquired, Comp(*this));
        DBG_(Iface, Comp(*this) ( << "  -> ToDir: get dir entry") );

        // wait for the next response
        entry->state = StateWaitDir;

        if (entry->trans[TransactionTrackerTag]) {
          entry->trans[TransactionTrackerTag]->setDelayCause(name(), "Dir Read");
        }

        // now request the directory entry - reuse the transport
        trans[DirectoryMessageTag]->op = DirectoryCommand::Get;
        FLEXUS_CHANNEL(ToDirectory) << trans;

        break;

      case StateWaitDir:
        DBG_Assert(trans[DirectoryMessageTag]->op == DirectoryCommand::Found, Comp(*this));

        // this request  can be handled here in the local engine if the line
        // state is invalid, or if the state is shared and the request is a read
        if (trans[DirectoryEntryTag]->state() == DIR_STATE_INVALID ||
            (trans[DirectoryEntryTag]->state() == DIR_STATE_SHARED &&
             ( (entry->trans[MemoryMessageTag]->type() == MemoryMessage::ReadReq) ||
               (entry->trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchReadAllocReq) ||
               (entry->trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchReadNoAllocReq) ) ) ) {
          // yes - it's local... send the original request to memory

          DBG_(Iface, Comp(*this) ( << "  -> ToMemory: perform op") );
          entry->state = StateLocalWaitMem;

          if (entry->trans[TransactionTrackerTag]) {
            entry->trans[TransactionTrackerTag]->setDelayCause(name(), "Mem Access");
          }

          // flushes and evictions do not expect responses
          if (entry->trans[MemoryMessageTag]->type() == MemoryMessage::Flush ||
              entry->trans[MemoryMessageTag]->type() == MemoryMessage::EvictDirty) {
            txcnList.remove(entry);

            // unlock the directory
            intrusive_ptr<DirectoryMessage> msg ( new DirectoryMessage(DirectoryCommand::Unlock, addr) );
            DirectoryTransport dir;
            dir.set(DirectoryMessageTag, msg);
            FLEXUS_CHANNEL(ToDirectory) << dir;
          }

          /* CMU-ONLY-BLOCK-BEGIN */
          if (entry->trans[MemoryMessageTag]->type() == MemoryMessage::Flush) {
            // let the SordManager know that a flush has occured
            intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eFlush, theNodeId, entry->trans[MemoryMessageTag]->address()));
            PredictorTransport transport;
            transport.set(PredictorMessageTag, msg);
            transport.set(TransactionTrackerTag, entry->trans[TransactionTrackerTag]);
            FLEXUS_CHANNEL(ToPredictor) << transport;
          }
          /* CMU-ONLY-BLOCK-END */

          // if directory state is shared, remove modifiable permission
          if (trans[DirectoryEntryTag]->state() == DIR_STATE_SHARED) {
            entry->disallowModify = true;
          }

          // update the transaction statistics
          setTransactionType( entry->trans[MemoryMessageTag]->type(), trans[DirectoryEntryTag], entry->trans[TransactionTrackerTag], entry->trans[MemoryMessageTag]->address() );

          // update the sharers, past sharers, and modified flag to include this node (i.e. the home node)
          if (    entry->trans[MemoryMessageTag]->type() == MemoryMessage::ReadReq
                  || entry->trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchReadNoAllocReq
                  || entry->trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchReadAllocReq
                  || entry->trans[MemoryMessageTag]->type() == MemoryMessage::WriteReq
                  || entry->trans[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq
             ) {
            bool changed = false;
            intrusive_ptr<DirectoryEntry> updated( new DirectoryEntry(*trans[DirectoryEntryTag]));

            if ( !(trans[DirectoryEntryTag]->isSharer(flexusIndex())) ) {
              updated->setSharer(flexusIndex());
              changed = true;
            }

            if (       !( trans[DirectoryEntryTag]->wasModified())
                       &&  (    ( entry->trans[MemoryMessageTag]->type() == MemoryMessage::WriteReq   )
                                ||   ( entry->trans[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq )
                           )
               ) {
              updated->markModified();
              changed = true;
            }

            if (changed) {
              DirectoryTransport newDirTrans;
              newDirTrans.set(DirectoryEntryTag, updated);
              intrusive_ptr<DirectoryMessage> dirMsg ( new DirectoryMessage(DirectoryCommand::Set, addr) );
              newDirTrans.set(DirectoryMessageTag, dirMsg);
              FLEXUS_CHANNEL(ToDirectory) << newDirTrans;
            }
          } //End update of sharers/modified/past sharers

          FLEXUS_CHANNEL(ToMemory) << entry->trans;
        } else {
          if ( (entry->trans[MemoryMessageTag]->type() != MemoryMessage::ReadReq) &&
               (entry->trans[MemoryMessageTag]->type() != MemoryMessage::PrefetchReadNoAllocReq) &&
               (entry->trans[MemoryMessageTag]->type() != MemoryMessage::WriteReq) &&
               (entry->trans[MemoryMessageTag]->type() != MemoryMessage::UpgradeReq) ) {
            DBG_(Crit, ( << "dir state:" << *(trans[DirectoryEntryTag])
                         << "  pass to home engine: " << *(entry->trans[MemoryMessageTag])
                       )
                 Comp(*this)
                );
            DBG_Assert(false);
          }
          // can't be handled here... pass the original request along to the
          // home engine, adding the directory information
          DBG_(Iface, Comp(*this) ( << " -> ToProtEngines: perform op" ) );

          MemoryTransport memTrans = entry->trans;
          txcnList.findAndRemove(addr);

          if (memTrans[TransactionTrackerTag]) {
            memTrans[TransactionTrackerTag]->setDelayCause(name(), "Pass to PE");
          }

          memTrans.set(DirectoryEntryTag, trans[DirectoryEntryTag]);
          FLEXUS_CHANNEL(ToProtEngines) << memTrans;
        }
        break;

      default:
        throw FlexusException("Invalid state for a directory response");
    }
  }  // fromDirectory()

  // handle a response from memory (must be for a local address)
  void fromMemory(MemoryTransport & trans) {
    DBG_(Iface, Comp(*this) ( << "FromMemory (local): " << *trans[MemoryMessageTag]
                              << " -> ToCache: mem response"
                              << " -> ToDir: unlock request"
                            ) );

    // find the corresponding entry from the transaction list and remove it
    MemoryAddress addr(trans[MemoryMessageTag]->address());
    intrusive_ptr<TxcnEntry> entry( txcnList.findAndRemove(addr) );
    DBG_Assert(entry);

    if (entry->state == StateLocalWaitMem) {

      // remove modify permission if necessary
      if (entry->disallowModify) {
        if (trans[MemoryMessageTag]->type() == MemoryMessage::MissReplyWritable) {
          trans[MemoryMessageTag]->type() = MemoryMessage::MissReply;
        }
        if (trans[MemoryMessageTag]->type() == MemoryMessage::PrefetchWritableReply) {
          trans[MemoryMessageTag]->type() = MemoryMessage::PrefetchReadReply;
        }
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      switch (entry->trans[MemoryMessageTag]->type()) {
        case MemoryMessage::ReadReq: {
          intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eReadNonPredicted, theNodeId, addr));
          PredictorTransport transport;
          transport.set(PredictorMessageTag, msg);
          transport.set(TransactionTrackerTag, entry->trans[TransactionTrackerTag]);
          FLEXUS_CHANNEL(ToPredictor) << transport;
          break;
        }
        case MemoryMessage::PrefetchReadAllocReq:
        case MemoryMessage::PrefetchReadNoAllocReq: {
          intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eReadPredicted, theNodeId, addr));
          PredictorTransport transport;
          transport.set(PredictorMessageTag, msg);
          transport.set(TransactionTrackerTag, entry->trans[TransactionTrackerTag]);
          FLEXUS_CHANNEL(ToPredictor) << transport;
          break;
        }
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate: {
          intrusive_ptr<PredictorMessage> msg (new PredictorMessage(PredictorMessage::eWrite, theNodeId, addr));
          PredictorTransport transport;
          transport.set(PredictorMessageTag, msg);
          transport.set(TransactionTrackerTag, entry->trans[TransactionTrackerTag]);
          FLEXUS_CHANNEL(ToPredictor) << transport;
          break;
        }
        default:
          break;
      }
      /* CMU-ONLY-BLOCK-END */

      if (trans[TransactionTrackerTag]) {
        trans[TransactionTrackerTag]->setDelayCause(name(), "Pass to Cache");
      }

      // since we passed the original transport to memory, we can pass
      // this transport to the cache without worry that some slices
      // have been dropped
      DBG_Assert( FLEXUS_CHANNEL( ToCache).available() );
      FLEXUS_CHANNEL(ToCache) << trans;
    } else {
      DBG_Assert(false, Comp(*this) ( << "Invalid state for transaction: " << LocalEngineStateStr[entry->state] ) );
    }

    // all done - unlock the directory
    intrusive_ptr<DirectoryMessage> msg ( new DirectoryMessage(DirectoryCommand::Unlock, addr) );
    DirectoryTransport dir;
    dir.set(DirectoryMessageTag, msg);
    FLEXUS_CHANNEL(ToDirectory) << dir;
  }  // fromMemory()

  // take care of the side effects of a snoop operation
  void handleSnoop(MemoryTransport & trans) {
    intrusive_ptr<MemoryMessage> msg( trans[MemoryMessageTag] );

    // look for all pending transactions to the same address
    std::list<TxcnEntry_p> * entries = txcnList.findAll(msg->address());
    std::list<TxcnEntry_p>::iterator iter;
    for (iter = entries->begin(); iter != entries->end(); ++iter) {
      // this indicates a race is in progress, one that the protocol engine won
      // - base our action of the type of waiting message
      switch ((*iter)->trans[MemoryMessageTag]->type()) {
          // NOTE: this should match very closely to the performSnoop() function
          //       in the cache (i.e. CacheController.hpp)

        case MemoryMessage::ReadReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::PrefetchReadAllocReq:
        case MemoryMessage::PrefetchReadNoAllocReq:
          // no changes required for these requests
          break;

        case MemoryMessage::UpgradeReq:
          // if this is an invalidation request, convert this upgrade (no data)
          // to a write (no data)
          if ( (msg->type() == MemoryMessage::InvalidateAck) ||
               (msg->type() == MemoryMessage::InvUpdateAck) ) {
            (*iter)->trans[SharedTypes::MemoryMessageTag]->type() = MemoryMessage::WriteReq;
          }
          break;
        case MemoryMessage::UpgradeAllocate:
          // if this is an invalidation request, convert this upgrade (with data)
          // to a write (with data)
          if ( (msg->type() == MemoryMessage::InvalidateAck) ||
               (msg->type() == MemoryMessage::InvUpdateAck) ) {
            (*iter)->trans[SharedTypes::MemoryMessageTag]->type() = MemoryMessage::WriteAllocate;
          }
          break;

        case MemoryMessage::Flush:
        case MemoryMessage::EvictDirty:
          // these requests can be removed, since the snoop has already performed
          // the necessary actions at each level of the hierarchy, and is
          // responsible for carrying data through to the protocol engine

          // TODO: copy data from txcnEntry to snoop ack message
          // indicate the message contains data
          if (msg->type() == MemoryMessage::InvalidateAck) {
            msg->type() = MemoryMessage::InvUpdateAck;
          }
          if (msg->type() == MemoryMessage::DowngradeAck) {
            msg->type() = MemoryMessage::DownUpdateAck;
          }
          if (trans[SharedTypes::TransactionTrackerTag]) {
            trans[SharedTypes::TransactionTrackerTag]->setPreviousState(eModified);
          }

          txcnList.remove(*iter);
          // squash the associated pending lock request
          {
            intrusive_ptr<DirectoryMessage> dir ( new DirectoryMessage(DirectoryCommand::Squash, msg->address()) );
            DirectoryTransport dirTrans;
            dirTrans.set(DirectoryMessageTag, dir);
            FLEXUS_CHANNEL(ToDirectory) << dirTrans;
          }
          break;

        case MemoryMessage::Invalidate:
          // (only for remote addresses)
          // if the ack corresponds to this request, remove the entry
          if ( (msg->type() == MemoryMessage::InvalidateAck) ||
               (msg->type() == MemoryMessage::InvUpdateAck) ) {
            // if a flush/evict placed data into this transaction entry,
            // copy it to this ack, and mark as containing data
            if ((*iter)->withData) {
              // TODO: copy data
              msg->type() = MemoryMessage::InvUpdateAck;
              if (trans[SharedTypes::TransactionTrackerTag]) {
                trans[SharedTypes::TransactionTrackerTag]->setPreviousState(eModified);
              }
            }
            // bye bye!
            txcnList.remove(*iter);
          }
          break;
        case MemoryMessage::Downgrade:
          // (only for remote addresses)
          // if the ack corresponds to this request, remove the entry
          if ( (msg->type() == MemoryMessage::DowngradeAck) ||
               (msg->type() == MemoryMessage::DownUpdateAck) ) {
            // if a flush/evict placed data into this transaction entry,
            // copy it to this ack, and mark as containing data
            if ((*iter)->withData) {
              // TODO: copy data
              msg->type() = MemoryMessage::DownUpdateAck;
              if (trans[SharedTypes::TransactionTrackerTag]) {
                trans[SharedTypes::TransactionTrackerTag]->setPreviousState(eModified);
              }
            }
            // bye bye!
            txcnList.remove(*iter);
          }
          break;

        default:
          DBG_( Crit, Comp(*this) ( << "Invalid message in TxcnList during snoop: " << *msg ) );
          throw FlexusException();
      }
    }
    delete entries;

  }  // handleSnoop()

  // take care of a probe response
  void handleProbeReply(MemoryTransport & trans) {
    MemoryAddress addr( trans[MemoryMessageTag]->address() );
    intrusive_ptr<TxcnEntry> entry( findProbeTxcn(addr) );
    DBG_Assert(entry);

    if (entry->state == StateDropProbe) {
      // the eviction has been superceeded by a snoop - ignore what the
      // probe response tells us and remove the transaction entry
      txcnList.remove(entry);
    } else {
      DBG_Assert(entry->state == StateWaitProbe);
      switch (trans[MemoryMessageTag]->type()) {
        case MemoryMessage::ProbedDirty:
          // this response indicates that data at least as recent as
          // contained in the eviction are present somewhere in the
          // cache hierarchy - this means we can just drop the eviction
          // and its data
          txcnList.remove(entry);
          break;
        case MemoryMessage::ProbedNotPresent:
          // this response indicates that the data in the eviction are
          // the only up-to-date copy... therefore the original evict
          // must be passed to the protocol engines
          DBG_( Iface, ( <<  " probed: passing Evict to ProtEngines" ) Comp(*this) );
          if (entry->trans[TransactionTrackerTag]) {
            entry->trans[TransactionTrackerTag]->setDelayCause(name(), "Pass to PE");
          }
          FLEXUS_CHANNEL(ToProtEngines) << entry->trans;
          txcnList.remove(entry);
          break;
        case MemoryMessage::ProbedClean:
          // this is an error condition... we cannot pass the eviction
          // along to the protocol engines, since it is still present
          // in the hierarchy above... but we cannot drop the eviction
          // either, since the line in the cache above is clean and
          // thus may be dropped at any time
          DBG_Assert(false);
        default:
          DBG_Assert(false);
      }
    }

    // now wake up requests that were queued because of a probe
    wakeRequests(addr);
  }  // handleProbeReply()

  void wakeRequests(MemoryAddress anAddr) {
    std::list<TxcnEntry_p> * entries = txcnList.findAll(anAddr);
    std::list<TxcnEntry_p>::iterator iter;

    for (iter = entries->begin(); iter != entries->end(); ++iter) {
      if ((*iter)->state == StateWaitWakeup) {
        // found one
        txcnList.remove(*iter);

        if ((*iter)->trans[TransactionTrackerTag]) {
          (*iter)->trans[TransactionTrackerTag]->setDelayCause(name(), "Pass to PE");
        }

        // send it to the remote engine
        DBG_(Iface, ( << "Waking request:" << *((*iter)->trans[MemoryMessageTag]) ) Comp(*this) );
        DBG_( Iface, ( <<  " -> ProtEngines" ) Comp(*this) );
        FLEXUS_CHANNEL(ToProtEngines) << (*iter)->trans;
      }
    }
    delete entries;
  }  // wakeRequests()

  TxcnEntry_p findSnoopTxcn(MemoryAddress anAddr) {
    std::list<TxcnEntry_p> * entries = txcnList.findAll(anAddr);
    std::list<TxcnEntry_p>::iterator iter;

    TxcnEntry_p ret(0);
    for (iter = entries->begin(); iter != entries->end(); ++iter) {
      if ( ((*iter)->trans[MemoryMessageTag]->type() == MemoryMessage::Invalidate) ||
           ((*iter)->trans[MemoryMessageTag]->type() == MemoryMessage::Downgrade) ) {
        // found it
        ret = *iter;
      }
    }
    delete entries;
    return ret;
  }  // findSnoopTxcn()

  TxcnEntry_p findProbeTxcn(MemoryAddress anAddr) {
    std::list<TxcnEntry_p> * entries = txcnList.findAll(anAddr);
    std::list<TxcnEntry_p>::iterator iter;

    TxcnEntry_p ret(0);
    for (iter = entries->begin(); iter != entries->end(); ++iter) {
      if ( ((*iter)->state == StateWaitProbe) || ((*iter)->state == StateDropProbe) ) {
        // found it
        ret = *iter;
      }
    }
    delete entries;
    return ret;
  }  // findProbeTxcn()

};

} //End Namespace nLocalEngine

FLEXUS_COMPONENT_INSTANTIATOR( LocalEngine, nLocalEngine );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT LocalEngine

#define DBG_Reset
#include DBG_Control()
