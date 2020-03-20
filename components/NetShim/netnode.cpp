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

#include "netnode.hpp"
#include "netcontainer.hpp"

namespace nNetShim {

NetNode::NetNode(const int32_t nodeId_, NetContainer *nc_)
    : nodeId(nodeId_), nc(nc_), messagesWaiting(0) {
  fromNodePort = new ChannelOutputPort(INT_MAX);
  toNodePort = new ChannelInputPort(1, 1, nullptr, this);
}

bool NetNode::drive(void) {
  MessageState *msg;

  int i;

  if (!messagesWaiting)
    return false;

  for (i = 0; i < MAX_VC; i++) {
    if (toNodePort->hasMessage(i) && nc->isNodeAvailable(nodeId, NETVC_TO_PROTVC(i))) {

      toNodePort->removeMessage(i, msg);

      if (msg->destNode != nodeId) {
        cerr << "Message " << msg->serial << " delivered to wrong node (" << nodeId << ")" << endl;
        return true;
      }

      TRACE(msg, "Node " << nodeId << " is delivering message ");

      if (nc->deliverMessage(msg)) {
        return true;
      }

      messagesWaiting--;

    } else if (toNodePort->hasMessage(i)) {
      toNodePort->peekMessage(i, msg);
      TRACE(msg, "Port unavailable to deliver msg to " << msg->destNode << " on vc " << i);
    }
  }

  return false;
}

bool NetNode::checkTopology(void) const {
  int i;

  for (i = 0; i < MAX_NET_VC; i++) {
    if (!fromNodePort->isConnected() || !toNodePort->isConnected()) {

      std::cerr << "ERROR: node " << nodeId << " is missing channel connections (node unreachable)"
                << endl;

      return true;
    }
  }

  return false;
}

bool NetNode::dumpState(ostream &out) {
  out << "Node: " << nodeId << endl;
  if (fromNodePort->dumpState(out) || toNodePort->dumpState(out))
    return true;

  out << endl;
  return false;
}

} // namespace nNetShim
