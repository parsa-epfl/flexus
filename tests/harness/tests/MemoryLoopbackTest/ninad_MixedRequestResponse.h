#include <gtest/gtest.h>
#include <vector>
#include <utility>
#include <boost/range/combine.hpp>

TEST_F(MemoryLoopbackTestFixture, MixedRequestResponse)
{
    MemoryLoopbackConfiguration_struct aCfg("MixedRequestTestConfig");
    aCfg.Delay = 2;
    aCfg.MaxRequests = 128;
    aCfg.UseFetchReply = true;

    MemoryLoopbackJumpTable aJumpTable;
    aJumpTable.wire_available_LoopbackOut = LoopbackOut_avail_message_type;
    aJumpTable.wire_manip_LoopbackOut = LoopbackOut_manip_message_type;

    Flexus::Core::index_t anIndex = 1;
    Flexus::Core::index_t aWidth = 1;

    nMemoryLoopback::MemoryLoopbackComponent dut(
        aCfg,
        aJumpTable,
        anIndex,
        aWidth);

    dut.initialize();

    MemoryLoopbackInterface::LoopbackDrive LoopbackDrive_tmp;
    MemoryLoopbackInterface::LoopbackIn LoopbackIn_tmp;

    std::vector<std::pair<MemoryMessage::MemoryMessageType, uint64_t>> trigger =
    {
        std::make_pair(MemoryMessage::LoadReq, 0x1000),
        std::make_pair(MemoryMessage::StoreReq, 0x2000),
        std::make_pair(MemoryMessage::FetchReq, 0x3000),
        std::make_pair(MemoryMessage::WriteReq, 0x4000),
        std::make_pair(MemoryMessage::CmpxReq, 0x5000),
        std::make_pair(MemoryMessage::ReadReq, 0x6000)
    };

    std::vector<std::pair<MemoryMessage::MemoryMessageType, uint64_t>> expected =
    {
        std::make_pair(MemoryMessage::LoadReply, 0x1000),
        std::make_pair(MemoryMessage::StoreReply, 0x2000),
        std::make_pair(MemoryMessage::MissReplyWritable, 0x3000),
        std::make_pair(MemoryMessage::MissReplyWritable, 0x4000),
        std::make_pair(MemoryMessage::CmpxReply, 0x5000),
        std::make_pair(MemoryMessage::MissReplyWritable, 0x6000)
    };

    for (auto testCase : boost::combine(trigger, expected))
    {
        std::pair<MemoryMessage::MemoryMessageType, uint64_t> trig, exp;
        boost::tie(trig, exp) = testCase;

        ASSERT_EQ(dut.available(LoopbackIn_tmp), true) << "LoopbackIn is not ready to receive data. Failed!";

        using boost::intrusive_ptr;
        MemoryTransport aMemoryTransport;
        intrusive_ptr<MemoryMessage> aMemoryMessage = new MemoryMessage(trig.first, PhysicalMemoryAddress(trig.second));
        aMemoryTransport.set(MemoryMessageTag, aMemoryMessage);

        dut.push(LoopbackIn_tmp, aMemoryTransport);

        dut.drive(LoopbackDrive_tmp);

        ASSERT_EQ(exp.first, type) << "Got wrong message type. Failed!";
        ASSERT_EQ(exp.second, addr) << "Got wrong address from DUT. Failed! Got " << std::hex << addr << " Expected " << exp.second;
    }
}
