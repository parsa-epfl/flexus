
#ifndef __CMPCACHE_ABSTRACT_CACHE_CONTROLLER_HPP__
#define __CMPCACHE_ABSTRACT_CACHE_CONTROLLER_HPP__

#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/ProcessEntry.hpp>
#include <components/CommonQEMU/AbstractFactory.hpp>
#include <components/CommonQEMU/MessageQueues.hpp>
#include <core/debug/debug.hpp>
#include <list>
#include <map>

namespace nCMPCache {

using namespace Flexus::SharedTypes;
using namespace nMessageQueues;

class AbstractCacheController
{

  protected:
    std::string theName;
    CMPCacheInfo theCMPCacheInfo;
    AbstractPolicy* thePolicy;

  public:
    AbstractCacheController(const CMPCacheInfo& anInfo)
      : theName(anInfo.theName)
      , theCMPCacheInfo(anInfo)
      , RequestIn(anInfo.theQueueSize)
      , SnoopIn(anInfo.theQueueSize)
      , ReplyIn(anInfo.theQueueSize)
      , RequestOut(anInfo.theQueueSize)
      , SnoopOut(anInfo.theQueueSize)
      , ReplyOut(anInfo.theQueueSize)
    {
        thePolicy =
          AbstractFactory<AbstractPolicy, CMPCacheInfo>::createInstance(anInfo.thePolicyType, theCMPCacheInfo);
        DBG_(Dev, (<< theName << ": created AbstractCacheController '" << theName << "'"));
    }

    virtual ~AbstractCacheController() { delete thePolicy; }

    // Message queues
    MessageQueue<MemoryTransport> RequestIn;
    MessageQueue<MemoryTransport> SnoopIn;
    MessageQueue<MemoryTransport> ReplyIn;

    MessageQueue<MemoryTransport> RequestOut;
    MessageQueue<MemoryTransport> SnoopOut;
    MessageQueue<MemoryTransport> ReplyOut;

    std::map<MemoryAddress, int> theEBReservations;
    std::map<ProcessEntry*, int> theSnoopReservations;

    virtual bool isQuiesced() const = 0;

    virtual void processMessages() = 0;

    virtual void loadState(std::string const& aDirName) = 0;

    inline void reserveSnoopOut(ProcessEntry_p process, uint8_t n)
    {

        DBG_(Trace, (<< "reserve | Snoop(Out){" << n << "} | " << theName));

        DBG_(Trace, (<< theName << "reserveSnoopOut(" << (int)n << "): " << *process));
        DBG_Assert(n > 0);
        SnoopOut.reserve(n);
        process->snoop_out_reserved += n;
        theSnoopReservations[process.get()] += n;
        DBG_Assert(SnoopOut.getReserve() >= 0);
    }

    inline void reserveReplyOut(ProcessEntry_p process, uint8_t n)
    {
        ReplyOut.reserve(n);
        process->reply_out_reserved += n;
    }

    inline void reserveRequestOut(ProcessEntry_p process, uint8_t n)
    {
        RequestOut.reserve(n);
        process->req_out_reserved += n;
    }

    inline void reserveCacheEB(ProcessEntry_p process, int32_t n)
    {
        thePolicy->reserveCacheEB(n);
        process->cache_eb_reserved += n;
        theEBReservations[process->transport()[MemoryMessageTag]->address()] += n;
        if (strcmp(theName.c_str(), "13-L2") == 0) {
            DBG_(
              Verb,
              (<< theName << " reserveEB for addr: " << process->transport()[MemoryMessageTag]->address()
               << ", total reservations = " << theEBReservations[process->transport()[MemoryMessageTag]->address()]));
        }
    }

    inline void reserveEB(ProcessEntry_p process, int32_t n)
    {
        thePolicy->reserveEB(n);
        process->dir_eb_reserved += n;
        process->cache_eb_reserved += n;
        theEBReservations[process->transport()[MemoryMessageTag]->address()] += n;
        if (strcmp(theName.c_str(), "13-L2") == 0) {
            DBG_(
              Verb,
              (<< theName << " reserveEB for addr: " << process->transport()[MemoryMessageTag]->address()
               << ", total reservations = " << theEBReservations[process->transport()[MemoryMessageTag]->address()]));
        }
    }

    inline void reserveMAF(ProcessEntry_p process)
    {
        thePolicy->MAF().reserve();
        process->maf_reserved = true;
    }

    inline void unreserveSnoopOut(ProcessEntry_p process, uint8_t n)
    {
        DBG_(Trace, (<< theName << "unreserveSnoopOut(" << (int)n << "): " << *process));
        if (n == 0) return;
        SnoopOut.unreserve(n);
        DBG_Assert(process->snoop_out_reserved >= n,
                   (<< theName << " process tried to unreserve " << n << " snoop entries, but only has "
                    << process->snoop_out_reserved << " entries reserved: " << *process));
        process->snoop_out_reserved -= n;
        theSnoopReservations[process.get()] -= n;
        if (theSnoopReservations[process.get()] == 0) { theSnoopReservations.erase(process.get()); }
        DBG_Assert(SnoopOut.getReserve() >= 0);
    }

    inline void unreserveReplyOut(ProcessEntry_p process, uint8_t n)
    {
        ReplyOut.unreserve(n);
        process->reply_out_reserved -= n;
    }

    inline void unreserveRequestOut(ProcessEntry_p process, uint8_t n)
    {
        RequestOut.unreserve(n);
        process->req_out_reserved -= n;
    }

    inline void unreserveCacheEB(ProcessEntry_p process)
    {
        thePolicy->unreserveCacheEB(process->cache_eb_reserved);
        if (theEBReservations[process->transport()[MemoryMessageTag]->address()] == 0) {
            theEBReservations[MemoryAddress(0)] -= process->cache_eb_reserved;
        } else {
            theEBReservations[process->transport()[MemoryMessageTag]->address()] -= process->cache_eb_reserved;
        }
        if (strcmp(theName.c_str(), "13-L2") == 0)
            DBG_(Verb,
                 (<< theName << " unreserveCacheEB for addr: " << process->transport()[MemoryMessageTag]->address()
                  << ", remaining reservations = "
                  << theEBReservations[process->transport()[MemoryMessageTag]->address()]));
        if (theEBReservations[process->transport()[MemoryMessageTag]->address()] == 0) {
            theEBReservations.erase(process->transport()[MemoryMessageTag]->address());
        }
        process->cache_eb_reserved = 0;
    }

    inline void unreserveEB(ProcessEntry_p process)
    {
        thePolicy->unreserveDirEB(process->dir_eb_reserved);
        thePolicy->unreserveCacheEB(process->cache_eb_reserved);
        if (theEBReservations[process->transport()[MemoryMessageTag]->address()] == 0) {
            theEBReservations[MemoryAddress(0)] -= process->cache_eb_reserved;
        } else {
            theEBReservations[process->transport()[MemoryMessageTag]->address()] -= process->cache_eb_reserved;
        }
        if (strcmp(theName.c_str(), "13-L2") == 0)
            DBG_(Verb,
                 (<< theName << " unreserveEB for addr: " << process->transport()[MemoryMessageTag]->address()
                  << ", remaining reservations = "
                  << theEBReservations[process->transport()[MemoryMessageTag]->address()]));
        if (theEBReservations[process->transport()[MemoryMessageTag]->address()] == 0) {
            theEBReservations.erase(process->transport()[MemoryMessageTag]->address());
        }
        process->dir_eb_reserved   = 0;
        process->cache_eb_reserved = 0;
    }

    inline void unreserveMAF(ProcessEntry_p process)
    {
        thePolicy->MAF().unreserve();
        process->maf_reserved = false;
    }

    inline void dumpEBReservations()
    {
        std::map<MemoryAddress, int>::iterator iter, end;
        iter = theEBReservations.begin();
        end  = theEBReservations.end();
        for (; iter != end; iter++) {
            if (strcmp(theName.c_str(), "13-L2") == 0)
                DBG_(Verb,
                     (<< theName << " reserved " << iter->second << " EB entries for address " << std::hex
                      << iter->first));
        }
    }

    inline void dumpSnoopReservations()
    {
        DBG_(Iface, (<< theName << " dumping snoop reservations."));
        std::map<ProcessEntry*, int>::iterator iter, end;
        iter = theSnoopReservations.begin();
        end  = theSnoopReservations.end();
        for (; iter != end; iter++) {
            DBG_(Iface, (<< theName << " reserved " << iter->second << " snoop entries for " << *(iter->first)));
        }
    }

    inline void dumpMAFEntries(MemoryAddress addr)
    {
        MissAddressFile::iterator iter = thePolicy->MAF().find(addr);
        for (; iter != thePolicy->MAF().end(); iter++) {
            DBG_(Trace,
                 (<< theName << " MAF contains: " << *(iter->transport()[MemoryMessageTag]) << " in state "
                  << (int)(iter->state())));
        }
    }

}; // AbstractCacheController

#define REGISTER_CMP_CACHE_CONTROLLER(type, n)                                                                         \
    const std::string type::name = n;                                                                                  \
    static ConcreteFactory<AbstractCacheController, type, CMPCacheInfo> type##_Factory

}; // namespace nCMPCache

#endif // !__CMPCACHE_ABSTRACT_CACHE_CONTROLLER_HPP__
