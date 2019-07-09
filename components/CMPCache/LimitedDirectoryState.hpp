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

#ifndef __LIMITED_DIRECTORY_STATE_HPP__
#define __LIMITED_DIRECTORY_STATE_HPP__

#include <boost/dynamic_bitset.hpp>

#include <components/CMPCache/SimpleDirectoryState.hpp>

namespace nCMPCache {

class LimitedDirectoryState {
private:
  // no one ever calls this, so we always have a size
  LimitedDirectoryState() : theSharers(), theNumSharers(0) {
    DBG_Assert(false);
  }

  boost::dynamic_bitset<> theSharers;
  int theNumSharers;
  int32_t theNumPointers;
  int32_t theGranularity;
  bool coarse_vector;

  typedef boost::dynamic_bitset<>::size_type size_type;

public:
  boost::dynamic_bitset<> &getSharers() {
    return theSharers;
  }

  inline bool isSharer(int32_t sharer) const {
    return theSharers[sharer];
  }
  inline bool noSharers() const {
    return theSharers.none();
  }
  inline bool oneSharer() const {
    return (theSharers.count() == 1);
  }
  inline bool manySharers() const {
    return (theSharers.count() > 1);
  }
  inline int32_t countSharers() const {
    return theSharers.count();
  }

  void getOtherSharers(std::list<int> &list, int32_t exclude) const {
    size_type n = theSharers.find_first();
    const size_type end = boost::dynamic_bitset<>::npos;
    for (; n != end; n = theSharers.find_next(n)) {
      if ((int)n != exclude) {
        list.push_back((int)n);
      }
    }
  }

  void getSharerList(std::list<int> &list) const {
    size_type n = theSharers.find_first();
    const size_type end = boost::dynamic_bitset<>::npos;
    for (; n != end; n = theSharers.find_next(n)) {
      list.push_back((int)n);
    }
  }

  inline int32_t getFirstSharer() const {
    size_type first = theSharers.find_first();
    const size_type end = boost::dynamic_bitset<>::npos;
    if (first == end) {
      return -1;
    } else {
      return (int)first;
    }
  }

  inline void removeSharer(int32_t sharer) {
    DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
    if (!coarse_vector) {
      theSharers.set(sharer, false);
    }
  }
  inline void addSharer(int32_t sharer) {
    DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
    if (!coarse_vector) {
      theSharers.set(sharer, true);
      if ((int)theSharers.count() > theNumPointers) {
        std::list<int> slist;
        getSharerList(slist);
        theSharers.reset();
        while (!slist.empty()) {
          if (!theSharers[slist.front()]) {
            int32_t base = (slist.front() / theGranularity) * theGranularity;
            for (int32_t i = 0; i < theGranularity; i++) {
              theSharers.set(base + i, true);
            }
          }
          slist.pop_front();
        }
        coarse_vector = true;
      }
    } else if (!theSharers[sharer]) {
      int32_t base = (sharer / theGranularity) * theGranularity;
      for (int32_t i = 0; i < theGranularity; i++) {
        theSharers.set(base + i, true);
      }
    }
  }
  inline void setSharer(int32_t sharer) {
    theSharers.reset();
    theSharers.set(sharer, true);
    coarse_vector = false;
  }
  inline void reset() {
    theSharers.reset();
    coarse_vector = false;
  }
  inline void reset(int32_t numSharers) {
    theSharers.resize(numSharers);
    theSharers.reset();
    coarse_vector = false;
  }

  LimitedDirectoryState &operator=(uint64_t s) {
    for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
      theSharers[i] = (s & 1 ? true : false);
    }
    if ((int)theSharers.count() > theNumPointers) {
      std::list<int> slist;
      getSharerList(slist);
      theSharers.reset();
      while (!slist.empty()) {
        if (!theSharers[slist.front()]) {
          int32_t base = (slist.front() / theGranularity) * theGranularity;
          for (int32_t i = 0; i < theGranularity; i++) {
            theSharers.set(base + i, true);
          }
        }
        slist.pop_front();
      }
      coarse_vector = true;
    } else {
      coarse_vector = false;
    }
    return *this;
  }

  LimitedDirectoryState &operator|=(uint64_t s) {
    for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
      theSharers[i] = theSharers[i] || (s & 1 ? true : false);
    }
    if ((int)theSharers.count() > theNumPointers) {
      std::list<int> slist;
      getSharerList(slist);
      theSharers.reset();
      while (!slist.empty()) {
        if (!theSharers[slist.front()]) {
          int32_t base = (slist.front() / theGranularity) * theGranularity;
          for (int32_t i = 0; i < theGranularity; i++) {
            theSharers.set(base + i, true);
          }
        }
        slist.pop_front();
      }
      coarse_vector = true;
    } else {
      coarse_vector = false;
    }
    return *this;
  }

  LimitedDirectoryState &operator=(SimpleDirectoryState &s) {
    if ((int)s.getSharers().count() > theNumPointers) {
      theSharers.reset();
      size_type n = s.getSharers().find_first();
      const size_type end = boost::dynamic_bitset<>::npos;
      for (; n != end; n = s.getSharers().find_next(n)) {
        if (!theSharers[n]) {
          int32_t base = (n / theGranularity) * theGranularity;
          for (int32_t i = 0; i < theGranularity; i++) {
            theSharers.set(base + i, true);
          }
        }
      }
      coarse_vector = true;
    } else {
      theSharers = s.getSharers();
      coarse_vector = false;
    }
    return *this;
  }

  LimitedDirectoryState &operator&=(LimitedDirectoryState &s) {
    theSharers &= s.theSharers;
    if ((int)theSharers.count() > theNumPointers) {
      boost::dynamic_bitset<> temp_sharers(theSharers);
      theSharers.reset();
      size_type n = temp_sharers.find_first();
      const size_type end = boost::dynamic_bitset<>::npos;
      for (; n != end; n = temp_sharers.find_next(n)) {
        if (!theSharers[n]) {
          int32_t base = (n / theGranularity) * theGranularity;
          for (int32_t i = 0; i < theGranularity; i++) {
            theSharers.set(base + i, true);
          }
        }
      }
      coarse_vector = true;
    } else {
      coarse_vector = false;
    }
    return *this;
  }

  inline operator SimpleDirectoryState() const {
    return SimpleDirectoryState(theNumSharers, theSharers);
  }

  LimitedDirectoryState(const LimitedDirectoryState &s)
      : theSharers(s.theSharers), theNumSharers(s.theNumSharers), theNumPointers(s.theNumPointers),
        theGranularity(s.theGranularity), coarse_vector(s.coarse_vector) {
  }

  LimitedDirectoryState(int32_t aNumSharers = 64, int32_t aNumPointers = 1,
                        int32_t aGranularity = 1)
      : theSharers(aNumSharers), theNumSharers(aNumSharers), theNumPointers(aNumPointers),
        theGranularity(aGranularity), coarse_vector(false) {
    DBG_Assert(aNumPointers > 0);
  }

  LimitedDirectoryState(const SimpleDirectoryState &aState, int32_t aNumPointers,
                        int32_t aGranularity)
      : theSharers(aState.getSharers()), theNumSharers(aState.getNumSharers()),
        theNumPointers(aNumPointers), theGranularity(aGranularity), coarse_vector(false) {
    DBG_Assert(aNumPointers > 0);
    if ((int)theSharers.count() > theNumPointers) {
      theSharers.reset();
      size_type n = aState.getSharers().find_first();
      const size_type end = boost::dynamic_bitset<>::npos;
      for (; n != end; n = aState.getSharers().find_next(n)) {
        if (!theSharers[n]) {
          int32_t base = (n / theGranularity) * theGranularity;
          for (int32_t i = 0; i < theGranularity; i++) {
            theSharers.set(base + i, true);
          }
        }
      }
      coarse_vector = true;
    }
  }
};

inline void setPresence(int32_t sharer, const boost::dynamic_bitset<> &presence,
                        const boost::dynamic_bitset<> &exclusive,
                        std::vector<LimitedDirectoryState> &state) {
  for (uint32_t i = 0; i < presence.size(); i++) {
    if (presence[i]) {
      if (exclusive[i]) {
        state[i].setSharer(sharer);
      } else {
        state[i].addSharer(sharer);
      }
    } else {
      state[i].removeSharer(sharer);
    }
  }
}

}; // namespace nCMPCache

#endif // __LIMITED_DIRECTORY_STATE_HPP__
