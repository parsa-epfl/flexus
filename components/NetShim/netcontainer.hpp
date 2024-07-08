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

#ifndef _NS_NETCONTAINER_HPP_
#define _NS_NETCONTAINER_HPP_

#include "channel.hpp"
#include "netnode.hpp"
#include "netswitch.hpp"

#ifndef NS_STANDALONE
#include <functional>
#endif

namespace nNetShim {
// using namespace nNetwork;

class NetContainer {
public:
  NetContainer(void);

public:
  bool buildMesh();
  bool buildNetwork(const char *filename);

  bool drive(void);

public:
  static bool handleInclude(istream &infile, NetContainer *nc);
  static bool handleChannelLatency(istream &infile, NetContainer *nc);
  static bool handleChannelLatencyData(istream &infile, NetContainer *nc);
  static bool handleChannelLatencyControl(istream &infile, NetContainer *nc);
  static bool handleChannelBandwidth(istream &infile, NetContainer *nc);
  static bool handleLocalChannelLatencyDivider(istream &infile, NetContainer *nc);
  static bool handleNumNodes(istream &infile, NetContainer *nc);
  static bool handleNumSwitches(istream &infile, NetContainer *nc);
  static bool handleSwitchInputBuffers(istream &infile, NetContainer *nc);
  static bool handleSwitchOutputBuffers(istream &infile, NetContainer *nc);
  static bool handleSwitchInternalBuffersPerVC(istream &infile, NetContainer *nc);
  static bool handleSwitchBandwidth(istream &infile, NetContainer *nc);
  static bool handleSwitchPorts(istream &infile, NetContainer *nc);
  static bool handleTopology(istream &infile, NetContainer *nc);
  static bool handleRoute(istream &infile, NetContainer *nc);

  static bool readTopSrcDest(istream &file, bool &isSwitch, int32_t &node, int32_t &sw,
                             int32_t &port, NetContainer *nc);

  static bool readTopologyTripleToken(istream &infile, int32_t &sw, int32_t &port, int32_t &vc,
                                      NetContainer *nc);

  static bool readTopologyDoubleToken(istream &infile, int32_t &tok1, int32_t &tok2,
                                      NetContainer *nc);

  static bool readStringToken(istream &infile, char *str);
  static bool readIntToken(istream &infile, int32_t &i);

  static bool attachSwitchChannels(NetContainer *nc, const int32_t sw, const int32_t port,
                                   const bool destination);

  static bool attachNodeChannels(NetContainer *nc, const int32_t node);

  bool networkEmpty(void) const {
    return (mslHead == nullptr);
  }

  int32_t getActiveMessages(void) const {
    return activeMessages;
  }

  bool deliverMessage(MessageState *msg);

  bool insertMessage(MessageState *msg);

#ifndef NS_STANDALONE
  bool setCallbacks(std::function<bool(const int, const int)> isNodeAvailablePtr_,
                    std::function<bool(const MessageState *)> deliverMessagePtr_) {
    isNodeAvailablePtr = isNodeAvailablePtr_;
    deliverMessagePtr = deliverMessagePtr_;
    return false;
  }

  bool isNodeAvailable(const int32_t node, const int32_t vc) {
    return isNodeAvailablePtr(node, vc);
  }

  bool isNodeOutputAvailable(const int32_t node, const int32_t vc) {
    return nodes[node]->isOutputAvailable(vc);
  }

  int32_t getMaximumInfiniteBufferDepth(void) {
    int i, depth, maxDepth = 0;

    for (i = 0; i < numNodes; i++) {
      depth = nodes[i]->getInfiniteBufferUsed();
      if (depth > maxDepth)
        maxDepth = depth;
    }

    return maxDepth;
  }
#else

  bool setCallbacks(bool (*deliverMessagePtr_)(const MessageState *msg)) {
    deliverMessagePtr = deliverMessagePtr_;
    return false;
  }

  bool setCallbacks(bool (*deliverMessagePtr_)(const MessageState *msg),
                    bool (*isNodeAvailablePtr_)(const int, const int32_t)) {
    deliverMessagePtr = deliverMessagePtr_;
    isNodeAvailablePtr = isNodeAvailablePtr_;
    return false;
  }

  bool isNodeAvailable(const int32_t node, const int32_t vc) {
    return isNodeAvailablePtr(node, vc);
  }

  bool isNodeOutputAvailable(const int32_t node, const int32_t vc) {
    return nodes[node]->isOutputAvailable(vc);
  }
#endif

  int32_t getNumNodes(void) const {
    return numNodes;
  }

  bool dumpState(ostream &out, const int32_t flags);

protected:
  bool readFirstToken(istream &infile);

  bool validateParameters(void);
  bool validateNetwork(void);

  bool allocateNetworkStructures(void);

protected:
  ChannelP *channels;

  NetSwitchP *switches;

  NetNodeP *nodes;

  MessageStateList *mslHead, *mslTail;

  int activeMessages, channelLatency, channelLatencyData, channelLatencyControl, channelBandwidth,
      localChannelLatencyDivider, numNodes, numSwitches, switchInputBuffers, switchOutputBuffers,
      switchInternalBuffersPerVC, switchBandwidth, switchPorts,

      // Internally generated state
      numChannels, maxChannelIndex, openFiles;

#ifndef NS_STANDALONE
  std::function<bool(const int, const int)> isNodeAvailablePtr;
  std::function<bool(const MessageState *)> deliverMessagePtr;
#else
  bool (*deliverMessagePtr)(const MessageState *msg);
  bool (*isNodeAvailablePtr)(const int32_t node, const int32_t vc);
#endif
};

} // namespace nNetShim

#endif /* _NS_NETCONTAINER_HPP_ */
