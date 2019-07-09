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

#ifndef _NS_NETCOMMON_HPP_
#define _NS_NETCOMMON_HPP_

#include <iostream>
#include <limits.h>
#include <stdio.h>

#ifdef NS_STANDALONE

#include <assert.h>

#else

#undef assert
#define assert DBG_Assert

#include <core/debug/debug.hpp>

#endif

using namespace std;

namespace nNetShim {

typedef int32_t *intP;

#define NS_DUMP_METAINFO 0x01
#define NS_DUMP_NODES 0x02
#define NS_DUMP_CHANNELS 0x04

#define NS_DUMP_SWITCHES_IN 0x10
#define NS_DUMP_SWITCHES_OUT 0x20
#define NS_DUMP_SWITCHES_INTERNAL 0x40
#define NS_DUMP_SWITCHES 0x70

#define MAX_NET_VC (2)

// Maximum number of priorities
#define NUM_PRIORITIES (4)

#define MAX_PROT_VC (4)

#define PROTVC_TO_NETVC(VC) ((VC)*MAX_NET_VC)
#define NETVC_TO_PROTVC(VC) ((VC) / MAX_NET_VC)
#define MAX_VC (MAX_PROT_VC * MAX_NET_VC)
#define BUILD_VC(PROT, NET) ((PROT)*MAX_NET_VC + (NET))

#ifndef MIN
#define MIN(A, B) (((A) > (B)) ? B : A)
#endif

/***************************************************
 * GLOBAL VARIABLE
 ***************************************************/
extern int64_t currTime;

class MessageStateList;

class MessageState {

public:
  MessageState(void)
      : srcNode(-1), destNode(-1), priority(-1), networkVC(-1), nextHop(-1), nextVC(-1),
        transmitLatency(0), flexusInFastMode(false), hopCount(-1), bufferTime(0), atHeadTime(0),
        acceptTime(0), startTS(0),
  // CMU-ONLY-BLOCK-BEGIN
#ifdef NS_STANDALONE
        replTS(0),
#endif
        // CMU-ONLY-BLOCK-END
        myList(nullptr) {
  }

  MessageState(int32_t srcNode_, int32_t destNode_, int32_t priority_, int32_t transmitLatency_,
               int32_t flexusInFastMode_)
      : srcNode(srcNode_), destNode(destNode_), priority(priority_), networkVC(0), nextHop(-1),
        nextVC(-1), transmitLatency(transmitLatency_), flexusInFastMode(flexusInFastMode_),
        hopCount(-1), bufferTime(0), atHeadTime(0), acceptTime(0), startTS(0),
  // CMU-ONLY-BLOCK-BEGIN
#ifdef NS_STANDALONE
        replTS(0),
#endif
        // CMU-ONLY-BLOCK-END
        myList(nullptr) {
  }

public:
  void reinit(int32_t srcNode_ = 0, int32_t destNode_ = 0, int32_t priority_ = 0,
              int32_t transmitLatency_ = 0, bool flexusInFastMode_ = false, int64_t startTS_ = 0) {
    srcNode = srcNode_;
    destNode = destNode_;
    priority = priority_;
    networkVC = 0;
    nextHop = -1;
    nextVC = -1;

    bufferTime = 0;
    atHeadTime = 0;
    acceptTime = 0;
    transmitLatency = transmitLatency_;
    myList = nullptr;
    hopCount = -1;
    startTS = startTS_;

    // CMU-ONLY-BLOCK-BEGIN
#ifdef NS_STANDALONE
    replTS = 0;
#endif
    // CMU-ONLY-BLOCK-END
    flexusInFastMode = flexusInFastMode_;
  }

public:
  int32_t srcNode;
  int32_t destNode;
  int32_t priority;
  int32_t networkVC;
  int32_t nextHop; // Output port for the next hop
  int32_t nextVC;
  int32_t transmitLatency;
  bool flexusInFastMode;
  int32_t serial;

  // Statistics follow below...
  int32_t hopCount; /* Number of hops that the message has taken */

  int64_t bufferTime;

  int64_t atHeadTime; /* Number of cycles that the message has been at the
                       * head of a queue.  This can tell us about arbitration
                       * contention inside a switch.
                       */
  int64_t acceptTime; /* Number of cycles that the message has been at the
                       * head of an output queue.  This can tell us if we have
                       * channel contention/hot spot channels.
                       */
  int64_t startTS;    /* When did this message enter the network? */
  // CMU-ONLY-BLOCK-BEGIN
#ifdef NS_STANDALONE
  int64_t replTS;
#endif
  // CMU-ONLY-BLOCK-END

  /* Note that we calculate the total queuing time statistic with the following
   * formula:
   *
   *   Total transit time - (hopCount * channelLatency)
   *
   *  This works because if we are not on a link, we are sitting in a buffer.
   */

  MessageStateList *myList;
};

// A linked list for messages, to keep FIFO queues, free lists, etc...
class MessageStateList {

public:
  MessageStateList(void) : msg(nullptr), prev(nullptr), next(nullptr), delay(0) {
#ifdef NS_DEBUG
    usageCount = 0;
#endif
  }

public:
  MessageState *msg;
  MessageStateList *prev;
  MessageStateList *next;
  int64_t delay;

#ifdef NS_DEBUG
  int32_t usageCount;
#endif
};

//  Message and message list allocation/deallocation functions.
//  We want to allocate/free real memory as little as possible
MessageStateList *allocMessageStateList(void);
MessageStateList *allocMessageStateList(MessageState *msg);
bool freeMessageStateList(MessageStateList *msgl);

MessageState *allocMessageState(void);
bool freeMessageState(MessageState *msg);
bool resetMessageStateSerial(void);

#ifdef NS_DEBUG

#ifdef NS_STANDALONE
#define TRACE(M, TXT)                                                                              \
  { cerr << "Time: " << currTime << " Msg: " << (M)->serial << " " << TXT << endl; }
#else
#define TRACE(M, TXT)                                                                              \
  { DBG_(Iface, (<< "Time: " << currTime << " Msg: " << (M)->serial << " " << TXT << endl)); }
#endif

extern int64_t currTime;

#else

#define TRACE(M, TXT)

#endif

} // namespace nNetShim

#endif /* _NS_NETCOMMON_HPP_ */
