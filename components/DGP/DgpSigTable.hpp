#include "../Common/SetAssocArray.hpp"    // normal array
#include "../Common/SetAssocSetDefn.hpp"  // base set definition
#include "../Common/SetAssocReplacement.hpp"    // used for the replacement extension

#include "DgpSigTableTypes.hpp"
#include "DgpSigTablePolicies.hpp"

namespace nDgpTable {

class DgpSigTable {

  // construct the required types
  typedef DgpMappingType<Flexus::Core::Address32Bit> DgpMapping;
public:
  typedef DgpBlockType<DgpMapping::Types::Tag> DgpBlock;
private:
  // create the Set type - first the MRU extension, then our own top level
  typedef nSetAssoc::BaseSetDefinition<DgpMapping::Types, DgpBlock> DgpBaseSet;
  typedef nSetAssoc::LowAssociativeMRUSet<DgpBaseSet> DgpMruSet;
  class DgpSet
      : public DgpMruSet {
  public:
    DgpSet(const uint32_t anAssoc,
           const Index anIndex)
      : DgpMruSet(anAssoc, anIndex)
    {}

    friend std::ostream & operator << (std::ostream & anOstream, const DgpSet & aSet) {
      anOstream << "set " << aSet.myIndex << " (mru order): ";
      // traverse the MRU list and list blocks in this order
      for (uint32_t ii = 0; ii < aSet.myAssoc; ii++) {
        const DgpBlock & block = aSet[aSet.mruList[ii]];
        if (block.valid()) {
          anOstream << block;
        }
      }
      return anOstream;
    }

  };
  // end class DgpSet ====================

  typedef nSetAssoc::SetAssocArray<DgpSet> DgpArray;
  typedef DgpPoliciesType<DgpArray> DgpPolicies;
public:
  typedef DgpLookupResultType<DgpMapping, DgpPolicies> DgpLookupResult;

  typedef DgpMapping::Address Address;
  typedef DgpMapping::Types::Tag Tag;
  typedef DgpMapping::Types::Index Index;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & theArray;
  }

public:
  DgpSigTable(uint32_t numSets,
              uint32_t assoc)
    : theMapping(numSets, assoc)
    , theArray(numSets, assoc)
  {}

  DgpLookupResult operator[](const Address & anAddress) {
    return DgpLookupResult::lookup(anAddress, theArray, theMapping);
  }

  Tag makeTag(const Address & anAddress) {
    return theMapping.tag(anAddress);
  }

  Address makeAddress(const Tag & aTag, const Index & anIndex) {
    return theMapping.address(aTag, anIndex);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    anOstream << "Entries: " << aTable.theMapping.numSets()*aTable.theMapping.assoc()
              << "  Sets: " << aTable.theMapping.numSets()
              << "  Assoc: " << aTable.theMapping.assoc();
    return anOstream;
  }

  void finalize(DgpStats * stats) {
    // nothing to do
  }

private:
  DgpMapping theMapping;
  DgpArray theArray;

};  // end class DgpSigTable

}  // end namespace nDgpTable
