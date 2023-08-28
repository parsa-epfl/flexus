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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
using namespace boost::multi_index;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/export.hpp>
#include <boost/optional/optional_io.hpp>
#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/CommonQEMU/Tage.h>

#define DBG_DefineCategories BPred
#define DBG_SetDefaultOps AddCat(BPred)
#include DBG_Control

namespace Flexus {
namespace SharedTypes {

struct by_baddr {};
typedef multi_index_container<
    BTBEntry,
    indexed_by<sequenced<>, ordered_unique<tag<by_baddr>, member<BTBEntry, VirtualMemoryAddress,
                                                                 &BTBEntry::thePC>>>>
    btb_set_t;

template <class Archive> void save(Archive &ar, const btb_set_t &t, uint32_t version) {
  std::list<BTBEntry> btb;
  std::copy(t.begin(), t.end(), std::back_inserter(btb));
  // the hackish const is necessary to satisfy boost 1.33.1
  ar << (const std::list<BTBEntry>)btb;
}
template <class Archive> void load(Archive &ar, btb_set_t &t, uint32_t version) {
  std::list<BTBEntry> btb;
  ar >> btb;
  std::copy(btb.begin(), btb.end(), std::back_inserter(t));
}

} // namespace SharedTypes
} // namespace Flexus

namespace boost {
namespace serialization {
template <class Archive>
inline void serialize(Archive &ar, Flexus::SharedTypes::btb_set_t &t, const uint32_t file_version) {
  split_free(ar, t, file_version);
}
} // namespace serialization
} // namespace boost

namespace Flexus {
namespace SharedTypes {

int64_t log2(int64_t aVal) {
  DBG_Assert(aVal != 0);
  --aVal;
  uint32_t ii = 0;
  while (aVal > 0) {
    ii++;
    aVal >>= 1;
  }
  return ii;
}

eDirection moreTaken(eDirection aDirection) {
  if (aDirection == kStronglyTaken) {
    return kStronglyTaken;
  }
  return eDirection(aDirection - 1);
}

eDirection moreNotTaken(eDirection aDirection) {
  if (aDirection == kStronglyNotTaken) {
    return kStronglyNotTaken;
  }
  return eDirection(aDirection + 1);
}

eDirection apply(eDirection aDirection, eDirection aState) {
  if (aDirection <= kTaken) {
    return moreTaken(aState);
  } else {
    return moreNotTaken(aState);
  }
}

eDirection reverse(eDirection direction) {
  if (direction >= kNotTaken) {
    direction = kTaken;
  } else {
    direction = kNotTaken;
  }
  return direction;
}

struct gShare {
  std::vector<eDirection> thePatternTable;
  uint32_t theShiftReg;
  uint32_t theRunaheadShiftReg;
  int32_t theShiftRegSize;
  uint32_t thePatternMask;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    uint32_t shift_reg = theShiftReg;
    ar &thePatternTable;
    ar &theShiftRegSize;
    ar &thePatternMask;
    ar &shift_reg;
    theShiftReg = (theShiftReg & theShiftRegSize);
    theRunaheadShiftReg = (theRunaheadShiftReg & theShiftRegSize);
  }
  gShare() {
  }

public:
  gShare(int32_t aSize) : theShiftRegSize(aSize) {
    // Set the history pattern mask to 2^bits-1
    thePatternMask = (1 << aSize) - 1;

    reset();
  }

  void reset() {
    thePatternTable.clear();
    theShiftReg = 0;
    theRunaheadShiftReg = 0;
    eDirection direction = kNotTaken;
    for (int32_t i = 0; i < (1 << theShiftRegSize); ++i) {
      thePatternTable.push_back(direction);
      direction = reverse(direction);
    }
  }

  // This hashing function was stolen from SimpleScalar.  It is quite
  // arbitrary.
  int32_t index(VirtualMemoryAddress anAddress) {
    return ((theShiftReg) ^ (anAddress >> 2)) & thePatternMask;
  }

  int32_t index(VirtualMemoryAddress anAddress, uint32_t aShiftReg) {
    return ((aShiftReg) ^ (anAddress >> 2)) & thePatternMask;
  }

  void shiftIn(eDirection aDirection) {
    theShiftReg = (theShiftReg << 1) & thePatternMask;
    if (aDirection >= kNotTaken) {
      theShiftReg |= 1;
    }
  }

  void runaheadShiftIn(eDirection aDirection) {
    theRunaheadShiftReg = (theRunaheadShiftReg << 1) & thePatternMask;
    if (aDirection >= kNotTaken) {
      theRunaheadShiftReg |= 1;
    }
  }

  void setShiftReg(uint32_t aPreviousShiftRegState) {
    theShiftReg = aPreviousShiftRegState & thePatternMask;
  }

  uint32_t shiftReg() const {
    return theShiftReg;
  }

  eDirection direction(VirtualMemoryAddress anAddress) {
    return thePatternTable[index(anAddress)];
  }

  eDirection runaheadDirection(VirtualMemoryAddress anAddress) {
    return thePatternTable[((theRunaheadShiftReg) ^ (anAddress >> 2)) & thePatternMask];
  }

  void resetRunaheadHistory() {
    theRunaheadShiftReg = theShiftReg;
  }

  eDirection direction(VirtualMemoryAddress anAddress, uint32_t aShiftRegState) {
    return thePatternTable[index(anAddress, aShiftRegState)];
  }

  void update(VirtualMemoryAddress anAddress, eDirection aDirection) {
    thePatternTable[index(anAddress)] = aDirection;
  }

  void update(VirtualMemoryAddress anAddress, uint32_t aShiftRegState, eDirection aDirection) {
    thePatternTable[index(anAddress, aShiftRegState)] = aDirection;
  }
};

struct Bimodal {
  std::vector<eDirection> theTable;
  int32_t theSize;
  uint64_t theIndexMask;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &theTable;
    ar &theSize;
    ar &theIndexMask;
  }
  Bimodal() {
  }

public:
  Bimodal(int32_t aSize) : theSize(aSize) {
    // aBTBSize must be a power of 2
    DBG_Assert(((aSize - 1) & (aSize)) == 0);
    theIndexMask = aSize - 1;

    reset();
  }

  void reset() {
    theTable.clear();
    eDirection direction = kNotTaken;
    for (int32_t i = 0; i < theSize; ++i) {
      theTable.push_back(direction);
      direction = reverse(direction);
    }
  }

  // This hashing function was stolen from SimpleScalar.  It is quite
  // arbitrary.
  int32_t index(VirtualMemoryAddress anAddress) {
    return ((anAddress >> 19) ^ (anAddress >> 2)) & theIndexMask;
  }

  eDirection direction(VirtualMemoryAddress anAddress) {
    return theTable[index(anAddress)];
  }

  void update(VirtualMemoryAddress anAddress, eDirection aDirection) {
    theTable[index(anAddress)] = aDirection;
  }
};

struct BTB {
  std::vector<btb_set_t> theBTB;
  uint32_t theBTBSets;
  uint32_t theBTBAssoc;
  uint64_t theIndexMask;
  std::set<VirtualMemoryAddress> branches;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &theBTB;
    ar &theBTBSets;
    ar &theBTBAssoc;
    ar &theIndexMask;
  }
  BTB() {
  }

public:
  BTB(int32_t aBTBSets, int32_t aBTBAssoc) : theBTBSets(aBTBSets), theBTBAssoc(aBTBAssoc) {
    // aBTBSize must be a power of 2
    DBG_Assert(((aBTBSets - 1) & (aBTBSets)) == 0);
    theBTB.resize(aBTBSets);
    theIndexMask = aBTBSets - 1;
  }

  int32_t index(VirtualMemoryAddress anAddress) {
    // Shift address by 2, since we assume word-aligned PCs
    return (anAddress >> 2) & theIndexMask;
  }

  bool contains(VirtualMemoryAddress anAddress) {
    return (theBTB[index(anAddress)].get<by_baddr>().count(anAddress) > 0);
  }

  BTBEntry getEntry(VirtualMemoryAddress anAddress) {
    int32_t idx = index(anAddress);
    btb_set_t::index<by_baddr>::type::iterator iter = theBTB[idx].get<by_baddr>().find(anAddress);
    if (iter == theBTB[idx].get<by_baddr>().end()) {
      return (BTBEntry(VirtualMemoryAddress(0), kNonBranch, VirtualMemoryAddress(0), 0));
    }
    return *iter;
  }
  eBranchType type(VirtualMemoryAddress anAddress) {
    int32_t idx = index(anAddress);
    btb_set_t::index<by_baddr>::type::iterator iter = theBTB[idx].get<by_baddr>().find(anAddress);
    if (iter == theBTB[idx].get<by_baddr>().end()) {
      return kNonBranch;
    }
    return iter->theBranchType;
  }

  boost::optional<VirtualMemoryAddress> target(VirtualMemoryAddress anAddress) {
    int32_t idx = index(anAddress);
    btb_set_t::index<by_baddr>::type::iterator iter = theBTB[idx].get<by_baddr>().find(anAddress);
    if (iter == theBTB[idx].get<by_baddr>().end()) {
      return boost::none;
    }
    return iter->theTarget;
  }

  std::pair<bool, BTBEntry> update(VirtualMemoryAddress aPC, eBranchType aType,
                                   VirtualMemoryAddress aTarget, int BBsize,
                                   eDirection actualDirection, eDirection BPDirection,
                                   bool specialCall) {
    branches.insert(aPC);
    //std::cout << "branches: " << branches.size() << "\n";

    int32_t idx = index(aPC);
    btb_set_t::index<by_baddr>::type::iterator iter = theBTB[idx].get<by_baddr>().find(aPC);
    if (iter != theBTB[idx].get<by_baddr>().end()) {
      if (aType == kNonBranch) {
        theBTB[idx].get<by_baddr>().erase(iter);
      } else {
        iter->theBranchType = aType;
        iter->theBBsize = BBsize;
        iter->theBranchDirection = apply(actualDirection, iter->theBranchDirection);
        iter->isSpecialCall = specialCall;
        if (aTarget) {
          DBG_(Iface, (<< "BTB setting target for " << aPC << " to " << aTarget));
          iter->theTarget = aTarget;
        }
        theBTB[idx].relocate(theBTB[idx].end(), theBTB[idx].project<0>(iter));
      }
      return std::make_pair(false, BTBEntry(VirtualMemoryAddress(0), kNonBranch,
                                            VirtualMemoryAddress(0), 0)); // not a new entry
    } else if (aType != kNonBranch) {
      BTBEntry anEntry(VirtualMemoryAddress(0), kNonBranch, VirtualMemoryAddress(0), 0);
      if (theBTB[idx].size() >= theBTBAssoc) {
        anEntry = theBTB[idx].front();
        theBTB[idx].pop_front();
      }
      DBG_(Iface,
           (<< "New Entry for " << aPC << " size " << BBsize << " type " << aType << " direction "
            << BPDirection << " target " << aTarget << " special call " << specialCall));
      DBG_(Iface, (<< "Evicted " << anEntry.thePC));

      DBG_(Iface, (<< "BTB adding new branch for " << aPC << " to " << aTarget));
      theBTB[idx].push_back(BTBEntry(aPC, aType, aTarget, BBsize, BPDirection, specialCall));
      return std::make_pair(true, anEntry); // new entry
    }
    return std::make_pair(false, BTBEntry(VirtualMemoryAddress(0), kNonBranch,
                                          VirtualMemoryAddress(0), 0)); // not a new entry
  }

  std::pair<bool, BTBEntry> update(BranchFeedback const &aFeedback) {
    return update(aFeedback.thePC, aFeedback.theActualType, aFeedback.theActualTarget,
                  aFeedback.theBBsize, aFeedback.theActualDirection, aFeedback.theBPDirection,
                  aFeedback.theBPState->detectedSpecialCall);
  }
}; // namespace SharedTypes

struct discontTableEntry {
  VirtualMemoryAddress branchBlock;
  VirtualMemoryAddress targetBlock;
  eDirection satCounter;
  discontTableEntry(VirtualMemoryAddress branch, VirtualMemoryAddress target, eDirection confidence)
      : branchBlock(branch), targetBlock(target), satCounter(confidence) {
  }

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &branchBlock;
    ar &targetBlock;
    ar &satCounter;
  }
  discontTableEntry() {
  }
};

struct discontTable {
  std::vector<discontTableEntry> theTable;
  int32_t theSize;
  uint64_t theIndexMask;

public:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const uint32_t version) {
    ar &theTable;
    ar &theSize;
    ar &theIndexMask;
  }
  discontTable() {
  }

public:
  discontTable(int32_t aSize) : theSize(aSize) {
    // aBTBSize must be a power of 2
    assert(((aSize - 1) & (aSize)) == 0);
    theIndexMask = aSize - 1;

    reset();
  }

  void reset() {
    theTable.clear();
    discontTableEntry anEntry(VirtualMemoryAddress(0), VirtualMemoryAddress(0), kStronglyTaken);
    for (int32_t i = 0; i < theSize; ++i) {
      theTable.push_back(anEntry);
    }
  }

  // This hashing function was stolen from SimpleScalar.  It is quite
  // arbitrary.
  int32_t index(VirtualMemoryAddress anAddress) {
    return ((anAddress >> 19) ^ (anAddress >> 6)) & theIndexMask;
  }

  discontTableEntry getEntry(VirtualMemoryAddress anAddress) {
    return theTable[index(anAddress)];
  }

  void increaseConf(VirtualMemoryAddress anAddress) {
    if (theTable[index(anAddress)].satCounter != kStronglyNotTaken) {
      theTable[index(anAddress)].satCounter = eDirection(theTable[index(anAddress)].satCounter + 1);
    }
  }

  void decreaseConf(VirtualMemoryAddress anAddress) {
    if (theTable[index(anAddress)].satCounter != kStronglyTaken) {
      theTable[index(anAddress)].satCounter = eDirection(theTable[index(anAddress)].satCounter - 1);
    }
  }

  void update(discontTableEntry anEntry) {
    theTable[index(anEntry.branchBlock)] = anEntry;
  }
};

struct CombiningImpl : public BranchPredictor {

  std::string theName;
  uint32_t theIndex;
  uint32_t theSerial;

  bool enableRAS;       // RAS: Return Address Stack
  bool enableTCE;       // TCE: Tail Call Elimination
  bool redirectTrapRet; // Redirect fetch on a trap return

  BTB theBTB;
  Bimodal theBimodal;
  Bimodal theMeta;
  gShare theGShare;
  PREDICTOR theTage;
  std::list<VirtualMemoryAddress> theRAS;
  BTB theRASHelper;
  BTB theBB_BTB;
  uint32_t theTL;
  uint64_t theTPC[5];
  uint64_t theTNPC[5];

  Stat::StatCounter theBranches; // Retired Branches
  Stat::StatCounter theBranches_Unconditional;
  Stat::StatCounter theBranches_Conditional;
  Stat::StatCounter theBranches_Call;
  Stat::StatCounter theBranches_Return;
  Stat::StatCounter theBranches_Other;
  Stat::StatCounter theBranches_IndirectCall;
  Stat::StatCounter theBranches_RegIndirect;

  Stat::StatCounter thePredictions;
  Stat::StatCounter thePredictions_Bimodal;
  Stat::StatCounter thePredictions_GShare;
  Stat::StatCounter thePredictions_Unconditional;
  Stat::StatCounter thePredictions_Indirect;
  Stat::StatCounter thePredictions_Returns;

  Stat::StatCounter theCorrect;
  Stat::StatCounter theCorrect_Bimodal;
  Stat::StatCounter theCorrect_GShare;
  Stat::StatCounter theCorrect_Conditional;
  Stat::StatCounter theCorrect_Unconditional;
  Stat::StatCounter theCorrect_Indirect;
  Stat::StatCounter theCorrect_Returns;

  Stat::StatCounter theMispredict;
  Stat::StatCounter theMispredict_Conditional;
  Stat::StatCounter theMispredict_Unconditional;
  Stat::StatCounter theMispredict_Call;
  Stat::StatCounter theMispredict_Return;
  Stat::StatCounter theMispredict_Indirect;
  Stat::StatCounter theMispredict_UncondDir;
  Stat::StatCounter theMispredict_NewBranch;
  Stat::StatCounter theMispredict_NewBranchTransFailed;
  Stat::StatCounter theMispredict_nonBRasBR;
  Stat::StatCounter theMispredict_branchType;
  Stat::StatCounter theMispredict_Direction;
  Stat::StatCounter theMispredict_Meta;
  Stat::StatCounter theMispredict_MetaGShare;
  Stat::StatCounter theMispredict_MetaBimod;
  Stat::StatCounter theMispredict_Target;
  Stat::StatCounter theVictimHits;
  Stat::StatCounter theVictimMisses;
  Stat::StatCounter theMispredict_IndirectTargets;

  Stat::StatCounter *bimodalConfindence[4];
  Stat::StatCounter *tageConfindence[8];

  CombiningImpl(std::string const &aName, uint32_t anIndex, bool RAS, bool TCE, bool TrapRet,
                uint32_t btbSets, uint32_t btbWays)
      : theName(aName), theIndex(anIndex), theSerial(0), enableRAS(RAS), enableTCE(TCE),
        redirectTrapRet(TrapRet), theBTB(btbSets, btbWays), theBimodal(16384), theMeta(16384),
        theGShare(14), theRASHelper(btbSets, btbWays), theBB_BTB(btbSets, btbWays),
        theBranches(aName + "-branches"),
        theBranches_Unconditional(aName + "-branches:unconditional"),
        theBranches_Conditional(aName + "-branches:conditional"),
        theBranches_Call(aName + "-branches:call"), theBranches_Return(aName + "-branches:return"),
        theBranches_Other(aName + "-branches:other"),
        theBranches_IndirectCall(aName + "-branches:indirect:call"),
        theBranches_RegIndirect(aName + "-branches:indirect:reg"),
        thePredictions(aName + "-predictions"),
        thePredictions_Bimodal(aName + "-predictions:bimodal"),
        thePredictions_GShare(aName + "-predictions:gshare"),
        thePredictions_Unconditional(aName + "-predictions:unconditional"),
        thePredictions_Indirect(aName + "-predictions:indirect"),
        thePredictions_Returns(aName + "-predictions:returns"), theCorrect(aName + "-correct"),
        theCorrect_Bimodal(aName + "-correct:bimodal"),
        theCorrect_GShare(aName + "-correct:gshare"),
        theCorrect_Conditional(aName + "-correct:conditional"),
        theCorrect_Unconditional(aName + "-correct:unconditional"),
        theCorrect_Indirect(aName + "-correct:indirect"),
        theCorrect_Returns(aName + "-correct:returns"), theMispredict(aName + "-mispredict"),
        theMispredict_Conditional(aName + "-mispredict:conditional"),
        theMispredict_Unconditional(aName + "-mispredict:unconditional"),
        theMispredict_Call(aName + "-mispredict:call"),
        theMispredict_Return(aName + "-mispredict:return"),
        theMispredict_Indirect(aName + "-mispredict:indirect"),
        theMispredict_UncondDir(aName + "-mispredict:unconddir"),
        theMispredict_NewBranch(aName + "-mispredict:new"),
        theMispredict_NewBranchTransFailed(aName + "-mispredict:newTransFailed"),
        theMispredict_nonBRasBR(aName + "-mispredict:nonBRasBR"),
        theMispredict_branchType(aName + "-mispredict:branchType"),
        theMispredict_Direction(aName + "-mispredict:direction"),
        theMispredict_Meta(aName + "-mispredict:meta"),
        theMispredict_MetaGShare(aName + "-mispredict:meta:chose_gshare"),
        theMispredict_MetaBimod(aName + "-mispredict:meta:chose_bimod"),
        theMispredict_Target(aName + "-mispredict:target"), theVictimHits(aName + "-victimHits"),
        theVictimMisses(aName + "-victimMisses"),
        theMispredict_IndirectTargets(aName + "-mispredict:target:indirect") {
    DBG_(Tmp,
         (<< "Num BTB Entries: sets " << theBTB.theBTBSets << " assoc " << theBTB.theBTBAssoc));
    char stat_name[50];
    for (int i = 0; i < 4; i++) {
      sprintf(stat_name, "-bimodConfidence%d", i);
      bimodalConfindence[i] = new Stat::StatCounter(aName + stat_name);
    }
    for (int i = 0; i < 8; i++) {
      sprintf(stat_name, "-tageConfidence%d", i);
      tageConfindence[i] = new Stat::StatCounter(aName + stat_name);
    }
  }

  CombiningImpl(std::string const &aName, uint32_t anIndex, bool RAS, bool TCE, bool TrapRet)
      : theName(aName), theIndex(anIndex), theSerial(0), enableRAS(RAS), enableTCE(TCE),
        redirectTrapRet(TrapRet), theBTB(512, 4), theBimodal(16384), theMeta(16384), theGShare(14),
        theRASHelper(512, 4), theBB_BTB(512, 4), theBranches(aName + "-branches"),
        theBranches_Unconditional(aName + "-branches:unconditional"),
        theBranches_Conditional(aName + "-branches:conditional"),
        theBranches_Call(aName + "-branches:call"), theBranches_Return(aName + "-branches:return"),
        theBranches_Other(aName + "-branches:other"),
        theBranches_IndirectCall(aName + "-branches:indirect:call"),
        theBranches_RegIndirect(aName + "-branches:indirect:reg"),
        thePredictions(aName + "-predictions"),
        thePredictions_Bimodal(aName + "-predictions:bimodal"),
        thePredictions_GShare(aName + "-predictions:gshare"),
        thePredictions_Unconditional(aName + "-predictions:unconditional"),
        thePredictions_Indirect(aName + "-predictions:indirect"),
        thePredictions_Returns(aName + "-predictions:returns"), theCorrect(aName + "-correct"),
        theCorrect_Bimodal(aName + "-correct:bimodal"),
        theCorrect_GShare(aName + "-correct:gshare"),
        theCorrect_Conditional(aName + "-correct:conditional"),
        theCorrect_Unconditional(aName + "-correct:unconditional"),
        theCorrect_Indirect(aName + "-correct:indirect"),
        theCorrect_Returns(aName + "-correct:returns"), theMispredict(aName + "-mispredict"),
        theMispredict_Conditional(aName + "-mispredict:conditional"),
        theMispredict_Unconditional(aName + "-mispredict:unconditional"),
        theMispredict_Call(aName + "-mispredict:call"),
        theMispredict_Return(aName + "-mispredict:return"),
        theMispredict_Indirect(aName + "-mispredict:indirect"),
        theMispredict_UncondDir(aName + "-mispredict:unconddir"),
        theMispredict_NewBranch(aName + "-mispredict:new"),
        theMispredict_NewBranchTransFailed(aName + "-mispredict:newTransFailed"),
        theMispredict_nonBRasBR(aName + "-mispredict:nonBRasBR"),
        theMispredict_branchType(aName + "-mispredict:branchType"),
        theMispredict_Direction(aName + "-mispredict:direction"),
        theMispredict_Meta(aName + "-mispredict:meta"),
        theMispredict_MetaGShare(aName + "-mispredict:meta:chose_gshare"),
        theMispredict_MetaBimod(aName + "-mispredict:meta:chose_bimod"),
        theMispredict_Target(aName + "-mispredict:target"), theVictimHits(aName + "-victimHits"),
        theVictimMisses(aName + "-victimMisses"),
        theMispredict_IndirectTargets(aName + "-mispredict:target:indirect") {
    DBG_(Tmp,
         (<< "Num BTB Entries: sets " << theBTB.theBTBSets << " assoc " << theBTB.theBTBAssoc));
    char stat_name[50];
    for (int i = 0; i < 4; i++) {
      sprintf(stat_name, "-bimodConfidence%d", i);
      bimodalConfindence[i] = new Stat::StatCounter(aName + stat_name);
    }
    for (int i = 0; i < 8; i++) {
      sprintf(stat_name, "-tageConfidence%d", i);
      tageConfindence[i] = new Stat::StatCounter(aName + stat_name);
    }
  }

  void resetRAS() {
    theRAS.clear();
  }

  void reconstructRAS(std::list<boost::intrusive_ptr<BPredState>> &theRASops) {
    if (!enableRAS) {
      theRASops.clear();
      return;
    }
    //    	  DBG_( Tmp, ( << "Reconst RAS with num elems " << theRASops.size()));
    while (theRASops.size()) {
      //    		  DBG_( Tmp, ( << "RAS update for " << theRASops.front()->pc << " type " <<
      //    theRASops.front()->thePredictedType));
      switch (theRASops.front()->thePredictedType) {
      case kCall:
        if (theRAS.size() && theRASops.front()->callUpdatedRAS) {
          //    				  DBG_( Tmp, ( << "poping " <<  theRAS.back()));
          theRAS.pop_back();
        }
        break;
      case kReturn:
        if (theRASops.front()->returnUsedRAS) {
          theRAS.push_back(theRASops.front()->thePredictedTarget);
          //    			  	  DBG_( Tmp, ( << "pushing " <<
          //    theRASops.front()->thePredictedTarget));
        }
        /*if (theRASops.front()->returnPopRASTwice) {
                theRAS.push_back(theRASops.front()->theAltPredictedTarget);
//    			  	  DBG_( Tmp, ( << "pushing extra " <<
theRASops.front()->theAltPredictedTarget));
        }*/
        break;
      default:
        assert(0);
        break;
      }
      theRASops.pop_front();
    }
    //    	  printRAS();
  }

  void pushReturnAddresstoRAS(VirtualMemoryAddress retAddress) {
    theRAS.push_back(retAddress);
    //    	  printRAS();
  }

  void printRAS() {
    DBG_(Tmp, (<< "RAS is "));
    for (std::list<VirtualMemoryAddress>::iterator it = theRAS.begin(); it != theRAS.end(); it++) {
      DBG_(Tmp, (<< "pc " << *it));
    }
    // TODO: remove it later
    std::cout << "RAS is:\n";
    for (std::list<VirtualMemoryAddress>::iterator it = theRAS.begin(); it != theRAS.end(); it++) {
      std::cout << "\tpc " << *it << "\n";
    }
  }

  // SPARC code
  uint64_t getTPC(uint32_t aTL) {
    if (aTL == 0 || aTL > 5)
      return 0;
    return theTPC[aTL - 1];
  }

  // SPARC code
  uint64_t getTNPC(uint32_t aTL) {
    if (aTL == 0 || aTL > 5)
      return 0;
    return theTNPC[aTL - 1];
  }

  // SPARC code
  void resetTrapState(boost::intrusive_ptr<TrapState> aTrapState) {
    //    	  DBG_( Tmp, ( << "Trap state received it was " << theTL));
    theTL = aTrapState->theTL;
    for (int i = 0; i < 5; i++) {
      //    		  DBG_( Tmp, ( << "regs before " << std::hex << theTPC[i] << " " <<
      //    theTNPC[i]));
      theTPC[i] = aTrapState->theTPC[i];
      theTNPC[i] = aTrapState->theTNPC[i];
      //    		  DBG_( Tmp, ( << "regs after " << std::hex << theTPC[i] << " " <<
      //    theTNPC[i]));
    }
    //    	  DBG_( Tmp, ( << "and theTL " << theTL));
  }

  bool isBranchInsn(VirtualMemoryAddress anAddress) {
    if (theBTB.contains(anAddress) && ((theBTB.type(anAddress) != kNonBranch) || redirectTrapRet)) {
      return true;
    }
    return false;
  }

  bool isBranch(FetchAddr &aFetch) {
    if (theBTB.contains(aFetch.theAddress) &&
        ((theBTB.type(aFetch.theAddress) != kNonBranch) || redirectTrapRet)) {
      return true;
    }
    aFetch.theBPState = boost::intrusive_ptr<BPredState>(new BPredState());
    aFetch.theBPState->thePredictedType = kNonBranch;
    aFetch.theBPState->thePrediction = kStronglyTaken;
    aFetch.theBPState->theActualDirection = kStronglyTaken;
    aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->pc = aFetch.theAddress;
    aFetch.theBPState->is_runahead = 0;
    aFetch.theBPState->detectedSpecialCall = false;
    aFetch.theBPState->callUpdatedRAS = false;
    aFetch.theBPState->theTL = theTL;
    aFetch.theBPState->haltDispatch = false;
    aFetch.theBPState->exceptionSource = xUnknown;
    aFetch.theBPState->BTBPreFilled = false;
    aFetch.theBPState->causedSquash = false;
    aFetch.theBPState->BTBPreFilled = false;
    aFetch.theBPState->hasRetired = false;

#ifdef TAGE
    theTage.checkpoint_history(*(aFetch.theBPState));
#else
    aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
    return false;
    //        return theBTB.contains(anAddress);
  }

  void resetState(boost::intrusive_ptr<BPredState> aBPState) {
#ifdef TAGE
    //    	  theTage.restore_retired_state();
    theTage.restore_all_state(*aBPState);
    theTL = aBPState->theTL;
#else
    theGShare.setShiftReg(aBPState->theGShareShiftReg);
#endif
  }

  void resetUpdateState(boost::intrusive_ptr<BPredState> aBPState) {
#ifdef TAGE
    if (aBPState) {
      DBG_(Iface, (<< "in resetUpdateState( " << *aBPState));
      DBG_Assert(aBPState->theActualDirection != kStronglyTaken,
                 (<< "theActualDirection not updated.  Cannot be kStronglyTaken"));
      theTage.restore_all_state(*aBPState);
      theTL = aBPState->theTL;
      if (aBPState->theActualType == kConditional) {
        if (aBPState->theActualDirection == kTaken) {
          theTage.update_history(*aBPState, true, aBPState->pc);
        } else if (aBPState->theActualDirection == kNotTaken) {
          theTage.update_history(*aBPState, false, aBPState->pc);
        } else {
          DBG_Assert(false, (<< "Should never enter here"));
        }
      } else {
        theTage.update_history(*aBPState, true, aBPState->pc);
      }
    } else {
      DBG_Assert(false, (<< "No BP state to revert to"));
      theTage.restore_retired_state();
    }
#else
    DBG_Assert(aBPState->theActualDirection != kStronglyTaken,
               (<< "theActualDirection not updated.  Cannot be kStronglyTaken"));
    DBG_Assert(aBPState, (<< "No BP state to revert to"));
    theGShare.setShiftReg(aBPState->theGShareShiftReg);
    theTL = aBPState->theTL;
    if (aBPState->theActualType == kConditional) {
      theGShare.shiftIn(aBPState->theActualDirection);
    }
#endif
  }

  eDirection conditionalTaken(VirtualMemoryAddress anAddress) {
#ifdef TAGE
    return theTage.isCondTaken((uint64_t)anAddress);

#else
    eDirection bimodal = theBimodal.direction(anAddress);
    eDirection gshare = theGShare.direction(anAddress);
    eDirection meta = theMeta.direction(anAddress);
    eDirection prediction;

    // Decide which predictor to believe
    if (meta >= kNotTaken) {
      prediction = gshare;
    } else {
      prediction = bimodal;
    }

    return prediction;
#endif
  }

  // timing
  VirtualMemoryAddress theTage_predictConditional(uint64_t anAddress, BPredState &theBPState) {
    bool isTaken = theTage.get_prediction(anAddress, theBPState);
    if (isTaken) {
      theBPState.thePrediction = kTaken;
    } else {
      theBPState.thePrediction = kNotTaken;
    }

    VirtualMemoryAddress aVAddress(anAddress);
    if (theBPState.thePrediction <= kTaken && theBTB.target(aVAddress)) {
      return *theBTB.target(aVAddress);
    } else {
      if (theBTB.target(aVAddress)) {
        theBPState.theNextPredictedTarget = *theBTB.target(aVAddress);
      }
      return VirtualMemoryAddress(0);
    }
  }

  VirtualMemoryAddress predictConditional(uint64_t instruction_addr, BPredState &theBPState) {
    theBPState.saturationCounter = 0;
    VirtualMemoryAddress aVAddress(instruction_addr);
    eDirection bimodal = theBimodal.direction(aVAddress);
    eDirection gshare = theGShare.direction(aVAddress);
    eDirection meta = theMeta.direction(aVAddress);
    eDirection prediction;

    // Decide which predictor to believe
    if (meta >= kNotTaken) {
      ++thePredictions;
      ++thePredictions_GShare;
      prediction = gshare;
    } else {
      ++thePredictions;
      ++thePredictions_Bimodal;
      prediction = bimodal;
    }

    // Record the predictor state/predictions
    theBPState.thePrediction = prediction;
    theBPState.theBimodalPrediction = bimodal;
    theBPState.theMetaPrediction = meta;
    theBPState.theGSharePrediction = gshare;
    theBPState.theGShareShiftReg = theGShare.shiftReg();

    // Speculatively update the shift register with the prediction from the
    // gshare predictor.
    // TODO: evaluate whether it's better to shift in the
    // overall prediction or just the gshare part.
    theGShare.shiftIn(gshare);

    DBG_(Verb,
         (<< theIndex << "-BPRED-COND:  " << std::hex << aVAddress << std::dec << " BIMOD "
          << bimodal << " GSHARE " << gshare << " META " << meta << " OVERALL " << prediction));

    if (prediction <= kTaken && theBTB.target(aVAddress)) {
      return *theBTB.target(aVAddress);
    } else {
      if (theBTB.target(aVAddress)) {
        theBPState.theNextPredictedTarget = *theBTB.target(aVAddress);
      }
      return VirtualMemoryAddress(0);
    }
  }

  VirtualMemoryAddress predict(FetchAddr &aFetch) {
    aFetch.theBPState = boost::intrusive_ptr<BPredState>(new BPredState());
    aFetch.theBPState->thePredictedType = theBTB.type(aFetch.theAddress);
    aFetch.theBPState->theSerial = theSerial++;
    aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->theNextPredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->thePrediction = kStronglyTaken;
    aFetch.theBPState->theBimodalPrediction = kStronglyTaken;
    aFetch.theBPState->theMetaPrediction = kStronglyTaken;
    aFetch.theBPState->theGSharePrediction = kStronglyTaken;
    aFetch.theBPState->theActualDirection = kStronglyTaken;
    aFetch.theBPState->pc = aFetch.theAddress;
    aFetch.theBPState->theGShareShiftReg = 0;
    aFetch.theBPState->is_runahead = 0;
    aFetch.theBPState->detectedSpecialCall = false;
    aFetch.theBPState->callUpdatedRAS = false;
    aFetch.theBPState->haltDispatch = false;
    aFetch.theBPState->theTL = theTL;
    aFetch.theBPState->exceptionSource = xUnknown;
    aFetch.theBPState->BTBPreFilled = false;
    aFetch.theBPState->causedSquash = false;
    aFetch.theBPState->hasRetired = false;

    switch (aFetch.theBPState->thePredictedType) {
    case kNonBranch:
      assert(0);
      aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      break;
    case kConditional:
#ifdef TAGE
      aFetch.theBPState->thePredictedTarget =
          theTage_predictConditional((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
      if (aFetch.theBPState->bimodalPrediction) {
        assert(aFetch.theBPState->saturationCounter >= 0 &&
               aFetch.theBPState->saturationCounter < 4);
        (*bimodalConfindence[aFetch.theBPState->saturationCounter])++;
      } else {
        assert(aFetch.theBPState->saturationCounter >= 0 &&
               aFetch.theBPState->saturationCounter < 8);
        (*tageConfindence[aFetch.theBPState->saturationCounter])++;
      }
#else

      aFetch.theBPState->thePredictedTarget = predictConditional(aFetch.theAddress, aFetch);
#endif
      break;
    case kUnconditional:
      ++thePredictions;
      ++thePredictions_Unconditional;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }
#ifdef TAGE
      theTage.get_prediction((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
//            theTage.checkpoint_history(*(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kReturn:
      ++thePredictions;
      ++thePredictions_Returns;
      //        	DBG_( Tmp, ( << "predicting return " ));
      aFetch.theBPState->returnUsedRAS = false;
      aFetch.theBPState->returnPopRASTwice = false;
      /*if (theRAS.size() && theRASHelper.contains(aFetch.theAddress)) {
          DBG_( Tmp, ( << "poping extra " ));
          aFetch.theBPState->theAltPredictedTarget = theRAS.back();
          theRAS.pop_back();
          aFetch.theBPState->returnPopRASTwice = true;
      }*/
      if (enableRAS && theRAS.size()) {
        aFetch.theBPState->returnUsedRAS = true;
        aFetch.theBPState->thePredictedTarget = theRAS.back();
        theRAS.pop_back();
        /*if (aFetch.theBPState->returnPopRASTwice == false) {
                                if (theRAS.size()) {
                                        aFetch.theBPState->theAltPredictedTarget = theRAS.back();
  //						DBG_( Tmp, ( << "alt ret "  << theRAS.back()));
                                } else {
                                        aFetch.theBPState->theAltPredictedTarget =
  VirtualMemoryAddress(0);
                                }
        }*/
        //                printRAS();
      } else if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }

#ifdef TAGE
      theTage.get_prediction((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
//            theTage.checkpoint_history(*(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kCall:
      //        	DBG_( Tmp, ( << "predicting call " ));
      aFetch.theBPState->callUpdatedRAS = true;
      ++thePredictions;
      ++thePredictions_Unconditional;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }

      if (enableRAS) {
        if (enableTCE == false || theRASHelper.contains(aFetch.theAddress) == false) {
          theRAS.push_back(aFetch.theAddress + 4);
          //            	printRAS();
        } else {
          aFetch.theBPState->callUpdatedRAS = false;
        }
      }

#ifdef TAGE
      theTage.get_prediction((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
//            theTage.checkpoint_history(*(aFetch.theBPState));
#else

      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      // Need to push address onto retstack
      break;
      /* Difference between indirect jump w. subroutine link and non-link are set out in
       * ARM ISA Manual S6.2.30/6.2.31.
       * Reg-indirect does not have call-link.
       */
    case kIndirectCall:
      // this behaves like a call and will speculatively push the RAS
      ++thePredictions;
      ++thePredictions_Indirect;
      aFetch.theBPState->callUpdatedRAS = true;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }
      if (enableRAS) {
        if (enableTCE == false || theRASHelper.contains(aFetch.theAddress) == false) {
          theRAS.push_back(aFetch.theAddress + 4);
          //            	printRAS();
        } else {
          aFetch.theBPState->callUpdatedRAS = false;
        }
      }
#ifdef TAGE
      theTage.get_prediction((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
//            theTage.checkpoint_history(*(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kIndirectReg:
      // this does not push the RAS
      ++thePredictions;
      ++thePredictions_Indirect;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
      break;
    default:
      assert(0);
      aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      break;
    }

    if (aFetch.theBPState->thePredictedType != kNonBranch) {
      DBG_(Verb,
           (<< theIndex << "-BPRED-PREDICT: PC \t" << aFetch.theAddress << " serial "
            << aFetch.theBPState->theSerial << " Target \t" << aFetch.theBPState->thePredictedTarget
            << "\tType " << aFetch.theBPState->thePredictedType));
    }

    return aFetch.theBPState->thePredictedTarget;
  }

  BTBEntry access_BBTB(VirtualMemoryAddress anAddress) {
    return theBB_BTB.getEntry(anAddress);
  }

  void checkPointBPState(FetchAddr &aFetch) {
    aFetch.theBPState = boost::intrusive_ptr<BPredState>(new BPredState());
    aFetch.theBPState->thePredictedType = kNonBranch;
    aFetch.theBPState->thePrediction = kStronglyTaken;
    aFetch.theBPState->theActualDirection = kStronglyTaken;
    aFetch.theBPState->theActualType = kNonBranch;
    aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->pc = aFetch.theAddress;
    aFetch.theBPState->is_runahead = 0;
    aFetch.theBPState->detectedSpecialCall = false;
    aFetch.theBPState->callUpdatedRAS = false;
    aFetch.theBPState->theTL = theTL;
    aFetch.theBPState->haltDispatch = false;
    aFetch.theBPState->exceptionSource = xUnknown;
    aFetch.theBPState->BTBPreFilled = false;
    aFetch.theBPState->causedSquash = false;
    aFetch.theBPState->hasRetired = false;
#ifdef TAGE
    theTage.checkpoint_history(*(aFetch.theBPState));
#else
    aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
  }

  VirtualMemoryAddress predictBranch(FetchAddr &aFetch, BTBEntry aBTBEntry) {
    aFetch.theBPState = boost::intrusive_ptr<BPredState>(new BPredState());
    aFetch.theBPState->thePredictedType = aBTBEntry.theBranchType;
    aFetch.theBPState->theSerial = theSerial++;
    aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->theNextPredictedTarget = VirtualMemoryAddress(0);
    aFetch.theBPState->thePrediction = kStronglyTaken;
    aFetch.theBPState->theBimodalPrediction = kStronglyTaken;
    aFetch.theBPState->theMetaPrediction = kStronglyTaken;
    aFetch.theBPState->theGSharePrediction = kStronglyTaken;
    aFetch.theBPState->theActualDirection = kStronglyTaken;
    aFetch.theBPState->pc = aFetch.theAddress;
    aFetch.theBPState->theGShareShiftReg = 0;
    aFetch.theBPState->is_runahead = 0;
    aFetch.theBPState->bimodalPrediction = false;
    aFetch.theBPState->detectedSpecialCall = false;
    aFetch.theBPState->callUpdatedRAS = false;
    aFetch.theBPState->haltDispatch = false;
    aFetch.theBPState->theTL = theTL;
    aFetch.theBPState->exceptionSource = xUnknown;
    aFetch.theBPState->BTBPreFilled = false;
    aFetch.theBPState->causedSquash = false;
    aFetch.theBPState->hasRetired = false;

    switch (aFetch.theBPState->thePredictedType) {
    case kNonBranch:
      assert(0);
      break;
    case kConditional:
#ifdef TAGE
      theTage_predictConditional(aFetch.theAddress, *(aFetch.theBPState));
#else
      predictConditional(aFetch.theAddress, aFetch);
#endif
      if (aFetch.theBPState->bimodalPrediction ==
          false) { // The prediction was not from TAGE bimodal part, so it has precedence over BTB
                   // direction
        //				DBG_(Tmp, ( << "TAGE pred"));
        if (aFetch.theBPState->thePrediction <= kTaken) {
          aFetch.theBPState->thePredictedTarget = aBTBEntry.theTarget;
        } else {
          aFetch.theBPState->thePredictedTarget = aFetch.theAddress + 4;
        }
      } else {
        //				DBG_(Tmp, ( << "BTB Pred"));
        aFetch.theBPState->thePrediction = aBTBEntry.theBranchDirection;
        if (aBTBEntry.theBranchDirection <= kTaken) {
          aFetch.theBPState->thePredictedTarget = aBTBEntry.theTarget;
        } else {
          aFetch.theBPState->thePredictedTarget = aFetch.theAddress + 4;
        }
      }
      break;
    case kUnconditional:
      aFetch.theBPState->thePredictedTarget = aBTBEntry.theTarget;
#ifdef TAGE
      theTage.get_prediction(aFetch.theAddress, *(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kReturn:
      aFetch.theBPState->returnUsedRAS = false;
      aFetch.theBPState->returnPopRASTwice = false;
      if (enableRAS && theRAS.size()) {
        aFetch.theBPState->returnUsedRAS = true;
        aFetch.theBPState->thePredictedTarget = theRAS.back();
        theRAS.pop_back();
      } else {
        aFetch.theBPState->thePredictedTarget = aBTBEntry.theTarget;
      }

#ifdef TAGE
      theTage.get_prediction(aFetch.theAddress, *(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kCall:
      aFetch.theBPState->callUpdatedRAS = true;
      aFetch.theBPState->thePredictedTarget = aBTBEntry.theTarget;

      if (enableRAS) {
        if (enableTCE == false || aBTBEntry.isSpecialCall == false) {
          theRAS.push_back(VirtualMemoryAddress(aFetch.theAddress + 4));
          //            	printRAS();
        } else {
          aFetch.theBPState->callUpdatedRAS = false;
        }
      }

#ifdef TAGE
      theTage.get_prediction(aFetch.theAddress, *(aFetch.theBPState));
#else

      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
      /* Difference between indirect jump w. subroutine link and non-link are set out in
       * ARM ISA Manual S6.2.30/6.2.31.
       * Reg-indirect does not have call-link.
       */
    case kIndirectCall:
      // this behaves like a call and will speculatively push the RAS
      ++thePredictions;
      ++thePredictions_Indirect;
      aFetch.theBPState->callUpdatedRAS = true;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }
      // TODO: Ali: is has bugs! when you have a caller from different callees
      if (enableRAS) {
        if (enableTCE == false || theRASHelper.contains(aFetch.theAddress) == false) {
          theRAS.push_back(aFetch.theAddress + 4);
          //            	printRAS();
        } else {
          aFetch.theBPState->callUpdatedRAS = false;
        }
      }
#ifdef TAGE
      theTage.get_prediction((uint64_t)aFetch.theAddress, *(aFetch.theBPState));
//            theTage.checkpoint_history(*(aFetch.theBPState));
#else
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kIndirectReg:
      // this does not push the RAS
      ++thePredictions;
      ++thePredictions_Indirect;
      if (theBTB.target(aFetch.theAddress)) {
        aFetch.theBPState->thePredictedTarget = *theBTB.target(aFetch.theAddress);
      } else {
        aFetch.theBPState->thePredictedTarget = VirtualMemoryAddress(0);
      }
      aFetch.theBPState->theGShareShiftReg = theGShare.shiftReg();
      break;
    default:
      assert(0);
      break;
    }
    return aFetch.theBPState->thePredictedTarget;
  }

  void stats(BranchFeedback const &aFeedback, int flexusIndex) {
    if (!aFeedback.theBPState ||
        aFeedback.theActualType != aFeedback.theBPState->thePredictedType) {
      assert(aFeedback.theBPState);
      ++theMispredict;
      if (aFeedback.theBPState->thePredictedType == kNonBranch) {
        ++theMispredict_NewBranch;
        //        	  DBG_( Tmp, ( << "cpu " << flexusIndex << " pc " << aFeedback.thePC << "
        //        new branch "<< aFeedback.theActualType << " " << aFeedback.theBPState->
        //        thePredictedType));
        if (aFeedback.theBPState->translationFailed == true) {
          theMispredict_NewBranchTransFailed++;
        }
      } else if (aFeedback.theActualType == kNonBranch) {
        ++theMispredict_nonBRasBR;
        //        	  DBG_( Tmp, ( << "non branch as a branch "<< aFeedback.theActualType << " "
        //        << aFeedback.theBPState-> thePredictedType));
      } else {
        ++theMispredict_branchType;
        //        	  DBG_( Tmp, ( << "wrong branch type " << aFeedback.theActualType << " " <<
        //        aFeedback.theBPState-> thePredictedType));
      }
      DBG_(Verb, (<< "BPRED-RESOLVE Mispredict New-Branch " << aFeedback.theActualType << " @"
                  << aFeedback.thePC << " " << aFeedback.theActualDirection << " to "
                  << aFeedback.theActualTarget));
    } else {
      if (aFeedback.theActualType == kConditional) {
        if ((aFeedback.theBPState->thePrediction >= kNotTaken) &&
            (aFeedback.theActualDirection >= kNotTaken)) {
          ++theCorrect;
          ++theCorrect_Conditional;
          if (aFeedback.theBPState->theMetaPrediction >= kNotTaken) {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (GShare) " << aFeedback.theBPState->thePrediction
                        << " " << aFeedback.theActualType << " @" << aFeedback.thePC << " TAKEN to "
                        << aFeedback.theActualTarget));
            ++theCorrect_GShare;
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Bimodal) " << aFeedback.theBPState->thePrediction
                        << " " << aFeedback.theActualType << " @" << aFeedback.thePC << " TAKEN to "
                        << aFeedback.theActualTarget));
            ++theCorrect_Bimodal;
          }
        } else if ((aFeedback.theBPState->thePrediction <= kTaken) &&
                   (aFeedback.theActualDirection <= kTaken)) {
          if (aFeedback.theActualTarget == aFeedback.theBPState->thePredictedTarget) {
            ++theCorrect;
            ++theCorrect_Conditional;
            if (aFeedback.theBPState->theMetaPrediction >= kNotTaken) {
              DBG_(Verb,
                   (<< "BPRED-RESOLVE Correct (GShare) " << aFeedback.theBPState->thePrediction
                    << " " << aFeedback.theActualType << " @" << aFeedback.thePC << " NOT TAKEN"));
              ++theCorrect_GShare;
            } else {
              DBG_(Verb,
                   (<< "BPRED-RESOLVE Correct (Bimodal) " << aFeedback.theBPState->thePrediction
                    << " " << aFeedback.theActualType << " @" << aFeedback.thePC << " NOT TAKEN"));
              ++theCorrect_Bimodal;
            }
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Target) "
                        << aFeedback.theBPState->thePrediction << " " << aFeedback.theActualType
                        << " @" << aFeedback.thePC << " TAKEN to " << aFeedback.theActualTarget
                        << " predicted target " << aFeedback.theBPState->thePredictedTarget));
            ++theMispredict;
            ++theMispredict_Conditional;
            ++theMispredict_Target;
          }

        } else {
          ++theMispredict;
          ++theMispredict_Conditional;
          ++theMispredict_Direction;
          if ((aFeedback.theBPState->theBimodalPrediction >= kNotTaken) ==
              (aFeedback.theBPState->theGSharePrediction >= kNotTaken)) {
            DBG_(Verb,
                 (<< "BPRED-RESOLVE Mispredict (Direction) " << aFeedback.theBPState->thePrediction
                  << " " << aFeedback.theActualType << " @" << aFeedback.thePC << " "
                  << aFeedback.theActualDirection << " to " << aFeedback.theActualTarget));
          } else {
            ++theMispredict_Meta;
            if ((aFeedback.theBPState->thePrediction >= kNotTaken) ==
                (aFeedback.theBPState->theGSharePrediction >= kNotTaken)) {
              DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Meta:Gshare) "
                          << aFeedback.theBPState->thePrediction << " " << aFeedback.theActualType
                          << " @" << aFeedback.thePC << " " << aFeedback.theActualDirection
                          << " to " << aFeedback.theActualTarget));
              ++theMispredict_MetaGShare;
            } else {
              DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Meta:Bimod) "
                          << aFeedback.theBPState->thePrediction << " " << aFeedback.theActualType
                          << " @" << aFeedback.thePC << " " << aFeedback.theActualDirection
                          << " to " << aFeedback.theActualTarget));
              ++theMispredict_MetaBimod;
            }
          }
        } // end conditional mispredict
      } else if (aFeedback.theActualType == kIndirectCall ||
                 aFeedback.theActualType == kIndirectReg) { // MARK: Indirect
        if (aFeedback.theActualTarget == aFeedback.theBPState->thePredictedTarget) {
          theCorrect++;
          theCorrect_Indirect++;
          DBG_(Verb, (<< "BPRED-RESOLVE Correct (Indirect) " << aFeedback.theActualType << " @"
                      << aFeedback.thePC << " to " << aFeedback.theActualTarget));
        } else {
          theMispredict++;
          theMispredict_Target++;
          theMispredict_IndirectTargets++;
          DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Indirect-Target) " << aFeedback.theActualType
                      << "@" << aFeedback.thePC << " to " << aFeedback.theActualTarget
                      << " predicted target " << aFeedback.theBPState->thePredictedTarget));
        }
      } // end conditional or indirect call
      else {
        // Unconditional, call, and return
        bool target_correct =
            (aFeedback.theActualTarget == aFeedback.theBPState->thePredictedTarget);
        switch (aFeedback.theActualType) {
        case kReturn:
          if (target_correct) {
            theCorrect++;
            theCorrect_Returns++;
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Return) " << aFeedback.theActualType << " @"
                        << aFeedback.thePC << " to " << aFeedback.theActualTarget));
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Return-Target) " << aFeedback.theActualType
                        << "@" << aFeedback.thePC << " to " << aFeedback.theActualTarget
                        << " predicted target " << aFeedback.theBPState->thePredictedTarget));
            theMispredict++;
            theMispredict_Return++;
          }
          break;
        case kUnconditional:
        case kCall:
          if (target_correct) {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Unconditional) " << aFeedback.theActualType
                        << " @" << aFeedback.thePC << " to " << aFeedback.theActualTarget));
            ++theCorrect;
            ++theCorrect_Unconditional;
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Uncond-Target) " << aFeedback.theActualType
                        << "@" << aFeedback.thePC << " to " << aFeedback.theActualTarget
                        << " predicted target " << aFeedback.theBPState->thePredictedTarget));
            ++theMispredict;
            ++theMispredict_Target;
          }
        default:
          break;
        }
      } // end unconditional
    }   // end branch type == predicted branch type

    theBranches++;
    switch (aFeedback.theActualType) {
    case kConditional:
      theBranches_Conditional++;
      break;
    case kUnconditional:
      theBranches_Unconditional++;
      break;
    case kCall:
      theBranches_Call++;
      break;
    case kReturn:
      theBranches_Return++;
      break;
    case kIndirectReg:
      theBranches_RegIndirect++;
      break;
    case kIndirectCall:
      theBranches_IndirectCall++;
      break;
    default:
      break;
    }
  } // end stats fcn

  /*BTB update by prefill*/
  void updateBTB(VirtualMemoryAddress pc, eBranchType branchType, VirtualMemoryAddress target,
                 bool specialCall, int BBsize) {
    eDirection branchDirection = kNotTaken; // Fixme: Get the direction from base part of TAGE
    theBTB.update(pc, branchType, target, BBsize, branchDirection, branchDirection, specialCall);
    if (specialCall) {
      theRASHelper.update(pc, branchType, target, BBsize, branchDirection, branchDirection,
                          specialCall);
    }
    assert(0);
  }

  VirtualMemoryAddress getNextPrefetchAddr(VirtualMemoryAddress anAddress) {
    return anAddress + 64;
  }

  void updateBBTB(BTBEntry anEntry) {
    theBB_BTB.update(anEntry.thePC, anEntry.theBranchType, anEntry.theTarget, anEntry.theBBsize,
                     anEntry.theBranchDirection, anEntry.theBranchDirection, anEntry.isSpecialCall);
  }

  /*BTB update by backend*/
  void feedback(BranchFeedback &aFeedback, int flexusIndex) {
    DBG_(Iface, (<< " Feedback received for  " << aFeedback.thePC));
    DBG_(Iface,
         (<< " size " << aFeedback.theBBsize << " type " << aFeedback.theActualType << " direction "
          << aFeedback.theActualDirection << " target " << aFeedback.theActualTarget
          << " special call " << aFeedback.theBPState->detectedSpecialCall << " bpStatePC"
          << aFeedback.theBPState->pc));

    /*Get Tage prediction, it will be needed if the */
    aFeedback.theBPDirection = aFeedback.theActualDirection;
    theBB_BTB.update(aFeedback);

    aFeedback.thePC =
        aFeedback.theBPState
            ->pc; // Restore the PC to branch PC for conditional branch predictor updates
    stats(aFeedback, flexusIndex);
#ifdef TAGE
    uint64_t anAddress = aFeedback.thePC;
    bool taken = (aFeedback.theActualDirection <= kTaken);

    if (aFeedback.theBPState->thePredictedType == kConditional &&
        aFeedback.theActualType == kConditional) {
      theTage.update_predictor(anAddress, *(aFeedback.theBPState), taken);
    }

#else

    if (aFeedback.theActualType == kConditional) {
      if (is_new) {
        theBimodal.update(aFeedback.thePC, aFeedback.theActualDirection);
        theGShare.shiftIn(aFeedback.theActualDirection);
      } else if (aFeedback.theBPState) {

        // Update 2-bit counters
        // Get current counter values, being careful to use the shift register from before
        eDirection bimodal = theBimodal.direction(aFeedback.thePC);
        eDirection gshare =
            theGShare.direction(aFeedback.thePC, aFeedback.theBPState->theGShareShiftReg);
        eDirection meta = theMeta.direction(aFeedback.thePC);

        // Modify the tables, being careful to use the shift registers used to originally make the
        // prediction
        theBimodal.update(aFeedback.thePC, apply(aFeedback.theActualDirection, bimodal));
        theGShare.update(aFeedback.thePC, aFeedback.theBPState->theGShareShiftReg,
                         apply(aFeedback.theActualDirection, gshare));

        if ((gshare >= kNotTaken) != (bimodal >= kNotTaken)) {
          // Need to update meta
          if ((aFeedback.theActualDirection >= kNotTaken) == (gshare >= kNotTaken)) {
            // More gshare
            theMeta.update(aFeedback.thePC, moreNotTaken(meta));
          } else {
            // More bimodal
            theMeta.update(aFeedback.thePC, moreTaken(meta));
          }
        }
      } // end !is_new
    }

    if (aFeedback.theBPState) {
      DBG_(Iface,
           (<< theIndex << "-BPRED-FEEDBACK: PC \t" << aFeedback.thePC << " serial "
            << aFeedback.theBPState->theSerial << " Target \t" << aFeedback.theActualTarget
            << "\tType " << aFeedback.theActualType << " dir " << aFeedback.theActualDirection
            << " pred " << aFeedback.theBPState->thePrediction));
    } else {
      DBG_(Iface, (<< theIndex << "-BPRED-FEEDBACK: PC \t" << aFeedback.thePC
                            << " Target \t" << aFeedback.theActualTarget << "\tType "
                            << aFeedback.theActualType << " dir " << aFeedback.theActualDirection));
    }

    DBG_(Verb, (<< "Leaving feedback."));
#endif
  } // namespace SharedTypes

  void saveStateBinary(std::string const &aDirName) const {
    std::string fname(aDirName);
    fname += "/bpred-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << theBTB;
    oa << theBimodal;
    oa << theMeta;
    oa << theGShare;
    oa << theRASHelper;

    // close archive
    ofs.close();
  }

  void saveState(std::string const &aDirName) const {
    std::cout << std::endl
              << std::endl
              << std::endl
              << std::endl
              << " Save state from Timing" << std::endl
              << std::endl
              << std::endl;
    std::string fname(aDirName);
    fname += "/bpredtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ofstream ofs(fname.c_str());
    boost::archive::text_oarchive oa(ofs);

    oa << theBTB;
    oa << theBimodal;
    oa << theMeta;
    oa << theGShare;
    oa << theRASHelper;

    // close archive
    ofs.close();
  }

  void loadStateBinary(std::string const &aDirName) {
    std::string fname(aDirName);
    fname += "/bpred-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (ifs.good()) {
      try {
        boost::archive::binary_iarchive ia(ifs);

        ia >> theBTB;
        ia >> theBimodal;
        ia >> theMeta;
        ia >> theGShare;
        ia >> theRASHelper;
        DBG_(Dev, (<< theName << " loaded branch predictor.  BTB size: " << theBTB.theBTBSets
                   << " by " << theBTB.theBTBAssoc << " Bimodal size: " << theBimodal.theSize
                   << " Meta size: " << theMeta.theSize
                   << " Gshare size: " << theGShare.theShiftRegSize));

        ifs.close();
      } catch (...) {
        DBG_Assert(false, (<< "Unable to load bpred state"));
      }
    } else {
      DBG_(Dev, (<< "Unable to load bpred state " << fname << ". Using default state."));
    }
  }

  void loadState(std::string const &aDirName) {
    std::cout << std::endl
              << std::endl
              << std::endl
              << std::endl
              << " Load state in Timing" << std::endl
              << std::endl
              << std::endl;
    std::string fname(aDirName);
    fname += "/bpredtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
    DBG_(Dev, (<< "Going to load bpred state " << fname));
    std::ifstream ifs(fname.c_str());
    if (ifs.good()) {
      boost::archive::text_iarchive ia(ifs);

      DBG_(Dev, (<< "Going to load bpred state1 " << fname));
      ia >> theBTB;
      DBG_(Dev, (<< "Going to load bpred state2 " << fname));
      ia >> theBimodal;
      DBG_(Dev, (<< "Going to load bpred state3 " << fname));
      ia >> theMeta;
      DBG_(Dev, (<< "Going to load bpred state4 " << fname));
      ia >> theGShare;
      DBG_(Dev, (<< "Going to load bpred state5 " << fname));
      ia >> theRASHelper;
      DBG_(Dev, (<< "Going to load bpred state6 " << fname));
      DBG_(Dev,
           (<< theName << " loaded branch predictor.  BTB size: " << theBTB.theBTBSets << " by "
            << theBTB.theBTBAssoc << " Bimodal size: " << theBimodal.theSize
            << " Meta size: " << theMeta.theSize << " Gshare size: " << theGShare.theShiftRegSize));

      ifs.close();
    } else {
      loadStateBinary(aDirName);
    }

    {
      std::string fname(aDirName);
      fname += "/BBTBtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
      DBG_(Dev, (<< "Going to load bpred state " << fname));
      std::ifstream ifs(fname.c_str());
      DBG_(Dev, (<< "Going to load bpred state1 " << fname));
      if (ifs.good()) {
        DBG_(Dev, (<< "Going to load bpred state2 " << fname));
        boost::archive::text_iarchive ia(ifs);

        DBG_(Dev, (<< "Going to load bpred state3 " << fname));
        ia >> theBB_BTB;
        DBG_(Dev, (<< "Going to load bpred state4 " << fname));
        DBG_(Dev, (<< theName << " loaded branch predictor.  BTB size: " << theBB_BTB.theBTBSets
                   << " by " << theBB_BTB.theBTBAssoc));

        ifs.close();
      } else {
        DBG_(Dev, (<< "Unable to load bpred state " << fname << ". Using default state."));
        assert(0);
      }
    }
#ifdef TAGE
    {

      std::string fname(aDirName);
      fname += "/tage-" + boost::padded_string_cast<2, '0'>(theIndex);

      std::ios_base::openmode omode = std::ios::in;

      std::ifstream ifs(fname.c_str(), omode);
      if (!ifs.good()) {
        DBG_(Dev,
             (<< " saved checkpoint state " << fname << " not found.  Resetting to empty cache. "));
      } else {
        boost::iostreams::filtering_stream<boost::iostreams::input> in;
        in.push(ifs);
        theTage.loadState(in);
      }
    }
#endif
  }

}; // namespace Flexus

struct FastCombiningImpl : public FastBranchPredictor {

  std::string theName;
  uint32_t theIndex;
  uint32_t theSerial;
  BTB theBTB;
  std::list<VirtualMemoryAddress> theRAS;

  Bimodal theBimodal;
  Bimodal theMeta;
  gShare theGShare;
  PREDICTOR theTage;
  uint64_t branhces;
  uint64_t correct;
  uint64_t mispredict;
  BTB theRASHelper;
  BTB theBB_BTB;

  // MARK: Add stats for indirect branch types
  Stat::StatCounter theBranches; // Retired Branches
  Stat::StatCounter theBranches_Unconditional;
  Stat::StatCounter theBranches_Conditional;
  Stat::StatCounter theBranches_Call;
  Stat::StatCounter theBranches_IndirectCall;
  Stat::StatCounter theBranches_RegIndirect;
  Stat::StatCounter theBranches_Return;

  Stat::StatCounter thePredictions;
  Stat::StatCounter thePredictions_Bimodal;
  Stat::StatCounter thePredictions_GShare;
  Stat::StatCounter thePredictions_Unconditional;
  Stat::StatCounter thePredictions_Indirect;
  Stat::StatCounter thePredictions_Returns;
  Stat::StatCounter thePredictions_Calls;

  Stat::StatCounter theCorrect;
  Stat::StatCounter theCorrect_Direction;
  Stat::StatCounter theCorrect_Bimodal;
  Stat::StatCounter theCorrect_GShare;
  Stat::StatCounter theCorrect_Unconditional;
  Stat::StatCounter theCorrect_Indirect;
  Stat::StatCounter theCorrect_Returns;

  Stat::StatCounter theMispredict;
  Stat::StatCounter theMispredict_NewBranch;
  Stat::StatCounter theMispredict_Direction;
  Stat::StatCounter theMispredict_Meta;
  Stat::StatCounter theMispredict_MetaGShare;
  Stat::StatCounter theMispredict_MetaBimod;
  Stat::StatCounter theMispredict_Target;
  Stat::StatCounter theMispredict_Unconditional;
  Stat::StatCounter theMispredict_IndirectTargets;
  Stat::StatCounter theMispredict_Returns;

  Stat::StatCounter theFalseNegBranches;
  Stat::StatCounter theFalsePosBranches;
  Stat::StatCounter theTrueBranches;
  Stat::StatCounter theTrueNonBranches;
  Stat::StatCounter theFalseNegReturns;

  /*Stat::StatCounter BTBMisses;
  Stat::StatCounter BTBHits;
  Stat::StatCounter theInstructions;
  Stat::StatCounter BTBSkips;
  */

  FastCombiningImpl(std::string const &aName, uint32_t anIndex)
      : theName(aName), theIndex(anIndex), theSerial(0), theBTB(512, 4), theBimodal(16384),
        theMeta(16384), theGShare(14), theRASHelper(512, 4), theBB_BTB(512, 4),
        theBranches(aName + "-branches"),
        theBranches_Unconditional(aName + "-branches:unconditional"),
        theBranches_Conditional(aName + "-branches:conditional"),
        theBranches_Call(aName + "-branches:call"),
        theBranches_IndirectCall(aName + "-branches:indirect:call"),
        theBranches_RegIndirect(aName + "-branches:indirect:reg"),
        theBranches_Return(aName + "-branches:return"), 
        thePredictions(aName + "-predictions"),
        thePredictions_Bimodal(aName + "-predictions:bimodal"),
        thePredictions_GShare(aName + "-predictions:gshare"),
        thePredictions_Unconditional(aName + "-predictions:unconditional"),
        thePredictions_Indirect(aName + "-predictions:indirect"),
        thePredictions_Returns(aName + "-predictions:returns"), 
        thePredictions_Calls(aName + "-predictions:calls"),
        theCorrect(aName + "-correct"),
        theCorrect_Direction(aName + "-correct:direction"),
        theCorrect_Bimodal(aName + "-correct:bimodal"),
        theCorrect_GShare(aName + "-correct:gshare"),
        theCorrect_Unconditional(aName + "-correct:unconditional"),
        theCorrect_Indirect(aName + "-correct:indirect"),
        theCorrect_Returns(aName + "-correct:returns"), theMispredict(aName + "-mispredict"),
        theMispredict_NewBranch(aName + "-mispredict:new"),
        theMispredict_Direction(aName + "-mispredict:direction"),
        theMispredict_Meta(aName + "-mispredict:meta"),
        theMispredict_MetaGShare(aName + "-mispredict:meta:chose_gshare"),
        theMispredict_MetaBimod(aName + "-mispredict:meta:chose_bimod"),
        theMispredict_Target(aName + "-mispredict:target"),
        theMispredict_Unconditional(aName + "-mispredict:unconditional"),
        theMispredict_IndirectTargets(aName + "-mispredict:target:indirect"),
        theMispredict_Returns(aName + "-mispredict:returns"),
        theFalseNegBranches(aName + "-mispredict:falseNeg"),
        theFalsePosBranches(aName + "-mispredict:falsePos"),
        theTrueBranches(aName + "-correct:trueBranches"),
        theTrueNonBranches(aName + "-correct:trueNonBranches"),
        theFalseNegReturns(aName + "-mispredict:falseNegReturns") {
        //BTBMisses(aName + "-btb:misses"),
        //BTBHits(aName + "-btb:hits"),
        //theInstructions(aName + "-instructions"),
        //BTBSkips(aName + "-btb:skips") {
  }

  FastCombiningImpl(std::string const &aName, uint32_t anIndex, uint32_t aNumBTBSets,
                    uint32_t aNumBTBWays)
      : theName(aName), theIndex(anIndex), theSerial(0), theBTB(aNumBTBSets, aNumBTBWays),
        theBimodal(16384), theMeta(16384), theGShare(14), theRASHelper(aNumBTBSets, aNumBTBWays),
        theBB_BTB(aNumBTBSets, aNumBTBWays), theBranches(aName + "-branches"),
        theBranches_Unconditional(aName + "-branches:unconditional"),
        theBranches_Conditional(aName + "-branches:conditional"),
        theBranches_Call(aName + "-branches:call"),
        theBranches_IndirectCall(aName + "-branches:indirect:call"),
        theBranches_RegIndirect(aName + "-branches:indirect:reg"),
        theBranches_Return(aName + "-branches:return"), 
        thePredictions(aName + "-predictions"),
        thePredictions_Bimodal(aName + "-predictions:bimodal"),
        thePredictions_GShare(aName + "-predictions:gshare"),
        thePredictions_Unconditional(aName + "-predictions:unconditional"),
        thePredictions_Indirect(aName + "-predictions:indirect"),
        thePredictions_Returns(aName + "-predictions:returns"),
        thePredictions_Calls(aName + "-predictions:calls"),
        theCorrect(aName + "-correct"),
        theCorrect_Direction(aName + "-correct:direction"),
        theCorrect_Bimodal(aName + "-correct:bimodal"),
        theCorrect_GShare(aName + "-correct:gshare"),
        theCorrect_Unconditional(aName + "-correct:unconditional"),
        theCorrect_Indirect(aName + "-correct:indirect"),
        theCorrect_Returns(aName + "-correct:returns"), theMispredict(aName + "-mispredict"),
        theMispredict_NewBranch(aName + "-mispredict:new"),
        theMispredict_Direction(aName + "-mispredict:direction"),
        theMispredict_Meta(aName + "-mispredict:meta"),
        theMispredict_MetaGShare(aName + "-mispredict:meta:chose_gshare"),
        theMispredict_MetaBimod(aName + "-mispredict:meta:chose_bimod"),
        theMispredict_Target(aName + "-mispredict:target"),
        theMispredict_Unconditional(aName + "-mispredict:unconditional"),
        theMispredict_IndirectTargets(aName + "-mispredict:target:indirect"),
        theMispredict_Returns(aName + "-mispredict:returns"),
        theFalseNegBranches(aName + "-mispredict:falseNeg"),
        theFalsePosBranches(aName + "-mispredict:falsePos"),
        theTrueBranches(aName + "-correct:trueBranches"),
        theTrueNonBranches(aName + "-correct:trueNonBranches"),
        theFalseNegReturns(aName + "-mispredict:falseNegReturns"){
        //BTBMisses(aName + "-btb:misses"),
        //BTBHits(aName + "-btb:hits"),
        //theInstructions(aName + "-instructions"),
        //BTBSkips(aName + "-btb:skips") {
  }

  void printRAS() {
    //DBG_(Tmp, (<< "RAS is "));
    //for (std::list<VirtualMemoryAddress>::iterator it = theRAS.begin(); it != theRAS.end(); it++) {
    //  DBG_(Tmp, (<< "pc " << *it));
    //}
    // TODO: remove it later
    std::cout << "RAS is:\n";
    for (std::list<VirtualMemoryAddress>::iterator it = theRAS.begin(); it != theRAS.end(); it++) {
      std::cout << "\tpc " << *it << "\n";
    }
  }

  void increaseInstCount(void) {
    // theInstructions++;
  }

  // trace
  VirtualMemoryAddress theTage_predictConditional(VirtualMemoryAddress anAddress,
                                                  BPredState &aBPState) {
    bool isTaken = theTage.get_prediction((uint64_t)anAddress, aBPState);
    if (isTaken) {
      aBPState.thePrediction = kTaken;
    } else {
      aBPState.thePrediction = kNotTaken;
    }

    if (aBPState.thePrediction <= kTaken && theBTB.target(anAddress)) {
      return *theBTB.target(anAddress);
    } else {
      return VirtualMemoryAddress(0);
    }
  }

  eDirection conditionalTaken(VirtualMemoryAddress anAddress) {
#ifdef TAGE
    return theTage.isCondTaken((uint64_t)anAddress);
#else
    eDirection bimodal = theBimodal.direction(anAddress);
    eDirection gshare = theGShare.direction(anAddress);
    eDirection meta = theMeta.direction(anAddress);
    eDirection prediction;

    // Decide which predictor to believe
    if (meta >= kNotTaken) {
      prediction = gshare;
    } else {
      prediction = bimodal;
    }

    return prediction;
#endif
  }

  VirtualMemoryAddress predictRunaheadConditional(VirtualMemoryAddress anAddress,
                                                  BPredState &aBPState) {
    eDirection bimodal = theBimodal.direction(anAddress);
    eDirection gshare = theGShare.runaheadDirection(anAddress);
    eDirection meta = theMeta.direction(anAddress);
    eDirection prediction;

    // Decide which predictor to believe
    if (meta >= kNotTaken) {
      prediction = gshare;
    } else {
      prediction = bimodal;
    }

    aBPState.thePrediction = prediction;
    //          DBG_(Tmp, ( << "Predictrunahead: " << bimodal << " index " <<
    //          theBimodal.index(anAddress)));

    // Record the predictor state/predictions. Not used in runahead, so commented
    /*aBPState.thePrediction = prediction;
    aBPState.theBimodalPrediction = bimodal;
    aBPState.theMetaPrediction = meta;
    aBPState.theGSharePrediction = gshare;
    aBPState.theGShareShiftReg = theGShare.shiftReg();*/

    // Speculatively update the shift register with the prediction from the
    // gshare predictor.
    // TODO: evaluate whether it's better to shift in the
    // overall prediction or just the gshare part.
    theGShare.runaheadShiftIn(gshare);

    DBG_(Verb, (<< theIndex << "-BPRED-COND:  " << anAddress << " BIMOD " << bimodal << " GSHARE "
                << gshare << " META " << meta << " OVERALL " << prediction));

    if (prediction <= kTaken && theBTB.target(anAddress)) {
      return *theBTB.target(anAddress);
    } else {
      return VirtualMemoryAddress(0);
    }
  }

  VirtualMemoryAddress predictConditional(VirtualMemoryAddress anAddress, BPredState &aBPState) {
    eDirection bimodal = theBimodal.direction(anAddress);
    eDirection gshare = theGShare.direction(anAddress);
    eDirection meta = theMeta.direction(anAddress);
    eDirection prediction;

    // Decide which predictor to believe
    if (meta >= kNotTaken) {
      ++thePredictions;
      ++thePredictions_GShare;
      prediction = gshare;
    } else {
      ++thePredictions;
      ++thePredictions_Bimodal;
      prediction = bimodal;
    }
    //          DBG_(Tmp, ( << "Predictfetch: " << bimodal << " index " <<
    //          theBimodal.index(anAddress)));
    //    	std::cout << "Prediction for " << &std::hex << anAddress << " Gsindex " <<
    //    theGShare.index(anAddress) << " pred "<< (gshare)  << " Bindex " <<
    //    theBimodal.index(anAddress) << " pred "<< (bimodal) << " Mindex "<<
    //    theMeta.index(anAddress) << " pred "<< (meta) << " history "<< theGShare.shiftReg() <<
    //    std::endl;

    // Record the predictor state/predictions
    aBPState.thePrediction = prediction;
    aBPState.theBimodalPrediction = bimodal;
    aBPState.theMetaPrediction = meta;
    aBPState.theGSharePrediction = gshare;
    aBPState.theGShareShiftReg = theGShare.shiftReg();

    // Speculatively update the shift register with the prediction from the
    // gshare predictor.
    // TODO: evaluate whether it's better to shift in the
    // overall prediction or just the gshare part.
    theGShare.shiftIn(gshare);

    //        std::cout << "history new " << &std::hex << theGShare.shiftReg() <<std::endl;
    DBG_(Verb, (<< theIndex << "-BPRED-COND:  " << anAddress << " BIMOD " << bimodal << " GSHARE "
                << gshare << " META " << meta << " OVERALL " << prediction));

    if (prediction <= kTaken && theBTB.target(anAddress)) {
      return *theBTB.target(anAddress);
    } else {
      return VirtualMemoryAddress(0);
    }
  }

  // unused function
  void reset_runahead_history() {
#ifdef TAGE
    theTage.reset_runahead_history();
#else
    theGShare.resetRunaheadHistory();
#endif
  }

  // unused
  void runahead_predict(VirtualMemoryAddress anAddress, BPredState &aBPState) {
    //    	  DBG_(Tmp, ( << "Rprediction for " << std::hex << anAddress));
    aBPState.thePredictedType = theBTB.type(anAddress);
    aBPState.thePredictedTarget = VirtualMemoryAddress(0);
    aBPState.thePrediction = kStronglyTaken;

    switch (aBPState.thePredictedType) {
    case kNonBranch:
      break;
    case kConditional:
#ifdef TAGE
      aBPState.thePredictedTarget =
          theTage_predictConditional(VirtualMemoryAddress(anAddress), aBPState);
#else
      aBPState.thePredictedTarget =
          predictRunaheadConditional(VirtualMemoryAddress(anAddress), aBPState);
#endif
      break;
    case kUnconditional:
    case kCall:
    case kReturn:
      if (theBTB.target(anAddress)) {
        aBPState.thePredictedTarget = *theBTB.target(anAddress);
      } else {
        aBPState.thePredictedTarget = VirtualMemoryAddress(0);
      }
#ifdef TAGE
      theTage.get_prediction((uint64_t)anAddress, aBPState);
#endif
      break;
    default:
      assert(0);
      aBPState.thePredictedTarget = VirtualMemoryAddress(0);
      break;
    }
  }

  // unused
  void predict_BBTB(VirtualMemoryAddress anAddress, BPredState &aBPState) {
    BTBEntry anEntry = theBB_BTB.getEntry(anAddress);
    if (anEntry.thePC == 0) {
      aBPState.pc = VirtualMemoryAddress(0);
      //    		  DBG_(Tmp, ( << "BTB entry NOT found for " << &std::hex << anAddress));
    } else {
      aBPState.pc = anAddress;
      aBPState.thePredictedType = anEntry.theBranchType;
      aBPState.theNextPredictedTarget = anAddress + ((anEntry.theBBsize - 1) * 4);
      if (anEntry.theBranchDirection <= kTaken) {
        aBPState.thePredictedTarget = anEntry.theTarget;
      } else {
        aBPState.thePredictedTarget = anAddress + (anEntry.theBBsize * 4);
      }
      //    		  DBG_(Tmp, ( << "Target " << &std::hex << anEntry.theTarget << " size " <<
      //    anEntry.theBBsize << " direction " << anEntry.theBranchDirection << " type " <<
      //    anEntry.theBranchType));
    }
  }

  void predict(VirtualMemoryAddress anAddress, BPredState &aBPState) {
    //    	DBG_(Tmp, ( << "Predict for " << &std::hex << anAddress));
    aBPState.pc = anAddress;
    aBPState.thePredictedType = theBTB.type(anAddress);
    aBPState.theSerial = theSerial++;
    aBPState.thePredictedTarget = VirtualMemoryAddress(0);
    aBPState.thePrediction = kStronglyTaken;
    aBPState.callUpdatedRAS = false;
    aBPState.detectedSpecialCall = false;
#ifndef TAGE
    aBPState.theBimodalPrediction = kStronglyTaken;
    aBPState.theMetaPrediction = kStronglyTaken;
    aBPState.theGSharePrediction = kStronglyTaken;
    aBPState.theGShareShiftReg = 0;
#endif

    switch (aBPState.thePredictedType) {
    case kNonBranch:
#ifdef TAGE
      theTage.checkpoint_history(aBPState);
#else
      aBPState.theGShareShiftReg = theGShare.shiftReg(); // Rak added
#endif
      aBPState.thePredictedTarget = VirtualMemoryAddress(0);
      break;
    case kConditional:
#ifdef TAGE
      aBPState.thePredictedTarget =
          theTage_predictConditional(VirtualMemoryAddress(anAddress), aBPState);
#else
      aBPState.thePredictedTarget = predictConditional(VirtualMemoryAddress(anAddress), aBPState);
#endif
      break;
    case kReturn:
      ++thePredictions;
      ++thePredictions_Returns;
      
      if(theRAS.size()){
        aBPState.thePredictedTarget = theRAS.back();
        std::cout << "theRAS gives: " << aBPState.thePredictedTarget << " length: " << theRAS.size() << "\n";
        theRAS.pop_back();
      }
      else if (theBTB.target(anAddress)) {
        aBPState.thePredictedTarget = *theBTB.target(anAddress);
      } 
      else {
        // it is a BTB hit and has to have a non-zero target
        assert(0);
      }
      
#ifdef TAGE
      //            theTage.checkpoint_history(aBPState);
      theTage.get_prediction((uint64_t)anAddress, aBPState);
#else
      aBPState.theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    case kCall:
    case kIndirectCall:
      aBPState.callUpdatedRAS = true;
      //aBPState.detectedSpecialCall == true;
      ++thePredictions;
      ++thePredictions_Calls;

      // Here, we assume that we don't have an indirect target predictor
      // If we have, for indirect branches, the target has to be read from 
      // the indirect target predictor instead of the BTB
      if (theBTB.target(anAddress)) {
        aBPState.thePredictedTarget = *theBTB.target(anAddress);
      } else {
        assert(0);
      }     
      theRAS.push_back(anAddress + 4);
      /*if (theRASHelper.contains(anAddress) == false) {
        //            	DBG_( Tmp, ( << " Predicting normal call: "));
        theRAS.push_back(anAddress + 4);
      } else {
        //            	DBG_( Tmp, ( << " Predicting special call: "));
        aBPState.callUpdatedRAS = false;
      }*/

#ifdef TAGE
      //            theTage.checkpoint_history(aBPState);
      theTage.get_prediction((uint64_t)anAddress, aBPState);
#else
      aBPState.theGShareShiftReg = theGShare.shiftReg();
#endif
      // Need to push address onto retstack
      break;
    case kUnconditional:
    case kIndirectReg:
      // Here, we assume that we don't have an indirect target predictor
      // If we have, for indirect branches, the target has to be read from 
      // the indirect target predictor instead of the BTB
      if (theBTB.target(anAddress)) {
        aBPState.thePredictedTarget = *theBTB.target(anAddress);
      } else {
        assert(0);
      }  
      
#ifdef TAGE
      //            theTage.checkpoint_history(aBPState);
      theTage.get_prediction((uint64_t)anAddress, aBPState);
#else
      aBPState.theGShareShiftReg = theGShare.shiftReg();
#endif
      break;
    
    case kLastBranchType:
      // TODO: these branch types are not implemented yet!
      assert(0);
      break;
    default:
      aBPState.thePredictedTarget = VirtualMemoryAddress(0);
      assert(0);
      break;
    }

    if (aBPState.thePredictedType != kNonBranch) {
      DBG_(Verb, (<< theIndex << "-BPRED-PREDICT: PC \t" << anAddress << " serial "
                  << aBPState.theSerial << " Target \t" << aBPState.thePredictedTarget << "\tType "
                  << aBPState.thePredictedType));
    }
  } // namespace Flexus

  bool stats(VirtualMemoryAddress anAddress, eBranchType anActualType, eDirection anActualDirection,
             VirtualMemoryAddress anActualTarget, BPredState &aBPState) {
    bool is_mispredict = false;
    if (anActualType != aBPState.thePredictedType) {
      if (anActualType == kNonBranch){
        ++theFalsePosBranches;
        assert(0);
      }
      else if (aBPState.thePredictedType == kNonBranch) {
        ++theFalseNegBranches;
        if (anActualType == kReturn)
          ++theFalseNegReturns;
      }
      else{
        std::cout << "addr: " << anAddress << "\n";
        fflush(stdout);
        assert(0);
      }

      is_mispredict = true;
      ++theMispredict;
      ++theMispredict_NewBranch;
      DBG_(Verb, (<< "BPRED-RESOLVE Mispredict New-Branch " << anActualType << " @" << anAddress
                  << " " << anActualDirection << " to " << anActualTarget));
    } 
    else {
      if (anActualType == kNonBranch)
        ++theTrueNonBranches;
      else if (anActualType != kNonBranch)
        ++theTrueBranches;

      // check if direction prediction is correct 
      if (anActualType == kConditional) {
        if ((aBPState.thePrediction >= kNotTaken) && (anActualDirection >= kNotTaken)) {
          ++theCorrect;
          //correct++;
          ++theCorrect_Direction;
          // TODO: add stats for TAGE
          // TODO: add ifndef to comment out gshare code
#ifndef TAGE
          if (aBPState.theMetaPrediction >= kNotTaken) {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (GShare) " << aBPState.thePrediction << " "
                        << anActualType << " @" << anAddress << " TAKEN to " << anActualTarget));
            ++theCorrect_GShare;
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Bimodal) " << aBPState.thePrediction << " "
                        << anActualType << " @" << anAddress << " TAKEN to " << anActualTarget));
            ++theCorrect_Bimodal;
          }
#endif
        } 
        else if ((aBPState.thePrediction <= kTaken) && (anActualDirection <= kTaken)) {
          ++theCorrect_Direction;
          if (anActualTarget == aBPState.thePredictedTarget) {
            ++theCorrect;
            //correct++;
            // TODO: add stats for TAGE
            if (aBPState.theMetaPrediction >= kNotTaken) {
              DBG_(Verb, (<< "BPRED-RESOLVE Correct (GShare) " << aBPState.thePrediction << " "
                          << anActualType << " @" << anAddress << " NOT TAKEN"));
              ++theCorrect_GShare;
            } else {
              DBG_(Verb, (<< "BPRED-RESOLVE Correct (Bimodal) " << aBPState.thePrediction << " "
                          << anActualType << " @" << anAddress << " NOT TAKEN"));
              ++theCorrect_Bimodal;
            }
          } 
          else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Target) " << aBPState.thePrediction << " "
                        << anActualType << " @" << anAddress << " TAKEN to " << anActualTarget
                        << " predicted target " << aBPState.thePredictedTarget));
            is_mispredict = true;
            ++theMispredict;
            ++theMispredict_Target;
          }
        } 
        else {
          is_mispredict = true;
          ++theMispredict;
          ++theMispredict_Direction;
          // TODO: add stats for TAGE
          if ((aBPState.theBimodalPrediction >= kNotTaken) ==
              (aBPState.theGSharePrediction >= kNotTaken)) {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Direction) " << aBPState.thePrediction << " "
                        << anActualType << " @" << anAddress << " " << anActualDirection << " to "
                        << anActualTarget));
          } else {
            ++theMispredict_Meta;
            if ((aBPState.thePrediction >= kNotTaken) ==
                (aBPState.theGSharePrediction >= kNotTaken)) {
              DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Meta:Gshare) " << aBPState.thePrediction
                          << " " << anActualType << " @" << anAddress << " " << anActualDirection
                          << " to " << anActualTarget));
              ++theMispredict_MetaGShare;
            } else {
              DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Meta:Bimod) " << aBPState.thePrediction
                          << " " << anActualType << " @" << anAddress << " " << anActualDirection
                          << " to " << anActualTarget));
              ++theMispredict_MetaBimod;
            }
          }
        }
      } else if (anActualType == kIndirectCall || anActualType == kIndirectReg) { // MARK: Indirect
        if (anActualTarget == aBPState.thePredictedTarget) {
          theCorrect++;
          theCorrect_Indirect++;
          DBG_(Verb, (<< "BPRED-RESOLVE Correct (Indirect) " << anActualType << " @" << anAddress
                      << " to " << anActualTarget));
        } else {
          theMispredict++;
          theMispredict_Target++;
          theMispredict_IndirectTargets++;
          DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Indirect-Target) " << anActualType << " @"
                      << anAddress << " to " << anActualTarget << " predicted target "
                      << aBPState.thePredictedTarget));
        }
      } else {
        // Unconditional, call, return
        bool target_correct = (anActualTarget == aBPState.thePredictedTarget);
        switch (anActualType) {
        case kReturn:
          if (target_correct) {
            theCorrect++;
            theCorrect_Returns++;
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Return) " << anActualType << " @" << anAddress
                        << " to " << anActualTarget));
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Return-Target) " << anActualType << " @"
                        << anAddress << " to " << anActualTarget << " predicted target "
                        << aBPState.thePredictedTarget));
            theMispredict++;
            theMispredict_Returns++;
          }
          break;
        case kUnconditional:
        case kCall:
          if (target_correct) {
            DBG_(Verb, (<< "BPRED-RESOLVE Correct (Unconditional) " << anActualType << " @"
                        << anAddress << " to " << anActualTarget));
            ++theCorrect;
            ++theCorrect_Unconditional;
          } else {
            DBG_(Verb, (<< "BPRED-RESOLVE Mispredict (Uncond-Target) " << anActualType << " @"
                        << anAddress << " to " << anActualTarget << " predicted target "
                        << aBPState.thePredictedTarget));
            ++theMispredict;
            ++theMispredict_Target;
          }
        default:
          break;
        } // end switch
      }   // end else { } condition -> unconditional branch types
    }     // end else { } condition -> not a new branch

    switch (anActualType) {
    case kConditional:
      theBranches++;
      theBranches_Conditional++;
      break;
    case kUnconditional:
      theBranches++;
      theBranches_Unconditional++;
      break;
    case kCall:
      theBranches++;
      theBranches_Call++;
      break;
    case kReturn:
      theBranches++;
      theBranches_Return++;
      break;
    case kIndirectReg:
      theBranches++;
      theBranches_RegIndirect++;
      break;
    case kIndirectCall:
      theBranches++;
      theBranches_IndirectCall++;
      break;
    default:
      break;
    }
    return is_mispredict;
  }

  void reconstructTageHistory(BPredState aBPState) {
    assert(aBPState.theActualType != kNonBranch);
    //    	  DBG_ (Tmp, ( << " Predicted direction "<< aBPState->thePrediction << " Actual
    //    Direction:  " << aBPState->theActualDirection << " pc " << aBPState->pc));
    theTage.restore_all_state(aBPState);
    if (aBPState.theActualType == kConditional) {
      if (aBPState.theActualDirection == kTaken) {
        theTage.update_history(aBPState, true, aBPState.pc);
      } else if (aBPState.theActualDirection == kNotTaken) {
        theTage.update_history(aBPState, false, aBPState.pc);
      } else {
        DBG_Assert(false, (<< "Should never enter here"));
      }
    } else {
      theTage.update_history(aBPState, true, aBPState.pc);
    }
  }

  // unused
  void updateBB_BTB(VirtualMemoryAddress anAddress, eBranchType anActualType,
                    eDirection anActualDirection, VirtualMemoryAddress anActualTarget,
                    BPredState &aBPState, int aBBsize) {
    //    	DBG_(Tmp, ( << "Feedback for " << &std::hex << anAddress));

    theBB_BTB.update(anAddress, anActualType, anActualTarget, aBBsize, anActualDirection,
                     anActualDirection, aBPState.detectedSpecialCall);

    if (aBPState.detectedSpecialCall) {
      assert(anActualType == kCall || anActualType == kIndirectReg ||
             anActualType == kIndirectCall);
      //			  DBG_(Tmp, ( << "Adding special Call " << &std::hex << anAddress));
      //			  theRASHelper.update(anAddress, anActualType, anActualTarget, false
      ///*used only in trace*/, aBBsize, anActualDirection, anActualDirection);
    }
  }

  // unused
  void updateBBTB(BTBEntry anEntry) {
    theBB_BTB.update(anEntry.thePC, anEntry.theBranchType, anEntry.theTarget, anEntry.theBBsize,
                     anEntry.theBranchDirection, anEntry.theBranchDirection, anEntry.isSpecialCall);
  }

  bool feedback(VirtualMemoryAddress anAddress, eBranchType anActualType,
                eDirection anActualDirection, VirtualMemoryAddress anActualTarget,
                BPredState &aBPState, int aBBsize) {
    //    	DBG_(Tmp, ( << "Feedback for " << &std::hex << anAddress));
    bool is_mispredict =
        stats(anAddress, anActualType, anActualDirection, anActualTarget, aBPState);

    //std::cout << "addess: " << anAddress << "\ttype:"<< theBTB.type(anAddress) << "\ttarget: " << theBTB.target(anAddress) << "\n";
    if(!theBTB.contains(anAddress)){
      if(anActualType == kCall || anActualType == kIndirectCall){
          theRAS.push_back(anAddress + 4);
      }
      else if(anActualType == kReturn && theRAS.size()){
          aBPState.thePredictedTarget = theRAS.back();
          theRAS.pop_back();
      }
    }
    theBTB.update(anAddress, anActualType, anActualTarget, aBBsize, anActualDirection,
                  anActualDirection, aBPState.detectedSpecialCall);

    // we don't need it
    /*if (aBPState.detectedSpecialCall) {
      assert(anActualType == kCall || anActualType == kIndirectReg ||
             anActualType == kIndirectCall);
      //			  DBG_(Tmp, ( << "Adding special Call " << &std::hex << anAddress));
      theRASHelper.update(anAddress, anActualType, anActualTarget, aBBsize, anActualDirection,
                          anActualDirection, false);
    }*/

#ifdef TAGE
    aBPState.theActualDirection = anActualDirection;
    aBPState.theActualType = anActualType;

    if (is_mispredict) {
      reconstructTageHistory(aBPState);
    }

    if (aBPState.thePredictedType == kConditional && anActualType == kConditional) {
      bool taken = (anActualDirection <= kTaken);
      theTage.update_predictor(anAddress, aBPState, taken);
    }

#else
    if (anActualType == kConditional) {

      if (is_new) {
        theBimodal.update(anAddress, anActualDirection);
        theGShare.shiftIn(anActualDirection);
      } else if (aBPState.thePredictedType != kNonBranch) {

        // Restore shift register and shift in the actual prediction
        if ((aBPState.theGSharePrediction >= kNotTaken) != (anActualDirection >= kNotTaken)) {
          theGShare.setShiftReg(aBPState.theGShareShiftReg);
          theGShare.shiftIn(anActualDirection);
        }

        // Update 2-bit counters
        // Get current counter values, being careful to use the shift register from before
        eDirection bimodal = theBimodal.direction(anAddress);
        eDirection gshare = theGShare.direction(anAddress, aBPState.theGShareShiftReg);
        eDirection meta = theMeta.direction(anAddress);

        // Modify the tables, being careful to use the shift registers used to originally make
        // the prediction
        theBimodal.update(anAddress, apply(anActualDirection, bimodal));
        theGShare.update(anAddress, aBPState.theGShareShiftReg, apply(anActualDirection, gshare));

        if ((gshare >= kNotTaken) != (bimodal >= kNotTaken)) {
          // Need to update meta
          if ((anActualDirection >= kNotTaken) == (gshare >= kNotTaken)) {theOpcode
        }
      } // end !is_new
    } else if (aBPState.thePredictedType != kNonBranch &&
               aBPState.thePredictedTarget != anActualTarget) {
      if (!is_new) {
        // Unconditional branch which missed its target.  Restore shift register to the time of
        // the branch
        theGShare.setShiftReg(aBPState.theGShareShiftReg);
      } else {
        // New unconditional branch.  We do not know the shift register as of the time of the
        // branch.
      }
    }

    DBG_(Verb, (<< theIndex << "-BPRED-FEEDBACK: PC \t" << anAddress << " serial "
                << aBPState.theSerial << " Target \t" << anActualTarget << "\tType " << anActualType
                << " dir " << anActualDirection << " pred " << aBPState.thePrediction));

    DBG_(Verb, (<< "Leaving feedback."));
#endif

    return is_mispredict;
  }

  // TODO: save state for 'theRAS'
  void saveStateBinary(std::string const &aDirName) const {
    std::string fname(aDirName);
    fname += "/bpred-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << theBTB;
    oa << theBimodal;
    oa << theMeta;
    oa << theGShare;
    oa << theRASHelper;

    // close archive
    ofs.close();
  }

  void saveState(std::string const &aDirName) const {
    {
      std::string fname(aDirName);
      fname += "/bpredtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
      std::ofstream ofs(fname.c_str());
      boost::archive::text_oarchive oa(ofs);

      oa << theBTB;
      oa << theBimodal;
      oa << theMeta;
      oa << theGShare;
      oa << theRASHelper;

      // close archive
      ofs.close();
    }

    {
      std::string fname(aDirName);
      fname += "/BBTBtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
      std::ofstream ofs(fname.c_str());
      boost::archive::text_oarchive oa(ofs);

      oa << theBB_BTB;

      // close archive
      ofs.close();
    }
#ifdef TAGE
    {
      std::string fname(aDirName);
      fname += "/tage-" + boost::padded_string_cast<2, '0'>(theIndex);

      std::ios_base::openmode omode = std::ios::out;
      omode |= std::ios::binary;

      std::ofstream ofs(fname.c_str(), omode);

      boost::iostreams::filtering_stream<boost::iostreams::output> out;
      out.push(ofs);

      theTage.saveState(out);
    }
#endif
  }

  void loadState(std::string const &aDirName) {
    std::string fname(aDirName);
    fname += "/bpredtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ifstream ifs(fname.c_str());
    if (ifs.good()) {
      boost::archive::text_iarchive ia(ifs);

      ia >> theBTB;
      ia >> theBimodal;
      ia >> theMeta;
      ia >> theGShare;
      ia >> theRASHelper;
      DBG_(Dev,
           (<< theName << " loaded branch predictor.  BTB size: " << theBTB.theBTBSets << " by "
            << theBTB.theBTBAssoc << " Bimodal size: " << theBimodal.theSize
            << " Meta size: " << theMeta.theSize << " Gshare size: " << theGShare.theShiftRegSize));

      ifs.close();
    } else {
      loadStateBinary(aDirName);
      DBG_(Dev, (<< "Unable to load bpred state " << fname << ". Using default state."));
    }

    {
      std::string fname(aDirName);
      fname += "/BBTBtxt-" + boost::padded_string_cast<2, '0'>(theIndex);
      std::ifstream ifs(fname.c_str());
      if (ifs.good()) {
        boost::archive::text_iarchive ia(ifs);

        ia >> theBB_BTB;
        DBG_(Dev, (<< theName << " loaded branch predictor.  BTB size: " << theBB_BTB.theBTBSets
                   << " by " << theBB_BTB.theBTBAssoc));

        ifs.close();
      } else {
        loadStateBinary(aDirName);
        DBG_(Dev, (<< "Unable to load bpred state " << fname << ". Using default state."));
      }
    }
#ifdef TAGE
    {

      std::string fname(aDirName);
      fname += "/tage-" + boost::padded_string_cast<2, '0'>(theIndex);

      std::ios_base::openmode omode = std::ios::in;

      std::ifstream ifs(fname.c_str(), omode);
      if (!ifs.good()) {
        DBG_(Dev,
             (<< " saved checkpoint state " << fname << " not found.  Resetting to empty cache. "));
      } else {
        boost::iostreams::filtering_stream<boost::iostreams::input> in;
        in.push(ifs);
        theTage.loadState(in);
      }
    }
#endif
  }

  void loadStateBinary(std::string const &aDirName) {
    std::string fname(aDirName);
    fname += "/bpred-" + boost::padded_string_cast<2, '0'>(theIndex);
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (ifs.good()) {
      try {
        boost::archive::binary_iarchive ia(ifs);

        ia >> theBTB;
        ia >> theBimodal;
        ia >> theMeta;
        ia >> theGShare;
        ia >> theRASHelper;
        DBG_(Dev, (<< theName << " loaded branch predictor.  BTB size: " << theBTB.theBTBSets
                   << " by " << theBTB.theBTBAssoc << " Bimodal size: " << theBimodal.theSize
                   << " Meta size: " << theMeta.theSize
                   << " Gshare size: " << theGShare.theShiftRegSize));

        ifs.close();
      } catch (...) {
        DBG_Assert(false, (<< "Unable to load bpred state"));
      }
    } else {
      DBG_(Dev, (<< "Unable to load bpred state " << fname << ". Using default state."));
    }
  }

}; // namespace SharedTypes

BranchPredictor *BranchPredictor::combining(std::string const &aName, uint32_t anIndex, bool RAS,
                                            bool TCE, bool TrapRet) {
  return new CombiningImpl(aName, anIndex, RAS, TCE, TrapRet);
}

FastBranchPredictor *FastBranchPredictor::combining(std::string const &aName, uint32_t anIndex) {
  return new FastCombiningImpl(aName, anIndex);
}

/* Constructors with BTB sizes */
BranchPredictor *BranchPredictor::combining(std::string const &aName, uint32_t anIndex, bool RAS,
                                            bool TCE, bool TrapRet, uint32_t aBTBSets,
                                            uint32_t aBTBWays) {
  return new CombiningImpl(aName, anIndex, RAS, TCE, TrapRet, aBTBSets, aBTBWays);
}

FastBranchPredictor *FastBranchPredictor::combining(std::string const &aName, uint32_t anIndex,
                                                    uint32_t aBTBSets, uint32_t aBTBWays) {
  return new FastCombiningImpl(aName, anIndex, aBTBSets, aBTBWays);
}

std::ostream &operator<<(std::ostream &anOstream, xExceptionSource aSource) {
  char const *types[] = {"Should never happen",
                         "xUnknown",
                         "xSaveTrap",
                         "xRestoreTrap",
                         "xIMMU",
                         "xFlushw",
                         "xTcc",
                         "xResyn",
                         "xTrans",
                         "xIntr"};
  if (aSource > xIntr) {
    anOstream << "Invalid Exception Source(" << static_cast<int>(aSource) << ")";
  } else {
    anOstream << types[aSource];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, eDirection aDir) {
  char const *dir[] = {"StronglyTaken", "Taken", "NotTaken", "StronglyNotTaken"};
  if (static_cast<unsigned int>(aDir) > kStronglyNotTaken) {
    anOstream << "InvalidBranchType(" << static_cast<unsigned int>(aDir) << ")";
  } else {
    anOstream << dir[aDir];
  }
  return anOstream;
}

std::ostream &operator<<(std::ostream &anOstream, eBranchType aType) {
  char const *types[] = {"NonBranch",         "Conditional",   "Unconditional", "Call",
                         "Indirect Register", "Indirect Call", "Return"};
  if (aType >= kLastBranchType) {
    anOstream << "InvalidBranchType(" << static_cast<int>(aType) << ")";
  } else {
    anOstream << types[aType];
  }
  return anOstream;
}

} // namespace SharedTypes
} // namespace Flexus
