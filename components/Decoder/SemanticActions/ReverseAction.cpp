
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
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

            DBG_(Iface, (<< "writing " << out_val << " into " << theOutputCode));
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
    bool the64;

    CRCAction(SemanticInstruction* anInstruction,
              uint32_t aPoly,
              eOperandCode anInputCode,
              eOperandCode anInputCode2,
              eOperandCode anOutputCode,
              bool is64)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theInputCode(anInputCode)
      , theOutputCode(anOutputCode)
      , thePoly(aPoly)
      , the64(is64)
    {
    }

    uint32_t bitReverse(uint32_t input)
    {
        uint32_t out_val = 0;
        for (int i = 0; i < 32; i++) {
            if ((input & (1 << i)) == 1) { out_val |= (1 << (32 - i)); }
        }
        return out_val;
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        Operand in   = theInstruction->operand(theInputCode);
        Operand in2  = theInstruction->operand(theInputCode2);
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
          bool is64)
{
    CRCAction* act = new CRCAction(anInstruction, aPoly, anInputCode, anInputCode2, anOutputCode, is64);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
