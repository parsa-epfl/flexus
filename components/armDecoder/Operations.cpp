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

#include "SemanticActions.hpp"
#include "OperandMap.hpp"
#include "Conditions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

typedef struct ADD : public Operation {
    ADD();
    virtual ~ADD();
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    if (hasOwnOperands())
        operands = theOperands;

    DBG_Assert( operands.size() == 2);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    return op1+op2;
  }
  virtual char const * describe() const {
    return "Add";
  }
} ADD_;

typedef struct ADDS : public Operation {
    ADDS();
    virtual ~ADDS();

  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    bits carry_in = boost::get<bits>(operands[2]);
    uint64_t carry = carry_in[0];
    uint64_t sresult =  boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1]) + carry;
    uint64_t uresult =  boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1]) + carry;
    uint64_t result =  0;
    result &= uresult;

    uint64_t N = (1ul << 63) & result;
    uint64_t Z = ((result == 0) ? 1ul << 62 : 0);
    uint64_t C = ((result - uresult == 0) ? 1ul << 61 : 0);
    uint64_t V = ((result - sresult == 0) ? 1ul << 60 : 0);

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "ADDS";
  }
} ADDS_;

typedef struct SUB : public Operation {
    SUB();
    virtual ~SUB();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "SUB";
  }
} SUB_;

typedef struct SUBS : public Operation {
    SUBS();
    virtual ~SUBS();
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    bits carry_in = boost::get<bits >(operands[2]);
    int32_t carry = carry_in[0];



    int64_t sresult =  (int64_t)boost::get<uint64_t>(operands[0]) + (~(int64_t)boost::get<uint64_t>(operands[1])) + carry;
    uint64_t uresult =  boost::get<uint64_t>(operands[0]) + (~boost::get<uint64_t>(operands[1])) + carry;
    uint64_t result =  0;
    result &= uresult;

    uint64_t N = ((1ul << 63)) & result;
    uint64_t Z = ((result == 0) ? 1ul << 62 : 0);
    uint64_t C = ((result - uresult == 0) ? 1ul << 61 : 0);
    uint64_t V = ((result - sresult == 0) ? 1ul << 60 : 0);

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "SUBS";
  }
} SUBS_;


typedef struct CONCAT : public Operation {
    CONCAT();
    virtual ~CONCAT();
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    bits op1 =  boost::get<bits>(operands[0]);
    bits op2 =  boost::get<bits>(operands[1]);
    bits concat = concat_bits(op1, op2);

    return concat;
  }
  virtual char const * describe() const {
    return "CONCAT";
  }
} CONCAT_;


typedef struct AND : public Operation {
    AND();
    virtual ~AND();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "AND";
  }
} AND_;

typedef struct ANDS : public Operation {
    ANDS();
    virtual ~ANDS();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint64_t result = boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]);

    uint64_t N = ((1ul << 63)) & result;
    uint64_t Z = ((result == 0) ? 1ul << 62 : 0);
    uint64_t C = 0;
    uint64_t V = 0;

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "ANDS";
  }
} ANDS_;

typedef struct ORR : public Operation  {
    ORR();
    virtual ~ORR();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) | boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "ORR";
  }
} ORR_;

typedef struct XOR : public Operation  {
    XOR();
    virtual ~XOR();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "XOR";
  }
} XOR_;



typedef struct AndN : public Operation {

    AndN();
    virtual ~AndN();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) & ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "AndN";
  }
} AndN_;


typedef struct EoN : public Operation  {
    EoN();
    virtual ~EoN();

  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0])  | ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "EoN";
  }
} EoN_;

typedef struct OrN : public Operation  {
    OrN();
    virtual ~OrN();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0])  | ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "OrN";
  }
} OrN_;

typedef struct Not : public Operation  {
    Not();
    virtual ~Not();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "Invert";
  }
} Not_;


typedef struct ROR : public Operation  {
    ROR();
    virtual ~ROR();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "ROR";
  }
} ROR_;

typedef struct LSL : public Operation  {
    LSL();
    virtual ~LSL();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "LSL";
  }
} LSL_;

typedef struct ASR : public Operation  {
    ASR();
    virtual ~ASR();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "ASR";
  }
} ASR_;

typedef struct LSR : public Operation  {
    LSR();
    virtual ~LSR();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<uint64_t>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "LSR";
  }
} LSR_;

typedef struct Sext : public Operation  {
    Sext();
    virtual ~Sext();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0])  | SIGNED_UPPER_BOUND_64;
  }
  virtual char const * describe() const {
    return "singned extend";
  }
} Sext_;

typedef struct Zext : public Operation  {
    Zext();
    virtual ~Zext();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]);
  }
  virtual char const * describe() const {
    return "zero extend";
  }
} Zext_;

typedef struct Xnor : public Operation  {
    Xnor();
    virtual ~Xnor();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "Xnor";
  }
} Xnor_;

typedef struct UMul : public Operation  {
    UMul();
    virtual ~UMul();
  uint64_t calc(std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF;
    uint64_t prod = op0 * op1;
    return prod;
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "UMul";
  }
} UMul_;

typedef struct UMulH : public Operation  {
    UMulH();
    virtual ~UMulH();
  uint64_t calc(std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = (boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF);
    uint64_t op1 = (boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF);
    uint64_t prod = (uint64_t)(((__uint128_t)op0 * op1) >> 64);
    return prod;
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "UMulH";
  }
} UMulH_;

typedef struct SMul : public Operation  {
    SMul();
    virtual ~SMul();
  uint64_t calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFFULL;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFFULL;
    int64_t op0_s = op0 | ( op0 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t op1_s = op1 | ( op1 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t prod = op0_s * op1_s;
    return static_cast<uint64_t>(prod);
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "SMul";
  }
} SMul_;

typedef struct SMulH : public Operation  {
    SMulH();
    virtual ~SMulH();
  uint64_t calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFFULL;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFFULL;
    int64_t op0_s = op0 | ( op0 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t op1_s = op1 | ( op1 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t prod = (int64_t)(((__int128_t)op0_s * op1_s) >> 64);
    return static_cast<uint64_t>(prod);
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "SMulH";
  }
} SMulH_;
typedef struct UDiv : public Operation  {
    UDiv();
    virtual ~UDiv();
  uint64_t calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 3);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF;
    uint64_t y = boost::get<uint64_t>(operands[2]) & 0xFFFFFFFF;
    uint64_t dividend = op0 | (y << 32 );
    if (op1 == 0) {
      return 0ULL; //ret_val is irrelevant when divide by zero
    }
    uint64_t prod = dividend / op1;
    if (prod > 0xFFFFFFFFULL) {
      prod = 0xFFFFFFFFULL;
    }
    return prod;
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "UDiv";
  }
} UDiv_;

typedef struct SDiv : public Operation  {
    SDiv();
    virtual ~SDiv();
  uint64_t calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 3);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF;
    uint64_t y = boost::get<uint64_t>(operands[2]) & 0xFFFFFFFF;
    int64_t dividend = op0 | (y << 32 );
    int64_t divisor = op1 | (op1 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL);
    if (divisor == 0) {
      return 0ULL; //ret_val is irrelevant when divide by zero
    }
    int64_t prod = dividend / divisor;
    if (prod > 0x7FFFFFFF) {
      prod = 0x7FFFFFFFLL;
    } else if (prod < (-1 * 0x7FFFFFFFLL) ) {
      prod = 0xFFFFFFFF80000000LL;
    }
    return prod;
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "SDiv";
  }
} SDiv_;

typedef struct MulX : public Operation  {
    MulX();
    virtual ~MulX();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) * boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "MulX";
  }
} MulX_;

typedef struct UDivX : public Operation  {
    UDivX();
    virtual ~UDivX();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    if (boost::get<uint64_t>(operands[1]) != 0) {
      return boost::get<uint64_t>(operands[0]) / boost::get<uint64_t>(operands[1]);
    } else {
      return 0ULL; //Will result in an exception, so it doesn't matter what we return
    }
  }
  virtual char const * describe() const {
    return "UDivX";
  }
} UDivX_;

typedef struct SDivX : public Operation  {
    SDivX();
    virtual ~SDivX();
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    int64_t op0_s = boost::get<uint64_t>(operands[0]);
    int64_t op1_s = boost::get<uint64_t>(operands[1]);
    if (op1_s != 0) {
      return static_cast<uint64_t>( op0_s / op1_s);
    } else {
      return 0ULL; //Will result in an exception, so it doesn't matter what we return
    }
  }
  virtual char const * describe() const {
    return "SDivX";
  }
} SDivX_;

typedef struct MOV_ : public Operation {
    MOV_();
    virtual ~MOV_();
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "MOV";
  }
} MOV_;

typedef struct MOVN_ : public Operation {
    MOVN_();
    virtual ~MOVN_();
    virtual Operand operator()( std::vector<Operand> const & operands  ) {
      DBG_Assert( operands.size() == 2);
      return ~boost::get<uint64_t>(operands[1]);
    }
  virtual char const * describe() const {
    return "MOVN";
  }
} MOVN_;

typedef struct MOVK_ : public Operation {
    MOVK_();
    virtual ~MOVK_();
    virtual Operand operator()( std::vector<Operand> const & operands  ) {
      DBG_Assert( operands.size() == 2);
      uint64_t rd =  boost::get<uint64_t>(operands[0]);
      uint64_t imm =  boost::get<uint64_t>(operands[1]);

      return (rd & imm) | imm;
    }
  virtual char const * describe() const {
    return "MOVK";
  }
} MOVK_;


//typedef struct LastOp : public Operation {
//    virtual ~LastOp();
//  virtual Operand operator()( std::vector<Operand> const & operands  ) {
//    DBG_Assert(false);
//    return NULL;
//  }
//  virtual char const * describe() const {
//    return "LastOp";
//  }
//} LastOp_;



//std::unique_ptr<Operation> shift( eOpType T ) {
//}


std::unique_ptr<Operation> operation( eOpType T ) {
  switch (T) {
    case kADD_:
      return std::make_unique<ADD_>();
    case kAND_:
      return std::make_unique<AND_>();
  case kANDS_:
    return std::make_unique<ANDS_>();
    case kORR_:
      return std::make_unique<ORR_>();
    case kXOR_:
      return std::make_unique<XOR_>();
    case kSUB_:
      return std::make_unique<SUB_>();
    case kAndN_:
      return std::make_unique<AndN_>();
    case kOrN_:
      return std::make_unique<OrN_>();
    case kEoN_:
      return std::make_unique<EoN_>();
    case kXnor_:
      return std::make_unique<Xnor_>();
    case kADDS_:
      return std::make_unique<ADDS_>();
    case kCONCAT_:
        return std::make_unique<CONCAT_>();
    case kMulX_:
      return std::make_unique<MulX_>();
    case kSUBS_:
      return std::make_unique<SUBS_>();
    case kUMul_:
      return std::make_unique<UMul_>();
    case kUMulH_:
      return std::make_unique<UMulH_>();
    case kSMul_:
      return std::make_unique<SMul_>();
    case kSMulH_:
      return std::make_unique<SMulH_>();
    case kUDivX_:
      return std::make_unique<UDivX_>();
    case kUDiv_:
      return std::make_unique<UDiv_>();
    case kSDiv_:
      return std::make_unique<SDiv_>();
    case kSDivX_:
      return std::make_unique<SDivX_>();
    case kMOV_:
      return std::make_unique<MOV_>();
    case kMOVN_:
      return std::make_unique<MOVN_>();
    case kMOVK_:
      return std::make_unique<MOVK_>();
    case kSext_:
      return std::make_unique<Sext_>();
    case kZext_:
      return std::make_unique<Zext_>();
    case kNot_:
      return std::make_unique<Not_>();
    case kROR_:
        return std::make_unique<ROR_>();
    case kASR_:
        return std::make_unique<ASR_>();
    case kLSR:
        return std::make_unique<LSR_>();
    case kLSL:
        return std::make_unique<LSL_>();
    default:
        DBG_Assert( false, ( << "Unimplemented operation type: " << T ) );
        return nullptr;
  }
}


} //narmDecoder
