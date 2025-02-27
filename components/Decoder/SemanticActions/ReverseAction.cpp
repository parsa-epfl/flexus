
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <cstdint>
#include <iomanip>
#include <iostream>
namespace ll = boost::lambda;

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/none.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

struct ReverseAction : public PredicatedSemanticAction
{
    eOperandCode theInputCode;
    eOperandCode theOutputCode;
    bool the64;

    ReverseAction(SemanticInstruction* anInstruction, eOperandCode anInputCode, eOperandCode anOutputCode, bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
      , the64(is64)
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        Operand in       = theInstruction->operand(theInputCode);
        uint64_t in_val  = boost::get<uint64_t>(in);
        int dbsize       = the64 ? 64 : 32;
        uint64_t out_val = 0;
        for (int i = 0; i < dbsize; i++) {
            if (in_val & ((uint64_t)1 << i)) { out_val |= ((uint64_t)1 << (dbsize - i - 1)); }
        }

        theInstruction->setOperand(theOutputCode, out_val);
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " ReverseAction "; }
};

struct ReorderAction : public PredicatedSemanticAction
{
    eOperandCode theInputCode;
    eOperandCode theOutputCode;
    uint8_t theContainerSize;
    bool the64;

    ReorderAction(SemanticInstruction* anInstruction,
                  eOperandCode anInputCode,
                  eOperandCode anOutputCode,
                  uint8_t aContainerSize,
                  bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
      , theContainerSize(aContainerSize)
      , the64(is64)
    {
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (ready()) {
            Operand in       = theInstruction->operand(theInputCode);
            uint64_t in_val  = boost::get<uint64_t>(in);
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
                    if (the64) out_val |= (((uint64_t)(__builtin_bswap32((uint32_t)(in_val >> 32)))) << 32);
                    break;
                case 64: out_val = (__builtin_bswap64((uint64_t)in_val)); break;
            }

            theInstruction->setOperand(theOutputCode, out_val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " ReorderAction "; }
};

struct CountAction : public PredicatedSemanticAction
{
    eOperandCode theInputCode;
    eOperandCode theOutputCode;
    eCountOp theCountOp;
    bool the64;

    CountAction(SemanticInstruction* anInstruction,
                eOperandCode anInputCode,
                eOperandCode anOutputCode,
                eCountOp aCountOp,
                bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
      , theCountOp(aCountOp)
      , the64(is64)
    {
    }

    void doEvaluate()
    {
        if (ready()) {
            SEMANTICS_DBG(*this);

            Operand in       = theInstruction->operand(theInputCode);
            uint64_t in_val  = boost::get<uint64_t>(in);
            uint64_t out_val = 0;

            switch (theCountOp) {
                case kCountOp_CLZ:
                    out_val = the64 ? (((in_val >> 32) == 0) ? (32 + clz32((uint32_t)in_val)) : (clz32(in_val >> 32)))
                                    : (clz32((uint32_t)in_val));
                    break;
                case kCountOp_CLS: {
                    bits mask;
                    if (the64) {
                        mask = 0x7FFFFFFFFFFFFFFFULL;
                    } else {
                        mask = 0x000000007FFFFFFFULL;
                    }
                    bits i  = (in_val >> 1) ^ (in_val & mask);
                    out_val = the64 ? (((i >> 32) == 0) ? (31 + clz32((uint32_t)i)) : clz32((uint32_t)(i >> 32)))
                                    : (clz32((uint32_t)i));
                    break;
                }
                default: DBG_Assert(false); break;
            }

            DBG_(VVerb, (<< "writing " << out_val << " into " << theOutputCode));
            theInstruction->setOperand(theOutputCode, out_val);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " CountAction "; }
};

struct CRCAction : public PredicatedSemanticAction
{
    eOperandCode theInputCode, theInputCode2;
    eOperandCode theOutputCode;
    uint32_t thePoly;
    uint32_t theSize;

    CRCAction(SemanticInstruction* anInstruction,
              uint32_t aPoly,
              eOperandCode anInputCode,
              eOperandCode anInputCode2,
              eOperandCode anOutputCode,
              uint32_t aSize)
      : PredicatedSemanticAction(anInstruction, 2, true)
      , theInputCode(anInputCode)
      , theInputCode2(anInputCode2)
      , theOutputCode(anOutputCode)
      , thePoly(aPoly)
      , theSize(aSize)
    {
    }

    uint64_t bitReverse(uint64_t input, uint32_t limit)
    {
        uint64_t out_val = 0;
        for (uint32_t i = 0; i < limit; i++) {
            if (input & (1ULL << i)) { out_val |= (1ULL << (limit - i - 1)); }
        }
        return out_val;
    }

    // https://developer.arm.com/documentation/ddi0602/2024-12/Base-Instructions/CRC32B--CRC32H--CRC32W--CRC32X--CRC32-checksum-
    // constant bits(32)      acc     = X[n, 32];     // accumulator
    // constant bits(size)    val     = X[m, size];   // input value
    // constant bits(32)      poly    = 0x04C11DB7<31:0>;

    // constant bits(32+size) tempacc = BitReverse(acc):Zeros(size);
    // constant bits(size+32) tempval = BitReverse(val):Zeros(32);

    // // Poly32Mod2 on a bitstring does a polynomial Modulus over {0,1} operation
    // X[d, 32] = BitReverse(Poly32Mod2(tempacc EOR tempval, poly));
    
    // // Poly32Mod2()
    // // ============
    // // Poly32Mod2 on a bitstring does a polynomial Modulus over {0,1} operation
    // bits(32) Poly32Mod2(bits(N) data_in, bits(32) poly)
    //     assert N > 32;
    //     bits(N) data = data_in;
    //     for i = N-1 downto 32
    //         if data<i> == '1' then
    //             data<i-1:0> = data<i-1:0> EOR (poly:Zeros(i-32));
    //     return data<31:0>;

    uint32_t poly32Mod2(boost::multiprecision::uint128_t data_in, uint32_t poly) {
        boost::multiprecision::uint128_t data = data_in;
        for (int i = 127; i >= 32; i--) {
            if (data & (static_cast<boost::multiprecision::uint128_t>(1) << i)) {
                data ^= (static_cast<boost::multiprecision::uint128_t>(poly)) << (i - 32);
            }
        }
        return static_cast<uint32_t>(data);
    }
    

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        Operand in   = theInstruction->operand(theInputCode);
        Operand in2  = theInstruction->operand(theInputCode2);
        uint32_t acc = static_cast<uint32_t>(boost::get<uint64_t>(in));
        uint64_t val = boost::get<uint64_t>(in2);
        val          = val & ((1ULL << theSize) - 1);

        boost::multiprecision::uint128_t tempacc = static_cast<boost::multiprecision::uint128_t>(bitReverse(acc, 32)) << theSize;
        boost::multiprecision::uint128_t tempval = static_cast<boost::multiprecision::uint128_t>(bitReverse(val, theSize)) << 32;

        // Poly32Mod2 on a bitstring does a polynomial Modulus over {0,1} operation;
        boost::multiprecision::uint128_t data = tempacc ^ tempval;

        uint32_t polyres = poly32Mod2(data, thePoly);
        uint32_t data_out = bitReverse(polyres, 32);

        theInstruction->setOperand(theOutputCode, static_cast<uint64_t>(data_out));
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " CRCAction "; }
};

predicated_action
reverseAction(SemanticInstruction* anInstruction,
              eOperandCode anInputCode,
              eOperandCode anOutputCode,
              std::vector<std::list<InternalDependance>>& rs_deps,
              bool is64)
{
    ReverseAction* act = new ReverseAction(anInstruction, anInputCode, anOutputCode, is64);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < rs_deps.size(); ++i) {
        rs_deps[i].push_back(act->dependance(i));
    }
    return predicated_action(act, act->predicate());
}

predicated_action
reorderAction(SemanticInstruction* anInstruction,
              eOperandCode anInputCode,
              eOperandCode anOutputCode,
              uint8_t aContainerSize,
              std::vector<std::list<InternalDependance>>& rs_deps,
              bool is64)
{
    ReorderAction* act = new ReorderAction(anInstruction, anInputCode, anOutputCode, aContainerSize, is64);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < rs_deps.size(); ++i) {
        rs_deps[i].push_back(act->dependance(i));
    }
    return predicated_action(act, act->predicate());
}

predicated_action
countAction(SemanticInstruction* anInstruction,
            eOperandCode anInputCode,
            eOperandCode anOutputCode,
            eCountOp aCountOp,
            std::vector<std::list<InternalDependance>>& rs_deps,
            bool is64)
{
    CountAction* act = new CountAction(anInstruction, anInputCode, anOutputCode, aCountOp, is64);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < rs_deps.size(); ++i) {
        rs_deps[i].push_back(act->dependance(i));
    }

    return predicated_action(act, act->predicate());
}
predicated_action
crcAction(SemanticInstruction* anInstruction,
          uint32_t aPoly,
          eOperandCode anInputCode,
          eOperandCode anInputCode2,
          eOperandCode anOutputCode,
          std::vector<std::list<InternalDependance>>& rs_deps,
          uint32_t size)
{
    CRCAction* act = new CRCAction(anInstruction, aPoly, anInputCode, anInputCode2, anOutputCode, size);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < rs_deps.size(); ++i) {
        rs_deps[i].push_back(act->dependance(i));
    }

    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
