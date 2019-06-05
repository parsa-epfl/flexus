#ifndef __CACHE_BASIC_CACHE_STATE_HPP
#define __CACHE_BASIC_CACHE_STATE_HPP

#include <string>
#include <ostream>
#include <vector>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#define MAX_NUM_STATES 16
#define STATE_MASK  0xF
#define PREFETCH_MASK 0x10
#define PROTECT_MASK 0x20

using namespace std;

namespace nCache {
class BasicCacheState {
public:
  BasicCacheState(const BasicCacheState & s) : val(s.val) {}

  // Statble states
  static const BasicCacheState Modified;
  static const BasicCacheState Owned;
  static const BasicCacheState Exclusive;
  static const BasicCacheState Shared;
  static const BasicCacheState Invalid;

  inline string name() const {
    return names()[val & STATE_MASK];
  }

  inline const bool operator==(const BasicCacheState & a) const {
    return ( ((a.val ^ val) & STATE_MASK) == 0);
  }

  inline const bool operator!=(const BasicCacheState & a) const {
    return ( ((a.val ^ val) & STATE_MASK) != 0);
  }

  inline BasicCacheState & operator=(const BasicCacheState & a) {
    val = (val & ~STATE_MASK) | (a.val & STATE_MASK);
    return *this;
  }

  inline const bool prefetched() const {
    return ((val & PREFETCH_MASK) != 0);
  }

  inline const bool isProtected() const {
    return ((val & PROTECT_MASK) != 0);
  }

  inline const bool isValid() const {
    return (*this != Invalid);
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

  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & val;
  }

  static const BasicCacheState & char2State(uint8_t c) {
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
      default:
        DBG_Assert(false, ( << "Unknown state '" << c << "'"));
        break;
    }
    return Invalid;
  }

  static const BasicCacheState & int2State(int32_t c) {
    // For loading text flexpoints ONLY!
    switch (c) {
      case 7:
        return Modified;
        break;
      case 3:
        return Owned;
        break;
      case 5:
        return Exclusive;
        break;
      case 1:
        return Shared;
        break;
      case 0:
        return Invalid;
        break;
      default:
        DBG_Assert(false, ( << "Unknown state '" << c << "'"));
        break;
    }
    return Invalid;
  }

private:
  explicit BasicCacheState() : val(Invalid.val) {
    /* Never called */ *(int *)0 = 0;
  }
  BasicCacheState(const string & name) : val(names().size()) {
    names().push_back(name);
    DBG_Assert(names().size() <= MAX_NUM_STATES);
  }
  static std::vector<string>& names() {
    static std::vector<string> theNames;
    return theNames;
  }

  char val;

  friend std::ostream & operator<<( std::ostream & os, const BasicCacheState & state);
};
};
#endif // ! __CACHE_BASIC_CACHE_STATE_HPP
