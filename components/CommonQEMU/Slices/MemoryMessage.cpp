#include <components/CommonQEMU/Slices/MemoryMessage.hpp>

namespace Flexus {
namespace SharedTypes {

static uint32_t theMemoryMessageSerial = 0;

uint32_t
memoryMessageSerial(void)
{
    return ++theMemoryMessageSerial;
}

std::ostream&
operator<<(std::ostream& s, MemoryMessage::MemoryMessageType const& aMemMsgType)
{
    static char const* message_types[] = { "Load Request",
                                           "Store Request",
                                           "Store Prefetch Request",
                                           "Fetch Request",
                                           "Non-allocating Store Request",
                                           "RMW Request",
                                           "Cmp-Swap Request",
                                           "Atomic Preload Request",
                                           "Flush Request",
                                           "Read Request",

                                           "Write Request",
                                           "Write Allocate",
                                           "Upgrade Request",
                                           "Upgrade Allocate",
                                           "Flush",
                                           "Eviction (dirty)",
                                           "Eviction (writable)",
                                           "Eviction (clean)",
                                           "SVB Clean Eviction",
                                           "Load Reply",
                                           "Store Reply",
                                           "Store Prefetch Reply",
                                           "Fetch Reply",
                                           "RMW Reply",
                                           "Cmp-Swap Reply",
                                           "Atomic Preload Reply",
                                           "Miss Reply",
                                           "Miss Writable Reply",
                                           "Miss Dirty Reply",

                                           "Upgrade Reply",
                                           "Non-Allocating Store Reply",
                                           "Invalidate",
                                           "Downgrade",
                                           "Probe",
                                           "DownProbe",
                                           "ReturnReq",
                                           "Invalidate Ack",
                                           "Invalidate Update Ack",
                                           "Downgrade Ack",

                                           "Downgrade Update Ack",
                                           "Probed - Not Present",
                                           "Probed - Clean",
                                           "Probed - Writable",
                                           "Probed - Dirty",
                                           "DownProbe - Present",
                                           "DownProbe - Not Present",
                                           "ReturnReply",
                                           "Stream Fetch",

                                           "Prefetch Read No-Allocate Request",
                                           "Prefetch Read Allocate",
                                           "Prefetch Insert Request",
                                           "Prefetch Insert Writable Request",
                                           "Prefetch Read Reply",
                                           "Prefetch Read Reply Writable",
                                           "Prefetch Read Reply Dirty",
                                           "Prefetch Read Redundant",
                                           "Stream Fetch Writable Reply",
                                           "Stream Fetch Rejected",
                                           "ReturnNAck",
                                           "ReturnReplyDirty",
                                           "FetchFwd",
                                           "ReadFwd",
                                           "WriteFwd",
                                           "FwdNAck",
                                           "FwdReply",
                                           "FwdReplyOwned",
                                           "FwdReplyWritable",
                                           "FwdReplyDirty",
                                           "ReadAck",
                                           "ReadAckDirty",
                                           "FetchAck",
                                           "FetchAckDirty",
                                           "WriteAck",
                                           "UpgradeAck",
                                           "Non-Allocating Store Ack",
                                           "ReadNAck",
                                           "FetchNAck",
                                           "WriteNAck",
                                           "UpgradeNAck",
                                           "Non-Allocating Store NAck",
                                           "MissNotify",
                                           "MissNotifyData",
                                           "BackInvalidate",
                                           "InvalidateNAck",
                                           "Protocol Message",
                                           "Evict Ack",
                                           "Write Retry",
                                           "NumMemoryMessageTypes" };
    assert(aMemMsgType <= MemoryMessage::NumMemoryMessageTypes);
    return s << message_types[aMemMsgType];
}
std::ostream&
operator<<(std::ostream& s, MemoryMessage const& mem)
{
    // if (aMemMsg.theInstruction) { s << "instr=>> " << *(aMemMsg.theInstruction) << " << "; }

    s << "MemoryMessage(" << mem.type() << ")";
    s << " | " << mem.address();
    s << " | From(" << mem.coreIdx() << ")";
    if (mem.isPageWalk()) s << "| PageWalk";
    if (mem.ackRequired()) {
        s << " | Require(Ack";

        if (mem.ackRequiresData()) s << "+Data";

        s << ")";
    }

    //    s << "MemoryMessage[" << aMemMsg.type() << "]: Addr:0x" << std::hex << aMemMsg.address() << " Size:" <<
    //    std::dec << aMemMsg.reqSize() << " Serial: " << aMemMsg.serial() << " Core: " << aMemMsg.coreIdx()
    //      << " DStream: " << std::boolalpha << aMemMsg.isDstream() << " Outstanding Msgs: " <<
    //      aMemMsg.outstandingMsgs()
    //      << " PageWalk: " << aMemMsg.isPageWalk()
    //      << (aMemMsg.ackRequired() ? (aMemMsg.ackRequiresData() ? " Requires Ack+Data" : " Requires Ack") : "");

    return s;
}

} // namespace SharedTypes
} // namespace Flexus
