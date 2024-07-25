//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <components/FastMemoryLoopback/FastMemoryLoopback.hpp>
#include <components/FastMemoryLoopback/MemStats.hpp>
#include <core/stats.hpp>

#define FLEXUS_BEGIN_COMPONENT FastMemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FastMemoryLoopback
#define DBG_SetDefaultOps    AddCat(FastMemoryLoopback)
#include DBG_Control()

namespace nFastMemoryLoopback {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(FastMemoryLoopback)
{
    FLEXUS_COMPONENT_IMPL(FastMemoryLoopback);

    MemStats* theStats;

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(FastMemoryLoopback)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    // Initialization
    void initialize() { theStats = new MemStats(statName()); }

    void finalize() {}

    // Ports
    FLEXUS_PORT_ALWAYS_AVAILABLE(FromCache);
    void push(interface::FromCache const&, MemoryMessage& message)
    {

         message.fillLevel() = eLocalMem;


        DBG_(Iface, Addr(message.address())(<< "request received: " << message));
        switch (message.type()) {
            case MemoryMessage::LoadReq:
            case MemoryMessage::FetchReq:
                message.type()      = MemoryMessage::MissReply;
                (theStats->theReadRequests_stat)++;
                break;
            case MemoryMessage::ReadReq:
            case MemoryMessage::PrefetchReadNoAllocReq:
            case MemoryMessage::PrefetchReadAllocReq:
                message.type()      = MemoryMessage::MissReply;
                (theStats->theReadRequests_stat)++;
                break;
            case MemoryMessage::StoreReq:
            case MemoryMessage::StorePrefetchReq:
            case MemoryMessage::RMWReq:
            case MemoryMessage::CmpxReq:
            case MemoryMessage::WriteReq:
            case MemoryMessage::WriteAllocate: (theStats->theWriteRequests_stat)++; break;
            case MemoryMessage::UpgradeReq:
            case MemoryMessage::UpgradeAllocate:
                message.type()      = MemoryMessage::MissReplyWritable;
                (theStats->theUpgradeRequest_stat)++;
                break;
            case MemoryMessage::EvictDirty: (theStats->theEvictDirtys_stat)++; break;
            case MemoryMessage::EvictWritable: (theStats->theEvictWritables_stat)++; break;
            case MemoryMessage::EvictClean:
                // no response necessary (and no state to track here)
                (theStats->theEvictCleans_stat)++;
                break;
            case MemoryMessage::NonAllocatingStoreReq:
                message.type()      = MemoryMessage::NonAllocatingStoreReply;
                (theStats->theNonAllocatingStoreReq_stat)++;
                break;
            default: DBG_Assert(false, (<< "unknown request received: " << message));
        }
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(DMA);
    void push(interface::DMA const&, MemoryMessage& message)
    {
        DBG_(Iface, Addr(message.address())(<< "DMA " << message));
        if (message.type() == MemoryMessage::WriteReq) {
            (theStats->theWriteDMA_stat)++;
            DBG_(Trace, Addr(message.address())(<< "Sending Invalidate in response to DMA " << message));
            message.type() = MemoryMessage::Invalidate;
            FLEXUS_CHANNEL(ToCache) << message;
        } else {
            (theStats->theReadDMA_stat)++;
            DBG_Assert(message.type() == MemoryMessage::ReadReq);
            message.type() = MemoryMessage::Downgrade;
            FLEXUS_CHANNEL(ToCache) << message;
        }
    }

    void drive(interface::UpdateStatsDrive const&) { theStats->update(); }
};

} // End namespace nFastMemoryLoopback

FLEXUS_COMPONENT_INSTANTIATOR(FastMemoryLoopback, nFastMemoryLoopback);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastMemoryLoopback

#define DBG_Reset
#include DBG_Control()