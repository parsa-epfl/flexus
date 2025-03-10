
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

struct ExclusiveMonitorAction : public PredicatedSemanticAction
{
    eOperandCode theAddressCode, theRegisterCode;
    eSize theSize;
    boost::optional<eOperandCode> theBypass;

    ExclusiveMonitorAction(SemanticInstruction* anInstruction,
                           eOperandCode anAddressCode,
                           eSize aSize,
                           boost::optional<eOperandCode> aBypass)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theAddressCode(anAddressCode)
      , theSize(aSize)
      , theBypass(aBypass)
    {
    }

    void doEvaluate()
    {
        DBG_(VVerb, (<< *this));
        uint64_t addr             = theInstruction->operand<uint64_t>(theAddressCode);
        Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
        PhysicalMemoryAddress pAddress =
          PhysicalMemoryAddress(c.translate_va2pa(VirtualMemoryAddress((addr >> 6) << 6), theInstruction->unprivAccess()));

        // passed &= core()->isExclusiveGlobal(pAddress, theSize);
        // passed &= core()->isExclusiveVA(VirtualMemoryAddress(addr), theSize);
        if (core()->isROBHead(theInstruction)) {
            int passed = core()->isExclusiveLocal(pAddress, theSize);
            if (passed == kMonitorDoesntExist) {
                DBG_(Dev, (<< "ExclusiveMonitorAction resulted in kMonitorDoesntExist!"));
                passed = kMonitorUnset;
            }
            core()->markExclusiveLocal(pAddress, theSize, kMonitorDoesntExist);
            theInstruction->setOperand(kResult, (uint64_t)passed);
            DBG_(VVerb,
                 (<< "Exclusive Monitor resulted in " << passed << " for address " << pAddress << ", "
                  << *theInstruction));
            if (theBypass) {
                mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
                SEMANTICS_DBG(*this << " bypassing value=" << std::hex << passed << " to " << name);
                core()->bypass(name, (uint64_t)passed);
            }
            satisfyDependants();
        } else {
            DBG_(VVerb, (<< "Recheduling Exclusive Monitor for address " << pAddress << ", " << *theInstruction));
            setReady(0, false);
            satisfy(0);
        }
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ExclusiveMonitor " << theAddressCode;
    }
};

predicated_action
exclusiveMonitorAction(SemanticInstruction* anInstruction,
                       eOperandCode anAddressCode,
                       eSize aSize,
                       boost::optional<eOperandCode> aBypass)
{
    ExclusiveMonitorAction* act = new ExclusiveMonitorAction(anInstruction, anAddressCode, aSize, aBypass);
    anInstruction->addNewComponent(act);
    return predicated_action(act, act->predicate());
}

} // namespace nDecoder
