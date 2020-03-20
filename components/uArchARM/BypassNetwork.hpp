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

#ifndef FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
#define FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED

#include "uArchInterfaces.hpp"
#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <list>
#include <vector>
namespace ll = boost::lambda;
namespace nuArchARM {

class BypassNetwork {
protected:
  uint32_t theXRegs;
  uint32_t theVRegs;
  uint32_t theCCRegs;
  typedef std::function<bool(register_value)> bypass_fn;
  typedef std::pair<boost::intrusive_ptr<Instruction>, bypass_fn> bypass_handle;
  typedef std::list<bypass_handle> bypass_handle_list;
  typedef std::vector<bypass_handle_list> bypass_map;
  typedef std::vector<int32_t> collect_counter;
  bypass_map theXDeps;
  collect_counter theXCounts;
  bypass_map theVDeps;
  collect_counter theVCounts;
  bypass_map theCCDeps;
  collect_counter theCCCounts;

public:
  BypassNetwork(uint32_t anXRegs, uint32_t anVRegs, uint32_t anCCRegs)
      : theXRegs(anXRegs), theVRegs(anVRegs), theCCRegs(anCCRegs) {
    reset();
  }

  void doCollect(bypass_handle_list &aList) {
    FLEXUS_PROFILE();
    bypass_handle_list::iterator iter, temp, end;
    iter = aList.begin();
    end = aList.end();
    while (iter != end) {
      temp = iter;
      ++iter;
      if (temp->first->isComplete()) {
        aList.erase(temp);
      }
    }
  }

  bypass_handle_list &lookup(mapped_reg anIndex) {
    switch (anIndex.theType) {
    case xRegisters:
      return theXDeps[anIndex.theIndex];
    case vRegisters:
      return theVDeps[anIndex.theIndex];
    case ccBits:
      return theCCDeps[anIndex.theIndex];
    default:
      DBG_Assert(false);
      return theXDeps[0]; // Suppress compiler warning
    }
  }

  void collect(mapped_reg anIndex) {
    switch (anIndex.theType) {
    case xRegisters:
      --theXCounts[anIndex.theIndex];
      if (theXCounts[anIndex.theIndex] <= 0) {
        theXCounts[anIndex.theIndex] = 10;
        doCollect(theXDeps[anIndex.theIndex]);
      }
      break;
    case vRegisters:
      --theVCounts[anIndex.theIndex];
      if (theVCounts[anIndex.theIndex] <= 0) {
        theVCounts[anIndex.theIndex] = 10;
        doCollect(theVDeps[anIndex.theIndex]);
      }
      break;
    case ccBits:
      --theCCCounts[anIndex.theIndex];
      if (theCCCounts[anIndex.theIndex] <= 0) {
        theCCCounts[anIndex.theIndex] = 10;
        doCollect(theCCDeps[anIndex.theIndex]);
      }
      break;
    default:
      DBG_Assert(false);
    }
  }

  void collectAll() {
    FLEXUS_PROFILE();
    for (uint32_t i = 0; i < theXRegs; ++i) {
      theXCounts[i] = 10;
      doCollect(theXDeps[i]);
    }
    for (uint32_t i = 0; i < theVRegs; ++i) {
      theVCounts[i] = 10;
      doCollect(theVDeps[i]);
    }
    for (uint32_t i = 0; i < theCCRegs; ++i) {
      theCCCounts[i] = 10;
      doCollect(theCCDeps[i]);
    }
  }

  void reset() {
    FLEXUS_PROFILE();
    theXDeps.clear();
    theVDeps.clear();
    theCCDeps.clear();
    theXDeps.resize(theXRegs);
    theVDeps.resize(theVRegs);
    theCCDeps.resize(theCCRegs);
    theXCounts.clear();
    theVCounts.clear();
    theCCCounts.clear();
    theXCounts.resize(theXRegs, 10);
    theVCounts.resize(theVRegs, 10);
    theCCCounts.resize(theCCRegs, 10);
  }

  void connect(mapped_reg anIndex, boost::intrusive_ptr<Instruction> inst, bypass_fn fn) {
    FLEXUS_PROFILE();
    collect(anIndex);
    lookup(anIndex).push_back(std::make_pair(inst, fn));
  }
  void unmap(mapped_reg anIndex) {
    FLEXUS_PROFILE();
    lookup(anIndex).clear();
  }

  void write(mapped_reg anIndex, register_value aValue, uArchARM &aCore) {
    FLEXUS_PROFILE();
    bypass_handle_list &list = lookup(anIndex);
    bypass_handle_list::iterator iter = list.begin();
    bypass_handle_list::iterator end = list.end();
    while (iter != end) {
      if (iter->second(aValue)) {
        auto temp = iter;
        ++iter;
        list.erase(temp);
      } else {
        ++iter;
      }
    }
  }
};

} // namespace nuArchARM

#endif // FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
