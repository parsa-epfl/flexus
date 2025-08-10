#include "SMS.hpp"

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
    PHT thePHT;
    AGT theAGT;

    public:
        FLEXUS_COMPONENT_CONSTRUCTOR(SMS)
            : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
        {
        }

        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push(interface::RequestIn const&, MemoryTransport& aTransport)
        {

        }

        void drive(interface::SMSDrive const&) override
        {

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
