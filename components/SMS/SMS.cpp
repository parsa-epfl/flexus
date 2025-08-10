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
        }

        void drive(interface::SMSDrive const&) override
        {
            ts++;

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
