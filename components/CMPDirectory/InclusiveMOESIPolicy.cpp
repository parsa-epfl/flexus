#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/flexus.hpp>

#include <core/simulator_layout.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/MessageQueues.hpp>

using namespace Flexus;

#define DBG_DefineCategories InclusiveMOESIPolicyCat
#define DBG_SetDefaultOps AddCat(InclusiveMOESIPolicyCat)
#include DBG_Control()

#include <components/CMPDirectory/ProcessEntry.hpp>
#include <components/CMPDirectory/AbstractPolicy.hpp>
#include <components/CMPDirectory/InclusiveMOESIPolicy.hpp>

namespace nCMPDirectory {

REGISTER_DIRECTORY_POLICY(InclusiveMOESIPolicy, "InclusiveMOESI");

InclusiveMOESIPolicy::InclusiveMOESIPolicy(const DirectoryInfo & params)
  : theDirectoryInfo(params)
  , theMAF(params.theMAFSize)
  , theDefaultState(params.theCores) {
  theDirectory = constructDirectory<State, State>(params);
  theEvictBuffer = theDirectory->getEvictBuffer();
}

InclusiveMOESIPolicy::~InclusiveMOESIPolicy() {
  delete theDirectory;
}

AbstractPolicy * InclusiveMOESIPolicy::createInstance(std::list<std::pair<std::string, std::string> > &args, const DirectoryInfo & params) {
  DBG_Assert(args.empty(), NoDefaultOps());
  return new InclusiveMOESIPolicy(params);
}

bool InclusiveMOESIPolicy::loadState(std::istream & is) {
  return theDirectory->loadState(is);
}

void InclusiveMOESIPolicy::handleRequest( ProcessEntry_p process ) {
  doRequest(process, false);
}

bool InclusiveMOESIPolicy::allocateDirectoryEntry( LookupResult_p lookup, MemoryAddress anAddress, const State & aState ) {
  return theDirectory->allocate(lookup, anAddress, aState);
}

void InclusiveMOESIPolicy::doRequest( ProcessEntry_p process, bool has_maf ) {
  // Evicts are sent as REQUESTS, although you could argue they should be snoops
  // This ensures that a request message can't bypass an evict message in the network
  if (process->transport()[MemoryMessageTag]->isEvictType()) {
    return handleSnoop(process);
  }

  MemoryAddress address = process->transport()[MemoryMessageTag]->address();
  MemoryMessage::MemoryMessageType req_type = process->transport()[MemoryMessageTag]->type();
  int32_t requester = process->transport()[DestinationTag]->requester;

  // By default assume we'll stall and take no action
  process->setAction(eStall);

  // First we need to look for conflicting MAF entries
  maf_iter_t maf = theMAF.find(address);
  bool maf_waiting = false;

  if (maf != theMAF.end()) {
    switch (maf->state()) {
      case eWaitSet:
        // There's already a stalled request, to reduce starvation, we'll stall as well
        if (has_maf) {
          theMAF.setState(process->maf(), eWaitSet);
        } else {
          theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
        }
        return;
      case eWaitEvict:
      case eWaitRequest:
        // There's already a stalled request, to reduce starvation, we'll stall as well
        if (has_maf) {
          theMAF.setState(process->maf(), eWaitRequest);
        } else {
          theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
        }
        return;
      case eWaitAck: {
        // This is where things get tricky
        // We can overlap two reads/fetches, but not a read and a write
        MemoryMessage::MemoryMessageType maf_type = maf->transport()[MemoryMessageTag]->type();
        if (req_type == MemoryMessage::WriteReq || req_type == MemoryMessage::UpgradeReq || req_type == MemoryMessage::NonAllocatingStoreReq
            || maf_type == MemoryMessage::WriteReq || maf_type == MemoryMessage::UpgradeReq || maf_type == MemoryMessage::NonAllocatingStoreReq) {
          if (has_maf) {
            theMAF.setState(process->maf(), eWaitRequest);
          } else {
            theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
          }
          return;
        } else {
          maf_waiting = true;
        }
        // Both the maf and incoming request are reads so we can overlap them and just ignore the maf
        break;
      }
      // entries in the process of waking up are ignored
      case eWaking:
      case eInPipeline:
        break;

      default:
        DBG_Assert(false, ( << "Invalid MAFState: " << maf->state() ));
        break;
    }
  }
  // No MAF entry, what about the evict buffer
  const AbstractEBEntry<State>* eb = theEvictBuffer->find(address);
  if (eb != nullptr) {
    if (eb->invalidatesPending()) {
      // We've sent out back invalidates to evict this block
      // Wait until they get back before processing the new request
      if (has_maf) {
        theMAF.setState(process->maf(), eWaitEvict);
      } else {
        theMAF.insert(process->transport(), address, eWaitEvict, theDefaultState);
      }
      return;
    }
  }

  // Okay, we're going to try to handle the request
  // Ne need to determine the state of the block
  // 3 cases:
  //  1. block is in the directory
  //  2. block is in the evict buffer
  //  3. block is in neither directory nor evict buffer
  //
  // If it's case 2, we should move the block BACK into the directory
  // If it's case 3, we should allocate the block in the directory
  // In either case, we can do the directory lookup first

  LookupResult_p lookup = theDirectory->lookup(address);
  // If we didn't find it in the directory
  if (!lookup->found()) {
    // We already did an EB lookup, check if we found it
    if (eb != nullptr) {
      // Move the entry from the EB into the directory
      if (!allocateDirectoryEntry(lookup, address, eb->state())) {
        // if this fails then we have a set conflict, so insert the MAF and return
        if (has_maf) {
          theMAF.setState(process->maf(), eWaitSet);
        } else {
          theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
        }
        DBG_(Trace, ( << theDirectoryInfo.theName << " - failed to allocate directory entry for " << std::hex << address << " found in evict buffer." ));
        return;
      }
      // remove the old eb entry
      theEvictBuffer->remove(address);
    } else if (req_type == MemoryMessage::NonAllocatingStoreReq) {
      process->transport()[DestinationTag]->type = DestinationMessage::Memory;
      process->addSnoopTransport(process->transport());
      process->setAction(eFwdAndWaitAck);
      if (has_maf) {
        theMAF.setState(process->maf(), eWaitAck);
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
      }
      return;
    } else {
      // Create a new entry
      if (!allocateDirectoryEntry(lookup, address, theDefaultState)) {
        if (has_maf) {
          theMAF.setState(process->maf(), eWaitSet);
        } else {
          theMAF.insert(process->transport(), address, eWaitSet, theDefaultState);
        }
        DBG_(Trace, ( << theDirectoryInfo.theName << " - failed to allocate directory entry for " << std::hex << address << " stalling." ));
        return;
      }
    }
  }

  // Protect the directory entry so it can't be removed until we get a final acknowledgement
  if (req_type != MemoryMessage::NonAllocatingStoreReq) {
    lookup->setProtected(true);
  }

  // It's possible a Write got ordered before our Upgrade message
  // this would have invalidated our data, so we need to change this to a WriteReq
  // the local cache should have recognized this and is expecting the directory to fix things
  if (req_type == MemoryMessage::UpgradeReq && !lookup->state().isSharer(requester)) {
    DBG_(Trace, ( << "Received Upgrade from Non-Sharer, converting to WriteReq: " << *(process->transport()[MemoryMessageTag]) ));
    req_type = MemoryMessage::WriteReq;
    process->transport()[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
  }

  MemoryTransport rep_transport(process->transport());

  // Now lookup should point to the directory entry
  // Check the coherence state of the block
  if (lookup->state().noSharers()) {
    if (maf_waiting) {
      // If we found a waiting MAF entry then we wait for it to complete first, then do an on-chip transfer
      // This avoids a number of complications, reduces off-chip bandwidth, and will likely be faster in the int64_t rung
      if (has_maf) {
        theMAF.setState(process->maf(), eWaitRequest);
      } else {
        theMAF.insert(process->transport(), address, eWaitRequest, theDefaultState);
      }
      return;
    }

    // We don't have the data on chip, we need to forward the request to memory
    process->transport()[DestinationTag]->type = DestinationMessage::Memory;
    process->addSnoopTransport(process->transport());
    process->setAction(eFwdAndWaitAck);
    if (has_maf) {
      theMAF.setState(process->maf(), eWaitAck);
    } else {
      theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
    }

    // Write requests need a miss-notify
    if (req_type == MemoryMessage::WriteReq) {
      MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
      rep_msg->outstandingMsgs() = 1;
      rep_transport.set(MemoryMessageTag, rep_msg);
      rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
      rep_transport[DestinationTag]->type = DestinationMessage::Requester;
      process->setReplyTransport(rep_transport);
      process->setAction(eNotifyAndWaitAck);
    }
    return;
  }

  // We have 1 or more sharers, look at the request type to determine the action
  switch (req_type) {
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq: {
      // Select a sharer, forward the message to them
      MemoryMessage_p msg(new MemoryMessage(MemoryMessage::ReadFwd, address));
      if (req_type == MemoryMessage::FetchReq) {
        msg->type() = MemoryMessage::FetchFwd;
      }
      msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();

      rep_transport.set(MemoryMessageTag, msg);

      // Setup the destination as one of the sharers
      rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
      rep_transport[DestinationTag]->type = DestinationMessage::Source;

      int32_t sharer = pickSharer(lookup->state(), rep_transport[DestinationTag]->requester, rep_transport[DestinationTag]->directory);
      rep_transport[DestinationTag]->source = sharer;
      process->transport()[DestinationTag]->source = sharer;

      process->addSnoopTransport(rep_transport);
      process->setAction(eFwdAndWaitAck);

      if (has_maf) {
        theMAF.setState(process->maf(), eWaitAck);
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
      }

      break;
    }
    case MemoryMessage::WriteReq: {
      // Need to send out a Fwd request, possilby some invalidates, and a miss notify reply

      // First, pick a sharer
      int32_t sharer = pickSharer(lookup->state(), process->transport()[DestinationTag]->requester, process->transport()[DestinationTag]->directory);

      // Now, setup the forward
      MemoryMessage_p fwd_msg(new MemoryMessage(MemoryMessage::WriteFwd, address));
      fwd_msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();

      MemoryTransport fwd_transport(process->transport());
      fwd_transport.set(MemoryMessageTag, fwd_msg);
      fwd_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
      fwd_transport[DestinationTag]->type = DestinationMessage::Source;
      fwd_transport[DestinationTag]->source = sharer;
      process->addSnoopTransport(fwd_transport);

      // Now setup the invalidates if necessary
      if (lookup->state().manySharers()) {
        MemoryMessage_p inv_msg(new MemoryMessage(MemoryMessage::Invalidate, address));
        //inv_msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();
        // Make sure size is 0 or we might accidentally get back data we didn't ask for
        inv_msg->reqSize() = 0;
        MemoryTransport inv_transport(process->transport());
        inv_transport.set(MemoryMessageTag, inv_msg);
        inv_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
        inv_transport[DestinationTag]->type = DestinationMessage::Multicast;
        lookup->state().getOtherSharers(inv_transport[DestinationTag]->multicast_list, sharer);
        process->addSnoopTransport(inv_transport);
      }

      // Finally setup the reply msg
      MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
      rep_msg->outstandingMsgs() = lookup->state().countSharers();

      rep_transport.set(MemoryMessageTag, rep_msg);
      rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
      rep_transport[DestinationTag]->type = DestinationMessage::Requester;
      process->setReplyTransport(rep_transport);
      process->setAction(eNotifyAndWaitAck);

      process->transport()[DestinationTag]->source = sharer;

      if (has_maf) {
        theMAF.setState(process->maf(), eWaitAck);
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
      }
      break;
    }
    case MemoryMessage::UpgradeReq:
      if (lookup->state().oneSharer()) {
        // Assert sharer == requester
        DBG_Assert(process->transport()[DestinationTag]->requester == lookup->state().getFirstSharer() );

        MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::UpgradeReply, address));

        boost::intrusive_ptr<TransactionTracker> tracker = process->transport()[TransactionTrackerTag];
        tracker->setFillLevel(eDirectory);
        tracker->setFillType(eCoherence);

        rep_transport.set(MemoryMessageTag, rep_msg);
        rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
        rep_transport[DestinationTag]->type = DestinationMessage::Requester;

        process->setReplyTransport(rep_transport);

        if (has_maf) {
          theMAF.setState(process->maf(), eWaitAck);
        } else {
          theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
        }
        process->setAction(eReply);
      } else {
        MemoryMessage_p inv_msg(new MemoryMessage(MemoryMessage::Invalidate, address));
        //inv_msg->reqSize() = process->transport()[MemoryMessageTag]->reqSize();
        inv_msg->reqSize() = 0;
        MemoryTransport inv_transport(process->transport());
        inv_transport.set(MemoryMessageTag, inv_msg);
        inv_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
        inv_transport[DestinationTag]->type = DestinationMessage::Multicast;
        lookup->state().getOtherSharers(inv_transport[DestinationTag]->multicast_list, inv_transport[DestinationTag]->requester);
        process->addSnoopTransport(inv_transport);

        // Finally setup the reply msg
        MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::MissNotify, address));
        rep_msg->outstandingMsgs() = lookup->state().countSharers() - 1;

        rep_transport.set(MemoryMessageTag, rep_msg);
        rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
        rep_transport[DestinationTag]->type = DestinationMessage::Requester;
        process->setReplyTransport(rep_transport);

        process->setAction(eNotifyAndWaitAck);

        process->transport()[DestinationTag]->source = process->transport()[DestinationTag]->requester;

        if (has_maf) {
          theMAF.setState(process->maf(), eWaitAck);
        } else {
          theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
        }
      }
      break;
    case MemoryMessage::NonAllocatingStoreReq: {
      // Cheat and ignore the on-chip sharers
      // These should be rare enough that it won't make any difference
      // This should really invalidate all of the lines too
      process->transport()[DestinationTag]->type = DestinationMessage::Memory;
      process->addSnoopTransport(process->transport());
      process->setAction(eFwdAndWaitAck);
      if (has_maf) {
        theMAF.setState(process->maf(), eWaitAck);
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, theDefaultState);
      }

      break;
    }
    default:
      DBG_Assert( false, ( << "Unknown Request type received: " << *(process->transport()[MemoryMessageTag]) ));
      break;
  }

}

void InclusiveMOESIPolicy::handleSnoop( ProcessEntry_p process ) {
  MemoryMessage_p req = process->transport()[MemoryMessageTag];
  MemoryAddress address = req->address();

  int32_t source = process->transport()[DestinationTag]->requester;

  // We received an Evict msg
  DBG_Assert( req->isEvictType(), ( << "Directory received Non-Evict Snoop msg: " << (*req) ));

  process->setAction(eNoAction);

  // Look for active entries in the MAF that might require special actions
  maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);

  // We can ignore outstanding Reads/Fetches, they will complete as normal before the EvictAck arrives
  // Writes/Upgrades will complete as normal but will get rid of the block, so we DON'T need to send
  // an Ack

  if (maf != theMAF.end() && ((maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq)
                              || (maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq))) {
    // We can return without sending any Acknowledgements
    // When the write/upgrade completes it will set the proper sharers
    // Nothing else will happen while the write is waiting, so we're good to go
    process->setAction(eNoAction);
    return;
  }

  MemoryTransport ack(process->transport());
  ack.set(MemoryMessageTag, new MemoryMessage(MemoryMessage::EvictAck, req->address()));
  ack.set(DestinationTag, new DestinationMessage(process->transport()[DestinationTag]));

  ack[DestinationTag]->type = DestinationMessage::Requester;

  process->addSnoopTransport(ack);

  // Now look for any EB entries
  const AbstractEBEntry<State>* eb = theEvictBuffer->find(req->address());

  if (eb != nullptr) {
    eb->state().removeSharer( source );

    bool wake_maf = false;
    if (eb->state().noSharers()) {
      theEvictBuffer->remove(eb->address());

      maf_iter_t waiting_maf = theMAF.findFirst(req->address(), eWaitEvict);
      if (waiting_maf != theMAF.end()) {
        process->setMAF(maf);
        wake_maf = true;
      }
    }

    if (req->type() == MemoryMessage::EvictDirty) {
      process->transport()[DestinationTag]->type = DestinationMessage::Memory;
      process->addSnoopTransport(process->transport());
    }

    if (wake_maf) {
      process->setAction(eFwdAndWakeEvictMAF);
    } else {
      process->setAction(eForward);
    }

    return;
  }

  // Okay, it wasn't in the Evict Buffer, and we've looked after any active MAF requests
  LookupResult_p lookup = theDirectory->lookup(address);

  DBG_Assert( lookup->found(), ( << "Directory received evict for block not in directory: " << (*req) ));

  lookup->removeSharer( source );

  if (process->action() == eNoAction && req->type() == MemoryMessage::EvictDirty) {
    process->transport()[DestinationTag]->type = DestinationMessage::Memory;
    process->addSnoopTransport(process->transport());
  }

  // Always forward (either just Ack or Ack+Evict to mem)
  process->setAction(eForward);
}

void InclusiveMOESIPolicy::handleReply( ProcessEntry_p process ) {
  // Replies include InvalidateAck, InvUpdateAck, FwdNAck, FetchAck, ReadAck, WriteAck, UpgradeAck
  // The rest refer to oustanding requests

  MemoryMessage_p req = process->transport()[MemoryMessageTag];
  int32_t requester = process->transport()[DestinationTag]->requester;

  if (req->type() == MemoryMessage::InvalidateAck || req->type() == MemoryMessage::InvUpdateAck) {
    // InvalidateAck+InvUpdateAck imply there is an outstanding Eviction

    const AbstractEBEntry<State>* eb = theEvictBuffer->find(req->address());
    DBG_Assert(eb != nullptr);

    eb->state().removeSharer( process->transport()[DestinationTag]->other );

    bool wake_maf = false;
    if (eb->state().noSharers()) {
      theEvictBuffer->remove(eb->address());

      maf_iter_t waiting_maf = theMAF.findFirst(req->address(), eWaitEvict);
      if (waiting_maf != theMAF.end()) {
        process->setMAF(waiting_maf);
        wake_maf = true;
      }
    }

    if (req->type() == MemoryMessage::InvUpdateAck) {
      // We need to forward the data to memory as a writeback
      req->type() = MemoryMessage::EvictDirty;
      process->transport()[DestinationTag]->type = DestinationMessage::Memory;
      process->addSnoopTransport(process->transport());
      if (wake_maf) {
        process->setAction(eFwdAndWakeEvictMAF);
      } else {
        process->setAction(eForward);
      }
    } else {
      if (wake_maf) {
        process->setAction(eWakeEvictMAF);
      } else {
        process->setAction(eNoAction);
      }
    }

  } else if (req->type() == MemoryMessage::FetchAck || req->type() == MemoryMessage::ReadAck) {
    maf_iter_t first, last;
    boost::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
    DBG_Assert(first != theMAF.end());
    for (; first != last; first++) {
      // The matching request is the one with the same requester
      if (first->transport()[DestinationTag]->requester == requester) {
        process->setMAF(first);
        break;
      }
    }
    DBG_Assert( first != last, ( << "No matching MAF found for " << *req));

    // Do a directory lookup and set the state
    LookupResult_p lookup = theDirectory->lookup(req->address());
    DBG_Assert( lookup->found(), ( << "Received ACK but couldn't find matching directory entry: " << *req ));
    lookup->addSharer( requester );

    // When we try to Wake other MAF's we'll make sure there aren't any active ones
    // We'll also remove the protected bit at that point.
    process->setAction(eRemoveAndWakeMAF);
  } else if (req->type() == MemoryMessage::NASAck) {
    maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
    DBG_Assert( maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::NonAllocatingStoreReq );

    process->setMAF(maf);

    // remove this maf and look for stalled requests
    process->setAction(eRemoveAndWakeMAF);
  } else if (req->type() == MemoryMessage::WriteAck) {

    maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
    DBG_Assert( maf != theMAF.end(), ( << "No MAF waiting for ack: " << *req ));
    DBG_Assert( maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq, ( << "Received WriteAck in response to non-write: " << *(maf->transport()[MemoryMessageTag]) ));
    DBG_Assert( maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::WriteReq );

    // Do a directory lookup and set the state
    LookupResult_p lookup = theDirectory->lookup(req->address());
    DBG_Assert( lookup->found(), ( << theDirectoryInfo.theName << "Received WriteAck for unfound block: " << *req ));
    lookup->setSharer( requester );
    lookup->setProtected(false);

    process->setMAF(maf);

    // remove this maf and look for stalled requests
    process->setAction(eRemoveAndWakeMAF);

  } else if (req->type() == MemoryMessage::UpgradeAck) {
    maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
    DBG_Assert( maf != theMAF.end(), ( << "No matching MAF for request: " << *req ));
    DBG_Assert( maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq, ( << "Matching MAF is not an Upgrade. MAF = " << *(maf->transport()[MemoryMessageTag]) << " reply = " << *req ));
    DBG_Assert( maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq );

    // Do a directory lookup and set the state
    LookupResult_p lookup = theDirectory->lookup(req->address());
    DBG_Assert( lookup->found() );
    lookup->setSharer( requester );
    lookup->setProtected(false);

    process->setMAF(maf);

    // remove this maf and look for stalled requests
    process->setAction(eRemoveAndWakeMAF);

  } else if (req->type() == MemoryMessage::FwdNAck) {
    LookupResult_p lookup = theDirectory->lookup(req->address());
    DBG_Assert( lookup->found() );

    // Need to find the missing request
    maf_iter_t first, last;
    boost::tie(first, last) = theMAF.findAll(req->address(), eWaitAck);
    DBG_Assert(first != theMAF.end());
    for (; first != last; first++) {
      // The matching request is the one with the same requester
      if (first->transport()[DestinationTag]->requester == requester) {
        break;
      }
    }
    DBG_Assert( first != last );

    // Okay, we have the maf entry, either send a new fwd msg or forward the original message to memory
    if (lookup->state().noSharers()) {
      // Send the original message on to memory
      first->transport()[DestinationTag]->type = DestinationMessage::Memory;
      first->transport()[DestinationTag]->source = -1;
      process->addSnoopTransport(first->transport());
      process->setAction(eForward);
    } else {
      // Select a sharer, forward the message to them
      MemoryMessage_p msg(new MemoryMessage(MemoryMessage::ReadFwd, first->transport()[MemoryMessageTag]->address()));
      if (first->transport()[MemoryMessageTag]->type() == MemoryMessage::FetchReq) {
        msg->type() = MemoryMessage::FetchFwd;
      }
      msg->reqSize() = first->transport()[MemoryMessageTag]->reqSize();

      MemoryTransport transport(process->transport());
      transport.set(MemoryMessageTag, msg);

      // Setup the destination as one of the sharers
      transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(first->transport()[DestinationTag])));
      transport[DestinationTag]->type = DestinationMessage::Source;

      int32_t sharer = pickSharer(lookup->state(), transport[DestinationTag]->requester, transport[DestinationTag]->directory);
      transport[DestinationTag]->source = sharer;
      first->transport()[DestinationTag]->source = sharer;

      process->addSnoopTransport(transport);
      process->setAction(eForward);
    }
  } else {
    DBG_Assert(false, ( << "Un-expected reply received at directory: " << *req ));
  }
}

void InclusiveMOESIPolicy::handleWakeMAF( ProcessEntry_p process ) {
  doRequest(process, true);
}

void InclusiveMOESIPolicy::handleEvict( ProcessEntry_p process ) {
  process->addSnoopTransport(process->transport());
  process->setAction(eForward);
}

void InclusiveMOESIPolicy::handleIdleWork( ProcessEntry_p process ) {
  if (process->transport()[MemoryMessageTag]->type() == MemoryMessage::NumMemoryMessageTypes) {
    // The directory has some Idle Work it wants to do
    // perform a lookup, but no other action
    process->setLookupsRequired(theDirectory->doIdleWork());
    process->setAction(eNoAction);
  } else {
    process->addSnoopTransport(process->transport());
    process->setAction(eForward);
  }
}

bool InclusiveMOESIPolicy::isQuiesced() const {
  // Empty MAF and empty EB
  return theEvictBuffer->empty() && theMAF.empty();
}

bool InclusiveMOESIPolicy::hasIdleWorkAvailable() {
  // Need to check if any evict buffer entries have un-sent invalidates
  if (!theEvictBuffer->empty()) {
    return theEvictBuffer->idleWorkReady() || theDirectory->idleWorkReady();
  } else {
    return false;
  }
}

MemoryTransport InclusiveMOESIPolicy::getIdleWorkTransport() {
  MemoryTransport transport;

  const AbstractEBEntry<State>* eb = theEvictBuffer->oldestRequiringInvalidates();
  //DBG_Assert(eb != nullptr, ( << "Called getIdleWorkTransport while no IdleWork outstanding!"));

  if (eb == nullptr) {
    transport.set(MemoryMessageTag, MemoryMessage_p(new MemoryMessage(MemoryMessage::NumMemoryMessageTypes)));
    return transport;
  }

  transport.set(MemoryMessageTag, MemoryMessage_p(new MemoryMessage(MemoryMessage::BackInvalidate)));
  transport[MemoryMessageTag]->address() = eb->address();

  eb->setInvalidatesPending();

  // Setup destinations
  transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(DestinationMessage::Multicast)));
  eb->state().getSharerList(transport[DestinationTag]->multicast_list);
  transport[DestinationTag]->directory = theDirectoryInfo.theNodeId;

  // Make sure we have a transaction tracker
  boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
  tracker->setAddress(eb->address());
  tracker->setInitiator(theDirectoryInfo.theNodeId);
  tracker->setSource(theDirectoryInfo.theName + " BackInvalidate");
  tracker->setDelayCause(theDirectoryInfo.theName, "BackInvalidate");
  transport.set(TransactionTrackerTag, tracker);

  return transport;
}

MemoryTransport InclusiveMOESIPolicy::getEvictBlockTransport() {
  // for us this is the same as our normal idle work

  return getIdleWorkTransport();
}

void InclusiveMOESIPolicy::wakeMAFs(MemoryAddress anAddress) {

  // First, check for active MAFs
  MissAddressFile::iterator active = theMAF.findFirst(anAddress, eWaitAck);
  if (active != theMAF.end()) {
    return;
  }

  // If we had multiple simultaneous Reads/Fetches, we unprotect the block
  // here because this is when we know there are no more outstanding
  LookupResult_p lookup = theDirectory->lookup(anAddress);
  if (lookup->found()) {
    lookup->setProtected(false);
  }

  MissAddressFile::order_iterator iter = theMAF.ordered_begin();
  MissAddressFile::order_iterator next = iter;

  while (iter != theMAF.ordered_end()) {
    next++;

    if ((iter->state() == eWaitRequest) && (iter->address() == anAddress)) {
      theMAF.wake(iter);
    } else if ((iter->state() == eWaitSet)
               && theDirectory->sameSet(anAddress, iter->address())) {
      theMAF.wake(iter);
    }

    iter = next;
  }
}

int32_t InclusiveMOESIPolicy::pickSharer(const SimpleDirectoryState & state, int32_t requester, int32_t dir) {
  if (state.isSharer(dir)) {
    return dir;
  } else {
    return state.getFirstSharer();
  }
}

}; // namespace nCMPDirectory
