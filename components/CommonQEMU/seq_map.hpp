#ifndef SEQ_MAP__
#define SEQ_MAP__

#include <deque>
#include <map>
#define _BACKWARD_BACKWARD_WARNING_H
#include <ext/hash_map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

using namespace boost::multi_index;

struct by_LRU {};
struct by_index {};

template <class T_key, class T_val>
struct MapEntry_T {
  MapEntry_T(const std::pair<T_key, T_val> & apair)
    : first(apair.first)
    , second(apair.second)
  {}
  T_key first;
  mutable T_val second;
};

template <class T_key, class T_val>
class flexus_boost_seq_map {
  typedef MapEntry_T<T_key, T_val> MapEntry;

  typedef multi_index_container
  < MapEntry
  , indexed_by
  < sequenced< tag<by_LRU> >
  , ordered_unique
  < tag<by_index>
  , member<MapEntry, T_key, &MapEntry::first>
  >
  >
  > MapTable;
  typedef typename MapTable::template index<by_LRU>::type::iterator LruIter;
  typedef typename MapTable::template index<by_index>::type::iterator IndexIter;

  MapTable theMap;

public:
  typedef IndexIter iterator;
  typedef LruIter seq_iter;
  typedef typename MapTable::size_type size_type;

  iterator begin() {
    return theMap.get<by_index>().begin();
  }
  iterator end() {
    return theMap.get<by_index>().end();
  }

  seq_iter beginSeq() {
    return theMap.get<by_LRU>().begin();
  }
  seq_iter endSeq() {
    return theMap.get<by_LRU>().end();
  }

  size_type size() const {
    return theMap.size();
  }

  const T_val & front() const {
    return theMap.get<by_LRU>().front().second;
  }

  const T_key & front_key() const {
    return theMap.get<by_LRU>().front().first;
  }

  std::pair<iterator, bool> insert( const std::pair<T_key, T_val> & apair ) {
    std::pair<LruIter, bool> inspair = theMap.get<by_LRU>().push_back(apair);
    IndexIter iter = theMap.project<by_index>(inspair.first);
    return std::make_pair(iter, inspair.second);
  }

  iterator find(const T_key & key) const {
    return theMap.get<by_index>().find(key);
  }

  seq_iter findSeq(const T_key & key) const {
    return theMap.project<by_LRU>( theMap.get<by_index>().find(key) );
  }

  void erase(iterator iter) {
    theMap.get<by_index>().erase(iter);
  }

  void eraseSeq(seq_iter iter) {
    theMap.get<by_LRU>().erase(iter);
  }

  void push_back( const std::pair<T_key, T_val> & apair ) {
    theMap.get<by_LRU>().push_back(apair);
  }

  void pop_front() {
    theMap.get<by_LRU>().pop_front();
  }

  void move_back(iterator const & iter) {
    theMap.relocate( theMap.get<by_LRU>().end(), theMap.project<by_LRU>(iter) );
  }

  unsigned dist_back(iterator const & iter) const {
    LruIter pos = theMap.project<by_LRU>(iter);
    unsigned dist = 0;
    while (pos != theMap.get<by_LRU>().end()) {
      dist++;
      pos++;
    }
    return dist;
  }
};

#define FLEXUS_BOOST_SET_ASSOC_STATS 0

template <class T_key, class T_val>
class flexus_boost_set_assoc {
  typedef flexus_boost_seq_map<T_key, T_val> MapTable;
  typedef std::vector<MapTable> SetAssocTable;

  SetAssocTable theTable;
  T_key theCurrIndex;
#ifdef FLEXUS_BOOST_SET_ASSOC_STATS
  std::vector<int> theSetCounts;
#endif

  T_key theIndexMask;
  T_key theUsefulBottomMask;
  uint32_t theSets;
  uint32_t theAssoc;
  uint32_t theUselessNextBits;

  T_key makeIndex(const T_key key) {
    return (key & theIndexMask);
  }

public:
  typedef typename MapTable::iterator iterator;
  typedef typename MapTable::seq_iter seq_iter;
  typedef typename MapTable::size_type size_type;

  void init(uint32_t size, uint32_t assoc, uint32_t usefulBottomBits) {
    theSets = size / assoc;
    theAssoc = assoc;
    theTable.resize(theSets);
    theCurrIndex = 0;
    theIndexMask = theSets - 1;
#ifdef FLEXUS_BOOST_SET_ASSOC_STATS
    theSetCounts.resize(theSets);
#endif
  }

  size_type sets() const {
    return theSets;
  }

  size_type assoc() const {
    return theAssoc;
  }

  iterator end() {
    return theTable[theCurrIndex].end();
  }

  iterator index_begin() {
    theCurrIndex = 0;
    iterator iter = theTable[theCurrIndex].begin();
    index_adv(iter);
    return iter;
  }
  void index_next(iterator & iter) {
    ++iter;
    index_adv(iter);
  }
  void index_adv(iterator & iter) {
    while (iter == theTable[theCurrIndex].end()) {
      // check if there is another set
      if ( (theCurrIndex + 1) == makeIndex(theCurrIndex + 1) ) {
        theCurrIndex++;
        iter = theTable[theCurrIndex].begin();
      } else {
        break;
      }
    }
  }
  iterator index_end() {
    return theTable[theCurrIndex].end();
  }

  seq_iter seq_begin() {
    theCurrIndex = 0;
    seq_iter iter = theTable[theCurrIndex].beginSeq();
    seq_adv(iter);
    return iter;
  }
  void seq_next(seq_iter & iter) {
    ++iter;
    seq_adv(iter);
  }
  void seq_adv(seq_iter & iter) {
    while (iter == theTable[theCurrIndex].endSeq()) {
      // check if there is another set
      if ( (theCurrIndex + 1) == makeIndex(theCurrIndex + 1) ) {
        theCurrIndex++;
        iter = theTable[theCurrIndex].beginSeq();
      } else {
        break;
      }
    }
  }
  seq_iter seq_end() {
    return theTable[theCurrIndex].endSeq();
  }

  size_type size() const {
    return theTable[theCurrIndex].size();
  }

  const T_val & front() const {
    return theTable[theCurrIndex].front();
  }

  const T_key & front_key() const {
    return theTable[theCurrIndex].front_key();
  }

  std::pair<iterator, bool> insert( const std::pair<T_key, T_val> & apair ) {
    theCurrIndex = makeIndex(apair.first);
#ifdef FLEXUS_BOOST_SET_ASSOC_STATS
    theSetCounts[theCurrIndex]++;
#endif
    return theTable[theCurrIndex].insert(apair);
  }

  iterator find(const T_key & key) {
    theCurrIndex = makeIndex(key);
    return theTable[theCurrIndex].find(key);
  }

  void erase(iterator iter) {
    theTable[theCurrIndex].erase(iter);
  }

  void push_back( const std::pair<T_key, T_val> & apair ) {
    theCurrIndex = makeIndex(apair.first);
#ifdef FLEXUS_BOOST_SET_ASSOC_STATS
    theSetCounts[theCurrIndex]++;
#endif
    theTable[theCurrIndex].push_back(apair);
  }

  void pop_front() {
    theTable[theCurrIndex].pop_front();
  }

  void move_back(iterator const & iter) {
    theTable[theCurrIndex].move_back(iter);
  }

#ifdef FLEXUS_BOOST_SET_ASSOC_STATS
  std::vector<int> & get_counts() {
    return theSetCounts;
  }
#endif
};

#endif
