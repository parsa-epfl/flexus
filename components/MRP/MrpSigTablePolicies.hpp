#include "../Common/SetAssocTypes.hpp"

namespace nMrpTable {

template < class SignatureMapping, class AddressLength >
class MrpMappingBase;

template < class SignatureMappingN >
class MrpMappingType
  : public MrpMappingBase < typename SignatureMappingN::SignatureMapping,
      typename SignatureMappingN::AddressLength > {
  typedef MrpMappingBase < typename SignatureMappingN::SignatureMapping,
          typename SignatureMappingN::AddressLength > Base;
public:
  MrpMappingType(uint32_t aNumSets,
                 uint32_t anAssoc)
    : Base(aNumSets, anAssoc)
  {}
};  // end class MrpMappingType

template < class SignatureMapping >
class MrpMappingBase<SignatureMapping, Flexus::Core::Address32Bit> {
public:

  // first define the size of base types used everywhere
  struct Types {
    typedef nSetAssoc::SimpleTag<uint32_t> Tag;
    typedef nSetAssoc::SimpleIndex Index;
    typedef nSetAssoc::SimpleBlockNumber BlockNumber;
  };

  // allow the address type to be extracted
  typedef typename SignatureMapping::Signature Address;

private:
  typedef typename Types::SetAssocTag Tag;
  typedef typename Types::SetAssocIndex Index;

public:
  MrpMappingBase(uint32_t aNumSets,
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
    return Address( (aTag << myIndexBits) | anIndex );
  }

private:
  // Number of sets in the array
  const uint32_t myNumSets;

  // Associativity of the array
  const uint32_t myAssoc;

  // Bits required to represent the index
  const uint32_t myIndexBits;

};  // end class MrpMappingBase - 32bit

template < class SignatureMapping >
class MrpMappingBase<SignatureMapping, Flexus::Core::Address64Bit> {
public:

  // first define the size of base types used everywhere
  struct Types {
    typedef nSetAssoc::SimpleTag<int64_t> Tag;
    typedef nSetAssoc::SimpleIndex Index;
    typedef nSetAssoc::SimpleBlockNumber BlockNumber;
  };

  // allow the address type to be extracted
  typedef typename SignatureMapping::Signature Address;

private:
  typedef typename Types::Tag Tag;
  typedef typename Types::Index Index;

public:
  MrpMappingBase(uint32_t aNumSets,
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
    return Address( ((int64_t)aTag << myIndexBits) | (int64_t)anIndex );
  }

private:
  // Number of sets in the array
  const uint32_t myNumSets;

  // Associativity of the array
  const uint32_t myAssoc;

  // Bits required to represent the index
  const uint32_t myIndexBits;

};  // end class MrpMappingBase - 64bit

template < class BaseLookup >
class MrpAccessPolicyType {
public:
  static void access(BaseLookup & aLookup, const MrpAccessType & anAccess) {
    // do nothing - the access type is not sufficient to determine what
    // state the block should go into - more information is needed, and
    // this comes from outside the MRP signature table
  }
};  // end class MrpAccessPolicyType

template < class Set >
class MrpReplacementPolicyType {
  typedef typename Set::Block Block;

public:
  static Block & victim(Set & aSet) {
    // choose the LRU entry - no update of the MRU chain is required
    return aSet[aSet.listTail()];
  }

  static void perform(Set & aSet, Block & aBlock, const MrpAccessType & anAccess) {
    switch (anAccess) {
      case AnyAccess:
        // move to MRU position on every access
        aSet.moveToHead(aSet.indexOf(aBlock));
        break;
      default:
        break;
    }
  }

};  // end class MruReplacementPolicyType

template < class TArrayType >
class MrpPoliciesType {
public:
  // allow others to extract the array type
  typedef TArrayType ArrayType;

  // convenience typedefs for policies
  typedef MrpReplacementPolicyType<typename ArrayType::Set> ReplacementPolicy;
  typedef MrpAccessPolicyType<typename ArrayType::BaseLookupResult> AccessPolicy;
};  // end class MruPoliciesType

}  // end namespace nMruTable
