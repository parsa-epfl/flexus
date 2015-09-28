#include <memory>
#include <fstream>
#include <sstream>

#include "seq_map.hpp"

using namespace boost::multi_index;

#define DBG_SetDefaultOps AddCat(SpatialPrefetch)
#include DBG_Control()

namespace nGHBPrefetcher {

typedef uint32_t MemoryAddress;

struct ghb_it_ent_t {
  MemoryAddress tag;
  int32_t ptr;
  ghb_it_ent_t(): tag(0), ptr(0) { }
};
struct ghb_ent_t {
  MemoryAddress addr;
  int32_t prev;
  ghb_ent_t(): addr(0), prev(-1) { }
};

class GHBPrefetcher {
  //LRUarray<ghb_it_ent_t,ghb_it_log_nsets,1> ghb_it;
  flexus_boost_set_assoc<MemoryAddress, ghb_it_ent_t> theIndexTable;
  typedef flexus_boost_set_assoc<MemoryAddress, ghb_it_ent_t>::iterator IndexTableIter;
  ghb_ent_t * ghb;
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
  {}

  void init(int32_t blockSize, int32_t ghbSize) {
    theBlockSize = blockSize;
    theBlockOffsetMask = theBlockSize - 1;
    ghb_size = ghbSize;
    ghb_max_idx = ghb_size << 4;
    theIndexTable.init(ghb_size, 1, 0);
    ghb = new ghb_ent_t[ghb_size];
  }

  void ghb_prefetch(int64_t req_id, const MemoryAddress addr) {
    if (req_id != ghb_req_id) return;
    thePrefetches.push_back(addr);
  }

  bool prefetchReady() {
    return( !thePrefetches.empty() );
  }

  MemoryAddress getPrefetch() {
    MemoryAddress addr = thePrefetches.front();
    thePrefetches.pop_front();
    return addr;
  }

  void access(const MemoryAddress PC, const MemoryAddress addr) {
    int32_t ptr = -1;
    IndexTableIter it_iter = theIndexTable.find(PC);
    if (it_iter != theIndexTable.end()) {
      ptr = it_iter->second.ptr;
    } else {
      if (theIndexTable.size() >= 1) {
        theIndexTable.pop_front();
      }
      it_iter = theIndexTable.insert( std::make_pair(PC, ghb_it_ent_t()) ).first;
      it_iter->second.tag = PC;
    }
    it_iter->second.ptr = ghb_head;
    ghb_ent_t * head_ent = &ghb[ghb_head%ghb_size];
    MemoryAddress baddr = makeBlockAddress(addr);
    head_ent->addr = baddr;
    head_ent->prev = ptr;
    ghb_head = (ghb_head + 1) % ghb_max_idx;

    int32_t ghb_deltas[ghb_size-1];
    int32_t dc = 0;
    for (ptr = it_iter->second.ptr; unsigned(dc) < (ghb_size - 1); ptr = ghb[ptr%ghb_size].prev) {
      int32_t prev_ptr = ghb[ptr%ghb_size].prev;
      if (((ghb_head - 1 - prev_ptr + ghb_max_idx) % ghb_max_idx) > ghb_size) break;
      ghb_deltas[dc++] = ghb[ptr%ghb_size].addr - ghb[prev_ptr%ghb_size].addr;
    }

    int32_t d0, d1;
    d0 = ghb_deltas[0];
    d1 = ghb_deltas[1];
    int32_t walk_len = 2;
    for (dc -= 2; dc > 0 && ghb_deltas[dc] != d0 && ghb_deltas[dc+1] != d1; --dc) ++walk_len;
    if (dc <= 0) return;

    int64_t new_req_id = ++ghb_req_id;

    for (uint32_t depth = 1; (depth <= ghb_depth) && dc--; ++depth) {
      baddr += ghb_deltas[dc];
      //call(walk_len + depth,this,&self_t::ghb_prefetch,new_req_id,baddr);
      ghb_prefetch(new_req_id, baddr);
    }
  }

  MemoryAddress makeBlockAddress(MemoryAddress addr) {
    MemoryAddress res = (addr & ~(theBlockOffsetMask));
    //DBG_(Dev, ( << "makeBlockAddress: addr=" << std::hex << addr << "res=" << res ) );
    return res;
  }

}; // end class GHBPrefetcher

} // end namespace nGHBPrefetcher

#define DBG_Reset
#include DBG_Control()
