
#ifndef __ABSTRACT_POLICY_HPP
#define __ABSTRACT_POLICY_HPP

#include <components/CMPCache/CMPCacheInfo.hpp>
#include <components/CMPCache/CacheBuffers.hpp>
#include <components/CMPCache/EvictBuffer.hpp>
#include <components/CMPCache/ProcessEntry.hpp>
#include <components/CommonQEMU/AbstractFactory.hpp>
#include <iostream>

namespace nCMPCache {

using namespace Flexus::SharedTypes;

class AbstractPolicy
{
  public:
    virtual ~AbstractPolicy() {}

    virtual void handleRequest(ProcessEntry_p process)    = 0;
    virtual void handleSnoop(ProcessEntry_p process)      = 0;
    virtual void handleReply(ProcessEntry_p process)      = 0;
    virtual void handleWakeMAF(ProcessEntry_p process)    = 0;
    virtual void handleCacheEvict(ProcessEntry_p process) = 0;
    virtual void handleDirEvict(ProcessEntry_p process)   = 0;
    virtual void handleIdleWork(ProcessEntry_p process)   = 0;

    virtual bool isQuiesced() const = 0;

    virtual AbstractDirEvictBuffer& DirEB()            = 0;
    virtual AbstractEvictBuffer& CacheEB()             = 0;
    virtual const AbstractEvictBuffer& CacheEB() const = 0;
    virtual MissAddressFile& MAF()                     = 0;

    virtual bool hasIdleWorkAvailable() { return false; }
    virtual MemoryTransport getIdleWorkTransport()   = 0;
    virtual MemoryTransport getCacheEvictTransport() = 0;
    virtual MemoryTransport getDirEvictTransport()   = 0;
    virtual void getIdleWorkReservations(ProcessEntry_p process) {}

    virtual bool freeCacheEBPending() { return CacheEB().freeSlotsPending(); }
    virtual bool freeDirEBPending() { return DirEB().freeSlotsPending(); }

    virtual bool EBFull() { return (DirEB().full() || !CacheEBHasSpace()); }

    virtual int32_t getEBRequirements(const MemoryTransport& transport) { return 1; }

    virtual bool arrayEvictResourcesAvailable() const = 0;
    virtual bool CacheEBHasSpace() const { return (!CacheEB().full() && arrayEvictResourcesAvailable()); }
    virtual bool EBHasSpace(const MemoryTransport& transport) { return (!DirEB().full() && CacheEBHasSpace()); }

    virtual int32_t maxSnoopsPerRequest() { return 2; }

    virtual void wakeMAFs(MemoryAddress anAddress) = 0;

    virtual void load_dir_from_ckpt(std::string const&)   = 0;
    virtual void load_cache_from_ckpt(std::string const&) = 0;

    virtual void serialize_cache(std::string const&) const = 0;

    virtual void reserveArrayEvictResource(int32_t n)   = 0;
    virtual void unreserveArrayEvictResource(int32_t n) = 0;

    virtual void reserveEB(int32_t n)
    {
        DirEB().reserve(n);
        reserveCacheEB(n);
    }

    virtual void reserveCacheEB(int32_t n)
    {
        CacheEB().reserve(n);
        reserveArrayEvictResource(n);
    }

    virtual void unreserveCacheEB(int32_t n)
    {
        CacheEB().unreserve(n);
        unreserveArrayEvictResource(n);
    }

    virtual void unreserveDirEB(int32_t n) { DirEB().unreserve(n); }

}; // AbstractPolicy

#define REGISTER_CMP_CACHE_POLICY(type, n)                                                                             \
    const std::string type::name = n;                                                                                  \
    static ConcreteFactory<AbstractPolicy, type, CMPCacheInfo> type##_Factory

}; // namespace nCMPCache

#endif // ! __ABSTRACT_POLICY_HPP