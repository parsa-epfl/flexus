#ifndef FLEXUS_RAS
#define FLEXUS_RAS

#include "core/types.hpp"
#include <vector>
class ReturnAddressStack{
    private:
      struct Entry{
        uint64_t target;
        uint64_t timestamp;
      };

      std::vector<Entry> stack;
      uint64_t size = 64;

    public:
      ReturnAddressStack();
      void push(uint64_t target, uint64_t timestamp);
      uint64_t pop();
      void recover(uint64_t mispred_timestamp);
      uint32_t get_occupancy();
};
#endif 