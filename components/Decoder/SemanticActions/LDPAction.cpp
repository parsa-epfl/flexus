
#include <boost/function.hpp>
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
using nuArch::Instruction;

struct LDPAction : public PredicatedSemanticAction
{
    eSize theSize;
    eSignCode theSignExtend;
    boost::optional<eOperandCode> theBypass0;
    boost::optional<eOperandCode> theBypass1;
    bool theLoadExtended;

    LDPAction(SemanticInstruction* anInstruction,
              eSize aSize,
              eSignCode aSigncode,
              boost::optional<eOperandCode> aBypass0,
              boost::optional<eOperandCode> aBypass1)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theSize(aSize)
      , theSignExtend(aSigncode)
      , theBypass0(aBypass0)
      , theBypass1(aBypass1)
    {
    }

    void satisfy(int32_t anArg)
    {
        BaseSemanticAction::satisfy(anArg);
        SEMANTICS_DBG(*theInstruction);
        if (!cancelled() && ready() && thePredicate) { doLoad(); }
    }

    void predicate_on(int32_t anArg)
    {
        PredicatedSemanticAction::predicate_on(anArg);
        if (!cancelled() && ready() && thePredicate) { doLoad(); }
    }

    void doLoad()
    {
        SEMANTICS_DBG(*this);
        bits value;
        uint64_t size;
        value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
        switch (theSize) {
            case kQuadWord: size = 128; break;
            default: size = 64; break;
        }

        Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
        uint64_t addr             = theInstruction->operand<uint64_t>(kAddress);
        uint64_t addr_final       = addr + (size >> 3) - 1;
        bits value_new            = c.read_va(VirtualMemoryAddress(addr), size >> 3, theInstruction->unprivAccess());
        if (((addr & 0x1000) != (addr_final & 0x1000)) && value != value_new) {
            DBG_(Iface,
                 (<< theInstruction->identify() << " Correcting LDP access across pages at address: " << std::hex
                  << addr << ", size: " << size << ", original: " << value << ", final: " << value_new));
            core()->setLoadValue(boost::intrusive_ptr<Instruction>(theInstruction), value_new);
            value = value_new;
        }
        std::pair<uint64_t, uint64_t> pairValues = splitBits(value, size);

        theInstruction->setOperand(kResult, pairValues.second);
        theInstruction->setOperand(kResult1, pairValues.first);

        DBG_(Iface, (<< *this << " received pair load value = " << std::hex << value << std::dec));
        DBG_(Iface,
             (<< *this << " received load values = " << std::hex << pairValues.first << " and " << pairValues.second
              << std::dec));

        if (theBypass0) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass0);
            core()->bypass(name, pairValues.second);
        }
        if (theBypass1) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass1);
            core()->bypass(name, pairValues.first);
        }
        satisfyDependants();
    }

    void doEvaluate() {}

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " LDPAction"; }
};

predicated_dependant_action
ldpAction(SemanticInstruction* anInstruction,
          eSize aSize,
          eSignCode aSignCode,
          boost::optional<eOperandCode> aBypass0,
          boost::optional<eOperandCode> aBypass1)
{
    LDPAction* act = new LDPAction(anInstruction, aSize, aSignCode, aBypass0, aBypass1);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}
predicated_dependant_action
caspAction(SemanticInstruction* anInstruction,
           eSize aSize,
           eSignCode aSignCode,
           boost::optional<eOperandCode> aBypass0,
           boost::optional<eOperandCode> aBypass1)
{
    LDPAction* act = new LDPAction(anInstruction, aSize, aSignCode, aBypass0, aBypass1);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}
} // namespace nDecoder