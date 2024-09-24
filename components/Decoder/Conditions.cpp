
#include "Conditions.hpp"

#include "OperandMap.hpp"
#include "SemanticActions.hpp"
#include "components/uArch/CoreModel/PSTATE.hpp"
#include "components/uArch/CoreModel/SCTLR_EL.hpp"
#include "components/uArch/RegisterType.hpp"
#include "components/uArch/uArchInterfaces.hpp"
#include "core/debug/debug.hpp"
#include "core/target.hpp"
#include "core/types.hpp"

#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {
using namespace nuArch;

bool
ConditionHolds(const PSTATE& pstate, int condcode)
{

    bool result = false;
    switch (condcode >> 1) {
        case 0: // EQ or NE
            result = pstate.Z();
            break;
        case 1: // CS or CC
            result = pstate.C();
            break;
        case 2: // MI or PL
            result = pstate.N();
            break;
        case 3: // VS or VC
            result = pstate.V();
            break;
        case 4: // HI or LS
            result = (pstate.C() && pstate.Z() == 0);
            break;
        case 5: // GE or LT
            result = (pstate.N() == pstate.V());
            break;
        case 6: // GT or LE
            result = ((pstate.N() == pstate.V()) && (pstate.Z() == 0));
            break;
        case 7: // AL
            result = true;
            break;
        default: DBG_Assert(false); break;
    }

    // Condition flag values in the set '111x' indicate always true
    // Otherwise, invert condition if necessary.

    if (((condcode & 1) == 1) && (condcode != 0xf /*1111*/)) return !result;

    return result;
}

typedef struct CMPBR : public Condition
{
    CMPBR() {}
    virtual ~CMPBR() {}
    virtual bool operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return boost::get<uint64_t>(operands[0]) == 0;
    }
    virtual char const* describe() const { return "Compare and Branch on Zero"; }
} CMPBR;

typedef struct CBNZ : public Condition
{
    CBNZ() {}
    virtual ~CBNZ() {}
    virtual bool operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 1);
        return boost::get<uint64_t>(operands[0]) != 0;
    }
    virtual char const* describe() const { return "Compare and Branch on Non Zero"; }
} CBNZ;

typedef struct TBZ : public Condition
{
    TBZ() {}
    virtual ~TBZ() {}
    virtual bool operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1])) == 0;
    }
    virtual char const* describe() const { return "Test and Branch on Zero"; }
} TBZ;

typedef struct TBNZ : public Condition
{
    TBNZ() {}
    virtual ~TBNZ() {}
    virtual bool operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);
        return (boost::get<uint64_t>(operands[0]) & boost::get<uint64_t>(operands[1])) != 0;
    }
    virtual char const* describe() const { return "Test and Branch on Non Zero"; }
} TBNZ;

typedef struct BCOND : public Condition
{
    BCOND() {}
    virtual ~BCOND() {}
    virtual bool operator()(std::vector<Operand> const& operands)
    {
        DBG_Assert(operands.size() == 2);

        PSTATE p      = boost::get<uint64_t>(operands[0]);
        uint64_t test = boost::get<uint64_t>(operands[1]);

        // hacking  --- update pstate
        //    uint32_t pstate =
        //    Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPSTATE();

        //    DBG_(Iface, (<< "Flexus pstate = " <<
        //    theInstruction->core()->getPSTATE())); DBG_(Iface, (<< "qemu pstate = "
        //    << pstate));

        return ConditionHolds(p, test);
    }
    virtual char const* describe() const { return "Branch Conditionally"; }
} BCOND;

std::unique_ptr<Condition>
condition(eCondCode aCond)
{

    std::unique_ptr<Condition> ptr;
    switch (aCond) {
        case kCBZ_:
            DBG_(Iface, (<< "CBZ"));
            ptr.reset(new CMPBR());
            break;
        case kCBNZ_:
            DBG_(Iface, (<< "CBNZ"));
            ptr.reset(new CBNZ());
            break;
        case kTBZ_:
            DBG_(Iface, (<< "kTBZ_"));
            ptr.reset(new TBZ());
            break;
        case kTBNZ_:
            DBG_(Iface, (<< "TBNZ"));
            ptr.reset(new TBNZ());
            break;
        case kBCOND_:
            DBG_(Iface, (<< "BCOND"));
            ptr.reset(new BCOND());
            break;
        default: DBG_Assert(false); return nullptr;
    }
    return ptr;
}

} // namespace narmDecoder
