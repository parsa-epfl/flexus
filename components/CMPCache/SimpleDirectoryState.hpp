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

#ifndef __SIMPLE_DIRECTORY_STATE_HPP__
#define __SIMPLE_DIRECTORY_STATE_HPP__
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#define MAX_NUM_SHARERS 512

namespace nCMPCache {

class SimpleDirectoryState {
private:
  // no one ever calls this, so we always have a size
  SimpleDirectoryState() : theSharers(), theNumSharers(0) {
  }

  boost::dynamic_bitset<uint64_t> theSharers;
  int theNumSharers;

  typedef boost::dynamic_bitset<uint64_t>::size_type size_type;

public:
  const boost::dynamic_bitset<uint64_t> &getSharers() const {
    return theSharers;
  }
  int32_t getNumSharers() const {
    return theNumSharers;
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
    const size_type end = boost::dynamic_bitset<uint64_t>::npos;
    for (; n != end; n = theSharers.find_next(n)) {
      if ((int)n != exclude) {
        list.push_back((int)n);
      }
    }
  }

  void getSharerList(std::list<int> &list) const {
    size_type n = theSharers.find_first();
    const size_type end = boost::dynamic_bitset<uint64_t>::npos;
    for (; n != end; n = theSharers.find_next(n)) {
      list.push_back((int)n);
    }
  }

  inline int32_t getFirstSharer() const {
    size_type first = theSharers.find_first();
    const size_type end = boost::dynamic_bitset<uint64_t>::npos;
    if (first == end) {
      return -1;
    } else {
      return (int)first;
    }
  }
  inline int32_t getNextSharer(int32_t sharer) const {
    size_type first = theSharers.find_next(sharer);
    const size_type end = boost::dynamic_bitset<uint64_t>::npos;
    if (first == end) {
      return -1;
    } else {
      return (int)first;
    }
  }

  inline void removeSharer(int32_t sharer) {
    DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
    theSharers.set(sharer, false);
  }
  inline void addSharer(int32_t sharer) {
    DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
    theSharers.set(sharer, true);
  }
  inline void setSharer(int32_t sharer) {
    theSharers.reset();
    theSharers.set(sharer, true);
  }
  inline void reset() {
    theSharers.reset();
  }
  inline void reset(int32_t numSharers) {
    theSharers.resize(numSharers);
    theSharers.reset();
    theNumSharers = numSharers;
  }

  SimpleDirectoryState &operator=(uint64_t s) {
    for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
      theSharers[i] = (((s & 1) == 1) ? true : false);
    }
    return *this;
  }

  SimpleDirectoryState &operator=(std::bitset<MAX_NUM_SHARERS> s) {
    for (int32_t i = 0; i < theNumSharers; i++) {
      theSharers[i] = s[i];
    }
    return *this;
  }

  SimpleDirectoryState &operator|=(std::bitset<MAX_NUM_SHARERS> s) {
    for (int32_t i = 0; i < theNumSharers; i++) {
      theSharers[i] |= s[i];
    }
    return *this;
  }

  SimpleDirectoryState &operator|=(uint64_t s) {
    for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
      theSharers[i] = theSharers[i] || (((s & 1) == 1) ? true : false);
    }
    return *this;
  }

  SimpleDirectoryState(const boost::dynamic_bitset<uint64_t> &sharers,
                       const SimpleDirectoryState &s)
      : theSharers(sharers), theNumSharers(s.theNumSharers) {
  }

  SimpleDirectoryState(const SimpleDirectoryState &s)
      : theSharers(s.theSharers), theNumSharers(s.theNumSharers) {
  }

  SimpleDirectoryState(int32_t aNumSharers) : theSharers(aNumSharers), theNumSharers(aNumSharers) {
  }
  SimpleDirectoryState(int32_t aNumSharers, const boost::dynamic_bitset<uint64_t> &aSharers)
      : theSharers(aSharers), theNumSharers(aNumSharers) {
  }

  SimpleDirectoryState &operator&=(SimpleDirectoryState &a) {
    theSharers &= a.theSharers;
    return *this;
  }
};

inline bool isNonShared(const std::vector<SimpleDirectoryState> &state, int32_t requester) {
  if (state.empty()) {
    return true;
  }
  std::vector<SimpleDirectoryState>::const_iterator iter = state.begin();
  boost::dynamic_bitset<uint64_t> sharers(iter->getSharers());
  for (iter++; iter != state.end(); iter++) {
    sharers |= iter->getSharers();
  }
  return (sharers.none() || (sharers[requester] && (sharers.count() == 1)));
}

inline bool isNonShared(const std::vector<boost::dynamic_bitset<uint64_t>> &state,
                        int32_t requester) {
  if (state.empty()) {
    return true;
  }
  std::vector<boost::dynamic_bitset<uint64_t>>::const_iterator iter = state.begin();
  boost::dynamic_bitset<uint64_t> sharers(*iter);
  for (iter++; iter != state.end(); iter++) {
    sharers |= *iter;
  }
  return (sharers.none() || (sharers[requester] && (sharers.count() == 1)));
}

inline boost::dynamic_bitset<uint64_t>
getPresenceVector(const std::vector<SimpleDirectoryState> &state) {
  boost::dynamic_bitset<uint64_t> present(state.size(), 0);
  for (uint32_t i = 0; i < state.size(); i++) {
    present[i] = !state[i].noSharers();
  }
  return present;
}

inline void presence2State(const boost::dynamic_bitset<uint64_t> &presence,
                           std::vector<SimpleDirectoryState> &state,
                           const SimpleDirectoryState &defaultState, int32_t sharer) {
  state.clear();
  state.resize(presence.size(), defaultState);
  for (uint32_t i = 0; i < presence.size(); i++) {
    if (presence[i]) {
      state[i].setSharer(sharer);
    }
  }
}

inline SimpleDirectoryState bits2State(const boost::dynamic_bitset<uint64_t> &aState,
                                       const SimpleDirectoryState &aDefaultState) {
  return SimpleDirectoryState(aState, aDefaultState);
}

inline void copyState(const std::vector<SimpleDirectoryState> &orig,
                      std::vector<boost::dynamic_bitset<uint64_t>> &copy) {
  // transform(orig.begin(), orig.end(), copy.begin(), [](auto& x){ return
  // x.getSharers(); });//(&SimpleDirectoryState::getSharers, _1));
  transform(orig.begin(), orig.end(), copy.begin(),
            boost::bind(&SimpleDirectoryState::getSharers, _1));
}

inline void copyState(const std::vector<boost::dynamic_bitset<uint64_t>> &orig,
                      std::vector<SimpleDirectoryState> &copy,
                      const SimpleDirectoryState &aDefaultState) {
  // transform(orig.begin(), orig.end(), copy.begin(), [&aDefaultState](auto&
  // x){ return bits2State(x, aDefaultState); });//std::bind(&bits2State, _1,
  // aDefaultState));
  transform(orig.begin(), orig.end(), copy.begin(), boost::bind(&bits2State, _1, aDefaultState));
}

inline void setPresence(int32_t sharer, const boost::dynamic_bitset<uint64_t> &presence,
                        const boost::dynamic_bitset<uint64_t> &exclusive,
                        std::vector<SimpleDirectoryState> &state) {
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

#endif // __SIMPLE_DIRECTORY_STATE_HPP__
