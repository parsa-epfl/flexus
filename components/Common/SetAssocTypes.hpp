#ifndef SET_ASSOC_TYPES_HPP_
#define SET_ASSOC_TYPES_HPP_

namespace nSetAssoc {

template<class UnderlyingType>
class SimpleTag {
public:
  //Underlying type must be:
  //Assignable
  //Copy-constructible
  //Default-constructible
  //Comparable
  //writable to an ostream

  SimpleTag () {} //Can only be used if UnderlyingType is default-constructable
  explicit SimpleTag(UnderlyingType newTag)
    : tag(newTag)
  {}
  SimpleTag(const SimpleTag & aTag)
    : tag(aTag.tag)
  {}
  SimpleTag & operator= (const SimpleTag & aTag) {
    tag = aTag.tag;
    return *this;
  }
  operator UnderlyingType () const {
    return tag;
  }
  bool operator ==(SimpleTag const & anOther) const {
    return tag == anOther.tag;
  }
  friend std::ostream & operator <<(std::ostream & anOstream, SimpleTag const & aTag) {
    anOstream << aTag.tag;
    return anOstream;
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & tag;
  }

private:
  UnderlyingType tag;
};

class SimpleIndex  {
public:
  explicit SimpleIndex (int32_t newIndex)
    : index(newIndex)
  {}
  SimpleIndex(const SimpleIndex & anIndex)
    : index(anIndex.index)
  {}
  SimpleIndex & operator= (const SimpleIndex & anIndex) {
    index = anIndex.index;
    return *this;
  }
  operator int32_t () const {
    return index;
  }
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & index;
  }

private:
  int32_t index;
};

class SimpleBlockNumber {
public:
  explicit SimpleBlockNumber(int32_t newNumber)
    : number(newNumber)
  {}
  SimpleBlockNumber(const SimpleBlockNumber & aBlockNumber)
    : number(aBlockNumber.number)
  {}
  SimpleBlockNumber & operator= (const SimpleBlockNumber & aBlockNumber) {
    number = aBlockNumber.number;
    return *this;
  }
  SimpleBlockNumber & operator= (int32_t newNumber) {
    number = newNumber;
    return *this;
  }
  operator int32_t () const {
    return number;
  }
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & number;
  }

private:
  int32_t number;
};

}  // end namespace nSetAssoc

#endif
