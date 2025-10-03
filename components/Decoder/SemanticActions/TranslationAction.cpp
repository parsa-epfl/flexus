
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

struct CalcAddressUpdateVATranslateMaybeRegExtendAndShiftAction : public BaseSemanticAction
{
    bool theRegExtend;
    std::unique_ptr<Operation> theRegExtendType;
    bool theShift;
    CalcAddressUpdateVATranslateMaybeRegExtendAndShiftAction(SemanticInstruction* anInstruction, bool aRegExtend, std::unique_ptr<Operation> aRegExtendType, bool aShift)
      : BaseSemanticAction(anInstruction, aRegExtend ? 2 : 1, true)
        , theRegExtend(aRegExtend)
        , theShift(aShift)
    {
        theRegExtendType = std::move(aRegExtendType);
        theEU = eAGU;
    }

    void squash(int32_t anOperand)
    {
        if (!cancelled()) {
            DBG_(VVerb, (<< *this << " Squashing vaddr."));
            core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction), kUnresolved /*, 0x80*/);
        }

        if (!cancelled()) {
            DBG_(VVerb, (<< *this << " Squashing paddr."));
            core()->resolvePAddr(boost::intrusive_ptr<Instruction>(theInstruction), (PhysicalMemoryAddress)kUnresolved);
        }
        boost::intrusive_ptr<Instruction>(theInstruction)->setResolved(false);
        BaseSemanticAction::squash(anOperand);

    }

    bool canDispatch() {
        return core()->canExecute(theEU);
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (ready()) {
            if (theInstruction->hasPredecessorExecuted()) {
                    if (!core()->reqEU(eAGU)) {
                    reschedule();
                    return;
                }
                // Reg immediate case with possible shift
                uint64_t op2, op3, address;
                if (theRegExtend) {
                    op2 = boost::get<uint64_t>(theRegExtendType->operator()({theInstruction->operand<uint64_t>(kOperand3)}));
                    if (theShift) {
                        std::vector<Operand> operands = { Operand(op2), theInstruction->operand<uint64_t>(kResult2), theInstruction->operand<uint64_t>(kOperand4) };
                        op2 = boost::get<uint64_t>(operation(kLSL_)->operator()(operands));
                        theInstruction->setOperand(kOperand2, op2);
                    } else {
                        theInstruction->setOperand(kOperand2, op2);
                    }
                }

                // calculate the address
                std::vector<Operand> operands;
                operands.push_back(theInstruction->operand<uint64_t>(kOperand1));
                if (theRegExtend) {
                    operands.push_back(op2);
                }
                address = boost::get<uint64_t>(operation(kADD_)->operator()(operands));
                theInstruction->setOperand(kAddress, address);
                
                // update virtual address
                if (theInstruction->hasOperand(kUopAddressOffset)) {
                    uint64_t offset = theInstruction->operand<uint64_t>(kUopAddressOffset);
                    DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address " << std::hex
                                                   << address << std::dec);
                    address += offset;
                    theInstruction->setOperand(kAddress, address);
                    DECODER_DBG("final address is " << std::hex << address << std::dec);
                } else if (theInstruction->hasOperand(kSopAddressOffset)) {
                    int64_t offset = theInstruction->operand<int64_t>(kSopAddressOffset);
                    DECODER_DBG("adding offset 0x" << std::hex << offset << std::dec << " to address " << std::hex
                                                   << address << std::dec);
                    address += offset;
                    theInstruction->setOperand(kAddress, address);
                    DECODER_DBG("final address is " << std::hex << address << std::dec);
                }
                VirtualMemoryAddress vaddr(address);
                core()->resolveVAddr(boost::intrusive_ptr<Instruction>(theInstruction), vaddr);
                SEMANTICS_DBG(*this << " updating vaddr = " << vaddr);


                // translate the address
                theInstruction->core()->translate(boost::intrusive_ptr<Instruction>(theInstruction));

                satisfyDependants();
            } else {
                DBG_(VVerb, (<< *this << " waiting for predecessor "));
                reschedule();
            }
        } else {
            reschedule();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " CalcAddressUpdateVATranslateAction"; }
};

struct TranslationAction : public BaseSemanticAction
{

    TranslationAction(SemanticInstruction* anInstruction)
      : BaseSemanticAction(anInstruction, 1)
    {
        theEU = eAGU;
    }

    void squash(int32_t anOperand)
    {
        if (!cancelled()) {
            DBG_(VVerb, (<< *this << " Squashing paddr."));
            core()->resolvePAddr(boost::intrusive_ptr<Instruction>(theInstruction), (PhysicalMemoryAddress)kUnresolved);
        }
        boost::intrusive_ptr<Instruction>(theInstruction)->setResolved(false);
        BaseSemanticAction::squash(anOperand);
    }

    bool canDispatch() {
        return core()->canExecute(eAGU);
    }

    void doEvaluate()
    {
        SEMANTICS_DBG(*this);

        if (ready()) {
            if (!core()->reqEU(eAGU)) {
                reschedule();
                return;
            }

            DBG_Assert(theInstruction->hasOperand(kAddress));
            VirtualMemoryAddress addr(theInstruction->operand<uint64_t>(kAddress));

            theInstruction->core()->translate(boost::intrusive_ptr<Instruction>(theInstruction));

            satisfyDependants();
        } else {
            reschedule();
        }
    }

    void describe(std::ostream& anOstream) const { anOstream << theInstruction->identify() << " TranslationAction"; }
};

simple_action
translationAction(SemanticInstruction* anInstruction)
{
    TranslationAction* act = new TranslationAction(anInstruction);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

simple_action
calcAddressUpdateVATranslateMaybeRegExtendAndShiftAction(SemanticInstruction* anInstruction,
                                                            std::vector<std::list<InternalDependance>>& opDeps,
                                                            bool aRegExtend,
                                                            std::unique_ptr<Operation> aRegExtendType,
                                                            std::vector<std::list<InternalDependance>>& opDepsExtend,
                                                            bool aShift)
{
    CalcAddressUpdateVATranslateMaybeRegExtendAndShiftAction* act = new CalcAddressUpdateVATranslateMaybeRegExtendAndShiftAction(anInstruction, aRegExtend, std::move(aRegExtendType), aShift);
    anInstruction->addNewComponent(act);
    for (uint32_t i = 0; i < opDeps.size(); ++i) {
        opDeps[i].push_back(act->dependance(i));
    }
    if (aRegExtend) {
        for (uint32_t i = 0; i < opDepsExtend.size(); ++i) {
            opDepsExtend[i].push_back(act->dependance(i + opDeps.size()));
        }
    }
    return simple_action(act);
}

} // namespace nDecoder
