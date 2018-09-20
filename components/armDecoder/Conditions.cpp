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

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uArchARM/RegisterType.hpp>
#include "components/uArchARM/CoreModel/PSTATE.hpp"

#include "SemanticActions.hpp"
#include "OperandMap.hpp"
#include "Conditions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {
using namespace nuArchARM;

bool ConditionHolds(const PSTATE & pstate, int condcode )
{
    bool result;
    switch (condcode) {
    case 0: // EQ or NE
        result =  pstate.Z();
    case 1: // CS or CC
        result =  pstate.C();
    case 2: // MI or PL
        result =  pstate.N();
    case 3: // VS or VC
        result =  pstate.V();
    case 4: // HI or LS
        result =  (pstate.C() && pstate.Z() == 0);
    case 5: // GE or LT
        result =  (pstate.N() == pstate.V());
    case 6: // GT or LE
        result =  ((pstate.N() == pstate.V()) && (pstate.Z() == 0));
    case 7: // AL
        result =  true;
    default:
        DBG_Assert(false);
        break;
    }

    // Condition flag values in the set '111x' indicate always true
    // Otherwise, invert condition if necessary.

    if (((condcode & 1) == 0) && (condcode != 15 /*1111*/))
        return !result;
    else
        return result;
}


typedef struct CBZ : public Condition {
    CBZ(){}
    virtual ~CBZ(){}
  virtual bool operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]).none();
  }
  virtual char const * describe() const {
    return "Compare and Branch on Zero";
  }
}CBZ;

typedef struct CBNZ : public Condition {
    CBNZ(){}
    virtual ~CBNZ(){}
  virtual bool operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]).any();
  }
  virtual char const * describe() const {
    return "Compare and Branch on Non Zero";
  }
}CBNZ;

typedef struct TBZ : public Condition {
    TBZ(){}
    virtual ~TBZ(){}
  virtual bool operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return (boost::get<bits>(operands[0]) & boost::get<bits>(operands[0])).none();
  }
  virtual char const * describe() const {
    return "Test and Branch on Zero";
  }
}TBZ;

typedef struct TBNZ : public Condition {
    TBNZ(){}
    virtual ~TBNZ(){}
  virtual bool operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return (boost::get<bits>(operands[0]) & boost::get<bits>(operands[0])).any();
  }
  virtual char const * describe() const {
    return "Test and Branch on Non Zero";
  }
}TBNZ;


typedef struct BCOND : public Condition {
    BCOND(){}
    virtual ~BCOND(){}
  virtual bool operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);

    PSTATE p (theInstruction->core()->getPSTATE());
    return ConditionHolds(p, boost::get<uint64_t>(operands[0]) );
  }
  virtual char const * describe() const {
    return "Branch COnditionally";
  }
}BCOND;


std::unique_ptr<Condition> condition(eCondCode aCond) {
    switch(aCond)
    {
    case kCBZ_:
        return std::make_unique<CBZ>();
    case kCBNZ_:
        return std::make_unique<CBNZ>();
    case kTBZ_:
        return std::make_unique<TBZ>();
    case kTBNZ_:
         return std::make_unique<TBNZ>();
    case kBCOND_:
         return std::make_unique<BCOND>();
    }
}





} //armDecoder