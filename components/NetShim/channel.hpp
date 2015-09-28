#ifndef _NS_CHANNEL_HPP_
#define _NS_CHANNEL_HPP_

#include "netcommon.hpp"

namespace nNetShim {

enum ChannelState {
  CS_IDLE,
  CS_WAIT_FOR_ACCEPT,
  CS_TRANSFERRING
};

class Channel;
class NetSwitch;
class NetNode;

//////////////////////////////////////////////////////////////////////
//

class ChannelPort {
public:
  virtual ~ChannelPort ( ) {}
  ChannelPort ( const int32_t bufferCount_ );

public:

  virtual bool drive ( void ) = 0;

  virtual bool insertMessage ( MessageState  * msg ) = 0;

  virtual bool removeMessage ( const int32_t       vc,
                               MessageState *& msg );

  inline bool peekMessage ( const int32_t       vc,
                            MessageState *& msg ) {
    assert(vc >= 0 && vc < MAX_VC);
    if ( mslHead[vc] ) {
      msg = mslHead[vc]->msg;
      return false;
    }

    msg = nullptr;
    return true;
  }

  bool dumpState ( ostream & out );
  bool dumpMessageList ( ostream & out, int32_t vc ) const {

    out << "VC " << vc << ": ";
    MessageStateList * msl = mslHead[vc];
    for (; msl != nullptr; msl = msl->next ) {
      out << msl->msg->serial << ", ";
    }

    return false;
  }

  // Note that for messages in a delay buffer, the count
  // may be non-zero, but the message is not yet available.
  inline bool hasMessage ( const int32_t vc ) {
    assert(vc >= 0 && vc < MAX_VC);
    return ( mslHead[vc] != nullptr );
  }

  inline bool hasBufferSpace ( const int32_t vc ) {
    assert(vc >= 0 && vc < MAX_VC);
    return ( buffersUsed[vc] < bufferCount[vc] );
  }

  int32_t getBuffersUsed ( void ) {
    int
    i,
    count = 0;

    for ( i = 0; i < MAX_VC; i++ )
      count += buffersUsed[i];

    return count;
  }

  bool setChannel ( Channel * channel_ ) {
    channel = channel_;
    return false;
  }

  bool isConnected ( void ) {
    return ( channel != nullptr );
  }

protected:

  bool insertMessageHelper ( MessageState * msg );

protected:

  int32_t bufferCount[MAX_VC];
  int32_t buffersUsed[MAX_VC];

  MessageStateList * mslHead[MAX_VC];
  MessageStateList * mslTail[MAX_VC];

  Channel * channel;

};

//////////////////////////////////////////////////////////////////////
//

class ChannelInputPort : public ChannelPort {
public:
  ChannelInputPort ( const int32_t bufferCount_,
                     const int32_t channelLatency_,
                     NetSwitch * netSwitch_,
                     NetNode  *  netNode_  );
public:
  virtual ~ChannelInputPort() {}

  virtual bool drive();

  virtual bool insertMessage ( MessageState * msg );

  virtual bool removeMessage ( const int32_t       vc,
                               MessageState *& msg ) {
    if ( ChannelPort::removeMessage ( vc, msg ) )
      return true;
    msg->bufferTime -= MIN ( msg->transmitLatency, channelLatency );
    return false;
  }

  // Configure this channel to be a "local" high-bandwidth/low-latency
  // channel between the local node and the switch.
  // This sets the latency to be low.
  bool setLocalDelay ( void );

  bool updateAtHeadStatistics ( void );

protected:

  int32_t channelLatency;

  MessageStateList * delayHead,
                   * delayTail;

  NetSwitch     *    netSwitch;
  NetNode      *     netNode;

};

//////////////////////////////////////////////////////////////////////
//

class ChannelOutputPort : public ChannelPort {
public:
  ChannelOutputPort ( const int32_t bufferCount_ );

public:

  virtual bool drive();

  virtual bool insertMessage ( MessageState * msg );

  virtual bool removeMessage ( const int32_t       vc,
                               MessageState *& msg ) {
    if ( ChannelPort::removeMessage ( vc, msg ) )
      return true;
    msg->acceptTime += currTime;
    if ( mslHead[vc] != nullptr ) {
      mslHead[vc]->msg->acceptTime -= currTime;
    }
    return false;
  }

protected:

};

class Channel {
public:
  Channel ( const int32_t id_ );
  virtual ~Channel ( ) {}

public:

  bool drive ( void );

  bool setFromPort ( ChannelOutputPort * port );

  bool setToPort ( ChannelInputPort * port );

  bool dumpState ( ostream & out );

  bool notifyWaitingMessage ( void ) {
    messagesWaiting++;
    return false;
  }

  void setLocalLatencyDivider ( const int32_t latency_ ) {
    localLatencyDivider = latency_;
  }

protected:

  int32_t id;
  int32_t busy;
  int32_t localLatencyDivider;
  ChannelState state;

  ChannelOutputPort * fromPort;
  ChannelInputPort  * toPort;

  int32_t messagesWaiting;

};

typedef ChannelInputPort  * ChannelInputPortP;
typedef ChannelOutputPort * ChannelOutputPortP;
typedef ChannelPort    *    ChannelPortP;
typedef Channel      *      ChannelP;

} // namespace nNetShim

#endif /* _NS_CHANNEL_HPP_ */
