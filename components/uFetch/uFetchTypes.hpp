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

#ifndef FLEXUS_uFETCH_TYPES_HPP_INCLUDED
#define FLEXUS_uFETCH_TYPES_HPP_INCLUDED

#include <iostream>
#include <list>

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/qemu/mai_api.hpp>

namespace Flexus {
namespace SharedTypes {

struct Translation; // fwd declare
typedef boost::intrusive_ptr<Translation> TranslationPtr;

using boost::counted_base;
using Flexus::SharedTypes::TransactionTracker;
using Flexus::SharedTypes::Translation;
using Flexus::SharedTypes::VirtualMemoryAddress;

/* Added for boomerang
 * FIXME: Remove these exception sources when QFlex actually handles exceptions. */
typedef std::pair<Flexus::SharedTypes::VirtualMemoryAddress,
                  Flexus::SharedTypes::VirtualMemoryAddress>
    vaddr_pair;
enum xExceptionSource {
  xUnknown = 1,
  xSaveTrap = 2,
  xRestoreTrap = 3,
  xIMMU = 4,
  xFlushw = 5,
  xTcc = 6,
  xResyn = 7,
  xTrans = 8,
  xIntr = 9
};
std::ostream &operator<<(std::ostream &anOstream, xExceptionSource aSource);
enum eBranchResolution { NotYetResolved, Misprediction, CorrectPrediction };

enum eBranchType {
  kNonBranch,
  kConditional,
  kUnconditional,
  kCall,
  kIndirectReg,
  kIndirectCall,
  kReturn,
  kLastBranchType
};
std::ostream &operator<<(std::ostream &anOstream, eBranchType aType);

enum eDirection {
  kStronglyTaken,
  kTaken // Bimodal
  ,
  kNotTaken // gShare
  ,
  kStronglyNotTaken
};
std::ostream &operator<<(std::ostream &anOstream, eDirection aDir);

struct BPMessage : boost::counted_base {
  VirtualMemoryAddress pc;
  uint32_t opcode;
  uint64_t timeStamp;
};

struct TrapState : boost::counted_base {
  uint32_t theTL;
  uint64_t theTPC[5];
  uint64_t theTNPC[5];
};

struct BTBEntry {
  VirtualMemoryAddress thePC;
  mutable eBranchType theBranchType;
  mutable VirtualMemoryAddress theTarget;
  mutable int theBBsize; // Size of the basic block, used only in basic block based BTB
  mutable eDirection theBranchDirection;
  mutable bool isSpecialCall;
  BTBEntry(VirtualMemoryAddress aPC, eBranchType aType, VirtualMemoryAddress aTarget,
           int BBSize = 0, eDirection branchDirection = kNotTaken, bool specialCall = false)
      : thePC(aPC), theBranchType(aType), theTarget(aTarget), theBBsize(BBSize),
        theBranchDirection(branchDirection), isSpecialCall(specialCall) {
  }

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &thePC;
    ar &theBranchType;
    ar &theTarget;
    ar &theBBsize;
    ar &theBranchDirection;
    ar &isSpecialCall;
  }
  BTBEntry() {
  }
};

struct BPredState : boost::counted_base {
  eBranchType thePredictedType;
  VirtualMemoryAddress thePredictedTarget;
  VirtualMemoryAddress theNextPredictedTarget;
  eDirection thePrediction;
  eDirection theBimodalPrediction;
  eDirection theMetaPrediction;
  eDirection theGSharePrediction;
  eDirection theActualDirection;
  eBranchType theActualType;
  VirtualMemoryAddress pc;
  uint32_t theGShareShiftReg;
  uint32_t theSerial;

  // stuff for tage
  int bank;
  bool pred_taken;
  bool alttaken;
  int BI;
  int GI[15]; // 15 is random, upper bound on #tables?

  unsigned ch_i[15];
  unsigned ch_t[2][15];

  int altbank;
  int PWIN;

  int phist;
  std::bitset<131> ghist; // Fixme: replace 131 with a correct macro

  bool caused_ICache_miss;
  VirtualMemoryAddress ICache_miss_address;
  uint32_t last_miss_distance;

  bool is_runahead;       // 1: if it is prediction from runahead path
  bool bimodalPrediction; // Is the final prediction from bimoal (in case of Tage)
  bool returnUsedRAS;     // Did the return instruction used RAS to get the return address
  bool returnPopRASTwice;
  bool callUpdatedRAS;
  bool detectedSpecialCall;
  bool haltDispatch;
  bool translationFailed;
  bool BTBPreFilled;
  bool causedSquash;
  bool hasRetired;
  xExceptionSource exceptionSource;
  int8_t saturationCounter; // What is the counter value
  uint32_t theTL;
  uint32_t theBBSize;
};
std::ostream &operator<<(std::ostream &anOstream, const BPredState &aBPState);

struct FetchAddr {
  Flexus::SharedTypes::VirtualMemoryAddress theAddress;
  boost::intrusive_ptr<BPredState> theBPState;
  uint64_t theSerial;
  uint64_t PAQArrival_cycle;
  int redirectionCause;
  bool wasRedirected;
  bool theFAQFull;
  FetchAddr(Flexus::SharedTypes::VirtualMemoryAddress anAddress)
      : theAddress(anAddress), theSerial((uint64_t)0xdeadbeef), PAQArrival_cycle(0),
        redirectionCause(0), wasRedirected(false), theFAQFull(false) {
  }
  FetchAddr(Flexus::SharedTypes::VirtualMemoryAddress anAddress, uint64_t &aSerial)
      : theAddress(anAddress), theSerial(aSerial++), PAQArrival_cycle(0), redirectionCause(0),
        wasRedirected(false), theFAQFull(false) {
  }
};

struct PrefetchStatus {
  Flexus::SharedTypes::VirtualMemoryAddress theAddress;
  Flexus::SharedTypes::PhysicalMemoryAddress thePAddress;
  uint64_t PBQArrival_cycle;
  uint64_t frontEndResetDistance;
  bool prefetchIssued;
  bool blockCached;
  bool blockEvicted;
  bool translationFailed;
  PrefetchStatus(Flexus::SharedTypes::VirtualMemoryAddress anAddress, uint64_t PBQarrival_cycle)
      : theAddress(anAddress), thePAddress(0), PBQArrival_cycle(PBQarrival_cycle),
        frontEndResetDistance(0), prefetchIssued(false), blockCached(false), blockEvicted(false),
        translationFailed(false) {
  }
};

struct InfoMissStats : boost::counted_base {
  int64_t IcacheMissCycles;
  uint64_t BTBMissCycles;
  uint32_t lastResetDistance;
  uint32_t PBQWaitTime;
  /*eSquashCause*/ int lastSquashCause;
  tFillLevel FillLevel;
  boost::intrusive_ptr<BPredState> lastSquashedBPState;
  boost::intrusive_ptr<BPredState> bpState;
  bool blockEvicted;
  bool prefetchIssued;
  bool blockCached;
  bool translationFailed;
  bool IcacheMiss;
  bool missPrefetchOnWay;
  bool wasRedirected;
  bool theFAQFull;
  bool isVisibleBTBMiss;

  InfoMissStats(int64_t _IcacheMissCycles, uint32_t _lastResetDistance,
                /*eSquashCause*/ int _lastSquashCause,
                boost::intrusive_ptr<BPredState> _lastSquashedBPState,
                boost::intrusive_ptr<BPredState> _currentBPState, bool _IcacheMiss,
                bool _missPrefetchOnWay, bool _wasRedirected, bool _theFAQFull)
      : IcacheMissCycles(_IcacheMissCycles), BTBMissCycles(0),
        lastResetDistance(_lastResetDistance), PBQWaitTime(0), lastSquashCause(_lastSquashCause),
        FillLevel(eUnknown), lastSquashedBPState(_lastSquashedBPState), bpState(_currentBPState),
        blockEvicted(false), prefetchIssued(false), blockCached(false), translationFailed(false),
        IcacheMiss(_IcacheMiss), missPrefetchOnWay(_missPrefetchOnWay),
        wasRedirected(_wasRedirected), theFAQFull(_theFAQFull), isVisibleBTBMiss(false) {
  }
};

struct RetireNotice {
  int theTL;
  uint64_t thePC;
  uint64_t theFetchSerial;
  boost::intrusive_ptr<InfoMissStats> missStatsInfo;
  /*bool theHadIcacheMiss;
  bool theMissPrefetchOnWay;
  int64_t theIcacheMissCycles;*/
  RetireNotice(int aTL,
               uint64_t aPC, uint64_t theFetchSerial, boost::intrusive_ptr<InfoMissStats> _missStatsInfo /*bool hadIcacheMiss, bool missPrefetchOnWay, int64_t theIcacheMissCycles*/)
      : theTL(aTL), thePC(aPC), theFetchSerial(theFetchSerial), missStatsInfo(_missStatsInfo)
  /*, theHadIcacheMiss(hadIcacheMiss)
  , theMissPrefetchOnWay(missPrefetchOnWay)
  , theIcacheMissCycles(theIcacheMissCycles)*/
  {
  }
};

struct FetchCommand : boost::counted_base {
  std::list<FetchAddr> theFetches;
};

struct RecordedMisses : boost::counted_base {
  std::list<VirtualMemoryAddress> theMisses;
};

struct BranchFeedback : boost::counted_base {
  VirtualMemoryAddress thePC;
  eBranchType theActualType;
  eDirection theActualDirection;
  eDirection theBPDirection;
  eBranchResolution branchResolution;
  VirtualMemoryAddress theActualTarget;
  int theBBsize;
  boost::intrusive_ptr<BPredState> theBPState;
};
std::ostream &operator<<(std::ostream &anOstream, const BranchFeedback &aBPState);

typedef uint32_t Opcode;

struct FetchedOpcode {
  VirtualMemoryAddress thePC;
  VirtualMemoryAddress theNextPC;
  Opcode theOpcode;
  boost::intrusive_ptr<BPredState> theBPState;
  boost::intrusive_ptr<TransactionTracker> theTransaction;
  uint32_t theSerial;
  boost::intrusive_ptr<InfoMissStats> missStatsInfo;
  /*
  bool thehadIcacheMiss;
  bool theMissPrefetchOnWay;
  uint64_t theIcacheMissCycles;
  // */

  FetchedOpcode(Opcode anOpcode) : theOpcode(anOpcode) {
  }
  FetchedOpcode(VirtualMemoryAddress anAddr, Opcode anOpcode,
                boost::intrusive_ptr<BPredState> aBPState,
                boost::intrusive_ptr<TransactionTracker> aTransaction)
      : thePC(anAddr), theOpcode(anOpcode), theBPState(aBPState), theTransaction(aTransaction) {
  }
  FetchedOpcode(VirtualMemoryAddress anAddr, VirtualMemoryAddress aNextAddr, Opcode anOpcode,
                boost::intrusive_ptr<BPredState> aBPState,
                boost::intrusive_ptr<TransactionTracker> aTransaction, uint32_t aSerial,
                boost::intrusive_ptr<InfoMissStats> _missStatsInfo
                /*
                bool hadIcacheMiss,
                bool missPrefetchOnWay,
                uint64_t IcacheMissCycles
                //  */
                )
      : thePC(anAddr), theNextPC(aNextAddr), theOpcode(anOpcode), theBPState(aBPState),
        theTransaction(aTransaction), theSerial(aSerial), missStatsInfo(_missStatsInfo)
        /*
        thehadIcacheMiss(hadIcacheMiss),
        theMissPrefetchOnWay(missPrefetchOnWay),
        theIcacheMissCycles(IcacheMissCycles)
        // */
  {
  }
};

struct FetchBundle : public boost::counted_base {
  std::list<FetchedOpcode> theOpcodes;
  std::list<tFillLevel> theFillLevels;
  int32_t coreID;

  void updateOpcode(VirtualMemoryAddress anAddress, std::list<FetchedOpcode>::iterator it,
                    Opcode anOpcode) {
    DBG_AssertSev(Crit, it->thePC == anAddress,
                  (<< "ERROR: FetchedOpcode iterator did not match!! Iterator PC " << it->thePC
                   << ", translation returned vaddr " << anAddress));
    it->theOpcode = anOpcode;
  }

  void clear() {
    theOpcodes.clear();
    theFillLevels.clear();
  }
};

typedef boost::intrusive_ptr<FetchBundle> pFetchBundle;

struct CPUState {
  int32_t theTL;
  int32_t thePSTATE;
};

} // end namespace SharedTypes
} // end namespace Flexus

#endif // FLEXUS_uFETCH_TYPES_HPP_INCLUDED
