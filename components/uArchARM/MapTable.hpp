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

#ifndef FLEXUS_uARCHARM_MAPTABLE_HPP_INCLUDED
#define FLEXUS_uARCHARM_MAPTABLE_HPP_INCLUDED

#include <algorithm>
#include <list>
#include <vector>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <core/debug/debug.hpp>
#include <core/performance/profile.hpp>

#include "RegisterType.hpp"
#include <components/uArchARM/uArchInterfaces.hpp>
namespace ll = boost::lambda;

namespace nuArchARM {

// Implements architectural r-register renaming from op-code register names
//(%g, %l, %o, %i and CWP) to architectural register numbers.  Also tracks
// the current state of the CWP.
struct RegisterWindowMap {
  int32_t theCWP;
  bool theAG;
  bool theIG;
  bool theMG;

  RegisterWindowMap() {
    reset();
  }

  void reset() {
    theCWP = 0;
    theAG = false;
    theIG = false;
    theMG = false;
  }

  reg rotate(reg aReg) {
    FLEXUS_PROFILE();
    if (aReg.theType != xRegisters) {
      return aReg;
    }
    reg reg(aReg);
    if (reg.theIndex >= kFirstSpecialReg) {
      // Special register
      return reg;
    } else if (reg.theIndex < kGlobalRegCount) {
      if (theAG && reg.theIndex < kGlobalRegCount) {
        reg.theIndex += kGlobalRegCount;
      } else if (theMG && reg.theIndex < kGlobalRegCount) {
        reg.theIndex += 2 * kGlobalRegCount;
      } else if (theIG && reg.theIndex < kGlobalRegCount) {
        reg.theIndex += 3 * kGlobalRegCount;
      }

      // The global registers %g0 through %g7 and alternate globals
      //%ag0 through %ag7 do not rotate, nor does Y (comes between %g7 and %ag0)

      return reg;
    } else {
      // NOTE: I haven't fixed these comments since adding the MG and IG
      // register sets aReg ranges from 8 to 32. When theCWP = 0, reg 8 should
      // map to 128, reg 24 should map to 16 When theCWP = 7, reg 8 should map
      // to 16, reg 24 should map to 32
      reg.theIndex = aReg.theIndex + (kNumWindows - 1 - theCWP) * kRegistersPerWindow +
                     (kGlobalRegSets - 1) * kGlobalRegCount;
      if (reg.theIndex >= kFirstSpecialReg) {
        reg.theIndex = reg.theIndex - kWindowRegCount;
      }
      return reg;
    }
  }

  void setCWP(int32_t aCWP) {
    theCWP = aCWP;
    DBG_Assert(theCWP < 8);
  }

  int32_t getCWP() {
    return theCWP;
  }

  void incrementCWP() {
    ++theCWP;
    if (theCWP > 7) {
      theCWP = 0;
    }
  }

  void decrementCWP() {
    --theCWP;
    if (theCWP < 0) {
      theCWP = 7;
    }
  }

  void setAG(bool anAG) {
    theAG = anAG;
  }

  void setIG(bool anIG) {
    theIG = anIG;
  }

  void setMG(bool anMG) {
    theMG = anMG;
  }
};

typedef uint32_t regName;
typedef uint32_t pRegister;

struct PhysicalMap {
  int32_t theNameCount;
  int32_t theRegisterCount;
  std::vector<pRegister> theMappings;
  std::list<pRegister> theFreeList;
  std::vector<std::list<pRegister>> theAssignedRegisters;
  std::vector<int> theReverseMappings;

  PhysicalMap(int32_t aNameCount, int32_t aRegisterCount)
      : theNameCount(aNameCount), theRegisterCount(aRegisterCount) {
    theMappings.resize(aNameCount);
    theAssignedRegisters.resize(aNameCount);
    theReverseMappings.resize(aRegisterCount);
    reset();
  }

  void reset() {
    std::fill(theReverseMappings.begin(), theReverseMappings.end(), -1);
    // Fill in initial mappings
    std::copy(boost::counting_iterator<int>(0), boost::counting_iterator<int>(theNameCount),
              theMappings.begin());
    for (int32_t i = 0; i < theNameCount; ++i) {
      theMappings[i] = i;
      theReverseMappings[i] = i;
      theAssignedRegisters[i].clear();
      theAssignedRegisters[i].push_back(i);
    }

    // Fill in initial free list
    theFreeList.clear();
    std::copy(boost::counting_iterator<int>(theNameCount),
              boost::counting_iterator<int>(theRegisterCount), std::back_inserter(theFreeList));
  }

  pRegister map(regName aRegisterName) {
    FLEXUS_PROFILE();
    DBG_Assert(aRegisterName < theMappings.size(),
               (<< "Name: " << aRegisterName << " number of names: " << theMappings.size()));
    return theMappings[aRegisterName];
  }

  pRegister mapArchitectural(regName aRegisterName) {
    FLEXUS_PROFILE();
    DBG_Assert(
        aRegisterName < theAssignedRegisters.size(),
        (<< "Name: " << aRegisterName << " number of names: " << theAssignedRegisters.size()));
    return theAssignedRegisters[aRegisterName].front();
  }

  // return value is new pRegister, previous pRegister
  std::pair<pRegister, pRegister> create(regName aRegisterName) {
    FLEXUS_PROFILE();
    DBG_Assert(aRegisterName < theMappings.size());
    pRegister previous_reg = theMappings[aRegisterName];
    pRegister new_reg = theFreeList.front();
    theFreeList.pop_front();
    theMappings[aRegisterName] = new_reg;
    DISPATCH_DBG("Mapping archReg[" << aRegisterName << "] -> pReg[" << new_reg
                                    << "] - previous pReg[" << previous_reg << "]");
    theAssignedRegisters[aRegisterName].push_back(new_reg);
    DBG_Assert(theReverseMappings[new_reg] == -1);
    theReverseMappings[new_reg] = aRegisterName;
    return std::make_pair(new_reg, previous_reg);
  }

  void free(pRegister aRegisterName) {
    FLEXUS_PROFILE();
    theFreeList.push_back(aRegisterName);
    std::list<pRegister>::iterator iter, end;
    int32_t arch_name = theReverseMappings[aRegisterName];
    DBG_Assert(arch_name >= 0 && arch_name < static_cast<int>(theAssignedRegisters.size()));
    DBG_Assert(theMappings[arch_name] != aRegisterName);
    DBG_Assert(theAssignedRegisters[arch_name].size());
    theReverseMappings[aRegisterName] = -1;
    iter = theAssignedRegisters[arch_name].begin();
    end = theAssignedRegisters[arch_name].end();
    bool found = false;
    while (iter != end) {
      if (*iter == aRegisterName) {
        theAssignedRegisters[arch_name].erase(iter);
        found = true;
        break;
      }
      ++iter;
    }
    DBG_Assert(found);
  }

  void restore(regName aRegisterName, pRegister aReg) {
    FLEXUS_PROFILE();
    theMappings[aRegisterName] = aReg;
    // Restore should have no effect on the register stacks in
    // theReverseMappings because the restored register had not been freed
  }

  bool checkInvariants() {
    FLEXUS_PROFILE();
    // No register name appears twice in either table
    std::vector<pRegister> registers;
    registers.resize(theRegisterCount);

    // for(const auto& aMapping: theMappings)
    //  ++registers[aMapping];
    std::for_each(theMappings.begin(), theMappings.end(), ++ll::var(registers)[ll::_1]);

    // for(const auto& aFree: theFreeList)
    //  ++registers[aFree];
    std::for_each(theFreeList.begin(), theFreeList.end(), ++ll::var(registers)[ll::_1]);

    if (std::find_if(registers.begin(),
                     registers.end()
                     //, [](const auto& x){ return x > 1U; }
                     ,
                     ll::_1 > 1U) != registers.end()) {
      DBG_(Crit, (<< "MapTable: Invariant check failed"));
      return false;
    }

    return true;
  }

  void dump(std::ostream &anOstream) {
    anOstream << "\n\tMappings\n\t";
    for (uint32_t i = 0; i < theMappings.size(); ++i) {
      anOstream << "r" << i << ": p" << theMappings[i] << "\t";
      if ((i & 7) == 7)
        anOstream << "\n\t";
    }
    anOstream << "\n\tFree List\n\t";
    // for(const auto& aFree: theFreeList)
    //  anOstream << 'p' << aFree << ' ';
    std::for_each(theFreeList.begin(), theFreeList.end(),
                  ll::var(anOstream) << 'p' << ll::_1 << ' ');
    anOstream << std::endl;
  }

  void dumpMappings(std::ostream &anOstream) {
    for (uint32_t i = 0; i < theMappings.size(); ++i) {
      anOstream << "r" << i << ": p" << theMappings[i] << " ";
      if ((i & 7) == 7)
        anOstream << "\n";
    }
  }
  void dumpFreeList(std::ostream &anOstream) {
    // for(const auto& aFree: theFreeList)
    //  anOstream << 'p' << aFree << ' ';
    std::for_each(theFreeList.begin(), theFreeList.end(),
                  ll::var(anOstream) << 'p' << ll::_1 << ' ');
    anOstream << std::endl;
  }
  void dumpReverseMappings(std::ostream &anOstream) {
    for (uint32_t i = 0; i < theReverseMappings.size(); ++i) {
      if (theReverseMappings[i] == -1) {
        anOstream << "p" << i << ": -"
                  << " ";
      } else {
        anOstream << "p" << i << ": r" << theReverseMappings[i] << " ";
      }
      if ((i & 7) == 7)
        anOstream << "\n";
    }
  }
  void dumpAssignments(std::ostream &anOstream) {
    for (uint32_t i = 0; i < theAssignedRegisters.size(); ++i) {
      anOstream << "r" << i << ": ";
      std::list<pRegister>::iterator iter, end;
      iter = theAssignedRegisters[i].begin();
      end = theAssignedRegisters[i].end();
      while (iter != end) {
        anOstream << "p" << *iter << " ";
        ++iter;
      }
      anOstream << " ";
      if ((i & 7) == 7)
        anOstream << "\n";
    }
  }
};

std::ostream &operator<<(std::ostream &anOstream, PhysicalMap &aMap);

} // namespace nuArchARM

#endif // FLEXUS_uARCHARM_MAPTABLE_HPP_INCLUDED
