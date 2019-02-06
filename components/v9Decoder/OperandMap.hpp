#ifndef FLEXUS_v9DECODER_OPERANDMAP_HPP_INCLUDED
#define FLEXUS_v9DECODER_OPERANDMAP_HPP_INCLUDED

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/list.hpp>

#include <components/uArch/uArchInterfaces.hpp>
#include "OperandCode.hpp"

namespace nv9Decoder {

std::ostream & operator << ( std::ostream &, std::bitset<8> const & );

using nuArch::register_value;
using nuArch::mapped_reg;
using nuArch::unmapped_reg;

typedef
mpl::push_front
< register_value::types
, mapped_reg
>::type operand_typelist_1;

typedef
mpl::push_front
< operand_typelist_1
, unmapped_reg
>::type operand_typelist;

typedef boost::make_variant_over< operand_typelist >::type Operand;

class OperandMap {

  typedef std::map< eOperandCode, Operand > operand_map;
  operand_map theOperandMap;

public:
  template <class T>
  T & operand( eOperandCode anOperandId ) {
    return boost::get<T>(theOperandMap[ anOperandId ] );
  }

  Operand & operand( eOperandCode anOperandId ) {
    return theOperandMap[ anOperandId ];
  }

  template <class T>
  void set( eOperandCode anOperandId, T aT ) {
    theOperandMap[ anOperandId ] = aT;
  }

  void set( eOperandCode anOperandId, Operand const & aT ) {
    theOperandMap[ anOperandId ] = aT;
  }

  bool hasOperand( eOperandCode anOperandId) const {
    return (theOperandMap.find(anOperandId) != theOperandMap.end());
  }

  struct operandPrinter {
    std::ostream & theOstream;
    operandPrinter(std::ostream & anOstream)
      : theOstream(anOstream)
    {}
    void operator()( operand_map::value_type const & anEntry) {
      theOstream << "\t   " << anEntry.first << " = " << anEntry.second << "\n";
    }
  };

  void dump( std::ostream & anOstream) const {
    std::for_each
    ( theOperandMap.begin()
      , theOperandMap.end()
      , operandPrinter(anOstream)
    );
  }

};

} //nv9Decoder

#endif //FLEXUS_v9DECODER_OPERANDMAP_HPP_INCLUDED
