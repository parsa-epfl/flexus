// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#include "coreModelImpl.hpp"
#include <boost/optional/optional_io.hpp>
#include <core/qemu/mai_api.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {

inline bits mask(eSize aSize) {
  switch (aSize) {
  case kByte:
    return (bits)0xFFULL;
  case kHalfWord:
    return (bits)0xFFFFULL;
  case kWord:
    return (bits)0xFFFFFFFFULL;
  case kDoubleWord:
    return (bits)0xFFFFFFFFFFFFFFFFULL;
  case kQuadWord:
    return (bits)-1;
  default:
    DBG_Assert(false);
    return 0;
  }
}

bits value(MemQueueEntry const &anEntry, bool aFlipEndian) {
  DBG_Assert(anEntry.theValue);
  return *anEntry.theValue << (16 - anEntry.theSize) * 8;
}

bits loadValue(MemQueueEntry const &anEntry) {
  DBG_Assert(anEntry.loadValue());
  return *anEntry.loadValue() << (16 - anEntry.theSize) * 8;
}

uint64_t offset(MemQueueEntry const &anEntry) {
  return (16 - static_cast<int>(anEntry.theSize)) -
         (static_cast<uint64_t>(anEntry.thePaddr) - anEntry.thePaddr_aligned);
}

bits makeMask(MemQueueEntry const &anEntry) {
  bits mask = nuArchARM::mask(anEntry.theSize) << (16 - anEntry.theSize) * 8;
  mask = mask >> (offset(anEntry) * 8);
  return mask;
}

bool covers(MemQueueEntry const &aStore, MemQueueEntry const &aLoad) {
  bits required = makeMask(aLoad);
  bits available = makeMask(aStore);
  return ((required & available) == required);
}

bool intersects(MemQueueEntry const &aStore, MemQueueEntry const &aLoad) {
  bits required = makeMask(aLoad);
  bits available = makeMask(aStore);
  return ((required & available) != 0);
}

bits align(MemQueueEntry const &anEntry, bool flipEndian) {
  return (value(anEntry, flipEndian) >> (offset(anEntry) * 8));
}

bits alignLoad(MemQueueEntry const &anEntry) {
  return (loadValue(anEntry) >> (offset(anEntry) * 8));
}

void writeAligned(MemQueueEntry const &anEntry, bits anAlignedValue, bool use_ext = false) {
  bits masked_value = anAlignedValue & makeMask(anEntry);
  masked_value <<= (offset(anEntry) * 8);
  masked_value >>= (16 - anEntry.theSize) * 8;
  if (!use_ext) {
    anEntry.loadValue() = masked_value;
  } else {
    anEntry.theExtendedValue = masked_value;
  }
}

void overlay(MemQueueEntry const &aStore, MemQueueEntry const &aLoad, bool use_ext = false) {
  bits available = makeMask(aStore); // Determine what part of the int64_t the store covers
  bits store_aligned = align(aStore,
                             aStore.theInverseEndian !=
                                 aLoad.theInverseEndian); // align the store value for computation,
                                                          // flip endianness if neccessary
  bits load_aligned = alignLoad(aLoad);                   // align the load value for computation
  load_aligned = load_aligned & (~available);             // zero out the overlap region in the load
  load_aligned =
      load_aligned | (store_aligned & available); // paste the store value into the overlap
  writeAligned(aLoad, load_aligned,
               use_ext); // un-align the value and put it back in the load
}

void CoreImpl::forwardValue(MemQueueEntry const &aStore,
                            memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  if (covers(
          aStore,
          *aLoad) /*&& (aLoad->theASI != 0x24) && (aLoad->theASI != 0x2C)*/) { // QUAD-LDDs are
                                                                               // always considered
                                                                               // partial-snoops.
    if (aLoad->theSize == aStore.theSize) {
      // Sizes match and load covers store - simply copy value
      if (aLoad->theInverseEndian == aStore.theInverseEndian || !aStore.theValue) {
        aLoad->loadValue() = aStore.theValue;
      } else {
        assert(false);
        //        aLoad->loadValue() = bits(Flexus::Qemu::endianFlip(
        //        (*aStore.theValue), aStore.theSize));
        DBG_(Iface, (<< "Inverse endian forwarding of " << *aStore.theValue << " from " << aStore
                     << " to " << *aLoad << " load value: " << aLoad->loadValue()));
      }
      signalStoreForwardingHit_fn(true);
    } else {
      if (aStore.theValue) {
        if (aLoad->theInverseEndian == aStore.theInverseEndian) {
          writeAligned(*aLoad, align(aStore, false));
          DBG_(Verb, (<< "Forwarded value=" << aLoad->theValue << " to " << *aLoad << " from "
                      << aStore));
        } else {
          writeAligned(*aLoad, align(aStore, true));
          DBG_(Iface, (<< "Inverse endian forwarding of " << *aStore.theValue << " from " << aStore
                       << " to " << *aLoad << " load value: " << aLoad->loadValue()));
        }
        signalStoreForwardingHit_fn(true);
      } else {
        DBG_(Verb, (<< *aLoad << " depends completely on " << aStore
                    << " but value is not yet available"));
        aLoad->loadValue() = boost::none;
      }
    }
    if (aLoad->thePartialSnoop) {
      --thePartialSnoopersOutstanding;
    }
    aLoad->thePartialSnoop = false;
  } else {
    aLoad->loadValue() = boost::none;
    if (!aLoad->thePartialSnoop) {
      ++thePartialSnoopersOutstanding;
      ++thePartialSnoopLoads;
      aLoad->thePartialSnoop = true;
    }
    DBG_(Verb, (<< *aLoad << " depends partially on " << aStore
                << ". Must apply stores when completed."));
  }
}

template <class Iter>
boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::snoopQueue(Iter iter, Iter end, memq_t::index<by_insn>::type::iterator load) {
  FLEXUS_PROFILE();
  boost::reverse_iterator<Iter> r_iter(iter);
  boost::reverse_iterator<Iter> r_end(end);

  while (r_iter != r_end) {

    DBG_(Verb, (<< theName << " " << *load << " snooping " << *r_iter));
    if (r_iter->isStore() && (r_iter->status() != kAnnulled) && intersects(*r_iter, *load)) {
      DBG_Assert(r_iter->theSequenceNum < load->theSequenceNum);
      DBG_(Verb, (<< theName << " " << *load << " forwarding" << *r_iter));
      // See if the store covers the entire load
      forwardValue(*r_iter, load);
      return theMemQueue.get<by_insn>().find(r_iter->theInstruction);
    }
    ++r_iter;
  }
  return theMemQueue.get<by_insn>().end();
}

boost::optional<memq_t::index<by_insn>::type::iterator>
CoreImpl::snoopStores(memq_t::index<by_insn>::type::iterator aLoad,
                      boost::optional<memq_t::index<by_insn>::type::iterator> aCachedSnoopState) {
  FLEXUS_PROFILE();
  DBG_Assert(aLoad->isLoad());
  if (aLoad->isAtomic()) {
    DBG_Assert(theSpeculativeOrder);
  }

  if (aLoad->thePaddr == kUnresolved) {
    return aCachedSnoopState; // can't snoop without a resolved address
  }

  aLoad->loadValue() = boost::none; // assume no match unless we find something

  if (aCachedSnoopState) {
    DBG_(Verb, (<< "cached state in snoopStores."));
    // take advantage of the cached search
    if (*aCachedSnoopState != theMemQueue.get<by_insn>().end()) {
      DBG_(Verb, (<< "Forwarding via cached snoop state: " << **aCachedSnoopState));
      forwardValue(**aCachedSnoopState, aLoad);
    }
  } else {
    memq_t::index<by_paddr>::type::iterator load = theMemQueue.project<by_paddr>(aLoad);
    memq_t::index<by_paddr>::type::iterator oldest_match =
        theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(aLoad->thePaddr_aligned));
    aCachedSnoopState = snoopQueue(load, oldest_match, aLoad);
  }

  return aCachedSnoopState;
}

void CoreImpl::updateDependantLoads(memq_t::index<by_insn>::type::iterator anUpdatedStore) {
  FLEXUS_PROFILE();
  if (anUpdatedStore->thePaddr == kUnresolved || anUpdatedStore->thePaddr == kInvalid) {
    CORE_DBG("anUpdatedStore->thePaddr == kUnresolved || "
             "anUpdatedStore->thePaddr == kInvalid");
    return;
  }

  DBG_(Verb, (<< "Updating loads dependant on " << *anUpdatedStore));
  CORE_DBG("Updating loads dependant on " << *anUpdatedStore);

  memq_t::index<by_paddr>::type::iterator updated_store =
      theMemQueue.project<by_paddr>(anUpdatedStore);
  memq_t::index<by_paddr>::type::iterator iter, entry, last_match;
  last_match =
      theMemQueue.get<by_paddr>().upper_bound(std::make_tuple(updated_store->thePaddr_aligned));

  // Loads with higher sequence numbers than anUpdatedStore must be squashed and
  // obtain their new value
  boost::optional<memq_t::index<by_insn>::type::iterator> cached_search =
      boost::make_optional(false, memq_t::index<by_insn>::type::iterator());
  iter = updated_store;
  ++iter;
  while (iter != last_match) {
    entry = iter;
    ++iter;
    uint64_t seq_no = iter->theSequenceNum;
    DBG_Assert(entry->theSequenceNum > updated_store->theSequenceNum,
               (<< " entry: " << *entry << " updated_store: " << *updated_store));
    if (entry->isLoad() && intersects(*updated_store, *entry)) {
      CORE_DBG("Loads of overlapping address must re-snoop");

      // Loads of overlapping address must re-snoop
      bool research = (iter != last_match);
      cached_search = doLoad(theMemQueue.project<by_insn>(entry), cached_search);
      if (research) {
        iter = theMemQueue.get<by_paddr>().lower_bound(
            std::make_tuple(updated_store->thePaddr_aligned, seq_no));
        last_match = theMemQueue.get<by_paddr>().upper_bound(
            std::make_tuple(updated_store->thePaddr_aligned));
      }
    } else if (entry->isStore() && (entry->status() != kAnnulled) &&
               intersects(*updated_store, *entry)) {
      CORE_DBG("Search terminated at " << *entry);
      DBG_(Verb, (<< "Search terminated at " << *entry));
      break; // Stop on a subsequent store which intersects this store.  Loads
             // past this point will not change outcome as a result of this
             // store
    }
  }
  DBG_(Verb, (<< "Search complete."));
  CORE_DBG("Search complete.");
}

void CoreImpl::applyStores(memq_t::index<by_paddr>::type::iterator aFirstStore,
                           memq_t::index<by_paddr>::type::iterator aLastStore,
                           memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  while (aFirstStore != aLastStore) {
    // This optimization is broken if there is a store in the SRB
    //  if (aFirstStore->theQueue == kSB) {
    //    DBG_( Verb, ( << theName << " Reached start of SB in applyStores") );
    //    return;
    //  }
    if (aFirstStore->isStore() && aFirstStore->theValue && (aFirstStore->status() != kAnnulled)) {
      if (intersects(*aFirstStore, *aLoad)) {
        DBG_(Verb, (<< theName << " Applying " << *aFirstStore << " to " << *aLoad));
        overlay(*aFirstStore, *aLoad);
      }
    }
    ++aFirstStore;
  }
}
void CoreImpl::applyAllStores(memq_t::index<by_insn>::type::iterator aLoad) {
  FLEXUS_PROFILE();
  DBG_(Verb, (<< theName << " Partial snoop applying stores: " << *aLoad));
  // Need to apply every overlapping store to the value of this load.
  memq_t::index<by_paddr>::type::iterator load = theMemQueue.project<by_paddr>(aLoad);
  memq_t::index<by_paddr>::type::iterator oldest_match =
      theMemQueue.get<by_paddr>().lower_bound(std::make_tuple(aLoad->thePaddr_aligned));
  applyStores(oldest_match, load, aLoad);

  //  if ((aLoad->theASI == 0x24) || (aLoad->theASI == 0x2C)) {
  //    DBG_( Verb, ( << theName << " Partial snoop applying stores: " <<
  //    *aLoad) );
  //    //Need to apply every overlapping store to the value of this load.
  //    memq_t::index< by_paddr >::type::iterator end =
  //    theMemQueue.get<by_paddr>().upper_bound(
  //    std::make_tuple(aLoad->thePaddr_aligned + 8) ); memq_t::index< by_paddr
  //    >::type::iterator it  = theMemQueue.get<by_paddr>().lower_bound(
  //    std::make_tuple(aLoad->thePaddr_aligned + 8) ); do {
  //      DBG_(Verb, ( << theName << " QW Snoop applying " << *it << " to " <<
  //      *aLoad)); overlay(*it, *aLoad, true /* ext */);
  //      ++it;
  //    } while (it != end);
  //  }
}

} // namespace nuArchARM
