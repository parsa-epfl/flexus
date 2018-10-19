#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <iostream>
#include <iomanip>
#include <boost/serialization/serialization.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/operators.hpp>
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

template < class underlying_type, bool isVirtual = false >
class MemoryAddress_
  : boost::totally_ordered < MemoryAddress_<underlying_type, isVirtual>
  , boost::additive < MemoryAddress_<underlying_type, isVirtual>, int
    > > {
public:
  MemoryAddress_()
    : address(0)
  {}
  explicit MemoryAddress_(underlying_type newAddress)
    : address(newAddress)
  {}
  bool operator< (MemoryAddress_ const & other) {
    return (address < other.address);
  }
  bool operator== (MemoryAddress_ const & other) {
    return (address == other.address);
  }
  MemoryAddress_& operator= (MemoryAddress_ const & other) {
    address = other.address;
    return *this;
  }
  MemoryAddress_& operator += (underlying_type const & addend) {
    address += addend;
    return *this;
  }
  MemoryAddress_& operator -= (underlying_type const & addend) {
    address -= addend;
    return *this;
  }
  operator underlying_type() const {
    return address;
  }
  friend std::ostream & operator << ( std::ostream & anOstream, MemoryAddress_ const & aMemoryAddress) {
    anOstream << (isVirtual ? "v:" : "p:" ) << std::hex << std::setw(9) << std::right << std::setfill('0') << aMemoryAddress.address << std::dec;
    return anOstream;
  }

  friend underlying_type operator + ( MemoryAddress_ const & aMemoryAddress, const uint64_t aVal ) {
        return aMemoryAddress.address + aVal;
  }

  friend underlying_type operator - ( MemoryAddress_ const & aMemoryAddress, const uint64_t aVal ) {
        return aMemoryAddress.address - aVal;
  }

private:
  underlying_type address;

  //Serialization Support
public:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & address;
  }

}; // class MemoryAddress_

typedef boost::dynamic_bitset<> bits;

bits fillbits(const int bitSize);
bool anyBits(bits b);
bits concat_bits (const bits & lhs, const bits & rhs);
bits align(uint64_t x, int y);
std::pair<bits,bits> splitBits(const bits & input);

bits construct(uint8_t* bytes, int size);


typedef uint32_t Word32Bit;
typedef uint64_t Word64Bit;

} // end namespace Core

namespace SharedTypes {

#define FLEXUS_PhysicalMemoryAddress_TYPE_PROVIDED
#define FLEXUS_DataWord_TYPE_PROVIDED
#if (FLEXUS_TARGET_PA_BITS == 32)
// typedef Core::MemoryAddress_< Core::Word32Bit, false > PhysicalMemoryAddress;
// typedef Core :: Word32Bit DataWord;
typedef Core::MemoryAddress_< Core::Word64Bit, false > PhysicalMemoryAddress;
typedef Core :: Word64Bit DataWord;
#elif (FLEXUS_TARGET_PA_BITS == 64)
typedef Core::MemoryAddress_< Core::Word64Bit, false > PhysicalMemoryAddress;
typedef Core :: Word64Bit DataWord;
#else
#error "FLEXUS_TARGET_PA_BITS is not set. Did you forget to set a target?"
#endif

#define FLEXUS_VirtualMemoryAddress_TYPE_PROVIDED
#if (FLEXUS_TARGET_VA_BITS == 32)
// typedef Core::MemoryAddress_< Core::Word32Bit, true > VirtualMemoryAddress;
typedef Core::MemoryAddress_< Core::Word64Bit, true > VirtualMemoryAddress;
#elif (FLEXUS_TARGET_VA_BITS == 64)
typedef Core::MemoryAddress_< Core::Word64Bit, true > VirtualMemoryAddress;
#else
#error "FLEXUS_TARGET_PA_BITS is not set. Did you forget to set a target?"
#endif

using Flexus::Core::index_t;
using Flexus::Core::node_id_t;

} // end namespace SharedTypes
} // end namespace Flexus

#endif // TYPES_HPP
