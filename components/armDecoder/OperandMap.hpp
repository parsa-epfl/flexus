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


#ifndef FLEXUS_armDECODER_OPERANDMAP_HPP_INCLUDED
#define FLEXUS_armDECODER_OPERANDMAP_HPP_INCLUDED

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/list.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>
#include "OperandCode.hpp"

namespace narmDecoder {

std::ostream & operator << ( std::ostream &, std::bitset<8> const & );

using nuArchARM::register_value;
using nuArchARM::mapped_reg;
using nuArchARM::reg;

typedef
mpl::push_front
< register_value::types
, mapped_reg
>::type operand_typelist_1;

typedef
mpl::push_front
< operand_typelist_1
, reg
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

} //narmDecoder

#endif //FLEXUS_armDECODER_OPERANDMAP_HPP_INCLUDED
