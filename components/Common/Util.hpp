#ifndef __COMMON_UTIL_HPP__
#define __COMMON_UTIL_HPP__

#include <boost/functional/hash.hpp>
#include <core/types.hpp>

namespace nCommonUtil {

template<typename _Type>
int32_t log_base2(_Type val) {
  int32_t ret = 0;
  for (val >>= 1; val > 0; val >>= 1, ret++);
  return ret;
}

class AddressHash {
public:
  std::size_t operator()(Flexus::SharedTypes::PhysicalMemoryAddress addr) const {
    boost::hash<int> my_hash;
    return my_hash((int)(addr >> 6));
  }
};

// Find the closest prime number that is less than or equal to 'num'
// Works for numbers up to 7919
int32_t get_closest_prime(int32_t num);
};

#endif // ! __COMMON_UTIL_HPP__
