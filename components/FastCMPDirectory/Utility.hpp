#ifndef __FASTCMPDIRECTORY_UTILITY_HPP__
#define __FASTCMPDIRECTORY_UTILITY_HPP__

namespace nFastCMPDirectory {

inline int32_t log_base2(uint64_t num) {
  uint32_t ret;
  for (ret = 0; num > 1; ret++, num >>= 1) {}
  return ret;
}

inline int32_t ABS(int32_t n) {
  return ((n < 0) ? -n : n);
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

};

#endif
