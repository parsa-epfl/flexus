#ifndef SET_ASSOC_LOOKUP_RESULT_HPP_
#define SET_ASSOC_LOOKUP_RESULT_HPP_

#include <core/exception.hpp>

namespace nSetAssoc {
namespace aux_ {

template<class UserDefinedSet>
struct BaseLookupResult
{
    typedef UserDefinedSet Set;
    typedef typename Set::Block Block;
    typedef typename Set::Tag Tag;

  public:
    //! HIT/MISS check
    bool hit() const { return isHit; }
    bool miss() const { return !isHit; }

    //! SET access
    Set& set() { return theSet; }
    const Set& set() const { return theSet; }

    //! BLOCK access
    Block& block()
    {
        if (theBlock) { return *theBlock; }
        throw Flexus::Core::FlexusException("no block");
    }
    const Block& block() const
    {
        if (theBlock) { return *theBlock; }
        throw Flexus::Core::FlexusException("no block");
    }

  protected:
    //! CONSTRUCTORS
    BaseLookupResult(Set& aSet, Block& aBlock, bool aHit)
      : theSet(aSet)
      , theBlock(&aBlock)
      , isHit(aHit)
    {
    }
    BaseLookupResult(Set& aSet, bool aHit)
      : theSet(aSet)
      , theBlock(0)
      , isHit(aHit)
    {
    }

    //! MEMBERS
    Set& theSet;
    Block* theBlock;
    bool isHit;
};

template<class UserDefinedSet>
struct LowAssociativityLookupResult : public BaseLookupResult<UserDefinedSet>
{

    //! CREATION
    static typename LowAssociativityLookupResult::Block* find(typename LowAssociativityLookupResult::Set& aSet,
                                                              const typename LowAssociativityLookupResult::Tag& aTag)
    {
        for (typename LowAssociativityLookupResult::Set::BlockIterator iter = aSet.blocks_begin();
             iter != aSet.blocks_end();
             ++iter) {
            if (iter->tag() == aTag) {
                if (iter->valid()) { return iter; }
            }
        }
        return 0;
    }

  protected:
    //! CONSTRUCTORS
    LowAssociativityLookupResult(typename LowAssociativityLookupResult::Set& aSet,
                                 typename LowAssociativityLookupResult::Block& aBlock,
                                 bool aHit)
      : BaseLookupResult<UserDefinedSet>(aSet, aBlock, aHit)
    {
    }
    LowAssociativityLookupResult(typename LowAssociativityLookupResult::Set& aSet, bool aHit)
      : BaseLookupResult<UserDefinedSet>(aSet, aHit)
    {
    }
};

} // namespace aux_
} // namespace nSetAssoc

#endif // SET_ASSOC_LOOKUP_RESULT_HPP_
