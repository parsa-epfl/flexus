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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <list>

#include <boost/iterator/reverse_iterator.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant/get.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/metaprogram.hpp>
#include <memory>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
using namespace boost::multi_index;
#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

#include "../coreModel.hpp"
//#include "../MapTable.hpp"
#include "../BypassNetwork.hpp"
#include "../RegisterFile.hpp"

namespace nuArchARM {

using Flexus::Core::theFlexus;

static const int32_t kxRegs_Global = kGlobalRegCount;
static const int32_t kvRegs = 64;
static const int32_t kSpecialRegs = kSpecialRegCount;
static const int32_t kxRegs_Total = kTotalRegs;
static const int32_t kccRegs = 5;

struct by_insn {};
typedef multi_index_container<
    boost::intrusive_ptr<Instruction>,
    indexed_by<sequenced<>,
               ordered_non_unique<tag<by_insn>, identity<boost::intrusive_ptr<Instruction>>>>>
    rob_t;

typedef std::multimap<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>>
    SpeculativeLoadAddressTracker;

struct MSHR;
typedef std::map<PhysicalMemoryAddress, MSHR> MSHRs_t;

enum eStatus {
  kComplete,
  kAnnulled,
  kIssuedToMemory,
  kAwaitingIssue,
  kAwaitingPort,
  kAwaitingAddress,
  kAwaitingValue
};

enum eQueue { kLSQ, kSSB, kSB, kLastQueueType };
std::ostream &operator<<(std::ostream &anOstream, eQueue aCode);

bool mmuASI(int32_t asi);
bool interruptASI(int32_t asi);

struct MemQueueEntry {
  boost::intrusive_ptr<Instruction> theInstruction;
  PhysicalMemoryAddress thePaddr_aligned;
  uint64_t theSequenceNum;
  eQueue theQueue;
  mutable VirtualMemoryAddress theVaddr;
  mutable bool theSideEffect;
  mutable bool theInverseEndian;
  //  mutable bool theMMU;
  mutable bool theNonCacheable;
  mutable eExceptionType theException;
  mutable PhysicalMemoryAddress thePaddr;
  mutable eOperation theOperation;
  mutable eSize theSize;
  mutable boost::optional<InstructionDependance> theDependance;
  mutable bool theAnnulled;
  mutable bool thePartialSnoop;
  mutable bool theIssued;
  mutable bool theStoreComplete;
  mutable bool theSpeculatedValue;
  mutable bool theBypassSB;
  mutable boost::optional<bits> theExtendedValue;
  mutable boost::optional<bits> theValue;
  mutable boost::optional<bits> theCompareValue;
  mutable boost::optional<MSHRs_t::iterator> theMSHR;
  mutable std::set<uint64_t> theParallelAddresses;
  mutable boost::optional<uint64_t> theExtraLatencyTimeout;
  MemQueueEntry(boost::intrusive_ptr<Instruction> anInstruction, uint64_t aSequenceNum,
                eOperation anOperation, eSize aSize, bool aBypassSB,
                boost::optional<InstructionDependance> aDependance = boost::none)
      : theInstruction(anInstruction), thePaddr_aligned(kUnresolved), theSequenceNum(aSequenceNum),
        theQueue(kLSQ), theVaddr(kUnresolved), theSideEffect(false), theInverseEndian(false)
        //    , theMMU(false)
        ,
        theNonCacheable(false), theException(kException_None), thePaddr(kUnresolved),
        theOperation(anOperation), theSize(aSize), theDependance(aDependance), theAnnulled(false),
        thePartialSnoop(false), theIssued(false), theStoreComplete(false),
        theSpeculatedValue(false), theBypassSB(aBypassSB) {
  }
  void describe(std::ostream &anOstream) const;
  friend std::ostream &operator<<(std::ostream &anOstream, MemQueueEntry const &anEntry);
  bool isStore() const {
    return theOperation == kStore || theOperation == kRMW || theOperation == kCAS;
  }
  bool isLoad() const {
    return theOperation == kLoad || theOperation == kRMW || theOperation == kCAS;
  }
  void annul() const {
    theAnnulled = true;
  }
  bool isMarker() const {
    return theOperation == kMEMBARMarker;
  }
  bool isAtomic() const {
    return theOperation == kRMW || theOperation == kCAS || theOperation == kCASP;
  }
  bool isAbnormalAccess() const {
    return theSideEffect ||
           /*theMMU ||*/ theException != kException_None /*|| interruptASI(theASI)*/;
  }
  bool isNAW() const {
    return theBypassSB;
  }
  boost::optional<bits> &loadValue() const {
    if (isAtomic()) {
      return theExtendedValue;
    } else {
      return theValue;
    }
  }
  eStatus status() const {
    if (theAnnulled) {
      return kAnnulled;
    }
    if (theOperation == kMEMBARMarker) {
      return kIssuedToMemory; // Unless annulled, MEMBAR Markers are categorized
                              // as "issued to memory"
    }
    if (thePaddr == kUnresolved) {
      return kAwaitingAddress;
    }
    switch (theOperation) {
    case kLoad:
      if (theValue) {
        return kComplete;
      } else if (theMSHR) {
        return kIssuedToMemory;
      } else if (theIssued) {
        return kAwaitingPort;
      } else {
        return kAwaitingIssue;
      }
      break;
    case kStore:
      if (!theValue) {
        return kAwaitingValue;
      } else if (theMSHR) {
        return kIssuedToMemory;
      } else if (theIssued) {
        return kAwaitingPort;
      } else {
        return kAwaitingIssue;
      }
      break;
    case kRMW:
    case kCAS:
      if (theValue && theExtendedValue) {
        // Load is complete, store has obtained value
        return kComplete;
      } else if (theMSHR) {
        return kIssuedToMemory; // Preload is issued to memoty
      } else if (theIssued) {
        return kAwaitingPort; // Preload is awaiting port
      }                       /*else if ( theExtendedValue && ! theValue ) {
                              //Store is awaiting value
                              return kAwaitingValue;
                            }*/
      else {
        return kAwaitingIssue; // Preload is awaiting issue
      }
      break;
    default:
      DBG_Assert(false, (<< " Unknown operation: " << *this));
    }
    return kComplete; // Suppress warning
  }
};

struct by_paddr {};
struct by_seq {};
struct by_queue {};
struct by_prefetch {};
typedef multi_index_container<
    MemQueueEntry,
    indexed_by<
        sequenced<tag<by_seq>>,

        ordered_unique<tag<by_paddr>, composite_key<MemQueueEntry,
                                                    member<MemQueueEntry, PhysicalMemoryAddress,
                                                           &MemQueueEntry::thePaddr_aligned>,
                                                    member<MemQueueEntry, uint64_t,
                                                           &MemQueueEntry::theSequenceNum>>>,
        ordered_unique<tag<by_insn>, member<MemQueueEntry, boost::intrusive_ptr<Instruction>,
                                            &MemQueueEntry::theInstruction>>,
        ordered_unique<
            tag<by_queue>,
            composite_key<MemQueueEntry, member<MemQueueEntry, eQueue, &MemQueueEntry::theQueue>,
                          member<MemQueueEntry, uint64_t, &MemQueueEntry::theSequenceNum>>>>>
    memq_t;

struct ActionOrder {
  bool operator()(boost::intrusive_ptr<SemanticAction> const &l,
                  boost::intrusive_ptr<SemanticAction> const &r) const;
};

typedef std::priority_queue<boost::intrusive_ptr<SemanticAction>,
                            std::vector<boost::intrusive_ptr<SemanticAction>>, ActionOrder>
    action_list_t;

struct MSHR {
  PhysicalMemoryAddress thePaddr;
  eOperation theOperation;
  eSize theSize;
  std::list<memq_t::index<by_insn>::type::iterator> theWaitingLSQs;
  std::list<boost::intrusive_ptr<Instruction>> theBlockedOps;
  std::list<boost::intrusive_ptr<Instruction>> theBlockedPrefetches;
  boost::intrusive_ptr<TransactionTracker> theTracker;
  MSHR() : thePaddr(0), theOperation(kLoad), theSize(kWord) {
  }
};

std::ostream &operator<<(std::ostream &anOstream, MSHR const &anMSHR);

class CoreImpl;

struct PortRequest {
  uint64_t theAge;
  boost::intrusive_ptr<Instruction> theEntry;
  PortRequest(uint64_t age, boost::intrusive_ptr<Instruction> anInstruction)
      : theAge(age), theEntry(anInstruction) {
  }
  friend bool operator<(PortRequest const &left, PortRequest const &right) {
    return left.theAge > right.theAge; // higher age implies lower priority
  }
};

struct StorePrefetchRequest {
  uint64_t theAge;
  boost::intrusive_ptr<Instruction> theInstruction;
  StorePrefetchRequest(uint64_t anAge, boost::intrusive_ptr<Instruction> anInstruction)
      : theAge(anAge), theInstruction(anInstruction) {
  }
};

typedef multi_index_container<
    StorePrefetchRequest,
    indexed_by<
        ordered_unique<tag<by_seq>,
                       member<StorePrefetchRequest, uint64_t, &StorePrefetchRequest::theAge>>,
        ordered_unique<tag<by_insn>, member<StorePrefetchRequest, boost::intrusive_ptr<Instruction>,
                                            &StorePrefetchRequest::theInstruction>>>>
    prefetch_queue_t;

struct MemoryPortArbiter {
  uint32_t theNumPorts;

  std::priority_queue<PortRequest> theReadRequests;
  std::priority_queue<PortRequest> thePriorityRequests;
  CoreImpl &theCore;

  // Store Prefetching
  uint32_t theMaxStorePrefetches;
  prefetch_queue_t theStorePrefetchRequests;

  MemoryPortArbiter(CoreImpl &aCore, int32_t aNumPorts, int32_t aMaxStorePrefetches);
  void inOrderArbitrate();
  void arbitrate();
  void request(eOperation anOperation, uint64_t anAge,
               boost::intrusive_ptr<Instruction> anInstruction);
  void requestStorePrefetch(memq_t::index<by_insn>::type::iterator lsq_entry);
  bool empty() const;
};

struct MemOpCounter {
  MemOpCounter(std::string aName)
      : theCount(aName + "-Count"), theRetireStalls(aName + "-RetireStalls"),
        theRetireStalls_Histogram(aName + "-RetireStalls_Hist"),
        thePrefetchCount(aName + "-PrefetchCount"), thePrefetchLatency(aName + "-PrefetchLatency"),
        thePrefetchLatency_Histogram(aName + "-PrefetchLatency_Hist"),
        theRequestCount(aName + "-RequestCount"), theRequestLatency(aName + "-RequestLatency"),
        theRequestLatency_Histogram(aName + "-RequestLatency_Hist") {
  }
  Stat::StatCounter theCount;
  Stat::StatCounter theRetireStalls;
  Stat::StatInstanceCounter<int64_t> theRetireStalls_Histogram;
  Stat::StatCounter thePrefetchCount;
  Stat::StatCounter thePrefetchLatency;
  Stat::StatInstanceCounter<int64_t> thePrefetchLatency_Histogram;
  Stat::StatCounter theRequestCount;
  Stat::StatCounter theRequestLatency;
  Stat::StatInstanceCounter<int64_t> theRequestLatency_Histogram;
};

struct Checkpoint {
  Checkpoint() : theLostPermissionCount(0) {
    theState = {0};
  }
  armState theState;
  int32_t theLostPermissionCount;
  std::map<PhysicalMemoryAddress, boost::intrusive_ptr<Instruction>> theRequiredPermissions;
  std::set<PhysicalMemoryAddress> theHeldPermissions;
};

} // namespace nuArchARM
