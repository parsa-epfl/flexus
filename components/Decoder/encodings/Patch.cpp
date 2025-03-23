#include <vector>

#include "Patch.h"

#include "components/CommonQEMU/Slices/MemOp.hpp"
#include "components/Decoder/OperandCode.hpp"
#include "components/Decoder/Validations.hpp"
#include "components/Decoder/encodings/SharedFunctions.hpp"


using namespace nDecoder;

static std::vector<InstrPatch *> _patches [16][16];

InstrPatch *nDecoder::hitPatch(uint32_t raw) {
    auto x = (raw >> 28);
    auto y = (raw >> 24) & 0xf;

    for (auto &mat: _patches[x][y])
        if (mat->hit(raw))
            return mat;

    return nullptr;
}

static void addPatch(InstrPatch *pat) {
    auto min = pat->min() >> 24;
    auto max = pat->max() >> 24;

    for (int i = min; i <= max; i++) {
        auto x = i >> 0x4;
        auto y = i &  0xf;

        _patches[x][y].push_back(pat);
    }
}

class AMO: public InstrPatch {
public:
    AMO(): InstrPatch("ee111000ar1ssssswccc00nnnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto size = map.at('e');
        auto a    = map.at('a');
        auto r    = map.at('r');
        auto rs   = map.at('s');
        auto w    = map.at('w');
        auto opc  = map.at('c');
        auto rn   = map.at('n');
        auto rt   = map.at('t');

        ins->setClass(clsAtomic, codeRMW);
        ins->setExclusive();

        std::vector<std::list<InternalDependance>> rn_dep(1);
        std::vector<std::list<InternalDependance>> rs_dep(1);

        addAddressCompute(ins, rn_dep);

        boost::optional<eOperandCode> byp = boost::none;

        auto st =  rt   == 0x1f;

        auto sz = (size == 0       ) ? kByte       :
                  (size == 1       ) ? kHalfWord   :
                  (size == 2       ) ? kWord       :
                                       kDoubleWord;

        auto op =  w                 ? kAMOSWP     :
                  (opc  == 0       ) ? kAMOADD     :
                  (opc  == 1       ) ? kAMOCLR     :
                  (opc  == 2       ) ? kAMOOR      :
                  (opc  == 3       ) ? kAMOSET     :
                  (opc  == 4       ) ? kAMOSMAX    :
                  (opc  == 5       ) ? kAMOSMIN    :
                  (opc  == 6       ) ? kAMOUMAX    :
                                       kAMOUMIN;

        auto se = (op   == kAMOSMAX) ? kSignExtend :
                  (op   == kAMOSMIN) ? kSignExtend :
                                       kNoExtension;

        if (sz < kWord)
            op = (eRMWOperation)((int)(op) + size);

        if (!st)
            byp = kPD;

        auto ld  = amoAction(ins, sz, se, byp);
        auto rmw = updateRMWValueAction(ins, kOperand2, kLastOperandCode, kResult1, op);

        auto rrn = addReadXRegister(ins, 1, rn, rn_dep[0], true);
        auto rrs = addReadXRegister(ins, 2, rs, rs_dep[0], sz == kDoubleWord);

        if (!st)
            addDestination(ins, rt, ld, se);

        connectDependance(rmw.action->dependance(0), ld);
        connectDependance(rmw.action->dependance(1), rrs);
        connectDependance(ins->retirementDependance(), rmw);

        ins->setMayCommit(false);

        ins->addDispatchEffect  (allocateRMW(ins, sz, ld.dependance, kAccType_ATOMICRW));
        ins->addCheckTrapEffect (mmuPageFaultCheck(ins));
        ins->addCommitEffect    (accessMem(ins));
        ins->addRetirementEffect(retireMem(ins));
        ins->addSquashEffect    (eraseLSQ(ins));
        ins->addPostvalidation  (validateMemory(kAddress, kResult1, sz, ins));

        if (a || r)
            ins->addRetirementConstraint(membarSyncConstraint(ins));
    }
};

class CAS: public InstrPatch {
public:
    CAS(): InstrPatch("ee0010001l1ssssso11111nnnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto size = map.at('e');
        auto l    = map.at('l');
        auto rs   = map.at('s');
        auto o    = map.at('o');
        auto rn   = map.at('n');
        auto rt   = map.at('t');

        ins->setClass(clsAtomic, codeRMW);
        ins->setExclusive();

        std::vector<std::list<InternalDependance>> rn_dep(1);
        std::vector<std::list<InternalDependance>> rs_dep(1);
        std::vector<std::list<InternalDependance>> rt_dep(1);

        addAddressCompute(ins, rn_dep);

        auto sz = (size == 0) ? kByte     :
                  (size == 1) ? kHalfWord :
                  (size == 2) ? kWord     :
                                kDoubleWord;

        auto op = kAMOCAS;
        auto se = kNoExtension;

        if (sz < kWord)
            op = (eRMWOperation)((int)(op) + size);

        auto ld  = amoAction(ins, sz, se, kPD);
        auto rmw = updateRMWValueAction(ins, kOperand2, kOperand3, kResult1, op);

        auto rrn = addReadXRegister(ins, 1, rn, rn_dep[0], true);
        auto rrs = addReadXRegister(ins, 2, rs, rs_dep[0], sz == kDoubleWord);
        auto rrt = addReadXRegister(ins, 3, rt, rt_dep[0], sz == kDoubleWord);

        addDestination(ins, rs, ld, se);

        connectDependance(rmw.action->dependance(0), ld);
        connectDependance(rmw.action->dependance(1), rrs);
        connectDependance(rmw.action->dependance(2), rrt);
        connectDependance(ins->retirementDependance(), rmw);

        ins->setMayCommit(false);

        ins->addDispatchEffect  (allocateRMW(ins, sz, ld.dependance, kAccType_ATOMICRW));
        ins->addCheckTrapEffect (mmuPageFaultCheck(ins));
        ins->addCommitEffect    (accessMem(ins));
        ins->addRetirementEffect(retireMem(ins));
        ins->addSquashEffect    (eraseLSQ(ins));
        ins->addPostvalidation  (validateMemory(kAddress, kResult1, sz, ins));

        if (l || o)
            ins->addRetirementConstraint(membarSyncConstraint(ins));
    }
};

void nDecoder::initPatch() {
    addPatch(new AMO());
}