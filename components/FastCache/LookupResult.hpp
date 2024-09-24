#ifndef __LOOKUP_RESULT_HPP__
#define __LOOKUP_RESULT_HPP__
#include "components/FastCache/CoherenceStates.hpp"
#include "core/boost_extensions/intrusive_ptr.hpp"
#include "core/types.hpp"

#include <boost/multi_index_container.hpp>

using namespace Flexus::SharedTypes;

namespace nFastCache {

class LookupResult : public boost::counted_base
{
  public:
    LookupResult() {}
    virtual ~LookupResult() {}

    virtual void allocate(CoherenceState_t new_state) = 0;

    virtual void changeState(CoherenceState_t new_state, bool make_MRU, bool make_LRU) = 0;

    virtual void updateLRU() = 0;

    virtual CoherenceState_t getState()     = 0;
    virtual PhysicalMemoryAddress address() = 0;
};

typedef boost::intrusive_ptr<LookupResult> LookupResult_p;

} // namespace nFastCache

#endif