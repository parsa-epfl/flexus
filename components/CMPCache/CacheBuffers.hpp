//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

/*! \file CacheController.hpp
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

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

#include <boost/intrusive_ptr.hpp>

#ifndef _CMPCACHE_CACHEBUFFERS_HPP
#define _CMPCACHE_CACHEBUFFERS_HPP

namespace nCMPCache {

using boost::counted_base;
using boost::intrusive_ptr;
using namespace Flexus::SharedTypes;
using Flexus::SharedTypes::MemoryMessage;

template <typename _State> struct EvictEntry {
  MemoryAddress theBlockAddress;
  mutable MemoryMessage::MemoryMessageType theType;
  mutable bool theEvictable;
  mutable bool thePending;
  mutable _State theState;
  mutable bool theSnoopRequired;
  mutable bool theSnoopScheduled;
  // Note - evict buffer entries should also contain data
  EvictEntry(MemoryAddress anAddress, MemoryMessage::MemoryMessageType aType, _State aState,
             bool anEvictable = true, bool aPending = false)
      : theBlockAddress(anAddress), theType(aType), theEvictable(anEvictable), thePending(aPending),
        theState(aState), theSnoopRequired(!anEvictable), theSnoopScheduled(false) {
  }

public:
  MemoryAddress address() const {
    return theBlockAddress;
  }
  MemoryMessage::MemoryMessageType &type() const {
    return theType;
  }
  bool &evictable() {
    return theEvictable;
  }
  const bool evictable() const {
    return theEvictable;
  }
  void setEvictable(bool val) const {
    theEvictable = val; // it's mutable, so we can change it while maintaining const-ness
  }
  bool &pending() {
    return thePending;
  }
  const bool pending() const {
    return thePending;
  }
  void setPending(bool val) const {
    thePending = val; // it's mutable, so we can change it while maintaining const-ness
  }
  _State &state() const {
    return theState;
  }
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    // Version 0 of the EvictEntry does not contain theEvictable.
    // It is always considered to be true in older checkpoints.
    // Version 1 contains this boolean flag.
    ar &theBlockAddress;
    ar &theType;
    if (version > 0) {
      ar &theEvictable;
      ar &theState;
    } else {
      theEvictable = true;
    }
  }
  friend std::ostream &operator<<(std::ostream &anOstream, EvictEntry const &anEntry) {
    anOstream << "Evict(" << anEntry.theType << " @" << anEntry.theBlockAddress << " - "
              << anEntry.theState << ")";
    if (anEntry.theSnoopRequired) {
      anOstream << " SnoopRequired";
    }
    if (anEntry.theSnoopScheduled) {
      anOstream << " SnoopScheduled";
    }
    if (anEntry.theEvictable) {
      anOstream << " Evictable";
    }
    if (anEntry.thePending) {
      anOstream << " Pending";
    }
    return anOstream;
  }
};

// the evict buffer contains uninitiated evictions
class AbstractEvictBuffer {
  int32_t theSize;
  int32_t theReserve;

protected:
  uint32_t thePendingEvicts; // TODO: Change code to track # of outstanding WB
                             // messages
  int32_t theCurSize;

public:
  virtual void saveState(std::ostream &anOstream) = 0;

  virtual void loadState(std::istream &anIstream) = 0;

  AbstractEvictBuffer(int32_t aSize)
      : theSize(aSize), theReserve(0), thePendingEvicts(0), theCurSize(0) {
  }

  AbstractEvictBuffer() : theSize(0), theReserve(0), thePendingEvicts(0), theCurSize(0) {
  }

  virtual ~AbstractEvictBuffer() {
  }

  bool empty() const {
    return (theCurSize == 0);
  }

  bool full() const {
    return theCurSize + theReserve >= theSize;
  }

  void reserve() {
    ++theReserve;
    DBG_(Trace, (<< "CacheEB Reserve(" << 1 << ") -> " << theReserve << " entries reserved."));
    DBG_Assert(theCurSize + theReserve <= theSize);
  }

  void reserve(int32_t count) {
    theReserve += count;
    DBG_(Trace, (<< "CacheEB Reserve(" << count << ") -> " << theReserve << " entries reserved."));
    DBG_Assert(theCurSize + theReserve <= theSize);
  }

  void unreserve() {
    --theReserve;
    DBG_(Trace, (<< "CacheEB Un-Reserve(" << 1 << ") -> " << theReserve << " entries reserved."));
    DBG_Assert(theReserve >= 0);
  }

  void unreserve(int32_t n) {
    theReserve -= n;
    DBG_(Trace, (<< "CacheEB Un-Reserve(" << n << ") -> " << theReserve << " entries reserved."));
    DBG_Assert(theReserve >= 0);
  }

  virtual bool freeSlotsPending() const {
    return ((thePendingEvicts > 0) || ((theCurSize + theReserve) < theSize));
  }

  uint32_t freeEntries() const {
    return theSize - theCurSize - theReserve;
  }

  uint32_t reservedEntries() const {
    return theReserve;
  }

  uint32_t usedEntries() const {
    return theCurSize;
  }

  uint32_t pendingEntries() const {
    return thePendingEvicts;
  }

  uint32_t size() const {
    return theSize;
  }

  virtual bool headEvictable(int32_t anOffset) const = 0;
  virtual bool evictableReady() const = 0;

  virtual void setEvictable(MemoryAddress anAddress, const bool val) = 0;

  virtual MemoryMessage::MemoryMessageType getEvictType(MemoryAddress anAddress) = 0;

  virtual void setEvictType(MemoryAddress anAddress,
                            const MemoryMessage::MemoryMessageType val) = 0;

  virtual boost::intrusive_ptr<MemoryMessage> pop() = 0;
  virtual boost::intrusive_ptr<MemoryMessage> firstNonPending() = 0;

}; // end class AbstractEvictBuffer

// the evict buffer contains uninitiated evictions
template <typename _State> class CacheEvictBuffer : public AbstractEvictBuffer {
  struct by_address {};
  struct by_order {};
  typedef multi_index_container<
      EvictEntry<_State>,
      indexed_by<sequenced<tag<by_order>>,
                 ordered_unique<tag<by_address>, member<EvictEntry<_State>, MemoryAddress,
                                                        &EvictEntry<_State>::theBlockAddress>>>>
      evict_buf_t;

  evict_buf_t theEvictions;

public:
  // Using index<by_address> seems to cause problems because of all the
  // templates
  typedef typename evict_buf_t::template nth_index<1>::type::iterator iterator;
  typedef typename evict_buf_t::template nth_index<1>::type::const_iterator const_iterator;

  virtual void saveState(std::ostream &anOstream) {
  }

  virtual void loadState(std::istream &anIstream) {
    boost::archive::binary_iarchive ia(anIstream);
    theEvictions.clear();
  }

  CacheEvictBuffer(uint32_t aSize) : AbstractEvictBuffer(aSize) {
  }

  virtual bool headEvictable(int32_t anOffset) const {
    if (empty())
      return false;
    typename evict_buf_t::iterator iter = theEvictions.begin();
    typename evict_buf_t::iterator end = theEvictions.end();

    while (anOffset > 0) {
      if (iter == end) {
        return false;
      }
      ++iter;
      --anOffset;
    }
    if (iter == end) {
      return false;
    }
    return (iter->evictable());
  }

  virtual bool evictableReady() const {
    if (empty())
      return false;
    typename evict_buf_t::iterator iter = theEvictions.begin();
    typename evict_buf_t::iterator end = theEvictions.end();
    while ((iter->pending() || !iter->evictable()) && iter != end) {
      iter++;
    }
    return (iter != end);
  }

  iterator getOldestRequiringSnoops() const {
    typename evict_buf_t::iterator iter = theEvictions.begin();
    typename evict_buf_t::iterator end = theEvictions.end();
    while (iter != end && !iter->theSnoopRequired)
      iter++;
    return (theEvictions.template project<1>(iter));
  }

  virtual void setEvictable(MemoryAddress anAddress, const bool val) {
    iterator entry = find(anAddress);

    if (entry == end())
      return;

    entry->theEvictable = val;
  }

  virtual MemoryMessage::MemoryMessageType getEvictType(MemoryAddress anAddress) {
    iterator entry = find(anAddress);

    if (entry == end())
      return MemoryMessage::EvictClean;

    return entry->theType;
  }

  virtual void setEvictType(MemoryAddress anAddress, const MemoryMessage::MemoryMessageType val) {
    iterator entry = find(anAddress);

    if (entry == end())
      return;

    entry->theType = val;
  }

  virtual iterator allocEntry(MemoryAddress anAddress, MemoryMessage::MemoryMessageType aType,
                              _State aState, const bool evictable = true) {
    iterator existing = find(anAddress);
    if (existing != end()) {
      DBG_(Iface, (<< "When trying to allocate an evict buffer entry for " << anAddress
                   << " an existing entry with type " << existing->theType << " was found"));
      existing->theType = aType;
      existing->theState = aState;
      return existing;
    } else {
      theEvictions.push_back(EvictEntry<_State>(anAddress, aType, aState, evictable));
      theCurSize++;
    }
    return find(anAddress);
  }

  virtual boost::intrusive_ptr<MemoryMessage> pop() {
    DBG_Assert(!theEvictions.empty());
    boost::intrusive_ptr<MemoryMessage> retval =
        new MemoryMessage(theEvictions.front().theType, theEvictions.front().theBlockAddress);
    DBG_(Iface, (<< "Evict buffer popping entry for " << theEvictions.front().theBlockAddress));
    DBG_Assert(theEvictions.front().evictable());
    theEvictions.pop_front();
    theCurSize--;
    return retval;
  }

  virtual boost::intrusive_ptr<MemoryMessage> pop(int32_t index) {
    DBG_Assert(theCurSize > index);
    typename evict_buf_t::iterator iter = theEvictions.begin();
    //    typename evict_buf_t::iterator end = theEvictions.end();

    int32_t anOffset = index;
    while (anOffset > 0) {
      ++iter;
      --anOffset;
    }
    DBG_Assert(iter->evictable());
    boost::intrusive_ptr<MemoryMessage> retval =
        new MemoryMessage(iter->theType, iter->theBlockAddress);
    DBG_(Iface, (<< "Evict buffer getting entry for " << iter->theBlockAddress));
    iter->setPending(true);
    return retval;
  }

  virtual boost::intrusive_ptr<MemoryMessage> firstNonPending() {
    typename evict_buf_t::iterator iter = theEvictions.begin();
    typename evict_buf_t::iterator end = theEvictions.end();

    while ((iter->pending() || !iter->evictable()) && iter != end) {
      ++iter;
    }
    DBG_Assert(iter != end);

    boost::intrusive_ptr<MemoryMessage> retval =
        new MemoryMessage(iter->theType, iter->theBlockAddress);
    DBG_(Iface, (<< "Evict buffer getting entry for " << iter->theBlockAddress
                 << " setting to pending Evict"));
    iter->setPending(true);
    thePendingEvicts++;
    return retval;
  }

  virtual void remove(int32_t index) {
    DBG_Assert(theCurSize > index);
    typename evict_buf_t::iterator iter = theEvictions.begin();
    typename evict_buf_t::iterator end = theEvictions.end();

    int32_t anOffset = index;
    while (anOffset > 0) {
      ++iter;
      --anOffset;
    }
    DBG_Assert(iter->evictable());
    DBG_Assert(iter != end);

    if (iter->pending()) {
      thePendingEvicts--;
    }
    DBG_(Iface, (<< "Evict buffer removing entry for " << iter->theBlockAddress));
    theEvictions.erase(iter);
    theCurSize--;
  }

  // exact address checking should be fine here, since the original writeback
  // request should have been aligned on a block boundary
  iterator find(MemoryAddress const &anAddress) {
    return (theEvictions.template get<1>()).find(anAddress);
  }

  void remove(iterator iter) {
    if (iter != end()) {
      if (iter->pending()) {
        thePendingEvicts--;
      }
      DBG_(Iface, (<< "Evict buffer removing entry for " << iter->theBlockAddress));
      (theEvictions.template get<1>()).erase(iter);
      theCurSize--;
    }
  }

  const_iterator begin() const {
    return (theEvictions.template get<1>()).begin();
  }

  const_iterator end() const {
    return (theEvictions.template get<1>()).end();
  }

  iterator end() {
    return (theEvictions.template get<1>()).end();
  }

  const EvictEntry<_State> &front() const {
    return theEvictions.front();
  }

  const EvictEntry<_State> &back() const {
    return theEvictions.back();
  }

}; // end class EvictBuffer

enum SnoopStates { kSnoopOutstanding = 0, kSnoopWaking, kSnoopWaiting };

struct SnoopEntry : boost::counted_base {
  // boost::intrusive_ptr<MemoryMessage> message;
  mutable MemoryTransport transport;
  enum SnoopStates state;
  MemoryAddress theBlockAddress;
  mutable enum MemoryMessage::MemoryMessageType snoop_state;
  mutable bool i_snoop_outstanding;
  mutable bool d_snoop_outstanding;

  explicit SnoopEntry(MemoryTransport transport, SnoopStates s)
      : transport(transport), state(s), theBlockAddress(transport[MemoryMessageTag]->address()),
        snoop_state(MemoryMessage::ProbedNotPresent), i_snoop_outstanding(false),
        d_snoop_outstanding(true) {
  }
  /*
      explicit SnoopEntry( boost::intrusive_ptr<MemoryMessage> msg, SnoopStates
     s) : message(msg) , state(s) , theBlockAddress(msg->address()) ,
     snoop_state( MemoryMessage::ProbedNotPresent ) , i_snoop_outstanding(false)
     , d_snoop_outstanding(true)
        {}
  */
  SnoopEntry(const SnoopEntry &snp)
      : transport(snp.transport)
        //: message(snp.message)
        ,
        state(snp.state), theBlockAddress(snp.theBlockAddress), snoop_state(snp.snoop_state),
        i_snoop_outstanding(snp.i_snoop_outstanding), d_snoop_outstanding(snp.d_snoop_outstanding) {
  }

  SnoopEntry()
      //: message(0)
      : transport(), state(kSnoopOutstanding), theBlockAddress(0),
        snoop_state(MemoryMessage::ProbedNotPresent), i_snoop_outstanding(false),
        d_snoop_outstanding(true) {
  }
};
typedef boost::intrusive_ptr<SnoopEntry> SnoopEntry_p;

// the snoop buffer contains uninitiated evictions
class SnoopBuffer {
public:
  typedef multi_index_container<
      SnoopEntry, indexed_by<ordered_non_unique<composite_key<
                      SnoopEntry, member<SnoopEntry, MemoryAddress, &SnoopEntry::theBlockAddress>,
                      member<SnoopEntry, SnoopStates, &SnoopEntry::state>>>>>
      snoop_buf_t;

  typedef snoop_buf_t::iterator snoop_iter;

private:
  snoop_buf_t theSnoops;

  std::list<SnoopEntry> theWakeList;

  int32_t theSize;
  int32_t theReserve;
  int32_t theCurSize;

public:
  SnoopBuffer(int32_t aSize) : theSize(aSize), theReserve(0), theCurSize(0) {
  }

  bool empty() const {
    return theCurSize == 0;
  }

  bool full() const {
    return ((theCurSize + theReserve) >= theSize);
  }

  void reserve() {
    DBG_Assert((theCurSize + theReserve) < theSize,
               (<< theCurSize << " + " << theReserve << ">= " << theSize));
    theReserve++;
  }
  void unreserve() {
    DBG_Assert(theReserve > 0);
    theReserve--;
  }

  int32_t size() const {
    return theSize;
  }
  int32_t reservedEntries() const {
    return theReserve;
  }
  int32_t freeEntries() const {
    return theSize - theCurSize;
  }

  snoop_iter getActiveEntry(MemoryAddress const &anAddress) {
    return theSnoops.find(std::tuple<MemoryAddress, SnoopStates>(anAddress, kSnoopOutstanding));
  }

  std::pair<snoop_iter, snoop_iter> getWaitingEntries(MemoryAddress const &anAddress) {
    return theSnoops.equal_range(std::tuple<MemoryAddress, SnoopStates>(anAddress, kSnoopWaiting));
  }

  snoop_iter end() {
    return theSnoops.end();
  }

  // exact address checking should be fine here, since the original snoop
  // request should have been aligned on a block boundary
  snoop_iter findEntry(MemoryAddress const &anAddress) {

    snoop_iter iter = theSnoops.find(std::make_tuple(anAddress, kSnoopOutstanding));
    return iter;
  }

  bool hasEntry(MemoryAddress const &anAddress) {
    snoop_iter iter = theSnoops.find(std::make_tuple(anAddress, kSnoopOutstanding));
    if (iter != theSnoops.end()) {
      return true;
    }

    iter = theSnoops.find(std::make_tuple(anAddress, kSnoopWaiting));
    if (iter != theSnoops.end()) {
      return true;
    }
    std::list<SnoopEntry>::iterator wake_iter = theWakeList.begin();
    for (; wake_iter != theWakeList.end(); wake_iter++) {
      if (wake_iter->theBlockAddress == anAddress) {
        return true;
      }
    }
    return false;
  }

  bool hasSnoopsOutstanding(MemoryAddress const &anAddress) {
    snoop_iter iter = theSnoops.find(std::make_tuple(anAddress, kSnoopOutstanding));
    if (iter != theSnoops.end()) {
      return true;
    }
    return false;
  }

  // snoop_iter allocEntry(boost::intrusive_ptr<MemoryMessage> newMessage) {
  snoop_iter allocEntry(MemoryTransport newTransport) {
    DBG_Assert(theCurSize < theSize);
    theCurSize++;
    std::pair<snoop_iter, bool> ret = theSnoops.insert(SnoopEntry(newTransport, kSnoopOutstanding));
    DBG_Assert(ret.second,
               (<< "Failed to allocate SnoopBuffer entry for " << *newTransport[MemoryMessageTag]));
    return ret.first;
  }

  // snoop_iter addWaitingEntry(boost::intrusive_ptr<MemoryMessage> newMessage)
  // {
  snoop_iter addWaitingEntry(MemoryTransport newTransport) {
    DBG_Assert(theCurSize < theSize);
    theCurSize++;
    std::pair<snoop_iter, bool> ret = theSnoops.insert(SnoopEntry(newTransport, kSnoopWaiting));
    DBG_Assert(ret.second,
               (<< "Failed to allocate SnoopBuffer entry for " << *newTransport[MemoryMessageTag]));
    return ret.first;
  }

  bool wakeWaitingEntries(MemoryAddress const &anAddress) {
    snoop_iter iter, end;
    std::tie(iter, end) = getWaitingEntries(anAddress);
    if (iter != end) {
      theWakeList.insert(theWakeList.end(), iter, end);
      theSnoops.erase(iter, end);
      return true;
    }
    return false;
  }

  void remove(snoop_iter entry) {
    theSnoops.erase(entry);
    theCurSize--;
  }

  bool wakeListEmpty() {
    return theWakeList.empty();
  }

  // boost::intrusive_ptr<MemoryMessage> wakeSnoop() {
  MemoryTransport wakeSnoop() {
    MemoryTransport ret = theWakeList.front().transport;
    theWakeList.pop_front();
    theCurSize--;
    return ret;
  }

  void dump() const {
    DBG_(Dev, (<< "SnoopBuffer: Size = " << theSize << ", CurSize = " << theCurSize
               << ", Reserve = " << theReserve));
    snoop_iter iter = theSnoops.begin();
    for (int32_t i = 0; iter != theSnoops.end(); iter++) {
      DBG_(Dev, (<< "Snoop " << i << ": State " << (int)iter->state << ", Msg "
                 << *(iter->transport[MemoryMessageTag])));
    }
    std::list<SnoopEntry>::const_iterator w_iter = theWakeList.begin();
    for (int32_t i = 0; w_iter != theWakeList.end(); w_iter++) {
      DBG_(Dev, (<< "WakeList " << i << ": " << *(w_iter->transport[MemoryMessageTag])));
    }
  }

}; // end class SnoopBuffer

} // end namespace nCMPCache

#endif // _CMPCACHE_CACHEBUFFERS_HPP
