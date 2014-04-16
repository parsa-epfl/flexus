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

#define DBG_DefineCategories TaglessPolicyCat
#define DBG_SetDefaultOps AddCat(TaglessPolicyCat)
#include DBG_Control()

#include <components/CMPDirectory/ProcessEntry.hpp>
#include <components/CMPDirectory/AbstractPolicy.hpp>
#include <components/CMPDirectory/TaglessPolicy.hpp>

namespace nCMPDirectory {

REGISTER_DIRECTORY_POLICY(TaglessPolicy, "Tagless");

TaglessPolicy::TaglessPolicy(const DirectoryInfo & params)
  : theDirectoryInfo(params)
  , theMAF(params.theMAFSize)
  , theDefaultState(params.theCores) {
  AbstractDirectory<State> *abs_directory = constructDirectory<State, State>(params);
  theDirectory = dynamic_cast<TaglessDirectory<State>*>(abs_directory);
  DBG_Assert(theDirectory != NULL, ( << "Failed to create a Tagless directory." ));
}

TaglessPolicy::~TaglessPolicy() {
  delete theDirectory;
}

AbstractPolicy * TaglessPolicy::createInstance(std::list<std::pair<std::string, std::string> > &args, const DirectoryInfo & params) {
  DBG_Assert(args.empty(), NoDefaultOps());
  return new TaglessPolicy(params);
}

bool TaglessPolicy::loadState(std::istream & is) {
  return theDirectory->loadState(is);
}

void TaglessPolicy::handleRequest( ProcessEntry_p process ) {
  doRequest(process, false);
}

void TaglessPolicy::doRequest( ProcessEntry_p process, bool has_maf ) {
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

  MemoryTransport rep_transport(process->transport());

  // If there's no MAF, then we can continue (No need to check evict buffer because we don't use one

  // We lookup in the directory, and we ALWAYS find SOME state that may or may not be accurate

  LookupResult_p lookup = theDirectory->nativeLookup(address);

  // It's possible a Write got ordered before our Upgrade message
  // this would have invalidated our data, so we need to change this to a WriteReq
  // the local cache should have recognized this and is expecting the directory to fix things

  // With the Tagless Directory, we can't always catch this scenario because the sharing info is imprecise
  // HOWEVER, this ONLY happens if there's another write/upgrade, so the date SHOULD be DIRTY on-chip,
  // and the dirty data should get forwarded to the requester, so everything SHOULD work out fine

  if (req_type == MemoryMessage::UpgradeReq && !lookup->state().isSharer(requester)) {
    DBG_(Trace, ( << "Received Upgrade from Non-Sharer, converting to WriteReq: " << *(process->transport()[MemoryMessageTag]) ));
    req_type = MemoryMessage::WriteReq;
    process->transport()[MemoryMessageTag]->type() = MemoryMessage::WriteReq;
  }

  // Now lookup should point to the directory entry
  // Check the coherence state of the block

  // The data is off-chip if there are NO sharers, or the only sharer is the requester (and it's not an upgrade)
  if (lookup->state().noSharers() || (req_type != MemoryMessage::UpgradeReq && lookup->state().oneSharer() && lookup->state().isSharer(requester)) ) {
    if (maf_waiting) {
      // If we found a waiting MAF entry then we wait for it to complete first, then do an on-chip transfer
      // This avoids a number of complications, reduces off-chip bandwidth, and will likely be faster in the int64_t rung
      if (has_maf) {
        theMAF.setState(process->maf(), eWaitRequest);
        process->maf()->blockState() = theDefaultState;
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
      process->maf()->blockState() = theDefaultState;
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
        process->maf()->blockState() = lookup->state();
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, lookup->state());
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
        process->maf()->blockState() = lookup->state();
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, lookup->state());
      }
      break;
    }
    case MemoryMessage::UpgradeReq:
      if (lookup->state().oneSharer()) {
        // Assert sharer == requester
        DBG_Assert(process->transport()[DestinationTag]->requester == lookup->state().getFirstSharer() );

        MemoryMessage_p rep_msg(new MemoryMessage(MemoryMessage::UpgradeReply, address));

        rep_transport.set(MemoryMessageTag, rep_msg);
        rep_transport.set(DestinationTag, DestinationMessage_p(new DestinationMessage(process->transport()[DestinationTag])));
        rep_transport[DestinationTag]->type = DestinationMessage::Requester;

        process->setReplyTransport(rep_transport);

        if (has_maf) {
          theMAF.setState(process->maf(), eWaitAck);
          process->maf()->blockState() = lookup->state();
        } else {
          theMAF.insert(process->transport(), address, eWaitAck, lookup->state());
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
          process->maf()->blockState() = lookup->state();
        } else {
          theMAF.insert(process->transport(), address, eWaitAck, lookup->state());
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
        process->maf()->blockState() = lookup->state();
      } else {
        theMAF.insert(process->transport(), address, eWaitAck, lookup->state());
      }

      break;
    }
    default:
      DBG_Assert( false, ( << "Unknown Request type received: " << *(process->transport()[MemoryMessageTag]) ));
      break;
  }

}

void TaglessPolicy::handleSnoop( ProcessEntry_p process ) {
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

  // Update the sharing state in the maf to keep it up to date
  if (maf != theMAF.end()) {
    maf->blockState().removeSharer(source);
  }

  // Okay, it wasn't in the Evict Buffer (because we don't use one), and we've looked after any active MAF requests
  LookupResult_p lookup = theDirectory->nativeLookup(address);

  // We can't just remove the sharer, we need additional information from the node
  if (process->transport()[TaglessDirMsgTag]) {
    TaglessDirMsg_p tagless_msg = process->transport()[TaglessDirMsgTag];
    lookup->removeSharer( source, tagless_msg );
  }

  if (process->action() == eNoAction && req->type() == MemoryMessage::EvictDirty) {
    process->transport()[DestinationTag]->type = DestinationMessage::Memory;
    process->addSnoopTransport(process->transport());
  }

  process->setAction(eForward);
}

void TaglessPolicy::handleReply( ProcessEntry_p process ) {
  // Replies include InvalidateAck, InvUpdateAck, FwdNAck, FetchAck, ReadAck, WriteAck, UpgradeAck
  // The rest refer to oustanding requests

  MemoryMessage_p req = process->transport()[MemoryMessageTag];
  int32_t requester = process->transport()[DestinationTag]->requester;
  int32_t source = process->transport()[DestinationTag]->source;

  if (req->type() == MemoryMessage::InvalidateAck || req->type() == MemoryMessage::InvUpdateAck) {
    // Tagless directory should NOT receive InvalidateAck+InvUpdateAck
    DBG_Assert(false, ( << "Directory received unexpected msg: " << *req ));

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
    LookupResult_p lookup = theDirectory->nativeLookup(req->address());
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
    LookupResult_p lookup = theDirectory->nativeLookup(req->address());
    DBG_Assert( lookup->found(), ( << theDirectoryInfo.theName << "Received WriteAck for unfound block: " << *req ));

    // Need to include details about which sharers no longer have copies
    DBG_Assert(process->transport()[TaglessDirMsgTag] != NULL, ( << "Received WriteAck with no TaglessDirMsg: " << *req ));
    lookup->setSharer( requester, process->transport()[TaglessDirMsgTag] );

    process->setMAF(maf);

    // remove this maf and look for stalled requests
    process->setAction(eRemoveAndWakeMAF);

  } else if (req->type() == MemoryMessage::UpgradeAck) {
    maf_iter_t maf = theMAF.findFirst(req->address(), eWaitAck);
    DBG_Assert( maf != theMAF.end(), ( << "No matching MAF for request: " << *req ));
    DBG_Assert( maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq, ( << "Matching MAF is not an Upgrade. MAF = " << *(maf->transport()[MemoryMessageTag]) << " reply = " << *req ));
    DBG_Assert( maf != theMAF.end() && maf->transport()[MemoryMessageTag]->type() == MemoryMessage::UpgradeReq );

    // Do a directory lookup and set the state
    LookupResult_p lookup = theDirectory->nativeLookup(req->address());
    DBG_Assert( lookup->found() );

    // Need to include details about which sharers no longer have copies
    DBG_Assert(process->transport()[TaglessDirMsgTag] != NULL, ( << "Received WriteAck with no TaglessDirMsg: " << *req ));
    lookup->setSharer( requester, process->transport()[TaglessDirMsgTag] );

    process->setMAF(maf);

    // remove this maf and look for stalled requests
    process->setAction(eRemoveAndWakeMAF);

  } else if (req->type() == MemoryMessage::FwdNAck) {
    LookupResult_p lookup = theDirectory->nativeLookup(req->address());
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

    first->blockState().removeSharer(source);
    first->blockState().removeSharer(requester); // Just to make sure

    // Okay, we have the maf entry, either send a new fwd msg or forward the original message to memory
    if (first->blockState().noSharers()) {
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

      int32_t sharer = pickSharer(first->blockState(), transport[DestinationTag]->requester, transport[DestinationTag]->directory);
      transport[DestinationTag]->source = sharer;
      first->transport()[DestinationTag]->source = sharer;

      process->addSnoopTransport(transport);
      process->setAction(eForward);
    }
  } else if (req->type() == MemoryMessage::WriteRetry) {
    LookupResult_p lookup = theDirectory->nativeLookup(req->address());
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

    // Now forward the message to memory
    first->transport()[DestinationTag]->type = DestinationMessage::Memory;
    first->transport()[DestinationTag]->source = -1;
    process->addSnoopTransport(first->transport());
    process->setAction(eForward);
  } else {
    DBG_Assert(false, ( << "Un-expected reply received at directory: " << *req ));
  }
}

void TaglessPolicy::handleWakeMAF( ProcessEntry_p process ) {
  doRequest(process, true);
}

void TaglessPolicy::handleEvict( ProcessEntry_p process ) {
  process->addSnoopTransport(process->transport());
  process->setAction(eForward);
}

void TaglessPolicy::handleIdleWork( ProcessEntry_p process ) {
  process->addSnoopTransport(process->transport());
  process->setAction(eForward);
}

bool TaglessPolicy::isQuiesced() const {
  // Empty MAF
  return theMAF.empty();
}

bool TaglessPolicy::hasIdleWorkAvailable() {
  // Never any idle work
  return false;
}

MemoryTransport TaglessPolicy::getIdleWorkTransport() {

  DBG_Assert(false, ( << "Called getIdleWorkTransport() for Tagless Dir which NEVER has idle work!" ));
  return MemoryTransport();
}

MemoryTransport TaglessPolicy::getEvictBlockTransport() {
  DBG_Assert(false, ( << "Called getEvictBLockTransport() for Tagless Dir which NEVER evicts blocks!" ));
  return MemoryTransport();
}

void TaglessPolicy::wakeMAFs(MemoryAddress anAddress) {

  // First, check for active MAFs
  MissAddressFile::iterator active = theMAF.findFirst(anAddress, eWaitAck);
  if (active != theMAF.end()) {
    return;
  }

  // If we had multiple simultaneous Reads/Fetches, we unprotect the block
  // here because this is when we know there are no more outstanding
  LookupResult_p lookup = theDirectory->nativeLookup(anAddress);

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

int32_t TaglessPolicy::pickSharer(const SimpleDirectoryState & state, int32_t requester, int32_t dir) {
  int32_t ret = -1;
  if (state.isSharer(requester)) {
    SimpleDirectoryState temp_state(state);
    temp_state.removeSharer(requester);
    if (temp_state.isSharer(dir)) {
      ret = dir;
    } else {
      ret = temp_state.getFirstSharer();
    }
  } else {
    if (state.isSharer(dir)) {
      ret = dir;
    } else {
      ret = state.getFirstSharer();
    }
  }
  return ret;
}

}; // namespace nCMPDirectory
