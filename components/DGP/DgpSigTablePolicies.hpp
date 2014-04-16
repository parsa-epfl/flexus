#include "../Common/SetAssocTypes.hpp"

namespace nDgpTable {

template < class AddressLength >
class DgpMappingType;

template<>
class DgpMappingType<Flexus::Core::Address32Bit> {
public:

  // first define the size of base types used everywhere
  struct Types {
    typedef nSetAssoc::SimpleTag<uint32_t> Tag;
    typedef nSetAssoc::SimpleIndex Index;
    typedef nSetAssoc::SimpleBlockNumber BlockNumber;
  };

  // allow the address type to be extracted
  typedef SignatureMapping::Signature Address;

private:
  typedef Types::Tag Tag;
  typedef Types::Index Index;

public:
  DgpMappingType(uint32_t aNumSets,
                 uint32_t anAssoc)
    : myNumSets(aNumSets)
    , myAssoc(anAssoc)
    , myIndexBits(log_base2(aNumSets))
  {}

  const uint32_t numSets() const {
    return myNumSets;
  }
  const uint32_t assoc() const {
    return myAssoc;
  }

  Index index(const Address & anAddress) const {
    return Index( anAddress & (myNumSets - 1) );
  }
  Tag tag(const Address & anAddress) const {
    return Tag( anAddress >> myIndexBits );
  }
  Address address(const Tag & aTag,
                  const Index & anIndex) const {
    return Address( ( aTag << myIndexBits) | anIndex );
  }

private:
  // Number of sets in the array
  const uint32_t myNumSets;

  // Associativity of the array
  const uint32_t myAssoc;

  // Bits required to represent the index
  const uint32_t myIndexBits;

};  // end class DgpMappingType

template < class BaseLookup >
class DgpAccessPolicyType {
public:
  static void access(BaseLookup & aLookup, const DgpAccessType & anAccess) {
    // do nothing - the access type is not sufficient to determine what
    // state the block should go into - more information is needed, and
    // this comes from outside the DGP signature table
  }
};  // end class DgpAccessPolicyType

template < class Set >
class DgpReplacementPolicyType {
  typedef typename Set::Block Block;

public:
  static Block & victim(Set & aSet) {
    // choose the LRU entry - no update of the MRU chain is required
    return aSet[aSet.listTail()];
  }

  static void perform(Set & aSet, Block & aBlock, const DgpAccessType & anAccess) {
    switch (anAccess) {
      case WriteAccess:
      case SnoopAccess:
        // move to MRU position on every access
        aSet.moveToHead(aSet.indexOf(aBlock));
        break;
      case Eviction:
        // move to LRU position
        aSet.moveToTail(aSet.indexOf(aBlock));
        break;
      case DontCare:
      default:
        break;
    }
  }

};  // end class DgpReplacementPolicyType

template < class TArrayType >
class DgpPoliciesType {
public:
  // allow others to extract the array type
  typedef TArrayType ArrayType;

  // convenience typedefs for policies
  typedef DgpReplacementPolicyType<typename ArrayType::Set> ReplacementPolicy;
  typedef DgpAccessPolicyType<typename ArrayType::BaseLookupResult> AccessPolicy;
};  // end class DgpPoliciesType

}  // end namespace nDgpTable
