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

#include "channel.hpp"

#include "netnode.hpp"
#include "netswitch.hpp"

namespace nNetShim {

//////////////////////////////////////////////////////////////////////
//

ChannelPort::ChannelPort(const int32_t bufferCount_)
{
    int i;

    for (i = 0; i < MAX_VC; i++) {
        // HACK - Jared will fix this with a more elegant,
        // per-VC solution involving a param file.
        if ((i == 0 || i == 1) && bufferCount_ == INT_MAX) {
            bufferCount[i] = 1;
        } else {
            bufferCount[i] = bufferCount_;
        }
    }

    channel = nullptr;

    assert(bufferCount_ > 0);

    for (i = 0; i < MAX_VC; i++) {
        mslHead[i] = mslTail[i] = nullptr;
        buffersUsed[i]          = 0;
    }
}

bool
ChannelPort::insertMessageHelper(MessageState* msg)
{
    MessageStateList* msl;

    msl = allocMessageStateList(msg);

    assert(msg->networkVC >= 0 && msg->networkVC < MAX_VC);
    if (mslHead[msg->networkVC] == nullptr) {
        mslHead[msg->networkVC] = mslTail[msg->networkVC] = msl;

        // For time at head statistics
        msg->atHeadTime -= currTime;
    } else {
        mslTail[msg->networkVC]->next = msl;
        msl->prev                     = mslTail[msg->networkVC];
        mslTail[msg->networkVC]       = msl;
    }

    return false;
}

bool
ChannelPort::removeMessage(const int32_t vc, MessageState*& msg)
{
    MessageStateList* msl;

    assert(vc >= 0 && vc < MAX_VC);

    msl = mslHead[vc];

    assert(msl != nullptr);

    msg = msl->msg;

    if (msl->next == nullptr)
        mslHead[vc] = mslTail[vc] = nullptr;
    else {
        mslHead[vc] = mslHead[vc]->next;
        mslHead[vc]->msg->atHeadTime -= currTime;
    }

    freeMessageStateList(msl);

    buffersUsed[vc]--;

    msg->atHeadTime += currTime;
    msg->bufferTime += currTime;

    return false;
}

bool
ChannelPort::dumpState(ostream& out)
{
    int i;

    out << " ChannelPort: ";

    for (i = 0; i < MAX_VC; i++) {
        out << " " << buffersUsed[i];

        out << " / " << bufferCount[i] << " buffers used";

        out << endl;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////
//

ChannelInputPort::ChannelInputPort(const int32_t bufferCount_,
                                   const int32_t channelLatency_,
                                   NetSwitch* netSwitch_,
                                   NetNode* netNode_)
  : ChannelPort(bufferCount_)
{
    channelLatency = channelLatency_;
    delayHead = delayTail = nullptr;
    netSwitch             = netSwitch_;
    netNode               = netNode_;
}

bool
ChannelInputPort::insertMessage(MessageState* msg)
{
    MessageStateList* msl;

    assert(buffersUsed[msg->networkVC] < bufferCount[msg->networkVC]);

    buffersUsed[msg->networkVC]++;

    TRACE(msg,
          "InputPort::insertMessage " << msg->serial << " into buffer with " << buffersUsed[msg->networkVC]
                                      << " entries");

    // Queue for a delay
    msl = allocMessageStateList(msg);
    if (msg->flexusInFastMode) {
        msl->delay = currTime + 1;
    } else {
        msl->delay = currTime + channelLatency;
    }

    // Insert into the ordered queue
    if (delayHead == nullptr) {
        delayHead = delayTail = msl;
    } else {
        delayTail->next = msl;
        delayTail       = msl;
    }

    // For time in buffer statistics
    msg->bufferTime -= currTime;

    TRACE(msg,
          "ChannelInputPort received message"
            << " to node " << msg->destNode << " with delay " << msl->delay);

    return false;
}

bool
ChannelInputPort::drive(void)
{
    while (delayHead && delayHead->delay <= currTime) {

        MessageStateList* oldNode = delayHead;

        delayHead = oldNode->next;
        if (delayHead == nullptr) delayTail = nullptr;

        // Notify the switch (if any) that a message is ready
        if (netSwitch != nullptr) { netSwitch->notifyWaitingMessage(oldNode->msg->networkVC); }

        if (netNode != nullptr) { netNode->notifyWaitingMessage(); }

        if (insertMessageHelper(oldNode->msg)) return true;

        freeMessageStateList(oldNode);
    }

    return false;
}

bool
ChannelInputPort::setLocalDelay(void)
{
    channelLatency = 1;
    return false;
}

//////////////////////////////////////////////////////////////////////
//

ChannelOutputPort::ChannelOutputPort(const int32_t bufferCount_)
  : ChannelPort(bufferCount_)
{
}

bool
ChannelOutputPort::insertMessage(MessageState* msg)
{
    assert(hasBufferSpace(msg->networkVC));
    msg->bufferTime -= currTime;

    if (buffersUsed[msg->networkVC] == 0) msg->acceptTime -= currTime;

    buffersUsed[msg->networkVC]++;

    TRACE(msg,
          "OutputPort::insertMessage " << msg->serial << " into buffer with " << buffersUsed[msg->networkVC]
                                       << " entries");

    channel->notifyWaitingMessage();

    TRACE(msg,
          "ChannelOutputPort received message"
            << " to node " << msg->destNode << " with priority " << msg->priority);

    return insertMessageHelper(msg);
}

bool
ChannelOutputPort::drive(void)
{
    // Nothing to do here
    return false;
}

//////////////////////////////////////////////////////////////////////
//

Channel::Channel(const int32_t id_)
{
    id                  = id_;
    busy                = 0;
    localLatencyDivider = 1;
    state               = CS_IDLE;

    fromPort = nullptr;
    toPort   = nullptr;

    messagesWaiting = 0;
}

bool
Channel::drive(void)
{
    int i;

    switch (state) {

        case CS_IDLE: {
            assert(busy <= 0);

            if (messagesWaiting > 0) {
                state = CS_WAIT_FOR_ACCEPT;
                // intentional fall through
            } else {
                break;
            }
        }

        case CS_WAIT_FOR_ACCEPT: {
            // Check the output port for available buffers
            // We give priority to the lowest numbered network
            // virtual channels
            assert(busy <= 0);
            assert(messagesWaiting > 0);

            for (i = 0; i < MAX_VC; i++) {
                MessageState* msg;

                if (fromPort->hasMessage(i) && toPort->hasBufferSpace(i)) {

                    state = CS_TRANSFERRING;

                    // Transfer the message
                    if (fromPort->removeMessage(i, msg)) return true;

                    if (toPort->insertMessage(msg)) return true;

                    // For local node-switch channels, the channel occupancy is shorter
                    // than for switch-switch channels
                    // busy = msg->transmitLatency / localLatencyDivider;
                    // automatically round up to nearest integer
                    busy = (msg->transmitLatency + localLatencyDivider - 1) / localLatencyDivider;
                    if (busy <= 0) busy = 1;
                    TRACE(msg, "Channel setting busy time to: " << busy << " with divider: " << localLatencyDivider);

                    messagesWaiting--;

                    break;
                }
            }

            if (state == CS_TRANSFERRING) {
                // intentional fall through
            } else {
                break;
            }
        }

        case CS_TRANSFERRING: {
            // Count down the transfer timer, if finished, go
            // to idle and allow the next transfer to occur in
            // later cycles
            DBG_(VVerb, (<< "Channel transferring. Busy: " << busy));
            busy--;
            if (busy <= 0) { state = CS_IDLE; }
        } break;

        default: assert(0); break;
    }

    return false;
}

bool
Channel::setFromPort(ChannelOutputPort* port)
{
    if (fromPort) return true;

    fromPort = port;
    fromPort->setChannel(this);
    return false;
}

bool
Channel::setToPort(ChannelInputPort* port)
{
    if (toPort) return true;

    toPort = port;
    toPort->setChannel(this);
    return false;
}

bool
Channel::dumpState(ostream& out)
{
    out << "Channel " << id << " state: " << state << endl;
    return false;
}

} // namespace nNetShim
