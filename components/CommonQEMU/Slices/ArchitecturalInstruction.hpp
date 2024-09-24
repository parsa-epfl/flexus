#ifndef FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED
#define FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED

#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif
#define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED

#include <components/CommonQEMU/DoubleWord.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

// Forward declare
// class SimicsTraceConsumer;
class StoreBuffer;

enum execType
{
    Normal,
    Second_MemPart,
    Hardware_Walk,
    Halt_instr
};

// InorderInstructionImpl - Implementation class for SPARC v9 memory operations
//==================================
class ArchitecturalInstruction : public boost::counted_base
{ /*, public FastAlloc*/
    enum eOpType
    {
        Nop,
        Read,
        Write,
        Rmw,
        Membar
    };

    // A memory operation needs to know its address, data, and operation type
    PhysicalMemoryAddress thePhysicalAddress;
    VirtualMemoryAddress theVirtualAddress;
    PhysicalMemoryAddress thePhysicalInstructionAddress; // i.e. the PC
    VirtualMemoryAddress theVirtualInstructionAddress;   // i.e. the PC
    DoubleWord theData;
    eOpType theOperation;
    // SimicsTraceConsumer * theConsumer;
    bool theReleased;
    bool thePerformed;
    bool theCommitted;
    bool theSync;
    bool thePriv;
    bool theShadow;
    bool theTrace; // a traced instruction
    bool theIsAtomic;
    uint64_t theStartTime;
    StoreBuffer* theStoreBuffer;
    uint32_t theOpcode;
    char theSize;
    char instructionSize;   // the size of the instruction...for x86 purposes
    execType hasIfetchPart; // for instructions that perform 2 memory operations,
                            // or hardware walks
    char theIO;

  private:
    ArchitecturalInstruction(ArchitecturalInstruction* anOriginal)
      : thePhysicalAddress(anOriginal->thePhysicalAddress)
      , theVirtualAddress(anOriginal->theVirtualAddress)
      , thePhysicalInstructionAddress(anOriginal->thePhysicalInstructionAddress)
      , theVirtualInstructionAddress(anOriginal->theVirtualInstructionAddress)
      , theData(anOriginal->theData)
      , theOperation(anOriginal->theOperation)
      , theReleased(false)
      , thePerformed(false)
      , theCommitted(false)
      , theSync(anOriginal->theSync)
      , thePriv(anOriginal->thePriv)
      , theShadow(true)
      , theTrace(anOriginal->theTrace)
      , theStartTime(0)
      , theStoreBuffer(anOriginal->theStoreBuffer)
      , theOpcode(anOriginal->theOpcode)
      , theSize(anOriginal->theSize)
      , instructionSize(anOriginal->instructionSize)
      , hasIfetchPart(Normal)
      , theIO(anOriginal->theIO)
    {
    }

  public:
    ArchitecturalInstruction()
      : thePhysicalAddress(0)
      , theVirtualAddress(0)
      , thePhysicalInstructionAddress(0)
      , theVirtualInstructionAddress(0)
      , theData(0)
      , theOperation(Nop)
      , theReleased(false)
      , thePerformed(false)
      , theCommitted(false)
      , theSync(false)
      , thePriv(false)
      , theShadow(false)
      , theTrace(false)
      , theStartTime(0)
      , theStoreBuffer(0)
      , theOpcode(0)
      , theSize(0)
      , instructionSize(0)
      , hasIfetchPart(Normal)
      , theIO(false)
    {
    }

    /*explicit ArchitecturalInstruction(SimicsTraceConsumer * aConsumer)
      : thePhysicalAddress(0)
      , theVirtualAddress(0)
      , thePhysicalInstructionAddress(0)
      , theVirtualInstructionAddress(0)
      , theData(0)
      , theOperation(Nop)
  //    , theConsumer(aConsumer)
      , theReleased(false)
      , thePerformed(false)
      , theCommitted(false)
      , theSync(false)
      , thePriv(false)
      , theShadow(false)
      , theTrace(false)
      , theIsAtomic(false)
      , theStartTime(0)
      , theStoreBuffer(0)
      , theOpcode(0)
      , theSize(0)
      , instructionSize(0)
      , hasIfetchPart(Normal)
      , theIO(false)
    {}*/

    boost::intrusive_ptr<ArchitecturalInstruction> createShadow()
    {
        return boost::intrusive_ptr<ArchitecturalInstruction>(new ArchitecturalInstruction(this));
    }

    // ArchitecturalInstruction Interface functions
    // Query for type of operation
    bool isNOP() const { return (theOperation == Nop); }
    bool isMemory() const
    {
        return (theOperation == Read) || (theOperation == Write) || (theOperation == Rmw) || (theOperation == Membar);
    }
    bool isLoad() const { return (theOperation == Read); }
    bool isStore() const { return (theOperation == Write); }
    bool isRmw() const { return (theOperation == Rmw); }
    bool isMEMBAR() const { return (theOperation == Membar); }
    bool isSync() const { return theSync; }
    bool isPriv() const { return thePriv; }

    bool isShadow() const { return theShadow; }

    bool isTrace() const { return theTrace; }

    uint64_t getStartTime() const { return theStartTime; }

    bool isCommitted() const { return theCommitted; }
    bool isIO() const { return theIO; }
    void setIO() { theIO = true; }

    void commit()
    {
        DBG_Assert((theCommitted == false));
        theCommitted = true;
        ;
    }

    bool isReleased() const { return theReleased; }

    bool canRelease() const
    {
        // TSO
        if (isSync()) {
            return isPerformed() && isCommitted() && !isReleased();
        } else {
            return isCommitted() && !isReleased();
        }
    }

    void release();

    bool isPerformed() const { return thePerformed; }

    DoubleWord const& data() const { return theData; }

    uint32_t size() const { return theSize; }

    uint32_t InstructionSize() const { return instructionSize; }

    uint32_t opcode() const { return theOpcode; }

    // Perform the instruction
    void perform();

    // InorderInstructionImpl Interface functions
    void setSync() { theSync = true; }
    void setPriv() { thePriv = true; }

    // Set operation types
    void setIsNop() { theOperation = Nop; }
    void setIsLoad() { theOperation = Read; }
    void setIsMEMBAR()
    {
        theOperation = Membar;
        setSync();
    }
    void setIsStore() { theOperation = Write; }
    void setIsRmw()
    {
        theOperation = Rmw;
        setSync();
    }

    void setShadow() { theShadow = true; }

    void setTrace() { theTrace = true; }

    void setStartTime(uint64_t start) { theStartTime = start; }

    void setSize(char aSize) { theSize = aSize; }

    void setInstructionSize(char const aSize) { instructionSize = aSize; }

    // Set the address for a memory operation
    void setAddress(PhysicalMemoryAddress const& addr) { thePhysicalAddress = addr; }

    // Set the virtual address for a memory operation
    void setVirtualAddress(VirtualMemoryAddress const& addr) { theVirtualAddress = addr; }

    void setIfPart(execType const hasifpart) { hasIfetchPart = hasifpart; }

    // Set the PC for the operation
    void setPhysInstAddress(PhysicalMemoryAddress const& addr) { thePhysicalInstructionAddress = addr; }
    void setVirtInstAddress(VirtualMemoryAddress const& addr) { theVirtualInstructionAddress = addr; }

    void setStoreBuffer(StoreBuffer* aStoreBuffer) { theStoreBuffer = aStoreBuffer; }

    void setData(DoubleWord const& aData) { theData = aData; }

    void setOpcode(uint32_t anOpcode) { theOpcode = anOpcode; }

    // Get the address for the instruction reference
    PhysicalMemoryAddress physicalInstructionAddress() const { return thePhysicalInstructionAddress; }

    VirtualMemoryAddress virtualInstructionAddress() const { return theVirtualInstructionAddress; }

    // Get the address for a memory operation
    PhysicalMemoryAddress physicalMemoryAddress() const { return thePhysicalAddress; }

    VirtualMemoryAddress virtualMemoryAddress() const { return theVirtualAddress; }

    execType getIfPart() const { return hasIfetchPart; }

    const char* opName() const
    {
        const char* opTypeStr[] = { "nop", "read", "write", "rmw", "membar" };
        return opTypeStr[theOperation];
    }

    // Forwarding functions from SimicsV9MemoryOp
    bool isBranch() const { return false; }
    bool isInterrupt() const { return false; }
    bool requiresSync() const { return false; }
    void execute()
    {
        if (!isMemory()) perform();
    }
    void squash() { DBG_Assert(false, (<< "squash not supported")); }
    bool isValid() const { return true; }
    bool isFetched() const { return true; }
    bool isDecoded() const { return true; }
    bool isExecuted() const { return true; }
    bool isSquashed() const
    {
        return false; // Squashing not supported
    }
    bool isExcepted() const
    {
        return false; // Exceptions not supported
    }
    bool wasTaken() const
    {
        return false; // Branches not supported
    }
    bool wasNotTaken() const
    {
        return false; // Branches not supported
    }
    bool canExecute() const
    {
        return true; // execute() always true
    }
    bool canPerform() const
    {
        return true; // execute() always true
    }

}; // End ArchitecturalInstruction

std::ostream&
operator<<(std::ostream& anOstream, const ArchitecturalInstruction& aMemOp);
bool
operator==(const ArchitecturalInstruction& a, const ArchitecturalInstruction& b);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED
