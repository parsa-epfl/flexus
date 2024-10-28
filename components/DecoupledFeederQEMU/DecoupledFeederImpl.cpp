
#include "Tracer.hpp"
#include "components/DecoupledFeederQEMU/DecoupledFeeder.hpp"
#include "components/uFetch/uFetchTypes.hpp"
#include "core/flexus.hpp"
#include "core/qemu/api_wrappers.hpp"
#include "core/stats.hpp"

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDecoupledFeeder {

using namespace Flexus;
using namespace Flexus::Qemu;

class FLEXUS_COMPONENT(DecoupledFeeder)
{
    FLEXUS_COMPONENT_IMPL(DecoupledFeeder);

    // The Qemu objects (one for each processor) for getting trace data
    std::size_t theNumCPUs;
    std::size_t theCMPWidth;
    Tracer* vCPUTracer;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
        theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
        vCPUTracer =
          Tracer::getInstance(theNumCPUs, [this](std::size_t x, MemoryMessage& y) { this->feeder_dispatch(x, y); });
        Flexus::SharedTypes::MemoryMessage msg(MemoryMessage::LoadReq);
    }

    void initialize(void)
    {
        DBG_(VVerb, (<< "Inititializing Decoupled feeder..."));
        theCMPWidth = cfg.CMPWidth ? theCMPWidth : Qemu::API::qemu_api.get_num_cores();
    }

    /**
     * Function to call periodically.
     * Mainly to update the statistics
     **/
    void housekeeping() { vCPUTracer->update_collector(); }

    void finalize(void) { housekeeping(); }

    /**
     * Callback function passed to the Tracer.
     *
     * This function dispatch the message from the Tracer to the
     * right subsystem.
     *
     * Eventually call housekeeping function after N instruction
     **/
    void feeder_dispatch(std::size_t anIndex, MemoryMessage& aMessage)
    {
        switch (aMessage.type()) {
            case MemoryMessage::FetchReq:
                modernToL1I(anIndex, aMessage);
                vCPUTracer->core(anIndex)->touch_itlb_request(aMessage.priv());
                break;

            case MemoryMessage::RMWReq:
            case MemoryMessage::LoadReq:
            case MemoryMessage::StoreReq:
                toL1D(anIndex, aMessage);
                vCPUTracer->core(anIndex)->touch_dtlb_request(aMessage.priv());
                break;

            case MemoryMessage::IOLoadReq:
            case MemoryMessage::IOStoreReq:
                toPCIeRootComplex(anIndex, aMessage);
                vCPUTracer->core(anIndex)->touch_dtlb_request(aMessage.priv());
                break;

            default: DBG_Assert(false);
        }

        static uint32_t housekeeping_counter = cfg.HousekeepingPeriod || 1000;
        if (0 < --housekeeping_counter) {
            housekeeping();
            housekeeping_counter = cfg.HousekeepingPeriod || 1000;
        }
    }

    void toL1D(std::size_t anIndex, MemoryMessage& aMessage)
    {
        TranslationPtr tr(new Translation);
        tr->setData();
        tr->theType     = (aMessage.type() == MemoryMessage::LoadReq) ? Translation::eLoad : Translation::eStore;
        tr->theVaddr    = aMessage.pc();
        tr->thePaddr    = aMessage.address();
        tr->inTraceMode = true;

        FLEXUS_CHANNEL_ARRAY(ToMMU, anIndex) << tr;

        MemoryMessage mmu_message(aMessage);
        mmu_message.setPageWalk();

        vCPUTracer->core(anIndex)->touch_dtlb_request(aMessage.priv());

        // If the page walk did not generated any trace
        // that mean it was a hit (sorta stupid but ok)
        if (tr->trace_addresses.empty())
            vCPUTracer->core(anIndex)->touch_dtlb_hit(aMessage.priv());
        else
            vCPUTracer->core(anIndex)->touch_dtlb_miss(aMessage.priv());

        while (tr->trace_addresses.size()) {
            mmu_message.type()    = MemoryMessage::LoadReq;
            mmu_message.address() = tr->trace_addresses.front();
            tr->trace_addresses.pop();

            FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << mmu_message;

            vCPUTracer->core(anIndex)->touch_dtlb_access(aMessage.priv());
        }

        FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << aMessage;
    }

    void modernToL1I(std::size_t anIndex, MemoryMessage& aMessage)
    {

        TranslationPtr tr(new Translation);
        tr->setInstr();
        tr->theType     = Translation::eFetch;
        tr->theVaddr    = aMessage.pc();
        tr->thePaddr    = aMessage.address();
        tr->inTraceMode = true;

        FLEXUS_CHANNEL_ARRAY(ToMMU, anIndex) << tr;

        MemoryMessage mmu_message(MemoryMessage::LoadReq);
        mmu_message.setPageWalk();

        // If the page walk did not generated any trace
        // that mean it was a hit (sorta stupid but ok)
        if (tr->trace_addresses.empty())
            vCPUTracer->core(anIndex)->touch_itlb_hit(aMessage.priv());
        else
            vCPUTracer->core(anIndex)->touch_itlb_miss(aMessage.priv());

        // Send all MMU trace to the TL1D and count the number of access
        while (tr->trace_addresses.size()) {
            mmu_message.type() = MemoryMessage::LoadReq;
            //! Stupid flow
            // front() return a reference to an object in a queue
            // pop() remove the same object from the queue
            // therefore mmu_message.address() hold a memory
            // address which is in a weird state
            mmu_message.address() = tr->trace_addresses.front();
            tr->trace_addresses.pop();

            FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << mmu_message;

            vCPUTracer->core(anIndex)->touch_itlb_access(aMessage.priv());
        }

        FLEXUS_CHANNEL_ARRAY(ToL1I, anIndex) << aMessage;

        pc_type_annul_triplet thePCTypeAndAnnulTriplet;
        thePCTypeAndAnnulTriplet.first = aMessage.pc();

        std::pair<uint32_t, uint32_t> theTypeAndAnnulPair;
        theTypeAndAnnulPair.first  = (uint32_t)aMessage.branchType();
        theTypeAndAnnulPair.second = (uint32_t)aMessage.branchAnnul();

        thePCTypeAndAnnulTriplet.second = theTypeAndAnnulPair;

        FLEXUS_CHANNEL_ARRAY(ToBPred, anIndex) << thePCTypeAndAnnulTriplet;
    }

    void toPCIeRootComplex(std::size_t anIndex, MemoryMessage& aMessage)
    {
        TranslationPtr tr(new Translation);
        tr->setData();
        tr->theType     = (aMessage.type() == MemoryMessage::IOLoadReq) ? Translation::eLoad : Translation::eStore;
        tr->theVaddr    = aMessage.pc();
        tr->thePaddr    = aMessage.address();
        tr->inTraceMode = true;

        FLEXUS_CHANNEL_ARRAY(ToMMU, anIndex) << tr;

        MemoryMessage mmu_message(aMessage);
        mmu_message.setPageWalk();

        vCPUTracer->core(anIndex)->touch_dtlb_request(aMessage.priv());

        // If the page walk did not generated any trace
        // that mean it was a hit (sorta stupid but ok)
        if (tr->trace_addresses.empty())
            vCPUTracer->core(anIndex)->touch_dtlb_hit(aMessage.priv());
        else
            vCPUTracer->core(anIndex)->touch_dtlb_miss(aMessage.priv());

        while (tr->trace_addresses.size()) {
            mmu_message.type()    = MemoryMessage::LoadReq;
            mmu_message.address() = tr->trace_addresses.front();
            tr->trace_addresses.pop();

            FLEXUS_CHANNEL_ARRAY(ToL1D, anIndex) << mmu_message;

            vCPUTracer->core(anIndex)->touch_dtlb_access(aMessage.priv());
        }

        DBG_(Iface,
             Addr(aMessage.address())(<< "IO Request[" << anIndex << "]: " << aMessage << std::dec));
    }

}; // end class DecoupledFeeder
} // end Namespace nDecoupledFeeder

FLEXUS_COMPONENT_INSTANTIATOR(DecoupledFeeder, nDecoupledFeeder);
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToL1D)
{
    return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToL1I)
{
    return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToBPred)
{
    return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(DecoupledFeeder, ToMMU)
{
    return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder

#define DBG_Reset
#include DBG_Control()
