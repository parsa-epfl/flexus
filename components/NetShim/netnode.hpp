#ifndef _NS_NETNODE_HPP_
#define _NS_NETNODE_HPP_

#include "channel.hpp"

namespace nNetShim {

class NetContainer;

class NetNode {
public:

  NetNode ( const int32_t      nodeId_,
            NetContainer * nc_);

public:

  bool addFromChannel ( Channel * fromChan ) {
    return fromChan->setFromPort ( fromNodePort );
  }

  bool addToChannel   ( Channel * toChan ) {
    return toChan->setToPort ( toNodePort );
  }

  bool insertMessage ( MessageState * msg ) {
    TRACE ( msg, "NetNode " << nodeId << " received message "
            << " to node " << msg->destNode << " with priority "
            << msg->priority );
    bool ret = fromNodePort->insertMessage ( msg );
    TRACE (msg, "Returning from insertMessage");
    return ret;
  }

  bool drive ( void );

  bool driveInputPorts ( void ) {
    return toNodePort->drive();
  }

  bool checkTopology ( void ) const;

  bool dumpState ( ostream & out );

  // We want to closely monitor this statistic, to make sure the
  // buffers don't fill up too much in normal usage.
  //
  // This interface is left as future work, right now I'm going to assume
  // the queues don't get too big.
  //
  int32_t getInfiniteBufferUsed ( void ) const {
    int
    i,
    count = 0;

    for ( i = 0; i < MAX_NET_VC; i++ )
      count += fromNodePort->getBuffersUsed();

    return count;
  }

  bool isOutputAvailable ( const int32_t vc ) {
    return fromNodePort->hasBufferSpace(vc);
  }

  bool notifyWaitingMessage ( void ) {
    messagesWaiting++;
    return false;
  }

protected:

  int32_t nodeId;

  NetContainer * nc;

  ChannelOutputPort * fromNodePort;
  ChannelInputPort  * toNodePort;

  int32_t messagesWaiting;
};

typedef NetNode * NetNodeP;
}

#endif /* _NS_NETNODE_HPP_ */
