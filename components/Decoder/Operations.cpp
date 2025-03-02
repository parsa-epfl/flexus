
#include "BitManip.hpp"
#include "Conditions.hpp"
#include "OperandMap.hpp"
#include "SemanticActions.hpp"
#include "components/uArch/CoreModel/PSTATE.hpp"
#include "encodings/SharedFunctions.hpp"

#include <boost/throw_exception.hpp>
#include <components/uArch/RegisterType.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <cstdint>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

using namespace nuArch;

namespace nDecoder {

typedef struct OFFSET : public Operation
{
    OFFSET() {}
    virtual ~OFFSET() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        if (theOperands.size()) {
            DBG_Assert(theOperands.size() == 1 || theOperands.size() == 2, (<< *theInstruction));
            uint64_t op1 = boost::get<uint64_t>(theOperands[0]);
            uint64_t op2 = boost::get<uint64_t>(theOperands[1]);
            return op1 + op2;
        } else {
            DBG_Assert(operands.size() == 2);
            uint64_t op1 = boost::get<uint64_t>(operands[0]);
            uint64_t op2 = boost::get<uint64_t>(operands[1]);
            return op1 + op2;
        }
    }
    virtual char const* describe() const { return "Add"; }
} OFFSET_;

typedef struct ADD : public Operation
{
    ADD() {}
    virtual ~ADD() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        std::vector<Operand> finalOperands = theOperands.size() ? theOperands : operands;
        SEMANTICS_DBG("ADDING with " << finalOperands.size());
        DBG_Assert(finalOperands.size() <= 3 && finalOperands.size() > 0,
                   (<< *theInstruction << "num operands: " << finalOperands.size()));

        auto result = boost::get<uint64_t>(finalOperands[0]);

        if (finalOperands.size() == 1) {
            return result;
        } else if (finalOperands.size() == 2) {
            return result + boost::get<uint64_t>(finalOperands[1]);
        } else if (finalOperands.size() == 3) {
            result += boost::get<uint64_t>(finalOperands[1]);
            PSTATE pstate = boost::get<uint64_t>(operands[2]);
            uint64_t carry = pstate.C();
            result += carry; // ! This is only for ADC and SUBC.
        }

        return result;
    }
    virtual char const* describe() const { return "Add"; }
} ADD_;

typedef struct ADDS : public Operation
{
    using Operation::Operation;
    virtual ~ADDS() {}

    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2 || operands.size() == 3);
        uint64_t carry;

        if (operands.size() == 2) {
            carry = 0;
        } else {
            PSTATE pstate = boost::get<uint64_t>(operands[2]);
            carry = pstate.C();
        }

        
        // https://developer.arm.com/documentation/ddi0602/2024-03/Shared-Pseudocode/shared-functions-integer?lang=en#impl-shared.AddWithCarry.3
        // (bits(N), bits(4)) AddWithCarry(bits(N) x, bits(N) y, bit carry_in)
        //     integer unsigned_sum = UInt(x) + UInt(y) + UInt(carry_in);
        //     integer signed_sum = SInt(x) + SInt(y) + UInt(carry_in);
        //     bits(N) result = unsigned_sum<N-1:0>; // same value as signed_sum<N-1:0>
        //     bit n = result<N-1>;
        //     bit z = if IsZero(result) then '1' else '0';
        //     bit c = if UInt(result) == unsigned_sum then '0' else '1';
        //     bit v = if SInt(result) == signed_sum then '0' else '1';
        //     return (result, n:z:c:v);
        uint64_t result;
        uint32_t N;
        uint32_t Z;
        uint32_t C;
        uint32_t V;
        if (theSize == 64) {
            uint64_t op1 = boost::get<uint64_t>(operands[0]);
            uint64_t op2 = boost::get<uint64_t>(operands[1]);
            boost::multiprecision::int128_t sresult;
            sresult = (boost::multiprecision::int128_t)((int64_t)op1) + (boost::multiprecision::int128_t)((int64_t)op2) + carry;
            boost::multiprecision::uint128_t uresult = (boost::multiprecision::uint128_t)op1 + (boost::multiprecision::uint128_t)op2 + carry;
            result = uint64_t(uresult);
            N = ((result & ((uint64_t)1 << 63)) ? PSTATE_N : 0);
            Z = ((result == 0) ? PSTATE_Z : 0);
            C = uresult == result ? 0 : PSTATE_C;
            V = sresult == (int64_t)result ? 0 : PSTATE_V;
        } else {
            uint32_t op1 = static_cast<uint32_t>(boost::get<uint64_t>(operands[0]));
            uint32_t op2 = static_cast<uint32_t>(boost::get<uint64_t>(operands[1]));
            int64_t sresult = static_cast<int64_t>(static_cast<int32_t>(op1)) + static_cast<int64_t>(static_cast<int32_t>(op2)) + carry;
            uint64_t uresult = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2) + carry;
            result = uint32_t(uresult);
            N = ((result & ((uint32_t)1 << 31)) ? PSTATE_N : 0);
            Z = ((result == 0) ? PSTATE_Z : 0);
            C = uresult == result ? 0 : PSTATE_C;
            V = sresult == static_cast<int64_t>(static_cast<int32_t>(result)) ? 0 : PSTATE_V;
        }
        theNZCV = N | Z | C | V;

        theNZCVFlags = true;

        return result;
    }
    virtual char const* describe() const { 
        if (theSize == 64) {
            return "ADDS64"; 
        } else {
            return "ADDS32";
        }
    }
} ADDS_;

typedef struct SUB : public Operation
{
    SUB() {}
    virtual ~SUB() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2 || operands.size() == 3); // operand 3 is carry
        if (operands.size() == 2) {
            return boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]);
        } else {
            PSTATE pstate = boost::get<uint64_t>(operands[2]);
            auto fix_from_carry = pstate.C() ? 0 : 1;

            DBG_Assert(fix_from_carry == 0 || fix_from_carry == 1);
            return (boost::get<uint64_t>(operands[0]) - boost::get<uint64_t>(operands[1]) - fix_from_carry);
        }
    }
    virtual char const* describe() const { return "SUB"; }
} SUB_;

typedef struct SUBS : public Operation
{
    SUBS(uint32_t aSize = 64)
    {
        theSize = aSize;
    }
    virtual ~SUBS() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2 || operands.size() == 3);
        uint64_t carry;

        if (operands.size() == 2) {
            carry = 1;
        } else {
            PSTATE pstate = boost::get<uint64_t>(operands[2]);
            carry = pstate.C();
        }

        
        // https://developer.arm.com/documentation/ddi0602/2024-03/Shared-Pseudocode/shared-functions-integer?lang=en#impl-shared.AddWithCarry.3
        // (bits(N), bits(4)) AddWithCarry(bits(N) x, bits(N) y, bit carry_in)
        //     integer unsigned_sum = UInt(x) + UInt(y) + UInt(carry_in);
        //     integer signed_sum = SInt(x) + SInt(y) + UInt(carry_in);
        //     bits(N) result = unsigned_sum<N-1:0>; // same value as signed_sum<N-1:0>
        //     bit n = result<N-1>;
        //     bit z = if IsZero(result) then '1' else '0';
        //     bit c = if UInt(result) == unsigned_sum then '0' else '1';
        //     bit v = if SInt(result) == signed_sum then '0' else '1';
        //     return (result, n:z:c:v);
        uint64_t result;
        uint32_t N;
        uint32_t Z;
        uint32_t C;
        uint32_t V;
        if (theSize == 64) {
            uint64_t op1 = boost::get<uint64_t>(operands[0]);
            uint64_t op2 = ~boost::get<uint64_t>(operands[1]);
            boost::multiprecision::int128_t sresult;
            sresult = (boost::multiprecision::int128_t)((int64_t)op1) + (boost::multiprecision::int128_t)((int64_t)op2) + carry;
            boost::multiprecision::uint128_t uresult = (boost::multiprecision::uint128_t)op1 + (boost::multiprecision::uint128_t)op2 + carry;
            result = uint64_t(uresult);
            N = ((result & ((uint64_t)1 << 63)) ? PSTATE_N : 0);
            Z = ((result == 0) ? PSTATE_Z : 0);
            C = uresult == result ? 0 : PSTATE_C;
            V = sresult == (int64_t)result ? 0 : PSTATE_V;
        } else {
            uint32_t op1 = static_cast<uint32_t>(boost::get<uint64_t>(operands[0]));
            uint32_t op2 = ~static_cast<uint32_t>(boost::get<uint64_t>(operands[1]));
            int64_t sresult = static_cast<int64_t>(static_cast<int32_t>(op1)) + static_cast<int64_t>(static_cast<int32_t>(op2)) + carry;
            uint64_t uresult = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2) + carry;
            result = uint32_t(uresult);
            N = ((result & ((uint32_t)1 << 31)) ? PSTATE_N : 0);
            Z = ((result == 0) ? PSTATE_Z : 0);
            C = uresult == result ? 0 : PSTATE_C;
            V = sresult == static_cast<int64_t>(static_cast<int32_t>(result)) ? 0 : PSTATE_V;
        }
        theNZCV = N | Z | C | V;

        theNZCVFlags = true;

        return result;
    }
    virtual char const* describe() const { 
        if (theSize == 64) {
            return "SUBS64"; 
        } else {
            return "SUBS32";
        }
    }
} SUBS_;

typedef struct CONCAT32 : public Operation
{
    CONCAT32() {}
    virtual ~CONCAT32() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op1 = boost::get<uint64_t>(operands[0]);
        uint64_t op2 = boost::get<uint64_t>(operands[1]);

        the128 = true;

        op1 &= 0xffffffff;
        op2 &= 0xffffffff;
        return bits((op1 << 32) | op2);
    }
    virtual char const* describe() const { return "CONCAT32"; }
} CONCAT32_;

typedef struct CONCAT64 : public Operation
{
    CONCAT64() {}
    virtual ~CONCAT64() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op1 = boost::get<uint64_t>(operands[0]);
        uint64_t op2 = boost::get<uint64_t>(operands[1]);

        the128 = true;
        return concat_bits((bits)op1, (bits)op2);
    }
    virtual char const* describe() const { return "CONCAT64"; }
} CONCAT64_;

typedef struct AND : public Operation
{
    AND() {}
    virtual ~AND() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        DBG_(VVerb,
             (<< "ANDING " << std::hex << boost::get<uint64_t>(operands[0]) << " & "
              << boost::get<uint64_t>(operands[1]) << std::dec));
        return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]));
    }
    virtual char const* describe() const { return "AND"; }
} AND_;

typedef struct ANDS : public Operation
{
    ANDS() {}
    virtual ~ANDS() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t result = boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1]);

        uint32_t N = ((result & ((uint64_t)1 << 63)) ? PSTATE_N : 0);
        uint32_t Z = ((result == 0) ? PSTATE_Z : 0);
        uint32_t C = 0;
        uint32_t V = 0;

        theNZCV = N | Z | C | V;

        theNZCVFlags = true;

        return result;
    }
    virtual char const* describe() const { return "ANDS"; }
} ANDS_;

typedef struct ANDSN : public Operation
{
    ANDSN() {}
    virtual ~ANDSN() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t result = boost::get<uint64_t>(operands[0]) & ~boost::get<uint64_t>(operands[1]);

        uint32_t N = ((result & ((uint64_t)1 << 63)) ? PSTATE_N : 0);
        uint32_t Z = ((result == 0) ? PSTATE_Z : 0);
        uint32_t C = 0;
        uint32_t V = 0;

        theNZCV = N | Z | C | V;

        theNZCVFlags = true;

        return result;
    }
    virtual char const* describe() const { return "ANDSN"; }
} ANDSN_;

typedef struct ORR : public Operation
{
    ORR() {}
    virtual ~ORR() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) | boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "ORR"; }
} ORR_;

typedef struct XOR : public Operation
{
    XOR() {}
    virtual ~XOR() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) ^ boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "XOR"; }
} XOR_;

typedef struct AndN : public Operation
{

    AndN() {}
    virtual ~AndN() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) & ~boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "AndN"; }
} AndN_;

typedef struct EoN : public Operation
{
    EoN() {}
    virtual ~EoN() {}

    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) | ~boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "EoN"; }
} EoN_;

typedef struct OrN : public Operation
{
    OrN() {}
    virtual ~OrN() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) | ~boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "OrN"; }
} OrN_;

typedef struct Not : public Operation
{
    Not() {}
    virtual ~Not() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return ~boost::get<uint64_t>(operands[0]);
    }
    virtual char const* describe() const { return "Invert"; }
} Not_;

typedef struct ROR : public Operation
{
    ROR() {}
    virtual ~ROR() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t input      = boost::get<uint64_t>(operands[0]);
        uint64_t input_size = boost::get<uint64_t>(operands[2]);

        /* Mask off by 32 or 64 depending on input size according to C6.2.215 of ARM64 manual */
        uint64_t shift_size = (input_size == 32) ? boost::get<uint64_t>(operands[1]) & ((32 - 1))
                                                 : boost::get<uint64_t>(operands[1]) & ((64 - 1));
        return ror((uint64_t)input, (uint64_t)input_size, (uint64_t)shift_size);
    }
    virtual char const* describe() const { return "ROR"; }
} ROR_;

typedef struct LSL : public Operation
{
    LSL() {}
    virtual ~LSL() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t op1       = boost::get<uint64_t>(operands[0]);
        uint64_t op2       = boost::get<uint64_t>(operands[1]);
        uint64_t data_size = boost::get<uint64_t>(operands[2]);

        DBG_Assert(data_size == 32 || data_size == 64);

        uint64_t mask = (data_size == 32) ? 0xffffffff : 0xffffffffffffffff;
        return (op1 << (op2 % data_size) & mask);
    }
    virtual char const* describe() const { return "LSL"; }
} LSL_;

typedef struct ASR : public Operation
{
    ASR() {}
    virtual ~ASR() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t input      = boost::get<uint64_t>(operands[0]);
        uint64_t shift_size = boost::get<uint64_t>(operands[1]);
        uint64_t input_size = boost::get<uint64_t>(operands[2]);

        return asr((uint64_t)input, (uint64_t)input_size, (uint64_t)shift_size);
    }
    virtual char const* describe() const { return "ASR"; }
} ASR_;

typedef struct LSR : public Operation
{
    LSR() {}
    virtual ~LSR() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t op1       = boost::get<uint64_t>(operands[0]);
        uint64_t op2       = boost::get<uint64_t>(operands[1]);
        uint64_t data_size = boost::get<uint64_t>(operands[2]);

        uint64_t mask = (data_size == 32) ? 0xffffffff : 0xffffffffffffffff;
        return (op1 >> (op2 % data_size) & mask);
    }
    virtual char const* describe() const { return "LSR"; }
} LSR_;

typedef struct SextB : public Operation
{
    SextB() {}
    virtual ~SextB() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
        if (op1 & (1 << (8 - 1))) op1 |= SIGNED_UPPER_BOUND_B;
        return op1;
    }
    virtual char const* describe() const { return "signed extend Byte"; }
} SextB_;

typedef struct SextH : public Operation
{
    SextH() {}
    virtual ~SextH() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
        if (op1 & (1 << (16 - 1))) op1 |= SIGNED_UPPER_BOUND_H;
        return op1;
    }
    virtual char const* describe() const { return "signed extend Half Word"; }
} SextH_;

typedef struct SextW : public Operation
{
    SextW() {}
    virtual ~SextW() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op1 = (uint64_t)(boost::get<uint64_t>(operands[0]));
        if (op1 & (1 << (32 - 1))) op1 |= SIGNED_UPPER_BOUND_W;
        return op1;
    }
    virtual char const* describe() const { return "signed extend Word"; }
} SextW_;

typedef struct SextX : public Operation
{
    SextX() {}
    virtual ~SextX() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return (uint64_t)(boost::get<uint64_t>(operands[0]) | SIGNED_UPPER_BOUND_X);
    }
    virtual char const* describe() const { return "signed extend Double Word"; }
} SextX_;

typedef struct ZextB : public Operation
{
    ZextB() {}
    virtual ~ZextB() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op = boost::get<uint64_t>(operands[0]);
        op &= ~SIGNED_UPPER_BOUND_B;
        return op;
    }
    virtual char const* describe() const { return "Zero extend Byte"; }
} ZextB_;

typedef struct ZextH : public Operation
{
    ZextH() {}
    virtual ~ZextH() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op = boost::get<uint64_t>(operands[0]);
        op &= ~SIGNED_UPPER_BOUND_H;
        return op;
    }
    virtual char const* describe() const { return "Zero extend Half Word"; }
} ZextH_;

typedef struct ZextW : public Operation
{
    ZextW() {}
    virtual ~ZextW() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        uint64_t op = boost::get<uint64_t>(operands[0]);
        op &= ~SIGNED_UPPER_BOUND_W;
        return op;
    }
    virtual char const* describe() const { return "Zero extend Word"; }
} ZextW_;

typedef struct ZextX : public Operation
{
    ZextX() {}
    virtual ~ZextX() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return boost::get<uint64_t>(operands[0]);
    }
    virtual char const* describe() const { return "Zero extend Double Word"; }
} ZextX_;

typedef struct Xnor : public Operation
{
    Xnor() {}
    virtual ~Xnor() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) ^ ~boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "Xnor"; }
} Xnor_;

typedef struct UMul : public Operation
{
    UMul() {}
    virtual ~UMul() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        bits op0      = boost::get<uint64_t>(operands[0]);
        bits op1      = boost::get<uint64_t>(operands[1]);
        uint64_t prod = uint64_t((op0 * op1) & (bits)0xFFFFFFFFFFFFFFFF);
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "UMul"; }
} UMul_;

typedef struct UMulH : public Operation
{
    UMulH() {}
    virtual ~UMulH() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        bits op0      = boost::get<uint64_t>(operands[0]);
        bits op1      = boost::get<uint64_t>(operands[1]);
        uint64_t prod = (uint64_t)((op0 * op1) >> 64);
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "UMulH"; }
} UMulH_;

typedef struct UMulL : public Operation
{
    UMulL() {}
    virtual ~UMulL() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op0  = (uint32_t)boost::get<uint64_t>(operands[0]);
        uint64_t op1  = (uint32_t)boost::get<uint64_t>(operands[1]);
        uint64_t prod = op0 * op1;
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "UMulL"; }
} UMulL_;

typedef struct SMul : public Operation
{
    SMul() {}
    virtual ~SMul() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op0   = boost::get<uint64_t>(operands[0]) & (uint64_t)0xFFFFFFFFULL;
        uint64_t op1   = boost::get<uint64_t>(operands[1]) & (uint64_t)0xFFFFFFFFULL;
        uint64_t op0_s = op0 | (op0 & (uint64_t)0x80000000ULL ? (uint64_t)0xFFFFFFFF00000000ULL : 0ULL);
        uint64_t op1_s = op1 | (op1 & (uint64_t)0x80000000ULL ? (uint64_t)0xFFFFFFFF00000000ULL : 0ULL);
        uint64_t prod  = op0_s * op1_s;
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "SMul"; }
} SMul_;

typedef struct SMulH : public Operation
{
    SMulH() {}
    virtual ~SMulH() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        int64_t op0   = boost::get<uint64_t>(operands[0]);
        int64_t op1   = boost::get<uint64_t>(operands[1]);
        bits multi    = (bits)((boost::multiprecision::int128_t)op0 * (boost::multiprecision::int128_t)op1);
        uint64_t prod = (uint64_t)((multi & ((bits)0xFFFFFFFFFFFFFFFF << 64)) >> 64);
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "SMulH"; }
} SMulH_;

typedef struct SMulL : public Operation
{
    SMulL() {}
    virtual ~SMulL() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op0  = (int64_t)(int32_t)(boost::get<uint64_t>(operands[0]));
        uint64_t op1  = (int64_t)(int32_t)(boost::get<uint64_t>(operands[1]));
        uint64_t prod = op0 * op1;
        return prod;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "SMulL"; }
} SMulL_;

typedef struct UDiv : public Operation
{
    UDiv() {}
    virtual ~UDiv() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op0 = boost::get<uint64_t>(operands[0]);
        uint64_t op1 = boost::get<uint64_t>(operands[1]);

        if (op1 == 0) return uint64_t(0);

        return op0 / op1;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "UDiv"; }
} UDiv_;

typedef struct SDiv : public Operation
{
    using Operation::Operation;
    virtual ~SDiv() {}
    uint64_t calc(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        int64_t op0;
        int64_t op1;
        if (theSize == 64) {
            op0 = int64_t(boost::get<uint64_t>(operands[0]));
            op1 = int64_t(boost::get<uint64_t>(operands[1]));
        } else {
            op0 = int32_t(boost::get<uint64_t>(operands[0]));
            op1 = int32_t(boost::get<uint64_t>(operands[1]));
        }

        if (op1 == 0) return uint64_t(0);

        return op0 / op1;
    }
    virtual Operand operator()(std::vector<Operand> const& operands) { return calc(operands); }
    virtual Operand evalExtra(std::vector<Operand> const& operands) { return calc(operands) >> 32; }
    virtual char const* describe() const { return "SDiv"; }
} SDiv_;

typedef struct MulX : public Operation
{
    MulX() {}
    virtual ~MulX() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return boost::get<uint64_t>(operands[0]) * boost::get<uint64_t>(operands[1]);
    }
    virtual char const* describe() const { return "MulX"; }
} MulX_;

typedef struct UDivX : public Operation
{
    UDivX() {}
    virtual ~UDivX() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        if (boost::get<uint64_t>(operands[1]) != 0) {
            return boost::get<uint64_t>(operands[0]) / boost::get<uint64_t>(operands[1]);
        } else {
            return uint64_t(0ULL); // Will result in an exception, so it doesn't
                                   // matter what we return
        }
    }
    virtual char const* describe() const { return "UDivX"; }
} UDivX_;

typedef struct SDivX : public Operation
{
    SDivX() {}
    virtual ~SDivX() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        uint64_t op0_s = boost::get<uint64_t>(operands[0]);
        uint64_t op1_s = boost::get<uint64_t>(operands[1]);
        if (op1_s != 0) {
            return op0_s / op1_s;
        } else {
            return uint64_t(0ULL); // Will result in an exception, so it doesn't
                                   // matter what we return
        }
    }
    virtual char const* describe() const { return "SDivX"; }
} SDivX_;

typedef struct MOV_ : public Operation
{
    MOV_() {}
    virtual ~MOV_() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        if (theOperands.size()) {
            DBG_Assert(theOperands.size() == 1);
            return boost::get<uint64_t>(theOperands[0]);

        } else {
            DBG_Assert(operands.size() == 1);
            return boost::get<uint64_t>(operands[0]);
        }
    }
    virtual char const* describe() const { return "MOV"; }
} MOV_;

typedef struct MOVN_ : public Operation
{
    MOVN_() {}
    virtual ~MOVN_() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return ~boost::get<uint64_t>(operands[0]);
    }
    virtual char const* describe() const { return "MOVN"; }
} MOVN_;

typedef struct MOVK_ : public Operation
{
    MOVK_() {}
    virtual ~MOVK_() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t imm  = boost::get<uint64_t>(operands[0]);
        uint64_t rd   = boost::get<uint64_t>(operands[1]);
        uint64_t mask = boost::get<uint64_t>(operands[2]);

        rd &= mask;
        rd |= imm;

        return rd;
    }
    virtual char const* describe() const { return "MOVK"; }
} MOVK_;

typedef struct OVERWRITE_ : public Operation
{
    OVERWRITE_() {}
    virtual ~OVERWRITE_() {}
    virtual Operand operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 3);
        uint64_t lhs  = boost::get<uint64_t>(operands[0]);
        uint64_t rhs  = boost::get<uint64_t>(operands[1]);
        uint64_t mask = boost::get<uint64_t>(operands[2]);

        return ((lhs & mask) | rhs);
    }
    virtual char const* describe() const { return "OVERWRITE_"; }
} OVERWRITE_;

std::unique_ptr<Operation>
shift(uint32_t aType)
{
    std::unique_ptr<Operation> ptr;

    switch (aType) {
        case 0: ptr.reset(new LSL_()); break;
        case 1: ptr.reset(new LSR_()); break;
        case 2: ptr.reset(new ASR_()); break;
        case 3: ptr.reset(new ROR_()); break;
        default: DBG_Assert(false);
    }
    return ptr;
}

std::unique_ptr<Operation>
operation(eOpType aType)
{

    std::unique_ptr<Operation> ptr;
    switch (aType) {
        case kADD_: ptr.reset(new ADD_()); break;
        case kADDS32_: ptr.reset(new ADDS_(32)); break;
        case kADDS64_: ptr.reset(new ADDS_(64)); break;
        case kAND_: ptr.reset(new AND_()); break;
        case kANDS_: ptr.reset(new ANDS_()); break;
        case kANDSN_: ptr.reset(new ANDSN_()); break;
        case kORR_: ptr.reset(new ORR_()); break;
        case kXOR_: ptr.reset(new XOR_()); break;
        case kSUB_: ptr.reset(new SUB_()); break;
        case kSUBS64_: ptr.reset(new SUBS_(64)); break;
        case kSUBS32_: ptr.reset(new SUBS_(32)); break;
        case kAndN_: ptr.reset(new AndN_()); break;
        case kOrN_: ptr.reset(new OrN_()); break;
        case kEoN_: ptr.reset(new EoN_()); break;
        case kXnor_: ptr.reset(new Xnor_()); break;
        case kCONCAT32_: ptr.reset(new CONCAT32_()); break;
        case kCONCAT64_: ptr.reset(new CONCAT64_()); break;
        case kMulX_: ptr.reset(new MulX_()); break;
        case kUMul_: ptr.reset(new UMul_()); break;
        case kUMulH_: ptr.reset(new UMulH_()); break;
        case kUMulL_: ptr.reset(new UMulL_()); break;
        case kSMul_: ptr.reset(new SMul_()); break;
        case kSMulH_: ptr.reset(new SMulH_()); break;
        case kSMulL_: ptr.reset(new SMulL_()); break;
        case kUDivX_: ptr.reset(new UDivX_()); break;
        case kUDiv_: ptr.reset(new UDiv_()); break;
        case kSDiv32_: ptr.reset(new SDiv_(32)); break;
        case kSDiv64_: ptr.reset(new SDiv_(64)); break;
        case kSDivX_: ptr.reset(new SDivX_()); break;
        case kMOV_: ptr.reset(new MOV_()); break;
        case kMOVN_: ptr.reset(new MOVN_()); break;
        case kMOVK_: ptr.reset(new MOVK_()); break;
        case kSextB_: ptr.reset(new SextB_()); break;
        case kSextH_: ptr.reset(new SextH_()); break;
        case kSextW_: ptr.reset(new SextW_()); break;
        case kSextX_: ptr.reset(new SextX_()); break;
        case kZextB_: ptr.reset(new ZextB_()); break;
        case kZextH_: ptr.reset(new ZextH_()); break;
        case kZextW_: ptr.reset(new ZextW_()); break;
        case kZextX_: ptr.reset(new ZextX_()); break;
        case kNot_: ptr.reset(new Not_()); break;
        case kROR_: ptr.reset(new ROR_()); break;
        case kASR_: ptr.reset(new ASR_()); break;
        case kLSR_: ptr.reset(new LSR_()); break;
        case kLSL_: ptr.reset(new LSL_()); break;
        case kOVERWRITE_: ptr.reset(new OVERWRITE_()); break;
        default: DBG_Assert(false, (<< "Unimplemented operation type: " << aType));
    }
    return ptr;
}

} // namespace nDecoder
