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

#ifndef _NS_NETSWITCH_HPP_
#define _NS_NETSWITCH_HPP_

#include "channel.hpp"

namespace nNetShim {

// Forward declaration
class NetSwitch;

class NetSwitchInternalBuffer
{
  public:
    NetSwitchInternalBuffer(const int32_t bufferCount_, NetSwitch* netSwitch_);

  public:
    inline bool isFull(const int32_t vc) const { return (buffersUsed[vc] >= bufferCount[vc]); }

    // Nobody calls this function, so far, so it can be slow
    bool hasMessage(void) const
    {
        int i;

        for (i = 0; i < MAX_VC; i++) {
            if (buffersUsed[i] > 0) { return true; }
        }

        return false;
    }

    bool insertMessage(MessageState* msg);

    bool gotoFirstMessage(const int32_t vc)
    {
        currPriority = vc;
        currMessage  = ageBufferHead[vc];
        return false;
    }

    inline bool getMessage(MessageState*& msg)
    {
        msg = nullptr;

        if (currMessage == nullptr) return true;

        msg = currMessage->msg;
        return false;
    }

    bool nextMessage(void)
    {
        if (currMessage == nullptr) return true;

        currMessage = currMessage->next;

        if (currMessage == nullptr) return true;

        return false;
    }

    bool removeMessage(void);

    bool dumpState(ostream& out);

    void dumpMessageList(ostream& out);

  private:
    int bufferCount[MAX_VC], buffersUsed[MAX_VC];

    MessageStateList *ageBufferHead[MAX_VC], *ageBufferTail[MAX_VC], *currMessage;

    int currPriority;

    NetSwitch* netSwitch;
};

class NetSwitch
{
  public:
    NetSwitch(const int32_t name_,     // Name/id of the switch
              const int32_t numNodes_, // Number of nodes in the system
              const int32_t numPorts_, // Number of ports/bidirectional channels
              const int32_t inputBufferDepth_,
              const int32_t outputBufferDepth_,
              const int32_t vcBufferDepth_,
              const int32_t crossbarBandwidth_,
              const int32_t channelLatency_);
    virtual ~NetSwitch() {}

  public:
    bool drive(void);
    bool driveInputPorts(void);

    bool attachChannel(Channel* channel, const int32_t port, const bool isInput);

    bool checkTopology(void) const;
    bool checkRoutingTable(void) const;

    bool addRoutingEntry(const int32_t destNode, const int32_t outPort, const int32_t outVC);

    bool dumpState(ostream& out, const int32_t flags);

    // These ports give the minimum delay possible - reserved for the local node
    bool setLocalDelayOnly(const int32_t port);

    bool notifyWaitingMessage(const int32_t vc)
    {
        messagesWaiting[vc]++;
        return false;
    }

  protected:
    virtual bool routingPolicy(MessageState* msg);

    bool sendMessageToOutput(MessageState* msg);

  protected:
    int name, numNodes, numPorts, inputBufferDepth, outputBufferDepth, vcBufferDepth, crossbarBandwidth,
      nextStartingPort; // To fairly weight ports, we start arbitration RR

    ChannelInputPortP* inputPorts;
    ChannelOutputPortP* outputPorts;

    NetSwitchInternalBuffer* internalBuffer;

    int **routingTable, **vcTable;

    int messagesWaiting[MAX_VC];
};

typedef NetSwitch* NetSwitchP;

} // namespace nNetShim

#endif /* _NS_NETSWITCH_HPP_ */
