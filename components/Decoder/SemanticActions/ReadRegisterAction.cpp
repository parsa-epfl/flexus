
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

struct ReadRegisterAction : public BaseSemanticAction
{
    eOperandCode theRegisterCode;
    eOperandCode theOperandCode;
    bool theConnected;
    bool the64;
    bool theSP;

    ReadRegisterAction(SemanticInstruction* anInstruction,
                       eOperandCode aRegisterCode,
                       eOperandCode anOperandCode,
                       bool aSP,
                       bool is64)
      : BaseSemanticAction(anInstruction, 1)
      , theRegisterCode(aRegisterCode)
      , theOperandCode(anOperandCode)
      , theConnected(false)
      , the64(is64)
      , theSP(aSP)
    {
    }

    bool bypass(register_value aValue)
    {
        theSP = false;

        if (!theSP) {
            if (cancelled() || theInstruction->isRetired() || theInstruction->isSquashed()) { return true; }

            if (!signalled()) {
                aValue = (uint64_t)(boost::get<uint64_t>(aValue) & (the64 ? -1LL : 0xFFFFFFFF));
                theInstruction->setOperand(theOperandCode, aValue);
                DBG_(
                  VVerb,
                  (<< *this << " bypassed " << theRegisterCode << " = " << aValue << " written to " << theOperandCode));
                satisfyDependants();
                setReady(0, true);
            }
            return false;
        }
        return true;
    }

    void doEvaluate()
    {
        DBG_(Iface, (<< *this));

        theSP = false;

        register_value aValue;
        uint64_t val;
        if (theSP) {
            if (core()->_PSTATE().SP() == 0) {
                val = core()->getSP_el(EL0);
            } else {
                switch (core()->_PSTATE().EL()) {
                    case EL0: val = core()->getSP_el(EL0); break;
                    case EL1: val = core()->getSP_el(EL1); break;
                    case EL2:
                    case EL3:
                    default:
                        DBG_Assert(false);
                        //                    theInstruction->setWillRaise(kException_ILLEGALSTATE);
                        //                    core()->takeTrap(boost::intrusive_ptr<Instruction>(theInstruction),
                        //                    theInstruction->willRaise());
                        break;
                }
            }
        } else {
            if (!theConnected) {

                mapped_reg name = theInstruction->operand<mapped_reg>(theRegisterCode);
                setReady(0,
                         core()->requestRegister(name, theInstruction->makeInstructionDependance(dependance())) ==
                           kReady);
                core()->connectBypass(name, theInstruction, ll::bind(&ReadRegisterAction::bypass, this, ll::_1));
                theConnected = true;
            }
            if (!signalled()) {
                SEMANTICS_DBG("Signalling");

                mapped_reg name        = theInstruction->operand<mapped_reg>(theRegisterCode);
                eResourceStatus status = core()->requestRegister(name);

                if (status == kReady) {
                    aValue = core()->readRegister(name);
                    val    = boost::get<uint64_t>(aValue);
                } else {
                    setReady(0, false);
                    return;
                }
            } else {
                return;
            }
        }

        if (!the64) { val &= 0xffffffff; }
        aValue = val;

        DBG_(Iface, (<< "Reading register " << theRegisterCode << " with a value " << std::hex << aValue << std::dec));

        theInstruction->setOperand(theOperandCode, val);
        satisfyDependants();
    }

    void describe(std::ostream& anOstream) const
    {
        anOstream << theInstruction->identify() << " ReadRegister " << theRegisterCode << " store in "
                  << theOperandCode;
    }
};

simple_action
readRegisterAction(SemanticInstruction* anInstruction,
                   eOperandCode aRegisterCode,
                   eOperandCode anOperandCode,
                   bool aSP,
                   bool is64)
{
    ReadRegisterAction* act = new ReadRegisterAction(anInstruction, aRegisterCode, anOperandCode, aSP, is64);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

} // namespace nDecoder
