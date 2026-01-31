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

    std::tuple<uint64_t, uint64_t, boost::optional<bool>> parseMessage(MemoryTransport& aMessage) {
        uint64_t block = (aMessage[MemoryMessageTag]->address()) >> (__builtin_ctzll(cfg.BlockSize));  // TODO: use flexus datatypes
        uint64_t pc = aMessage[MemoryMessageTag]->pc();
        boost::optional<bool> is_store;
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
                is_store = boost::none;
        }
        return std::make_tuple(block, pc, is_store);
    }

    public:
        FLEXUS_COMPONENT_CONSTRUCTOR(SMS)
            : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
        {
            ts = 0;
            prefetchQueue = std::queue<std::tuple<MemoryTransport, uint64_t>>();
            thePHT = PHT(flexusIndex(), cfg.NumPHTSets, cfg.PHTAssociativity, cfg.NumBlks, cfg.SMSRot, cfg.SMSSepRdWr, cfg.SMSUseSatCnts, cfg.PerfectPHT);
            theAGT = AGT(cfg.NumAccTableEntries, cfg.NumFilterTableEntries, cfg.NumBlks);
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(PredictIn);
        void push(interface::PredictIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            uint64_t block, pc;
            boost::optional<bool> is_store;
            std::tie(block, pc, is_store) = parseMessage(aMessage);
            DBG_Assert(is_store, (<< "SMS PredictIn received message of unsupported type: " << aMessage[MemoryMessageTag]->type()));
            DBG_(VVerb, (<< "Prefetching blocks for request: " << std::hex << block << ", PC: " << std::hex << pc));
            auto blocksToPrefetch = thePHT.lookup(pc, block, !(*is_store), ts);
            if (blocksToPrefetch)
            {
                for (auto const& prefetchBlock : *blocksToPrefetch) {
                    if (prefetchBlock != block) {
                        uint64_t fullAddr = prefetchBlock << __builtin_ctzll(cfg.BlockSize);
                        if (!prefetchQueue.empty() && fullAddr == std::get<1>(prefetchQueue.back())) {
                            continue; // Avoid duplicate prefetches
                        }
                        prefetchQueue.push(std::tie(aMessage, fullAddr));
                        DBG_(VVerb, (<< "Adding block to prefetch queue: " << std::hex << prefetchBlock << " on trigger address: " << block));
                    }
                }
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(TrainIn);
        void push(interface::TrainIn const&, boost::intrusive_ptr<SMSTrainInfo>& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            uint64_t block = (aMessage->address) >> (__builtin_ctzll(cfg.BlockSize));  // TODO: use flexus datatypes
            uint64_t pc = aMessage->pc;
            bool is_store = aMessage->isStore;
            DBG_(VVerb, (<< "Recording access for block: " << std::hex << block << ", PC: " << std::hex << pc));
            auto entry = theAGT.record(block, pc, is_store, ts);
            if (entry) {
                DBG_(VVerb, (<< "Recording led to eviction, " <<  std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
                thePHT.insert(*entry);
            }
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(EvictInvalIn);
        void push(interface::EvictInvalIn const&, MemoryTransport& aMessage)
        {
            if(!cfg.EnableSMS)
                return;
            if (aMessage[MemoryMessageTag]->isInvalidateType() || aMessage[MemoryMessageTag]->isEvictType()) {
                uint64_t block, pc;
                boost::optional<bool> is_store;
                std::tie(block, pc, is_store) = parseMessage(aMessage);
                DBG_Assert(!is_store, (<< "SMS EvictInvalIn received message of unsupported type: " << aMessage[MemoryMessageTag]->type()));
                DBG_(VVerb, (<< "Received evict/invalidate for address: " << std::hex << block));
                auto entry = theAGT.evict(block);
                if (entry) {
                    DBG_(VVerb, (<< "Eviction led to recording, " << std::hex << entry->tag << ", " << entry->pc << ", " << entry->offset));
                    thePHT.insert(*entry);
                }
            }
        }

        void drive(interface::SMSDrive const&) override
        {
            if(!cfg.EnableSMS)
                return;
            ts++;
            while (FLEXUS_CHANNEL(PredictOut).available() && !prefetchQueue.empty()) {
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
                FLEXUS_CHANNEL(PredictOut) << aMessage;
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
