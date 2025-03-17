#include "RAS.hpp"

ReturnAddressStack::ReturnAddressStack(){};

void ReturnAddressStack::push(uint64_t target, uint64_t timestamp){
    stack.push_back({target, timestamp});
    if (stack.size() > size){
        stack.erase(stack.begin());
    }
}

uint64_t ReturnAddressStack::pop(){
    if (stack.size() == 0){
        return 0;
    }

    uint64_t top = stack.back().target;
    stack.pop_back();

    return top;
}

void ReturnAddressStack::recover(uint64_t mispred_timestamp){
    for(auto it = stack.begin(); it != stack.end(); ){
        if (it->timestamp > mispred_timestamp){
            stack.erase(it);
        }
        else{
            it++;
        }
    }
}

uint32_t ReturnAddressStack::get_occupancy(){
    return stack.size();
}