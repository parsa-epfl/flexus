
#include "Instruction.hpp"

#include "components/uArch/uArchInterfaces.hpp"
#include "encodings/Encodings.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

ArchInstruction::ArchInstruction(VirtualMemoryAddress aPC,
                                 Opcode anOpcode,
                                 boost::intrusive_ptr<BPredState> bp_state,
                                 uint32_t aCPU,
                                 int64_t aSequenceNo,
                                 eInstructionClass aClass,
                                 eInstructionCode aCode)
  : thePC(aPC)
  , thePCReg(aPC + 4)
  , theOpcode(anOpcode)
  , theBPState(bp_state)
  , theCPU(aCPU)
  , theSequenceNo(aSequenceNo)
  , theuArch(0)
  , theRaisedException(kException_None)
  , theResync(false)
  , theWillRaise(kException_None)
  , theAnnulled(false)
  , theRetired(false)
  , theSquashed(false)
  , theExecuted(true)
  , thePageFault(false)
  , theInstructionClass(aClass)
  , theInstructionCode(aCode)
  , theHaltDispatch(false)
  , theHasCheckpoint(false)
  , theRetireStallCycles(0)
  , theMayCommit(true)
  , theResolved(false)
  , theUsesIntAlu(true)
  , theUsesIntMult(false)
  , theUsesIntDiv(false)
  , theUsesFpAdd(false)
  , theUsesFpCmp(false)
  , theUsesFpCvt(false)
  , theUsesFpMult(false)
  , theUsesFpDiv(false)
  , theUsesFpSqrt(false)
  , theInsnSourceLevel(eL1I)
{
}

void
ArchInstruction::describe(std::ostream& anOstream) const
{
    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
    anOstream << "#" << std::dec << theSequenceNo << "[" << std::setfill('0') << std::right << std::setw(2) << cpu.id()
              << "] " << printInstClass() << " QEMU disas: " << cpu.disassemble(thePC);

    if (theAnnulled) { anOstream << " {annuled}"; }
    if (theRaisedException != kException_None) { anOstream << " {raised}"; }
    if (theResync) { anOstream << " {force-resync}"; }
    if (haltDispatch()) { anOstream << " {halt-dispatch}"; }
}

std::string
ArchInstruction::disassemble() const
{
    return Flexus::Qemu::Processor::getProcessor(theCPU).disassemble(thePC);
}

void
ArchInstruction::setWillRaise(eExceptionType aSetting)
{
    theWillRaise = aSetting;
}

void
ArchInstruction::doDispatchEffects()
{
    auto bp_state = bpState();

    DBG_Assert(bp_state, (<< "No branch predictor state exists, but it must"));
    if (bp_state->theActualType == kNonBranch) return;
    if (isBranch()) return;

    // Branch predictor identified an instruction that is not a branch as a branch.
    DBG_(VVerb, (<< *this << " predicted as a branch, but is a non-branch. Fixing"));

    boost::intrusive_ptr<BranchFeedback> feedback(new BranchFeedback());
    feedback->thePC              = pc();
    feedback->theActualType      = kNonBranch;
    feedback->theActualDirection = kNotTaken;
    feedback->theActualTarget    = VirtualMemoryAddress(0);
    feedback->theBPState         = bpState();
    core()->branchFeedback(feedback);
}

bool
ArchInstruction::usesIntAlu() const
{
    return theUsesIntAlu;
}

bool
ArchInstruction::usesIntMult() const
{
    return theUsesIntMult;
}

bool
ArchInstruction::usesIntDiv() const
{
    return theUsesIntDiv;
}

bool
ArchInstruction::usesFpAdd() const
{
    return theUsesFpAdd;
}

bool
ArchInstruction::usesFpCmp() const
{
    return theUsesFpCmp;
}

bool
ArchInstruction::usesFpCvt() const
{
    return theUsesFpCvt;
}

bool
ArchInstruction::usesFpMult() const
{
    return theUsesFpMult;
}

bool
ArchInstruction::usesFpDiv() const
{
    return theUsesFpDiv;
}

bool
ArchInstruction::usesFpSqrt() const
{
    return theUsesFpSqrt;
}

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop)
{
    DBG_(VVerb, (<< "\033[1;31m DECODER: Decoding " << std::hex << aFetchedOpcode.theOpcode << std::dec << "\033[0m"));

    bool last_uop                                     = true;
    boost::intrusive_ptr<AbstractInstruction> ret_val = disas_a64_insn(aFetchedOpcode, aCPU, aSequenceNo, aUop);
    return std::make_pair(ret_val, last_uop);
}

} // namespace nDecoder
