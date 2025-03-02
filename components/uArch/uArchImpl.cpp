
#include "core/types.hpp"
#include <components/uArch/uArch.hpp>

#define FLEXUS_BEGIN_COMPONENT uArch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "core/qemu/configuration_api.hpp"
#include "microArch.hpp"
#include "uArchInterfaces.hpp"

#include <components/CommonQEMU/Slices/ExecuteState.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/MTManager/MTManager.hpp>
#include <core/debug/debug.hpp>
#include <core/qemu/mai_api.hpp>

#define DBG_DefineCategories uArchCat, Special
#define DBG_SetDefaultOps    AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class uArch_QemuObject_Impl
{
    std::shared_ptr<microArch> theMicroArch;

  public:
    uArch_QemuObject_Impl(Flexus::Qemu::API::conf_object_t* /*ignored*/) {}

    void setMicroArch(std::shared_ptr<microArch> aMicroArch) { theMicroArch = aMicroArch; }

    void printROB()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printROB();
    }
    void printMemQueue()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printMemQueue();
    }
    void printSRB()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printSRB();
    }
    void printMSHR()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printMSHR();
    }
    void pregs()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->pregs();
    }
    void pregsAll()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->pregsAll();
    }
    void resynchronize()
    {
        DBG_Assert(theMicroArch);
        theMicroArch->resynchronize(false, nullptr);
    }
    void printRegMappings(std::string aRegSet)
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printRegMappings(aRegSet);
    }
    void printRegFreeList(std::string aRegSet)
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printRegFreeList(aRegSet);
    }
    void printRegReverseMappings(std::string aRegSet)
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printRegReverseMappings(aRegSet);
    }
    void printRegAssignments(std::string aRegSet)
    {
        DBG_Assert(theMicroArch);
        theMicroArch->printAssignments(aRegSet);
    }
};

class uArch_QemuObject : public Qemu::AddInObject<uArch_QemuObject_Impl>
{

    typedef Qemu::AddInObject<uArch_QemuObject_Impl> base;

  public:
    static const Qemu::Persistence class_persistence = Qemu::Session;
    // These constants are defined in Simics/simics.cpp
    static std::string className() { return "uArchARM"; }
    static std::string classDescription() { return "uArchARM object"; }

    uArch_QemuObject()
      : base()
    {
    }
    uArch_QemuObject(Qemu::API::conf_object_t* aQemuObject)
      : base(aQemuObject)
    {
    }
    uArch_QemuObject(uArch_QemuObject_Impl* anImpl)
      : base(anImpl)
    {
    }
};

Qemu::Factory<uArch_QemuObject> theuArchQemuFactory;

class FLEXUS_COMPONENT(uArch)
{
    FLEXUS_COMPONENT_IMPL(uArch);

    std::shared_ptr<microArch> theMicroArch;
    uArch_QemuObject theuArchObject;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(uArch)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const { return !theMicroArch || theMicroArch->isQuiesced(); }

    void initialize()
    {
        uArchOptions_t options;

        options.ROBSize              = cfg.ROBSize;
        options.SBSize               = cfg.SBSize;
        options.NAWBypassSB          = cfg.NAWBypassSB;
        options.NAWWaitAtSync        = cfg.NAWWaitAtSync;
        options.retireWidth          = cfg.RetireWidth;
        options.numMemoryPorts       = cfg.MemoryPorts;
        options.numSnoopPorts        = cfg.SnoopPorts;
        options.numStorePrefetches   = cfg.StorePrefetches;
        options.prefetchEarly        = cfg.PrefetchEarly;
        options.spinControlEnabled   = cfg.SpinControl;
        options.consistencyModel     = (nuArch::eConsistencyModel)cfg.ConsistencyModel;
        options.coherenceUnit        = cfg.CoherenceUnit;
        options.breakOnResynchronize = cfg.BreakOnResynchronize;
        //    options.validateMMU          = cfg.ValidateMMU;
        options.speculativeOrder              = cfg.SpeculativeOrder;
        options.speculateOnAtomicValue        = cfg.SpeculateOnAtomicValue;
        options.speculateOnAtomicValuePerfect = cfg.SpeculateOnAtomicValuePerfect;
        options.speculativeCheckpoints        = cfg.SpeculativeCheckpoints;
        options.checkpointThreshold           = cfg.CheckpointThreshold;
        options.earlySGP                      = cfg.EarlySGP;              /* CMU-ONLY */
        options.trackParallelAccesses         = cfg.TrackParallelAccesses; /* CMU-ONLY */
        options.inOrderMemory                 = cfg.InOrderMemory;
        options.inOrderExecute                = cfg.InOrderExecute;
        options.onChipLatency                 = cfg.OnChipLatency;
        options.offChipLatency                = cfg.OffChipLatency;
        options.name                          = statName();
        options.node                          = flexusIndex();

        options.numIntAlu                 = cfg.NumIntAlu;
        options.intAluOpLatency           = cfg.IntAluOpLatency;
        options.intAluOpPipelineResetTime = cfg.IntAluOpPipelineResetTime;

        options.numIntMult                 = cfg.NumIntMult;
        options.intMultOpLatency           = cfg.IntMultOpLatency;
        options.intMultOpPipelineResetTime = cfg.IntMultOpPipelineResetTime;
        options.intDivOpLatency            = cfg.IntDivOpLatency;
        options.intDivOpPipelineResetTime  = cfg.IntDivOpPipelineResetTime;

        options.numFpAlu                 = cfg.NumFpAlu;
        options.fpAddOpLatency           = cfg.FpAddOpLatency;
        options.fpAddOpPipelineResetTime = cfg.FpAddOpPipelineResetTime;
        options.fpCmpOpLatency           = cfg.FpCmpOpLatency;
        options.fpCmpOpPipelineResetTime = cfg.FpCmpOpPipelineResetTime;
        options.fpCvtOpLatency           = cfg.FpCvtOpLatency;
        options.fpCvtOpPipelineResetTime = cfg.FpCvtOpPipelineResetTime;

        options.numFpMult                 = cfg.NumFpMult;
        options.fpMultOpLatency           = cfg.FpMultOpLatency;
        options.fpMultOpPipelineResetTime = cfg.FpMultOpPipelineResetTime;
        options.fpDivOpLatency            = cfg.FpDivOpLatency;
        options.fpDivOpPipelineResetTime  = cfg.FpDivOpPipelineResetTime;
        options.fpSqrtOpLatency           = cfg.FpSqrtOpLatency;
        options.fpSqrtOpPipelineResetTime = cfg.FpSqrtOpPipelineResetTime;

        theMicroArch = microArch::construct(options,
                                            ll::bind(&uArchComponent::squash, this, ll::_1),
                                            ll::bind(&uArchComponent::redirect, this, ll::_1),
                                            ll::bind(&uArchComponent::trainBP, this, ll::_1),
                                            ll::bind(&uArchComponent::signalStoreForwardingHit, this, ll::_1),
                                            ll::bind(&uArchComponent::resyncMMU, this, ll::_1));

        theuArchObject = theuArchQemuFactory.create(
          (std::string("uarch-") + boost::padded_string_cast<2, '0'>(flexusIndex())).c_str());
        theuArchObject->setMicroArch(theMicroArch);
    }

    void finalize() {}

  public:
    FLEXUS_PORT_ALWAYS_AVAILABLE(DispatchIn);
    void push(interface::DispatchIn const&, boost::intrusive_ptr<AbstractInstruction>& anInstruction)
    {
        DBG_(VVerb, (<< "Get the inst in uArch: "));
        theMicroArch->dispatch(anInstruction);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableDispatchOut);
    std::pair<int, bool> pull(AvailableDispatchOut const&)
    {
        return std::make_pair(theMicroArch->availableROB(), theMicroArch->isSynchronized());
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const&) { return theMicroArch->isStalled(); }

    FLEXUS_PORT_ALWAYS_AVAILABLE(CoreHalted);
    bool pull(CoreHalted const&) { return theMicroArch->isHalted(); }

    FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
    int32_t pull(ICount const&) { return theMicroArch->iCount(); }

    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryIn);
    void push(interface::MemoryIn const&, MemoryTransport& aTransport) { handleMemoryMessage(aTransport); }

    FLEXUS_PORT_ALWAYS_AVAILABLE(WritePermissionLost);
    void push(interface::WritePermissionLost const&, PhysicalMemoryAddress& anAddress)
    {
        theMicroArch->writePermissionLost(anAddress);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(dTranslationIn);
    void push(interface::dTranslationIn const&, TranslationPtr& aTranslate)
    {

        PhysicalMemoryAddress magicTranslation =
          Flexus::Qemu::Processor::getProcessor(theMicroArch->core()).translate_va2pa(aTranslate->theVaddr, (aTranslate->getInstruction() ? aTranslate->getInstruction()->unprivAccess(): false));

        if (aTranslate->thePaddr == magicTranslation || magicTranslation == kUnresolved) {
            DBG_(VVerb,
                 (<< "Magic QEMU translation == MMU Translation. Vaddr = " << std::hex << aTranslate->theVaddr
                  << ", PADDR_MMU = " << aTranslate->thePaddr << ", PADDR_QEMU = " << magicTranslation << std::dec
                  << ", ID: " << aTranslate->theID));
        } else {
            if (!aTranslate->isPagefault()) {
                DBG_Assert(false,
                           (<< "ERROR: Magic QEMU translation NOT EQUAL TO MMU Translation. Vaddr = " << std::hex
                            << aTranslate->theVaddr << ", PADDR_MMU = " << aTranslate->thePaddr
                            << ", PADDR_QEMU = " << magicTranslation << std::dec));
            }
        }
        // respect qemu result as flexus does not have pmp
        // TODO: but this should only happen for access faults
        if (!aTranslate->isPagefault() && (magicTranslation == kUnresolved)) aTranslate->setPagefault();

        aTranslate->thePaddr = magicTranslation;
        theMicroArch->pushTranslation(aTranslate);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryRequestIn);
    void push(interface::MemoryRequestIn const&, TranslationPtr& aTranslation) { theMicroArch->issueMMU(aTranslation); }

  public:
    // The FetchDrive drive interface sends a commands to the Feeder and then
    // fetches instructions, passing each instruction to its FetchOut port.
    void drive(interface::uArchDrive const&) { doCycle(); }

  private:

    void squash(eSquashCause aSquashReason) { FLEXUS_CHANNEL(SquashOut) << aSquashReason; }
    void resyncMMU(int32_t aNode) { FLEXUS_CHANNEL(ResyncOut) << aNode; }

    void redirect(boost::intrusive_ptr<BPredRedictRequest> aRequest)
    {
        FLEXUS_CHANNEL(RedirectOut) << aRequest;
    }

    void trainBP(boost::intrusive_ptr<BPredState> aBPState) { FLEXUS_CHANNEL(BranchTrainOut) << aBPState; }

    void signalStoreForwardingHit(bool garbage)
    {
        bool value = true;
        FLEXUS_CHANNEL(StoreForwardingHitSeen) << value;
    }

    void doCycle()
    {
        if (cfg.Multithread) {
            if (nMTManager::MTManager::get()->runThisEX(flexusIndex())) {
                theMicroArch->cycle();
            } else {
                theMicroArch->skipCycle();
            }
        } else {
            theMicroArch->cycle();
        }
        sendMemoryMessages();
        requestTranslations();
    }

    void requestTranslations()
    {
        while (FLEXUS_CHANNEL(dTranslationOut).available()) {
            TranslationPtr op(theMicroArch->popTranslation());
            if (!op) break;

            FLEXUS_CHANNEL(dTranslationOut) << op;
        }
    }

    void sendMemoryMessages()
    {
        while (FLEXUS_CHANNEL(MemoryOut_Request).available()) {
            boost::intrusive_ptr<MemOp> op(theMicroArch->popMemOp());
            if (!op) break;

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;

            DBG_(VVerb,
                 (<< "Sending Memory Request: " << op->theOperation << "  -- vaddr: " << op->theVAddr << "  -- paddr: "
                  << op->thePAddr
                  //  << "  --  Instruction: " <<  *(op->theInstruction)
                  << " --  PC: " << op->thePC << " -- size: " << op->theSize));

            if (op->theNAW) {
                DBG_Assert(op->theOperation == kStore);
                operation = new MemoryMessage(MemoryMessage::NonAllocatingStoreReq, op->thePAddr, op->thePC);
            } else {

                switch (op->theOperation) {
                    case kLoad:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newLoad(op->thePAddr, op->thePC);
                        break;

                    case kAtomicPreload:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newAtomicPreload(op->thePAddr, op->thePC);
                        break;

                    case kStorePrefetch:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newStorePrefetch(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kStore:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newStore(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kRMW:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newRMW(op->thePAddr, op->thePC, op->theValue);
                        break;

                    case kCAS:
                        // pc =
                        // Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
                        operation = MemoryMessage::newCAS(op->thePAddr, op->thePC, op->theValue);
                        break;
                    case kPageWalkRequest:
                        operation = MemoryMessage::newPWRequest(op->thePAddr);
                        operation->setPageWalk();
                        break;
                    default: DBG_Assert(false, (<< "Unknown memory operation type: " << op->theOperation));
                }
            }
            if (op->theOperation != kPageWalkRequest) operation->theInstruction = op->theInstruction;

            operation->reqSize() = op->theSize;
            if (op->theTracker) {
                transport.set(TransactionTrackerTag, op->theTracker);
            } else {
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                tracker->setAddress(op->thePAddr);
                tracker->setInitiator(flexusIndex());
                tracker->setSource("uArchARM");
                tracker->setOS(false); // TWENISCH - need to set this properly
                transport.set(TransactionTrackerTag, tracker);
                op->theTracker = tracker;
            }

            transport.set(MemoryMessageTag, operation);
            transport.set(uArchStateTag, op);

            if (op->theNAW && (op->thePAddr & 63) != 0) {
                // Auto-reply to the unaligned parts of NAW
                transport[MemoryMessageTag]->type() = MemoryMessage::NonAllocatingStoreReply;
                handleMemoryMessage(transport);
            } else {
                FLEXUS_CHANNEL(MemoryOut_Request) << transport;
            }
        }

        while (FLEXUS_CHANNEL(MemoryOut_Snoop).available()) {
            boost::intrusive_ptr<MemOp> op(theMicroArch->popSnoopOp());
            if (!op) break;

            DBG_(VVerb, (<< "Send Snoop: " << *op));

            MemoryTransport transport;
            boost::intrusive_ptr<MemoryMessage> operation;

            PhysicalMemoryAddress pc;

            switch (op->theOperation) {
                case kInvAck:
                    DBG_(Verb, (<< "Send InvAck."));
                    operation = new MemoryMessage(MemoryMessage::InvalidateAck, op->thePAddr);
                    break;

                case kDowngradeAck:
                    DBG_(Verb, (<< "Send DowngradeAck."));
                    operation = new MemoryMessage(MemoryMessage::DowngradeAck, op->thePAddr);
                    break;

                case kProbeAck: operation = new MemoryMessage(MemoryMessage::ProbedNotPresent, op->thePAddr); break;

                case kReturnReply: operation = new MemoryMessage(MemoryMessage::ReturnReply, op->thePAddr); break;

                default: DBG_Assert(false, (<< "Unknown memory operation type: " << op->theOperation));
            }

            operation->reqSize() = op->theSize;
            if (op->theTracker) {
                transport.set(TransactionTrackerTag, op->theTracker);
            } else {
                boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
                tracker->setAddress(op->thePAddr);
                tracker->setInitiator(flexusIndex());
                tracker->setSource("uArchARM");
                transport.set(TransactionTrackerTag, tracker);
            }

            transport.set(MemoryMessageTag, operation);

            FLEXUS_CHANNEL(MemoryOut_Snoop) << transport;
        }
    }

    void handleMemoryMessage(MemoryTransport& aTransport)
    {
        boost::intrusive_ptr<MemOp> op;
        boost::intrusive_ptr<MemoryMessage> msg(aTransport[MemoryMessageTag]);

        // For Invalidates and Downgrades, the uArchState isn't for us, it's for the
        // original requester So in those cases we always want to construct a new
        // MemOp based on the MemoryMesage
        if (msg->isPageWalk()) {}
        if (aTransport[uArchStateTag] && msg->type() != MemoryMessage::Invalidate &&
            msg->type() != MemoryMessage::Downgrade) {
            op = aTransport[uArchStateTag];
        } else {
            op             = new MemOp();
            op->thePAddr   = msg->address();
            op->theSize    = eSize(msg->reqSize());
            op->theTracker = aTransport[TransactionTrackerTag];
        }

        switch (msg->type()) {
            case MemoryMessage::LoadReply: op->theOperation = kLoadReply; break;

            case MemoryMessage::AtomicPreloadReply: op->theOperation = kAtomicPreloadReply; break;

            case MemoryMessage::StoreReply: op->theOperation = kStoreReply; break;

            case MemoryMessage::NonAllocatingStoreReply: op->theOperation = kStoreReply; break;

            case MemoryMessage::StorePrefetchReply: op->theOperation = kStorePrefetchReply; break;

            case MemoryMessage::Invalidate: op->theOperation = kInvalidate; break;

            case MemoryMessage::Downgrade: op->theOperation = kDowngrade; break;

            case MemoryMessage::Probe: op->theOperation = kProbe; break;

            case MemoryMessage::RMWReply: op->theOperation = kRMWReply; break;

            case MemoryMessage::CmpxReply: op->theOperation = kCASReply; break;

            case MemoryMessage::ReturnReq: op->theOperation = kReturnReq; break;

            default: DBG_Assert(false, (<< "Unhandled Memory Message type: " << msg->type()));
        }

        theMicroArch->pushMemOp(op);
    }
};

} // End namespace nuArchARM

FLEXUS_COMPONENT_INSTANTIATOR(uArch, nuArch);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uArch

#define DBG_Reset
#include DBG_Control()
