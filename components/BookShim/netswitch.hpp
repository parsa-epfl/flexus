#ifndef _NS_NETSWITCH_HPP_
#define _NS_NETSWITCH_HPP_

#include "channel.hpp"

namespace nNetShim
{

  // Forward declaration
class NetSwitch;

class NetSwitchInternalBuffer
{
public:

  NetSwitchInternalBuffer ( const int32_t   bufferCount_,
                            NetSwitch * netSwitch_ );

public:

  inline bool isFull ( const int32_t vc ) const
  {
    return ( buffersUsed[vc] >= bufferCount[vc] );
  }

  // Nobody calls this function, so far, so it can be slow
  bool hasMessage ( void ) const
  {
    int
      i;

    for ( i = 0; i < MAX_VC; i++ ) {
      if ( buffersUsed[i] > 0 ) {
        return true;
      }
    }

    return false;
  }

  bool insertMessage ( MessageState * msg );

  bool gotoFirstMessage ( const int32_t vc )
  {
    currPriority = vc;
    currMessage = ageBufferHead[vc];
    return false;
  }

  inline bool getMessage ( MessageState *& msg )
  {
    msg = nullptr;

    if ( currMessage == nullptr )
      return true;

    msg = currMessage->msg;
    return false;
  }

  bool nextMessage ( void )
  {
    if ( currMessage == nullptr )
      return true;

    currMessage = currMessage->next;

    if ( currMessage == nullptr )
      return true;

    return false;
  }

  bool removeMessage ( void );

  bool dumpState ( ostream & out );

  void dumpMessageList(ostream &out);

private:

  int
    bufferCount[MAX_VC],
    buffersUsed[MAX_VC];

  MessageStateList
    * ageBufferHead[MAX_VC],
    * ageBufferTail[MAX_VC],
    * currMessage;

  int
    currPriority;

  NetSwitch
    * netSwitch;
};


class NetSwitch
{
public:

  NetSwitch ( const int32_t name_,      // Name/id of the switch
              const int32_t numNodes_,  // Number of nodes in the system
              const int32_t numPorts_,  // Number of ports/bidirectional channels
              const int32_t inputBufferDepth_,
              const int32_t outputBufferDepth_,
              const int32_t vcBufferDepth_,
              const int32_t crossbarBandwidth_,
              const int32_t channelLatency_ );
  virtual ~NetSwitch () {}

public:

  bool drive ( void );
  bool driveInputPorts ( void );

  bool attachChannel ( Channel   * channel,
                       const int32_t   port,
                       const bool  isInput );

  bool checkTopology ( void ) const;
  bool checkRoutingTable ( void ) const;

  bool addRoutingEntry ( const int32_t destNode,
                         const int32_t outPort,
                         const int32_t outVC );

  bool dumpState ( ostream & out, const int32_t flags );

  // These ports give the minimum delay possible - reserved for the local node
  bool setLocalDelayOnly ( const int32_t port );

  bool notifyWaitingMessage ( const int32_t vc )
  {
    messagesWaiting[vc]++;
    return false;
  }

protected:

  virtual bool routingPolicy ( MessageState * msg );

  bool sendMessageToOutput ( MessageState * msg );

protected:

  int
  name,
    numNodes,
    numPorts,
    inputBufferDepth,
    outputBufferDepth,
    vcBufferDepth,
    crossbarBandwidth,
    nextStartingPort;  // To fairly weight ports, we start arbitration RR

  ChannelInputPortP * inputPorts;
  ChannelOutputPortP * outputPorts;

  NetSwitchInternalBuffer
    * internalBuffer;

  int
    ** routingTable,
    ** vcTable;

  int
    messagesWaiting[MAX_VC];

};

typedef NetSwitch * NetSwitchP;

} // namespace nNetShim

#endif /* _NS_NETSWITCH_HPP_ */
