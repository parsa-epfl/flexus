#ifndef SET_ASSOC_SET_DEFN_HPP_
#define SET_ASSOC_SET_DEFN_HPP_

namespace nSetAssoc {

template
< class Types
, class TBlock
>
class BaseSetDefinition {
  //Types must provide
  // a Tag typedef
  // a BlockNumber typedef
  // an Index typedef
  //TBlock should be the block type.
  //It must supply at least these functions:
  //Default-constructor
  //

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & myAssoc;
    ar & myIndex;
    for (uint32_t i = 0; i < myAssoc; ++i) {
      ar & theBlocks[i];
    }
  }

public:

  // convenience typedefs
  typedef typename Types::Tag Tag;
  typedef typename Types::BlockNumber BlockNumber;
  typedef typename Types::Index Index;

  // allow others to extract the block type
  typedef TBlock Block;

  // CONSTRUCTION
  BaseSetDefinition(const uint32_t anAssoc,
                    const Index anIndex)
    : myIndex(anIndex)
    , myAssoc(anAssoc) {
    uint32_t ii;
    theBlocks = (Block *)( new char[myAssoc * sizeof(Block)] );
    // this uses placement new, so on delete, make sure to manually call
    // the destructor for each Block, then delete[] the array
    for (ii = 0; ii < myAssoc; ii++) {
      new(theBlocks + ii) Block();  //No corresponding delete yet
    }
  }

  // BLOCK LOOKUP
  Block & operator[] (const BlockNumber & aBlockNumber) {
    // TODO: bounds checking/assertion?
    return theBlocks[aBlockNumber];
  }
  const Block & operator[] (const BlockNumber & aBlockNumber) const {
    // TODO: bounds checking/assertion?
    return theBlocks[aBlockNumber];
  }

  // INDEXING
  BlockNumber indexOf(const Block & aBlock) const {
    int32_t index = &aBlock - theBlocks;
    DBG_Assert((index >= 0) && (index < (int)myAssoc), ( << "index=" << index ) );
    return BlockNumber(index);
  }
  Index index() const {
    return myIndex;
  }

  // ITERATION
  typedef Block * BlockIterator;

  BlockIterator blocks_begin() {
    return theBlocks;
  }
  const BlockIterator blocks_begin() const {
    return theBlocks;
  }
  BlockIterator blocks_end() {
    return theBlocks + myAssoc;
  }
  const BlockIterator blocks_end() const {
    return theBlocks + myAssoc;
  }

  friend std::ostream & operator << (std::ostream & anOstream, const BaseSetDefinition<Types, TBlock> & aSet) {
    anOstream << "set " << aSet.myIndex << ": ";
    for (BlockIterator iter = aSet.blocks_begin(); iter != aSet.blocks_end(); ++iter) {
      anOstream << *iter;
    }
    return anOstream;
  }

protected:
  // The index of this set
  Index myIndex;

  // The associativity of this set
  uint32_t myAssoc;

  // The actual blocks that belong to this set
  Block * theBlocks;

};  // end BaseSetDefinition

}  // end namespace nSetAssoc

#endif
