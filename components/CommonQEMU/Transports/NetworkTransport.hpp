// DO-NOT-REMOVE begin-copyright-block 
//QFlex consists of several software components that are governed by various
//licensing terms, in addition to software that was developed internally.
//Anyone interested in using QFlex needs to fully understand and abide by the
//licenses governing all the software components.
//
//### Software developed externally (not by the QFlex group)
//
//    * [NS-3](https://www.gnu.org/copyleft/gpl.html)
//    * [QEMU](http://wiki.qemu.org/License) 
//    * [SimFlex] (http://parsa.epfl.ch/simflex/)
//
//Software developed internally (by the QFlex group)
//**QFlex License**
//
//QFlex
//Copyright (c) 2016, Parallel Systems Architecture Lab, EPFL
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//    * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//      nor the names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior written
//      permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
//EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// DO-NOT-REMOVE end-copyright-block   
#ifndef FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/CommonQEMU/Slices/NetworkMessage.hpp>
#include <components/CommonQEMU/Slices/ProtocolMessage.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
// Msutherl - RMC Port
#include <components/CommonQEMU/Slices/MRCMessage.hpp>
#include <components/CommonQEMU/Slices/PrefetchCommand.hpp>
#include <components/CommonQEMU/Slices/RMCPacket.hpp>
// END Msutherl - RMC Port

namespace Flexus {
namespace SharedTypes {

#ifndef FLEXUS_TAG_NetworkMessageTag
#define FLEXUS_TAG_NetworkMessageTag
struct NetworkMessageTag_t {};
struct NetworkMessage;
namespace {
NetworkMessageTag_t NetworkMessageTag;
}
#endif //FLEXUS_TAG_NetworkMessageTag

// Msutherl - RMC Port
#ifndef FLEXUS_TAG_PrefetchCommandTag
#define FLEXUS_TAG_PrefetchCommandTag
struct PrefetchCommandTag_t {};
struct PrefetchCommand;
namespace {
PrefetchCommandTag_t PrefetchCommandTag;
}
#endif //FLEXUS_TAG_PrefetchCommandTag

#ifndef FLEXUS_TAG_MRCMessageTag
#define FLEXUS_TAG_MRCMessageTag
struct MRCMessageTag_t {};
struct MRCMessage;
namespace {
MRCMessageTag_t MRCMessageTag;
}
#endif //FLEXUS_TAG_PrefetchCommandTag
// END Msutherl - RMC Port

#ifndef FLEXUS_TAG_ProtocolMessageTag
#define FLEXUS_TAG_ProtocolMessageTag
struct ProtocolMessageTag_t {};
struct ProtocolMessage;
namespace {
ProtocolMessageTag_t ProtocolMessageTag;
}
#endif //FLEXUS_TAG_NetworkMessageTag


#ifndef FLEXUS_TAG_TransactionTrackerTag
#define FLEXUS_TAG_TransactionTrackerTag
struct TransactionTrackerTag_t {};
struct TransactionTracker;
namespace {
TransactionTrackerTag_t TransactionTrackerTag;
}
#endif //FLEXUS_TAG_TransactionTrackerTag

// Msutherl - begin RMC port
#ifndef FLEXUS_TAG_RMCPacketTag
#define FLEXUS_TAG_RMCPacketTag
struct RMCPacketTag_t {};
struct RMCPacket;
namespace {
RMCPacketTag_t RMCPacketTag;
}
#endif //FLEXUS_TAG_RMCPacketTag
// END Msutherl - RMC port


typedef Transport
< mpl::vector
< transport_entry< NetworkMessageTag_t, NetworkMessage >
, transport_entry< ProtocolMessageTag_t, ProtocolMessage >
, transport_entry< TransactionTrackerTag_t, TransactionTracker >
// Msutherl - begin RMC port
, transport_entry< PrefetchCommandTag_t, PrefetchCommand >
, transport_entry< MRCMessageTag_t, MRCMessage >
, transport_entry< RMCPacketTag_t, RMCPacket >
// END Msutherl - RMC port
>
> NetworkTransport;

} //namespace SharedTypes
} //namespace Flexus

#endif //FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED

