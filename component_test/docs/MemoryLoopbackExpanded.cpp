#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>


struct MemoryLoopbackConfiguration_struct
{
    std::string theConfigName_;
    MemoryLoopbackConfiguration_struct (std::string const &aConfigName) : theConfigName_(aConfigName) { }

    std::string const &name() const
    {
        return theConfigName_;
    }

    template <class Parameter> struct get;

    int Delay;
    struct Delay_param {};
    int MaxRequests;
    struct MaxRequests_param {};
    bool UseFetchReply;
    struct UseFetchReply_param {};
};

template <> struct MemoryLoopbackConfiguration_struct::get< MemoryLoopbackConfiguration_struct::Delay_param>
{
    static std::string name()
    {
        return "Delay";
    }
    static std::string description()
    {
        return "Access time";
    }
    static std::string cmd_line_switch()
    {
        return "time";
    }

    typedef int type;
    static type defaultValue()
    {
        return 1;
    }
    static type MemoryLoopbackConfiguration_struct::*member()
    {
        return &MemoryLoopbackConfiguration_struct::Delay;
    }
};

template <> struct MemoryLoopbackConfiguration_struct::get< MemoryLoopbackConfiguration_struct::MaxRequests_param>
{
    static std::string name()
    {
        return "MaxRequests";
    }
    static std::string description()
    {
        return "Maximum requests queued in loopback";
    }
    static std::string cmd_line_switch()
    {
        return "max_requests";
    }

    typedef int type;
    static type defaultValue()
    {
        return 64;
    }
    static type MemoryLoopbackConfiguration_struct::*member()
    {
        return &MemoryLoopbackConfiguration_struct::MaxRequests;
    }
};

template <> struct MemoryLoopbackConfiguration_struct::get< MemoryLoopbackConfiguration_struct::UseFetchReply_param>
{
    static std::string name()
    {
        return "UseFetchReply";
    }
    static std::string description()
    {
        return "Send FetchReply in response to FetchReq (instead of MissReply)";
    }
    static std::string cmd_line_switch()
    {
        return "UseFetchReply";
    }

    typedef bool type;
    static type defaultValue()
    {
        return false;
    }
    static type MemoryLoopbackConfiguration_struct::*member()
    {
        return &MemoryLoopbackConfiguration_struct::UseFetchReply;
    }
};

struct MemoryLoopbackConfiguration
{
    typedef MemoryLoopbackConfiguration_struct cfg_struct_;
    cfg_struct_ theCfg_;

    std::string name()
    {
        return theCfg_.theConfigName_;
    }
    MemoryLoopbackConfiguration_struct & cfg()
    {
        return theCfg_;
    }
    MemoryLoopbackConfiguration (const std::string &aConfigName) : theCfg_(aConfigName), Delay(theCfg_), MaxRequests(theCfg_), UseFetchReply(theCfg_) { }

    Flexus::Core::aux_::DynamicParameter< MemoryLoopbackConfiguration_struct, MemoryLoopbackConfiguration_struct::Delay_param> Delay;
    Flexus::Core::aux_::DynamicParameter< MemoryLoopbackConfiguration_struct, MemoryLoopbackConfiguration_struct::MaxRequests_param> MaxRequests;
    Flexus::Core::aux_::DynamicParameter< MemoryLoopbackConfiguration_struct, MemoryLoopbackConfiguration_struct::UseFetchReply_param> UseFetchReply;
};



struct MemoryLoopbackJumpTable;
struct MemoryLoopbackInterface : public Flexus::Core::ComponentInterface
{
    typedef MemoryLoopbackConfiguration_struct configuration;
    typedef MemoryLoopbackJumpTable jump_table;
    static MemoryLoopbackInterface * instantiate(configuration &, jump_table &, Flexus::Core::index_t, Flexus::Core::index_t);

    struct LoopbackOut
    {
        typedef MemoryTransport payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = false;
    };

    struct LoopbackIn
    {
        typedef MemoryTransport payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = false;
    };
    virtual void push(LoopbackIn const &, LoopbackIn::payload &) = 0;
    virtual bool available(LoopbackIn const &) = 0;

    struct LoopbackDrive
    {
        static std::string name()
        {
            return "MemoryLoopbackInterface" "::" "LoopbackDrive";
        }
    };
    virtual void drive(LoopbackDrive const &) = 0;
};

struct MemoryLoopbackJumpTable
{
    typedef MemoryLoopbackInterface iface;

    bool (*wire_available_LoopbackOut)(Flexus::Core::index_t anIndex);
    void (*wire_manip_LoopbackOut)(Flexus::Core::index_t anIndex, iface::LoopbackOut::payload & aPayload);

    MemoryLoopbackJumpTable()
    {
        wire_available_LoopbackOut = 0;
    }

    void check(std::string const &anInstance)
    {
        if (wire_available_LoopbackOut == 0)
        {
            if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !0))
            {
                if ((true) && true)
                {
                    using namespace DBG_Cats;
                    using DBG_Cats::Core;
                    Flexus::Dbg::Entry entry__( 	Flexus::Dbg::Severity(6),
                                                    "./components/MemoryLoopback/MemoryLoopback.hpp",
                                                    64,
                                                    __FUNCTION__,
                                                    Flexus::Dbg::Debugger::theDebugger->count(),
                                                    Flexus::Dbg::Debugger::theDebugger->cycleCount()
                                              );
                    (void)(entry__).set(	"Message",
                                            static_cast<std::stringstream &>(
                                                std::stringstream() << std::dec << anInstance << " port " "LoopbackOut" " is not wired"
                                            ).str()
                                       );

                    Flexus::Dbg::Debugger::theDebugger->process(entry__);
                }
            }

            do { }
            while (0);
        }
    }
};

#include <components/CommonQEMU/MessageQueues.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories Memory
#define DBG_SetDefaultOps AddCat(Memory)
#include DBG_Control()

#include <components/CommonQEMU/MemoryMap.hpp>
#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

namespace nMemoryLoopback
{

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using Flexus::SharedTypes::MemoryMap;

using boost::intrusive_ptr;

class MemoryLoopbackComponent : public Flexus::Core::FlexusComponentBase<MemoryLoopbackComponent, MemoryLoopbackConfiguration, MemoryLoopbackInterface>
{
    typedef Flexus::Core::FlexusComponentBase<MemoryLoopbackComponent, MemoryLoopbackConfiguration, MemoryLoopbackInterface> base;
    typedef base::cfg_t cfg_t;
    static std::string componentType()
    {
        return "MemoryLoopback";
    }

public:
    using base::flexusIndex;
    using base::flexusWidth;
    using base::name;
    using base::statName;
    using base::cfg;
    using base::interface;
    using interface::get_channel;
    using interface::get_channel_array;

private:
    typedef base::self self;

    boost::intrusive_ptr<MemoryMap> theMemoryMap;

    MemoryMessage::MemoryMessageType theFetchReplyType;

public:
    MemoryLoopbackComponent (
        cfg_t & aCfg,
        MemoryLoopbackInterface::jump_table & aJumpTable,
        Flexus::Core::index_t anIndex,
        Flexus::Core::index_t aWidth
    ) : base(aCfg, aJumpTable, anIndex, aWidth) { }


    bool isQuiesced() const
    {
        if (!outQueue)
        {
            return true; // Quiesced before initialization
        }
        return outQueue->empty();
    }

    // Initialization
    void initialize()
    {
        if (cfg.Delay < 1)
        {
            std::cout << "Error: memory access time must be greater than 0." << std::endl;
            throw FlexusException();
        }
        theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());
        outQueue.reset(new nMessageQueues::DelayFifo<MemoryTransport>(cfg.MaxRequests));

        if (cfg.UseFetchReply)
        {
            theFetchReplyType = MemoryMessage::FetchReply;
        }
        else
        {
            theFetchReplyType = MemoryMessage::MissReplyWritable;
        }
    }

    void finalize()
    {
    }

    void fillTracker(MemoryTransport &aMessageTransport)
    {
        if (aMessageTransport[TransactionTrackerTag])
        {
            aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
            if (!aMessageTransport[TransactionTrackerTag]->fillType())
            {
                aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
            }
            aMessageTransport[TransactionTrackerTag]->setResponder(flexusIndex());
            aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
        }
    }

    void fillTracker(MemoryTransport &aMessageTransport)
    {
        if (aMessageTransport[TransactionTrackerTag])
        {
            aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
            if (!aMessageTransport[TransactionTrackerTag]->fillType())
            {
                aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
            }
            aMessageTransport[TransactionTrackerTag]->setResponder(flexusIndex());
            aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
        }
    }

    // LoopBackIn PushInput Port
    //=========================
    bool available(interface::LoopbackIn const &)
    {
        return !outQueue->full();
    }

    void push(interface::LoopbackIn const &, MemoryTransport &aMessageTransport)
    {
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0))
        {
            if ((DBG_Cats::Memory_debug_enabled) && (*this).debugEnabled())
            {
                using namespace DBG_Cats;
                using DBG_Cats::Core;
                Flexus::Dbg::Entry entry__(
                    Flexus::Dbg::Severity(1),
                    "components/MemoryLoopback/MemoryLoopbackImpl.cpp",
                    128,
                    __FUNCTION__,
                    Flexus::Dbg::Debugger::theDebugger->count(),
                    Flexus::Dbg::Debugger::theDebugger->cycleCount()
                );

                (void)(entry__).set(
                    "CompName",
                    (*this).statName()
                ).set(
                    "CompIndex",
                    (*this).flexusIndex()
                ).set
                (
                    "Message",
                    static_cast<std::stringstream &>(std::stringstream() << std::dec << "request received: " << *aMessageTransport[MemoryMessageTag]).str()
                ).set(
                    "Address",
                    (aMessageTransport[MemoryMessageTag]->address())
                );
                (entry__).addCategories(Memory);
                Flexus::Dbg::Debugger::theDebugger->process(entry__);
            }
        }
        do { }
        while (0);

        intrusive_ptr<MemoryMessage> reply;
        switch (aMessageTransport[MemoryMessageTag]->type())
        {
        case MemoryMessage::LoadReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::LoadReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::FetchReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(theFetchReplyType, aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::StoreReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::StoreReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 0;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::StorePrefetchReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::StorePrefetchReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 0;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::CmpxReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::CmpxReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::ReadReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::MissReplyWritable,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::MissReplyWritable,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::NonAllocatingStoreReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            // reply = aMessageTransport[MemoryMessageTag];
            // reply->type() = MemoryMessage::NonAllocatingStoreReply;
            // make a new msg just loks ALL the other msg types
            reply = new MemoryMessage(MemoryMessage::NonAllocatingStoreReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = aMessageTransport[MemoryMessageTag]->reqSize();
            fillTracker(aMessageTransport);
            break;

        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
            reply = new MemoryMessage(MemoryMessage::UpgradeReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 0;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::FlushReq:
        case MemoryMessage::Flush:
        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
            if (aMessageTransport[TransactionTrackerTag])
            {
                aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
                aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
                aMessageTransport[TransactionTrackerTag]->complete();
            }
            if (aMessageTransport[MemoryMessageTag]->ackRequired())
            {
                reply = new MemoryMessage(MemoryMessage::EvictAck,
                                          aMessageTransport[MemoryMessageTag]->address());
                reply->reqSize() = 0;
            }
            else
            {
                return;
            }
            break;
        case MemoryMessage::PrefetchReadAllocReq:
        case MemoryMessage::PrefetchReadNoAllocReq:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::PrefetchWritableReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::StreamFetch:
            theMemoryMap->recordAccess(aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
            reply = new MemoryMessage(MemoryMessage::StreamFetchWritableReply,
                                      aMessageTransport[MemoryMessageTag]->address());
            reply->reqSize() = 64;
            fillTracker(aMessageTransport);
            break;
        case MemoryMessage::PrefetchInsert:
            // should never happen
            if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1))
            {
                if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::Memory_debug_enabled) && !(false) && (*this).debugEnabled())
                {
                    using namespace DBG_Cats;
                    using DBG_Cats::Core;
                    Flexus::Dbg::Entry entry__(
                        Flexus::Dbg::Severity(6),
                        "components/MemoryLoopback/MemoryLoopbackImpl.cpp",
                        235,
                        __FUNCTION__,
                        Flexus::Dbg::Debugger::theDebugger->count(),
                        Flexus::Dbg::Debugger::theDebugger->cycleCount()
                    );

                    (void)(entry__).append("Condition", "!(false)").set(
                        "CompName", (*this).statName()
                    ).set(
                        "CompIndex", (*this).flexusIndex()
                    ).set(
                        "Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "MemoryLoopback received PrefetchInsert request").str()
                    );

                    (entry__).addCategories(Assert | Memory);
                    Flexus::Dbg::Debugger::theDebugger->process(entry__);
                }
            }
            do { }
            while (0) ;
            break;
        case MemoryMessage::PrefetchInsertWritable:
            // should never happen
            if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1))
            {
                if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::Memory_debug_enabled) && !(false) && (*this).debugEnabled())
                {
                    using namespace DBG_Cats;
                    using DBG_Cats::Core;
                    Flexus::Dbg::Entry entry__(
                        Flexus::Dbg::Severity(6),
                        "components/MemoryLoopback/MemoryLoopbackImpl.cpp",
                        240,
                        __FUNCTION__,
                        Flexus::Dbg::Debugger::theDebugger->count(),
                        Flexus::Dbg::Debugger::theDebugger->cycleCount()
                    );

                    (void)(entry__).append("Condition", "!(false)").set(
                        "CompName", (*this).statName()
                    ).set(
                        "CompIndex", (*this).flexusIndex()
                    ).set(
                        "Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "MemoryLoopback received PrefetchInsertWritable request").str()
                    );

                    (entry__).addCategories(Assert | Memory);
                    Flexus::Dbg::Debugger::theDebugger->process(entry__);
                }
            }
            do { }
            while (0);
            break;

        default:
            // Some more debug info ...
            aMessageTransport.set(MemoryMessageTag, reply);

            // account for the one cycle delay inherent from Flexus when sending a
            // response back up the hierarchy: actually stall for one less cycle
            // than the configuration calls for
            outQueue->enqueue(aMessageTransport, cfg.Delay - 1);
        }


        // Drive Interfaces
        void drive(interface::LoopbackDrive const &)
        {
            if (outQueue->ready() &&
                    !get_channel(interface::LoopbackOut(), jump_table_.wire_available_LoopbackOut, jump_table_.wire_manip_LoopbackOut, flexusIndex()).available())
            {
                // Some debug info
            }
            while (get_channel(interface::LoopbackOut(), jump_table_.wire_available_LoopbackOut, jump_table_.wire_manip_LoopbackOut, flexusIndex()).available() &&
                    outQueue->ready())
            {
                MemoryTransport trans(outQueue->dequeue());
                // Some debug info
                get_channel(interface::LoopbackOut(), jump_table_.wire_available_LoopbackOut, jump_table_.wire_manip_LoopbackOut, flexusIndex()) << trans;
            }
        }

private:
        std::unique_ptr<nMessageQueues::DelayFifo<MemoryTransport>> outQueue;
    };

}// End Namespace nMemoryLoopback

MemoryLoopbackInterface * MemoryLoopbackInterface::instantiate(
    MemoryLoopbackConfiguration::cfg_struct_ &aCfg,
    MemoryLoopbackInterface::jump_table &aJumpTable,
    Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth)
{
    return new nMemoryLoopback::MemoryLoopbackComponent(aCfg, aJumpTable, anIndex, aWidth);
} struct eat_semicolon_;

