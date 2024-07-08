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
#ifndef TYPES_HPP
#define TYPES_HPP

#include <boost/dynamic_bitset.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <iomanip>
#include <iostream>
#include <vector>

#if __cplusplus > 199711L
#include <cstdint>
#else
#include <stdint.h>
#endif

#include <core/target.hpp>

namespace Flexus {
namespace Core {

// specifiers for address lengths
struct Address32Bit {};
struct Address64Bit {};

typedef uint32_t index_t;
typedef uint32_t node_id_t;

} // namespace Core
} // namespace Flexus

namespace Flexus {
namespace Core {

template <class underlying_type, bool isVirtual = false>
class MemoryAddress_
    : boost::totally_ordered<MemoryAddress_<underlying_type, isVirtual>,
                             boost::additive<MemoryAddress_<underlying_type, isVirtual>, int>> {
public:
  MemoryAddress_() : address(0) {
  }
  explicit MemoryAddress_(underlying_type newAddress) : address(newAddress) {
  }
  bool operator<(MemoryAddress_ const &other) {
    return (address < other.address);
  }
  bool operator==(MemoryAddress_ const &other) {
    return (address == other.address);
  }
  MemoryAddress_ &operator=(underlying_type const &other) {
    address = other;
    return *this;
  }
  MemoryAddress_ &operator=(MemoryAddress_ const &other) {
    address = other.address;
    return *this;
  }
  MemoryAddress_ &operator|=(MemoryAddress_ const &other) {
    address |= other.address;
    return *this;
  }
  MemoryAddress_ &operator+=(underlying_type const &addend) {
    address += addend;
    return *this;
  }
  MemoryAddress_ &operator-=(underlying_type const &addend) {
    address -= addend;
    return *this;
  }
  operator underlying_type() const {
    return address;
  }
  bool operator==(MemoryAddress_ const &other) const {
    return (address == other.address);
  }
  friend std::ostream &operator<<(std::ostream &anOstream, MemoryAddress_ const &aMemoryAddress) {
    anOstream << (isVirtual ? "v:" : "p:") << std::hex << std::setw(9) << std::right
              << std::setfill('0') << aMemoryAddress.address << std::dec;
    return anOstream;
  }

  friend underlying_type operator+(MemoryAddress_ const &aMemoryAddress, const uint64_t aVal) {
    return aMemoryAddress.address + aVal;
  }

  friend underlying_type operator-(MemoryAddress_ const &aMemoryAddress, const uint64_t aVal) {
    return aMemoryAddress.address - aVal;
  }

private:
  underlying_type address;

  // Serialization Support
public:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &address;
  }

}; // class MemoryAddress_

typedef __uint128_t bits;

// bits fillbits(const int bitSize);
bool anyBits(bits b);
bits align(uint64_t x, int y);

bits construct(uint8_t *bytes, size_t size);

typedef uint32_t Word32Bit;
typedef uint64_t Word64Bit;

} // end namespace Core

namespace SharedTypes {

#define FLEXUS_PhysicalMemoryAddress_TYPE_PROVIDED
#define FLEXUS_DataWord_TYPE_PROVIDED
#if (FLEXUS_TARGET_PA_BITS == 32)
// typedef Core::MemoryAddress_< Core::Word32Bit, false > PhysicalMemoryAddress;
// typedef Core :: Word32Bit DataWord;
typedef Core::MemoryAddress_<Core::Word64Bit, false> PhysicalMemoryAddress;
typedef Core ::Word64Bit DataWord;
#elif (FLEXUS_TARGET_PA_BITS == 64)
typedef Core::MemoryAddress_<Core::Word64Bit, false> PhysicalMemoryAddress;
typedef Core ::Word64Bit DataWord;
#else
#error "FLEXUS_TARGET_PA_BITS is not set. Did you forget to set a target?"
#endif

#define FLEXUS_VirtualMemoryAddress_TYPE_PROVIDED
#if (FLEXUS_TARGET_VA_BITS == 32)
// typedef Core::MemoryAddress_< Core::Word32Bit, true > VirtualMemoryAddress;
typedef Core::MemoryAddress_<Core::Word64Bit, true> VirtualMemoryAddress;
#elif (FLEXUS_TARGET_VA_BITS == 64)
typedef Core::MemoryAddress_<Core::Word64Bit, true> VirtualMemoryAddress;
#else
#error "FLEXUS_TARGET_PA_BITS is not set. Did you forget to set a target?"
#endif

using Flexus::Core::index_t;
using Flexus::Core::node_id_t;

} // end namespace SharedTypes
} // end namespace Flexus

namespace std {
std::ostream &operator<<(std::ostream &s, const Flexus::Core::bits &aBits);
}

#endif // TYPES_HPP
