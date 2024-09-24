#ifndef TYPES_HPP
#define TYPES_HPP

#include <boost/dynamic_bitset.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/multiprecision/cpp_int.hpp>
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
struct Address32Bit
{};
struct Address64Bit
{};

typedef uint32_t index_t;
typedef uint32_t node_id_t;

} // namespace Core
} // namespace Flexus

namespace Flexus {
namespace Core {

template<class underlying_type, bool isVirtual = false>
class MemoryAddress_
  : boost::totally_ordered<MemoryAddress_<underlying_type, isVirtual>,
                           boost::additive<MemoryAddress_<underlying_type, isVirtual>, int>>
{
  public:
    MemoryAddress_()
      : address(0)
    {
    }
    explicit MemoryAddress_(underlying_type newAddress)
      : address(newAddress)
    {
    }
    bool operator<(MemoryAddress_ const& other) { return (address < other.address); }
    bool operator==(MemoryAddress_ const& other) { return (address == other.address); }
    MemoryAddress_& operator=(underlying_type const& other)
    {
        address = other;
        return *this;
    }
    MemoryAddress_& operator=(MemoryAddress_ const& other)
    {
        address = other.address;
        return *this;
    }
    MemoryAddress_& operator|=(MemoryAddress_ const& other)
    {
        address |= other.address;
        return *this;
    }
    MemoryAddress_& operator+=(underlying_type const& addend)
    {
        address += addend;
        return *this;
    }
    MemoryAddress_& operator-=(underlying_type const& addend)
    {
        address -= addend;
        return *this;
    }
    operator underlying_type() const { return address; }
    bool operator==(MemoryAddress_ const& other) const { return (address == other.address); }
    friend std::ostream& operator<<(std::ostream& s, MemoryAddress_ const& mem)
    {
        s << (isVirtual ? "v(" : "p(");
        s << "0x" << std::hex << std::setw(9) << std::right << std::setfill('0');
        s << mem.address;
        s << std::dec;
        s << ")";

        return s;
    }

    friend underlying_type operator+(MemoryAddress_ const& aMemoryAddress, const uint64_t aVal)
    {
        return aMemoryAddress.address + aVal;
    }

    friend underlying_type operator-(MemoryAddress_ const& aMemoryAddress, const uint64_t aVal)
    {
        return aMemoryAddress.address - aVal;
    }

  private:
    underlying_type address;

    // Serialization Support
  public:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar & address;
    }
};
typedef boost::multiprecision::uint128_t bits;

bool
anyBits(bits b);
bits
concat_bits(const bits& lhs, const bits& rhs);
bits
align(uint64_t x, int y);
std::pair<uint64_t, uint64_t>
splitBits(const bits& input, uint64_t size);

bits
construct(uint8_t* bytes, size_t size);

typedef uint32_t Word32Bit;
typedef uint64_t Word64Bit;

} // end namespace Core

namespace SharedTypes {

#define FLEXUS_PhysicalMemoryAddress_TYPE_PROVIDED
#define FLEXUS_DataWord_TYPE_PROVIDED

typedef Core ::Word64Bit DataWord;
typedef Core::MemoryAddress_<Core::Word64Bit, false> PhysicalMemoryAddress;
typedef Core::MemoryAddress_<Core::Word64Bit, true> VirtualMemoryAddress;

struct CPU_State
{
    uint64_t pc;
    uint64_t regs[32];
};

using Flexus::Core::index_t;
using Flexus::Core::node_id_t;

} // end namespace SharedTypes
} // end namespace Flexus

#endif // TYPES_HPP
