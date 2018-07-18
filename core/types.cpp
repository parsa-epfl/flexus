#include "types.hpp"

using namespace Flexus::Core;

bits add_bits(const bits & lhs, const bits & rhs){
    bits sum = lhs ^ rhs;
    bits carry = (lhs & rhs) << 1;
    if ((sum & carry).to_ulong() > 0)
        return add_bits (sum, carry);
    else
        return sum ^ carry;
}

bits sub_bits(bits x, bits y){
    while (y.to_ulong() != 0)
    {
        bits borrow = (~x) & y;
        x = x ^ y;
        y = borrow << 1;
    }
    return x;
}

static void populateBitSet (std::string &buffer, bits &bitMap)
{
   const bits &temp = bits(buffer);
   bitMap = temp;
}

bits fillbits(const int bitSize){
    bits result;
    std::string buffer;
    for (int i = 0; i < bitSize; i++){
        buffer += "1";
    }
    populateBitSet(buffer, result);
    return result;
}

bool anyBits(bits b){
    return b.any();
}

bits concat_bits (const bits & lhs, const bits & rhs){
    bits result;
    std::string b1,b2,all;
    to_string(lhs, b1);
    to_string(rhs, b2);
    all = b1+b2;
    populateBitSet(all, result);
    return result;
}


std::pair<bits,bits> splitBits(const bits & input){

  std::string buffer;
    to_string(input, buffer);
    bits first( buffer.substr(0,buffer.length()/2));
    bits second( buffer.substr(buffer.length()/2,buffer.length()));

    return std::make_pair(first, second);

}

