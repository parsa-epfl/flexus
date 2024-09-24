#ifndef _TRACE_TRACKER_HPP_
#define _TRACE_TRACKER_HPP_

#include <boost/optional.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <map>
#include <memory>
#include <tuple>

#include DBG_Control()

namespace nTraceTracker {

typedef uint32_t address_t;

class TraceTracker
{

  public:
    void access(int32_t aNode,
                Flexus::SharedTypes::tFillLevel cache,
                address_t addr,
                address_t pc,
                bool prefetched,
                bool write,
                bool miss,
                bool priv,
                uint64_t ltime);
    void commit(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, uint64_t ltime);
    void store(int32_t aNode,
               Flexus::SharedTypes::tFillLevel cache,
               address_t addr,
               address_t pc,
               bool miss,
               bool priv,
               uint64_t ltime);
    void prefetch(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void fill(int32_t aNode,
              Flexus::SharedTypes::tFillLevel cache,
              address_t block,
              Flexus::SharedTypes::tFillLevel fillLevel,
              bool isFetch,
              bool isWrite);
    void prefetchFill(int32_t aNode,
                      Flexus::SharedTypes::tFillLevel cache,
                      address_t block,
                      Flexus::SharedTypes::tFillLevel fillLevel);
    void prefetchHit(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite);
    void prefetchRedundant(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void insert(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void eviction(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool drop);
    void invalidation(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidAck(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagCreate(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagRefill(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagReplace(int32_t aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);

    void accessLoad(int32_t aNode,
                    Flexus::SharedTypes::tFillLevel cache,
                    address_t block,
                    uint32_t offset,
                    int32_t size);
    void accessStore(int32_t aNode,
                     Flexus::SharedTypes::tFillLevel cache,
                     address_t block,
                     uint32_t offset,
                     int32_t size);
    void accessFetch(int32_t aNode,
                     Flexus::SharedTypes::tFillLevel cache,
                     address_t block,
                     uint32_t offset,
                     int32_t size);
    void accessAtomic(int32_t aNode,
                      Flexus::SharedTypes::tFillLevel cache,
                      address_t block,
                      uint32_t offset,
                      int32_t size);

    TraceTracker();
    void initialize();
    void finalize();
};

} // namespace nTraceTracker

extern nTraceTracker::TraceTracker theTraceTracker;

#endif
