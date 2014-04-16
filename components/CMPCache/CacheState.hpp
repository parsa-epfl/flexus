#ifndef __CACHE_STATE_HPP__
#define __CACHE_STATE_HPP__

#include <string>
#include <vector>

namespace nCMPCache {

#define MAX_NUM_STATES  16
#define STATE_MASK      0xF
#define PREFETCH_MASK   0x10
#define PROTECT_MASK    0x20
#define LOCKED_MASK    	0x40

class CacheState {
public:
  CacheState(const CacheState & s) : val(s.val) {}

  // Standard Cache states
  // Individual policies can use any combination of these states
  static const CacheState Modified;
  static const CacheState Owned;
  static const CacheState Exclusive;
  static const CacheState Shared;
  static const CacheState Invalid;
  static const CacheState InvalidPresent;
  static const CacheState Forward;

  inline std::string name() const {
    return names()[val & STATE_MASK];
  }

  inline const bool operator==(const CacheState & a) const {
    return ( ((a.val ^ val) & STATE_MASK) == 0);
  }

  inline const bool operator!=(const CacheState & a) const {
    return ( ((a.val ^ val) & STATE_MASK) != 0);
  }

  inline CacheState & operator=(const CacheState & a) {
    val = (val & ~STATE_MASK) | (a.val & STATE_MASK);
    return *this;
  }

  inline const bool prefetched() const {
    return ((val & PREFETCH_MASK) != 0);
  }

  inline const bool isProtected() const {
    return ((val & PROTECT_MASK) != 0);
  }

  inline const bool isLocked() const {
    return ((val & LOCKED_MASK) != 0);
  }

  inline const bool isValid() const {
    return ((*this != Invalid) && (*this != InvalidPresent));
  }

  inline void setPrefetched(bool p) {
    if (p) {
      val = val | PREFETCH_MASK;
    } else {
      val = val & ~PREFETCH_MASK;
    }
  }

  inline void setProtected(bool p) {
    if (p) {
      val = val | PROTECT_MASK;
    } else {
      val = val & ~PROTECT_MASK;
    }
  }

  inline void setLocked(bool p) {
    if (p) {
      val = val | LOCKED_MASK;
    } else {
      val = val & ~LOCKED_MASK;
    }
  }

  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & val;
  }

  static const CacheState & char2State(uint8_t c) {
    switch (c) {
      case 'M':
        return Modified;
        break;
      case 'O':
        return Owned;
        break;
      case 'E':
        return Exclusive;
        break;
      case 'S':
        return Shared;
        break;
      case 'I':
        return Invalid;
        break;
      case 'P':
        return InvalidPresent;
        break;
      case 'F':
        return Forward;
        break;
      default:
        DBG_Assert(false, ( << "Unknown state '" << c << "'"));
        break;
    }
    return Invalid;
  }

private:
  explicit CacheState() : val(Invalid.val) {
    /* Never called */ *(int *)0 = 0;
  }
  CacheState(const std::string & name) : val(names().size()) {
    names().push_back(name);
    DBG_Assert(names().size() <= MAX_NUM_STATES);
  }
  static std::vector<std::string> & names() {
    static std::vector<std::string> theNames;
    return theNames;
  }

  char val;
};

std::ostream & operator<<(std::ostream & os, const CacheState & state);

};

#endif // ! __CACHE_STATE_HPP__
