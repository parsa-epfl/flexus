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
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index_container.hpp>
#include <components/CommonQEMU/Slices/MemoryMessage.hpp>
#include <components/FastCMPCache/AbstractProtocol.hpp>
#include <core/debug/debug.hpp>

using namespace boost::multi_index;

#include <list>

namespace nFastCMPCache {

class SingleCMPProtocol : public AbstractProtocol
{
  private:
    struct hash_entry_t
    {
        CoherenceState_t c_state;
        SharingState d_state;
        MemoryMessage::MemoryMessageType m_type;
        PrimaryAction action;

        hash_entry_t(const CoherenceState_t& cs,
                     const SharingState& ds,
                     const MemoryMessage::MemoryMessageType& mt,
                     const PrimaryAction& a)
          : c_state(cs)
          , d_state(ds)
          , m_type(mt)
          , action(a)
        {
        }
    };

    typedef multi_index_container<
      hash_entry_t,
      indexed_by<
        hashed_unique<composite_key<hash_entry_t,
                                    member<hash_entry_t, CoherenceState_t, &hash_entry_t::c_state>,
                                    member<hash_entry_t, SharingState, &hash_entry_t::d_state>,
                                    member<hash_entry_t, MemoryMessage::MemoryMessageType, &hash_entry_t::m_type>>>>>
      protocol_hash_t;

    protocol_hash_t theProtocolHash;

    PrimaryAction poison_action;

#define ADD_ACTION(req,                                                                                                \
                   c_state,                                                                                            \
                   d_state,                                                                                            \
                   snoop,                                                                                              \
                   terminal1,                                                                                          \
                   terminal2,                                                                                          \
                   resp1,                                                                                              \
                   resp2,                                                                                              \
                   ns1,                                                                                                \
                   ns2,                                                                                                \
                   multi,                                                                                              \
                   fwd,                                                                                                \
                   alloc,                                                                                              \
                   update)                                                                                             \
    theProtocolHash.insert(hash_entry_t(c_state,                                                                       \
                                        d_state,                                                                       \
                                        Flexus::SharedTypes::MemoryMessage::req,                                       \
                                        PrimaryAction(Flexus::SharedTypes::MemoryMessage::snoop,                       \
                                                      Flexus::SharedTypes::MemoryMessage::terminal1,                   \
                                                      Flexus::SharedTypes::MemoryMessage::terminal2,                   \
                                                      Flexus::SharedTypes::MemoryMessage::resp1,                       \
                                                      Flexus::SharedTypes::MemoryMessage::resp2,                       \
                                                      ns1,                                                             \
                                                      ns2,                                                             \
                                                      multi,                                                           \
                                                      fwd,                                                             \
                                                      alloc,                                                           \
                                                      update,                                                          \
                                                      &CacheStats::req##_##c_state##_##d_state,                        \
                                                      false)))

#define ADD_POISON_ACTION(req, c_state, d_state)                                                                       \
    theProtocolHash.insert(hash_entry_t(c_state,                                                                       \
                                        d_state,                                                                       \
                                        Flexus::SharedTypes::MemoryMessage::req,                                       \
                                        PrimaryAction(Flexus::SharedTypes::MemoryMessage::NumMemoryMessageTypes,       \
                                                      Flexus::SharedTypes::MemoryMessage::NumMemoryMessageTypes,       \
                                                      Flexus::SharedTypes::MemoryMessage::NumMemoryMessageTypes,       \
                                                      Flexus::SharedTypes::MemoryMessage::NumMemoryMessageTypes,       \
                                                      Flexus::SharedTypes::MemoryMessage::NumMemoryMessageTypes,       \
                                                      kInvalid,                                                        \
                                                      kInvalid,                                                        \
                                                      false,                                                           \
                                                      false,                                                           \
                                                      false,                                                           \
                                                      false,                                                           \
                                                      &CacheStats::NoStat,                                             \
                                                      true)))

#define NO_RESP NumMemoryMessageTypes
#define NO_REQ  NumMemoryMessageTypes
#define NO_FWD  NumMemoryMessageTypes

    /* Some protocol notes:
     * Exlcusive means exclusive on chip. One Sharer when in Exclusive state means
     * the sharer *might* be in Exclusive or Modified state One sharer in Shared
     * state means that one sharer is also in shared state.
     */

    inline void initializeProtocol()
    {

        //         Request, CState,  DState,   Snoop,  Terminal[0], Terminal[1],
        //         Response1, Response2, NS[1],  NS[2],  Multi, Fwd, Alloc, Update,

        // ADD_ACTION(ReadReq, kModified, ZeroSharers, NoRequest, NoRequest,
        // NoRequest,   MissReplyWritable,NO_RESP, kModified, kModified, false,
        // false, false, true);
        ADD_ACTION(ReadReq,
                   kModified,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyDirty,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        // ADD_ACTION(ReadReq, kModified, OneSharer,  ReturnReq, ReturnReply,
        // ReturnReplyDirty, MissReply, MissReply, kModified, kModified, false,
        // false, false, true);
        ADD_ACTION(ReadReq,
                   kModified,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(ReadReq,
                   kModified,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(ReadReq,
                   kExclusive,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyWritable,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(ReadReq,
                   kExclusive,
                   OneSharer,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kExclusive,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(ReadReq,
                   kExclusive,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(ReadReq,
                   kShared,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(ReadReq,
                   kShared,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(ReadReq,
                   kShared,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(ReadReq,
                   kInvalid,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyWritable,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   true,
                   true,
                   true);
        ADD_ACTION(ReadReq,
                   kInvalid,
                   OneSharer,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kShared,
                   kModified,
                   false,
                   false,
                   true,
                   true);
        ADD_ACTION(ReadReq,
                   kInvalid,
                   ManySharers,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kShared,
                   kModified,
                   false,
                   false,
                   true,
                   true);

        ADD_ACTION(FetchReq,
                   kModified,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kModified,
                   OneSharer,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kModified,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(FetchReq,
                   kExclusive,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kExclusive,
                   OneSharer,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kExclusive,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kExclusive,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(FetchReq,
                   kShared,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kShared,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(FetchReq,
                   kShared,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   true);

        ADD_ACTION(FetchReq,
                   kInvalid,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   true,
                   true,
                   true);
        ADD_ACTION(FetchReq,
                   kInvalid,
                   OneSharer,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   MissReply,
                   kShared,
                   kModified,
                   false,
                   false,
                   true,
                   true);
        ADD_ACTION(FetchReq,
                   kInvalid,
                   ManySharers,
                   ReturnReq,
                   ReturnReply,
                   ReturnReplyDirty,
                   MissReply,
                   NO_RESP,
                   kShared,
                   kModified,
                   false,
                   false,
                   true,
                   true);

        ADD_POISON_ACTION(UpgradeReq, kModified, ZeroSharers);
        ADD_ACTION(UpgradeReq,
                   kModified,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   UpgradeReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(UpgradeReq,
                   kModified,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   UpgradeReply,
                   UpgradeReply,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_POISON_ACTION(UpgradeReq, kExclusive, ZeroSharers);
        ADD_ACTION(UpgradeReq,
                   kExclusive,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   UpgradeReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(UpgradeReq,
                   kExclusive,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   UpgradeReply,
                   UpgradeReply,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_POISON_ACTION(UpgradeReq, kShared, ZeroSharers);
        ADD_ACTION(UpgradeReq,
                   kShared,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   UpgradeReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(UpgradeReq,
                   kShared,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   UpgradeReply,
                   UpgradeReply,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_POISON_ACTION(UpgradeReq, kInvalid, ZeroSharers);
        ADD_ACTION(UpgradeReq,
                   kInvalid,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   UpgradeReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(UpgradeReq,
                   kInvalid,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   UpgradeReply,
                   UpgradeReply,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(WriteReq,
                   kModified,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyDirty,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kModified,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReplyDirty,
                   MissReplyDirty,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kModified,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReplyDirty,
                   MissReplyDirty,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(WriteReq,
                   kExclusive,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyWritable,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kExclusive,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   MissReplyDirty,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kExclusive,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   MissReplyDirty,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(WriteReq,
                   kShared,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyWritable,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kShared,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   MissReply,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kShared,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(WriteReq,
                   kInvalid,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   MissReplyWritable,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   true,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kInvalid,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   MissReply,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(WriteReq,
                   kInvalid,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   MissReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(NonAllocatingStoreReq,
                   kModified,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kModified,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kModified,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kModified,
                   kModified,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(NonAllocatingStoreReq,
                   kExclusive,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kExclusive,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kExclusive,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kModified,
                   kModified,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(NonAllocatingStoreReq,
                   kShared,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kShared,
                   OneSharer,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kShared,
                   ManySharers,
                   Invalidate,
                   InvalidateAck,
                   InvUpdateAck,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kModified,
                   kModified,
                   true,
                   false,
                   false,
                   true);

        ADD_ACTION(NonAllocatingStoreReq,
                   kInvalid,
                   ZeroSharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   true,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kInvalid,
                   OneSharer,
                   Invalidate,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NonAllocatingStoreReply,
                   kInvalid,
                   kInvalid,
                   false,
                   true,
                   false,
                   true);
        ADD_ACTION(NonAllocatingStoreReq,
                   kInvalid,
                   ManySharers,
                   Invalidate,
                   NoRequest,
                   NoRequest,
                   NonAllocatingStoreReply,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   true,
                   true,
                   false,
                   true);

        ADD_POISON_ACTION(EvictClean, kModified, ZeroSharers);
        ADD_ACTION(EvictClean,
                   kModified,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   false);
        ADD_ACTION(EvictClean,
                   kModified,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   false);

        ADD_POISON_ACTION(EvictClean, kExclusive, ZeroSharers);
        ADD_ACTION(EvictClean,
                   kExclusive,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   false);
        ADD_ACTION(EvictClean,
                   kExclusive,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   false);

        ADD_POISON_ACTION(EvictClean, kShared, ZeroSharers);
        ADD_ACTION(EvictClean,
                   kShared,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   false);
        ADD_ACTION(EvictClean,
                   kShared,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kShared,
                   kShared,
                   false,
                   false,
                   false,
                   false);

        ADD_POISON_ACTION(EvictClean, kInvalid, ZeroSharers);
        ADD_ACTION(EvictClean,
                   kInvalid,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   false);
        ADD_ACTION(EvictClean,
                   kInvalid,
                   ManySharers,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kInvalid,
                   kInvalid,
                   false,
                   false,
                   false,
                   false);

        ADD_POISON_ACTION(EvictWritable, kModified, ZeroSharers);
        ADD_ACTION(EvictWritable,
                   kModified,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   false,
                   false);
        ADD_POISON_ACTION(EvictWritable, kModified, ManySharers);

        ADD_POISON_ACTION(EvictWritable, kExclusive, ZeroSharers);
        ADD_ACTION(EvictWritable,
                   kExclusive,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   false,
                   false);
        ADD_POISON_ACTION(EvictWritable, kExclusive, ManySharers);

        ADD_POISON_ACTION(EvictWritable, kShared, ZeroSharers);
        ADD_POISON_ACTION(EvictWritable, kShared, OneSharer);
        ADD_POISON_ACTION(EvictWritable, kShared, ManySharers);

        ADD_POISON_ACTION(EvictWritable, kInvalid, ZeroSharers);
        ADD_ACTION(EvictWritable,
                   kInvalid,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kExclusive,
                   kExclusive,
                   false,
                   false,
                   true,
                   false);
        ADD_POISON_ACTION(EvictWritable, kInvalid, ManySharers);

        ADD_POISON_ACTION(EvictDirty, kModified, ZeroSharers);
        ADD_POISON_ACTION(EvictDirty, kModified, OneSharer);
        ADD_POISON_ACTION(EvictDirty, kModified, ManySharers);

        ADD_POISON_ACTION(EvictDirty, kExclusive, ZeroSharers);
        // ADD_POISON_ACTION(EvictDirty, kExclusive, OneSharer);
        ADD_ACTION(EvictDirty,
                   kExclusive,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   true,
                   false);
        ADD_POISON_ACTION(EvictDirty, kExclusive, ManySharers);

        ADD_POISON_ACTION(EvictDirty, kShared, ZeroSharers);
        ADD_POISON_ACTION(EvictDirty, kShared, OneSharer);
        ADD_POISON_ACTION(EvictDirty, kShared, ManySharers);

        ADD_POISON_ACTION(EvictDirty, kInvalid, ZeroSharers);
        ADD_ACTION(EvictDirty,
                   kInvalid,
                   OneSharer,
                   NoRequest,
                   NoRequest,
                   NoRequest,
                   NO_RESP,
                   NO_RESP,
                   kModified,
                   kModified,
                   false,
                   false,
                   true,
                   false);
        ADD_POISON_ACTION(EvictDirty, kInvalid, ManySharers);
    }

#undef ADD_PROTOCOL_ACTION

  public:
    SingleCMPProtocol() { initializeProtocol(); };

    virtual const PrimaryAction& getAction(CoherenceState_t c_state,
                                           SharingState d_state,
                                           MemoryMessage::MemoryMessageType type,
                                           PhysicalMemoryAddress address)
    {
        protocol_hash_t::const_iterator iter = theProtocolHash.get<0>().find(std::make_tuple(c_state, d_state, type));
        if (iter == theProtocolHash.end() || iter->action.poison) {
            DBG_Assert(false,
                       (<< "Poison Action! C_State: " << state2String(c_state)
                        << ", D_State: " << SharingStatePrinter(d_state) << ", Request: " << type << ", Address: 0x"
                        << std::hex << (uint64_t)address));
            return poison_action;
        }
        return iter->action;
    };

    virtual MemoryMessage::MemoryMessageType evict(uint64_t tagset, CoherenceState_t state)
    {
        if (state == kModified) {
            return MemoryMessage::EvictDirty;
        } else {
            return MemoryMessage::EvictClean;
        }

        // Don't worry about writable or need to maintain inclusion
    }

    static AbstractProtocol* createInstance(std::list<std::pair<std::string, std::string>>& args)
    {
        std::list<std::pair<std::string, std::string>>::iterator iter = args.begin();
        for (; iter != args.end(); iter++) {
            DBG_Assert(false, (<< "SingleCMPProtocol passed unrecognized parameter '" << iter->first << "'"));
        }
        return new SingleCMPProtocol();
    }

    static const std::string name;
};

REGISTER_PROTOCOL_TYPE(SingleCMPProtocol, "SingleCMP");

}; // namespace nFastCMPCache
