//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
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
