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

#ifndef _NS_NETNODE_HPP_
#define _NS_NETNODE_HPP_

#include "channel.hpp"

namespace nNetShim {

class NetContainer;

class NetNode
{
  public:
    NetNode(const int32_t nodeId_, NetContainer* nc_);

  public:
    bool addFromChannel(Channel* fromChan) { return fromChan->setFromPort(fromNodePort); }

    bool addToChannel(Channel* toChan) { return toChan->setToPort(toNodePort); }

    bool insertMessage(MessageState* msg)
    {
        TRACE(msg,
              "NetNode " << nodeId << " received message "
                         << " to node " << msg->destNode << " with priority " << msg->priority);
        bool ret = fromNodePort->insertMessage(msg);
        TRACE(msg, "Returning from insertMessage");
        return ret;
    }

    bool drive(void);

    bool driveInputPorts(void) { return toNodePort->drive(); }

    bool checkTopology(void) const;

    bool dumpState(ostream& out);

    // We want to closely monitor this statistic, to make sure the
    // buffers don't fill up too much in normal usage.
    //
    // This interface is left as future work, right now I'm going to assume
    // the queues don't get too big.
    //
    int32_t getInfiniteBufferUsed(void) const
    {
        int i, count = 0;

        for (i = 0; i < MAX_NET_VC; i++)
            count += fromNodePort->getBuffersUsed();

        return count;
    }

    bool isOutputAvailable(const int32_t vc) { return fromNodePort->hasBufferSpace(vc); }

    bool notifyWaitingMessage(void)
    {
        messagesWaiting++;
        return false;
    }

  protected:
    int32_t nodeId;

    NetContainer* nc;

    ChannelOutputPort* fromNodePort;
    ChannelInputPort* toNodePort;

    int32_t messagesWaiting;
};

typedef NetNode* NetNodeP;
} // namespace nNetShim

#endif /* _NS_NETNODE_HPP_ */
