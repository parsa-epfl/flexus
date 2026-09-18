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

struct MapDestInOrderAction : public BaseSemanticAction {
    eOperandCode theRd;

    MapDestInOrderAction(SemanticInstruction *anInstruction, eOperandCode anRd)
        : BaseSemanticAction(anInstruction, 1, true), theRd(anRd) {
        setReady(0, true);
    }

    void doEvaluate() {
        if (ready()) {
            core()->mapDestInOrder(theInstruction->sequenceNo(), theInstruction->operand<mapped_reg>(theRd));
            satisfyDependants();
        }
    }

    bool canDispatch() {
        return core()->canDispatch(theInstruction->operand<mapped_reg>(theRd), false);
    }

    void describe(std::ostream &anOstream) const {
        anOstream << theInstruction->identify() << " MapInOrderAction to " << theRd;
    }
};

simple_action mapDestInOrderAction(SemanticInstruction *anInstruction, eOperandCode aMappedRegisterCode) {
    MapDestInOrderAction *act = new MapDestInOrderAction(anInstruction, aMappedRegisterCode);
    anInstruction->addNewComponent(act);
    return simple_action(act);
}

}
