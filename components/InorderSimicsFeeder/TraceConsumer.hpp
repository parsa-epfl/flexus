#include <core/types.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/stats.hpp>

namespace Flexus {
namespace SharedTypes {
using namespace nInorderSimicsFeeder;

struct ConsumerEntry {
  enum FeederStatus {
    Idle,        // no operation ready for fetch; no transaction in progress
    Ready,       // the operation is waiting to be executed
    InProgress,  // the transaction is in progress
    Complete     // the current operation has completed
  };

  //Current status
  FeederStatus theStatus;
  boost::intrusive_ptr<ArchitecturalInstruction> theReadyInstruction;

  ConsumerEntry()
    : theStatus(Idle)
  {}

  ConsumerEntry(boost::intrusive_ptr<ArchitecturalInstruction> & anInstruction)
    : theStatus(Ready)
    , theReadyInstruction(anInstruction)
  {}

  bool isIdle() const {
    return theStatus == Idle;
  }

  bool isReady() const {
    return theStatus == Ready;
  }

  bool isInProgress() const {
    return theStatus == InProgress;
  }

  bool isComplete() const {
    return theStatus == Complete;
  }

  void acknowledgeComplete() {
    DBG_Assert( (isComplete()) );
    theStatus = Idle;
  }

  void beginInstruction() {
    DBG_Assert( (isReady()) );
    theStatus = InProgress;
  }

  void releaseInstruction(ArchitecturalInstruction const & anInstruction) {
    DBG_Assert( (isInProgress()) );
    DBG_Assert( (&anInstruction == theReadyInstruction.get()) );
    theStatus = Complete;
  }

  void consumeInstruction(boost::intrusive_ptr<ArchitecturalInstruction> & anInstruction) {
    DBG_Assert( (isIdle()) );
    theStatus = Ready;
    theReadyInstruction = anInstruction;
  }

  //Provides a reference to the contained instruction object.
  //Allows faster access to the instruction in the SimicsTracer.
  ArchitecturalInstruction & optimizedGetInstruction() {
    return *theReadyInstruction;
  }

  //Note the conversion from boost::intrusive_ptr<ArchitecturalInstruction> to
  //boost::intrusive_ptr<Instruction>
  boost::intrusive_ptr<ArchitecturalInstruction> ready_instruction() {
    DBG_Assert( (isReady()) );
    return theReadyInstruction;
  }

};  // struct ConsumerEntry

inline std::ostream & operator<< (std::ostream & anOstream, ConsumerEntry & entry) {
  static const char * statusStr[] = {"Idle", "Ready", "InProgress", "Complete"};
  anOstream << statusStr[entry.theStatus] << "-" << *entry.theReadyInstruction;
  return anOstream;
};

struct SimicsTraceConsumer {
  // queue of outstanding operations

  std::deque<ConsumerEntry> theEntries;
  // has Simics been told to wait (if this is true, the next memory request
  // must be a repeat of the last one)
  bool simicsPending;

  uint64_t theInitialCycleCount;

  Flexus::Stat::StatCounter theInstructionCount;
#if FLEXUS_TARGET_IS(x86)
  Flexus::Stat::StatCounter theInstructionCount_x86;
#endif

  // info on the most recent atomic operation, for verification purposes
  PhysicalMemoryAddress theAtomicInstAddress;
  PhysicalMemoryAddress theAtomicDataAddress;
  bool atomicVerifyPending;

  uint32_t theIndex;

#ifdef FLEXUS_FEEDER_TRACE_DEBUGGER
  // trace info on the last several instructions
  SimicsTraceDebugger debugger;
#endif //FLEXUS_FEEDER_TRACE_DEBUGGER

  SimicsTraceConsumer(std::string const & aName)
    : simicsPending(false)
    , theInitialCycleCount(0)
    , theInstructionCount(aName + "-Instructions")
#if FLEXUS_TARGET_IS(x86)
    , theInstructionCount_x86(aName + "-Instructions x86")
#endif
    , atomicVerifyPending(false)
  {}

  void init(uint32_t anIndex) {
    theIndex = anIndex;
#ifdef FLEXUS_FEEDER_TRACE_DEBUGGER
    debugger.init(anIndex);
#endif //FLEXUS_FEEDER_TRACE_DEBUGGER
  }

  void setInitialCycleCount(uint64_t aCycleCount) {
    theInitialCycleCount = aCycleCount;
  }

  uint64_t initialCycleCount() {
    return theInitialCycleCount;
  }

  bool largeQueue() const {
    return (theEntries.size() > 1000) ;
  }

  uint32_t queueSize() const {
    return theEntries.size() ;
  }

  bool isIdle() const {
    return (!simicsPending);
  }

  bool isQuiesced() const {
    return isIdle() && theEntries.empty();
  }

  void simicsDone() {
    DBG_Assert( (simicsPending) );
    simicsPending = false;
  }

  bool isReady() const {
    if (theEntries.empty()) {
      return false;
    } else {
      return theEntries.front().isReady();
    }
  }

  bool isInProgress() const {
    if (theEntries.empty()) {
      return false;
    } else {
      return theEntries.front().isInProgress();
    }
  }

  bool isComplete() const {
    // We're done if all outstanding operations have been completed and
    // acknowledged (i.e. the instruction queue is empty) but Simics is
    // still expecting a response
    return (theEntries.empty() && simicsPending);
  }

  void releaseInstruction(ArchitecturalInstruction const & anInstruction) {
    DBG_Assert( (!theEntries.empty()) );
    theEntries.front().releaseInstruction(anInstruction);
    theEntries.pop_front();
  }

  void consumeInstOperation(boost::intrusive_ptr<ArchitecturalInstruction> & anInstruction) {
    // a new instruction operation always requires a new entry
    theEntries.push_back( ConsumerEntry(anInstruction) );
    clearAtomicPending();
  }

  bool isEmpty() {
    return theEntries.empty();
  }

  ArchitecturalInstruction & optimizedGetInstruction() {
    DBG_Assert( ! theEntries.empty() );
    return theEntries.back().optimizedGetInstruction();
  }

  void consumeDataOperation() {
    simicsPending = true;
  }

  void recordAtomicVerification( PhysicalMemoryAddress const & aPC, PhysicalMemoryAddress const & aDataAddr ) {
    DBG_Assert(!atomicVerifyPending);
    theAtomicInstAddress = aPC;
    theAtomicDataAddress = aDataAddr;
    atomicVerifyPending = true;
  }

  bool isAtomicOperationPending() {
    return atomicVerifyPending;
  }

  PhysicalMemoryAddress getAtomicPC() {
    return theAtomicInstAddress;
  }

  PhysicalMemoryAddress getAtomicDataAddr() {
    return theAtomicDataAddress;
  }

  void clearAtomicPending() {
    atomicVerifyPending = false;
  }

  void verifyAtomicOperation(bool anIsWrite, PhysicalMemoryAddress const & aPC, PhysicalMemoryAddress const & aDataAddr) {
    DBG_Assert(atomicVerifyPending);
    DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex)
         Condition(aPC != theAtomicInstAddress)
         ( << "atomic PC's don't match: saved="
           << theAtomicInstAddress << " new_op="
           << aPC
         ) );
    if (anIsWrite) {
      DBG_(VVerb, SetNumeric( (FlexusIdx) theIndex)
           Condition(aDataAddr != theAtomicDataAddress)
           ( << "atomic data addresses don't match: saved="
             << theAtomicDataAddress << " new_op="
             << aDataAddr
           ) );
      //DBG_Assert(aDataAddr == theAtomicDataAddress);
    }
    //DBG_Assert(aPC == theAtomicInstAddress);
    atomicVerifyPending = false;
  }

  boost::intrusive_ptr<ArchitecturalInstruction> ready_instruction() {
    DBG_Assert( (isReady()) );
    return theEntries.front().ready_instruction();
  }

  void begin_instruction() {
    DBG_Assert( (isReady()) );
    theEntries.front().beginInstruction();
    theInstructionCount++;

#if FLEXUS_TARGET_IS(x86)
    if (theEntries.front().optimizedGetInstruction().getIfPart() == Normal)
      //Don't count the second part of an instruction that performs two memory operations
      theInstructionCount_x86++;
#endif
  }

};  // struct SimicsTraceConsumer

inline std::ostream & operator<< (std::ostream & anOstream, SimicsTraceConsumer & cons) {
  std::deque<ConsumerEntry>::iterator iter;
  anOstream << "FeederMemOpQueue: ";
  for (iter = cons.theEntries.begin(); iter != cons.theEntries.end(); iter++) {
    anOstream << *iter << " ";
  }
  anOstream << std::endl;
  return anOstream;
};

} //SharedTypes
} //Flexus
