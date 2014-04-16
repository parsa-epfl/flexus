#include "netcommon.hpp"
#include <string.h>

namespace nNetShim
{
  
  /***************************************************
   * GLOBAL VARIABLE 
   ***************************************************/
  
  int64_t currTime;
  
MessageStateList * mslFreeList = NULL;
MessageStateList * msFreeList  = NULL;

#define ALLOCATION_BLOCK_SIZE  (512)

int32_t  msSerial = 0;

MessageStateList * allocMessageStateList ( void )
{
  MessageStateList 
    * newNode;

  if ( mslFreeList == NULL ) {
    int
      i;
    
    // Allocate more nodes 
    mslFreeList = new MessageStateList[ALLOCATION_BLOCK_SIZE];
    
    assert ( mslFreeList != NULL );

    // Set up next pointers
    for ( i = 0; i < ALLOCATION_BLOCK_SIZE - 1; i++ )
      mslFreeList[i].next = &mslFreeList[i+1];
  }

  newNode = mslFreeList;
  mslFreeList = newNode->next;

  newNode->prev = NULL;
  newNode->next = NULL;
  newNode->delay = 0;

#ifdef NS_DEBUG
  assert ( newNode->usageCount == 0 );
  newNode->usageCount++;
#endif

  return newNode;  
}

MessageStateList * allocMessageStateList ( MessageState * msg )
{
  MessageStateList 
    * newNode = allocMessageStateList();
  
  newNode->msg = msg;
  return newNode;
}

bool freeMessageStateList ( MessageStateList * msgl )
{
#ifdef NS_DEBUG
  msgl->usageCount--;
  assert ( msgl->usageCount == 0 );
#endif

  msgl->msg = (MessageState*)msgl->msg->serial;
  //  msgl->msg   = NULL;
  msgl->prev  = NULL;
  msgl->next  = mslFreeList;
  mslFreeList = msgl;

  return false;
}

MessageState * allocMessageState ( void )
{
  MessageState
    * newState;

  MessageStateList
    * msl;

  if ( msFreeList == NULL ) {
    int32_t 
      i;
    
    newState = new MessageState[ALLOCATION_BLOCK_SIZE];
    assert ( newState != NULL );

    memset ( newState, 0, sizeof(MessageState[ALLOCATION_BLOCK_SIZE]) );

    for ( i = 0; i < ALLOCATION_BLOCK_SIZE; i++ )
      freeMessageState ( &newState[i] );
  }

  assert ( msFreeList != NULL );

  msl = msFreeList;
  msFreeList = msl->next;

  newState = msl->msg;

  freeMessageStateList ( msl );

  newState->reinit();
  newState->serial = msSerial++;

  TRACE ( newState, " allocating message with new serial " << newState->serial );
  
  return newState;
}

bool freeMessageState  ( MessageState * msg )
{
  MessageStateList 
    * newNode;

  TRACE ( msg, " deallocating message" );

  msg->myList = NULL;
  msg->serial = -msg->serial;

  newNode       = allocMessageStateList ( msg );
  newNode->next = msFreeList;
  msFreeList    = newNode;

  return false;
}

bool resetMessageStateSerial ( void )
{
  msSerial = 0;
  return false;
}



} // nNetShim
