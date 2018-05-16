// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED
#define FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED

#include <iostream>

#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

namespace narmDecoder {

struct Condition {
  bool isNegated;
  bool isXcc;
  std::function<bool(std::bitset<8> const & aCC, int)> theTest;

  Condition(bool negate, bool xcc, std::function<bool(std::bitset<8> const & aCC, int)> test)
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
  std::function<bool(std::bitset<8> const & aCC)> theTest;

  FCondition(std::function<bool(std::bitset<8> const & aCC)> test)
    : theTest(test)
  {}
  bool operator()( std::bitset<8> const & aCC) {
    return theTest( aCC );
  }
};
FCondition fcondition(uint32_t aCondition);

struct RCondition {
  bool isNegated;
  std::function<bool(int64_t)> theTest;

  RCondition(bool negate, std::function<bool(int64_t aCC)> test)
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

} //armDecoder

#endif //FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED
