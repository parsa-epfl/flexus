//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#ifndef _NS_REGRESS_HPP_
#define _NS_REGRESS_HPP_

#include "netcommon.hpp"
#include "netcontainer.hpp"
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace nNetShim;

extern nNetShim::NetContainer *nc;

extern bool (*timeStep)(void);
extern int32_t errorCount;
extern int32_t localPendingMsgs;

bool timeStepDefault(void);
bool waitUntilEmpty(void);
bool deliverMessage(const MessageState *msg);
bool isNodeAvailable(const int32_t node, const int32_t vc);
bool dumpStats(void);
bool dumpCSVStats(void);
bool runRegressSuite(void);
bool idleNetwork(int32_t length);
bool reinitNetwork(void);
bool singlePacket1(int32_t src, int32_t dest);
bool floodNode(int32_t numNodes, int32_t numMessages, int32_t targetNode);
bool worstCaseLatency(int32_t srcNode, int32_t destNode);
bool uniformRandomTraffic(int32_t numNodes, int32_t totalMessages, float intensity);
bool uniformRandomTraffic2(int32_t numNodes, int32_t totalMessages, int32_t hopLatency,
                           float intensity);
bool poissonRandomTraffic(int64_t totalCycles, int32_t numNodes, int32_t hopLatency,
                          float intensity, bool usePriority0 = true);

#define REGRESS_ERROR(MSG)                                                                         \
  do {                                                                                             \
    cout << "REGRESSION ERROR at cycle " << currTime << "  IN: " << __FUNCTION__ << ", "           \
         << __FILE__ << ":" << __LINE__ << ": " << MSG << endl;                                    \
    return true;                                                                                   \
  } while (0)

#define INSERT(MSG)                                                                                \
  do {                                                                                             \
    if (nc->insertMessage((MSG)))                                                                  \
      REGRESS_ERROR(" inserting message");                                                         \
  } while (0)

#define TRY_TEST(T)                                                                                \
  do {                                                                                             \
    if (reinitNetwork())                                                                           \
      return true;                                                                                 \
    cout << "\n*** REGRESSION: " #T << endl;                                                       \
    if (T) {                                                                                       \
      cout << "*** TEST " #T << " FAILED" << endl;                                                 \
      errorCount++;                                                                                \
    } else {                                                                                       \
      cout << "*** Test succeeded: ";                                                              \
      dumpStats();                                                                                 \
      dumpCSVStats();                                                                              \
    }                                                                                              \
  } while (0)

#ifdef LOG_RESULTS
extern ofstream outFile;
extern int64_t totalLatency;
extern int64_t messagesDelivered;
#endif

#endif /* _NS_REGRESS_HPP_ */
