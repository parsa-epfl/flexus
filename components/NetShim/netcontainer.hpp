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


#ifndef _NS_NETCONTAINER_HPP_
#define _NS_NETCONTAINER_HPP_

#include "channel.hpp"
#include "netswitch.hpp"
#include "netnode.hpp"

#ifndef NS_STANDALONE
# include <functional>
#endif

namespace nNetShim {
//using namespace nNetwork;

class NetContainer {
public:

  NetContainer ( void );

public:

  bool buildNetwork ( const char * filename );

  bool drive ( void );

public:

  static bool handleInclude ( istream & infile, NetContainer * nc );
  static bool handleChannelLatency ( istream & infile, NetContainer * nc );
  static bool handleChannelLatencyData ( istream & infile, NetContainer * nc );
  static bool handleChannelLatencyControl ( istream & infile, NetContainer * nc );
  static bool handleChannelBandwidth ( istream & infile, NetContainer * nc );
  static bool handleLocalChannelLatencyDivider ( istream & infile, NetContainer * nc );
  static bool handleNumNodes ( istream & infile, NetContainer * nc );
  static bool handleNumSwitches ( istream & infile, NetContainer * nc );
  static bool handleSwitchInputBuffers ( istream & infile, NetContainer * nc );
  static bool handleSwitchOutputBuffers ( istream & infile, NetContainer * nc );
  static bool handleSwitchInternalBuffersPerVC ( istream & infile, NetContainer * nc );
  static bool handleSwitchBandwidth ( istream & infile, NetContainer * nc );
  static bool handleSwitchPorts ( istream & infile, NetContainer * nc );
  static bool handleTopology ( istream & infile, NetContainer * nc );
  static bool handleRoute ( istream & infile, NetContainer * nc );

  static bool readTopSrcDest ( istream & file,
                               bool & isSwitch,
                               int32_t  & node,
                               int32_t  & sw,
                               int32_t  & port,
                               NetContainer * nc );

  static bool readTopologyTripleToken ( istream & infile,
                                        int32_t & sw,
                                        int32_t & port,
                                        int32_t & vc,
                                        NetContainer * nc );

  static bool readTopologyDoubleToken ( istream & infile,
                                        int32_t & tok1,
                                        int32_t & tok2,
                                        NetContainer * nc );

  static bool readStringToken ( istream & infile, char * str );
  static bool readIntToken ( istream & infile, int32_t & i );

  static bool attachSwitchChannels ( NetContainer * nc,
                                     const int32_t sw,
                                     const int32_t port,
                                     const bool destination );

  static bool attachNodeChannels ( NetContainer * nc,
                                   const int32_t node );

  bool networkEmpty ( void ) const {
    return ( mslHead == nullptr );
  }

  int32_t getActiveMessages ( void ) const {
    return activeMessages;
  }

  bool deliverMessage ( MessageState * msg );

  bool insertMessage ( MessageState * msg );

#ifndef NS_STANDALONE
  bool setCallbacks (  std::function<bool(const int, const int)> isNodeAvailablePtr_,
                       std::function<bool(const MessageState *)> deliverMessagePtr_ ) {
    isNodeAvailablePtr = isNodeAvailablePtr_;
    deliverMessagePtr  = deliverMessagePtr_;
    return false;
  }

  bool isNodeAvailable ( const int32_t node,
                         const int32_t vc ) {
    return isNodeAvailablePtr ( node, vc );
  }

  bool isNodeOutputAvailable( const int32_t node,
                              const int32_t vc ) {
    return nodes[node]->isOutputAvailable(vc);
  }

  int32_t getMaximumInfiniteBufferDepth ( void ) {
    int
    i,
    depth,
    maxDepth = 0;

    for ( i = 0; i < numNodes; i++ ) {
      depth = nodes[i]->getInfiniteBufferUsed();
      if ( depth > maxDepth )
        maxDepth = depth;
    }

    return maxDepth;
  }
#else

  bool setCallbacks ( bool ( * deliverMessagePtr_ ) ( const MessageState * msg ) ) {
    deliverMessagePtr = deliverMessagePtr_;
    return false;
  }

  bool setCallbacks ( bool ( * deliverMessagePtr_ ) ( const MessageState * msg ) ,
                      bool ( * isNodeAvailablePtr_ ) ( const int, const int32_t ) ) {
    deliverMessagePtr = deliverMessagePtr_;
    isNodeAvailablePtr = isNodeAvailablePtr_;
    return false;
  }

  bool isNodeAvailable ( const int32_t node,
                         const int32_t vc ) {
    return isNodeAvailablePtr ( node, vc );
  }

  bool isNodeOutputAvailable( const int32_t node,
                              const int32_t vc ) {
    return nodes[node]->isOutputAvailable(vc);
  }
#endif

  int32_t getNumNodes ( void ) const {
    return numNodes;
  }

  bool dumpState ( ostream & out, const int32_t flags );

protected:

  bool readFirstToken ( istream & infile );

  bool validateParameters ( void );
  bool validateNetwork ( void );

  bool allocateNetworkStructures ( void );

protected:

  ChannelP
  * channels;

  NetSwitchP
  * switches;

  NetNodeP
  * nodes;

  MessageStateList
  * mslHead,
  * mslTail;

  int
  activeMessages,
  channelLatency,
  channelLatencyData,
  channelLatencyControl,
  channelBandwidth,
  localChannelLatencyDivider,
  numNodes,
  numSwitches,
  switchInputBuffers,
  switchOutputBuffers,
  switchInternalBuffersPerVC,
  switchBandwidth,
  switchPorts,

  // Internally generated state
  numChannels,
  maxChannelIndex,
  openFiles;

#ifndef NS_STANDALONE
  std::function<bool(const int, const int)> isNodeAvailablePtr;
  std::function<bool(const MessageState *)> deliverMessagePtr;
#else
  bool ( * deliverMessagePtr ) ( const MessageState * msg );
  bool ( * isNodeAvailablePtr ) ( const int32_t node, const int32_t vc );
#endif

};

}

#endif /* _NS_NETCONTAINER_HPP_ */
