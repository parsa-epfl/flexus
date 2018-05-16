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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
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


#include "netnode.hpp"
#include "netcontainer.hpp"

namespace nNetShim {

NetNode::NetNode ( const int32_t      nodeId_,
                   NetContainer * nc_ ) :
  nodeId          ( nodeId_ ),
  nc              ( nc_ ),
  messagesWaiting ( 0 ) {
  fromNodePort = new ChannelOutputPort ( INT_MAX );
  toNodePort   = new ChannelInputPort ( 1, 1, nullptr, this );
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
