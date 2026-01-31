
#include <components/CommonQEMU/MessageQueues.hpp>
#include <components/MemoryLoopback/MemoryLoopback.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories Memory
#define DBG_SetDefaultOps    AddCat(Memory)
#include DBG_Control()

#include <components/CommonQEMU/MemoryMap.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

namespace nMemoryLoopback {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using Flexus::SharedTypes::MemoryMap;

using boost::intrusive_ptr;

class FLEXUS_COMPONENT(MemoryLoopback)
{
    FLEXUS_COMPONENT_IMPL(MemoryLoopback);

    boost::intrusive_ptr<MemoryMap> theMemoryMap;

    MemoryMessage::MemoryMessageType theFetchReplyType;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MemoryLoopback)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const
    {
        if (!outQueue) {
            return true; // Quiesced before initialization
        }
        return outQueue->empty();
    }

    // Initialization
    void initialize()
    {
        if (cfg.Delay < 1) {
            std::cout << "Error: memory access time must be greater than 0." << std::endl;
            throw FlexusException();
        }
        theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());
        outQueue.reset(new nMessageQueues::DelayFifo<MemoryTransport>(cfg.MaxRequests));

        if (cfg.UseFetchReply) {
            theFetchReplyType = MemoryMessage::FetchReply;
        } else {
            theFetchReplyType = MemoryMessage::MissReplyWritable;
        }
    }

    void finalize() {}

    void fillTracker(MemoryTransport& aMessageTransport)
    {
        if (aMessageTransport[TransactionTrackerTag]) {
            aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
            if (!aMessageTransport[TransactionTrackerTag]->fillType()) {
                aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
            }
            aMessageTransport[TransactionTrackerTag]->setResponder(flexusIndex());
            aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
        }
    }

    // LoopBackIn PushInput Port
    //=========================
    bool available(interface::LoopbackIn const&) { return !outQueue->full(); }

    void push(interface::LoopbackIn const&, MemoryTransport& aMessageTransport)
    {
        DBG_(VVerb,
             Comp(*this)(<< "request received: " << *aMessageTransport[MemoryMessageTag])
               Addr(aMessageTransport[MemoryMessageTag]->address()));
        intrusive_ptr<MemoryMessage> reply;
        switch (aMessageTransport[MemoryMessageTag]->type()) {
            case MemoryMessage::LoadReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
                reply = new MemoryMessage(MemoryMessage::LoadReply, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::FetchReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
                reply            = new MemoryMessage(theFetchReplyType, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::StoreReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                reply = new MemoryMessage(MemoryMessage::StoreReply, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 0;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::StorePrefetchReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                reply =
                  new MemoryMessage(MemoryMessage::StorePrefetchReply, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 0;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::CmpxReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                reply = new MemoryMessage(MemoryMessage::CmpxReply, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;

            case MemoryMessage::ReadReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
                reply =
                  new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::WriteReq:
            case MemoryMessage::WriteAllocate:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                reply =
                  new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::NonAllocatingStoreReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                // reply = aMessageTransport[MemoryMessageTag];
                // reply->type() = MemoryMessage::NonAllocatingStoreReply;
                // make a new msg just loks ALL the other msg types
                reply            = new MemoryMessage(MemoryMessage::NonAllocatingStoreReply,
                                          aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = aMessageTransport[MemoryMessageTag]->reqSize();
                fillTracker(aMessageTransport);
                break;

            case MemoryMessage::UpgradeReq:
            case MemoryMessage::UpgradeAllocate:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
                reply = new MemoryMessage(MemoryMessage::UpgradeReply, aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 0;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::FlushReq:
            case MemoryMessage::Flush:
            case MemoryMessage::EvictDirty:
            case MemoryMessage::EvictWritable:
            case MemoryMessage::EvictClean:
                if (aMessageTransport[TransactionTrackerTag]) {
                    aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
                    aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
                    aMessageTransport[TransactionTrackerTag]->complete();
                }
                if (aMessageTransport[MemoryMessageTag]->ackRequired()) {
                    reply = new MemoryMessage(MemoryMessage::EvictAck, aMessageTransport[MemoryMessageTag]->address());
                    reply->reqSize() = 0;
                } else {
                    return;
                }
                break;
            case MemoryMessage::PrefetchReadAllocReq:
            case MemoryMessage::PrefetchReadNoAllocReq:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
                reply            = new MemoryMessage(MemoryMessage::PrefetchWritableReply,
                                          aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::StreamFetch:
                theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
                reply            = new MemoryMessage(MemoryMessage::StreamFetchWritableReply,
                                          aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 64;
                fillTracker(aMessageTransport);
                break;
            case MemoryMessage::PrefetchInsert:
                // should never happen
                DBG_Assert(false, Component(*this)(<< "MemoryLoopback received PrefetchInsert request"));
                break;
            case MemoryMessage::PrefetchInsertWritable:
                // should never happen
                DBG_Assert(false, Component(*this)(<< "MemoryLoopback received PrefetchInsertWritable request"));
                break;

            default:
                DBG_Assert(false,
                           Component(*this)(<< "Don't know how to handle message: "
                                            << aMessageTransport[MemoryMessageTag]->type() << "  No reply sent."));
                return;
        }
        DBG_(VVerb, Comp(*this)(<< "Queueing reply: " << *reply) Addr(aMessageTransport[MemoryMessageTag]->address()));
        aMessageTransport.set(MemoryMessageTag, reply);

        // account for the one cycle delay inherent from Flexus when sending a
        // response back up the hierarchy: actually stall for one less cycle
        // than the configuration calls for
        outQueue->enqueue(aMessageTransport, cfg.Delay - 1);
    }

    // Drive Interfaces
    void drive(interface::LoopbackDrive const&)
    {
        if (outQueue->ready() && !FLEXUS_CHANNEL(LoopbackOut).available()) {
            DBG_(VVerb, Comp(*this)(<< "Failed to send reply, channel not available."));
        }
        while (FLEXUS_CHANNEL(LoopbackOut).available() && outQueue->ready()) {
            MemoryTransport trans(outQueue->dequeue());
            DBG_(VVerb,
                 Comp(*this)(<< "Sending reply: " << *(trans[MemoryMessageTag]))
                   Addr(trans[MemoryMessageTag]->address()));
            FLEXUS_CHANNEL(LoopbackOut) << trans;
        }
    }

  private:
    std::unique_ptr<nMessageQueues::DelayFifo<MemoryTransport>> outQueue;
};

} // End Namespace nMemoryLoopback

FLEXUS_COMPONENT_INSTANTIATOR(MemoryLoopback, nMemoryLoopback);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MemoryLoopback

#define DBG_Reset
#include DBG_Control()
