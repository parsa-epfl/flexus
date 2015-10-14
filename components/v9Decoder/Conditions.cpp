#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <functional>
#include <functional>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "Conditions.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

static const int32_t Z = nuArch::iccZ;
static const int32_t V = nuArch::iccV;
static const int32_t C = nuArch::iccC;
static const int32_t N = nuArch::iccN;

static const uint32_t E = nuArch::fccE;
static const uint32_t L = nuArch::fccL;
static const uint32_t G = nuArch::fccG;
static const uint32_t U = nuArch::fccU;

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
               , [](auto& x, auto y){ return true; } //Binary functor which always returns true
             );
    case 1: //Equal:  Z
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(Z+y); }//ll::bind( &std::bitset<8>::test, _1, Z + _2 )
             );
    case 2: //LessOrEqual: Z or ( N xor V )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(Z+y) | (x.test(N+y) ^ x.test(V+y)); }//ll::bind( &std::bitset<8>::test, _1, Z + _2 ) | ( ll::bind( &std::bitset<8>::test, _1, N + _2 ) ^ ll::bind( &std::bitset<8>::test, _1, V + _2 ) )
             );
    case 3: //Less: ( N xor V )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(N+y) ^ x.test(V+y); }//ll::bind( &std::bitset<8>::test, _1, N + _2 ) ^ ll::bind( &std::bitset<8>::test, _1, V + _2 )
             );
    case 4: //LessOrEqualUnsigned: ( C or Z )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(C+y) | x.test(Z+y); }//ll::bind( &std::bitset<8>::test, _1, C + _2 ) | ll::bind( &std::bitset<8>::test, _1, Z + _2 )
             );
    case 5: //CarrySet: ( C )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(C+y); }//ll::bind( &std::bitset<8>::test, _1, C + _2 )
             );
    case 6: //Negative: ( N )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(N+y); }//ll::bind( &std::bitset<8>::test, _1, N + _2 )
             );
    case 7: //OverflowSet: ( V )
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(V+y); }//ll::bind( &std::bitset<8>::test, _1, V + _2 )
             );
    default:
      DBG_Assert(false);
      //Suppress warning:
      return Condition
             ( aNegate
               , Xcc
               , [](auto& x, auto y){ return x.test(Z+y); }//ll::bind( &std::bitset<8>::test, _1, Z + _2 )
             );
  }
}

FCondition fcondition(uint32_t aCondition) {
  switch (aCondition & 0xF) {
    case 0: //Never
      return FCondition
             ( [](auto& x){ return false; }//_1 != _1 //unary functor which returns false
             );
    case 1: //NotEqual : L or G or U
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L || x_n == G || x_n == U; }
                //( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 2: //LessOrGreator: L or G
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L || x_n == G; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
             );
    case 3: //UnorderedOrLess: ( L or U )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L || x_n == U; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 4: //Less
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L; }//( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
             );
    case 5: //UnorderedOrGreater ( G or U )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G || x_n == U; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 6: //Greater ( G )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G; } //( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
             );
    case 7: //Unordered ( U )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == U; } //( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 8: //Always
      return FCondition
             ( [](auto& x){ return true; } //_1 == _1 //unary functor which returns true
             );
    case 9: //Equal: E
      return FCondition
             ( [](auto& x){ auto x_n = x.to_ulong(); return x_n == E; } //ll::bind( &std::bitset<8>::to_ulong, _1 ) == E
             );
    case 10: //UnorderedOrEqual ( U or E )
      return FCondition
             ( [](auto& x){ auto x_n = x.to_ulong(); return x_n == U || x_n == E; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
             );
    case 11: //GreaterOrEqual ( G or E )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G || x_n == E; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
             );
    case 12: //UnorderedGreaterOrEqual ( U or G or E )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G || x_n == E || x_n == U; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 13: //LessOrEqual ( L or E )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L || x_n == E; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
             );
    case 14: //UnorderedLessOrEqual ( U or L or E )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == L || x_n == E || x_n == U; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == U )
             );
    case 15: //Ordered( L or G or E )
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G || x_n == E || x_n == L; }
                // ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == E )
                // || ( ll::bind( &std::bitset<8>::to_ulong, _1 ) == L )
             );

    default:
      DBG_Assert(false);
      //Suppress warning:
      return FCondition
             (  [](auto& x){ auto x_n = x.to_ulong(); return x_n == G; } //( ll::bind( &std::bitset<8>::to_ulong, _1 ) == G )
             );
  }
}

RCondition rcondition(uint32_t aCondition) {
  bool negate = aCondition & 0x4;
  switch (aCondition & 0x3) {
    case 1: //EqualZero
      return RCondition
             ( negate
               , [](auto x){ return x == 0LL; } //_1 == 0LL
             );
    case 2: //LessOrEqualZero
      return RCondition
             ( negate
               , [](auto x){ return x <= 0LL; } //_1 <= 0LL
             );
    case 3: //LessZero
      return RCondition
             ( negate
               , [](auto x){ return x < 0LL; } // _1 < 0LL
             );
    case 0: //Reserved
    default:
      DBG_Assert(false);
      //Suppress warning:
      return RCondition
             ( negate
               , [](auto x){ return x == 0LL; } // _1 == 0LL
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

} //nv9Decoder
