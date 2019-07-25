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

#include "netcommon.hpp"
#include <string.h>

namespace nNetShim {

/***************************************************
 * GLOBAL VARIABLE
 ***************************************************/

int64_t currTime;

MessageStateList *mslFreeList = nullptr;
MessageStateList *msFreeList = nullptr;

#define ALLOCATION_BLOCK_SIZE (512)

int32_t msSerial = 0;

MessageStateList *allocMessageStateList(void) {
  MessageStateList *newNode;

  if (mslFreeList == nullptr) {
    int i;

    // Allocate more nodes
    mslFreeList = new MessageStateList[ALLOCATION_BLOCK_SIZE];

    assert(mslFreeList != nullptr);

    // Set up next pointers
    for (i = 0; i < ALLOCATION_BLOCK_SIZE - 1; i++)
      mslFreeList[i].next = &mslFreeList[i + 1];
  }

  newNode = mslFreeList;
  mslFreeList = newNode->next;

  newNode->prev = nullptr;
  newNode->next = nullptr;
  newNode->delay = 0;

#ifdef NS_DEBUG
  assert(newNode->usageCount == 0);
  newNode->usageCount++;
#endif

  return newNode;
}

MessageStateList *allocMessageStateList(MessageState *msg) {
  MessageStateList *newNode = allocMessageStateList();

  newNode->msg = msg;
  return newNode;
}

bool freeMessageStateList(MessageStateList *msgl) {
#ifdef NS_DEBUG
  msgl->usageCount--;
  assert(msgl->usageCount == 0);
#endif

  //  msgl->msg = (MessageState *)msgl->msg->serial;
  //  msgl->msg   = nullptr;
  msgl->prev = nullptr;
  msgl->next = mslFreeList;
  mslFreeList = msgl;

  return false;
}

MessageState *allocMessageState(void) {
  MessageState *newState;

  MessageStateList *msl;

  if (msFreeList == nullptr) {
    int32_t i;

    newState = new MessageState[ALLOCATION_BLOCK_SIZE];
    assert(newState != nullptr);

    // TODO: Value initialization is done in the constructor
    // memset(newState, 0, sizeof(MessageState[ALLOCATION_BLOCK_SIZE]));

    for (i = 0; i < ALLOCATION_BLOCK_SIZE; i++)
      freeMessageState(&newState[i]);
  }

  assert(msFreeList != nullptr);

  msl = msFreeList;
  msFreeList = msl->next;

  newState = msl->msg;

  freeMessageStateList(msl);

  newState->reinit();
  newState->serial = msSerial++;

  TRACE(newState, " allocating message with new serial " << newState->serial);

  return newState;
}

bool freeMessageState(MessageState *msg) {
  MessageStateList *newNode;

  TRACE(msg, " deallocating message");

  msg->myList = nullptr;
  msg->serial = -msg->serial;

  newNode = allocMessageStateList(msg);
  newNode->next = msFreeList;
  msFreeList = newNode;

  return false;
}

bool resetMessageStateSerial(void) {
  msSerial = 0;
  return false;
}

} // namespace nNetShim
