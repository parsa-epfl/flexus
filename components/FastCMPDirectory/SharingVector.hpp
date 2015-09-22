#ifndef __FASTCMPDIRECTORY_SHARINGVECTOR_HPP__
#define __FASTCMPDIRECTORY_SHARINGVECTOR_HPP__

#include <bitset>
#include <list>

namespace nFastCMPDirectory {

#define MAX_NUM_SHARERS 512

class SharingVector {
protected:
  std::bitset<MAX_NUM_SHARERS> sharers;

  SharingVector(const std::bitset<MAX_NUM_SHARERS> &s) : sharers(s) {}

public:
  virtual ~SharingVector() {}

  SharingVector() {
    sharers.reset();
  }

  virtual void addSharer(int32_t index) {
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
    sharers[index] = true;
  }

  virtual void removeSharer(int32_t index) {
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
    sharers[index] = false;
  }

  virtual int32_t countSharers() const {
    return sharers.count();
  }

  virtual bool isSharer(int32_t index) const {
    DBG_Assert( (index >= 0) && (index < MAX_NUM_SHARERS), ( << "Invalid index " << index ));
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
      if (sharers[lhs]) break;
    }
    for (; rhs >= 0; rhs--) {
      if (sharers[rhs]) break;
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

  virtual const std::bitset<MAX_NUM_SHARERS>& getSharers() const {
    return sharers;
  }

  virtual const bool operator==(const SharingVector & a) const {
    return (sharers == a.sharers);
  }

  virtual const bool operator!=(const SharingVector & a) const {
    return (sharers != a.sharers);
  }

  virtual const SharingVector operator&(const SharingVector & a) const {
    return SharingVector(sharers & a.sharers);
  }

  virtual SharingVector & operator=(const SharingVector & a) {
    sharers = a.sharers;
    return *this;
  }

  virtual SharingVector & operator&=(const SharingVector & a) {
    sharers &= a.sharers;
    return *this;
  }

  virtual SharingVector & operator|=(const SharingVector & a) {
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

  virtual uint64_t getUInt64() const {
    uint64_t ret = 0;
    for (int32_t i = 63; i >= 0; i--) {
      ret = ret << 1;
      ret |= (sharers[i] ? 1 : 0);
    }
    return ret;
  }

  virtual void setSharers(uint64_t s) {
#if MAX_NUM_SHARERS > 64
#error "MAX_NUM_SHARERS must be less than or equal to 64, or you just re-write some functions"
#endif
    for (int32_t i = 0; i < 64; i++, s >>= 1) {
      sharers[i] = (s & 1);
    }
  }
};

inline std::ostream & operator<<(std::ostream & os, const SharingVector & sharers) {
  os << sharers.getSharers();
  return os;
}

}; // namespace

#endif // __FASTCMPDIRECTORY_SHARINGVECTOR_HPP__
