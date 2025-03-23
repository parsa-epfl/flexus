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
        auto rmw = updateRMWValueAction(ins, kOperand3, kOperand2, kResult1, op);

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

static uint64_t signext(uint64_t val, uint32_t sz) {
    if (val & (1ul << (sz - 1)))
        val |= ~((1ul << sz) - 1);

    return val;
}

static void addImm(SemanticInstruction *ins, uint32_t rn, uint64_t val) {
    ins->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rn_dep(2);

    auto add = addExecute(ins, operation(kADD_), rn_dep);

    addReadXRegister(ins, 1, rn, rn_dep[0], true);
    addReadConstant (ins, 2, val, rn_dep[1]);

    addDestination(ins, rn, add, true);
}

static void addReg(SemanticInstruction *ins, uint32_t rn, uint32_t rm) {
    ins->setClass(clsComputation, codeALU);

    std::vector<std::list<InternalDependance>> rn_dep(2);

    auto add = addExecute(ins, operation(kADD_), rn_dep);

    addReadXRegister(ins, 1, rn, rn_dep[0], true);
    addReadXRegister(ins, 2, rm, rn_dep[1], true);

    addDestination(ins, rn, add, true);
}

class LSR: public InstrPatch {
public:
    LSR(): InstrPatch("ee111100cc0iiiiiiiiix1nnnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto imm9 = map.at('i');
        auto rn   = map.at('n');

        addImm(ins, rn, signext(imm9, 9));
    }
};

class LSP: public InstrPatch {
public:
    LSP(): InstrPatch("cc10110x1liiiiiii22222nnnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto opc  = map.at('c');
        auto imm7 = map.at('i');
        auto rn   = map.at('n');

        addImm(ins, rn, signext(imm7, 7) << (opc + 2));
    }
};

class LSM: public InstrPatch {
public:
    LSM(): InstrPatch("0q0011001l0mmmmmooooeennnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto rm = map.at('m');
        auto rn = map.at('n');

        if (rm == 31) {
            auto q      = map.at('q');
            auto opcode = map.at('o');
            auto imm    = 0u;

            switch (opcode) {
                case 0x0: // ld/st4
                case 0x2: // ld/st1, 4 reg
                    imm = q ? 64 : 32;
                    break;
                case 0x4: // ld/st3
                case 0x6: // ld/st1, 3 reg
                    imm = q ? 48 : 24;
                    break;
                case 0x7: // ld/st1, 1 reg
                    imm = q ? 16 :  8;
                    break;
                case 0x8: // ld/st2
                case 0xa: // ld/st1, 2 reg
                    imm = q ? 32 : 16;
                    break;
            }

            addImm(ins, rn, imm);

        } else
            addReg(ins, rn, rm);
    }
};

class LSS: public InstrPatch {
public:
    LSS(): InstrPatch("0q0011011lrmmmmmoooseennnnnttttt") {
    }

    virtual void dec(SemanticInstruction *ins, const map &map) {
        auto rm = map.at('m');
        auto rn = map.at('n');

        if (rm == 31) {
            auto r      = map.at('r');
            auto opcode = map.at('o');
            auto size   = map.at('e');
            auto imm    = 0u;

            if (r == 0)
                switch (opcode) {
                    case 0x0: // st1, 8b
                        imm = 1;
                        break;
                    case 0x1: // st3, 8b
                        imm = 3;
                        break;
                    case 0x2: // st1, 16b
                        imm = 2;
                        break;
                    case 0x3: // st3, 16b
                        imm = 6;
                        break;
                    case 0x4:
                        imm = size ? 8  :  4; // st1, 64/32b
                        break;
                    case 0x5:
                        imm = size ? 24 : 12; // st3, 64/32b
                        break;
                    case 0x6: // ld1r
                        imm = 1u << size;
                        break;
                    case 0x7: // ld3r
                        imm = 3u << size;
                        break;
                }
            else
                switch (opcode) {
                    case 0x0: // st2, 8b
                        imm = 2;
                        break;
                    case 0x1: // st4, 8b
                        imm = 4;
                        break;
                    case 0x2: // st2, 16b
                        imm = 4;
                        break;
                    case 0x3: // st4, 16b
                        imm = 8;
                        break;
                    case 0x4:
                        imm = size ? 16 :  8; // st2, 64/32b
                        break;
                    case 0x5:
                        imm = size ? 32 : 16; // st4, 64/32b
                        break;
                    case 0x6: // ld2r
                        imm = 2u << size;
                        break;
                    case 0x7: // ld4r
                        imm = 4u << size;
                        break;
                }

            addImm(ins, rn, imm);

        } else
            addReg(ins, rn, rm);
    }
};

void nDecoder::initPatch() {
    addPatch(new AMO());
    addPatch(new CAS());
    addPatch(new LSR());
    addPatch(new LSP());
    addPatch(new LSM());
    addPatch(new LSS());
}