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
#include "seq_map.hpp"

#include <fstream>
#include <memory>
#include <sstream>

using namespace boost::multi_index;

#define DBG_SetDefaultOps AddCat(SpatialPrefetch)
#include DBG_Control()

namespace nGHBPrefetcher {

typedef uint32_t MemoryAddress;

struct ghb_it_ent_t
{
    MemoryAddress tag;
    int32_t ptr;
    ghb_it_ent_t()
      : tag(0)
      , ptr(0)
    {
    }
};
struct ghb_ent_t
{
    MemoryAddress addr;
    int32_t prev;
    ghb_ent_t()
      : addr(0)
      , prev(-1)
    {
    }
};

class GHBPrefetcher
{
    // LRUarray<ghb_it_ent_t,ghb_it_log_nsets,1> ghb_it;
    flexus_boost_set_assoc<MemoryAddress, ghb_it_ent_t> theIndexTable;
    typedef flexus_boost_set_assoc<MemoryAddress, ghb_it_ent_t>::iterator IndexTableIter;
    ghb_ent_t* ghb;
    int32_t ghb_head;

    int64_t ghb_req_id;

    std::string theName;
    int32_t theNodeId;
    uint32_t theBlockSize;
    uint32_t theBlockOffsetMask;
    std::deque<MemoryAddress> thePrefetches;

    uint32_t ghb_size;
    uint32_t ghb_max_idx;
    uint32_t ghb_depth;

  public:
    GHBPrefetcher(std::string statName, int32_t aNode)
      : theName(statName)
      , theNodeId(aNode)
      , ghb_depth(4)
    {
    }

    void init(int32_t blockSize, int32_t ghbSize)
    {
        theBlockSize       = blockSize;
        theBlockOffsetMask = theBlockSize - 1;
        ghb_size           = ghbSize;
        ghb_max_idx        = ghb_size << 4;
        theIndexTable.init(ghb_size, 1, 0);
        ghb = new ghb_ent_t[ghb_size];
    }

    void ghb_prefetch(int64_t req_id, const MemoryAddress addr)
    {
        if (req_id != ghb_req_id) return;
        thePrefetches.push_back(addr);
    }

    bool prefetchReady() { return (!thePrefetches.empty()); }

    MemoryAddress getPrefetch()
    {
        MemoryAddress addr = thePrefetches.front();
        thePrefetches.pop_front();
        return addr;
    }

    void access(const MemoryAddress PC, const MemoryAddress addr)
    {
        int32_t ptr            = -1;
        IndexTableIter it_iter = theIndexTable.find(PC);
        if (it_iter != theIndexTable.end()) {
            ptr = it_iter->second.ptr;
        } else {
            if (theIndexTable.size() >= 1) { theIndexTable.pop_front(); }
            it_iter             = theIndexTable.insert(std::make_pair(PC, ghb_it_ent_t())).first;
            it_iter->second.tag = PC;
        }
        it_iter->second.ptr = ghb_head;
        ghb_ent_t* head_ent = &ghb[ghb_head % ghb_size];
        MemoryAddress baddr = makeBlockAddress(addr);
        head_ent->addr      = baddr;
        head_ent->prev      = ptr;
        ghb_head            = (ghb_head + 1) % ghb_max_idx;

        int32_t ghb_deltas[ghb_size - 1];
        int32_t dc = 0;
        for (ptr = it_iter->second.ptr; unsigned(dc) < (ghb_size - 1); ptr = ghb[ptr % ghb_size].prev) {
            int32_t prev_ptr = ghb[ptr % ghb_size].prev;
            if (((ghb_head - 1 - prev_ptr + ghb_max_idx) % ghb_max_idx) > ghb_size) break;
            ghb_deltas[dc++] = ghb[ptr % ghb_size].addr - ghb[prev_ptr % ghb_size].addr;
        }

        int32_t d0, d1;
        d0               = ghb_deltas[0];
        d1               = ghb_deltas[1];
        int32_t walk_len = 2;
        for (dc -= 2; dc > 0 && ghb_deltas[dc] != d0 && ghb_deltas[dc + 1] != d1; --dc)
            ++walk_len;
        if (dc <= 0) return;

        int64_t new_req_id = ++ghb_req_id;

        for (uint32_t depth = 1; (depth <= ghb_depth) && dc--; ++depth) {
            baddr += ghb_deltas[dc];
            // call(walk_len + depth,this,&self_t::ghb_prefetch,new_req_id,baddr);
            ghb_prefetch(new_req_id, baddr);
        }
    }

    MemoryAddress makeBlockAddress(MemoryAddress addr)
    {
        MemoryAddress res = (addr & ~(theBlockOffsetMask));
        // DBG_(Dev, ( << "makeBlockAddress: addr=" << std::hex << addr << "res=" <<
        // res ) );
        return res;
    }

}; // end class GHBPrefetcher

} // end namespace nGHBPrefetcher

#define DBG_Reset
#include DBG_Control()
