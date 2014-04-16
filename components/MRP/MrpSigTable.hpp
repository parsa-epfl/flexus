#include "../Common/SetAssocArray.hpp"    // normal array
#include "../Common/SetAssocSetDefn.hpp"  // base set definition
#include "../Common/SetAssocReplacement.hpp"    // used for the replacement extension

#include "MrpSigTableTypes.hpp"
#include "MrpSigTablePolicies.hpp"

namespace nMrpTable {

template < class SignatureMappingN >
class MrpSigTableType {

  // construct the required types
  typedef MrpMappingType<SignatureMappingN> MrpMapping;
public:
  typedef MrpBlockType<typename MrpMapping::Types::Tag> MrpBlock;
private:
  // create the Set type - first the MRU extension, then our own top level
  typedef nSetAssoc::BaseSetDefinition<typename MrpMapping::Types, MrpBlock> MrpBaseSet;
  typedef nSetAssoc::LowAssociativeMRUSet<MrpBaseSet> MrpMruSet;
  class MrpSet
      : public MrpMruSet {
  public:
    MrpSet(const uint32_t anAssoc,
           const typename MrpMruSet::Index anIndex)
      : MrpMruSet(anAssoc, anIndex)
    {}

    friend std::ostream & operator << (std::ostream & anOstream, const MrpSet & aSet) {
      anOstream << "set " << aSet.myIndex << " (mru order): ";
      // traverse the MRU list and list blocks in this order
      for (uint32_t ii = 0; ii < aSet.myAssoc; ii++) {
        const MrpBlock & block = aSet[aSet.mruList[ii]];
        if (block.valid()) {
          anOstream << block;
        }
      }
      return anOstream;
    }

  };
  // end class MrpSet ====================

  typedef nSetAssoc::SetAssocArray<MrpSet> MrpArray;
  typedef MrpPoliciesType<MrpArray> MrpPolicies;
public:
  typedef MrpLookupResultType<MrpMapping, MrpPolicies> MrpLookupResult;

  typedef typename MrpMapping::Address Address;
  typedef typename MrpMapping::Types::Tag Tag;
  typedef typename MrpMapping::Types::Index Index;

public:
  MrpSigTableType(uint32_t numSets,
                  uint32_t assoc)
    : theMapping(numSets, assoc)
    , theArray(numSets, assoc)
  {}

  MrpLookupResult operator[](const Address & anAddress) {
    return MrpLookupResult::lookup(anAddress, theArray, theMapping);
  }

  Tag makeTag(const Address & anAddress) {
    return theMapping.tag(anAddress);
  }

  Address makeAddress(const Tag & aTag, const Index & anIndex) {
    return theMapping.address(aTag, anIndex);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const MrpSigTableType & aTable) {
    anOstream << "Entries: " << aTable.theMapping.numSets()*aTable.theMapping.assoc()
              << "  Sets: " << aTable.theMapping.numSets()
              << "  Assoc: " << aTable.theMapping.assoc();
    return anOstream;
  }

private:
  MrpMapping theMapping;
  MrpArray theArray;

};  // end class MrpSigTableType

}  // end namespace nMrpTable
