#include "RAS.hpp"

#include "components/uFetch/uFetchTypes.hpp"

void ReturnAddressStack::push(uint64_t target) {
    stack.push_back(target);

    if (stack.size() > size)
        stack.erase(stack.begin());
}

uint64_t ReturnAddressStack::pop() {
    auto top = stack.back();

    if (stack.size())
        stack.pop_back();

    return top;
}

void ReturnAddressStack::get(std::vector<uint64_t> &vec) {
    vec = stack;
}

void ReturnAddressStack::recover(BPredState &bpred) {
    // first revert back to the old state before the speculative push/pop
    stack = bpred.theRAS;

    // then adjust according to the actual branch type
    if (bpred.theActualType == kCall ||
        bpred.theActualType == kIndirectCall)
        push(bpred.pc + 4);

    if (bpred.theActualType == kReturn)
        pop();
}