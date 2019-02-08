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

#include "encodings/armSharedFunctions.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

typedef struct OFFSET : public Operation {
    OFFSET(){}
    virtual ~OFFSET(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    if (hasOwnOperands()) {
        DBG_Assert( theOperands.size() == 1 || theOperands.size() == 2, (<< *theInstruction) );
        bits op1 = boost::get<bits>(theOperands[0]);
        bits op2 = boost::get<bits>(theOperands[1]);
        return op1+op2;
    } else {
        DBG_Assert( operands.size() == 2);
        bits op1 = boost::get<bits>(operands[0]);
        bits op2 = boost::get<bits>(operands[1]);
        return op1+op2;
    }
  }
  virtual char const * describe() const {
    return "Add";
  }
} OFFSET_;


typedef struct ADD : public Operation {
    ADD(){}
    virtual ~ADD(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
        make_local(operands);
        SEMANTICS_DBG( "ADDING with " << operands.size() << " the: " << theOperands.size() );
        DBG_Assert( checkNumOperands({1,2}), (<< *theInstruction << "num operands: " << operands.size()) );

        if (theOperands.size()>1){
        return
                boost::get<bits>(hasOwnOperands() ? theOperands[0] : operands[0]) +
                boost::get<bits>(hasOwnOperands() ? theOperands[1] : operands[1]);
        } else {
            return
                    boost::get<bits>(hasOwnOperands() ? theOperands[0] : operands[0]);
        }
  }
  virtual char const * describe() const {
    return "Add";
  }
} ADD_;

typedef struct ADDS : public Operation {
    ADDS(){}
    virtual ~ADDS(){}

  virtual Operand operator()( std::vector<Operand> const & operands  ) {
        DBG_Assert( operands.size() == 2 || operands.size() == 3);
        bits carry ;

        if (operands.size() == 2){
            carry = 0;
        } else {
            carry = boost::get<bits >(operands[2]);
        }

    bits sresult =  boost::get<bits>(operands[0]) + boost::get<bits>(operands[1]) + carry;
    bits uresult =  boost::get<bits>(operands[0]) + boost::get<bits>(operands[1]) + carry;
    bits result =  0;
    result &= uresult;

    uint32_t N = (1ul << 31) & result;
    uint32_t Z = ((result == 0) ? 1ul << 30 : 0);
    uint32_t C = ((result - uresult == 0) ? 1ul << 29 : 0);
    uint32_t V = ((result - sresult == 0) ? 1ul << 28 : 0);

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "ADDS";
  }
} ADDS_;

typedef struct SUB : public Operation {
    SUB(){}
    virtual ~SUB(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) - boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "SUB";
  }
} SUB_;

typedef struct SUBS : public Operation {
    SUBS(){}
    virtual ~SUBS(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2 || operands.size() == 3);
    bits carry ;

    if (operands.size() == 2){
        carry = 1;
    } else {
        carry = boost::get<bits >(operands[2]);
    }


    int64_t sresult =  (int64_t)boost::get<bits>(operands[0]) + (~(int64_t)boost::get<bits>(operands[1])) + carry;
    bits uresult =  boost::get<bits>(operands[0]) + (~boost::get<bits>(operands[1])) + carry;
    bits result =  0;
    result &= uresult;

    uint32_t N = ((1ul << 31)) & result;
    uint32_t Z = ((result == 0) ? 1ul << 30 : 0);
    uint32_t C = ((result - uresult == 0) ? 1ul << 29 : 0);
    uint32_t V = ((result - sresult == 0) ? 1ul << 28 : 0);

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "SUBS";
  }
} SUBS_;


typedef struct CONCAT32 : public Operation {
    CONCAT32(){}
    virtual ~CONCAT32(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    bits op1 (64, boost::get<bits>(operands[0]));
    bits op2 (64, boost::get<bits>(operands[1]));

    op1.resize(32);
    op2.resize(32);
    return concat_bits(op1, op2);
  }
  virtual char const * describe() const {
    return "CONCAT32";
  }
} CONCAT32_;

typedef struct CONCAT64 : public Operation {
    CONCAT64(){}
    virtual ~CONCAT64(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    bits op1 (64, boost::get<bits>(operands[0]));
    bits op2 (64, boost::get<bits>(operands[1]));

    return concat_bits(op1, op2);
  }
  virtual char const * describe() const {
    return "CONCAT64";
  }
} CONCAT64_;


typedef struct AND : public Operation {
    AND(){}
    virtual ~AND(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) & boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "AND";
  }
} AND_;

typedef struct ANDS : public Operation {
    ANDS(){}
    virtual ~ANDS(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    bits result = boost::get<bits>(operands[0]) & boost::get<bits>(operands[1]);

    uint32_t N = ((1ul << 31)) & result;
    uint32_t Z = ((result == 0) ? 1ul << 30 : 0);
    uint32_t C = 0;
    uint32_t V = 0;

    theNZCV = N | Z | C | V;

    theNZCVFlags = true;

    return result;
  }
  virtual char const * describe() const {
    return "ANDS";
  }
} ANDS_;

typedef struct ORR : public Operation  {
    ORR(){}
    virtual ~ORR(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) | boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "ORR";
  }
} ORR_;

typedef struct XOR : public Operation  {
    XOR(){}
    virtual ~XOR(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) ^ boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "XOR";
  }
} XOR_;



typedef struct AndN : public Operation {

    AndN(){}
    virtual ~AndN(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) & ~boost::get<bits>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "AndN";
  }
} AndN_;


typedef struct EoN : public Operation  {
    EoN(){}
    virtual ~EoN(){}

  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0])  | ~boost::get<bits>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "EoN";
  }
} EoN_;

typedef struct OrN : public Operation  {
    OrN(){}
    virtual ~OrN(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0])  | ~boost::get<bits>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "OrN";
  }
} OrN_;

typedef struct Not : public Operation  {
    Not(){}
    virtual ~Not(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return ~boost::get<bits>(operands[0]) ;
  }
  virtual char const * describe() const {
    return "Invert";
  }
} Not_;


typedef struct ROR : public Operation  {
    ROR(){}
    virtual ~ROR(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    bits input = boost::get<bits>(operands[0]);
    bits shift_size = boost::get<bits>(operands[1]);
    bits input_size = boost::get<bits>(operands[2]);

    if (input_size == 1) input_size = 64; else input_size = 32;

    return ror(input, input_size, shift_size);
  }
  virtual char const * describe() const {
    return "ROR";
  }
} ROR_;

typedef struct LSL : public Operation  {
    LSL(){}
    virtual ~LSL(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 3);
    bits input = boost::get<bits>(operands[0]);
    bits shift_size = boost::get<bits>(operands[1]);
    bits input_size = boost::get<bits>(operands[2]);

    return lsl(input, input_size, shift_size);
    }
  virtual char const * describe() const {
    return "LSL";
  }
} LSL_;

typedef struct ASR : public Operation  {
    ASR(){}
    virtual ~ASR(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
        DBG_Assert( operands.size() == 3);
        bits input = boost::get<bits>(operands[0]);
        bits shift_size = boost::get<bits>(operands[1]);
        bits input_size = boost::get<bits>(operands[2]);

        return asr(input, input_size, shift_size);
  }
  virtual char const * describe() const {
    return "ASR";
  }
} ASR_;

typedef struct LSR : public Operation  {
    LSR(){}
    virtual ~LSR(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
        DBG_Assert( operands.size() == 3);
        bits input = boost::get<bits>(operands[0]);
        bits shift_size = boost::get<bits>(operands[1]);
        bits input_size = boost::get<bits>(operands[2]);

        return lsr(input, input_size, shift_size);
  }
  virtual char const * describe() const {
    return "LSR";
  }
} LSR_;

typedef struct SextB : public Operation  {
    SextB(){}
    virtual ~SextB(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0])  | SIGNED_UPPER_BOUND_B;
  }
  virtual char const * describe() const {
    return "singned extend Byte";
  }
} SextB_;

typedef struct SextH : public Operation  {
    SextH(){}
    virtual ~SextH(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0])  | SIGNED_UPPER_BOUND_H;
  }
  virtual char const * describe() const {
    return "singned extend Half Word";
  }
} SextH_;

typedef struct SextW : public Operation  {
    SextW(){}
    virtual ~SextW(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0])  | SIGNED_UPPER_BOUND_W;
  }
  virtual char const * describe() const {
    return "singned extend Word";
  }
} SextW_;

typedef struct SextX : public Operation  {
    SextX(){}
    virtual ~SextX(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0])  | SIGNED_UPPER_BOUND_X;
  }
  virtual char const * describe() const {
    return "singned extend Double Word";
  }
} SextX_;

typedef struct ZextB : public Operation  {
    ZextB(){}
    virtual ~ZextB(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]);
  }
  virtual char const * describe() const {
    return "Zero extend Byte";
  }
} ZextB_;

typedef struct ZextH : public Operation  {
    ZextH(){}
    virtual ~ZextH(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]);
  }
  virtual char const * describe() const {
    return "Zero extend Half Word";
  }
} ZextH_;

typedef struct ZextW : public Operation  {
    ZextW(){}
    virtual ~ZextW(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]);
  }
  virtual char const * describe() const {
    return "Zero extend Word";
  }
} ZextW_;

typedef struct ZextX : public Operation  {
    ZextX(){}
    virtual ~ZextX(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 1);
    return boost::get<bits>(operands[0]);
  }
  virtual char const * describe() const {
    return "Zero extend Double Word";
  }
} ZextX_;

typedef struct Xnor : public Operation  {
    Xnor(){}
    virtual ~Xnor(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) ^ ~boost::get<bits>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "Xnor";
  }
} Xnor_;

typedef struct UMul : public Operation  {
    UMul(){}
    virtual ~UMul(){}
  bits calc(std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    bits op0 = boost::get<bits>(operands[0]) & 0xFFFFFFFF;
    bits op1 = boost::get<bits>(operands[1]) & 0xFFFFFFFF;
    bits prod = op0 * op1;
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
    UMulH(){}
    virtual ~UMulH(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    bits op0 = boost::get<bits>(operands[0]);
    bits op1 = boost::get<bits>(operands[1]);
    bits prod = (bits)(((__uint128_t)op0 * op1) >> 64);
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

typedef struct UMulL : public Operation  {
    UMulL(){}
    virtual ~UMulL(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<bits>(operands[0]) & 0xffffffff;
    uint32_t op1 = boost::get<bits>(operands[1]) & 0xffffffff;
    bits prod = op0 * op1;
    return prod;
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "UMulL";
  }
} UMulL_;

typedef struct SMul : public Operation  {
    SMul(){}
    virtual ~SMul(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    bits op0 = boost::get<bits>(operands[0]) & 0xFFFFFFFFULL;
    bits op1 = boost::get<bits>(operands[1]) & 0xFFFFFFFFULL;
    int64_t op0_s = op0 | ( op0 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t op1_s = op1 | ( op1 & 0x80000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL );
    int64_t prod = op0_s * op1_s;
    return static_cast<bits>(prod);
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
    SMulH(){}
    virtual ~SMulH(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    int64_t op0 = boost::get<bits>(operands[0]);
    int64_t op1 = boost::get<bits>(operands[1]);
    int64_t prod = (int64_t)(((__int128_t)op0 * op1) >> 64);
    return static_cast<bits>(prod);
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

typedef struct SMulL : public Operation  {
    SMulL(){}
    virtual ~SMulL(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    int32_t op0 = boost::get<bits>(operands[0]) & 0xffffffff;
    int32_t op1 = boost::get<bits>(operands[1]) & 0xffffffff;
    int64_t prod = op0 * op1;
    return static_cast<bits>(prod);
  }
  virtual Operand operator()( std::vector<Operand> const & operands) {
    return calc(operands);
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands) {
    return calc(operands) >> 32;
  }
  virtual char const * describe() const {
    return "SMulL";
  }
} SMulL_;

typedef struct UDiv : public Operation  {
    UDiv(){}
    virtual ~UDiv(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 3);
    bits op0 = boost::get<bits>(operands[0]) & 0xFFFFFFFF;
    bits op1 = boost::get<bits>(operands[1]) & 0xFFFFFFFF;
    bits y = boost::get<bits>(operands[2]) & 0xFFFFFFFF;
    bits dividend = op0 | (y << 32 );
    if (op1 == 0) {
      return 0ULL; //ret_val is irrelevant when divide by zero
    }
    bits prod = dividend / op1;
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
    SDiv(){}
    virtual ~SDiv(){}
  bits calc( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 3);
    bits op0 = boost::get<bits>(operands[0]) & 0xFFFFFFFF;
    bits op1 = boost::get<bits>(operands[1]) & 0xFFFFFFFF;
    bits y = boost::get<bits>(operands[2]) & 0xFFFFFFFF;
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
    MulX(){}
    virtual ~MulX(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[0]) * boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "MulX";
  }
} MulX_;

typedef struct UDivX : public Operation  {
    UDivX(){}
    virtual ~UDivX(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    if (boost::get<bits>(operands[1]) != 0) {
      return boost::get<bits>(operands[0]) / boost::get<bits>(operands[1]);
    } else {
      return 0ULL; //Will result in an exception, so it doesn't matter what we return
    }
  }
  virtual char const * describe() const {
    return "UDivX";
  }
} UDivX_;

typedef struct SDivX : public Operation  {
    SDivX(){}
    virtual ~SDivX(){}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    int64_t op0_s = boost::get<bits>(operands[0]);
    int64_t op1_s = boost::get<bits>(operands[1]);
    if (op1_s != 0) {
      return static_cast<bits>( op0_s / op1_s);
    } else {
      return 0ULL; //Will result in an exception, so it doesn't matter what we return
    }
  }
  virtual char const * describe() const {
    return "SDivX";
  }
} SDivX_;

typedef struct MOV_ : public Operation {
    MOV_(){}
    virtual ~MOV_(){}
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<bits>(operands[1]);
  }
  virtual char const * describe() const {
    return "MOV";
  }
} MOV_;

typedef struct MOVN_ : public Operation {
    MOVN_(){}
    virtual ~MOVN_(){}
    virtual Operand operator()( std::vector<Operand> const & operands  ) {
      DBG_Assert( operands.size() == 2);
      return ~boost::get<bits>(operands[1]);
    }
  virtual char const * describe() const {
    return "MOVN";
  }
} MOVN_;

typedef struct MOVK_ : public Operation {
    MOVK_(){}
    virtual ~MOVK_(){}
    virtual Operand operator()( std::vector<Operand> const & operands  ) {
      DBG_Assert( operands.size() == 2);
      bits rd =  boost::get<bits>(operands[0]);
      bits imm =  boost::get<bits>(operands[1]);

      return (rd & imm) | imm;
    }
  virtual char const * describe() const {
    return "MOVK";
  }
} MOVK_;



typedef struct OVERWRITE_ : public Operation {
    OVERWRITE_(){}
    virtual ~OVERWRITE_(){}
    virtual Operand operator()( std::vector<Operand> const & operands  ) {
      DBG_Assert( operands.size() == 3);
      bits lhs =  boost::get<bits>(operands[0]);
      bits rhs =  boost::get<bits>(operands[1]);
      bits mask =  boost::get<bits>(operands[2]);

      return ((lhs & mask) | rhs);
    }
  virtual char const * describe() const {
    return "OVERWRITE_";
  }
} OVERWRITE_;

std::unique_ptr<Operation> shift( uint32_t aType ) {
    std::unique_ptr<Operation> ptr;

    switch (aType) {
    case 0: ptr.reset(new LSL_()); break;
    case 1: ptr.reset(new LSR_()); break;
    case 2: ptr.reset(new ASR_()); break;
    case 3: ptr.reset(new ROR_()); break;
    default:
        DBG_Assert(false);
    }
    return ptr;
}

std::unique_ptr<Operation> operation( eOpType aType ) {

  std::unique_ptr<Operation> ptr;
  switch (aType) {
  case kADD_:
      ptr.reset(new ADD_()); break;
  case kADDS_:
      ptr.reset(new ADDS_()); break;
  case kAND_:
      ptr.reset(new AND_()); break;
  case kANDS_:
      ptr.reset(new ANDS_()); break;
    case kORR_:
      ptr.reset(new ORR_()); break;
    case kXOR_:
      ptr.reset(new XOR_()); break;
    case kSUB_:
      ptr.reset(new SUB_()); break;
    case kSUBS_:
        ptr.reset(new SUBS_()); break;
    case kAndN_:
      ptr.reset(new AndN_()); break;
    case kOrN_:
      ptr.reset(new OrN_()); break;
    case kEoN_:
      ptr.reset(new EoN_()); break;
    case kXnor_:
      ptr.reset(new Xnor_()); break;
    case kCONCAT32_:
        ptr.reset(new CONCAT32_()); break;
    case kCONCAT64_:
      ptr.reset(new CONCAT64_()); break;
    case kMulX_:
      ptr.reset(new MulX_()); break;
    case kUMul_:
      ptr.reset(new UMul_()); break;
    case kUMulH_:
      ptr.reset(new UMulH_()); break;
    case kUMulL_:
        ptr.reset(new UMulL_()); break;
    case kSMul_:
      ptr.reset(new SMul_()); break;
    case kSMulH_:
      ptr.reset(new SMulH_()); break;
    case kSMulL_:
        ptr.reset(new SMulL_()); break;
    case kUDivX_:
      ptr.reset(new UDivX_()); break;
    case kUDiv_:
      ptr.reset(new UDiv_()); break;
    case kSDiv_:
      ptr.reset(new SDiv_()); break;
    case kSDivX_:
      ptr.reset(new SDivX_()); break;
    case kMOV_:
      ptr.reset(new MOV_()); break;
    case kMOVN_:
      ptr.reset(new MOVN_()); break;
    case kMOVK_:
      ptr.reset(new MOVK_()); break;
    case kSextB_:
      ptr.reset(new SextB_()); break;
    case kSextH_:
      ptr.reset(new SextH_()); break;
    case kSextW_:
      ptr.reset(new SextW_()); break;
    case kSextX_:
      ptr.reset(new SextX_()); break;
    case kZextB_:
      ptr.reset(new ZextB_()); break;
    case kZextH_:
      ptr.reset(new ZextH_()); break;
    case kZextW_:
      ptr.reset(new ZextW_()); break;
    case kZextX_:
      ptr.reset(new ZextX_()); break;
    case kNot_:
      ptr.reset(new Not_()); break;
    case kROR_:
        ptr.reset(new ROR_()); break;
    case kASR_:
        ptr.reset(new ASR_()); break;
    case kLSR_:
        ptr.reset(new LSR_()); break;
    case kLSL_:
        ptr.reset(new LSL_()); break;
  case kOVERWRITE_:
        ptr.reset(new OVERWRITE_()); break;
    default:
        DBG_Assert( false, ( << "Unimplemented operation type: " << aType ) );
  }
  return ptr;

}


} //narmDecoder
