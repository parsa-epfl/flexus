/*! \file PBController.hpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <fstream>

#include <core/metaprogram.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

#include <boost/throw_exception.hpp>
#include <boost/serialization/list.hpp>

#include <boost/shared_ptr.hpp>

#include <core/stats.hpp>

#include "PBController.hpp"

#include <components/Common/TraceTracker.hpp>

namespace nPrefetchBuffer {

namespace Stat = Flexus::Stat;

class PBControllerImpl : public PBController {

  PB * thePB;
  std::string theName;
  unsigned theNodeId;

  enum ePrefetchFillType {
    Unknown
    , Cold
    , Coherence
    , Replacement
  };

  friend std::ostream & operator << (std::ostream & out, ePrefetchFillType aType) {
    switch ( aType ) {
      case Unknown:
        out << "(unknown)";
        break;
      case Cold:
        out << "(cold)";
        break;
      case Replacement:
        out << "(replacement)";
        break;
      case Coherence:
        out << "(coherence)";
        break;
      default:
        DBG_Assert(false);
    };
    return out;
  }

  struct PBTableEntry {
    MemoryAddress theAddress;
    mutable uint8_t theWritable;
    mutable ePrefetchFillType theFillType;
    mutable tFillLevel theFillLevel;
    PBTableEntry(MemoryAddress const & anAddress, uint8_t aWritable, ePrefetchFillType aFillType, tFillLevel aFillLevel)
      : theAddress(anAddress)
      , theWritable(aWritable)
      , theFillType(aFillType)
      , theFillLevel(aFillLevel)
    {}

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const uint32_t version) {
      ar & theAddress;
      ar & theWritable;
      ar & theFillType;
    }
    PBTableEntry() {} //For serialization
  };

  struct by_order {};
  struct by_address {};

  typedef multi_index_container
  < PBTableEntry
  , indexed_by
  < sequenced < tag<by_order> >
  , ordered_unique
  < tag<by_address>
  , member< PBTableEntry, MemoryAddress, &PBTableEntry::theAddress >
  >
  >
  >
  pb_table_t;

  int32_t theNumEntries;
  pb_table_t thePrefetchBuffer;
  int32_t theWatchEntries;
  pb_table_t theWatchList;
  bool theEvictClean;
  bool theUseStreamFetch;

  //statistics
  Stat::StatCounter statRequestedPrefetches;
  Stat::StatCounter statCompletedPrefetches;
  Stat::StatCounter statCompletedPrefetches_Unknown;
  Stat::StatCounter statCompletedPrefetches_Cold;
  Stat::StatCounter statCompletedPrefetches_Coherence;
  Stat::StatCounter statCompletedPrefetches_Replacement;
  Stat::StatCounter statRedundantPrefetches;
  Stat::StatCounter statRedundantPrefetches_PresentInBuffer;
  Stat::StatCounter statRedundantPrefetches_PrefetchInProgress;
  Stat::StatCounter statRedundantPrefetches_PresentInHigherCache;
  Stat::StatCounter statRedundantPrefetches_PresentInLowerCache;
  Stat::StatCounter statRedundantPrefetches_Cancelled;
  Stat::StatCounter statRejectedPrefetches_NotPresentOnChip;

  Stat::StatCounter statRequestedWatches;
  Stat::StatCounter statWatch_Present;
  Stat::StatCounter statWatch_Redundant;
  Stat::StatCounter statWatch_Requested;
  Stat::StatCounter statWatch_Removed;
  Stat::StatCounter statWatch_Replaced;

  Stat::StatCounter statReadAccesses;
  Stat::StatCounter statHits;
  Stat::StatCounter statHits_Partial;
  Stat::StatCounter statHits_FillType_Unknown;
  Stat::StatCounter statHits_FillType_Cold;
  Stat::StatCounter statHits_FillType_Coherence;
  Stat::StatCounter statHits_FillType_Replacement;
  Stat::StatCounter statHits_FillOnChip;
  Stat::StatCounter statHits_FillOffChip;

  Stat::StatCounter statInvalidatedBlocks;
  Stat::StatCounter statWriteMissBlocks;
  Stat::StatCounter statFlushedBlocks;
  Stat::StatCounter statReplacedBlocks;

  Stat::StatCounter statRequestedDiscards;
  Stat::StatCounter statConflictingTransports;

public:
  void saveState(std::string const & aDirName) const {
    std::string fname( aDirName);
    fname += "/" + theName;
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    std::list< PBTableEntry > pb;
    std::copy( thePrefetchBuffer.begin(), thePrefetchBuffer.end(), std::back_inserter(pb));

    oa << (const std::list<PBTableEntry>)pb;
    // close archive
    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + theName;
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (ifs) {
      boost::archive::binary_iarchive ia(ifs);

      std::list< PBTableEntry > pb;
      ia >> pb;
      std::copy( pb.begin(), pb.end(), std::back_inserter(thePrefetchBuffer));

      // close archive
      ifs.close();
    } else {
      DBG_( Crit, ( << "WARNING: saved state for " << theName << " not found in " << fname) );
    }
  }

  //PBController creation
  PBControllerImpl(PB * aPB, std::string const & aName, unsigned aNodeId, int32_t aNumEntries, int32_t aWatchEntries, bool anEvictClean, bool aUseStreamFetch)
    : thePB(aPB)
    , theName(aName)
    , theNodeId(aNodeId)
    , theNumEntries(aNumEntries)
    , theWatchEntries(aWatchEntries)
    , theEvictClean(anEvictClean)
    , theUseStreamFetch(aUseStreamFetch)

    , statRequestedPrefetches                       (aName + "-Prefetches")
    , statCompletedPrefetches                       (aName + "-Prefetches:Completed")
    , statCompletedPrefetches_Unknown               (aName + "-Prefetches:Completed:Unknown")
    , statCompletedPrefetches_Cold                  (aName + "-Prefetches:Completed:Cold")
    , statCompletedPrefetches_Coherence             (aName + "-Prefetches:Completed:Coherence")
    , statCompletedPrefetches_Replacement           (aName + "-Prefetches:Completed:Replacement")

    , statRedundantPrefetches                       (aName + "-Prefetches:Redundant")
    , statRedundantPrefetches_PresentInBuffer       (aName + "-Prefetches:Redundant:PresentInBuffer")
    , statRedundantPrefetches_PrefetchInProgress    (aName + "-Prefetches:Redundant:AlreadyInProgress")
    , statRedundantPrefetches_PresentInHigherCache  (aName + "-Prefetches:Redundant:PresentInHigherCache")
    , statRedundantPrefetches_PresentInLowerCache   (aName + "-Prefetches:Redundant:PresentInLowerCache")
    , statRedundantPrefetches_Cancelled             (aName + "-Prefetches:Redundant:Cancelled")
    , statRejectedPrefetches_NotPresentOnChip       (aName + "-Prefetches:Rejected:NotPresentOnChip")

    , statRequestedWatches                          (aName + "-Watches")
    , statWatch_Present                             (aName + "-Watches:Present")
    , statWatch_Redundant                           (aName + "-Watches:Redundant")
    , statWatch_Requested                           (aName + "-Watches:Requested")
    , statWatch_Removed                             (aName + "-Watches:Removed")
    , statWatch_Replaced                            (aName + "-Watches:Replaced")

    , statReadAccesses                              (aName + "-ReadAccesses")
    , statHits                                      (aName + "-ReadHits")
    , statHits_Partial                              (aName + "-ReadHits:Partial")

    , statHits_FillType_Unknown                     (aName + "-ReadHits:FillType:Unknown")
    , statHits_FillType_Cold                        (aName + "-ReadHits:FillType:Cold")
    , statHits_FillType_Coherence                   (aName + "-ReadHits:FillType:Coherence")
    , statHits_FillType_Replacement                 (aName + "-ReadHits:FillType:Replacement")
    , statHits_FillOnChip                           (aName + "-ReadHits:FillFrom:OnChip")
    , statHits_FillOffChip                          (aName + "-ReadHits:FillFrom:OffChip")

    , statInvalidatedBlocks                         (aName + "-InvalidatedBlocks")
    , statWriteMissBlocks                           (aName + "-WriteMissBlocks")
    , statFlushedBlocks                             (aName + "-FlushedBlocks")
    , statReplacedBlocks                            (aName + "-ReplacedBlocks")

    , statRequestedDiscards                         (aName + "-RequestedDiscards")
    , statConflictingTransports                     (aName + "-ConflictingTransports")
  { }

private:

  bool drop(MemoryAddress anAddress) {
    DBG_(VVerb, ( << theName << "Dropping address from prefetchBuffer: " << & std::hex << anAddress << & std::dec ) Addr(anAddress) );
    pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(anAddress);
    if (iter != thePrefetchBuffer.get<by_address>().end() ) {
      thePrefetchBuffer.get<by_address>().erase(iter);
      return true;
    } else {
      return false;
    }
  }

  void cancel(MemoryAddress anAddress) {
    bool was_cancelled = thePB->cancel(anAddress);
    if (was_cancelled) {
      DBG_(VVerb, ( << theName << "Cancelling prefetch: " << & std::hex << anAddress << & std::dec ) Addr(anAddress) );
      ++statRedundantPrefetches;
      ++statRedundantPrefetches_Cancelled;
    }
  }

  //Can require a MasterOut buffer
  void add(MemoryAddress anAddress, bool aWritable, ePrefetchFillType aFillType, tFillLevel aFillLevel) {
    DBG_(VVerb,  ( << theName << " Adding block to prefetch buff : " << & std::hex << anAddress << & std::dec << ( aWritable ? " (writeable) " : " " ) << aFillType << " " << aFillLevel ) Addr(anAddress) );

    if (theNumEntries == 0) {
      return;
    }

    while ( thePrefetchBuffer.size() >= static_cast<size_t>(theNumEntries)) {
      ++statReplacedBlocks;
      sendReplaced(thePrefetchBuffer.back().theAddress);
      theTraceTracker.eviction(theNodeId, ePrefetchBuffer, thePrefetchBuffer.back().theAddress, true);
      thePrefetchBuffer.pop_back();
    }

    thePrefetchBuffer.push_front( PBTableEntry( anAddress, aWritable, aFillType, aFillLevel ) );
  }

  void addWatch(MemoryAddress anAddress) {
    DBG_(VVerb,  ( << theName << " Adding block to watch list: " << anAddress ) Addr(anAddress) );

    if (theWatchEntries == 0) {
      return;
    }

    while ( theWatchList.size() >= static_cast<size_t>(theWatchEntries)) {
      sendWatchReplaced(theWatchList.back().theAddress);
      theWatchList.pop_back();
    }

    theWatchList.push_front( PBTableEntry( anAddress, false, Unknown, eUnknown ) );
  }

  void satisfyWatch(MemoryAddress anAddress) {
    pb_table_t::index<by_address>::type::iterator iter = theWatchList.get<by_address>().find(anAddress);
    if (iter != theWatchList.get<by_address>().end() ) {
      DBG_(VVerb, ( << theName << " Satisfying watch on : " << anAddress ) Addr(anAddress) );
      theWatchList.get<by_address>().erase(iter);
      ++statWatch_Requested;
      sendWatchRequested(anAddress);
    }
  }

  void killWatch(MemoryAddress anAddress) {
    pb_table_t::index<by_address>::type::iterator iter = theWatchList.get<by_address>().find(anAddress);
    if (iter != theWatchList.get<by_address>().end() ) {
      DBG_(VVerb, ( << theName << " Stopping watch on : " << anAddress ) Addr(anAddress) );
      theWatchList.get<by_address>().erase(iter);
      ++statWatch_Removed;
      sendWatchRemoved(anAddress);
    }
  }

  void markNonWritable(MemoryAddress anAddress) {
    DBG_(VVerb, ( << theName << "Marking block non-writable: " << & std::hex << anAddress << & std::dec )  Addr(anAddress) );
    pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(anAddress);
    if (iter != thePrefetchBuffer.get<by_address>().end() ) {
      iter->theWritable = false;
    }
  }

  void makePresentReply(boost::intrusive_ptr<MemoryMessage> msg) {
    msg->type() = MemoryMessage::ProbedClean;
  }

  void makeReadReply(boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker, bool aWritable) {
    if (aWritable) {
      msg->type() = MemoryMessage::MissReplyWritable;
    } else {
      msg->type() = MemoryMessage::MissReply;
    }
    if (tracker) {
      tracker->setFillLevel(ePrefetchBuffer);
    }
  }

  void makeRedundantReply(boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker) {
    msg->type() = MemoryMessage::PrefetchReadRedundant;
  }

  void sendInsert(MemoryAddress anAddress, bool aWritable) {
    boost::intrusive_ptr<MemoryMessage> msg;
    if (aWritable) {
      msg = new MemoryMessage(MemoryMessage::PrefetchInsertWritable, anAddress /*, DATA */ );
    } else {
      msg = new MemoryMessage(MemoryMessage::PrefetchInsert, anAddress /*, DATA */ );
    }
    msg->reqSize() = 64; //Fix this properly

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(theNodeId);
    tracker->setOriginatorLevel(ePrefetchBuffer);

    thePB->sendBackSnoop( msg, tracker);
  }

  void sendDownProbe(MemoryAddress anAddress) {
    boost::intrusive_ptr<MemoryMessage> msg = new MemoryMessage(MemoryMessage::DownProbe, anAddress /*, DATA */ );

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(theNodeId);
    tracker->setOriginatorLevel(ePrefetchBuffer);

    thePB->sendBackSnoop( msg, tracker);
  }

  void sendPrefetchReadRequest(MemoryAddress anAddress) {
    boost::intrusive_ptr<MemoryMessage> msg;
    if (theUseStreamFetch) {
      msg = new MemoryMessage(MemoryMessage::StreamFetch, anAddress );
    } else {
      msg = new MemoryMessage(MemoryMessage::PrefetchReadNoAllocReq, anAddress );
    }

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anAddress);
    tracker->setInitiator(theNodeId);
    tracker->setOriginatorLevel(ePrefetchBuffer);

    thePB->sendBackPrefetch( msg, tracker);
  }

  void sendRedundant(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::PrefetchRedundant, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
    theTraceTracker.prefetchRedundant(theNodeId, ePrefetchBuffer, anAddress);
  }

  void sendHit(MemoryAddress anAddress, boost::intrusive_ptr<TransactionTracker> tracker) {

    //theAllHits << std::make_pair( anAddress, 1) ;
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineHit, anAddress );
    thePB->sendMaster(msg, tracker);
  }

  void sendPartialHit(MemoryAddress anAddress, boost::intrusive_ptr<TransactionTracker> tracker) {
    DBG_( Iface, ( << theName << " PB PART-HIT " << anAddress ) );

    //theAllHits << std::make_pair( anAddress, 1) ;
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineHit_Partial, anAddress );
    thePB->sendMaster(msg, tracker);
  }

  void sendReplaced(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineReplaced, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);

    if (theEvictClean) {
      boost::intrusive_ptr<MemoryMessage> msg;
      if (theUseStreamFetch) {
        msg = new MemoryMessage(MemoryMessage::EvictClean, anAddress );
      } else {
        msg = new MemoryMessage(MemoryMessage::SVBClean, anAddress );
      }
      thePB->sendBackSnoop( msg, tracker );
    }
  }

  void sendRemoved(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::LineRemoved, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendComplete(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::PrefetchComplete, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendWatchRedundant(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::WatchRedundant, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendWatchReplaced(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::WatchReplaced, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendWatchRemoved(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::WatchRemoved, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendWatchPresent(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::WatchPresent, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void sendWatchRequested(MemoryAddress anAddress) {
    boost::intrusive_ptr<PrefetchMessage> msg = new PrefetchMessage(PrefetchMessage::WatchRequested, anAddress );
    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    thePB->sendMaster(msg, tracker);
  }

  void processWatchProbe(MemoryAddress anAddress, MemoryMessage::MemoryMessageType aProbeReply) {
    pb_table_t::index<by_address>::type::iterator iter = theWatchList.get<by_address>().find(anAddress);
    if (iter == theWatchList.get<by_address>().end() ) {
      //This watch has already been fulfilled somehow
    } else {
      switch ( aProbeReply ) {
        case MemoryMessage::ProbedNotPresent:
          //Probe down
          sendDownProbe(anAddress);
          break;

        case MemoryMessage::DownProbeNotPresent:
          //No action, we leave the message in the watch list until it is
          //replaced or removed
          break;

        case MemoryMessage::ProbedClean:
        case MemoryMessage::ProbedWritable:
        case MemoryMessage::ProbedDirty:
        case MemoryMessage::DownProbePresent:
          sendWatchPresent( anAddress );
          theWatchList.get<by_address>().erase(iter);
          break;
        default:
          DBG_Assert(false);
          break;
      }
    }

  }

  //Requires a front message buffer
  void sendProbe(MemoryAddress anAddress, boost::intrusive_ptr<TransactionTracker> tracker) {
    boost::intrusive_ptr<MemoryMessage> msg = new MemoryMessage(MemoryMessage::Probe, anAddress );
    thePB->sendFront( msg, tracker );
  }

  eAction processProbeReply(boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker) {
    eProbeMAFResult result = thePB->probeMAF( msg->address() );

    if (result == kNoMatch) {
      DBG_(VVerb, ( << theName << "Passing non-matching probe result to back-side: " << *msg ) Addr(msg->address()) );
      return kSendToBackSnoop;
    } else {
      if (result == kWatch ) {
        //See if this watch is still in the watch list and take appropriate
        //action
        processWatchProbe(msg->address(), msg->type());

        //We remove the MAF entry in any case
        return kRemoveAndWakeMAF;
      } else {
        //Original logic for prefetches
        if (msg->type() == MemoryMessage::ProbedNotPresent && result != kCancelled ) {
          sendPrefetchReadRequest(msg->address());
          return kNoAction;
        } else {
          if (result != kCancelled) {
            ++statRedundantPrefetches;
            ++statRedundantPrefetches_PresentInHigherCache;
          }
          sendRedundant(msg->address());
          return kRemoveAndWakeMAF;
        }
      }
    }
  }

public:

  //Can require Front, BackRequest, BackSnoop, master, MAF Entry
  eAction processRequestMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker, bool hasMAFConflict, bool waited ) {
    DBG_(Iface, ( << theName << "Processing MemoryMessage(Request): " << *msg ) Addr(msg->address()) );
    DBG_Assert( msg );
    DBG_Assert( (msg->address() & 63) == 0, ( << theName << " received unaligned request: " << *msg) );

    switch (msg->type() ) {
      case MemoryMessage::StreamFetch:
        if ( hasMAFConflict ) {
          makeRedundantReply( msg, tracker);
          return kSendToFront;
        } else {
          ++statReadAccesses;
          pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(msg->address());
          if (iter != thePrefetchBuffer.get<by_address>().end() ) {
            makeRedundantReply( msg, tracker);
            return kSendToFront;
          } else {
            return kSendToBackRequest;
          }
        }
        break;

      case MemoryMessage::FetchReq:
      case MemoryMessage::ReadReq:
        satisfyWatch(msg->address());
        if ( hasMAFConflict ) {
          ++statConflictingTransports;
          return kBlockOnAddress;
        } else {
          ++statReadAccesses;
          pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(msg->address());
          if (iter != thePrefetchBuffer.get<by_address>().end() ) {
            if (waited) {
              ++statHits_Partial;
            }
            ++statHits;

            //See what the miss would have been based on the miss type of the
            //prefetch
            switch (iter->theFillType) {
              case Unknown:
                ++statHits_FillType_Unknown;
                break;
              case Cold:
                ++statHits_FillType_Cold;
                break;
              case Replacement:
                ++statHits_FillType_Replacement;
                break;
              case Coherence:
                ++statHits_FillType_Coherence;
                break;
              default:
                DBG_Assert(false);
            }
            switch (iter->theFillLevel) {
              case eRemoteMem:
              case eLocalMem:
                ++statHits_FillOffChip;
                break;
              default:
                ++statHits_FillOnChip;
                break;
            }
            theTraceTracker.prefetchHit(theNodeId, ePrefetchBuffer, msg->address(), false);
            if (waited) {
              sendPartialHit(msg->address(), tracker);
            } else {
              sendHit(msg->address(), tracker);
            }
            //Don't send inserts in the CMP
            //sendInsert(msg->address(), iter->theWritable);
            drop(msg->address());
            makeReadReply( msg, tracker, iter->theWritable);
            return kSendToFront;
          } else {
            return kSendToBackRequest;
          }
        }
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::NonAllocatingStoreReq:
        killWatch(msg->address());
        if ( hasMAFConflict ) {
          ++statConflictingTransports;
          return kBlockOnAddress;
        } else {
          if ( drop(msg->address()) ) {
            theTraceTracker.prefetchHit(theNodeId, ePrefetchBuffer, msg->address(), true);
            ++statWriteMissBlocks;
            sendRemoved(msg->address());
          }
          return kSendToBackRequest;
        }

      case MemoryMessage::PrefetchReadNoAllocReq:
        DBG_Assert( false, ( << "Prefetch buffer received requests from another prefetch buffer.  This is not supported.") );
        break;

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << msg->type()));
        break;
    }
    return kNoAction;

  }

  //Can require BackSnoop, MasterOut, BackRequest, BackPrefetch
  eAction processSnoopMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) {
    DBG_(Iface, ( << theName << "Processing MemoryMessage(Snoop): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0 );

    switch (msg->type() ) {

      case MemoryMessage::FlushReq:
        cancel(msg->address());
      case MemoryMessage::SVBClean:
      case MemoryMessage::EvictClean:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DowngradeAck:
        killWatch(msg->address());
        return kSendToBackSnoop;

      case MemoryMessage::Flush:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::DownUpdateAck:
        killWatch(msg->address());
        cancel(msg->address());
        if ( drop(msg->address()) ) {
          ++statFlushedBlocks;
          sendRemoved(msg->address());
        }
        return kSendToBackSnoop;

      case MemoryMessage::PrefetchInsert:
        DBG_Assert( false, ( << "Prefetch buffer received requests from another prefetch buffer.  This is not supported.") );
        break;

      case MemoryMessage::ReturnReply:
        return kSendToBackSnoop;

      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedWritable:
      case MemoryMessage::ProbedDirty:
        return processProbeReply(msg, tracker);

      case MemoryMessage::ProbedNotPresent: {
        eAction act = processProbeReply(msg, tracker);
        if ( act == kSendToBackSnoop) {
          //processProbeReply indicates we should pass the probe
          //result down the heirarchy
          pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(msg->address());
          if (iter != thePrefetchBuffer.get<by_address>().end() ) {
            makePresentReply(msg);
          }
        }
        return act;
      }

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg ));
        break;
    }
    return kNoAction;

  }

  //Can require 2 x MasterOut, Front
  eAction processBackMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) {
    DBG_(Iface, ( << theName << " Processing MemoryMessage(Back): " << *msg ) Addr(msg->address()) );
    DBG_Assert( (msg->address() & 63) == 0 );

    switch (msg->type() ) {
      case MemoryMessage::Invalidate:
        cancel(msg->address());
        if ( drop(msg->address()) ) {
          sendRemoved(msg->address());
          ++statInvalidatedBlocks;
          theTraceTracker.invalidation(theNodeId, ePrefetchBuffer, msg->address());
        }
        return kSendToFront;

      case MemoryMessage::ReturnReq:
        return kSendToFront;

      case MemoryMessage::Downgrade:
        cancel(msg->address());
        markNonWritable(msg->address());
        return kSendToFront;

      case MemoryMessage::Probe:
        return kSendToFront;

      case MemoryMessage::FetchReply:
      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyDirty:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::UpgradeReply:
      case MemoryMessage::NonAllocatingStoreReply:
        DBG_(Iface, ( << theName << " Before cancel") );
        cancel(msg->address());
        satisfyWatch(msg->address());
        DBG_Assert( thePrefetchBuffer.get<by_address>().find(msg->address()) == thePrefetchBuffer.get<by_address>().end(), ( << "Observed a reply message for a line that is in the buffer: " << *msg  ));
        DBG_(Iface, ( << theName << " returning kSendToFront") );
        return kSendToFrontAndClearMAF;

      case MemoryMessage::StreamFetchWritableReply:
        if (! theUseStreamFetch) {
          DBG_(Iface, ( << theName << " Before cancel") );
          cancel(msg->address());
          satisfyWatch(msg->address());
          DBG_Assert( thePrefetchBuffer.get<by_address>().find(msg->address()) == thePrefetchBuffer.get<by_address>().end(), ( << "Observed a reply message for a line that is in the buffer: " << *msg  ));
          DBG_(Iface, ( << theName << " returning kSendToFront") );
          return kSendToFrontAndClearMAF;
        }
        //intentional fall-through
      case MemoryMessage::PrefetchReadReply:
      case MemoryMessage::PrefetchWritableReply: {
        ++statCompletedPrefetches;
        sendComplete(msg->address());
        ePrefetchFillType fill_type = Unknown;
        if (tracker->fillType()) {
          switch ( * tracker->fillType()) {
            case eCold:
              fill_type = Cold;
              ++statCompletedPrefetches_Cold;
              break;
            case eReplacement:
              fill_type = Replacement;
              ++statCompletedPrefetches_Replacement;
              break;
            case eCoherence:
              fill_type = Coherence;
              ++statCompletedPrefetches_Coherence;
              break;
            default:
              //Leave fill type as Unknown
              fill_type = Unknown;
              ++statCompletedPrefetches_Unknown;
              break;
          }
        }
        tFillLevel fill_level = eUnknown;
        if (tracker->fillLevel()) {
          fill_level = *tracker->fillLevel();
        }
        add( msg->address(), msg->type() == MemoryMessage::PrefetchWritableReply, fill_type, ( tracker && tracker->fillLevel() ? *tracker->fillLevel() : eUnknown) );
        theTraceTracker.fill(theNodeId, ePrefetchBuffer, msg->address(), fill_level, false, false);
        return kRemoveAndWakeMAF;
      }

      case MemoryMessage::StreamFetchRejected:
        if (! theUseStreamFetch) {
          return kSendToFrontAndClearMAF;
        } else {
          ++statRejectedPrefetches_NotPresentOnChip;
          sendRedundant(msg->address());
          return kRemoveAndWakeMAF;
        }
      case MemoryMessage::PrefetchReadRedundant:
        ++statRedundantPrefetches;
        ++statRedundantPrefetches_PresentInLowerCache;
        sendRedundant(msg->address());
        return kRemoveAndWakeMAF;

      case MemoryMessage::DownProbePresent:
      case MemoryMessage::DownProbeNotPresent:
        processWatchProbe(msg->address(), msg->type());
        return kNoAction;

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << *msg ));
        return kNoAction;

    }
  }

  //May require Master, Front Message port, MAF entry,
  eAction processPrefetch( boost::intrusive_ptr<PrefetchMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker, bool hasMAFConflict ) {
    DBG_(Iface, ( << theName << "Processing Prefetch: " << *msg ) Addr(msg->address()) );

    if (theNumEntries == 0) {
      return kNoAction;
    }

    DBG_Assert( (msg->address() & 63) == 0 );

    switch (msg->type() ) {

      case PrefetchMessage::DiscardReq:
        ++statRequestedDiscards;
        drop(msg->address());
        break;

      case PrefetchMessage::WatchReq: {
        ++statRequestedWatches;
        pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(msg->address());
        if (iter != thePrefetchBuffer.get<by_address>().end() ) {
          ++statWatch_Redundant;
          sendWatchRedundant(msg->address());
        } else {
          //See if we already have a MAF entry
          if (hasMAFConflict) {
            ++statWatch_Redundant;
            sendWatchRedundant(msg->address());
          } else {
            addWatch(msg->address());
            sendProbe(msg->address(), tracker);
            return kBlockOnWatch;
          }
        }
      }
      break;

      case PrefetchMessage::PrefetchReq: {
        ++statRequestedPrefetches;
        pb_table_t::index<by_address>::type::iterator iter = thePrefetchBuffer.get<by_address>().find(msg->address());
        if (iter != thePrefetchBuffer.get<by_address>().end() ) {
          ++statRedundantPrefetches;
          ++statRedundantPrefetches_PresentInBuffer;
          sendRedundant(msg->address());
        } else {
          //See if we already have a MAF entry
          if (hasMAFConflict) {
            ++statRedundantPrefetches;
            ++statRedundantPrefetches_PrefetchInProgress;
            sendRedundant(msg->address());
          } else {
            sendProbe(msg->address(), tracker);
            return kBlockOnPrefetch;
          }
        }
      }
      break;

      default:
        DBG_Assert(false, ( << "Unhandled message type: " << msg->type()));
        break;
    }
    return kNoAction;
  }

};  // end class PBController

PBController * PBController::construct(PB * aPB, std::string const & aName, unsigned aNodeId, int32_t aNumEntries, int32_t aWatchEntries, bool anEvictClean, bool aUseStreamFetch) {
  return new PBControllerImpl( aPB, aName, aNodeId, aNumEntries, aWatchEntries, anEvictClean, aUseStreamFetch );
}

}  // end namespace nPrefetchBuffer

