#include "regress.hpp"
#include "histogram.h"

#include <fstream>

using namespace nNetShim;
using namespace std;

#ifdef LOG_RESULTS
ofstream outFile;
#endif

#define LATENCY  (32)

int64_t messagesDelivered = 0;
int64_t totalLatency = 0;
int64_t messagesDeliveredPrio[NUM_PRIORITIES] = { 0 };
int64_t totalLatencyPrio[NUM_PRIORITIES] = { 0 };

int64_t bufferTime = 0;
int64_t atHeadTime = 0;
int64_t acceptTime = 0;

int64_t atHeadTimePrio[NUM_PRIORITIES] = { 0 };
int64_t bufferTimePrio[NUM_PRIORITIES] = { 0 };
int64_t totalPopulation = 0;

int64_t valveSwitchTime = 0;
int32_t errorCount = 0;
int32_t localPendingMsgs = 0;

bool ( * timeStep ) ( void ) = timeStepDefault;

bool timeStepDefault ( void ) {
  currTime++;
  return nc->drive();
}

bool deliverMessage ( const MessageState * msg ) {

  messagesDelivered++;
  messagesDeliveredPrio[msg->priority]++;
  totalLatency += ( currTime - msg->startTS );
  totalLatencyPrio[msg->priority] += ( currTime - msg->startTS );

  bufferTime += msg->bufferTime;
  atHeadTime += msg->atHeadTime;
  acceptTime += msg->acceptTime;

  atHeadTimePrio[msg->priority] += msg->atHeadTime;
  bufferTimePrio[msg->priority] += msg->bufferTime;

  return false;
}

bool isNodeAvailable( const int32_t node, const int32_t vc ) {
  if (currTime < valveSwitchTime) {
    return false;
  } else {
    return true;
  }
}

bool dumpStats ( void ) {
  int
  i;

  cout << " total cycles: " << currTime
       << "\n ave latency: " << (float)totalLatency / (float)messagesDelivered;

  for ( i = 0; i < NUM_PRIORITIES; i++ )
    cout << "  P[" << i << "]: " << (float)totalLatencyPrio[i] / (float)messagesDeliveredPrio[i];

  cout << "\n ave pop:     " << (float)totalPopulation / (float)currTime
       << "\n buffer time: " << (float)  bufferTime / (float)messagesDelivered;

  for ( i = 0; i < NUM_PRIORITIES; i++ )
    cout << "  P[" << i << "]: " << (float)  bufferTimePrio[i] / (float)messagesDeliveredPrio[i];

  cout << "\n ave at head: " << (float)  atHeadTime / (float)messagesDelivered;

  for ( i = 0; i < NUM_PRIORITIES; i++ )
    cout << "  P[" << i << "]: " << (float)  atHeadTimePrio[i] / (float)messagesDeliveredPrio[i];

  cout << "\n ave wait:    " << (float)  acceptTime / (float)messagesDelivered
       << "\n Bandwidth:   " << (float)messagesDelivered / (float)currTime
       << endl;
  return false;
}

bool dumpCSVStats ( void ) {
#ifdef LOG_RESULTS

  int
  i;

  outFile << ", " << currTime
          << ", " << (float)totalLatency / (float)messagesDelivered
          << ", " << (float)messagesDelivered / (float)currTime;

  for ( i = 0; i < NUM_PRIORITIES; i++ )
    outFile << ", " << (float)totalLatencyPrio[i] / (float)messagesDeliveredPrio[i];

  outFile << endl;

#endif
  return false;
}

bool waitUntilEmpty ( void ) {
  int
  timeout = 131072,
  active;

  active = nc->getActiveMessages();

  while ( !nc->networkEmpty() || localPendingMsgs ) {
    if ( timeStep() )
      return true;

    timeout--;

    if ( timeout < 0 ) {
      if ( (active == nc->getActiveMessages()) ) {
        nc->dumpState ( cerr, NS_DUMP_METAINFO | NS_DUMP_SWITCHES | NS_DUMP_NODES | NS_DUMP_CHANNELS );
        REGRESS_ERROR ( "timeout when emptying network" );
      } else {
        active = nc->getActiveMessages();
        timeout = 131072;
        cout << "\rDraining, " << active << " messages remain      ";
        cout.flush();
      }
    }

  }

  cout << endl;

  return false;
}

bool reinitNetwork ( void ) {
  int
  i;

  if ( waitUntilEmpty() ) return true;

  if ( idleNetwork ( 1000 ) ) return true;

  resetMessageStateSerial();

  currTime = 0;
  messagesDelivered = 0;
  totalLatency = 0;

  bufferTime = 0;
  atHeadTime = 0;
  acceptTime = 0;
  localPendingMsgs = 0;

  for ( i = 0; i < NUM_PRIORITIES; i++ )
    messagesDeliveredPrio[i] = totalLatencyPrio[i] = atHeadTimePrio[i] = bufferTimePrio[i] = 0;

  return false;
}

bool runRegressSuite ( void ) {
  int32_t
  totalTests = 0,
  i,
  j,
  numNodes;

  float
  f;

  nc->setCallbacks ( deliverMessage, isNodeAvailable );
  numNodes = nc->getNumNodes();

#ifdef LOG_RESULTS
  outFile.open ( "output-regress.csv" );
#endif

  // Basic idle network test
#if 0
  TRY_TEST ( idleNetwork ( 100000 ) );
#endif

  // Basic network connectivity test
#if 0
  for ( i = 0; i < numNodes; i++ ) {
    for ( j = 0; j < numNodes; j++ ) {
      TRY_TEST ( singlePacket1 ( i, j ) );
    }
  }
#endif

  // Basic flood node test
#if 0
  for ( i = 0; i < numNodes; i++ ) {
    // For various numbers of packets
    for ( j = 1; j  < 8192; j = j * 2 ) {
      TRY_TEST ( floodNode ( numNodes, j, i ) );
    }
  }
#endif

#if 0
  for ( f = 0.1; f < 1.5; f = f * 3.0 ) {
    TRY_TEST ( uniformRandomTraffic ( numNodes, (int)1048576, f ) );
  }
#endif

#if 0
  for ( f = 0.05; f <= 1.0; f = f + .05 ) {
    TRY_TEST ( uniformRandomTraffic2 ( numNodes, (int)(1048576 * f), LATENCY, f ) );
  }
#endif

#if 0
  for ( f = 0.71; f < .80; f = f + .01 ) {
    TRY_TEST ( uniformRandomTraffic2 ( numNodes, 1048576, LATENCY, f ) );
  }
#endif

#if 1
  for ( f = 0.1; f <= 1.3; f = f + .05 ) {
    TRY_TEST ( poissonRandomTraffic ( 400000, numNodes, LATENCY, f ) );
  }
#endif

#if 0
  TRY_TEST( worstCaseLatency ( 0, 16 ) );
#endif

  cout << endl;
  cout << "Regression results: failed " << errorCount << " of " << totalTests << " tests." << endl;

#ifdef LOG_RESULTS
  outFile.close();
#endif

  return ( errorCount > 0 );
}

bool idleNetwork ( int32_t length ) {
  int32_t
  i;

  for ( i = 0; i < length; i++ )
    if ( timeStep() ) {
      REGRESS_ERROR ( "Idle network" );
    }

  return false;
}

bool singlePacket1 ( int32_t src, int32_t dest ) {
  MessageState
  * msg = allocMessageState();

  cout << "Single packet from " << src << " to " << dest << endl;

  msg->reinit ( src, dest, 0, LATENCY, false, currTime );

  INSERT ( msg );
  if ( waitUntilEmpty() ) return true;

  return false;
}

bool floodNode ( int32_t numNodes, int32_t numMessages, int32_t targetNode ) {
  MessageState
  * msg;

  int
  i,
  j;

  cout << "Target node: " << targetNode << ", messages sent = "
       << numMessages << " * " << numNodes << " nodes" << endl;

  for ( i = 0; i < numNodes; i++ ) {

    for ( j = 0; j < numMessages; j++ ) {

      msg = allocMessageState();
      msg->reinit ( i, targetNode, 0, LATENCY, false, currTime );

      INSERT ( msg );
    }
  }

  if ( waitUntilEmpty() ) return true;

  return false;
}

bool worstCaseLatency ( int32_t srcNode, int32_t destNode ) {
  MessageState * msg;
  int32_t i;

  cout << "Source node: " << srcNode << " target node: " << destNode << endl;

  // set the time at which the dest node will report 'available'
  valveSwitchTime = 10000;

  for (i = 0; i < 100 && currTime < valveSwitchTime; ) {
    if ( nc->isNodeOutputAvailable(srcNode, 0) ) {
      i++;
      msg = allocMessageState();
      msg->reinit( srcNode, destNode, 0, 1 /* 1 = data packets */ , currTime);
      INSERT( msg );
    }
    if (timeStep()) return true;
  }

  while (!nc->isNodeOutputAvailable(srcNode, 0))
    if (timeStep()) return true;

  msg = allocMessageState();
  msg->reinit( srcNode, destNode, 0, 1, currTime);
  INSERT( msg );

  int32_t drainStart =  currTime;

  if ( waitUntilEmpty() ) return true;

  cout << "INSERTED " << i << " PACKETS" << endl;
  cout << "DRAIN COMPLETED IN " << currTime - drainStart + 1 << " CYCLES" << endl;

  return false;
}

bool uniformRandomTraffic ( int32_t numNodes, int32_t totalMessages, float intensity ) {
  int
  messagesSent,
  messagesThisCycle;

  MessageState
  * msg;

  if ( intensity > 1.0 )
    intensity = 1.0;

  cout << "Randomly sending " << totalMessages << " messages to " << numNodes
       << " nodes with intensity of an average " << intensity
       << " messages per node per cycle " << endl;

  messagesSent = 0;

  while ( messagesSent < totalMessages ) {

    messagesThisCycle = numNodes * ( ( ( (double)random() / RAND_MAX ) <= intensity ) ? random() % numNodes : 0 );

    while ( messagesThisCycle > 0 ) {

      msg = allocMessageState();
      msg->reinit ( random() % numNodes, random() % numNodes, 0 * random() % NUM_PRIORITIES, LATENCY, false, currTime );

      INSERT ( msg );

      messagesSent++;
      messagesThisCycle--;
      if ( ( messagesSent % 1024 ) == 0 ) {
        cout << "\rSent " << messagesSent << " of " << totalMessages << "    ";
        cout.flush();
      }
    }

    if ( timeStep() ) {
      REGRESS_ERROR ( " driving network" );
    }

  }

  cout << endl;

  if ( waitUntilEmpty() ) return true;

  return false;
}

bool uniformRandomTraffic2 ( int32_t numNodes, int32_t totalMessages, int32_t hopLatency, float intensity ) {
  // For a 2D torus with bidirectional links, the number of channels in the
  // bisection = 2*sqrt(numNodes)
  //
  // Each channel can handle 1/hopLatency = B messages per cycle
  //
  // Each node can inject 4 * sqrt ( numNodes ) / ( numNodes * hopLatency ) messages/cycle.
  // Intensity is a fraction of this

  float
  messageIntensity;

  int32_t
  messagesSent = 1,
  priority,
  srcNode,
  i;

  MessageState
  * msg;

  messageIntensity =
    intensity * ( 4.0 * sqrt ( (double)numNodes ) ) /
    (float)( numNodes * hopLatency );

  if ( messageIntensity > 1.0 || messageIntensity <= 0.0 ) {
    REGRESS_ERROR ( "message intensity " << messageIntensity << " is out of whack" );
  }

  cout << "Randomly sending " << totalMessages << " messages to " << numNodes
       << " nodes with intensity of an average " << intensity
       << " fraction of the network throughput" << endl;

  cout << "Messages/cycle " << messageIntensity * (float)numNodes << endl;

  while ( messagesSent < totalMessages ) {

    for ( i = 0; i < numNodes; i++ ) {

      if ( ( (float)random() / (float)RAND_MAX ) < messageIntensity ) {

        srcNode = random() % numNodes;
        priority = random() % NUM_PRIORITIES;
        if ( nc->isNodeOutputAvailable ( srcNode, priority ) ) {
          msg = allocMessageState();
          msg->reinit ( srcNode,
                        random() % numNodes,
                        priority,
                        LATENCY,
                        false,
                        currTime );
          INSERT ( msg );
          messagesSent++;
        }

      }
    }

    if ( ( messagesSent % 131072 ) == 0 ) {
      cout << "\rSent " << messagesSent << " of " << totalMessages << ", "
           << nc->getActiveMessages() << " messages active           ";
      cout.flush();
    }

    if ( timeStep() ) {
      REGRESS_ERROR ( " driving network" );
    }
  }

#ifdef LOG_RESULTS
  outFile << "UR2, " << intensity << "%, "  << messageIntensity * numNodes;
#endif

  cout << endl;

  return waitUntilEmpty();
}

bool poissonRandomTraffic ( int64_t totalCycles, int32_t numNodes, int32_t hopLatency, float intensity, bool usePriority0 ) {
  float
  messageIntensity;

  int32_t
  messagesSent = 1,
  priority,
  srcNode,
  i,
  j;

  MessageState
  * msg;

  int
  * messageDeficit,
  numToSend = 0;

#define POISSON_LIMIT (5)

  float
  prob[POISSON_LIMIT];

  messageIntensity =
    intensity * ( 4.0 * sqrt ( (double)numNodes ) ) /
    (float)( numNodes * hopLatency );

  if ( messageIntensity > 1.0 || messageIntensity <= 0.0 ) {
    REGRESS_ERROR ( "message intensity " << messageIntensity << " is out of whack" );
  }

  cout << "Poisson randomly sending for " << totalCycles << " cycles to " << numNodes
       << " nodes with intensity of an average " << intensity
       << " fraction of the network throughput" << endl;

  cout << "Messages/cycle " << messageIntensity*(float)numNodes << endl;

#ifdef LOG_RESULTS
  outFile << "Poisson, " << intensity << ", "  << messageIntensity * numNodes;
#endif

  float
  f = exp ( -messageIntensity );

  // Calculate the poisson CDF
  prob[0] = f;
  for ( i = 1; i < POISSON_LIMIT; i++ ) {
    f = prob[i] = f * messageIntensity / (float)i;
    prob[i] += prob[i-1];
  }

  messageDeficit = new int[numNodes];
  for ( i = 0; i < numNodes; i++ )
    messageDeficit[i] = 0;

  while ( currTime < totalCycles ) {

    for ( i = 0; i < numNodes; i++ ) {
      // Determine the number of messages to send based upon a poisson distribution
      numToSend = 0;
      f = (float)random() / (float)RAND_MAX;
      for ( j = 0; j < POISSON_LIMIT; j++ ) {
        if ( f >= prob[j] ) {
          numToSend = j + 1;
          break;
        }
      }

      messageDeficit[i] += numToSend;

      while ( messageDeficit[i] > 0 ) {
        srcNode = random() % numNodes;

        if ( usePriority0 )
          priority = random() % NUM_PRIORITIES;
        else
          priority = ( random() % (NUM_PRIORITIES - 1)) + 1;

        if ( nc->isNodeOutputAvailable ( srcNode, priority ) ) {
          msg = allocMessageState();
          msg->reinit ( srcNode,
                        random() % numNodes,
                        priority,
                        LATENCY,
                        false,
                        currTime );
          INSERT ( msg );
          messagesSent++;
          messageDeficit[i]--;
        } else {
          break;
        }
      }
    }

    if ( ( messagesSent % 131072 ) == 0 ) {
      cout << "\rSent " << messagesSent << ", "
           << nc->getActiveMessages() << " messages active, "
           << currTime << " / " << totalCycles << " cycles        ";
      cout.flush();
    }

    if ( timeStep() ) {
      REGRESS_ERROR ( " driving network" );
    }
  }

  cout << endl;

  delete []messageDeficit;

  return false;
}
