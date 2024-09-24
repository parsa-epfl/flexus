
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

struct LoadAction : public PredicatedSemanticAction
{
    eSize theSize;
    eSignCode theSignExtend;
    boost::optional<eOperandCode> theBypass;
    bool theLoadExtended;

    LoadAction(SemanticInstruction* anInstruction,
               eSize aSize,
               eSignCode aSigncode,
               boost::optional<eOperandCode> aBypass,
               bool aLoadExtended)
      : PredicatedSemanticAction(anInstruction, 1, true)
      , theSize(aSize)
      , theSignExtend(aSigncode)
      , theBypass(aBypass)
      , theLoadExtended(aLoadExtended)
    {
    }

    void satisfy(int32_t anArg)
    {
        BaseSemanticAction::satisfy(anArg);
        SEMANTICS_DBG(*this);
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

        if (theLoadExtended) {
            value = core()->retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
        } else {
            value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
        }

        switch (theSize) {
            case kByte:
                value &= 0xFFULL;
                if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80ULL)) {
                    value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
                }
                break;
            case kHalfWord:
                value &= value;
                if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x8000ULL)) {
                    value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
                }
                break;
            case kWord:
                value &= value;
                if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80000000ULL)) {
                    value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
                }
                break;
            case kDoubleWord: break;
            case kQuadWord:
            default: DBG_Assert(false); break;
        }

        theInstruction->setOperand(kResult, (uint64_t)value);
        SEMANTICS_DBG(*this << " received load value=" << std::hex << value);
        if (theBypass) {
            mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
            SEMANTICS_DBG(*this << " bypassing value=" << std::hex << value << " to " << name);
            core()->bypass(name, (uint64_t)value);
        }
        satisfyDependants();
    }

    void doEvaluate() {}

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " LoadAction"; }
};

predicated_dependant_action
loadAction(SemanticInstruction* anInstruction, eSize aSize, eSignCode aSignCode, boost::optional<eOperandCode> aBypass)
{
    LoadAction* act = new LoadAction(anInstruction, aSize, aSignCode, aBypass, false);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

predicated_dependant_action
casAction(SemanticInstruction* anInstruction, eSize aSize, eSignCode aSignCode, boost::optional<eOperandCode> aBypass)
{
    LoadAction* act = new LoadAction(anInstruction, aSize, aSignCode, aBypass, false);
    anInstruction->addNewComponent(act);
    return predicated_dependant_action(act, act->dependance(), act->predicate());
}

} // namespace nDecoder
