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

/*
 * Revision History:
 *     twenisch    05 Oct 04 - Initial Revision
 */

#ifndef _MISSTRACKER_HPP
#define _MISSTRACKER_HPP

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
using namespace boost::multi_index;

#define DBG_DeclareCategories CMPCacheMissTracking
#include DBG_Control()

#ifdef FLEXUS_TRACK_COHMISSES
Flexus::Stat::StatInstanceCounter<int64_t> theCoherenceMisses_User("sys-CoherenceMisses:User");
Flexus::Stat::StatInstanceCounter<int64_t> theCoherenceMisses_System("sys-CoherenceMisses:System");
#endif // FLEXUS_TRACK_COHMISSES

namespace nCMPCache {
namespace Stat = Flexus::Stat;
using boost::counted_base;
using boost::intrusive_ptr;
using Flexus::SharedTypes::TransactionTracker;

enum eMissType {
  kStart_Unknown,
  kStart_Coherence,
  kStart_WrongPath,
  kStart_NonCoherence,
  kEnd_Coherence,
  kEnd_NonCoherence,
  kEnd_WrongPath
};

struct MissEvent {
  mutable eMissType theType;
  uint64_t theCycle;
  boost::intrusive_ptr<TransactionTracker> theTracker;
  MissEvent(boost::intrusive_ptr<TransactionTracker> aTracker, eMissType aType)
      : theType(aType), theCycle(Flexus::Core::theFlexus->cycleCount()), theTracker(aTracker) {
  }
};

struct by_cycle;
struct by_miss;
typedef multi_index_container<
    MissEvent,
    indexed_by<
        ordered_non_unique<tag<by_cycle>, member<MissEvent, uint64_t, &MissEvent::theCycle>>,
        ordered_non_unique<tag<by_miss>, member<MissEvent, boost::intrusive_ptr<TransactionTracker>,
                                                &MissEvent::theTracker>>>>
    miss_table_t;

struct MissTracker {
  std::string &theName;

  // Miss Tracking
  miss_table_t theMissTable;
  uint64_t theLastAccounting_Miss;
  uint64_t theLastAccounting_NonCoherence;
  uint64_t theLastAccounting_Coherence;
  uint64_t theLastStart_Miss;
  uint64_t theLastStart_NonCoherence;
  uint64_t theLastStart_Coherence;
  uint32_t theCount_Miss;
  uint32_t theCount_NonCoherence;
  uint32_t theCount_Coherence;

  int64_t theLastCoherenceAddress;

  // Stats
  Stat::StatCounter theMisses;
  Stat::StatCounter theMisses_U;
  Stat::StatCounter theMisses_S;
  Stat::StatCounter theMisses_U_NonCoherence;
  Stat::StatCounter theMisses_U_Coherence;
  Stat::StatCounter theMisses_U_SpecAtomicCoherence;
  Stat::StatCounter theMisses_U_CohContend;
  Stat::StatCounter theMisses_U_WrongPath;
  Stat::StatCounter theMisses_S_NonCoherence;
  Stat::StatCounter theMisses_S_Coherence;
  Stat::StatCounter theMisses_S_SpecAtomicCoherence;
  Stat::StatCounter theMisses_S_CohContend;
  Stat::StatCounter theMisses_S_WrongPath;

  Stat::StatLog2Histogram theLatencies_Miss;
  Stat::StatCounter theLatenciesSum_Miss;
  Stat::StatLog2Histogram theLatencies_NonCoherence;
  Stat::StatCounter theLatenciesSum_NonCoherence;
  Stat::StatLog2Histogram theLatencies_Coherence;
  Stat::StatCounter theLatenciesSum_Coherence;

  Stat::StatLog2Histogram theInterarrivals_Miss;
  Stat::StatCounter theInterarrivalsSum_Miss;
  Stat::StatLog2Histogram theInterarrivals_NonCoherence;
  Stat::StatCounter theInterarrivalsSum_NonCoherence;
  Stat::StatLog2Histogram theInterarrivals_Coherence;
  Stat::StatCounter theInterarrivalsSum_Coherence;

  Stat::StatInstanceCounter<int64_t> theCyclesWithN_Miss;
  Stat::StatInstanceCounter<int64_t> theCyclesWithN_NonCoherence;
  Stat::StatInstanceCounter<int64_t> theCyclesWithN_Coherence;

  MissTracker(std::string &aName)
      : theName(aName)
        // Counters
        ,
        theLastAccounting_Miss(0), theLastAccounting_NonCoherence(0),
        theLastAccounting_Coherence(0), theLastStart_Miss(0), theLastStart_NonCoherence(0),
        theLastStart_Coherence(0), theCount_Miss(0), theCount_NonCoherence(0),
        theCount_Coherence(0), theLastCoherenceAddress(0)
        // Stats
        ,
        theMisses(theName + "-Track:Miss"), theMisses_U(theName + "-Track:Miss:U"),
        theMisses_S(theName + "-Track:Miss:S")

        ,
        theMisses_U_NonCoherence(theName + "-Track:Miss:U:NonCoherence"),
        theMisses_U_Coherence(theName + "-Track:Miss:U:Coherence"),
        theMisses_U_SpecAtomicCoherence(theName + "-Track:Miss:U:SpecAtomicCoh"),
        theMisses_U_CohContend(theName + "-Track:Miss:U:CohContend"),
        theMisses_U_WrongPath(theName + "-Track:Miss:U:WrongPath"),
        theMisses_S_NonCoherence(theName + "-Track:Miss:S:NonCoherence"),
        theMisses_S_Coherence(theName + "-Track:Miss:S:Coherence"),
        theMisses_S_SpecAtomicCoherence(theName + "-Track:Miss:S:SpecAtomicCoh"),
        theMisses_S_CohContend(theName + "-Track:Miss:S:CohContend"),
        theMisses_S_WrongPath(theName + "-Track:Miss:S:WrongPath")

        ,
        theLatencies_Miss(theName + "-Track:Latency"),
        theLatenciesSum_Miss(theName + "-Track:LatencySum"),
        theLatencies_NonCoherence(theName + "-Track:Latency:NonCoherence"),
        theLatenciesSum_NonCoherence(theName + "-Track:LatencySum:NonCoherence"),
        theLatencies_Coherence(theName + "-Track:Latency:Coherence"),
        theLatenciesSum_Coherence(theName + "-Track:LatencySum:Coherence")

        ,
        theInterarrivals_Miss(theName + "-Track:Interarrivals"),
        theInterarrivalsSum_Miss(theName + "-Track:InterarrivalsSum"),
        theInterarrivals_NonCoherence(theName + "-Track:Interarrivals:NonCoherence"),
        theInterarrivalsSum_NonCoherence(theName + "-Track:InterarrivalsSum:NonCoherence"),
        theInterarrivals_Coherence(theName + "-Track:Interarrivals:Coherence"),
        theInterarrivalsSum_Coherence(theName + "-Track:InterarrivalsSum:Coherence")

        ,
        theCyclesWithN_Miss(theName + "-Track:CyclesWithN"),
        theCyclesWithN_NonCoherence(theName + "-Track:CyclesWithN:NonCoherence"),
        theCyclesWithN_Coherence(theName + "-Track:CyclesWithN:Coherence") {
  }

  void startMiss(boost::intrusive_ptr<TransactionTracker> tracker) {
    theMissTable.insert(MissEvent(tracker, kStart_Unknown));
  }

  void processTable() {
    FLEXUS_PROFILE();
    while (!theMissTable.empty()) {
      bool erased = false;
      miss_table_t::iterator front = theMissTable.begin();
      switch (front->theType) {
      case kStart_Unknown:
        return; // Can't process this entry yet

      case kStart_NonCoherence:
        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theCyclesWithN_NonCoherence << std::make_pair(
            theCount_NonCoherence, front->theCycle - theLastAccounting_NonCoherence);
        theLastAccounting_Miss = front->theCycle;
        theLastAccounting_NonCoherence = front->theCycle;

        ++theMisses;
        if (front->theTracker->OS() && *front->theTracker->OS()) {
          ++theMisses_S;
          ++theMisses_S_NonCoherence;
        } else {
          ++theMisses_U;
          ++theMisses_U_NonCoherence;
        }
        ++theCount_Miss;
        ++theCount_NonCoherence;

        theInterarrivals_Miss << (front->theCycle - theLastStart_Miss);
        theInterarrivalsSum_Miss += (front->theCycle - theLastStart_Miss);
        theInterarrivals_NonCoherence << (front->theCycle - theLastStart_NonCoherence);
        theInterarrivalsSum_NonCoherence += (front->theCycle - theLastStart_NonCoherence);

        theLastStart_Miss = front->theCycle;
        theLastStart_NonCoherence = front->theCycle;
        break;

      case kStart_Coherence:
        // Determine if this coherence miss is to the same address as the
        // most recent.
        if (front->theTracker->address()) {
          int64_t address = *front->theTracker->address() & (~63LL);
          if (address == theLastCoherenceAddress) {
            // This miss is to the same address as the most recent, thus
            // we count it as a contended coherence miss.

            ++theMisses;
            if (front->theTracker->OS() && *front->theTracker->OS()) {
              ++theMisses_S;
              ++theMisses_S_CohContend;
              DBG_(Trace,
                   AddCat(CMPCacheMissTracking)(<< theName << " CONTENTION Miss to " << std::hex
                                                << address << std::dec << "[S]"));
            } else {
              ++theMisses_U;
              ++theMisses_U_CohContend;
              DBG_(Trace,
                   AddCat(CMPCacheMissTracking)(<< theName << " CONTENTION Miss to " << std::hex
                                                << address << std::dec << "[U]"));
            }

            // Erase all markers for this miss
            theMissTable.get<by_miss>().erase(front->theTracker);
            erased = true;

            // Do not treat this as a coherence miss
            break;
          }

          theLastCoherenceAddress = address;
        } else {
          theLastCoherenceAddress = 0;
        }

        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theCyclesWithN_Coherence << std::make_pair(theCount_Coherence,
                                                   front->theCycle - theLastAccounting_Coherence);
        theLastAccounting_Miss = front->theCycle;
        theLastAccounting_Coherence = front->theCycle;

        ++theMisses;

        if (front->theTracker->speculativeAtomicLoad() &&
            *front->theTracker->speculativeAtomicLoad()) {
          if (front->theTracker->OS() && *front->theTracker->OS()) {
            ++theMisses_S;
            ++theMisses_S_SpecAtomicCoherence;
            DBG_(Trace, AddCat(CMPCacheMissTracking)(
                            << theName << " SPEC-ATOMIC-COHERENCE Miss to " << std::hex
                            << ((*front->theTracker->address() & (~63LL))) << std::dec << "[S]"));
          } else {
            ++theMisses_U;
            ++theMisses_U_SpecAtomicCoherence;
            DBG_(Trace, AddCat(CMPCacheMissTracking)(
                            << theName << " SPEC-ATOMIC-COHERENCE Miss to " << std::hex
                            << ((*front->theTracker->address() & (~63LL))) << std::dec << "[U]"));
          }
        } else {
          if (front->theTracker->OS() && *front->theTracker->OS()) {
            ++theMisses_S;
            ++theMisses_S_Coherence;
#ifdef FLEXUS_TRACK_COHMISSES
            theCoherenceMisses_System
                << std::make_pair(*front->theTracker->address() & (~63LL), 1LL);
#endif // FLEXUS_TRACK_COHMISSES
            DBG_(Trace, AddCat(CMPCacheMissTracking)(<< theName << " COHERENCE Miss to " << std::hex
                                                     << ((*front->theTracker->address() & (~63LL)))
                                                     << std::dec << "[S]"));
          } else {
            ++theMisses_U;
            ++theMisses_U_Coherence;
#ifdef FLEXUS_TRACK_COHMISSES
            theCoherenceMisses_User << std::make_pair(*front->theTracker->address() & (~63LL), 1LL);
#endif // FLEXUS_TRACK_COHMISSES
            DBG_(Trace, AddCat(CMPCacheMissTracking)(<< theName << " COHERENCE Miss to " << std::hex
                                                     << ((*front->theTracker->address() & (~63LL)))
                                                     << std::dec << "[U]"));
          }
        }

        ++theCount_Miss;
        ++theCount_Coherence;

        theInterarrivals_Miss << (front->theCycle - theLastStart_Miss);
        theInterarrivalsSum_Miss += (front->theCycle - theLastStart_Miss);
        theInterarrivals_Coherence << (front->theCycle - theLastStart_Coherence);
        theInterarrivalsSum_Coherence += (front->theCycle - theLastStart_Coherence);

        theLastStart_Miss = front->theCycle;
        theLastStart_Coherence = front->theCycle;
        break;

      case kStart_WrongPath:
        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theLastAccounting_Miss = front->theCycle;

        ++theMisses;
        if (front->theTracker->OS() && *front->theTracker->OS()) {
          ++theMisses_S;
          ++theMisses_S_WrongPath;
        } else {
          ++theMisses_U;
          ++theMisses_U_WrongPath;
        }

        ++theCount_Miss;

        theInterarrivals_Miss << (front->theCycle - theLastStart_Miss);
        theInterarrivalsSum_Miss += (front->theCycle - theLastStart_Miss);

        theLastStart_Miss = front->theCycle;
        break;

      case kEnd_NonCoherence:
        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theCyclesWithN_NonCoherence << std::make_pair(
            theCount_NonCoherence, front->theCycle - theLastAccounting_NonCoherence);
        theLastAccounting_Miss = front->theCycle;
        theLastAccounting_NonCoherence = front->theCycle;

        --theCount_Miss;
        --theCount_NonCoherence;

        theLatencies_Miss << (front->theCycle - front->theTracker->startCycle());
        theLatenciesSum_Miss += (front->theCycle - front->theTracker->startCycle());
        theLatencies_NonCoherence << (front->theCycle - front->theTracker->startCycle());
        theLatenciesSum_NonCoherence += (front->theCycle - front->theTracker->startCycle());
        break;

      case kEnd_Coherence:
        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theCyclesWithN_Coherence << std::make_pair(theCount_Coherence,
                                                   front->theCycle - theLastAccounting_Coherence);
        theLastAccounting_Miss = front->theCycle;
        theLastAccounting_Coherence = front->theCycle;

        --theCount_Miss;
        --theCount_Coherence;

        theLatencies_Miss << (front->theCycle - front->theTracker->startCycle());
        theLatenciesSum_Miss += (front->theCycle - front->theTracker->startCycle());
        theLatencies_Coherence << (front->theCycle - front->theTracker->startCycle());
        theLatenciesSum_Coherence += (front->theCycle - front->theTracker->startCycle());
        break;

      case kEnd_WrongPath:
        theCyclesWithN_Miss << std::make_pair(theCount_Miss,
                                              front->theCycle - theLastAccounting_Miss);
        theLastAccounting_Miss = front->theCycle;

        --theCount_Miss;

        theLatencies_Miss << (front->theCycle - front->theTracker->startCycle());
        theLatenciesSum_Miss += (front->theCycle - front->theTracker->startCycle());
        break;
      }

      if (!erased) {
        theMissTable.erase(front);
      }
    }
  }

  void finishMiss(boost::intrusive_ptr<TransactionTracker> tracker) {
    FLEXUS_PROFILE();

    // See if this is a miss
    if (tracker) {
      miss_table_t::index<by_miss>::type::iterator iter = theMissTable.get<by_miss>().find(tracker);
      if (iter != theMissTable.get<by_miss>().end()) {
        // We found the start event for this coherence miss
        DBG_Assert(iter->theType == kStart_Unknown);
        if (tracker->wrongPath() && *tracker->wrongPath()) {
          iter->theType = kStart_WrongPath;
          theMissTable.insert(MissEvent(tracker, kEnd_WrongPath));
        } else if (tracker->fillType() && *tracker->fillType() == Flexus::SharedTypes::eCoherence) {
          iter->theType = kStart_Coherence;
          theMissTable.insert(MissEvent(tracker, kEnd_Coherence));
        } else {
          iter->theType = kStart_NonCoherence;
          theMissTable.insert(MissEvent(tracker, kEnd_NonCoherence));
        }
      }
    }
    processTable();
  }
};

} // end namespace nCMPCache

#endif // _MISSTRACKER_HPP
