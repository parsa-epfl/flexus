#ifndef __MISS_ADDRESS_FILE_HPP__
#define __MISS_ADDRESS_FILE_HPP__

#include <components/CMPDirectory/SimpleDirectoryState.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

namespace nCMPDirectory {

using namespace Flexus::SharedTypes;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

enum MAFState {
  eWaitSet = 0,
  eWaitRequest = 1,
  eWaitEvict = 2,
  eWaitAck = 3,
  eWaking = 4,
  eInPipeline = 5,
  eWaitRevoke = 6
};

class MissAddressFile;

class MAFEntry {
private:
  MemoryAddress theAddress;
  MAFState  theState;
  mutable MemoryTransport theTransport;
  mutable SimpleDirectoryState theBlockState;

  friend class MissAddressFile;
public:
  MAFEntry(MemoryTransport & aTransport, MemoryAddress anAddress, MAFState aState, SimpleDirectoryState aBState)
    : theAddress(anAddress), theState(aState), theTransport(aTransport), theBlockState(aBState)
  { }

  const MemoryAddress & address() const {
    return theAddress;
  }
  const MAFState & state() const {
    return theState;
  }
  MemoryTransport & transport() const {
    return theTransport;
  }
  SimpleDirectoryState & blockState() const {
    return theBlockState;
  }
};

class MissAddressFile {
private:
  struct by_state {};
  struct by_order {};
  struct by_address {};

  typedef multi_index_container
  < MAFEntry
  , indexed_by
  < ordered_non_unique
  < tag<by_address>
  , composite_key
  < MAFEntry
  , const_mem_fun< MAFEntry, const MemoryAddress &, &MAFEntry::address >
  , const_mem_fun< MAFEntry, const MAFState &, &MAFEntry::state >
  >
  >
  , ordered_non_unique
  < tag<by_state>
  , const_mem_fun< MAFEntry, const MAFState &, &MAFEntry::state >
  >
  , sequenced< tag<by_order> >
  >
  > maf_t;

  maf_t theMAF;

  int32_t theSize;
  int32_t theReserve;
  int32_t theCurSize;

  /* Extra support for tracking number of entries by region, state, and requesters */

  std::function<MemoryAddress(MemoryAddress)> regionFunc;

  std::map< MemoryAddress, std::map<int, int> > theRegionRecorder;

  typedef std::map< MemoryAddress, std::map<int, int> >::iterator   region_iterator_t;
  typedef std::map< MemoryAddress, std::map<int, int> >::const_iterator const_region_iterator_t;

  MemoryAddress defaultRegionFunc(MemoryAddress addr) {
    return addr;
  }

public:
  MissAddressFile(int32_t aSize) : theSize(aSize), theReserve(0), theCurSize(0) {
    regionFunc = std::bind(&MissAddressFile::defaultRegionFunc, this, _1);
  }

  void registerRegionFunc(std::function<MemoryAddress(MemoryAddress)> func) {
    regionFunc = func;
  }

  typedef maf_t::iterator iterator;
  typedef maf_t::index<by_order>::type::iterator order_iterator;

  iterator find(MemoryAddress address) {
    return theMAF.find( std::make_tuple(address) );
  }

  iterator findFirst(MemoryAddress address, MAFState state) {
    return theMAF.find( std::make_tuple(address, state) );
  }

  std::pair<iterator, iterator> findAll(MemoryAddress address, MAFState state) {
    return theMAF.equal_range( std::make_tuple(address, state) );
  }

  order_iterator ordered_begin() {
    return theMAF.get<by_order>().begin();
  }
  order_iterator ordered_end() {
    return theMAF.get<by_order>().end();
  }

  iterator end() {
    return theMAF.end();
  }

  void reserve() {
    DBG_Assert( (theCurSize + theReserve) < theSize );
    theReserve++;
  }

  void unreserve() {
    DBG_Assert( theReserve > 0 );
    theReserve--;
  }

  bool full() const {
    return ((theCurSize + theReserve) >= theSize);
  }

  bool empty() const {
    return (theCurSize + theReserve) == 0;
  }

  bool usedEntries() const {
    return (theCurSize + theReserve);
  }

  bool activeEntries() const {
    return (theCurSize);
  }

  iterator insert(MemoryTransport & transport, MemoryAddress address, MAFState state, SimpleDirectoryState bstate) {
    std::pair<iterator, bool> ret = theMAF.insert(MAFEntry(transport, address, state, bstate));
    theCurSize++;

    // Track in RegionRecorder
    int32_t requester = transport[DestinationTag]->requester;
    theRegionRecorder[regionFunc(address)][requester]++;
    return ret.first;
  }

  void setState(iterator & entry, MAFState state) {
    theMAF.modify(entry, [state](auto& x){ return x.theState = state; }); //ll::bind( &MAFEntry::theState, ll::_1) = state);
  }

  void remove(iterator & entry) {
    // Track in RegionRecorder
    int32_t requester = entry->theTransport[DestinationTag]->requester;
    region_iterator_t r_iter = theRegionRecorder.find(regionFunc(entry->theAddress));
    DBG_Assert(r_iter != theRegionRecorder.end());
    std::map<int, int>::iterator iter = r_iter->second.find(requester);
    DBG_Assert(iter != r_iter->second.end());
    iter->second--;
    if (iter->second == 0) {
      r_iter->second.erase(iter);
      if (r_iter->second.size() == 0) {
        theRegionRecorder.erase(r_iter);
      }
    }

    theMAF.erase(entry);
    theCurSize--;
  }

  void wake(iterator & entry) {
    theMAF.modify(entry, ll::bind( &MAFEntry::theState, ll::_1) = eWaking);
  }
  void wakeAfterEvict(iterator & entry) {
    theMAF.modify(entry, ll::bind( &MAFEntry::theState, ll::_1) = eWaking);
    iterator first, last, next;
    std::tie(first, last) = theMAF.equal_range( std::make_tuple(entry->address(), eWaitRequest) );
    next = first;
    next++;
    for (; first != theMAF.end() && first != last; first = next, next++) {
      theMAF.modify(first, ll::bind( &MAFEntry::theState, ll::_1) = eWaking);
    }
  }

  void wake(order_iterator & entry) {
    theMAF.get<by_order>().modify(entry, ll::bind( &MAFEntry::theState, ll::_1) = eWaking);
  }

  bool hasWakingEntry() {
    maf_t::index<by_state>::type::iterator iter = theMAF.get<by_state>().find(eWaking);
    return (iter != theMAF.get<by_state>().end());
  }

  iterator peekWakingMAF() {
    // we loop through entries in order so we can prioritize entries on a FCFS basis
    maf_t::index<by_order>::type::iterator iter = theMAF.get<by_order>().begin();
    for (; iter != theMAF.get<by_order>().end() && iter->theState != eWaking; iter++);
    DBG_Assert( iter != theMAF.get<by_order>().end() );
    return theMAF.project<by_address>(iter);
  }
  iterator getWakingMAF() {
    // we loop through entries in order so we can prioritize entries on a FCFS basis
    maf_t::index<by_order>::type::iterator iter = theMAF.get<by_order>().begin();
    for (; iter != theMAF.get<by_order>().end() && iter->theState != eWaking; iter++);
    DBG_Assert( iter != theMAF.get<by_order>().end() );
    theMAF.get<by_order>().modify(iter, ll::bind( &MAFEntry::theState, ll::_1) = eInPipeline);
    return theMAF.project<by_address>(iter);
  }

  // Are there requests from multiple requesters for the given region
  bool otherRegionRequesters(MemoryAddress addr, int32_t requester) {
    // Map to a region
    MemoryAddress region(regionFunc(addr));

    // Do we have requests for this region
    region_iterator_t region_iter = theRegionRecorder.find(addr);
    if (region_iter == theRegionRecorder.end()) {
      return false;
    }

    // Are there requests from multiple requesters?
    int32_t size = region_iter->second.size();
    if (size < 1) {
      return false;
    } else if (size == 1) {
      // There's only requests from one node, is it the same guy who's asking?
      if (region_iter->second.begin()->first == requester) {
        return false;
      }
    }

    // If we got here then there are requests from other regions in progress
    return true;
  }

}; // class MissAddressFile

}; // namespace nCMPDirectory

#endif // __MISS_ADDRESS_FILE_HPP__
