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

#ifndef __SIMPLE_DIRECTORY_STATE_HPP__
#define __SIMPLE_DIRECTORY_STATE_HPP__
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#define MAX_NUM_SHARERS 128

namespace nCMPCache {

class SimpleDirectoryState
{
public:
    // no one ever calls this, so we always have a size
    SimpleDirectoryState(uint32_t nb_sharers = MAX_NUM_SHARERS)
      : theSharers()
      , theNumSharers(nb_sharers) {}

  private:
    boost::dynamic_bitset<uint64_t> theSharers;
    int theNumSharers;

    typedef boost::dynamic_bitset<uint64_t>::size_type size_type;

  public:
    const boost::dynamic_bitset<uint64_t>& getSharers() const { return theSharers; }
    int32_t getNumSharers() const { return theNumSharers; }

    inline bool isSharer(int32_t sharer) const { return theSharers[sharer]; }
    inline bool noSharers() const { return theSharers.none(); }
    inline bool oneSharer() const { return (theSharers.count() == 1); }
    inline bool manySharers() const { return (theSharers.count() > 1); }
    inline int32_t countSharers() const { return theSharers.count(); }

    void getOtherSharers(std::list<int>& list, int32_t exclude) const
    {
        size_type n         = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<uint64_t>::npos;
        for (; n != end; n = theSharers.find_next(n)) {
            if ((int)n != exclude) { list.push_back((int)n); }
        }
    }

    void getSharerList(std::list<int>& list) const
    {
        size_type n         = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<uint64_t>::npos;
        for (; n != end; n = theSharers.find_next(n)) {
            list.push_back((int)n);
        }
    }

    inline int32_t getFirstSharer() const
    {
        size_type first     = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<uint64_t>::npos;
        if (first == end) {
            return -1;
        } else {
            return (int)first;
        }
    }
    inline int32_t getNextSharer(int32_t sharer) const
    {
        size_type first     = theSharers.find_next(sharer);
        const size_type end = boost::dynamic_bitset<uint64_t>::npos;
        if (first == end) {
            return -1;
        } else {
            return (int)first;
        }
    }

    inline void removeSharer(int32_t sharer)
    {
        DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
        theSharers.set(sharer, false);
    }
    inline void addSharer(int32_t sharer)
    {
        DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
        theSharers.set(sharer, true);
    }
    inline void setSharer(int32_t sharer)
    {
        theSharers.reset();
        theSharers.set(sharer, true);
    }
    inline void reset() { theSharers.reset(); }
    inline void reset(int32_t numSharers)
    {
        theSharers.resize(numSharers);
        theSharers.reset();
        theNumSharers = numSharers;
    }

    SimpleDirectoryState& operator=(uint64_t s)
    {
        for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
            theSharers[i] = (((s & 1) == 1) ? true : false);
        }
        return *this;
    }

    SimpleDirectoryState& operator=(std::bitset<MAX_NUM_SHARERS> s)
    {
        for (int32_t i = 0; i < theNumSharers; i++) {
            theSharers[i] = s[i];
        }
        return *this;
    }

    SimpleDirectoryState& operator|=(std::bitset<MAX_NUM_SHARERS> s)
    {
        for (int32_t i = 0; i < theNumSharers; i++) {
            theSharers[i] |= s[i];
        }
        return *this;
    }

    SimpleDirectoryState& operator|=(uint64_t s)
    {
        for (int32_t i = 0; i < theNumSharers; i++, s >>= 1) {
            theSharers[i] = theSharers[i] || (((s & 1) == 1) ? true : false);
        }
        return *this;
    }

    SimpleDirectoryState(const boost::dynamic_bitset<uint64_t>& sharers, const SimpleDirectoryState& s)
      : theSharers(sharers)
      , theNumSharers(s.theNumSharers)
    {
    }

    SimpleDirectoryState(const SimpleDirectoryState& s)
      : theSharers(s.theSharers)
      , theNumSharers(s.theNumSharers)
    {
    }

    SimpleDirectoryState(int32_t aNumSharers)
      : theSharers(aNumSharers)
      , theNumSharers(aNumSharers)
    {
    }
    SimpleDirectoryState(int32_t aNumSharers, const boost::dynamic_bitset<uint64_t>& aSharers)
      : theSharers(aSharers)
      , theNumSharers(aNumSharers)
    {
    }


    SimpleDirectoryState& operator&=(SimpleDirectoryState& a)
    {
        theSharers &= a.theSharers;
        return *this;
    }
};

inline bool
isNonShared(const std::vector<SimpleDirectoryState>& state, int32_t requester)
{
    if (state.empty()) { return true; }
    std::vector<SimpleDirectoryState>::const_iterator iter = state.begin();
    boost::dynamic_bitset<uint64_t> sharers(iter->getSharers());
    for (iter++; iter != state.end(); iter++) {
        sharers |= iter->getSharers();
    }
    return (sharers.none() || (sharers[requester] && (sharers.count() == 1)));
}

inline bool
isNonShared(const std::vector<boost::dynamic_bitset<uint64_t>>& state, int32_t requester)
{
    if (state.empty()) { return true; }
    std::vector<boost::dynamic_bitset<uint64_t>>::const_iterator iter = state.begin();
    boost::dynamic_bitset<uint64_t> sharers(*iter);
    for (iter++; iter != state.end(); iter++) {
        sharers |= *iter;
    }
    return (sharers.none() || (sharers[requester] && (sharers.count() == 1)));
}

inline boost::dynamic_bitset<uint64_t>
getPresenceVector(const std::vector<SimpleDirectoryState>& state)
{
    boost::dynamic_bitset<uint64_t> present(state.size(), 0);
    for (uint32_t i = 0; i < state.size(); i++) {
        present[i] = !state[i].noSharers();
    }
    return present;
}

inline void
presence2State(const boost::dynamic_bitset<uint64_t>& presence,
               std::vector<SimpleDirectoryState>& state,
               const SimpleDirectoryState& defaultState,
               int32_t sharer)
{
    state.clear();
    state.resize(presence.size(), defaultState);
    for (uint32_t i = 0; i < presence.size(); i++) {
        if (presence[i]) { state[i].setSharer(sharer); }
    }
}

inline SimpleDirectoryState
bits2State(const boost::dynamic_bitset<uint64_t>& aState, const SimpleDirectoryState& aDefaultState)
{
    return SimpleDirectoryState(aState, aDefaultState);
}

inline void
copyState(const std::vector<SimpleDirectoryState>& orig, std::vector<boost::dynamic_bitset<uint64_t>>& copy)
{
    transform(orig.begin(),
              orig.end(),
              copy.begin(),
              std::bind(&SimpleDirectoryState::getSharers, std::placeholders::_1));
}

inline void
copyState(const std::vector<boost::dynamic_bitset<uint64_t>>& orig,
          std::vector<SimpleDirectoryState>& copy,
          const SimpleDirectoryState& aDefaultState)
{
    transform(orig.begin(), orig.end(), copy.begin(), std::bind(&bits2State, std::placeholders::_1, aDefaultState));
}

inline void
setPresence(int32_t sharer,
            const boost::dynamic_bitset<uint64_t>& presence,
            const boost::dynamic_bitset<uint64_t>& exclusive,
            std::vector<SimpleDirectoryState>& state)
{
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
