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
#ifndef __BLOCK_DIRECTORY_ENTRY_HPP__
#define __BLOCK_DIRECTORY_ENTRY_HPP__

#include <components/CommonQEMU/Serializers.hpp>

#define MAX_NUM_SHARERS 128

namespace nFastCMPCache {

using nCommonSerializers::StdDirEntrySerializer;

class BlockDirectoryEntry : public AbstractDirectoryEntry {
public:
  PhysicalMemoryAddress theAddress;
  SharingVector theSharers;
  SharingState theState;

public:
  BlockDirectoryEntry(PhysicalMemoryAddress addr) : theAddress(addr), theState(ZeroSharers) {
  }
  BlockDirectoryEntry() : theAddress(0), theState(ZeroSharers) {
  }
  virtual ~BlockDirectoryEntry() {
  }

  BlockDirectoryEntry(PhysicalMemoryAddress addr, std::bitset<MAX_NUM_SHARERS> state) : theAddress(addr) {
    theAddress = addr;
    theSharers.setSharers(state);
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }

  const SharingVector &sharers() const {
    return theSharers;
  }
  const SharingState &state() const {
    return theState;
  }
  const PhysicalMemoryAddress &tag() const {
    return theAddress;
  }

  inline void addSharer(int32_t index) {
    theSharers.addSharer(index);
    if (theSharers.countSharers() == 1) {
      theState = OneSharer;
    } else {
      theState = ManySharers;
    }
  }

  inline void makeExclusive(int32_t index) {
    DBG_Assert(theSharers.isSharer(index),
               (<< "Core " << index << " is not a sharer " << theSharers.getSharers()));
    DBG_Assert(theSharers.countSharers() == 1,
               (<< "Cannot make xclusive, sharers = " << theSharers));
    theState = OneSharer;
  }

  inline void removeSharer(int32_t index) {
    theSharers.removeSharer(index);
    if (theSharers.countSharers() == 0) {
      theState = ZeroSharers;
    } else if (theSharers.countSharers() == 1) {
      theState = OneSharer;
    }
  }
  void setSharers(const SharingVector &new_sharers) {
    theSharers = new_sharers;
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }
  void setSharers(uint64_t new_sharers) {
    theSharers.setSharers(new_sharers);
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }

  inline void clear() {
    theSharers.clear();
    theState = ZeroSharers;
  }

  inline void reset(PhysicalMemoryAddress addr) {
    theAddress = addr;
    theSharers.clear();
    theState = ZeroSharers;
  }

  BlockDirectoryEntry &operator&=(const BlockDirectoryEntry &entry) {
    theSharers &= entry.theSharers;
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
    return *this;
  }

  StdDirEntrySerializer getSerializer() {
    return StdDirEntrySerializer((uint64_t)theAddress, theSharers.getUInt64());
  }

  void operator=(const StdDirEntrySerializer &serializer) {
    theAddress = PhysicalMemoryAddress(serializer.tag);
    theSharers.setSharers(serializer.state);
    int32_t count = theSharers.countSharers();
    if (count == 1) {
      theState = OneSharer;
    } else if (count == 0) {
      theState = ZeroSharers;
    } else {
      theState = ManySharers;
    }
  }
};

typedef boost::intrusive_ptr<BlockDirectoryEntry> BlockDirectoryEntry_p;

struct BlockEntryWrapper : public AbstractDirectoryEntry {
  BlockEntryWrapper(BlockDirectoryEntry &block) : block(block) {
  }
  BlockDirectoryEntry &block;

  BlockDirectoryEntry *operator->() const {
    return &block;
  }
  BlockDirectoryEntry &operator*() const {
    return block;
  }
};

typedef boost::intrusive_ptr<BlockEntryWrapper> BlockEntryWrapper_p;

}; // namespace nFastCMPCache

#endif //! __BLOCK_DIRECTORY_ENTRY_HPP__
