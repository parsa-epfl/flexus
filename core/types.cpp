#include "types.hpp"

namespace Flexus {
namespace Core {

// bits add_bits(const bits & lhs, const bits & rhs){
//    bits sum = lhs ^ rhs;
//    bits carry = (lhs & rhs) << 1;
//    if ((sum & carry) > 0)
//        return add_bits (sum, carry);
//    else
//        return sum ^ carry;
//}

// bits sub_bits(bits x, bits y){
//    while (y.to_ulong() != 0)
//    {
//        bits borrow = (~x) & y;
//        x = x ^ y;
//        y = borrow << 1;
//    }
//    return x;
//}

// static void populateBitSet (std::string &buffer, bits &bitMap)
//{
//   const bits &temp = bits(buffer);
//   bitMap = temp;
//}

// bits fillbits(const int bitSize){
//    bits result;
//    std::string buffer;
//    for (int i = 0; i < bitSize; i++){
//        buffer += "1";
//    }
//    populateBitSet(buffer, result);
//    return result;
//}

bool anyBits(bits b) {
  return b != 0;
}

bits align(uint64_t x, int y) {
  return bits(y * (x / y));
}

bits concat_bits(const bits &lhs, const bits &rhs) {

  return ((lhs << 64) | rhs);
}

bits construct(uint8_t *bytes, size_t size) {
  bits result;
  for (int i = (size - 1); i >= 0; i--) {
    result |= bytes[i] << i;
  }
  return result;
}

std::pair<uint64_t, uint64_t> splitBits(const bits &input, uint64_t size) {
  size /= 2;
  bits a, b = -1;
  a = b <<= size;
  b = ~a;
  std::pair<uint64_t, uint64_t> ret;
  ret.first = static_cast<uint64_t>((a & input) >> size);
  ret.second = static_cast<uint64_t>(b & input);

  return ret;
}

} // namespace Core
} // namespace Flexus
