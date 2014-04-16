#ifndef _NS_REGRESS_HPP_
#define _NS_REGRESS_HPP_

#include "netcontainer.hpp"
#include "netcommon.hpp"
#include <stdlib.h>
#include <fstream>

using namespace std;
using namespace nNetShim;

extern nNetShim::NetContainer * nc;

extern bool ( * timeStep ) ( void );
extern int32_t errorCount;
extern int32_t localPendingMsgs;

bool timeStepDefault ( void );
bool waitUntilEmpty ( void );
bool deliverMessage ( const MessageState * msg );
bool isNodeAvailable( const int32_t node, const int32_t vc );
bool dumpStats ( void );
bool dumpCSVStats ( void );
bool runRegressSuite ( void );
bool idleNetwork ( int32_t length );
bool reinitNetwork ( void );
bool singlePacket1 ( int32_t src, int32_t dest );
bool floodNode ( int32_t numNodes, int32_t numMessages, int32_t targetNode );
bool worstCaseLatency ( int32_t srcNode, int32_t destNode );
bool uniformRandomTraffic ( int32_t numNodes, int32_t totalMessages, float intensity );
bool uniformRandomTraffic2 ( int32_t numNodes, int32_t totalMessages, int32_t hopLatency, float intensity );
bool poissonRandomTraffic ( int64_t totalCycles, int32_t numNodes, int32_t hopLatency, float intensity, bool usePriority0 = true );


#define REGRESS_ERROR(MSG) do { cout << "REGRESSION ERROR at cycle " << currTime << "  IN: " << __FUNCTION__ << ", " << __FILE__ << ":" << __LINE__ << ": " << MSG << endl;  return true;} while ( 0 )

#define INSERT(MSG) do { if ( nc->insertMessage ( (MSG) ) ) REGRESS_ERROR ( " inserting message" ); } while ( 0 )

#define TRY_TEST(T)  do { if ( reinitNetwork() ) return true; \
                           cout << "\n*** REGRESSION: "#T << endl;\
                           if ( T ) { cout << "*** TEST "#T << " FAILED" << endl; \
                              errorCount++; }\
                            else { \
                              cout << "*** Test succeeded: "; dumpStats(); dumpCSVStats(); } \
                      } while ( 0 )

#ifdef LOG_RESULTS
extern ofstream outFile;
extern int64_t totalLatency;
extern int64_t messagesDelivered;
#endif

#endif /* _NS_REGRESS_HPP_ */

