#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>
#include <components/uArch/RegisterType.hpp>

#include "SemanticActions.hpp"
#include "OperandMap.hpp"
#include "Conditions.hpp"

#define DBG_DeclareCategories v9Decoder
#define DBG_SetDefaultOps AddCat(v9Decoder)
#include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;

struct Add : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "Add";
  }
} Add_;

struct AddTrunc : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return ( (boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1])) & 0xFFFFFFFFULL );
  }
  virtual char const * describe() const {
    return "AddTrunc";
  }
} AddTrunc_;

struct Align : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    return ( (boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1])) & ~(7ULL) );
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    int32_t bits = (boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1])) & 7ULL;
    uint64_t gsr = (boost::get<uint64_t>(operands[2]) & ~7) | bits;
    DBG_( Verb, ( << "Align bits=" << bits << " gsr=" << gsr) );
    return gsr;
  }
  virtual char const * describe() const {
    return "Align";
  }
} Align_;

struct AlignLittle : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    return ( (boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1])) & ~(7ULL));
  }
  virtual Operand evalExtra( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    int32_t bits = (boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1])) & 7ULL;
    //2's complement bits
    bits = (~bits + 1) & 0x7;
    uint64_t gsr = (boost::get<uint64_t>(operands[2]) & ~7) | bits;
    DBG_( Verb, ( << "AlignLittle bits=" << bits << " gsr=" << gsr) );
    return gsr;
  }
  virtual char const * describe() const {
    return "AlignLittle";
  }
} AlignLittle_;

struct AddC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    std::bitset<8> ccbits = boost::get<std::bitset<8> >(operands[2]);
    int32_t carry = 0;
    if (ccbits[iccC]) {
      carry = 1;
    }
    return boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1]) + carry;
  }
  virtual char const * describe() const {
    return "AddC";
  }
} AddC_;

struct SubC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    std::bitset<8> ccbits = boost::get<std::bitset<8> >(operands[2]);
    int32_t carry = 0;
    if (ccbits[iccC]) {
      carry = 1;
    }
    return boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]) - carry;
  }
  virtual char const * describe() const {
    return "SubC";
  }
} SubC_;

struct And : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "And";
  }
} And_;

struct Or : public Operation  {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) | boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "Or";
  }
} Or_;

struct Xor : public Operation  {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "Xor";
  }
} Xor_;

struct Sub : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "Sub";
  }
} Sub_;

struct AndN : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) & ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "AndN";
  }
} AndN_;

struct OrN : public Operation  {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0])  | ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "OrN";
  }
} OrN_;

struct Xnor : public Operation  {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) ^ ~boost::get<uint64_t>(operands[1]) ;
  }
  virtual char const * describe() const {
    return "Xnor";
  }
} Xnor_;

struct UMul : public Operation  {
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

struct SMul : public Operation  {
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

struct UDiv : public Operation  {
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

struct SDiv : public Operation  {
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

struct MulX : public Operation  {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) * boost::get<uint64_t>(operands[1]);
  }
  virtual char const * describe() const {
    return "MulX";
  }
} MulX_;

struct UDivX : public Operation  {
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

struct SDivX : public Operation  {
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

struct MovCC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t false_val = boost::get<uint64_t>(operands[0]);
    uint64_t true_val = boost::get<uint64_t>(operands[1]);
    std::bitset<8> cc = boost::get<std::bitset<8> >(operands[2]);
    uint64_t packed_condition = boost::get<uint64_t >(operands[3]);

    bool result;
    if (isFloating(packed_condition)) {
      FCondition cond = fcondition(packed_condition);
      result = cond(cc);
    } else {
      Condition cond = condition(packed_condition);
      result = cond(cc);
    }
    if ( result ) {
      DBG_( Verb, ( << "MovCC returning true value: " << true_val ) );
      return true_val;
    } else {
      DBG_( Verb, ( << "MovCC returning false value: " << false_val ) );
      return false_val;
    }
  }
  virtual char const * describe() const {
    return "MovCC";
  }
} MovCC_;

struct MovR : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t false_val = boost::get<uint64_t>(operands[0]);
    uint64_t true_val = boost::get<uint64_t>(operands[1]);
    uint64_t cond_reg = boost::get<uint64_t>(operands[2]);
    uint64_t condition = boost::get<uint64_t >(operands[3]);

    RCondition cond = rcondition(condition);
    if ( cond( cond_reg ) ) {
      DBG_( Verb, ( << "MovR returning true value: " << true_val ) );
      return true_val;
    } else {
      DBG_( Verb, ( << "MovR returning false value: " << false_val ) );
      return false_val;
    }
  }
  virtual char const * describe() const {
    return "MovR";
  }
} MovR_;

struct TCC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    //uint64_t false_val = boost::get<uint64_t>(operands[0]);
    uint64_t true_val = boost::get<uint64_t>(operands[1]);
    std::bitset<8> cc = boost::get<std::bitset<8> >(operands[2]);
    uint64_t packed_condition = boost::get<uint64_t >(operands[3]);

    bool result;
    if (isFloating(packed_condition)) {
      FCondition cond = fcondition(packed_condition);
      result = cond(cc);
    } else {
      Condition cond = condition(packed_condition);
      result = cond(cc);
    }
    if ( result ) {
      DBG_( Verb, ( << "TCC tests true, raise trap: " << true_val ) );
      return true_val;
    } else {
      DBG_( Verb, ( << "TCC tests false, no trap") );
      return 0;
    }
  }
  virtual char const * describe() const {
    return "TCC";
  }
} TCC_;

struct ICC2ULONG : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    std::bitset<8> ccr = boost::get<std::bitset<8> >(operands[0]);
    return ccr.to_ulong();
  }
  virtual char const * describe() const {
    return "ICC2ULONG";
  }
} ICC2ULONG_;

struct ULONG2ICC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint64_t val = boost::get<uint64_t >(operands[0]);
    std::bitset<8> ccr(val);
    return ccr;
  }
  virtual char const * describe() const {
    return "ULONG2ICC";
  }
} ULONG2ICC_;

Operation & operation( int32_t anOp3Code ) {
  switch (anOp3Code) {
    case 0:
      return Add_;
    case 1:
      return And_;
    case 2:
      return Or_;
    case 3:
      return Xor_;
    case 4:
      return Sub_;
    case 5:
      return AndN_;
    case 6:
      return OrN_;
    case 7:
      return Xnor_;
    case 8:
      return AddC_;
    case 9:
      return MulX_;
    case 0xC:
      return SubC_;
    case 0xA:
    case 0x1A:
      return UMul_;
    case 0xB:
    case 0x1B:
      return SMul_;
    case 0xD:
      return UDivX_;
    case 0xE:
    case 0x1E:
      return UDiv_;
    case 0xF:
    case 0x1F:
      return SDiv_;
    case 0x2D:
      return SDivX_;
    case 0x2C:
      return MovCC_;
    case 0x2F:
      return MovR_;
    case 0x3A:
      return TCC_;
    case 0xFB:
      return Align_;
    case 0xFC:
      return AlignLittle_;
    case 0xFD:
      return ULONG2ICC_;
    case 0xFE:
      return ICC2ULONG_;
    case 0xFF:
      return AddTrunc_;
    default:
      DBG_Assert( false, ( << "Unimplemented op3 code: " << anOp3Code ) );
      return Or_;
  }
}

struct Add_CC: public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);

    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    uint64_t val = boost::get<uint64_t>(Add_(operands));
    std::bitset<8> ccbits;

    ccbits[xccZ] = (val == 0);
    ccbits[iccZ] = ( (val & 0xFFFFFFFFULL) == 0);

    ccbits[xccN] = val & 0x8000000000000000ULL;
    ccbits[iccN] = val & 0x80000000ULL;

    bool op1_63 = op1 & (1ULL << 63);
    bool op2_63 = op2 & (1ULL << 63);
    bool val_63 = val & (1ULL << 63);
    bool op1_31 = op1 & (1ULL << 31);
    bool op2_31 = op2 & (1ULL << 31);
    bool val_31 = val & (1ULL << 31);
    ccbits[xccV] = ( op1_63 == op2_63 ) && ( op1_63 != val_63 ) ;
    ccbits[iccV] = ( op1_31 == op2_31 ) && ( op1_31 != val_31 ) ;
    uint64_t xcarry_add = (op1 >> 1) + (op2 >> 1) + ( 1 & op1 & op2 );
    bool xcarry = xcarry_add & (1ULL << 63);
    ccbits[xccC] = xcarry;
    uint64_t icarry_add = (op1 & 0xFFFFFFFFULL) + (op2 & 0xFFFFFFFFULL);
    bool icarry = icarry_add & (1ULL << 32);
    ccbits[iccC] = icarry;
    return ccbits;
  }
  virtual char const * describe() const {
    return "Add_cc";
  }
} Add_CC_;

struct AddC_CC: public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    std::bitset<8> ccbits_in = boost::get< std::bitset<8> >(operands[2]);
    int32_t carry = 0;
    if (ccbits_in[iccC]) {
      carry = 1;
    }
    uint64_t val = boost::get<uint64_t>(operands[0]) + boost::get<uint64_t>(operands[1]) + carry;
    std::bitset<8> ccbits;

    ccbits[xccZ] = (val == 0);
    ccbits[iccZ] = ( (val & 0xFFFFFFFFULL) == 0);

    ccbits[xccN] = val & 0x8000000000000000ULL;
    ccbits[iccN] = val & 0x80000000ULL;

    bool op1_63 = op1 & (1ULL << 63);
    bool op2_63 = op2 & (1ULL << 63);
    bool val_63 = val & (1ULL << 63);
    bool op1_31 = op1 & (1ULL << 31);
    bool op2_31 = op2 & (1ULL << 31);
    bool val_31 = val & (1ULL << 31);
    bool op1_0 = op1 & 1;
    bool op2_0 = op2 & 1;
    ccbits[xccV] = ( op1_63 == op2_63 ) && ( op1_63 != val_63 ) ;
    ccbits[iccV] = ( op1_31 == op2_31 ) && ( op1_31 != val_31 ) ;
    uint64_t xcarry_add = (op1 >> 1) + (op2 >> 1) + ( (op1_0 & op2_0) | (op1_0 & carry) | (op2_0 & carry) );
    bool xcarry = xcarry_add & (1ULL << 63);
    ccbits[xccC] = xcarry;
    uint64_t icarry_add = (op1 & 0xFFFFFFFFULL) + (op2 & 0xFFFFFFFFULL);
    bool icarry = icarry_add & (1ULL << 32);
    ccbits[iccC] = icarry;
    return ccbits;
  }
  virtual char const * describe() const {
    return "AddC_cc";
  }
} AddC_CC_;

struct Sub_CC : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    uint64_t val = boost::get<uint64_t>(Sub_(operands));
    std::bitset<8> ccbits;

    ccbits[xccZ] = (val == 0);
    ccbits[iccZ] = ( (val & 0xFFFFFFFFULL) == 0);

    bool op1_63 = op1 & (1ULL << 63);
    bool op2_63 = op2 & (1ULL << 63);
    bool val_63 = val & (1ULL << 63);
    bool op1_31 = op1 & (1ULL << 31);
    bool op2_31 = op2 & (1ULL << 31);
    bool val_31 = val & (1ULL << 31);

    ccbits[xccN] = val_63;
    ccbits[iccN] = val_31;

    ccbits[xccV] = ( op1_63 != op2_63 ) && ( op1_63 != val_63 ) ;
    ccbits[iccV] = ( op1_31 != op2_31 ) && ( op1_31 != val_31 ) ;

    uint64_t xborrow_sub = (op1 >> 1) - (op2 >> 1) -  ( (op2 & 1 & (~op1) ))  ;
    bool xborrow = xborrow_sub &  ( 1ULL << 63 );
    ccbits[xccC] = xborrow;

    uint64_t iborrow_sub = (op1 & 0xFFFFFFFFULL) - (op2 & 0xFFFFFFFFULL);
    bool iborrow = iborrow_sub & (1ULL << 32);
    ccbits[iccC] = iborrow;
    return ccbits;
  }
  virtual char const * describe() const {
    return "Sub_cc";
  }
} Sub_CC_;

struct SubC_CC: public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands) {
    uint64_t op1 = boost::get<uint64_t>(operands[0]);
    uint64_t op2 = boost::get<uint64_t>(operands[1]);
    std::bitset<8> ccbits_in = boost::get< std::bitset<8> >(operands[2]);
    int32_t carry = 0;
    if (ccbits_in[iccC]) {
      carry = 1;
    }
    uint64_t val = boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]) - carry;
    std::bitset<8> ccbits;

    ccbits[xccZ] = (val == 0);
    ccbits[iccZ] = ( (val & 0xFFFFFFFFULL) == 0);

    bool op1_63 = op1 & (1ULL << 63);
    bool op2_63 = op2 & (1ULL << 63);
    bool val_63 = val & (1ULL << 63);
    bool op1_31 = op1 & (1ULL << 31);
    bool op2_31 = op2 & (1ULL << 31);
    bool val_31 = val & (1ULL << 31);

    ccbits[xccN] = val_63;
    ccbits[iccN] = val_31;

    ccbits[xccV] = ( op1_63 != op2_63 ) && ( op1_63 != val_63 ) ;
    ccbits[iccV] = ( op1_31 != op2_31 ) && ( op1_31 != val_31 ) ;

    int32_t borrow0 = 0;
    if ( (!(op1 & 0) && (carry || (op2 & 0))) || (carry && (op2 & 0)))   {
      borrow0 = 1;
    }
    uint64_t xborrow_sub = (op1 >> 1) - (op2 >> 1) -  borrow0  ;
    bool xborrow = xborrow_sub &  ( 1ULL << 63 );
    ccbits[xccC] = xborrow;

    uint64_t iborrow_sub = (op1 & 0xFFFFFFFFULL) - (op2 & 0xFFFFFFFFULL) - carry;
    bool iborrow = iborrow_sub & (1ULL << 32);
    ccbits[iccC] = iborrow;
    return ccbits;
  }
  virtual char const * describe() const {
    return "SubC_cc";
  }
} SubC_CC_;

struct Logical_CC : public Operation {
  Operation & Op;
  Logical_CC(Operation & anOperation)
    : Op(anOperation)
  {}
  virtual Operand operator()( std::vector<Operand> const & operands) {
    std::bitset<8> ccbits(0);
    uint64_t val = boost::get<uint64_t>(Op(operands));

    ccbits[xccZ] = (val == 0);
    ccbits[iccZ] = ( (val & 0xFFFFFFFFULL) == 0);

    ccbits[xccN] = val & 0x8000000000000000ULL;
    ccbits[iccN] = val & 0x80000000ULL;

    return ccbits;
  }
  virtual char const * describe() const {
    return "logical_cc";
  }
} And_CC_(And_)
, Or_CC_(Or_)
, Xor_CC_(Xor_)
, AndN_CC_(AndN_)
, OrN_CC_(OrN_)
, Xnor_CC_(Xnor_)
, UMul_CC_(UMul_)
, SMul_CC_(SMul_)
;

struct UDiv_CC: public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 3);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF;
    uint64_t y = boost::get<uint64_t>(operands[2]) & 0xFFFFFFFF;
    uint64_t dividend = op0 | (y << 32 );
    std::bitset<8> ccbits;
    if (op1 == 0) {
      return ccbits; //ret_val is irrelevant when divide by zero
    }
    uint64_t prod = dividend / op1;
    if ( prod > 0xFFFFFFFFULL) {
      ccbits[iccV] = true;
      prod = 0xFFFFFFFFULL;
    }

    ccbits[xccZ] = (prod == 0);
    ccbits[iccZ] = ( (prod & 0xFFFFFFFFULL) == 0);
    ccbits[xccN] = prod & 0x8000000000000000ULL;
    ccbits[iccN] = prod & 0x80000000ULL;
    return ccbits;
  }
  virtual char const * describe() const {
    return "UDiv_cc";
  }
} UDiv_CC_;

struct SDiv_CC: public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    uint64_t op0 = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFF;
    uint64_t op1 = boost::get<uint64_t>(operands[1]) & 0xFFFFFFFF;
    uint64_t y = boost::get<uint64_t>(operands[2]) & 0xFFFFFFFF;
    int64_t dividend = op0 | (y << 32 );
    int64_t divisor = op1 | (op1 & 0x8000000ULL ? 0xFFFFFFFF00000000ULL : 0ULL);
    std::bitset<8> ccbits;
    if (divisor == 0) {
      return ccbits; //ret_val is irrelevant when divide by zero
    }
    int64_t prod = dividend / divisor;
    if (prod > 0x7FFFFFFFLL) {
      prod = 0x7FFFFFFFLL;
      ccbits[iccV] = true;
    } else if (prod < (-1 * 0x7FFFFFFFLL)) {
      prod = 0xFFFFFFFF80000000LL;
      ccbits[iccV] = true;
    }

    ccbits[xccZ] = (prod == 0);
    ccbits[iccZ] = ( (prod & 0xFFFFFFFFULL) == 0);
    ccbits[xccN] = prod & 0x8000000000000000ULL;
    ccbits[iccN] = prod & 0x80000000ULL;
    return ccbits;
  }
  virtual char const * describe() const {
    return "SDiv_cc";
  }
} SDiv_CC_;

Operation & ccCalc( int32_t anOp3Code ) {
  switch (anOp3Code) {
    case 0:
      return Add_CC_;
    case 1:
      return And_CC_;
    case 2:
      return Or_CC_;
    case 3:
      return Xor_CC_;
    case 4:
      return Sub_CC_;
    case 5:
      return AndN_CC_;
    case 6:
      return OrN_CC_;
    case 7:
      return Xnor_CC_;
    case 8:
      return AddC_CC_;
    case 0xA:
      return UMul_CC_;
    case 0xB:
      return SMul_CC_;
    case 0xC:
      return SubC_CC_;
    case 0xE:
      return UDiv_CC_;
    case 0xF:
      return SDiv_CC_;

    default:
      DBG_Assert( false, ( << "Unimplemented op3 code: " << anOp3Code ) );
      return Or_;
  }
}

struct SLL_32 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0] ) << ( boost::get<uint64_t>(operands[1]) & 0x1F) ;
  }
  virtual char const * describe() const {
    return "SLL";
  }
} SLL_32_;

struct SLL_64 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) << ( boost::get<uint64_t>(operands[1]) & 0x3F);
  }
  virtual char const * describe() const {
    return "SLLX";
  }
} SLL_64_;

struct SRL_32 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t val = boost::get<uint64_t>(operands[0]);
    val &= 0xFFFFFFFFULL;
    val >>= ( boost::get<uint64_t>(operands[1]) & 0x1F);
    return val;
  }
  virtual char const * describe() const {
    return "SRL";
  }
} SRL_32_;

struct SRL_64 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    return boost::get<uint64_t>(operands[0]) >> ( boost::get<uint64_t>(operands[1]) & 0x3F);
  }
  virtual char const * describe() const {
    return "SRLX";
  }
} SRL_64_;

struct SRA_32 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t val = boost::get<uint64_t>(operands[0]) & 0xFFFFFFFFULL;
    bool bit = val & 0x80000000ULL;
    if (bit) val |= 0xFFFFFFFF00000000ULL;
    val >>= ( boost::get<uint64_t>(operands[1]) & 0x1F);
    if (bit) val |= 0xFFFFFFFF00000000ULL;
    return val;
  }
  virtual char const * describe() const {
    return "SRA";
  }
} SRA_32_;

struct SRA_64 : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t val = boost::get<uint64_t>(operands[0]);
    bool bit = val & 0x8000000000000000ULL;
    uint64_t mask = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t shift = ( boost::get<uint64_t>(operands[1]) & 0x3F);
    mask >>= shift;
    mask = ~mask;
    val >>= shift;
    if (bit) {
      val |= mask;
    }
    return val;
  }
  virtual char const * describe() const {
    return "SRAX";
  }
} SRA_64_;

Operation & shift( int32_t aShiftCode, bool a64Bit) {
  switch (aShiftCode) {
    case 1:
      if (a64Bit) {
        return SLL_64_;
      } else {
        return SLL_32_;
      }
    case 2:
      if (a64Bit) {
        return SRL_64_;
      } else {
        return SRL_32_;
      }
    case 3:
      if (a64Bit) {
        return SRA_64_;
      } else {
        return SRA_32_;
      }
    case 0:
    default:
      DBG_Assert( false, ( << "Unimplemented shift code: " << aShiftCode ) );
      return Or_;
  }
}

struct FPOP : Operation {
  int32_t theRoundingMode;

  virtual void setContext(uint64_t aContext) {
    theRoundingMode = aContext;
  }
};

struct FADDs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   fadds %2\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FADDs";
  }
} FADDs_;

struct FADDd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   fldl %1\n"
      "   faddl %2\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FADDd";
  }
} FADDd_;

struct FSUBs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   fsubs %2\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << "After fp_op.  result=" << result << std::dec) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FSUBs";
  }
} FSUBs_;

struct FSUBd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   fldl %1\n"
      "   fsubl %2\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FSUBd";
  }
} FSUBd_;

struct FMULs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " rounding=" << theRoundingMode << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   fmuls %2\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FMULs";
  }
} FMULs_;

struct FMULd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " rounding=" << theRoundingMode << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   fldl %1\n"
      "   fmull %2\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FMULd";
  }
} FMULd_;

struct FsMULd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   flds %2\n"
      "   fmulp\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FsMULd";
  }
} FsMULd_;

struct FDIVs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   fdivs %2\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FDIVs";
  }
} FDIVs_;

struct FDIVd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_op.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   fldl %1\n"
      "   fdivl %2\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    DBG_( Verb, ( << "After fp_op.  result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FDIVd";
  }
} FDIVd_;

struct FMOVs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    return static_cast<uint64_t>(op0);
  }
  virtual char const * describe() const {
    return "FMOVs";
  }
} FMOVs_;

struct FMOVd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    return static_cast<uint64_t>(op0);
  }
  virtual char const * describe() const {
    return "FMOVd";
  }
} FMOVd_;

/*
struct FMovCCs : public Operation {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t false_val = boost::get<uint64_t>(operands[0]);
    uint64_t true_val = boost::get<uint64_t>(operands[1]);
    std::bitset<8> cc = boost::get<std::bitset<8> >(operands[2]);
    uint64_t packed_condition = boost::get<uint64_t >(operands[3]);

    bool result;
    if (isFloating(packed_condition)) {
      FCondition cond = fcondition(packed_condition);
      result = cond(cc);
    } else {
      Condition cond = condition(packed_condition);
      result = cond(cc);
    }
    if ( result ) {
      DBG_( Verb, ( << "MovCC returning true value: " << true_val ) );
      return true_val;
    } else {
      DBG_( Verb, ( << "MovCC returning false value: " << false_val ) );
      return false_val;
    }
  }
  virtual char const * describe() const {
    return "MovCC";
  }
} MovCC_;
*/

struct FNEGs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fchs\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FNEGs";
  }
} FNEGs_;

struct FNEGd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fchs\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FNEGd";
  }
} FNEGd_;

struct FABSs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fabs\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FABSs";
  }
} FABSs_;

struct FABSd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fabs\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FABSd";
  }
} FABSd_;

struct FSQRTs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fsqrt\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FSQRTs";
  }
} FSQRTs_;

struct FSQRTd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x27F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fsqrt\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FSQRTd";
  }
} FSQRTd_;

struct FxTOs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fildq %1\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FxTOs";
  }
} FxTOs_;

struct FdTOs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FdTOs";
  }
} FdTOs_;

struct FdTOi : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= 3 << 10; //Always round toward zero

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fistpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FdTOi";
  }
} FdTOi_;

struct FxTOd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fildq %1\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FxTOd";
  }
} FxTOd_;

struct FdTOx : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t result = 0;
    int16_t control = 0x7F;
    control |= 3 << 10; //always round toward zero

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fldl %1\n"
      "   fistpq %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FdTOx";
  }
} FdTOx_;

struct FiTOs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fildl %1\n"
      "   fstps %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FiTOs";
  }
} FiTOs_;

struct FsTOi : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t result = 0;
    int16_t control = 0x7F;
    control |= 3 << 10; //always round toward zero

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fistpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FsTOi";
  }
} FsTOi_;

struct FiTOd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint64_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   fildl %1\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FiTOd";
  }
} FiTOd_;

struct FsTOd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint64_t result = 0;
    int16_t control = 0x7F;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fstpl %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FsTOd";
  }
} FsTOd_;

struct FsTOx : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 1);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint64_t result = 0;
    int16_t control = 0x7F;
    control |= 3 << 10; //always round toward zero

    DBG_( Verb, ( << std::hex << describe() << " op0=" << op0 << " control=" << control << std::dec ) );

    __asm__ (
      "   fldcw %2\n"
      "   flds %1\n"
      "   fistpq %0\n"
      : "=m" (result)
      : "m" (op0)
      , "m" (control)
    );

    DBG_( Verb, ( << std::hex << describe() << " result=" << result ) );

    return static_cast<uint64_t>(result);
  }
  virtual char const * describe() const {
    return "FsTOx";
  }
} FsTOx_;

Operation & fp_op1( int32_t anOpCode ) {

  switch (anOpCode) {
    case 0x41: //FADDs
      return FADDs_;
    case 0x42: //FADDd
      return FADDd_;
    case 0x45: //FSUBs
      return FSUBs_;
    case 0x46: //FSUBd
      return FSUBd_;
    case 0x49: //FMULs
      return FMULs_;
    case 0x4A: //FMULd
      return FMULd_;
    case 0x4D: //FDIVs
      return FDIVs_;
    case 0x4E: //FDIVd
      return FDIVd_;
    case 0x01: //FMOVs
      return FMOVs_;
    case 0x02: //FMOVd
      return FMOVd_;
    case 0x05: //FNEGs
      return FNEGs_;
    case 0x06: //FNEGd
      return FNEGd_;
    case 0x09: //FABSs
      return FABSs_;
    case 0x0A: //FABSd
      return FABSd_;
    case 0x29: //FSQRTs
      return FSQRTs_;
    case 0x2A: //FSQRTd
      return FSQRTd_;

    case 0x69: //FsMULd
      return FsMULd_;

    case 0x84: //FxTOs
      return FxTOs_;
    case 0xC6: //FdTOs
      return FdTOs_;
    case 0xD2: //FdTOi
      return FdTOi_;
    case 0x88: //FxTOd
      return FxTOd_;
    case 0x82: //FdTOx
      return FdTOx_;
    case 0xC4: //FiTOs
      return FiTOs_;
    case 0xD1: //FsTOi
      return FsTOi_;
    case 0xC8: //FiTOd
      return FiTOd_;
    case 0xC9: //FsTOd
      return FsTOd_;
    case 0x81: //FsTOx
      return FsTOx_;

    default:
      DBG_Assert( false, ( << "Unimplemented FP operation: " << anOpCode ) );
      return FADDs_;
  }
}

struct FCMPs : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 2);
    uint32_t op0 = boost::get<uint64_t>(operands[0]);
    uint32_t op1 = boost::get<uint64_t>(operands[1]);
    int16_t control = 0x7F;
    uint16_t status = 0;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_cmp.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   flds %1\n"
      "   fcomps %2\n"
      "   fstsw %0\n"
      : "=m" (status)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    nuArch::fccVals result = nuArch::fccE;
    if ( status & 0x4000 ) {
      //C3 is set;
      if ( status & 0x100) {
        //C0 is set - Unordered
        result = nuArch::fccE;
      } else {
        //C0 is clear - Equal
        result = nuArch::fccE;
      }
    } else {
      //C3 is clear
      if ( status & 0x100) {
        //C0 is set - Less
        result = nuArch::fccL;
      } else {
        //C0 is clear - Greater
        result = nuArch::fccG;
      }
    }

    DBG_( Verb, ( << "After fp_cmp.  result=" << result ) );
    std::bitset<8> ccval(static_cast<uint32_t>(result));

    return ccval;
  }
  virtual char const * describe() const {
    return "FCMPs";
  }
} FCMPs_;

struct FCMPd : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 4);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    int16_t control = 0x7F;
    uint16_t status = 0;
    control |= (theRoundingMode & 3) << 10;

    DBG_( Verb, ( << "Before fp_cmp.  op0=" << op0 << " op1=" << op1 << " control=" << std::hex << control << std::dec ) );

    __asm__ (
      "   fldcw %3\n"
      "   fldl %1\n"
      "   fcompl %2\n"
      "   fstsw %0\n"
      : "=m" (status)
      : "m" (op0)
      , "m" (op1)
      , "m" (control)
    );

    nuArch::fccVals result = nuArch::fccE;
    if ( status & 0x4000 ) {
      //C3 is set;
      if ( status & 0x100) {
        //C0 is set - Unordered
        result = nuArch::fccE;
      } else {
        //C0 is clear - Equal
        result = nuArch::fccE;
      }
    } else {
      //C3 is clear
      if ( status & 0x100) {
        //C0 is set - Less
        result = nuArch::fccL;
      } else {
        //C0 is clear - Greater
        result = nuArch::fccG;
      }
    }

    DBG_( Verb, ( << "After fp_cmp.  result=" << result ) );
    std::bitset<8> ccval(static_cast<uint32_t>(result));

    return ccval;

  }
  virtual char const * describe() const {
    return "FCMPd";
  }
} FCMPd_;

struct FALIGNDATA : public FPOP {
  virtual Operand operator()( std::vector<Operand> const & operands  ) {
    DBG_Assert( operands.size() == 5);
    uint64_t op0 = boost::get<uint64_t>(operands[0]) << 32;
    op0 |= boost::get<uint64_t>(operands[1]);
    uint64_t op1 = boost::get<uint64_t>(operands[2]) << 32;
    op1 |= boost::get<uint64_t>(operands[3]);
    int32_t gsr = boost::get<uint64_t>(operands[4]);
    int32_t align = gsr & 0x7;

    //Fill the MSBs of result with the MSB data from op0.  The top byte of
    //op0 is MSB of result when align = 0, the second byte when align = 1, and
    //so on.
    uint64_t result = (op0 << ( 8 * align));
    //Shift op1 right by 8 - align bytes, and or into the bottom of result
    uint64_t intermed = (op1 >> (8 * ( 8 - align) ));
    if (align == 0) {
      intermed = 0;
    }
    result |= intermed;

    DBG_( Verb, ( << "FAlignData.   align=" << align << " op0=" << std::hex << op0 << " op1=" << op1 << " intermed=" << intermed << " result=" << result << std::dec ) );

    return result;
  }
  virtual char const * describe() const {
    return "FALIGNDATA";
  }
} FALIGNDATA_;

Operation & fp_op2( int32_t anOpCode ) {
  switch (anOpCode) {
    case 0x51: //FCMPs
    case 0x55: //FCMPEs
      return FCMPs_;

    case 0x52: //FCMPd
    case 0x56: //FCMPEd
      return FCMPd_;

    case 0xFF: //FALIGNDATA
      return FALIGNDATA_;

    default:
      DBG_Assert( false, ( << "Unimplemented FP operation: " << anOpCode ) );
      return FADDs_;
  }
}

} //nv9Decoder
