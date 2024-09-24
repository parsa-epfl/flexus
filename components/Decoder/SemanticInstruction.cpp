
#include "SemanticInstruction.hpp"

#include "SemanticActions.hpp"
#include "Validations.hpp"

#include <boost/bind/bind.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <core/performance/profile.hpp>
#include <list>

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

uint32_t theInsnCount;
Flexus::Stat::StatCounter theICBs("sys-ICBs");
Flexus::Stat::StatMax thePeakInsns("sys-PeakSemanticInsns");

std::set<SemanticInstruction*> theGlobalLiveInsns;
int64_t theLastPrintCount = 0;

void
SemanticInstruction::constructorTrackLiveInsns()
{
    if (theLastPrintCount >= 10000) {
        DBG_(Dev, (<< "Live Insn Count: " << theInsnCount));
        theLastPrintCount = 0;
        if (theInsnCount > 10000) {
            DBG_(Dev, (<< "Identifying oldest live instruction."));
            DBG_Assert(!theGlobalLiveInsns.empty());
            SemanticInstruction* oldest                   = *theGlobalLiveInsns.begin();
            std::set<SemanticInstruction*>::iterator iter = theGlobalLiveInsns.begin();
            while (iter != theGlobalLiveInsns.end()) {
                if ((*iter)->sequenceNo() < oldest->sequenceNo()) { oldest = *iter; }
                ++iter;
            }
            DBG_(Dev, (<< "Oldest live insn: " << *oldest));
            DBG_(Dev, (<< "   Ref count: " << oldest->theRefCount));
            DBG_(Dev, (<< "Oldest live insn internals: " << std::internal << *oldest));
        }
    }
    ++theLastPrintCount;
    theGlobalLiveInsns.insert(this);
}

void
SemanticInstruction::constructorInitValidations()
{
    ++theInsnCount;
    thePeakInsns << theInsnCount;
    for (int32_t i = 0; i < 4; ++i) {
        theRetirementDepends[i] = true;
    }
    addPrevalidation(validatePC(this, true));
}

SemanticInstruction::SemanticInstruction(VirtualMemoryAddress aPC,
                                         Opcode anOpcode,
                                         boost::intrusive_ptr<BPredState> aBPState,
                                         uint32_t aCPU,
                                         int64_t aSequenceNo,
                                         eInstructionClass aClass,
                                         eInstructionCode aCode)
  : ArchInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo, aClass, aCode)
  , theOverrideSimics(false)
  , thePrevalidationsPassed(false)
  , theRetireDepCount(0)
  , theIsMicroOp(false)
  , theRetirementTarget(*this)
  , theCanRetireCounter(0)
{
    constructorInitValidations();
}

SemanticInstruction::SemanticInstruction(VirtualMemoryAddress aPC,
                                         Opcode anOpcode,
                                         boost::intrusive_ptr<BPredState> aBPState,
                                         uint32_t aCPU,
                                         int64_t aSequenceNo)
  : ArchInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo)
  , theOverrideSimics(false)
  , thePrevalidationsPassed(false)
  , theRetireDepCount(0)
  , theIsMicroOp(false)
  , theRetirementTarget(*this)
  , theCanRetireCounter(0)
{
    constructorInitValidations();
}

SemanticInstruction::~SemanticInstruction()
{
    --theInsnCount;
}

nuArch::InstructionDependance
SemanticInstruction::makeInstructionDependance(InternalDependance const& aDependance)
{
    DBG_Assert(reinterpret_cast<long>(aDependance.theTarget) != 0x1);
    nuArch::InstructionDependance ret_val;
    ret_val.instruction = boost::intrusive_ptr<nuArch::Instruction>(this);
    ret_val.satisfy     = boost::bind(&DependanceTarget::invokeSatisfy, aDependance.theTarget, aDependance.theArg);
    ret_val.squash      = boost::bind(&DependanceTarget::invokeSquash, aDependance.theTarget, aDependance.theArg);
    return ret_val;
}

void
SemanticInstruction::setMayRetire(int32_t aBit, bool aFlag)
{
    SEMANTICS_TRACE;
    DBG_Assert(aBit < theRetireDepCount,
               (<< "setMayRetire setting a bit that is out of range: " << aBit << "\n"
                << std::internal << *this));
    bool may_retire = mayRetire();
    SEMANTICS_DBG("aBit = " << aBit << ", aFlag = " << aFlag << ", " << *this);
    theRetirementDepends[aBit] = aFlag;
    if (mayRetire() && !may_retire) {
        DBG_(Iface, (<< identify() << " may retire"));
    } else if (!mayRetire() && may_retire) {
        DBG_(Iface, (<< identify() << " may not retire"));
    }
}

bool
SemanticInstruction::mayRetire() const
{
    FLEXUS_PROFILE();
    if (isPageFault()) return true;
    bool ok = theRetirementDepends[0] && theRetirementDepends[1] && theRetirementDepends[2] && theRetirementDepends[3];
    for (std::list<std::function<bool()>>::const_iterator iter = theRetirementConstraints.begin(),
                                                          end  = theRetirementConstraints.end();
         ok && (iter != end);
         ++iter) {
        ok &= (*iter)();
    }

    ok &= (theCanRetireCounter == 0);

    return ok; // May retire if no dependence bit is clear
}

void
SemanticInstruction::setCanRetireCounter(const uint32_t numCycles)
{
    theCanRetireCounter = numCycles;
}

void
SemanticInstruction::decrementCanRetireCounter()
{
    if (theCanRetireCounter > 0) { --theCanRetireCounter; }
}

InternalDependance
SemanticInstruction::retirementDependance()
{
    DBG_Assert(theRetireDepCount < 5);
    theRetirementDepends[theRetireDepCount] = false;
    return InternalDependance(&theRetirementTarget, theRetireDepCount++);
}

bool
SemanticInstruction::preValidate()
{
    FLEXUS_PROFILE();
    bool ok = true;
    while (ok && !thePreValidations.empty()) {
        ok &= thePreValidations.front()();
        thePreValidations.pop_front();
    }
    thePrevalidationsPassed = ok;
    return ok;
}

bool
SemanticInstruction::advancesSimics() const
{
    return (!theAnnulled && !theIsMicroOp);
}

void
SemanticInstruction::overrideSimics()
{
    theOverrideSimics = true;
}

bool
SemanticInstruction::postValidate()
{
    FLEXUS_PROFILE();
    if (theRaisedException != willRaise()) {
        DBG_(VVerb,
             (<< *this << " PostValidation failed: Exception mismatch flexus=" << std::hex << willRaise()
              << " simics=" << theRaisedException << std::dec));
        return false;
    }

    if (thePrevalidationsPassed && theOverrideSimics && !theOverrideFns.empty() &&
        (theRaisedException != kException_None) && !theAnnulled) {

        DBG_(VVerb, (<< *this << " overrideSimics flag set and override conditions passed."));
        while (!theOverrideFns.empty()) {
            theOverrideFns.front()();
            theOverrideFns.pop_front();
        }
    }
    bool ok = true;
    while (ok && !thePostValidations.empty()) {
        ok &= thePostValidations.front()();
        thePostValidations.pop_front();
    }
    return ok;
}

void
SemanticInstruction::addPrevalidation(std::function<bool()> aValidation)
{
    thePreValidations.push_back(aValidation);
}

void
SemanticInstruction::addPostvalidation(std::function<bool()> aValidation)
{
    thePostValidations.push_back(aValidation);
}

void
SemanticInstruction::addOverride(std::function<void()> anOverride)
{
    theOverrideFns.push_back(anOverride);
}

void
SemanticInstruction::addRetirementConstraint(std::function<bool()> aConstraint)
{
    theRetirementConstraints.push_back(aConstraint);
}

void
SemanticInstruction::annul()
{
    FLEXUS_PROFILE();
    if (!theAnnulled) {
        ArchInstruction::annul();
        theAnnulmentEffects.invoke(*this);
    }
}

void
SemanticInstruction::reinstate()
{
    FLEXUS_PROFILE();
    if (theAnnulled) {
        ArchInstruction::reinstate();
        theReinstatementEffects.invoke(*this);
    }
}

void
SemanticInstruction::doDispatchEffects()
{
    DISPATCH_DBG("START DISPATCHING ACTIONS");
    FLEXUS_PROFILE();
    ArchInstruction::doDispatchEffects();
    while (!theDispatchActions.empty()) {
        core()->create(theDispatchActions.front());
        theDispatchActions.pop_front();
    }
    DISPATCH_DBG("FINISH DISPATCHING ACTIONS");

    DISPATCH_DBG("START DISPATCHING EFFECTS");
    theDispatchEffects.invoke(*this);
    DISPATCH_DBG("FINISH DISPATCHING EFFECTS");
};

void
SemanticInstruction::doRetirementEffects()
{
    FLEXUS_PROFILE();
    theRetirementEffects.invoke(*this);
    theRetired = true;
    // Clear predecessor to avoid leaking instructions
    thePredecessor = 0;
};

void
SemanticInstruction::checkTraps()
{
    FLEXUS_PROFILE();
    theCheckTrapEffects.invoke(*this);
};

void
SemanticInstruction::doCommitEffects()
{
    FLEXUS_PROFILE();
    theCommitEffects.invoke(*this);
    // Clear predecessor to avoid leaking instructions
    thePredecessor = 0;
};

void
SemanticInstruction::pageFault()
{
    thePageFault = true;
}

bool
SemanticInstruction::isPageFault() const
{
    return thePageFault;
}

void
SemanticInstruction::squash()
{
    FLEXUS_PROFILE();
    DBG_(VVerb, (<< *this << " squashed"));
    theSquashed = true;
    theSquashEffects.invoke(*this);
    // Clear predecessor to avoid leaking instructions
    thePredecessor = 0;
};

void
SemanticInstruction::addDispatchEffect(Effect* anEffect)
{
    theDispatchEffects.append(anEffect);
}

void
SemanticInstruction::addDispatchAction(simple_action const& anAction)
{
    theDispatchActions.push_back(anAction.action);
}

void
SemanticInstruction::addRetirementEffect(Effect* anEffect)
{
    theRetirementEffects.append(anEffect);
}

void
SemanticInstruction::addCheckTrapEffect(Effect* anEffect)
{
    theCheckTrapEffects.append(anEffect);
}

void
SemanticInstruction::addCommitEffect(Effect* anEffect)
{
    theCommitEffects.append(anEffect);
}

void
SemanticInstruction::addSquashEffect(Effect* anEffect)
{
    theSquashEffects.append(anEffect);
}

void
SemanticInstruction::addAnnulmentEffect(Effect* anEffect)
{
    theAnnulmentEffects.append(anEffect);
}

void
SemanticInstruction::addReinstatementEffect(Effect* anEffect)
{
    theReinstatementEffects.append(anEffect);
}

void
SemanticInstruction::describe(std::ostream& anOstream) const
{
    bool internals = false;
    if (anOstream.flags() & std::ios_base::internal) {
        anOstream << std::left;
        internals = true;
    }
    ArchInstruction::describe(anOstream);
    if (theRetired) {
        anOstream << "\t{retired}";
    } else if (theSquashed) {
        anOstream << "\t{squashed}";
    } else if (mayRetire()) {
        anOstream << "\t{completed}";
    } else if (hasExecuted()) {
        anOstream << "\t{executed}";
    }
    if (theAnnulled) { anOstream << "\t{annulled}"; }
    if (theIsMicroOp) { anOstream << "\t{micro-op}"; }

    if (internals) {
        anOstream << std::endl;
        anOstream << "\tOperandMap:\n";
        theOperands.dump(anOstream);

        if (!theDispatchEffects.empty()) {
            anOstream << "\tDispatchEffects:\n";
            theDispatchEffects.describe(anOstream);
        }

        if (!theAnnulmentEffects.empty()) {
            anOstream << "\tAnnulmentEffects:\n";
            theAnnulmentEffects.describe(anOstream);
        }

        if (!theReinstatementEffects.empty()) {
            anOstream << "\tReinstatementEffects:\n";
            theReinstatementEffects.describe(anOstream);
        }

        if (!theSquashEffects.empty()) {
            anOstream << "\tSquashEffects:\n";
            theSquashEffects.describe(anOstream);
        }

        if (!theRetirementEffects.empty()) {
            anOstream << "\tRetirementEffects:\n";
            theRetirementEffects.describe(anOstream);
        }

        if (!theCommitEffects.empty()) {
            anOstream << "\tCommitEffects:\n";
            theCommitEffects.describe(anOstream);
        }
    }
}

eExceptionType
SemanticInstruction::retryTranslation()
{
    DBG_Assert(false);
    return eExceptionType(NULL);
}

PhysicalMemoryAddress
SemanticInstruction::translate()
{
    DBG_Assert(false);
    return PhysicalMemoryAddress(0);
}

size_t
SemanticInstruction::addNewComponent(UncountedComponent* aComponent)
{
    return theICB.addNewComponent(aComponent);
}

} // namespace nDecoder
