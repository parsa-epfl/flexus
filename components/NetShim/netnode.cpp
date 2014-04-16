#include "netnode.hpp"
#include "netcontainer.hpp"

namespace nNetShim {

NetNode::NetNode ( const int32_t      nodeId_,
                   NetContainer * nc_ ) :
  nodeId          ( nodeId_ ),
  nc              ( nc_ ),
  messagesWaiting ( 0 ) {
  fromNodePort = new ChannelOutputPort ( INT_MAX );
  toNodePort   = new ChannelInputPort ( 1, 1, NULL, this );
}

bool NetNode::drive ( void ) {
  MessageState
  * msg;

  int
  i;

  if ( !messagesWaiting )
    return false;

  for ( i = 0; i < MAX_VC; i++ ) {
    if ( toNodePort->hasMessage ( i ) && nc->isNodeAvailable ( nodeId, NETVC_TO_PROTVC(i) ) ) {

      toNodePort->removeMessage ( i, msg );

      if ( msg->destNode != nodeId ) {
        cerr << "Message " << msg->serial << " delivered to wrong node ("
             << nodeId << ")" << endl;
        return true;
      }

      TRACE ( msg, "Node " << nodeId << " is delivering message " );

      if ( nc->deliverMessage ( msg ) ) {
        return true;
      }

      messagesWaiting--;

    } else if (toNodePort->hasMessage(i)) {
      toNodePort->peekMessage(i, msg);
      TRACE ( msg, "Port unavailable to deliver msg to " << msg->destNode << " on vc " << i );
    }
  }

  return false;
}

bool NetNode::checkTopology ( void ) const {
  int
  i;

  for ( i = 0; i < MAX_NET_VC; i++ ) {
    if ( !fromNodePort->isConnected() ||
         !toNodePort->isConnected() ) {

      std::cerr << "ERROR: node " << nodeId
                << " is missing channel connections (node unreachable)"
                << endl;

      return true;
    }
  }

  return false;
}

bool NetNode::dumpState ( ostream & out ) {
  out << "Node: " << nodeId << endl;
  if ( fromNodePort->dumpState ( out ) ||
       toNodePort->dumpState ( out ) )
    return true;

  out << endl;
  return false;
}

}
