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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

#include <boost/serialization/map.hpp>
#include <core/metaprogram.hpp>

//#include <core/boost_extensions/lexical_cast.hpp>
#include <boost/lexical_cast.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>

#include <core/debug/debug.hpp>
#include <core/flexus.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include "bbv.hpp"

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

int32_t done_count = 0;

namespace nuArch {

static const int32_t kCountThreshold = 50000;

typedef std::map<uint64_t, int> bbindex_t;
bbindex_t theBBIndex;
int32_t theNextBBIndex = 0;
bool theMapChanged = false;

struct BBVTrackerImpl : public BBVTracker {
  int32_t theIndex;
  int32_t theDumpNo;
  int32_t theCountSinceDump;

  PhysicalMemoryAddress theLastPC;
  bool theLastWasBranch;
  bool theLastWasBranchDelay;

  std::map<uint64_t, long> theBBV;

  BBVTrackerImpl(int32_t anIndex)
      : theIndex(anIndex), theDumpNo(0), theCountSinceDump(0), theLastPC(PhysicalMemoryAddress(0)),
        theLastWasBranch(false), theLastWasBranchDelay(false) {
  }

  virtual ~BBVTrackerImpl() {
  }

  virtual void commitInsn(PhysicalMemoryAddress aPC, bool isBranch) {
    // Only process if we just processed a branch delay, or we had a
    // discontinuous fetch.

    bool same_basic_block = ((aPC == theLastPC + 4 && !theLastWasBranchDelay) || aPC == 0);
    theLastPC = aPC;
    theLastWasBranchDelay = theLastWasBranch;
    theLastWasBranch = isBranch;

    if (!same_basic_block) {
      // We have a new basic block. See if it has an index
      ++theBBV[aPC];
    }

    // See if it is time to dump stats
    ++theCountSinceDump;
    if (theCountSinceDump >= kCountThreshold) {
      dump();
      theCountSinceDump = 0;
      theBBV.clear();
    }
  }

  void dump() {
    if (theDumpNo > 100)
      return; // Don't bother dumping past 100
    // Write out and then clear the current BBV vector
    std::string name("bbv.cpu");
    name += boost::padded_string_cast<2, '0'>(theIndex);
    name += ".";
    name += boost::padded_string_cast<3, '0'>(theDumpNo);
    name += ".out";
    std::ofstream bbv(name.c_str());

    boost::archive::binary_oarchive oa(bbv);

    oa << const_cast<std::map<uint64_t, long> const &>(theBBV);

    // close archive
    bbv.close();
    ++theDumpNo;
    if (theDumpNo == 51) {
      ++done_count;
      if (done_count == 16) {
        DBG_Assert(false, (<< "Halting simulation - all CPUs have executed "
                              "2.5M instructions"));
      }
    }
  }
};

BBVTracker *BBVTracker::createBBVTracker(int32_t aCPUIndex) {
  return new BBVTrackerImpl(aCPUIndex);
}

} // namespace nuArch