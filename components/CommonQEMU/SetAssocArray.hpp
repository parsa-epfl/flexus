#ifndef SET_ASSOC_ARRAY_HPP_
#define SET_ASSOC_ARRAY_HPP_

#include "SetAssocLookupResult.hpp"

namespace nSetAssoc {

struct Dynamic
{};
struct HighAssociative
{};
struct LowAssociative
{};
struct Infinite
{};

template<class UserDefinedSet, class Implementation = LowAssociative>
class SetAssocArray;
// UserDefinedSet must be:
// derived from BaseSetDefinition
//
// Implementation may be:
// Dynamic
// HighAssociative
// LowAssociative
// Infinite

template<class UserDefinedSet>
class SetAssocArray<UserDefinedSet, LowAssociative>
{

  public:
    // allow others to extract the block and set types
    typedef UserDefinedSet Set;
    typedef typename Set::Block Block;

    // convenience typedefs for basic types
    typedef typename Set::Tag Tag;
    typedef typename Set::BlockNumber BlockNumber;
    typedef typename Set::Index Index;

    typedef aux_::LowAssociativityLookupResult<Set> BaseLookupResult;

  private:
    // The number of sets
    uint32_t myNumSets;

    // The associativity of each set
    uint32_t myAssoc;

    // The actual sets (which contain blocks and hence data)
    Set* theSets;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const uint32_t version)
    {
        ar & myNumSets;
        for (uint32_t i = 0; i < myNumSets; ++i) {
            ar& theSets[i];
        }
    }

  public:
    // CONSTRUCTION
    SetAssocArray(uint32_t aNumSets, uint32_t anAssoc)
      : myNumSets(aNumSets)
      , myAssoc(anAssoc)
    {
        theSets = (Set*)(new char[myNumSets * sizeof(Set)]);
        // this uses placement new, so on delete, make sure to manually call
        // the destructor for each Set, then delete[] the array
        for (uint32_t ii = 0; ii < myNumSets; ii++) {
            new (theSets + ii) Set(myAssoc, Index(ii));
        }

        // DBG_( VVerb, ( << "Constructed a dynamic set-associative array" ) );
    }

    // SET LOOKUP
    Set& operator[](const Index& anIndex)
    {
        // TODO: bounds checking/assertion?
        return theSets[anIndex];
    }
    const Set& operator[](const Index& anIndex) const
    {
        // TODO: bounds checking/assertion?
        return theSets[anIndex];
    }

}; // end class SetAssocArray

} // end namespace nSetAssoc

#endif
