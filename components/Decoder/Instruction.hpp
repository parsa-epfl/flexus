
#ifndef FLEXUS_DECODER_INSTRUCTION_HPP_INCLUDED
#define FLEXUS_DECODER_INSTRUCTION_HPP_INCLUDED

#include <components/uArch/CoreModel.hpp>
#include <list>

namespace nDecoder {

using Flexus::SharedTypes::Opcode;
using Flexus::SharedTypes::VirtualMemoryAddress;

using namespace nuArch;

std::pair<boost::intrusive_ptr<AbstractInstruction>, bool>
decode(Flexus::SharedTypes::FetchedOpcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, int32_t aUop);

class ArchInstruction : public nuArch::Instruction
{
  protected:
    VirtualMemoryAddress thePC;
    std::vector<uint32_t> theInstruction;
    VirtualMemoryAddress thePCReg;
    Opcode theOpcode;
    boost::intrusive_ptr<BPredState> theBPState;
    uint32_t theCPU;
    int64_t theSequenceNo;
    uArch* theuArch;
    eExceptionType theRaisedException;
    bool theResync;
    eExceptionType theWillRaise;
    bool theAnnulled;
    bool theRetired;
    bool theSquashed;
    bool theExecuted;
    bool thePageFault;
    eInstructionClass theInstructionClass;
    eInstructionCode theInstructionCode;
    eInstructionCode theOriginalInstructionCode;
    boost::intrusive_ptr<TransactionTracker> theTransaction;
    boost::intrusive_ptr<TransactionTracker> thePrefetchTransaction;
    boost::intrusive_ptr<nuArch::Instruction> thePredecessor;
    //  int32_t theASI;
    bool theHaltDispatch;
    bool theHasCheckpoint;
    uint64_t theRetireStallCycles;
    bool theMayCommit;
    bool theResolved;
    //  boost::optional<Flexus::Qemu::MMU::mmu_t> theMMU;

    bool theUsesIntAlu;
    bool theUsesIntMult;
    bool theUsesIntDiv;
    bool theUsesFpAdd;
    bool theUsesFpCmp;
    bool theUsesFpCvt;
    bool theUsesFpMult;
    bool theUsesFpDiv;
    bool theUsesFpSqrt;
    tFillLevel theInsnSourceLevel;

  public:
    virtual bool usesIntAlu() const;
    virtual bool usesIntMult() const;
    virtual bool usesIntDiv() const;
    virtual bool usesFpAdd() const;
    virtual bool usesFpCmp() const;
    virtual bool usesFpCvt() const;
    virtual bool usesFpMult() const;
    virtual bool usesFpDiv() const;
    virtual bool usesFpSqrt() const;

    virtual void setCanRetireCounter(const uint32_t numCycles) {}
    virtual void decrementCanRetireCounter() {}

    virtual void connectuArch(uArch& auArch) { theuArch = &auArch; };

    virtual void doDispatchEffects();
    virtual void squash() {}
    virtual void pageFault(bool p = true) { thePageFault = p; }
    virtual bool isPageFault() const { return thePageFault; }
    virtual void doRescheduleEffects() {}
    virtual void doRetirementEffects() {}
    virtual void checkTraps() {}
    virtual void doCommitEffects() {}
    virtual void annul() { theAnnulled = true; }
    virtual void reinstate() { theAnnulled = false; }

    virtual bool preValidate() { return true; }
    virtual bool advancesSimics() const { return true; }
    virtual bool postValidate() { return true; }

    virtual bool mayRetire() const { return false; }
    virtual void resolveSpeculation() { theMayCommit = true; }
    virtual void setMayCommit(bool aMayCommit) { theMayCommit = false; }
    virtual bool mayCommit() const { return theMayCommit; }

    virtual bool isResolved() const { return theResolved; }

    virtual void setResolved(bool value = true)
    {
        if (value) DBG_Assert(!theResolved, (<< *this));
        theResolved = value;
    }

    virtual eExceptionType willRaise() const { return theWillRaise; }
    virtual void setWillRaise(eExceptionType aSetting);

    virtual eExceptionType raised() { return theRaisedException; }
    virtual void raise(eExceptionType anException) { theRaisedException = anException; }
    virtual bool resync() const { return theResync; }
    virtual void forceResync(bool r = true) { theResync = r; }

    virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction)
    {
        theTransaction = aTransaction;
    }
    virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const { return theTransaction; }
    virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction)
    {
        thePrefetchTransaction = aTransaction;
    }
    virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const { return thePrefetchTransaction; }

    virtual bool willOverrideSimics() const { return false; }

    virtual void describe(std::ostream& anOstream) const;
    virtual std::string disassemble() const;
    virtual void overrideSimics() {}
    virtual int64_t sequenceNo() const { return theSequenceNo; }
    virtual bool isAnnulled() const { return theAnnulled; }
    bool isSquashed() const { return theSquashed; }
    bool isRetired() const { return theRetired; }
    virtual bool isComplete() const { return isRetired() || isSquashed(); }

    virtual void setUsesFpAdd()
    {
        theUsesFpAdd  = true;
        theUsesIntAlu = false;
    }

    virtual void setUsesFpCmp()
    {
        theUsesFpCmp  = true;
        theUsesIntAlu = false;
    }

    virtual void setUsesFpCvt()
    {
        theUsesFpCvt  = true;
        theUsesIntAlu = false;
    }

    virtual void setUsesFpMult()
    {
        theUsesFpMult = true;
        theUsesIntAlu = false;
    }

    virtual void setUsesFpDiv()
    {
        theUsesFpDiv  = true;
        theUsesIntAlu = false;
    }

    virtual void setUsesFpSqrt()
    {
        theUsesFpSqrt = true;
        theUsesIntAlu = false;
    }

    std::string printInstClass() const
    {
        switch (theInstructionClass) {
            case clsLoad: return " {clsLoad} ";
            case clsStore: return " {clsStore} ";
            case clsAtomic: return " {clsAtomic} ";
            case clsBranch: return " {clsBranch} ";
            case clsMEMBAR: return " {clsMEMBAR} ";
            case clsComputation: return " {clsComputation} ";
            case clsSynchronizing: return "{clsSynchronizing}";
            default: return "";
        }
    }

    virtual eInstructionClass instClass() const { return theInstructionClass; }

    virtual std::string instClassName() const
    {
        switch (theInstructionClass) {
            case clsLoad: return "clsLoad";

            case clsStore: return "clsStore";

            case clsAtomic: return "clsAtomic";

            case clsBranch: return "clsBranch";

            case clsMEMBAR: return "clsMEMBAR";

            case clsComputation: return "clsComputation";

            case clsSynchronizing: return "clsSynchronizing";

            default: assert(false); break;
        }
        __builtin_unreachable();
    }

    virtual eInstructionCode instCode() const { return theInstructionCode; }
    virtual eInstructionCode originalInstCode() const { return theOriginalInstructionCode; }

    virtual void restoreOriginalInstCode()
    {
        DBG_(VVerb,
             (<< "Restoring instruction code from " << theInstructionCode << " to " << theOriginalInstructionCode
              << ": " << *this));
        theInstructionCode = theOriginalInstructionCode;
    }

    virtual void changeInstCode(eInstructionCode aCode) { theInstructionCode = aCode; }

    void setClass(eInstructionClass anInstructionClass, eInstructionCode aCode)
    {
        theInstructionClass        = anInstructionClass;
        theOriginalInstructionCode = theInstructionCode = aCode;
        DECODER_DBG(*this);
    }

    uint32_t cpu() { return theCPU; }
    virtual bool isMicroOp() const { return false; }

    std::string identify() const
    {
        std::stringstream id;
        id << "CPU[" << std::setfill('0') << std::setw(2) << theCPU << "]#" << theSequenceNo;
        return id.str();
    }
    virtual ~ArchInstruction() { DBG_(VVerb, (<< identify() << " destroyed")); }

    virtual void redirectPC(VirtualMemoryAddress anPCReg) {
        thePCReg = anPCReg;
        DBG_Assert(this->bpState()->theActualTarget == anPCReg, (<< "Redirecting PC to " << anPCReg << " but BPState says " << this->bpState()->theActualTarget));
    }

    virtual VirtualMemoryAddress pc() const { return thePC; }

    virtual VirtualMemoryAddress pcNext() const { return thePCReg; }

    virtual bool isTrap() const { return theRaisedException != kException_None; }
    virtual boost::intrusive_ptr<BPredState> bpState() const { return theBPState; }
    bool isBranch() const { return theInstructionClass == clsBranch; }
    virtual void setAccessAddress(PhysicalMemoryAddress anAddress) {}
    virtual PhysicalMemoryAddress getAccessAddress() const { return PhysicalMemoryAddress(0); }

    virtual void setPreceedingInstruction(boost::intrusive_ptr<Instruction> aPredecessor)
    {
        thePredecessor = aPredecessor;
    }
    virtual bool hasExecuted() const { return theExecuted; }
    void setExecuted(bool aVal) { theExecuted = aVal; }
    bool hasPredecessorExecuted()
    {
        if (thePredecessor) {
            return thePredecessor->hasExecuted();
        } else {
            return true;
        }
    }

    uArch* core() { return theuArch; }

    bool haltDispatch() const { return theHaltDispatch; }
    void setHaltDispatch() { theHaltDispatch = true; }

    bool hasCheckpoint() const { return theHasCheckpoint; }
    void setHasCheckpoint(bool aCkpt) { theHasCheckpoint = aCkpt; }

    void setRetireStallCycles(uint64_t aDelay) { theRetireStallCycles = aDelay; }
    uint64_t retireStallCycles() const { return theRetireStallCycles; }

    void setSourceLevel(tFillLevel aLevel) { theInsnSourceLevel = aLevel; }
    tFillLevel sourceLevel() const { return theInsnSourceLevel; }

  protected:
    ArchInstruction(VirtualMemoryAddress aPC,
                    Opcode anOpcode,
                    boost::intrusive_ptr<BPredState> bp_state,
                    uint32_t aCPU,
                    int64_t aSequenceNo)
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
      , theInstructionClass(clsSynchronizing)
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

    /* Added constructor with explicit instruction class/code to simplify/improve accounting */
    ArchInstruction(VirtualMemoryAddress aPC,
                    Opcode anOpcode,
                    boost::intrusive_ptr<BPredState> bp_state,
                    uint32_t aCPU,
                    int64_t aSequenceNo,
                    eInstructionClass aClass,
                    eInstructionCode aCode);

    // So that Decoder can send opcodes out to PowerTracker
  public:
    Opcode getOpcode() { return theOpcode; }
};

typedef boost::intrusive_ptr<Instruction> archinst;
typedef Flexus::SharedTypes::FetchedOpcode archcode;

} // namespace nDecoder

#endif // FLEXUS_DECODER_INSTRUCTION_HPP_INCLUDED
