#include "SMS.hpp"
using Flexus::SharedTypes::MemoryMessage;

#define FLEXUS_BEGIN_COMPONENT SMS
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories SMS
#define DBG_SetDefaultOps    AddCat(SMS)
#include DBG_Control()

using namespace Flexus::Qemu;

namespace nuSMS {

class FLEXUS_COMPONENT(SMS)
{
    FLEXUS_COMPONENT_IMPL(SMS);
    uint64_t ts;    // Internal counter for LRU
    std::queue<std::tuple<MemoryTransport, uint64_t>> prefetchQueue; // Queue for prefetched blocks (trigger, prefetch addr)
    PHT thePHT;
    AGT theAGT;

    public:
        FLEXUS_COMPONENT_CONSTRUCTOR(SMS)
            : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
        {
            ts = 0;
            prefetchQueue = std::queue<std::tuple<MemoryTransport, uint64_t>>();
            thePHT = PHT(flexusIndex(), cfg.NumPHTSets, cfg.PHTAssociativity, cfg.NumBlks, cfg.SMSRot, cfg.SMSSepRdWr, cfg.SMSUseSatCnts, cfg.PerfectPHT);
            theAGT = AGT(cfg.NumAccTableEntries, cfg.NumFilterTableEntries, cfg.NumBlks);
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push(interface::RequestIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if(aMessage[uArchStateTag]->theInstruction) {
                if(aMessage[uArchStateTag]->theInstruction->isMicroOp()) {
                    DBG_(VVerb, (<< "Microop detected, skipping SMS processing." << std::hex << aMessage[MemoryMessageTag]->address()));
                    return;
                }
            }
            // Record the access
            uint64_t addr = (aMessage[MemoryMessageTag]->address()) >> (__builtin_ctzll(cfg.BlockSize));  // TODO: use flexus datatypes
            uint64_t pc = aMessage[MemoryMessageTag]->pc();
            if(pc == 0) {   // Why tf is PC 0 sometimes??
                DBG_(VVerb, (<< "PC is 0, skipping SMS processing for address: " << std::hex << addr));
                return;
            }
            DBG_(VVerb, (<< "Received memory request: " << std::hex << addr << ", PC: " << std::hex << pc));
            bool is_store;
            switch (aMessage[MemoryMessageTag]->type()) {
                case MemoryMessage::MemoryMessageType::LoadReq:
                case MemoryMessage::MemoryMessageType::ReadReq:
                    is_store = false;
                    break;
                case MemoryMessage::MemoryMessageType::StoreReq:
                case MemoryMessage::MemoryMessageType::RMWReq:
                case MemoryMessage::MemoryMessageType::CmpxReq:
                case MemoryMessage::MemoryMessageType::WriteReq:
                case MemoryMessage::MemoryMessageType::WriteAllocate:
                    is_store = true;
                    break;
                default:
                    return;
            }

            // Prefetch blocks if applicable
            DBG_(Crit, (<< "Prefetching blocks for request: " << std::hex << addr << ", PC: " << std::hex << pc));
            auto blocksToPrefetch = thePHT.lookup(pc, addr, !is_store, ts);
            if (blocksToPrefetch)
            {
                for (auto const& block : *blocksToPrefetch) {
                    if (block != addr) {
                        uint64_t fullAddr = block << __builtin_ctzll(cfg.BlockSize);
                        prefetchQueue.push(std::tie(aMessage, fullAddr));
                        DBG_(Crit, (<< "Adding block to prefetch queue: " << std::hex << block << " on trigger address: " << addr));
                    }
                }
            }

            DBG_(Crit, (<< "Recording access for block: " << std::hex << addr << ", PC: " << std::hex << pc));
            auto entry = theAGT.record(addr, pc, is_store, ts);
            if (entry) {
                AccTableEntry evicted = *entry;
                DBG_(Crit, (<< "Recording led to eviction, " <<  std::hex << evicted.tag << ", " << evicted.pc << ", " << evicted.offset));
                // DBG_(Crit, (<< "Pattern is : "));
                // for (auto bit : evicted.access_pattern) {
                //     DBG_(Crit, (<< bit));
                // }
                // DBG_(Crit, (<< ""));
                thePHT.insert(*entry);
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
        void push(interface::SnoopIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            // if (aMessage[MemoryMessageTag]->isInvalidateType() || aMessage[MemoryMessageTag]->isEvictType()) {
            //     uint64_t addr = (aMessage[MemoryMessageTag]->address())  >> (__builtin_ctzll(cfg.BlockSize));
            //     DBG_(VVerb, (<< "Received snoop invalidate for address: " << std::hex << addr));
            //     DBG_(VVerb, (<< "Evicting SMS for block: " << std::hex << addr));
            //     auto entry = theAGT.evict(addr);
            //     if (entry) {
            //         DBG_(VVerb, (<< "Eviction led to recording, " << std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
            //         // for( auto bit : entry->access_pattern) {
            //         //     DBG_(VVerb, (<< bit));
            //         // }
            //         // DBG_(VVerb, (<< ""));
            //         thePHT.insert(*entry);
            //     }
            // }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(L1DFrontOut);
        void push(interface::L1DFrontOut const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if (aMessage[MemoryMessageTag]->isInvalidateType() || aMessage[MemoryMessageTag]->isEvictType()) {
                uint64_t addr = (aMessage[MemoryMessageTag]->address())  >> (__builtin_ctzll(cfg.BlockSize));
                DBG_(Crit, (<< "Received L1D front out request to evict address: " << std::hex << addr));
                DBG_(Crit, (<< "Evicting SMS for block: " << std::hex << addr));
                auto entry = theAGT.evict(addr);
                if (entry) {
                    DBG_(Crit, (<< "Eviction led to recording, " << std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
                    // for( auto bit : entry->access_pattern) {
                    //     DBG_(Crit, (<< bit));
                    // }
                    // DBG_(Crit, (<< ""));
                    thePHT.insert(*entry);
                }
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(L1DRequestIn);
        void push(interface::L1DRequestIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            // if (aMessage[MemoryMessageTag]->isInvalidateType() || aMessage[MemoryMessageTag]->isEvictType()) {
            //     uint64_t addr = (aMessage[MemoryMessageTag]->address())  >> (__builtin_ctzll(cfg.BlockSize));
            //     DBG_(VVerb, (<< "Received L1D request to evict address: " << std::hex << addr));
            //     DBG_(VVerb, (<< "Evicting SMS for block: " << std::hex << addr));
            //     auto entry = theAGT.evict(addr);
            //     if (entry) {
            //         DBG_(VVerb, (<< "Eviction led to recording, " << std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
            //         // for( auto bit : entry->access_pattern) {
            //         //     DBG_(VVerb, (<< bit));
            //         // }
            //         // DBG_(VVerb, (<< ""));
            //         thePHT.insert(*entry);
            //     }
            // }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(L1DSnoopIn);
        void push(interface::L1DSnoopIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if (aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictClean ||
                aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictDirty ||
                aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictWritable) {
                uint64_t addr = (aMessage[MemoryMessageTag]->address())  >> (__builtin_ctzll(cfg.BlockSize));
                DBG_(Crit, (<< "Received L1D snoop to evict address: " << std::hex << addr));
                DBG_(Crit, (<< "Evicting SMS for block: " << std::hex << addr));
                auto entry = theAGT.evict(addr);
                if (entry) {
                    DBG_(Crit, (<< "Eviction led to recording, " << std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
                    thePHT.insert(*entry);
                }
            }
        }

        void drive(interface::SMSDrive const&) override
        {
            ts++;
            if (cfg.EnableSMS) {
                while (FLEXUS_CHANNEL(Prefetch_Request).available() && !prefetchQueue.empty()) {
                    MemoryTransport aMessage;
                    uint64_t block;
                    std::tie(aMessage, block) = prefetchQueue.front();
                    PhysicalMemoryAddress blockAddr(block);
                    VirtualMemoryAddress pc(aMessage[MemoryMessageTag]->pc());
                    prefetchQueue.pop();
                    DBG_(VVerb, (<< "Prefetching block: " << std::hex << block));

                    intrusive_ptr<MemoryMessage> operation = new MemoryMessage(MemoryMessage::MemoryMessageType::PrefetchReadAllocReq, blockAddr, pc);
                    operation->theInstruction = aMessage[MemoryMessageTag]->theInstruction;
                    aMessage.set(MemoryMessageTag, operation);
                    FLEXUS_CHANNEL(Prefetch_Request) << aMessage;
                }
            }
        }

        void initialize() override
        {
            DBG_(VVerb, (<< "Initializing SMS component..."));
            DBG_(VVerb, (<< "SMS Configuration: " << cfg.EnableSMS << ", "
                         << "NumAccTableEntries: " << cfg.NumAccTableEntries << ", "
                         << "NumFilterTableEntries: " << cfg.NumFilterTableEntries << ", "
                         << "NumPHTSets: " << cfg.NumPHTSets << ", "
                         << "PHTAssociativity: " << cfg.PHTAssociativity << ", "
                         << "NumBlks: " << cfg.NumBlks << ", "
                         << "SMSRot: " << cfg.SMSRot << ", "
                         << "SMSSepRdWr: " << cfg.SMSSepRdWr << ", "
                         << "SMSUseSatCnts: " << cfg.SMSUseSatCnts << ", "
                         << "PerfectPHT: " << cfg.PerfectPHT));

            ts = 0;
            prefetchQueue = std::queue<std::tuple<MemoryTransport, uint64_t>>();
            thePHT = PHT(flexusIndex(), cfg.NumPHTSets, cfg.PHTAssociativity, cfg.NumBlks, cfg.SMSRot, cfg.SMSSepRdWr, cfg.SMSUseSatCnts, cfg.PerfectPHT);
            theAGT = AGT(cfg.NumAccTableEntries, cfg.NumFilterTableEntries, cfg.NumBlks);
        }

        void finalize() override
        {
        }

        void loadState(std::string const& aDirName) { uint64_t max_ts = thePHT.loadState(aDirName);
            if (max_ts > ts) {
                ts = max_ts;
            }
            DBG_(VVerb, (<< "SMS state loaded. Max timestamp: " << max_ts << ", Current timestamp: " << ts));
        }

        void saveState(std::string const& aDirName) { thePHT.saveState(aDirName); }
};

}

FLEXUS_COMPONENT_INSTANTIATOR(SMS, nuSMS);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SMS

#define DBG_Reset
#include DBG_Control()
