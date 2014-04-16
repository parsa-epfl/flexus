#ifndef FLEXUS_v9DECODER_CONDITIONS_HPP_INCLUDED
#define FLEXUS_v9DECODER_CONDITIONS_HPP_INCLUDED

#include <iostream>

#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

namespace nv9Decoder {

struct Condition {
  bool isNegated;
  bool isXcc;
  boost::function<bool(std::bitset<8> const & aCC, int)> theTest;

  Condition(bool negate, bool xcc, boost::function<bool(std::bitset<8> const & aCC, int)> test)
    : isNegated(negate)
    , isXcc(xcc)
    , theTest(test)
  {}
  bool operator()( std::bitset<8> const & aCC) {
    bool cond = theTest( aCC, ( isXcc ? 4 :  0) );
    return cond ^ isNegated;
  }
};
Condition condition(bool aNegate, bool Xcc, uint32_t aCondition);
Condition condition(uint32_t aPackedCondition);
uint32_t packCondition(bool aFloating, bool Xcc, uint32_t aCondition);
bool isFloating(uint32_t aPackedCondition);

struct FCondition {
  boost::function<bool(std::bitset<8> const & aCC)> theTest;

  FCondition(boost::function<bool(std::bitset<8> const & aCC)> test)
    : theTest(test)
  {}
  bool operator()( std::bitset<8> const & aCC) {
    return theTest( aCC );
  }
};
FCondition fcondition(uint32_t aCondition);

struct RCondition {
  bool isNegated;
  boost::function<bool(int64_t)> theTest;

  RCondition(bool negate, boost::function<bool(int64_t aCC)> test)
    : isNegated(negate)
    , theTest(test)
  {}
  bool operator()( int64_t aVal) {
    bool cond = theTest( aVal );
    return cond ^ isNegated;
  }
};
RCondition rcondition(uint32_t aCondition);

bool rconditionValid ( uint32_t aCondition );

} //nv9Decoder

#endif //FLEXUS_v9DECODER_CONDITIONS_HPP_INCLUDED
