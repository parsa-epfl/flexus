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


#include <stdlib.h>
#include <math.h>
#include "regress.hpp"

using namespace nNetShim;
using namespace std;

NetContainer
* nc = nullptr;

int32_t
numNodes = 0,
numMasters = 0;

#define MAX_MASTER_Q (1)
#define MS_DELAY     (1)
#define LATENCY      (32)

int32_t        *        masterListSize;
MessageStateList ** masterReplListTail;
MessageStateList ** masterReplListHead;
MessageState   **   slaveInput;

bool trussIsNodeAvailable ( const int32_t node,
                            const int32_t vc );

bool trussDeliverMessage ( const MessageState * msg );

bool initializeTruss ( void );
bool runTrussSuite ( void );
bool trussTimeStep ( void );

int32_t main ( int32_t argc, char ** argv ) {
  if ( argc != 2 ) {
    cerr << "usage: " << argv[0] << " config_file" << endl;
    return 1;
  }

  cerr << "Creating NetContainer. " << endl;
  nc = new NetContainer();

  cerr << "Building network from " << argv[1] << endl;
  if ( nc->buildNetwork ( argv[1] ) ) {
    cerr << "Error building network. " << endl;
    return 1;
  }

  cerr << "Finished building network" << endl;

  initializeTruss();

  runTrussSuite();

  return 0;
}

inline bool isMaster ( const int32_t node ) {
  return (node < numMasters);
}

bool trussDeliverMessage ( const MessageState * msg ) {
  assert ( trussIsNodeAvailable ( msg->destNode, 0 ) );
  TRACE ( msg, "Delivered message from network" );
  if ( isMaster ( msg->destNode ) ) {

    // Replicate the message and place in the master's input queue
    MessageState
    * newMsg  = allocMessageState();

    MessageStateList
    * newNode = allocMessageStateList ( newMsg );

    deliverMessage ( msg );

    newMsg->reinit ( msg->destNode, // src is now the original destination node
                     msg->destNode + numMasters,
                     0,
                     LATENCY,
                     false,
                     currTime );

    // Replicated the message at this time
    newMsg->replTS = currTime + MS_DELAY;

    TRACE ( newMsg, "Message in master output queue" );

    if ( masterReplListTail[msg->destNode] ) {
      masterReplListTail[msg->destNode]->next = newNode;
      masterReplListTail[msg->destNode] = newNode;
    } else {
      masterReplListHead[msg->destNode] = masterReplListTail[msg->destNode] = newNode;
    }

    masterListSize[msg->destNode]++;
    localPendingMsgs++;

  } else {
    MessageState *
    newMsg = allocMessageState();

    newMsg->reinit ( msg->srcNode,
                     msg->destNode,
                     0,
                     LATENCY,
                     false,
                     msg->startTS );

    newMsg->replTS = msg->replTS;

    slaveInput[msg->destNode-numMasters] = newMsg;
    localPendingMsgs++;

    TRACE ( newMsg, "Message allocated in slave input queue" );
  }

  return false;
}

bool trussIsNodeAvailable ( const int32_t node,
                            const int32_t vc ) {

  if ( isMaster ( node ) ) {

    // The master can buffer a few messages
    return ( masterListSize[node] < MAX_MASTER_Q );

  } else {
    // If the slave has no message waiting in the delay buffer,
    // it's free to receive a message.
    return ( slaveInput[node-numMasters] == nullptr );
  }
}

bool deliverMasterMessages ( void ) {
  int
  i;

  MessageStateList
  * msl;

  for ( i = 0; i < numMasters; i++ ) {

    if ( masterReplListHead[i] &&
         nc->isNodeOutputAvailable ( i, masterReplListHead[i]->msg->priority ) ) {
      assert ( masterListSize > 0 );

      msl = masterReplListHead[i];

      TRACE(msl->msg, "Master sending message on vc: " << msl->msg->priority  );
      INSERT ( msl->msg );

      masterReplListHead[i] = msl->next;

      if ( msl->next == nullptr )
        masterReplListTail[i] = nullptr;

      freeMessageStateList ( msl );
      masterListSize[i]--;
      localPendingMsgs--;
    }
  }

  return false;
}

bool deliverSlaveMessages ( void ) {
  int
  i;

  for ( i = 0; i < numMasters; i++ ) {
    if ( slaveInput[i] &&
         slaveInput[i]->replTS <= currTime ) {

      deliverMessage ( slaveInput[i] );

      TRACE ( slaveInput[i], "Slave finally delivered message: deadline: "
              << slaveInput[i]->replTS << " @ " << currTime );

      freeMessageState ( slaveInput[i] );
      slaveInput[i] = nullptr;
      localPendingMsgs--;
    }
  }

  return false;
}

bool trussTimeStep ( void ) {
  return ( timeStepDefault() ||
           deliverMasterMessages() ||
           deliverSlaveMessages()  );
}

bool initializeTruss ( void ) {
  int
  i;

  cerr << "Initializing TRUSS" << endl;
  timeStep = trussTimeStep;

  nc->setCallbacks ( trussDeliverMessage,
                     trussIsNodeAvailable );

  numNodes   = nc->getNumNodes();
  numMasters = numNodes / 2;

  masterListSize = new int[numMasters];
  masterReplListHead = new MessageStateList*[numMasters];
  masterReplListTail = new MessageStateList*[numMasters];
  slaveInput         = new MessageState*[numMasters];

  for ( i = 0; i < numMasters; i++ ) {
    masterListSize[i] = 0;
    masterReplListHead[i] = masterReplListTail[i] = nullptr;
    slaveInput[i]         = nullptr;
  }

  reinitNetwork();

  cerr << numNodes << " nodes activated.  TRUSS initialized." << endl;

  return false;
}

bool runTrussSuite ( void ) {
  int
  i,
  j;

  float
  f;

#ifdef LOG_RESULTS
  outFile.open ( "output-truss.csv" );
#endif

#if 0
  for ( i = 0; i < numMasters; i++ ) {
    for ( j = 0; j < numMasters; j++ ) {
      TRY_TEST ( singlePacket1 ( i, j ) );
    }
  }
#endif

#if 0
  for ( f = 0.1; f <= 1.0; f = f + 0.05 ) {
    TRY_TEST ( uniformRandomTraffic2 ( numMasters, (int)(104857 * f), LATENCY, f ) );
  }
#endif

#if 1
  for ( f = 0.05; f <= 1.5; f = f + .05 ) {
    TRY_TEST ( poissonRandomTraffic ( 400000, numMasters, LATENCY, f, false ) );
  }
#endif

#ifdef LOG_RESULTS
  outFile.close();
#endif

  return false;
}
