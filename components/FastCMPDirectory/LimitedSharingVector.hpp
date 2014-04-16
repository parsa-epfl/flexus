#ifndef __FASTCMPDIRECTORY_LIMITEDSHARINGVECTOR_HPP__
#define __FASTCMPDIRECTORY_LIMITEDSHARINGVECTOR_HPP__

#include <bitset>
#include <list>

#include <components/FastCMPDirectory/SharingVector.hpp>

namespace nFastCMPDirectory {

#define MAX_NUM_SHARERS 64

class LimitedSharingVector : public SharingVector {
private:

  int32_t theMaxPointers;
  int32_t theNodesPerBit;
  bool using_vector;
  int32_t sharer_count;

public:
  virtual ~LimitedSharingVector() {}

  LimitedSharingVector(const LimitedSharingVector & v)
    : theMaxPointers(v.theMaxPointers), theNodesPerBit(v.theNodesPerBit), using_vector(v.using_vector), sharer_count(v.sharer_count) {
    sharers = v.sharers;
  }

  LimitedSharingVector(int32_t numPointers, int32_t granularity)
    : theMaxPointers(numPointers), theNodesPerBit(granularity) {
    sharers.reset();
    using_vector = false;
    sharer_count = 0;
  }

  void addSharer(int32_t index) {
    DBG_(Iface, ( << "Adding Sharer: " << index << " uv " << std::boolalpha << using_vector << " sc " << sharer_count << " max " << theMaxPointers ));
    if (sharers[index]) {
      DBG_(Iface, ( << "Bit already set! Adding Sharer: " << index << " uv " << std::boolalpha << using_vector << " sc " << sharer_count << " max " << theMaxPointers ));
      return;
    }
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
    if (!using_vector) {
      sharers[index] = true;
      sharer_count++;
      if (sharer_count > theMaxPointers) {
        std::list<int> slist(toList());
        sharers.reset();
        while (!slist.empty()) {
          if (!sharers[slist.front()]) {
            int32_t base = ((int)(slist.front() / theNodesPerBit)) * theNodesPerBit;
            for (int32_t i = 0; i < theNodesPerBit; i++) {
              sharers[base+i] = true;
            }
          }
          slist.pop_front();
        }
        using_vector = true;
      }
    } else {
      int32_t base = ((int)(index / theNodesPerBit)) * theNodesPerBit;
      for (int32_t i = 0; i < theNodesPerBit; i++) {
        sharers[base+i] = true;
      }
    }
  }

  void removeSharer(int32_t index) {
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
    if (!using_vector) {
      DBG_Assert( sharers[index], ( << "Removing sharer that was not in sharing list." ));
      sharers[index] = false;
      sharer_count--;
    } else {
      DBG_Assert(sharers[index], ( << "Trying to remove sharer not in list."));
    }
  }

  void setSharer(int32_t index) {
    sharers.reset();
    sharers[index] = true;
    sharer_count = 1;
    using_vector = false;

    // In case MaxPointers <= 0
    if (sharer_count > theMaxPointers) {
      std::list<int> slist(toList());
      sharers.reset();
      while (!slist.empty()) {
        if (!sharers[slist.front()]) {
          int32_t base = ((int)(slist.front() / theNodesPerBit)) * theNodesPerBit;
          for (int32_t i = 0; i < theNodesPerBit; i++) {
            sharers[base+i] = true;
          }
        }
        slist.pop_front();
      }
      using_vector = true;
    }
  }

  void removeAll(std::list<int> &indices) {
    if (using_vector) {
      indices.clear();
      return;
    }
    std::bitset<MAX_NUM_SHARERS> inval_bits;
    while (!indices.empty()) {
      inval_bits[indices.front()] = true;
      indices.pop_front();
    }
    inval_bits.flip();
    sharers &= inval_bits;
    if (sharers.count() <= (uint32_t)theMaxPointers) {
      sharer_count = sharers.count();
      using_vector = false;
    } else {
      std::list<int> slist(toList());
      sharers.reset();
      while (!slist.empty()) {
        if (!sharers[slist.front()]) {
          int32_t base = ((int)(slist.front() / theNodesPerBit)) * theNodesPerBit;
          for (int32_t i = 0; i < theNodesPerBit; i++) {
            sharers[base+i] = true;
          }
        }
        slist.pop_front();
      }
      using_vector = true;
    }
  }

  int32_t countSharers() const {
    return sharers.count();
  }

  bool isSharer(int32_t index) {
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
    return sharers[index];
  }

  int32_t getFirstSharer() {
    if (!sharers.none()) {
      for (int32_t i = 0; i < MAX_NUM_SHARERS; i++) {
        if (sharers[i]) {
          return i;
        }
      }
    }
    return -1;
  }

  std::list<int> toList() {
    std::list<int> l;
    for (int32_t i = 0; i < MAX_NUM_SHARERS; i++) {
      if (sharers[i]) {
        l.push_back(i);
      }
    }
    return l;
  }

  const std::bitset<MAX_NUM_SHARERS>& getSharers() const {
    return sharers;
  }

  SharingVector & operator=(const LimitedSharingVector & a) {
    sharers = a.sharers;
    using_vector = a.using_vector;
    theNodesPerBit = a.theNodesPerBit;
    theMaxPointers = a.theMaxPointers;
    return *this;
  }

  void clear() {
    sharers.reset();
    using_vector = false;
    sharer_count = 0;
  }

  void flip() {
    sharers.flip();
  }

  const bool any() const {
    return sharers.any();
  }

  uint64_t getUInt64() const {
    uint64_t ret = 0;
    for (int32_t i = 63; i >= 0; i--) {
      ret = ret << 1;
      ret |= (sharers[i] ? 1 : 0);
    }
    return ret;
  }

  void setSharers(uint64_t s) {
#if MAX_NUM_SHARERS > 64
#error "MAX_NUM_SHARERS must be less than or equal to 64, or you just re-write some functions
#endif
    for (int32_t i = 0; i < 64; i++, s >>= 1) {
      sharers[i] = (s & 1);
    }
    if ((int)sharers.count() < theMaxPointers) {
      using_vector = false;
    }
  }
};

inline std::ostream & operator<<(std::ostream & os, const LimitedSharingVector & sharers) {
  os << sharers.getSharers();
  return os;
}

}; // namespace

#endif // __FASTCMPDIRECTORY_LIMITEDSHARINGVECTOR_HPP__
