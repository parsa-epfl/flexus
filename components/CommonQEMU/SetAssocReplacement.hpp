#ifndef SET_ASSOC_REPLACEMENT_HPP_
#define SET_ASSOC_REPLACEMENT_HPP_

#include <boost/serialization/base_object.hpp>

namespace nSetAssoc {

template < class BaseSet >
class LowAssociativeMRUSet : public BaseSet {
public:

  // convenience typedefs
  typedef typename BaseSet::Tag Tag;
  typedef typename BaseSet::BlockNumber BlockNumber;
  typedef typename BaseSet::Index Index;
  typedef typename BaseSet::Block Block;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & boost::serialization::base_object<BaseSet>(*this);
    for (uint32_t i = 0; i < this->myAssoc; ++i) {
      ar & mruList[i];
    }
  }

  LowAssociativeMRUSet(const uint32_t anAssoc,
                       const Index anIndex)
    : BaseSet(anAssoc, anIndex)
    , mruList(0) {
    init();
  }

  void init() {
    uint32_t ii;
    mruList = (BlockNumber *)( new char[this->myAssoc * sizeof(BlockNumber)] );
    // this uses placement new, so on delete, make sure to manually call
    // the destructor for each BlockNumber, then delete[] the array
    for (ii = 0; ii < this->myAssoc; ii++) {
      new(mruList + ii) BlockNumber(ii);
    }
  }

  // returns the most recently used block
  BlockNumber listHead() {
    return mruList[0];
  }

  // returns the least recently used block
  BlockNumber listTail() {
    return mruList[this->myAssoc-1];
  }

  // moves the indicated block to the MRU position
  void moveToHead(BlockNumber aNumber) {
    int32_t ii = 0;
    // find the list entry for the specified index
    while (mruList[ii] != aNumber) {
      ii++;
    }
    // move appropriate entries down the MRU chain
    while (ii > 0) {
      mruList[ii] = mruList[ii-1];
      ii--;
    }
    mruList[0] = aNumber;
  }

  // moves the indicated block to the LRU position
  void moveToTail(BlockNumber aNumber) {
    uint32_t ii = 0;
    // find the list entry for the specified index
    while (mruList[ii] != aNumber) {
      ii++;
    }
    // move appropriate entries up the MRU chain
    while (ii < this->myAssoc - 1) {
      mruList[ii] = mruList[ii+1];
      ii++;
    }
    mruList[this->myAssoc-1] = aNumber;
  }

  // The list itself.  This is an array of indices corresponding to
  // blocks in the set.  The list is arranged in order of most recently
  // used to least recently used.
  BlockNumber * mruList;

};  // end struct MruSetExtension

}  // end namespace nSetAssoc

#endif //SET_ASSOC_REPLACEMENT_HPP_
