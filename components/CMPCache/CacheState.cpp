#include <core/debug/debug.hpp>

#include <components/CMPCache/CacheState.hpp>

namespace nCMPCache {

const CacheState CacheState::Modified("Modified");
const CacheState CacheState::Owned("Owned");
const CacheState CacheState::Exclusive("Exclusive");
const CacheState CacheState::Shared("Shared");
const CacheState CacheState::Invalid("Invalid");
const CacheState CacheState::InvalidPresent("InvalidPresent");
const CacheState CacheState::Forward("Forward");

std::ostream & operator<<( std::ostream & os, const CacheState & state) {
  os << state.name();
  if (state.isProtected()) {
    os << "_X";
  }
  if (state.prefetched()) {
    os << "_P";
  }
  if (state.isLocked()) {
    os << "_LOCKED";
  }
  return os;
}

};
