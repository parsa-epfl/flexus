#include "core/debug/debug.hpp"

#include <components/FastCache/CoherenceProtocol.hpp>
#include <components/FastCache/FastCache.hpp>
#include <components/FastCache/StdCache.hpp>
#include <cstdint>
#include <fstream>
#include <string>

#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nFastCache {

using namespace Flexus;

class FLEXUS_COMPONENT(FastCache)
{
    FLEXUS_COMPONENT_IMPL(FastCache);

    CoherenceProtocol* theProtocol;
    StdCache* theCache;
    CacheStats* theStats;

    uint64_t theBlockMask;

    MemoryMessage theEvictMessage;
    MemoryMessage theInvalidateMessage;

    int32_t theIndex;

    LookupResult_p lookup;
    LookupResult_p snp_lookup;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(FastCache)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
      , theEvictMessage(MemoryMessage::EvictDirty)
      , theInvalidateMessage(MemoryMessage::Invalidate)
    {
    }

    // InstructionOutputPort
    //=====================
    bool isQuiesced() const { return true; }

    void saveState(std::string const& aDirName)
    {
        std::string fname(aDirName);
        fname += "/" + statName() + ".json";

        std::ofstream ofs(fname.c_str(), std::ios::out);

        theCache->saveState(ofs);
        ofs.close();
    }

    // void loadState(std::string const &aDirName) {
    //     std::string fname(aDirName);
    //     fname += "/" + statName() + ".json";

    //    std::ifstream ifs(fname.c_str(), std::ios::in);

    //    if (!ifs.good()) {
    //        DBG_(Dev,
    //            (<< " saved checkpoint state " << fname << " not found.  Resetting to empty cache. "));
    //    } else {
    //        if (!theCache->loadState(ifs)) {
    //            DBG_(Dev, (<< "Error loading checkpoint state from file: " << fname
    //                    << ".  Make sure your checkpoints match your current cache "
    //                        "configuration."));
    //            DBG_Assert(false);
    //        }
    //    }
    //    ifs.close();
    //}

    void evict(uint64_t aTagset, CoherenceState_t aLineState)
    {
        theEvictMessage.address() = PhysicalMemoryAddress(aTagset);
        theEvictMessage.type()    = theProtocol->evict(aTagset, aLineState);

        if (theEvictMessage.type() == MemoryMessage::EvictDirty || cfg.CleanEvictions) {
            DBG_(Iface, Addr(aTagset)(<< "Evict: " << theEvictMessage));
            FLEXUS_CHANNEL(RequestOut) << theEvictMessage;
        }
    }

    bool sendInvalidate(uint64_t addr, bool icache, bool dcache)
    {
        bool was_dirty                 = false;
        theInvalidateMessage.address() = PhysicalMemoryAddress(addr);

        if (icache) {
            theInvalidateMessage.type() = MemoryMessage::Invalidate;
            DBG_(Iface, Addr(addr)(<< "Sending ICache Invalidate: " << std::hex << addr << std::dec));
            FLEXUS_CHANNEL(SnoopOutI) << theInvalidateMessage;
            // We can cached a dirty copy in the ICache too!!
            was_dirty = (theInvalidateMessage.type() == MemoryMessage::InvUpdateAck);
        }

        if (dcache) {
            theInvalidateMessage.type() = MemoryMessage::Invalidate;
            DBG_(Iface, Addr(addr)(<< "Sending DCache Invalidate: " << std::hex << addr << std::dec));
            FLEXUS_CHANNEL(SnoopOutD) << theInvalidateMessage;
        }

        return (was_dirty || (theInvalidateMessage.type() == MemoryMessage::InvUpdateAck));
    }

    void forwardMessage(MemoryMessage& msg) { FLEXUS_CHANNEL(RequestOut) << msg; }

    void continueSnoop(MemoryMessage& msg)
    {
        MemoryMessage dup_msg(msg);

        if (msg.type() != MemoryMessage::Downgrade) { FLEXUS_CHANNEL(SnoopOutI) << dup_msg; }

        FLEXUS_CHANNEL(SnoopOutD) << msg;

        // ReturnReq can get data from I or D cache.
        // All other requests, D cache response is the correct response
        if (msg.type() == MemoryMessage::ReturnNAck && dup_msg.type() != MemoryMessage::ReturnNAck) {
            msg.type() = dup_msg.type();
        }
    }

    void finalize(void) { theStats->update(); }

    void notifyRead(MemoryMessage& aMessage)
    {
        if (cfg.NotifyReads) { FLEXUS_CHANNEL(Reads) << aMessage; }
    }

    void notifyWrite(MemoryMessage& aMessage)
    {
        if (cfg.NotifyWrites) { FLEXUS_CHANNEL(Writes) << aMessage; }
    }
    void initialize(void)
    {
        static volatile bool widthPrintout = true;

        if (widthPrintout) {
            DBG_(Crit, (<< "Running with MT width " << cfg.MTWidth));
            widthPrintout = false;
        }

        theProtocol = GenerateCoherenceProtocol(
          cfg.Protocol,
          cfg.UsingTraces,
          [this](MemoryMessage& msg) { return this->forwardMessage(msg); },
          [this](MemoryMessage& msg) { return this->continueSnoop(msg); },
          [this](uint64_t addr, bool icache, bool dcache) { return this->sendInvalidate(addr, icache, dcache); },
          cfg.DowngradeLRU,
          cfg.SnoopLRU);
        theStats = new CacheStats(statName());
        theIndex = flexusIndex();

        theEvictMessage.coreIdx() = theIndex;

        // Confirm that BlockSize is a power of 2
        DBG_Assert((cfg.BlockSize & (cfg.BlockSize - 1)) == 0);
        DBG_Assert(cfg.BlockSize >= 4);

        int32_t num_sets = cfg.Size / cfg.BlockSize / cfg.Associativity;

        // Confirm that num_sets is a power of 2
        DBG_Assert((num_sets & (num_sets - 1)) == 0);

        // Confirm that settings are consistent
        DBG_Assert(cfg.BlockSize * num_sets * cfg.Associativity == cfg.Size);

        // Calculate shifts and masks
        theBlockMask = ~(cfg.BlockSize - 1);

        theCache = new StdCache(
          statName(),
          cfg.BlockSize,
          num_sets,
          cfg.Associativity,
          [this](uint64_t aTagset, CoherenceState_t aLineState) { return this->evict(aTagset, aLineState); },
          [this](uint64_t addr, bool icache, bool dcache) { return this->sendInvalidate(addr, icache, dcache); },
          theIndex,
          cfg.CacheLevel);

        Flexus::Stat::getStatManager()->addFinalizer([this]() { return this->finalize(); });
    }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchRequestIn);
    void push(interface::FetchRequestIn const&, index_t anIndex, MemoryMessage& aMessage)
    {
        aMessage.dstream() = false;
        DBG_(Iface,
             Addr(aMessage.address())(<< "FetchRequestIn[" << anIndex << "]: " << aMessage << " tagset: " << std::hex
                                      << (aMessage.address() & theBlockMask) << std::dec));
        push(interface::RequestIn(), anIndex, aMessage);
    }

    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RequestIn);
    void push(interface::RequestIn const&, index_t anIndex, MemoryMessage& aMessage)
    {

        aMessage.fillLevel() = cfg.CacheLevel;

        // Create a set and tag from the message's address
        uint64_t tagset = aMessage.address() & theBlockMask;
        DBG_(Iface,
             Addr(aMessage.address())(<< "Request[" << anIndex << "]: " << aMessage << " tagset: " << std::hex << tagset
                                      << std::dec));

        // Map memory message type to protocol request types
        CoherenceProtocol::access_t access_type = CoherenceProtocol::message2access(aMessage.type());
        CoherenceProtocol::CoherenceActionPtr fn_ptr;
        CoherenceProtocol::StatMemberPtr stat_ptr;

        // Perform Cache Lookup
        lookup = theCache->lookup(tagset);

        DBG_(Iface,
             Addr(aMessage.address())(<< flexusIndex() << ": Request[" << anIndex << "]: " << aMessage
                                      << " Initial State: " << std::hex << lookup->getState() << std::dec));

        // Determine Actions based on state and request
        tie(fn_ptr, stat_ptr) = theProtocol->getCoherenceAction(lookup->getState(), access_type);

        // Perform Actions this includes setting reply type
        (theProtocol->*fn_ptr)(lookup, aMessage);

        // Increment stat counter
        (theStats->*stat_ptr)++;
        theStats->update();

        DBG_(Iface, Addr(aMessage.address())(<< "Done, reply: " << aMessage));
        DBG_(Iface,
             Addr(aMessage.address())(<< "Request Left Lookup tagset: " << std::hex << lookup->address() << " in state "
                                      << state2String(lookup->getState()) << std::dec));
        if (snp_lookup != nullptr) {
            DBG_(Iface,
                 Addr(aMessage.address())(<< "Request Left Snoop Lookup tagset: " << std::hex << snp_lookup->address()
                                          << " in state " << state2String(snp_lookup->getState()) << std::dec));
        }
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
    void push(interface::SnoopIn const&, MemoryMessage& aMessage)
    {
        uint64_t tagset = aMessage.address() & theBlockMask;

        DBG_(Iface,
             Addr(aMessage.address())(<< "Snoop: " << aMessage << " tagset: " << std::hex << tagset << std::dec));
        snp_lookup = theCache->lookup(tagset);
        DBG_(Iface,
             Addr(aMessage.address())(<< "Snoop Found tagset: " << std::hex << tagset << " in state "
                                      << state2String(snp_lookup->getState()) << std::dec));

        CoherenceProtocol::BaseSnoopAction* fn_ptr;
        CoherenceProtocol::StatMemberPtr stat_ptr;

        // Determine Actions based on state and snoop type
        std::tie(fn_ptr, stat_ptr) = theProtocol->getSnoopAction(snp_lookup->getState(), aMessage.type());

        // Perform Actions
        (*fn_ptr)(snp_lookup, aMessage);

        // Increment counter
        (theStats->*stat_ptr)++;
        theStats->update();

        snp_lookup = theCache->lookup(tagset);
        DBG_(Iface,
             Addr(aMessage.address())(<< "Snoop Left tagset: " << std::hex << tagset << " in state "
                                      << state2String(snp_lookup->getState()) << std::dec));
    }

    void drive(interface::UpdateStatsDrive const&) { theStats->update(); }

}; // end class FastCache

} // end Namespace nFastCache

FLEXUS_COMPONENT_INSTANTIATOR(FastCache, nFastCache);

FLEXUS_PORT_ARRAY_WIDTH(FastCache, RequestIn)
{
    return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH(FastCache, FetchRequestIn)
{
    return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH(FastCache, SnoopOutD)
{
    return (cfg.MTWidth);
}
FLEXUS_PORT_ARRAY_WIDTH(FastCache, SnoopOutI)
{
    return (cfg.MTWidth);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCache

#define DBG_Reset
#include DBG_Control()