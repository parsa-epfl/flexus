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

#ifndef __LIMITED_DIRECTORY_STATE_HPP__
#define __LIMITED_DIRECTORY_STATE_HPP__

#include <boost/dynamic_bitset.hpp>
#include <components/CMPCache/SimpleDirectoryState.hpp>

namespace nCMPCache {

class LimitedDirectoryState
{
  private:
    // no one ever calls this, so we always have a size
    LimitedDirectoryState()
      : theSharers()
      , theNumSharers(0)
    {
        DBG_Assert(false);
    }

    boost::dynamic_bitset<> theSharers;
    int theNumSharers;
    int32_t theNumPointers;
    int32_t theGranularity;
    bool coarse_vector;

    typedef boost::dynamic_bitset<>::size_type size_type;

  public:
    boost::dynamic_bitset<>& getSharers() { return theSharers; }

    inline bool isSharer(int32_t sharer) const { return theSharers[sharer]; }
    inline bool noSharers() const { return theSharers.none(); }
    inline bool oneSharer() const { return (theSharers.count() == 1); }
    inline bool manySharers() const { return (theSharers.count() > 1); }
    inline int32_t countSharers() const { return theSharers.count(); }

    void getOtherSharers(std::list<int>& list, int32_t exclude) const
    {
        size_type n         = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<>::npos;
        for (; n != end; n = theSharers.find_next(n)) {
            if ((int)n != exclude) { list.push_back((int)n); }
        }
    }

    void getSharerList(std::list<int>& list) const
    {
        size_type n         = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<>::npos;
        for (; n != end; n = theSharers.find_next(n)) {
            list.push_back((int)n);
        }
    }

    inline int32_t getFirstSharer() const
    {
        size_type first     = theSharers.find_first();
        const size_type end = boost::dynamic_bitset<>::npos;
        if (first == end) {
            return -1;
        } else {
            return (int)first;
        }
    }

    inline void removeSharer(int32_t sharer)
    {
        DBG_Assert(sharer < (int)theSharers.size(), (<< sharer << " >= " << theSharers.size()));
        if (!coarse_vector) { theSharers.set(sharer, false); }
    }
    inline void addSharer(int32_t sharer)
    {
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
    inline void setSharer(int32_t sharer)
    {
        theSharers.reset();
        theSharers.set(sharer, true);
        coarse_vector = false;
    }
    inline void reset()
    {
        theSharers.reset();
        coarse_vector = false;
    }
    inline void reset(int32_t numSharers)
    {
        theSharers.resize(numSharers);
        theSharers.reset();
        coarse_vector = false;
    }

    LimitedDirectoryState& operator=(uint64_t s)
    {
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

    LimitedDirectoryState& operator|=(uint64_t s)
    {
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

    LimitedDirectoryState& operator=(SimpleDirectoryState& s)
    {
        if ((int)s.getSharers().count() > theNumPointers) {
            theSharers.reset();
            size_type n         = s.getSharers().find_first();
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
            theSharers    = s.getSharers();
            coarse_vector = false;
        }
        return *this;
    }

    LimitedDirectoryState& operator&=(LimitedDirectoryState& s)
    {
        theSharers &= s.theSharers;
        if ((int)theSharers.count() > theNumPointers) {
            boost::dynamic_bitset<> temp_sharers(theSharers);
            theSharers.reset();
            size_type n         = temp_sharers.find_first();
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

    inline operator SimpleDirectoryState() const { return SimpleDirectoryState(theNumSharers, theSharers); }

    LimitedDirectoryState(const LimitedDirectoryState& s)
      : theSharers(s.theSharers)
      , theNumSharers(s.theNumSharers)
      , theNumPointers(s.theNumPointers)
      , theGranularity(s.theGranularity)
      , coarse_vector(s.coarse_vector)
    {
    }

    LimitedDirectoryState(int32_t aNumSharers = 64, int32_t aNumPointers = 1, int32_t aGranularity = 1)
      : theSharers(aNumSharers)
      , theNumSharers(aNumSharers)
      , theNumPointers(aNumPointers)
      , theGranularity(aGranularity)
      , coarse_vector(false)
    {
        DBG_Assert(aNumPointers > 0);
    }

    LimitedDirectoryState(const SimpleDirectoryState& aState, int32_t aNumPointers, int32_t aGranularity)
      : theSharers(aState.getSharers())
      , theNumSharers(aState.getNumSharers())
      , theNumPointers(aNumPointers)
      , theGranularity(aGranularity)
      , coarse_vector(false)
    {
        DBG_Assert(aNumPointers > 0);
        if ((int)theSharers.count() > theNumPointers) {
            theSharers.reset();
            size_type n         = aState.getSharers().find_first();
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

inline void
setPresence(int32_t sharer,
            const boost::dynamic_bitset<>& presence,
            const boost::dynamic_bitset<>& exclusive,
            std::vector<LimitedDirectoryState>& state)
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

#endif // __LIMITED_DIRECTORY_STATE_HPP__
