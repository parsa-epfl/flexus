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
            thePHT = PHT();
            theAGT = AGT();
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push(interface::RequestIn const&, MemoryTransport& aMessage)
        {
            // Record the access
            uint64_t addr = aMessage[MemoryMessageTag]->address();  // TODO: use flexus datatypes
            uint64_t pc = aMessage[MemoryMessageTag]->pc();
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

        void drive(interface::SMSDrive const&) override
        {
            ts++;
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

        void initialize() override
        {

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
