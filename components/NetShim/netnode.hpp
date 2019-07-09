// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#ifndef _NS_NETNODE_HPP_
#define _NS_NETNODE_HPP_

#include "channel.hpp"

namespace nNetShim {

class NetContainer;

class NetNode {
public:
  NetNode(const int32_t nodeId_, NetContainer *nc_);

public:
  bool addFromChannel(Channel *fromChan) {
    return fromChan->setFromPort(fromNodePort);
  }

  bool addToChannel(Channel *toChan) {
    return toChan->setToPort(toNodePort);
  }

  bool insertMessage(MessageState *msg) {
    TRACE(msg, "NetNode " << nodeId << " received message "
                          << " to node " << msg->destNode << " with priority " << msg->priority);
    bool ret = fromNodePort->insertMessage(msg);
    TRACE(msg, "Returning from insertMessage");
    return ret;
  }

  bool drive(void);

  bool driveInputPorts(void) {
    return toNodePort->drive();
  }

  bool checkTopology(void) const;

  bool dumpState(ostream &out);

  // We want to closely monitor this statistic, to make sure the
  // buffers don't fill up too much in normal usage.
  //
  // This interface is left as future work, right now I'm going to assume
  // the queues don't get too big.
  //
  int32_t getInfiniteBufferUsed(void) const {
    int i, count = 0;

    for (i = 0; i < MAX_NET_VC; i++)
      count += fromNodePort->getBuffersUsed();

    return count;
  }

  bool isOutputAvailable(const int32_t vc) {
    return fromNodePort->hasBufferSpace(vc);
  }

  bool notifyWaitingMessage(void) {
    messagesWaiting++;
    return false;
  }

protected:
  int32_t nodeId;

  NetContainer *nc;

  ChannelOutputPort *fromNodePort;
  ChannelInputPort *toNodePort;

  int32_t messagesWaiting;
};

typedef NetNode *NetNodeP;
} // namespace nNetShim

#endif /* _NS_NETNODE_HPP_ */
