#ifndef _GLOBAL_HASHER_HPP_
#define _GLOBAL_HASHER_HPP_

#include "core/types.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <set>

namespace nGlobalHasher {

typedef Flexus::SharedTypes::PhysicalMemoryAddress Address;

class GlobalHasher
{
  private:
    typedef std::function<int(const Address&)> hash_fn_t;
    typedef std::list<hash_fn_t>::iterator hash_iterator_t;

    GlobalHasher();

    int32_t theHashShift;
    int32_t theHashMask;

    std::list<hash_fn_t> theHashList;

    bool has_been_initialized;

    int32_t simple_hash(int32_t offset, const Address& addr) const;
    int32_t xor_hash(int32_t offset, int32_t xor_shift, const Address& addr) const;
    int32_t shift_hash(int32_t offset, int32_t shift, const Address& addr) const;
    int32_t full_prime_hash(int32_t offset, int32_t prime, const Address& addr) const;

    std::function<int(int)> createMatrixHash(std::string args,
                                             int32_t num_buckets,
                                             int32_t shift,
                                             int32_t mask,
                                             int32_t offset) const;

  public:
    std::set<int> hashAddr(const Address& addr);

    void initialize(std::list<std::string>& hash_configs,
                    int32_t initial_shift,
                    int32_t buckets_per_hash,
                    bool partitioned);

    int32_t numHashes() { return theHashList.size(); }

    static GlobalHasher& theHasher();
};
}; // namespace nGlobalHasher

#endif // ! _GLOBAL_HASHER_HPP_