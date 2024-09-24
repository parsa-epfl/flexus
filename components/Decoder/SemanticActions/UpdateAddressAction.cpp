
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
using nuArch::Instruction;

struct UpdateAddressAction : public BaseSemanticAction
{

    eOperandCode theAddressCode, theAddressCode2;
    bool thePair, theVirtual;

    UpdateAddressAction(SemanticInstruction* anInstruction, eOperandCode anAddressCode, bool isVirtual)
      : BaseSemanticAction(anInstruction, 2)
      , theAddressCode(anAddressCode)
      , theVirtual(isVirtual)
    {
    }

    void squash(int32_t anOperand)
    {
        if (!cancelled()) {
            DBG_(VVerb, (<< *this << " Squashing vaddr."));
            core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction), kUnresolved /*, 0x80*/);
        }
        BaseSemanticAction::squash(anOperand);
    }

    void satisfy(int32_t anOperand)
    {
        // updateAddress as soon as dependence is satisfied
        BaseSemanticAction::satisfy(anOperand);
        updateAddress();
    }

    void doEvaluate()
    {
        // Address is now updated when satisfied.
    }

    void updateAddress()
    {
        if (ready()) {
            DBG_(Iface, (<< "Executing " << *this));

            if (theVirtual) {
                DBG_Assert(theInstruction->hasOperand(theAddressCode));

                uint64_t addr = theInstruction->operand<uint64_t>(theAddressCode);
                if (theInstruction->hasOperand(kUopAddressOffset)) {
                    uint64_t offset = theInstruction->operand<uint64_t>(kUopAddressOffset);
                    DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address " << std::hex
                                                   << addr << std::dec);
                    addr += offset;
                    theInstruction->setOperand(theAddressCode, addr);
                    DECODER_DBG("final address is " << std::hex << addr << std::dec);
                } else if (theInstruction->hasOperand(kSopAddressOffset)) {
                    int64_t offset = theInstruction->operand<int64_t>(kSopAddressOffset);
                    DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address " << std::hex
                                                   << addr << std::dec);
                    addr += offset;
                    theInstruction->setOperand(theAddressCode, addr);
                    DECODER_DBG("final address is " << std::hex << addr << std::dec);
                }
                VirtualMemoryAddress vaddr(addr);
                core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction), vaddr);
                SEMANTICS_DBG(*this << " updating vaddr = " << vaddr);

            } else {
                DBG_Assert(false);
            }
            satisfyDependants();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " UpdateAddressAction"; }
};

multiply_dependant_action
updateVirtualAddressAction(SemanticInstruction* anInstruction, eOperandCode aCode)
{
    UpdateAddressAction* act = new UpdateAddressAction(anInstruction, aCode, true);
    anInstruction->addNewComponent(act);
    std::vector<InternalDependance> dependances;
    dependances.push_back(act->dependance(0));
    dependances.push_back(act->dependance(1));
    return multiply_dependant_action(act, dependances);
}

} // namespace nDecoder
