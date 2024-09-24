
#include "core/boost_extensions/intrusive_ptr.hpp"

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <iostream>
namespace ll = boost::lambda;
#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "RegisterValueExtractor.hpp"
#include "components/uArch/systemRegister.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "core/debug/debug.hpp"
#include "core/qemu/api.h"
#include "core/target.hpp"
#include "core/types.hpp"

#include <boost/dynamic_bitset.hpp>
#include <boost/none.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

struct WritebackAction : public BaseSemanticAction
{
    eOperandCode theResult;
    eOperandCode theRd;
    bool the64, theSetflags, theSP;

    WritebackAction(SemanticInstruction* anInstruction,
                    eOperandCode aResult,
                    eOperandCode anRd,
                    bool a64,
                    bool aSP,
                    bool setflags = false)
      : BaseSemanticAction(anInstruction, 1)
      , theResult(aResult)
      , theRd(anRd)
      , the64(a64)
      , theSetflags(setflags)
      , theSP(aSP)
    {
    }

    void squash(int32_t anArg)
    {
        if (!cancelled()) {
            mapped_reg name = theInstruction->operand<mapped_reg>(theRd);
            core()->squashRegister(name);
            DBG_(VVerb, (<< *this << " squashing register rd=" << name));
        }
        BaseSemanticAction::squash(anArg);
    }

    void doEvaluate()
    {

        if (ready()) {
            DBG_(Iface, (<< "Writing " << theResult << " to " << theRd));

            register_value result =
              boost::apply_visitor(register_value_extractor(), theInstruction->operand(theResult));
            uint64_t res = boost::get<uint64_t>(result);

            mapped_reg name = theInstruction->operand<mapped_reg>(theRd);
            char r          = the64 ? 'X' : 'W';
            if (!the64) {
                res &= 0xFFFFFFFF;
                result = res;
                theInstruction->setOperand(theResult, res);
            }
            DBG_(Iface, (<< "Writing " << std::hex << result << std::dec << " to " << r << "-REG ->" << name));
            core()->writeRegister(name, result, !the64);
            DBG_(VVerb, (<< *this << " rd= " << name << " result=" << result));
            core()->bypass(name, result);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " WritebackAction to" << theRd;
    }
};

struct WriteccAction : public BaseSemanticAction
{
    eOperandCode theCC;
    bool the64;

    WriteccAction(SemanticInstruction* anInstruction, eOperandCode anCC, bool an64)
      : BaseSemanticAction(anInstruction, 1)
      , theCC(anCC)
      , the64(an64)
    {
    }

    void squash(int32_t anArg)
    {
        if (!cancelled()) {
            mapped_reg name = theInstruction->operand<mapped_reg>(theCC);
            core()->squashRegister(name);
            DBG_(VVerb, (<< *this << " squashing register cc=" << name));
        }
        BaseSemanticAction::squash(anArg);
    }

    void doEvaluate()
    {

        if (ready()) {
            mapped_reg name = theInstruction->operand<mapped_reg>(theCC);
            uint64_t ccresult =
              Flexus::Qemu::Processor::getProcessor(theInstruction->cpu()).read_register(Flexus::Qemu::API::PSTATE);
            uint64_t flags = boost::get<uint64_t>(
              boost::apply_visitor(register_value_extractor(), theInstruction->operand(kResultCC)));
            if (!the64) {
                uint64_t result = boost::get<uint64_t>(
                  boost::apply_visitor(register_value_extractor(), theInstruction->operand(kResult)));
                flags &= ~PSTATE_N;
                flags |= (result & ((uint64_t)1 << 31)) ? PSTATE_N : 0;
                flags |= !(result & 0xFFFFFFFF) ? PSTATE_Z : 0;
                flags |= ((result >> 32) == 1) ? PSTATE_C : 0;
            }
            ccresult &= ~(0xF << 28);
            ccresult |= flags;
            DBG_(Iface, (<< "Writing to CC: " << std::hex << ccresult << ", " << std::dec << *theInstruction));
            core()->writeRegister(name, ccresult, false);
            core()->bypass(name, ccresult);
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " WriteCCAction to" << theCC;
    }
};

dependant_action
writebackAction(SemanticInstruction* anInstruction,
                eOperandCode aRegisterCode,
                eOperandCode aMappedRegisterCode,
                bool is64,
                bool aSP,
                bool setflags)
{
    WritebackAction* act = new WritebackAction(anInstruction, aRegisterCode, aMappedRegisterCode, is64, aSP, setflags);
    anInstruction->addNewComponent(act);
    return dependant_action(act, act->dependance());
}

dependant_action
writeccAction(SemanticInstruction* anInstruction, eOperandCode aMappedRegisterCode, bool is64)
{
    WriteccAction* act = new WriteccAction(anInstruction, aMappedRegisterCode, is64);
    anInstruction->addNewComponent(act);
    return dependant_action(act, act->dependance());
}

} // namespace nDecoder
