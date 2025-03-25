#ifndef FLEXUS_RAS
#define FLEXUS_RAS

#include "components/uFetch/uFetchTypes.hpp"
#include "core/types.hpp"

#include <vector>

using namespace Flexus::SharedTypes;

class ReturnAddressStack
{
private:
    std::vector<uint64_t> stack;
    uint64_t size;

public:
    ReturnAddressStack():
        size(64) {
        stack.resize(size);
    }

    void push(uint64_t target);

    uint64_t pop();

    void recover(BPredState& bpred);

    bool valid() const {
        return stack.size() != 0;
    }

    void get(std::vector<uint64_t> &vec);
};

#endif