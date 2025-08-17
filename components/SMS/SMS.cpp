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
            thePHT = PHT(cfg.NumPHTSets, cfg.PHTAssociativity, cfg.NumBlks, cfg.SMSRot, cfg.SMSSepRdWr, cfg.SMSUseSatCnts, cfg.PerfectPHT);
            theAGT = AGT(cfg.NumAccTableEntries, cfg.NumFilterTableEntries, cfg.NumBlks);
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push(interface::RequestIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            // Record the access
            uint64_t addr = aMessage[MemoryMessageTag]->address();  // TODO: use flexus datatypes
            uint64_t pc = aMessage[MemoryMessageTag]->pc();
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
            auto entry = theAGT.record(addr, pc, is_store, ts);
            if (entry) {
                thePHT.insert(*entry);
            }

            // Prefetch blocks if applicable
            auto blocksToPrefetch = thePHT.lookup(pc, addr, !is_store, ts);
            if (blocksToPrefetch)
            {
                for (auto const& block : *blocksToPrefetch) {
                    if (block != addr) {
                        prefetchQueue.push(std::tie(aMessage, block));
                        DBG_(VVerb, (<< "Adding block to prefetch queue: " << std::hex << block));
                    }
                }
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
        void push(interface::SnoopIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if (aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::Invalidate) {
                uint64_t addr = aMessage[MemoryMessageTag]->address();
                DBG_(VVerb, (<< "Received snoop invalidate for address: " << std::hex << addr));
                auto entry = theAGT.evict(addr);
                if (entry) {
                    thePHT.insert(*entry);
                }
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(L1DRequestIn);
        void push(interface::L1DRequestIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if ((aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictClean) ||
                (aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictDirty) ||
                (aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictWritable)) {
                uint64_t addr = aMessage[MemoryMessageTag]->address();
                DBG_(VVerb, (<< "Received L1D request to evict address: " << std::hex << addr));
                auto entry = theAGT.evict(addr);
                if (entry) {
                    thePHT.insert(*entry);
                }
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(L1DSnoopIn);
        void push(interface::L1DSnoopIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if (aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictClean ||
                aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictDirty ||
                aMessage[MemoryMessageTag]->type() == MemoryMessage::MemoryMessageType::EvictWritable) {
                uint64_t addr = aMessage[MemoryMessageTag]->address();
                DBG_(VVerb, (<< "Received L1D snoop to evict address: " << std::hex << addr));
                auto entry = theAGT.evict(addr);
                if (entry) {
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
            thePHT = PHT(cfg.NumPHTSets, cfg.PHTAssociativity, cfg.NumBlks, cfg.SMSRot, cfg.SMSSepRdWr, cfg.SMSUseSatCnts, cfg.PerfectPHT);
            theAGT = AGT(cfg.NumAccTableEntries, cfg.NumFilterTableEntries, cfg.NumBlks);
        }

        void finalize() override
        {
        }
};

}

FLEXUS_COMPONENT_INSTANTIATOR(SMS, nuSMS);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SMS

#define DBG_Reset
#include DBG_Control()
