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
#ifndef __FASTCMPCACHE_SHARINGVECTOR_HPP__
#define __FASTCMPCACHE_SHARINGVECTOR_HPP__

#include <bitset>
#include <list>

namespace nFastCMPCache {

#define MAX_NUM_SHARERS 128

typedef unsigned __int128 uint128_t;

class SharingVector {
protected:
  std::bitset<MAX_NUM_SHARERS> sharers;

  SharingVector(const std::bitset<MAX_NUM_SHARERS> &s) : sharers(s) {
  }

public:
  virtual ~SharingVector() {
  }

  SharingVector() {
    sharers.reset();
  }

  virtual void addSharer(int32_t index) {
    DBG_Assert((index >= 0) && (index < MAX_NUM_SHARERS), (<< "Invalid index " << index));
    sharers[index] = true;
  }

  virtual void removeSharer(int32_t index) {
    DBG_Assert((index >= 0) && (index < MAX_NUM_SHARERS), (<< "Invalid index " << index));
    sharers[index] = false;
  }

  virtual int32_t countSharers() const {
    return sharers.count();
  }

  virtual bool isSharer(int32_t index) const {
    DBG_Assert((index >= 0) && (index < MAX_NUM_SHARERS), (<< "Invalid index " << index));
    return sharers[index];
  }

  virtual int32_t getFirstSharer() const {
    for (int32_t i = 0; i < MAX_NUM_SHARERS; i++) {
      if (sharers[i]) {
        return i;
      }
    }
    return -1;
  }

  int32_t getClosestSharer(int32_t index) const {
    int32_t lhs = index + 1;
    int32_t rhs = index - 1;
    for (; lhs < MAX_NUM_SHARERS; lhs++) {
      if (sharers[lhs])
        break;
    }
    for (; rhs >= 0; rhs--) {
      if (sharers[rhs])
        break;
    }
    if (rhs >= 0) {
      if (lhs < MAX_NUM_SHARERS) {
        if ((index - rhs) < (lhs - index)) {
          return rhs;
        } else {
          return lhs;
        }
      } else {
        return rhs;
      }
    } else if (lhs < MAX_NUM_SHARERS) {
      return lhs;
    } else if (sharers[index]) {
      return index;
    }
    return -1;
  }

  virtual std::list<int> toList() const {
    std::list<int> l;
    for (int32_t i = 0; i < MAX_NUM_SHARERS; i++) {
      if (sharers[i]) {
        l.push_back(i);
      }
    }
    return l;
  }

  virtual const std::bitset<MAX_NUM_SHARERS> &getSharers() const {
    return sharers;
  }

  virtual const bool operator==(const SharingVector &a) const {
    return (sharers == a.sharers);
  }

  virtual const bool operator!=(const SharingVector &a) const {
    return (sharers != a.sharers);
  }

  virtual const SharingVector operator&(const SharingVector &a) const {
    return SharingVector(sharers & a.sharers);
  }

  virtual SharingVector &operator=(const SharingVector &a) {
    sharers = a.sharers;
    return *this;
  }

  virtual SharingVector &operator&=(const SharingVector &a) {
    sharers &= a.sharers;
    return *this;
  }

  virtual SharingVector &operator|=(const SharingVector &a) {
    sharers |= a.sharers;
    return *this;
  }

  virtual void clear() {
    sharers.reset();
  }

  virtual void flip() {
    sharers.flip();
  }

  virtual const bool any() const {
    return sharers.any();
  }

  virtual uint64_t getUInt() const {
    uint128_t ret = 0;
    for (int32_t i = MAX_NUM_SHARERS; i >= 0; i--) {
      ret = ret << 1;
      ret |= (sharers[i] ? 1 : 0);
    }
    return ret;
  }

  virtual void setSharers(std::bitset<MAX_NUM_SHARERS> s) {
    for (int32_t i = 0; i < MAX_NUM_SHARERS; i++)
      sharers[i] = s[i];
  }

  //  virtual void setSharers(uint64_t s) {
  //#if MAX_NUM_SHARERS > 64
  //#error "MAX_NUM_SHARERS must be less than or equal to 64, or you just
  // re-write some functions #endif
  //    for (int32_t i = 0; i < 64; i++, s >>= 1) {
  //      sharers[i] = (s & 1);
  //    }
  //  }
};

inline std::ostream &operator<<(std::ostream &os, const SharingVector &sharers) {
  os << sharers.getSharers().to_ulong();
  return os;
}

}; // namespace nFastCMPCache

#endif // __FASTCMPCACHE_SHARINGVECTOR_HPP__
