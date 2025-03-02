
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <components/CMPCache/AbstractCacheController.hpp>
#include <components/CMPCache/CMPCache.hpp>
#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>

#define DBG_DefineCategories CMPCache
#define DBG_SetDefaultOps    AddCat(CMPCache)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT CMPCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nCMPCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using std::unique_ptr;

class FLEXUS_COMPONENT(CMPCache)
{
    FLEXUS_COMPONENT_IMPL(CMPCache);

  private:
    std::unique_ptr<AbstractCacheController> theController;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(CMPCache)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const
    {
        // TODO: fix this
        return theController->isQuiesced();
    }

    void loadState(std::string const& aDirName) { theController->loadState(aDirName); }

    // Initialization
    void initialize()
    {
        DBG_(Dev, (<< "GroupInterleaving = " << cfg.GroupInterleaving));

        auto cores = cfg.Cores ?: (Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2);

        CMPCacheInfo theInfo((int)flexusIndex(),
                             statName(),
                             cfg.Policy,
                             cfg.DirectoryType,
                             cfg.DirectoryConfig,
                             cfg.ArrayConfiguration,
                             cores,
                             cfg.BlockSize,
                             cfg.Banks,
                             cfg.BankInterleaving,
                             cfg.Groups,
                             cfg.GroupInterleaving,
                             cfg.MAFSize,
                             cfg.DirEvictBufferSize,
                             cfg.CacheEvictBufferSize,
                             cfg.EvictClean,
                             cfg.CacheLevel,
                             cfg.DirLatency,
                             cfg.DirIssueLatency,
                             cfg.TagLatency,
                             cfg.TagIssueLatency,
                             cfg.DataLatency,
                             cfg.DataIssueLatency,
                             cfg.QueueSize);

        //	theController.reset(new CMPCacheController(theInfo));
        theController.reset(
          AbstractFactory<AbstractCacheController, CMPCacheInfo>::createInstance(cfg.ControllerType, theInfo));
    }

    void finalize() {}

    // Ports
    //======

    // Request_In
    //-----------------
    bool available(interface::Request_In const&)
    {
        if (theController->RequestIn.full()) {}
        return !theController->RequestIn.full();
    }
    void push(interface::Request_In const&, MemoryTransport& aMessage)
    {
        DBG_Assert(!theController->RequestIn.full());

        DBG_(VVerb,
             (<< "received | Request(In){FromNode(" << aMessage[DestinationTag]->requester << ")} | "
              << *(aMessage[MemoryMessageTag])));

        if (aMessage[TransactionTrackerTag]) {
            aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Req");
        }

        DBG_Assert(aMessage[DestinationTag],
                   (<< "received | ERROR(NoTag) | Request(In){FromNode(" << aMessage[DestinationTag]->requester
                    << ")} | " << *(aMessage[MemoryMessageTag])));
        theController->RequestIn.enqueue(aMessage);
    }

    // Snoop_In
    //-----------------
    bool available(interface::Snoop_In const&)
    {
        if (theController->SnoopIn.full()) {}
        return !theController->SnoopIn.full();
    }
    void push(interface::Snoop_In const&, MemoryTransport& aMessage)
    {
        DBG_Assert(!theController->SnoopIn.full());
        DBG_(VVerb, (<< "received | Snoop(In){} | " << *(aMessage[MemoryMessageTag])));

        if (aMessage[TransactionTrackerTag]) {
            aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Snoop");
        }
        DBG_Assert(aMessage[DestinationTag],
                   (<< "received | ERROR(NoTag) | Snoop(In){FromNode(" << aMessage[DestinationTag]->requester << ")} | "
                    << *(aMessage[MemoryMessageTag])));

        theController->SnoopIn.enqueue(aMessage);
    }

    // Reply_In
    //-----------------
    bool available(interface::Reply_In const&)
    {
        if (theController->ReplyIn.full()) {}
        return !theController->ReplyIn.full();
    }
    void push(interface::Reply_In const&, MemoryTransport& aMessage)
    {
        DBG_Assert(!theController->ReplyIn.full());
        DBG_(VVerb, (<< "received | Reply(In){} | " << *(aMessage[MemoryMessageTag])));
        if (aMessage[TransactionTrackerTag]) {
            aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Reply");
        }
        DBG_Assert(aMessage[DestinationTag],
                   (<< "received | ERROR(NoTag) | Reply(In){FromNode(" << aMessage[DestinationTag]->requester << ")} | "
                    << *(aMessage[MemoryMessageTag])));

        theController->ReplyIn.enqueue(aMessage);
    }

    // DirectoryDrive
    //----------
    void drive(interface::CMPCacheDrive const&)
    {
        DBG_(VVerb, Comp(*this)(<< "drive()"));
        theController->processMessages();
        busCycle();
    }

    void busCycle()
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, Comp(*this)(<< "busCycle()"));

        while (!theController->ReplyOut.empty() && FLEXUS_CHANNEL(Reply_Out).available()) {

            DBG_(VVerb, (<< "dequeue | Reply(Out){} | " << statName()));

            MemoryTransport transport = theController->ReplyOut.dequeue();

            DBG_(VVerb, (<< "sent | Reply(Out){} | " << *(transport[MemoryMessageTag])));

            FLEXUS_CHANNEL(Reply_Out) << transport;
        }
        while (!theController->SnoopOut.empty() && FLEXUS_CHANNEL(Snoop_Out).available()) {
            if (theController->SnoopOut.peek()[DestinationTag]->isMultipleMsgs()) {
                MemoryTransport transport = theController->SnoopOut.peek();

                DBG_(VVerb, (<< "remove multicast | Snoop(Out){} | " << statName()));

                transport.set(DestinationTag, transport[DestinationTag]->removeFirstMulticastDest());
                transport.set(MemoryMessageTag, new MemoryMessage(*(transport[MemoryMessageTag])));

                DBG_(VVerb, (<< "sent | Snoop(Out){} | " << *(transport[MemoryMessageTag])));

                FLEXUS_CHANNEL(Snoop_Out) << transport;
            } else {
                DBG_(VVerb, (<< "dequeue | Snoop(Out){} | " << statName()));

                MemoryTransport transport = theController->SnoopOut.dequeue();
                DBG_Assert(transport[MemoryMessageTag]);
                DBG_Assert(transport[DestinationTag],
                           (<< statName() << " no dest for msg: " << *transport[MemoryMessageTag]));

                if (transport[DestinationTag]->type == DestinationMessage::Multicast) {

                    DBG_(VVerb, (<< "convert multicast | Snoop(Out){} | " << statName()));
                    transport[DestinationTag]->convertMulticast();
                }
                DBG_(VVerb, (<< "sent | Snoop(Out){} | " << *(transport[MemoryMessageTag])));
                FLEXUS_CHANNEL(Snoop_Out) << transport;
            }
        }
        while (!theController->RequestOut.empty() && FLEXUS_CHANNEL(Request_Out).available()) {

            DBG_(VVerb, (<< "dequeue | Request(Out){} | " << statName()));
            MemoryTransport transport = theController->RequestOut.dequeue();

            DBG_(VVerb, (<< "sent | Request(Out){} | " << *(transport[MemoryMessageTag])));
            FLEXUS_CHANNEL(Request_Out) << transport;
        }
    }
};

} // End Namespace nCMPCache

FLEXUS_COMPONENT_INSTANTIATOR(CMPCache, nCMPCache);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPCache

#define DBG_Reset
#include DBG_Control()
