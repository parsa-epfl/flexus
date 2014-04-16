namespace nDgpTable {

enum DgpAccessType {
  WriteAccess,
  SnoopAccess,
  Eviction,
  DontCare
};

template < class Tag >
class DgpBlockType {
public:
  DgpBlockType()
    : myTag(0)
    , myValid(0)
  {}

  const Tag & tag() const {
    return myTag;
  }
  Tag & tag() {
    return myTag;
  }
  bool valid() const {
    return myValid;
  }
  void setValid(bool value) {
    myValid = value;
  }
  const Confidence & conf() const {
    return myConf;
  }
  Confidence & conf() {
    return myConf;
  }

  template<class T>
  friend std::ostream & operator << (std::ostream & anOstream, const DgpBlockType<T> & aBlock) {
    return anOstream << "[tag=0x" << std::hex << aBlock.myTag << " v="
           << (int)aBlock.myValid << " conf=" << aBlock.myConf << "] ";
  }

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & myTag;
    ar & myValid;
    ar & myConf;
  }

  Tag myTag;
  uint8_t myValid;
  Confidence myConf;
};  // end class DgpBlockType

template < class Mapping
, class Policies
>
class DgpLookupResultType
    : public Policies::ArrayType::BaseLookupResult {
  typedef typename Mapping::Address Address;
  typedef typename Policies::ArrayType ArrayType;
  typedef typename Policies::AccessPolicy AccessPolicy;
  typedef typename Policies::ReplacementPolicy ReplacementPolicy;
  typedef typename ArrayType::BaseLookupResult BaseLookupResult;
  typedef typename ArrayType::Tag Tag;
  typedef typename ArrayType::Index Index;
  typedef typename ArrayType::Set Set;
  typedef typename ArrayType::Block Block;

public:

  // CREATION
  static DgpLookupResultType lookup(const Address & anAddress,
                                    ArrayType & anArray,
                                    Mapping & aMapping) {
    Set & set = anArray[ aMapping.index(anAddress) ];
    DBG_(VVerb, ( << "lookup for addr=" << std::hex << anAddress << " set=" << set.index() ) );
    Block * block = BaseLookupResult::find( set, aMapping.tag(anAddress) );
    if (block) {
      return DgpLookupResultType(aMapping, set, *block, true);
    }
    return DgpLookupResultType(aMapping, set, false);
  }

  // ADDRESSING
  Address address() {
    return theMapping.address(this->block().tag(), this->set().index());
  }

  //! CAUSE ARRAY STATE SIDE EFFECTS
  void access(const DgpAccessType & anAccess) {
    DBG_(VVerb, ( << "Performing set-assoc side effects for an access of type " << anAccess ) );
    AccessPolicy::access(*this, anAccess);
    ReplacementPolicy::perform(this->theSet, *(this->theBlock), anAccess);
  }

  // Find a victim block in the set, setting the Block reference in this
  // LookupResult to that victim block
  Block & victim() {
    Block & tempBlock = ReplacementPolicy::victim(this->theSet);
    this->theBlock = &tempBlock;
    return tempBlock;
  }

  // Remove a signature (mark a block as invalid)
  void remove() {
    this->theBlock->setValid(false);
    ReplacementPolicy::perform(this->theSet, *(this->theBlock), Eviction);
  }

private:
  // CONSTRUCTORS
  DgpLookupResultType(Mapping & aMapping,
                      Set & aSet,
                      Block & aBlock,
                      bool aHit)
    : BaseLookupResult(aSet, aBlock, aHit)
    , theMapping(aMapping)
  {}
  DgpLookupResultType(Mapping & aMapping,
                      Set & aSet,
                      bool aHit)
    : BaseLookupResult(aSet, aHit)
    , theMapping(aMapping)
  {}

  // save the mapping to perform addresses in the future
  Mapping & theMapping;

};  // end class DgpLookupResultType

}  // end namespace nDgpTable
