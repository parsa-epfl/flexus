#include "TraceTracker.hpp"

#include <core/boost_extensions/padded_string_cast.hpp>

#define DBG_DefineCategories TraceTrack
#define DBG_SetDefaultOps    AddCat(TraceTrack)
#include DBG_Control()

using namespace Flexus;
using namespace Core;


#define LOG2(x)                                                                                                                                     \
    ((x) == 1                                                                                                                                       \
       ? 0                                                                                                                                          \
       : ((x) == 2                                                                                                                                  \
            ? 1                                                                                                                                     \
            : ((x) == 4                                                                                                                             \
                 ? 2                                                                                                                                \
                 : ((x) == 8                                                                                                                        \
                      ? 3                                                                                                                           \
                      : ((x) == 16                                                                                                                  \
                           ? 4                                                                                                                      \
                           : ((x) == 32                                                                                                             \
                                ? 5                                                                                                                 \
                                : ((x) == 64                                                                                                        \
                                     ? 6                                                                                                            \
                                     : ((x) == 128                                                                                                  \
                                          ? 7                                                                                                       \
                                          : ((x) == 256                                                                                             \
                                               ? 8                                                                                                  \
                                               : ((x) == 512                                                                                        \
                                                    ? 9                                                                                             \
                                                    : ((x) == 1024                                                                                  \
                                                         ? 10                                                                                       \
                                                         : ((x) == 2048                                                                             \
                                                              ? 11                                                                                  \
                                                              : ((x) == 4096                                                                        \
                                                                   ? 12                                                                             \
                                                                   : ((x) == 8192                                                                   \
                                                                        ? 13                                                                        \
                                                                        : ((x) == 16384                                                             \
                                                                             ? 14                                                                   \
                                                                             : ((x) == 32768                                                        \
                                                                                  ? 15                                                              \
                                                                                  : ((x) == 65536                                                   \
                                                                                       ? 16                                                         \
                                                                                       : ((x) == 131072                                             \
                                                                                            ? 17                                                    \
                                                                                            : ((x) == 262144                                        \
                                                                                                 ? 18                                               \
                                                                                                 : ((x) == 524288                                   \
                                                                                                      ? 19                                          \
                                                                                                      : ((x) ==                                     \
                                                                                                             1048576                                \
                                                                                                           ? 20                                     \
                                                                                                           : ((x) ==                                \
                                                                                                                  2097152                           \
                                                                                                                ? 21                                \
                                                                                                                : ((x) ==                           \
                                                                                                                       4194304                      \
                                                                                                                     ? 22                           \
                                                                                                                     : ((x) ==                      \
                                                                                                                            8388608                 \
                                                                                                                          ? 23                      \
                                                                                                                          : ((x) ==                 \
                                                                                                                                 16777216           \
                                                                                                                               ? 24                 \
                                                                                                                               : ((x) ==            \
                                                                                                                                      33554432      \
                                                                                                                                    ? 25            \
                                                                                                                                    : ((x) ==       \
                                                                                                                                           67108864 \
                                                                                                                                         ? 26       \
                                                                                                                                         : -0xffff)))))))))))))))))))))))))))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace nTraceTracker {

void
TraceTracker::access(int32_t aNode,
                     SharedTypes::tFillLevel cache,
                     address_t addr,
                     address_t pc,
                     bool prefetched,
                     bool write,
                     bool miss,
                     bool priv,
                     uint64_t ltime)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] access 0x" << std::hex << addr));
    // DBG_(Dev, (<< "[" << aNode << ":" << cache << "] access 0x" << std::hex <<
    // addr << " (ts:" << ltime <<")"));
}

void
TraceTracker::commit(int32_t aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc, uint64_t aLogicalTime)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] commit 0x" << std::hex << addr));
}

void
TraceTracker::store(int32_t aNode,
                    SharedTypes::tFillLevel cache,
                    address_t addr,
                    address_t pc,
                    bool miss,
                    bool priv,
                    uint64_t aLogicalTime)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] store 0x" << std::hex << addr));
}

void
TraceTracker::prefetch(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] prefetch 0x" << std::hex << block));
}

void
TraceTracker::fill(int32_t aNode,
                   SharedTypes::tFillLevel cache,
                   address_t block,
                   SharedTypes::tFillLevel fillLevel,
                   bool isFetch,
                   bool isWrite)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] fill 0x" << std::hex << block));
    DBG_Assert(fillLevel != SharedTypes::eUnknown);
}

void
TraceTracker::prefetchFill(int32_t aNode,
                           SharedTypes::tFillLevel cache,
                           address_t block,
                           SharedTypes::tFillLevel fillLevel)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] prefetch fill 0x" << std::hex << block));
    DBG_Assert(fillLevel != SharedTypes::eUnknown);
}

void
TraceTracker::prefetchHit(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] prefetch hit 0x" << std::hex << block));
}

void
TraceTracker::prefetchRedundant(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] prefetch redundant 0x" << std::hex << block));
}

void
TraceTracker::insert(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] insert 0x" << std::hex << block));
}

void
TraceTracker::eviction(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, bool drop)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] evict 0x" << std::hex << block));
}

void
TraceTracker::invalidation(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] invalidate 0x" << std::hex << block));
}

void
TraceTracker::invalidAck(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] invAck 0x" << std::hex << block));
}

void
TraceTracker::invalidTagCreate(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] invTagCreate 0x" << std::hex << block));
}

void
TraceTracker::invalidTagRefill(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] invTagRefill 0x" << std::hex << block));
}

void
TraceTracker::invalidTagReplace(int32_t aNode, SharedTypes::tFillLevel cache, address_t block)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] invTagReplace 0x" << std::hex << block));
}

void
TraceTracker::accessLoad(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));
}

void
TraceTracker::accessStore(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] accessStore 0x" << std::hex << block << "," << offset));
}

void
TraceTracker::accessFetch(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));
}

void
TraceTracker::accessAtomic(int32_t aNode, SharedTypes::tFillLevel cache, address_t block, uint32_t offset, int32_t size)
{
    DBG_(VVerb, (<< "[" << aNode << ":" << cache << "] accessAtomic 0x" << std::hex << block << "," << offset));
}

TraceTracker::TraceTracker() {}

void
TraceTracker::initialize()
{
    DBG_(VVerb, (<< "initializing TraceTracker"));
    Flexus::Stat::getStatManager()->addFinalizer([this]() { return this->finalize(); });
}

void
TraceTracker::finalize()
{
    DBG_(VVerb, (<< "finalizing TraceTracker"));
}

} // namespace nTraceTracker

#ifndef _TRACETRACKER_OBJECT_DEFINED_
#define _TRACETRACKER_OBJECT_DEFINED_
nTraceTracker::TraceTracker theTraceTracker;
#endif
