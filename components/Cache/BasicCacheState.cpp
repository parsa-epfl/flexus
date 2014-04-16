#include <components/Cache/BasicCacheState.hpp>

namespace nCache {

const BasicCacheState BasicCacheState::Modified("Modified");
const BasicCacheState BasicCacheState::Owned("Owned");
const BasicCacheState BasicCacheState::Exclusive("Exclusive");
const BasicCacheState BasicCacheState::Shared("Shared");
const BasicCacheState BasicCacheState::Invalid("Invalid");

std::ostream & operator<<( std::ostream & os, const BasicCacheState & state) {
  os << state.name();
  if (state.isProtected()) {
    os << "_X";
  }
  if (state.prefetched()) {
    os << "_P";
  }
  return os;
}

};
