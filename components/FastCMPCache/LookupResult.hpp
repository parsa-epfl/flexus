#ifndef __FASTCMPCACHE_LOOKUP_RESULT_HPP__
#define __FASTCMPCACHE_LOOKUP_RESULT_HPP__

#include <components/FastCMPCache/CoherenceStates.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

namespace nFastCMPCache {

class LookupResult : public boost::counted_base {
public:
  LookupResult() {}
  virtual ~LookupResult() {}

  virtual void allocate(CoherenceState_t new_state) = 0;

  virtual void changeState(CoherenceState_t new_state, bool make_MRU, bool make_LRU) = 0;

  virtual void updateLRU() = 0;

  virtual CoherenceState_t getState() = 0;
  virtual PhysicalMemoryAddress address() = 0;
};

typedef boost::intrusive_ptr<LookupResult> LookupResult_p;

}

#endif
