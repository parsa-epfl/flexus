#include "netswitch.hpp"

#include <core/flexus.hpp>

namespace nNetShim
{

  ostream& operator<<(ostream &os, NetSwitch &ns) {
	ns.dumpState(os, NS_DUMP_SWITCHES_IN | NS_DUMP_SWITCHES_OUT | NS_DUMP_SWITCHES_INTERNAL );
	return os;
  }

  NetSwitch::NetSwitch  ( const int32_t name_,      // Name/id of the switch
                          const int32_t numNodes_,  // Number of nodes in the system
                          const int32_t numPorts_,  // Number of ports/bidirectional channels on the sw
                          const int32_t inputBufferDepth_, 
                          const int32_t outputBufferDepth_,
                          const int32_t vcBufferDepth_,
                          const int32_t crossbarBandwidth_, 
                          const int32_t channelLatency_ ) :
    name              ( name_ ),
    numNodes          ( numNodes_ ),
    numPorts          ( numPorts_ ),
    inputBufferDepth  ( inputBufferDepth_ ),
    outputBufferDepth ( outputBufferDepth_ ),
    vcBufferDepth     ( vcBufferDepth_ ),
    crossbarBandwidth ( crossbarBandwidth_ ),
    nextStartingPort  ( 0 )
  {  
    int
      i,
      j;

    // Check basic assumptions for switch parameters
    assert ( numNodes > 0 );
    assert ( numPorts > 0 );
    assert ( inputBufferDepth > 0 );
    assert ( outputBufferDepth > 0 );
    assert ( vcBufferDepth >= 0 );
    assert ( crossbarBandwidth != 0 );

    // Initialize all buffers and input/output ports
    inputPorts      = new ChannelInputPortP[numPorts];
    outputPorts     = new ChannelOutputPortP[numPorts];
      
    for ( i = 0; i < numPorts; i++ ) {
      inputPorts[i]  = new ChannelInputPort ( inputBufferDepth, channelLatency_, this, nullptr );
      outputPorts[i] = new ChannelOutputPort ( outputBufferDepth );
    }
    
    internalBuffer = new NetSwitchInternalBuffer ( vcBufferDepth,
                                                   this );
  
    // Initialize the routing table to bogus values.  We can't route yet.
    routingTable = new intP[numNodes];
    vcTable      = new intP[numNodes];

    for ( i = 0; i < numNodes; i++ ) {
      routingTable[i] = new int[numPorts*MAX_NET_VC];
      vcTable[i]      = new int[numPorts*MAX_NET_VC];
      for ( j = 0; j < numPorts*MAX_NET_VC; j++ ) {
        vcTable[i][j] = routingTable[i][j] = -1;
      }
    }

    for ( i = 0; i < MAX_VC; i++ ) 
      messagesWaiting[i] = 0;

  } 
  
  bool NetSwitch::attachChannel ( Channel    * channel,
                                  const int32_t    port,
                                  const bool   isInput )
  {
    assert ( channel );
    assert ( port >= 0 && port < numPorts );
  
    if ( isInput ) {
      
      if ( channel->setToPort ( inputPorts[port] ) ) 
        return true;
      
    } else {
      
      if ( channel->setFromPort ( outputPorts[port] ) ) 
        return true;
    }

    return false;
  }

  struct ChannelPortPrinter {
	ChannelPortPrinter(ChannelPort *port, int32_t vc) : my_port(port), my_vc(vc) {}
	ChannelPort *my_port;
	int32_t my_vc;
  };
	ostream& operator<<(ostream& os, const ChannelPortPrinter &printer) {
		printer.my_port->dumpMessageList(os, printer.my_vc);
		return os;
	}

  bool NetSwitch::drive ( void )
  {
    MessageState
      * msg;

    int32_t 
      vc,
      currPort,
      bandwidthRemaining = crossbarBandwidth;

    // For once and for all, here is the prioritized, no inversion, fair 
    // arbitration algorithm:
    //
    // for each vc { 
    //   for messages in internalBuffers ( vc ) { 
    //      AttemptDelivery()
    //   }
    //   for messages in each port ( vc ) { (start port RR each cycle)
    //     AttemptDelivery()
    //   }
    // }
    // 

    for ( vc = 0; vc < MAX_VC; vc++ ) {
      
      // Look at the internal buffers
      internalBuffer->gotoFirstMessage ( vc );
      while ( !internalBuffer->getMessage ( msg ) && bandwidthRemaining > 0 ) {
        
        if ( !routingPolicy ( msg ) ) {

          if ( internalBuffer->removeMessage() || sendMessageToOutput ( msg ) ) {
            return true;
		  }

          bandwidthRemaining--;
        }
        
        // Next message in at this priority level
        if ( internalBuffer->nextMessage() ) break;
      }
      
      // Don't look at an input port if there are no messages waiting
      // on that VC.
      if ( !messagesWaiting[vc] ) {
        continue;
      }

      // Look at the ports 
      currPort = nextStartingPort;
      do {

        if ( !inputPorts[currPort]->peekMessage ( vc, msg ) ) {
          
          // If we can route it to an output port, do so
          if ( !routingPolicy ( msg ) ) { 
            
            if ( inputPorts[currPort]->removeMessage ( vc, msg ) || sendMessageToOutput ( msg ) ) 
              return true;
            
            bandwidthRemaining--;
            messagesWaiting[vc]--;
              
            // Otherwise if we can send it to an internal buffer...
          } else if ( !internalBuffer->isFull ( msg->networkVC ) ) { 
            TRACE ( msg, " SW" << name << " transferring to internal buffer" );
            if ( inputPorts[currPort]->removeMessage ( vc, msg ) || internalBuffer->insertMessage ( msg ) )
              return true;
            messagesWaiting[vc]--;
          }
        }

        currPort = ( currPort + 1 ) % numPorts;
      } while ( currPort != nextStartingPort && bandwidthRemaining );
      
    } // For each vc

    nextStartingPort = ( nextStartingPort + 1 ) % numPorts;

    return false;
  }

  bool NetSwitch::sendMessageToOutput ( MessageState * msg )
  {
    TRACE ( msg, " SW" << name << " routing message to output port " << msg->nextHop << " with priority " << msg->priority );
    
    msg->networkVC = msg->nextVC;
    msg->nextVC    = -1;
    msg->hopCount++;
    
    // Send the message to its output port
    if ( outputPorts[msg->nextHop]->insertMessage ( msg ) ) {
      cerr << "ERROR: sending message from internal buffer to output buffer" << endl;
      return true;
    }

    return false;
  }

  bool NetSwitch::driveInputPorts ( void )
  {
    int32_t 
      i;

    for ( i = 0; i < numPorts; i++ )
      if ( inputPorts[i]->drive() ) return true;

    return false;
  }

  bool NetSwitch::routingPolicy ( MessageState * msg )
  {
    int32_t 
      i,
      routingPort,
      routingVC;

    // Search for an available and acceptable output port, if any  
    for ( i = 0; i < numPorts * MAX_NET_VC; i++ ) {

      if ( routingTable[msg->destNode][i] >= 0 ) {

        routingPort = routingTable[msg->destNode][i];
        routingVC   = vcTable[msg->destNode][i];

        // If there is buffer space, send the message this way
        if ( outputPorts[routingPort]->hasBufferSpace ( BUILD_VC ( msg->priority, routingVC ) ) ) {
          msg->nextHop = routingPort;
          msg->nextVC  = BUILD_VC ( msg->priority, routingVC );
          TRACE ( msg, " SW" << name << " routing destination " << msg->destNode 
                  << " to port " << routingPort << ":" << routingVC << " (priority = " << msg->priority );
          return false;
        }

      } else {
        break;
      }
    }

    // No alternative routes
    return true;
  }
  
  bool NetSwitch::checkTopology ( void ) const 
  {
    int
      i;
    
    bool
      foundErrors = false;

    for ( i = 0; i < numPorts; i++ ) { 
      if ( !inputPorts[i]->isConnected() ||
           !outputPorts[i]->isConnected() ) {
        
        std::cerr << "WARNING: switch " << name 
                  << " port " << i << " left unused (may be safe)" 
                  << endl;
        foundErrors = true;
      }
    }
    
    // We do not report these problems to the caller, since
    // they are generally non-fatal at this point (there may be 
    // no route to them)
    return false;
  }

  bool NetSwitch::checkRoutingTable ( void ) const 
  {
    int32_t 
      i,
      j;

    bool 
      foundErrors = false;
    
    // The most basic test we can make is to see that each
    // node has at least one valid routing entry
    assert ( routingTable );

    for ( i = 0; i < numNodes; i++ ) { 

      if ( routingTable[i][0] == -1 ) { 
        std::cerr << "ERROR: partitioned network possible: Switch " << name
                  << " has no routing table entries for node "
                  << i << endl;
        foundErrors = true;
      }

      for ( j = 0; j < numPorts * MAX_NET_VC; j++ ) { 
        if ( routingTable[i][j] != -1 && 
             !outputPorts[ routingTable[i][j] ]->isConnected() ) { 
          
          // This is a topology problem that actually matters!
          std::cerr << "ERROR: routing table specifies route to node " << i 
                    << " exiting switch " << name << " on unconnected port " 
                    << routingTable[i][j]<< ":" << vcTable[i][j] << endl;
          foundErrors = true;
        }
      }
      
    }
    
    return foundErrors;
  }

  bool NetSwitch::addRoutingEntry ( const int32_t node,
                                    const int32_t port,
                                    const int32_t vc )
  {
    int
      i;

    assert ( node < numNodes && node >= 0 );
    assert ( port < numPorts && port >= 0 );
    assert ( vc   < MAX_NET_VC && vc >= 0 );

    for ( i = 0; i < numPorts*MAX_NET_VC; i++ ) {
      if ( routingTable[node][i] == port && vcTable[node][i] == vc ) { 
        std::cerr << "WARNING: duplicate routing entry for switch " << name
                  << " to node " << node << " through port " << port 
                  << " along virtual channel " << vc << endl;
        return false;
      }
      if ( routingTable[node][i] < 0 ) { 
        routingTable[node][i] = port;
        vcTable[node][i]      = vc;
        return false;
      }
    }

    // If we have not found a place in the routing table by now, there
    // are too many entries
    std::cerr << "ERROR: too many routing table entries in switch " << name 
              << " for destination node " << node
              << " (port " << port << ", vc " << vc << ")" << endl; 

    return true;
  }

  bool NetSwitch::dumpState ( ostream & out, const int32_t flags ) 
  {
    int
      i;

    out << "Switch " << name << endl;
    
    if ( flags & NS_DUMP_SWITCHES_IN ) {
      for ( i = 0; i < numPorts; i++ ) {
        out << " IN (" << i << "): ";
        inputPorts[i]->dumpState ( out );
      }
    }
    
    if ( flags & NS_DUMP_SWITCHES_OUT ) { 
      for ( i = 0; i < numPorts; i++ ) {
        out << " OUT(" << i << "):  ";
        outputPorts[i]->dumpState ( out );
      }
    }
    
    if ( flags & NS_DUMP_SWITCHES_INTERNAL ) {
      internalBuffer->dumpState ( out );
    }

    out << endl;

    return false;
  }

  bool NetSwitch::setLocalDelayOnly ( const int32_t port )
  {
    assert ( port >= 0 && port < numPorts );

    return inputPorts[port]->setLocalDelay();
  }

  bool NetSwitchInternalBuffer::dumpState ( ostream & out )
  {
    int
      i;

    out << " INTERNAL BUFFERS: " << endl;
    for ( i = 0; i < MAX_VC; i++ ) {
      out << "  VC" << i << ": " << buffersUsed[i] << "/" << bufferCount[i] << endl;
    }

    for ( i = 0; i < MAX_VC; i++ ) { 
      if ( ageBufferHead[i] != nullptr ) {
        out << "  Head serial: " << ageBufferHead[i]->msg->serial << endl;
        break;
      }
    } 
    
    return false;
  }

  void NetSwitchInternalBuffer::dumpMessageList ( ostream & out )
  {
    out << " INTERNAL BUFFER[" << currPriority << "]: ";
	MessageStateList *msg = currMessage;
    for ( ; msg != nullptr; msg = msg->next ) {
		out << msg->msg->serial << ", ";
    }
	out << endl;
  }

  /////////////////////////////////////////////////////////////////////////////////
  // Internal Buffer code

  NetSwitchInternalBuffer::NetSwitchInternalBuffer ( const int32_t bufferCount_,
                                                     NetSwitch * netSwitch_ ) :
    currMessage   ( nullptr ),
    currPriority  ( 0 ),
    netSwitch     ( netSwitch_ )
  {
    int
      i;
    
    assert ( bufferCount_ >= 0 );
    assert ( netSwitch != nullptr );
    
    for ( i = 0; i < MAX_VC; i++ ) {
      buffersUsed[i] = 0;
      bufferCount[i] = bufferCount_;
      ageBufferHead[i] = ageBufferTail[i] = nullptr;
    }
  }

  bool NetSwitchInternalBuffer::insertMessage ( MessageState * msg )
  {
    MessageStateList
      * msl;
    
    int
      vc = msg->networkVC;

    assert ( msg );
    assert ( !isFull ( vc ) );

    buffersUsed[vc]++;

    msl = allocMessageStateList ( msg );

    if ( ageBufferTail[vc] == nullptr ) {
      ageBufferHead[vc] = ageBufferTail[vc] = msl;
    } else {
      msl->prev = ageBufferTail[vc];
      ageBufferTail[vc]->next = msl;
      ageBufferTail[vc] = msl;
    }

    // Buffer occupancy statistics
    msg->atHeadTime -= currTime;
    msg->bufferTime -= currTime;

    TRACE ( msg, "NetSwitch received message"
            << " to node " << msg->destNode << " with priority "
            << msg->priority );

    return false;
  }
  
  bool NetSwitchInternalBuffer::removeMessage ( void )
  {
    MessageStateList 
      * msl = currMessage;

    assert ( msl != nullptr );

    buffersUsed[msl->msg->networkVC]--;
    
    // Buffer occupancy statistics
    msl->msg->atHeadTime += currTime;
    msl->msg->bufferTime += currTime;

    if ( msl->next == nullptr ) {
      ageBufferTail[msl->msg->networkVC] = msl->prev;
    } else {
      msl->next->prev = msl->prev;
    }

    if ( msl->prev == nullptr ) {
      ageBufferHead[msl->msg->networkVC] = msl->next;
    } else {
      msl->prev->next = msl->next;
    }

    nextMessage();

    freeMessageStateList ( msl );


    return false;
  }

} // namespace nNetShim
