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


#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "Conditions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

static const int32_t Z = nuArchARM::Z;
static const int32_t V = nuArchARM::V;
static const int32_t C = nuArchARM::C;
static const int32_t N = nuArchARM::N;

static const uint32_t E = nuArchARM::fccE;
static const uint32_t L = nuArchARM::fccL;
static const uint32_t G = nuArchARM::fccG;
static const uint32_t U = nuArchARM::fccU;

uint32_t packCondition(bool aFloating, bool Xcc, uint32_t aCondition) {
  uint32_t ret_val = aCondition;
  if (Xcc) {
    ret_val |= 0x10;
  }
  if (aFloating) {
    ret_val |= 0x20;
  }
  return ret_val;
}

bool isFloating( uint32_t aPackedCondition) {
  return aPackedCondition & 0x20;
}

Condition condition(uint32_t aCondition) {
  return condition( aCondition & 0x8, aCondition & 0x10, (aCondition & 0x7) );
}

Condition condition(bool aNegate, bool Xcc, uint32_t aCondition) {
  DBG_Assert (aCondition <= 7);
  switch (aCondition) {
    case 0: //Always
      return Condition
             ( aNegate
               , Xcc
               , ll::_2 == ll::_2 //Binary functor which always returns true
             );
    case 1: //Equal:  Z
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, Z + ll::_2 )
             );
    case 2: //LessOrEqual: Z or ( N xor V )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, Z + ll::_2 ) | ( ll::bind( &std::bitset<8>::test, ll::_1, N + ll::_2 ) ^ ll::bind( &std::bitset<8>::test, ll::_1, V + ll::_2 ) )
             );
    case 3: //Less: ( N xor V )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, N + ll::_2 ) ^ ll::bind( &std::bitset<8>::test, ll::_1, V + ll::_2 )
             );
    case 4: //LessOrEqualUnsigned: ( C or Z )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, C + ll::_2 ) | ll::bind( &std::bitset<8>::test, ll::_1, Z + ll::_2 )
             );
    case 5: //CarrySet: ( C )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, C + ll::_2 )
             );
    case 6: //Negative: ( N )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, N + ll::_2 )
             );
    case 7: //OverflowSet: ( V )
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, V + ll::_2 )
             );
    default:
      DBG_Assert(false);
      //Suppress warning:
      return Condition
             ( aNegate
               , Xcc
               , ll::bind( &std::bitset<8>::test, ll::_1, Z + ll::_2 )
             );
  }
}

FCondition fcondition(uint32_t aCondition) {
  switch (aCondition & 0xF) {
    case 0: //Never
      return FCondition
             ( ll::_1 != ll::_1 //unary functor which returns false
             );
    case 1: //NotEqual : L or G or U
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 2: //LessOrGreator: L or G
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
             );
    case 3: //UnorderedOrLess: ( L or U )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 4: //Less
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
             );
    case 5: //UnorderedOrGreater ( G or U )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 6: //Greater ( G )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
             );
    case 7: //Unordered ( U )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 8: //Always
      return FCondition
             ( ll::_1 == ll::_1 //unary functor which returns true
             );
    case 9: //Equal: E
      return FCondition
             ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E
             );
    case 10: //UnorderedOrEqual ( U or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
             );
    case 11: //GreaterOrEqual ( G or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
             );
    case 12: //UnorderedGreaterOrEqual ( U or G or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 13: //LessOrEqual ( L or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
             );
    case 14: //UnorderedLessOrEqual ( U or L or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == U )
             );
    case 15: //Ordered( L or G or E )
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == E )
                || ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == L )
             );

    default:
      DBG_Assert(false);
      //Suppress warning:
      return FCondition
             (  ( ll::bind( &std::bitset<8>::to_ulong, ll::_1 ) == G )
             );
  }
}

RCondition rcondition(uint32_t aCondition) {
  bool negate = aCondition & 0x4;
  switch (aCondition & 0x3) {
    case 1: //EqualZero
      return RCondition
             ( negate
               , ll::_1 == 0LL
             );
    case 2: //LessOrEqualZero
      return RCondition
             ( negate
               , ll::_1 <= 0LL
             );
    case 3: //LessZero
      return RCondition
             ( negate
               , ll::_1 < 0LL
             );
    case 0: //Reserved
    default:
      DBG_Assert(false);
      //Suppress warning:
      return RCondition
             ( negate
               , ll::_1 == 0LL
             );
  }
}

bool rconditionValid ( uint32_t aCondition ) {

  switch ( aCondition & 0x3 ) {
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }

}

} //armDecoder
