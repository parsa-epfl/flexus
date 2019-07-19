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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

#include <iomanip>
#include <iostream>

#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

struct ReverseAction : public PredicatedSemanticAction {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;
  bool the64;

  ReverseAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
                eOperandCode anOutputCode, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true), theInputCode(anInputCode),
        theOutputCode(anOutputCode), the64(is64) {
  }

  void doEvaluate() {
    SEMANTICS_DBG(*this);

    Operand in = theInstruction->operand(theInputCode);
    bits in_val = boost::get<bits>(in);
    uint8_t dbsize = the64 ? 64 : 32;
    bits out_val = 0;
    for (int i = 0; i < dbsize; i++) {
      if ((in_val & 1 << i) == 1) {
        out_val |= (1 << (dbsize - i));
      }
    }

    theInstruction->setOperand(theOutputCode, out_val);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReverseAction ";
  }
};

struct ReorderAction : public PredicatedSemanticAction {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;
  uint8_t theContainerSize;
  bool the64;

  ReorderAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
                eOperandCode anOutputCode, uint8_t aContainerSize, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true), theInputCode(anInputCode),
        theOutputCode(anOutputCode), theContainerSize(aContainerSize), the64(is64) {
  }

  void doEvaluate() {
    SEMANTICS_DBG(*this);

    if (ready()) {
      Operand in = theInstruction->operand(theInputCode);
      uint64_t in_val = boost::get<uint64_t>(in);
      uint64_t out_val = 0;

      switch (theContainerSize) {
      case 16:
        out_val = ((uint64_t)(__builtin_bswap16((uint16_t)in_val))) |
                  (((uint64_t)(__builtin_bswap16((uint16_t)(in_val >> 16)))) << 16);
        if (the64)
          out_val |= (((uint64_t)(__builtin_bswap16((uint16_t)(in_val >> 32)))) << 32) |
                     (((uint64_t)(__builtin_bswap16((uint16_t)(in_val >> 48)))) << 48);
        break;
      case 32:
        out_val = ((uint64_t)(__builtin_bswap32((uint32_t)in_val)));
        if (the64)
          out_val |= (((uint64_t)(__builtin_bswap32((uint32_t)(in_val >> 32)))) << 32);
        break;
      case 64:
        out_val = (__builtin_bswap64((uint64_t)in_val));
        break;
      }

      theInstruction->setOperand(theOutputCode, out_val);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReverseAction ";
  }
};

struct CountAction : public PredicatedSemanticAction {
  eOperandCode theInputCode;
  eOperandCode theOutputCode;
  eCountOp theCountOp;
  bool the64;

  CountAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
              eOperandCode anOutputCode, eCountOp aCountOp, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true), theInputCode(anInputCode),
        theOutputCode(anOutputCode), theCountOp(aCountOp), the64(is64) {
  }

  void doEvaluate() {
    if (ready()) {
      SEMANTICS_DBG(*this);

      Operand in = theInstruction->operand(theInputCode);
      uint64_t in_val = boost::get<uint64_t>(in);
      uint64_t out_val = 0;

      switch (theCountOp) {
      case kCountOp_CLZ:
        out_val = the64 ? ((clz32((uint32_t)in_val >> 32) == 0)
                               ? (32 + clz32((uint32_t)in_val))
                               : (clz32((uint32_t)in_val >> 32) + clz32((uint32_t)in_val)))
                        : (clz32((uint32_t)in_val));
        break;
      case kCountOp_CLS: {
        bits mask;
        if (the64) {
          mask = 0x7FFFFFFFFFFFFFFFULL;
        } else {
          mask = 0x000000007FFFFFFFULL;
        }
        bits i = (in_val >> 1) ^ (in_val & mask);
        out_val = the64 ? ((clz32((uint32_t)i >> 32) == 0)
                               ? (31 + clz32((uint32_t)i))
                               : (clz32((uint32_t)i >> 32) + clz32((uint32_t)i)))
                        : (clz32((uint32_t)i));
        break;
      }
      default:
        DBG_Assert(false);
        break;
      }

      DBG_(Iface, (<< "writing " << out_val << " into " << theOutputCode));
      theInstruction->setOperand(theOutputCode, out_val);
      satisfyDependants();
    }
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReverseAction ";
  }
};

struct CRCAction : public PredicatedSemanticAction {
  eOperandCode theInputCode, theInputCode2;
  eOperandCode theOutputCode;
  uint32_t thePoly;
  bool the64;

  CRCAction(SemanticInstruction *anInstruction, uint32_t aPoly, eOperandCode anInputCode,
            eOperandCode anInputCode2, eOperandCode anOutputCode, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true), theInputCode(anInputCode),
        theOutputCode(anOutputCode), thePoly(aPoly), the64(is64) {
  }

  uint32_t bitReverse(uint32_t input) {
    uint32_t out_val = 0;
    for (int i = 0; i < 32; i++) {
      if ((input & (1 << i)) == 1) {
        out_val |= (1 << (32 - i));
      }
    }
    return out_val;
  }

  void doEvaluate() {
    SEMANTICS_DBG(*this);

    Operand in = theInstruction->operand(theInputCode);
    Operand in2 = theInstruction->operand(theInputCode2);
    uint32_t acc = (uint32_t)(boost::get<bits>(in));
    // bits val = boost::get<bits> (in2);

    bits tempacc = (bits)((bitReverse(acc)) << (the64 ? 64 : 32));
    bits tempval = 0; //(bits)((bitReverse(val)) << 32) ;

    // Poly32Mod2 on a bitstring does a polynomial Modulus over {0,1} operation;

    tempacc ^= tempval;
    bits data = tempacc;

    //    for (int i = data.size() -1; i >= 32; i--){
    //        if (data[i] == 1){
    //            data.resize(32 + (i-32));
    //            data ^= thePoly << (i-32);
    //            break;
    //        }
    //    }

    data &= 0xffffffff;

    theInstruction->setOperand(theOutputCode, data);
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " ReverseAction ";
  }
};

predicated_action reverseAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
                                eOperandCode anOutputCode, bool is64) {
  ReverseAction *act(new (anInstruction->icb())
                         ReverseAction(anInstruction, anInputCode, anOutputCode, is64));
  return predicated_action(act, act->predicate());
}

predicated_action reorderAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
                                eOperandCode anOutputCode, uint8_t aContainerSize,
                                std::vector<std::list<InternalDependance>> &rs_deps, bool is64) {
  ReorderAction *act(new (anInstruction->icb()) ReorderAction(anInstruction, anInputCode,
                                                              anOutputCode, aContainerSize, is64));
  for (uint32_t i = 0; i < rs_deps.size(); ++i) {
    rs_deps[i].push_back(act->dependance(i));
  }
  return predicated_action(act, act->predicate());
}

predicated_action countAction(SemanticInstruction *anInstruction, eOperandCode anInputCode,
                              eOperandCode anOutputCode, eCountOp aCountOp,
                              std::vector<std::list<InternalDependance>> &rs_deps, bool is64) {
  CountAction *act(new (anInstruction->icb())
                       CountAction(anInstruction, anInputCode, anOutputCode, aCountOp, is64));
  for (uint32_t i = 0; i < rs_deps.size(); ++i) {
    rs_deps[i].push_back(act->dependance(i));
  }

  return predicated_action(act, act->predicate());
}
predicated_action crcAction(SemanticInstruction *anInstruction, uint32_t aPoly,
                            eOperandCode anInputCode, eOperandCode anInputCode2,
                            eOperandCode anOutputCode, bool is64) {
  CRCAction *act(new (anInstruction->icb()) CRCAction(anInstruction, aPoly, anInputCode,
                                                      anInputCode2, anOutputCode, is64));
  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
