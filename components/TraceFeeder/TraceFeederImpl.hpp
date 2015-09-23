#include <memory>

#include <iostream>

#define FLEXUS_BEGIN_COMPONENT TraceFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories TraceFeeder, Feeder
#define DBG_SetDefaultOps AddCat(TraceFeeder | Feeder)
#include DBG_Control()

namespace nTraceFeeder {

using namespace Flexus::Core;
using namespace Flexus::Debug;
using namespace Flexus::SharedTypes;
using namespace Flexus::MemoryPool;

using boost::intrusive_ptr;
using Flexus::Debug::end;

struct TraceInstructionImpl : public TraceInstruction
    //, UseMemoryPool<TraceInstructionImpl, GlobalResizing<32> >
{
  typedef PhysicalMemoryAddress MemoryAddress;

  //enumerated op code type
  enum eOpCode { iNOP, iLOAD, iSTORE };

  enum eState { sFetched, sExecuted, sPerformed, sCommitted};

  //Only defined if eOpCode is iLOAD or iSTORE
  MemoryAddress theAddress;
  eOpCode theOpCode;
  eState theState;

  TraceInstructionImpl()
    : TraceInstruction(this)
    , theAddress(0)
    , theState(sFetched)
  {}

  //Query for type of operation
  int32_t isMemory() const {
    return (theOpCode != iNOP);
  }
  int32_t isLoad()   const {
    return (theOpCode == iLOAD);
  }
  int32_t isStore()  const {
    return (theOpCode == iSTORE);
  }

  void execute() {
    theState = sExecuted;
  }
  void perform() {
    theState = sPerformed;
  }
  void commit()  {
    theState = sCommitted;
  }

  bool isExecuted() const {
    return (theState != sFetched);
  }
  bool isPerformed() const {
    return (theState == sPerformed || theState == sCommitted);
  }
  bool isCommitted() const {
    return (theState == sCommitted);
  }
  bool canExecute() const {
    return (theState == sFetched);
  }
  bool canPerform() const {
    return (theState == sExecuted);
  }

  //Get the address for a memory operation
  MemoryAddress physicalMemoryAddress() const {
    return theAddress;
  }
};

inline bool TraceInstruction::isMemory() const {
  return theImpl->isMemory();
}
inline bool TraceInstruction::isLoad() const {
  return theImpl->isLoad();
}
inline bool TraceInstruction::isStore() const {
  return theImpl->isStore();
}
inline bool TraceInstruction::isRmw() const {
  return false;
}
inline bool TraceInstruction::isSync() const {
  return false;
}
inline bool TraceInstruction::isMEMBAR() const {
  return false;
}
inline bool TraceInstruction::isBranch() const {
  return false;
}
inline bool TraceInstruction::isInterrupt() const {
  return false;
}
inline bool TraceInstruction::requiresSync() const {
  return false;
}
inline PhysicalMemoryAddress TraceInstruction::physicalMemoryAddress() const {
  return theImpl->physicalMemoryAddress();
}
inline void TraceInstruction::execute() {
  theImpl->execute();
}
inline void TraceInstruction::perform() {
  theImpl->perform();
}
inline void TraceInstruction::commit() {
  theImpl->commit();
}
inline void TraceInstruction::squash() {}
inline void TraceInstruction::release() {}
inline bool TraceInstruction::isValid() const {
  return true;
}
inline bool TraceInstruction::isFetched() const {
  return true;
}
inline bool TraceInstruction::isDecoded() const {
  return true;
}
inline bool TraceInstruction::isExecuted() const {
  return theImpl->isExecuted();
}
inline bool TraceInstruction::isPerformed() const {
  return theImpl->isPerformed();
}
inline bool TraceInstruction::isCommitted() const {
  return theImpl->isCommitted();
}
inline bool TraceInstruction::isSquashed() const {
  return false;
}
inline bool TraceInstruction::isExcepted() const {
  return false;
}
inline bool TraceInstruction::isReleased() const {
  return true;
}
inline bool TraceInstruction::wasTaken() const {
  return false;
}
inline bool TraceInstruction::wasNotTaken() const {
  return false;
}
inline bool TraceInstruction::canExecute() const {
  return theImpl->canExecute();
}
inline bool TraceInstruction::canPerform() const {
  return theImpl->canPerform();
}
inline bool TraceInstruction::canRelease() const {
  return true;
}

inline std::ostream & operator <<(std::ostream & anOstream, const TraceInstruction & anInsn) {
  TraceInstructionImpl const & insn = static_cast<TraceInstructionImpl const &>(anInsn);
  switch (insn.theOpCode) {
    case TraceInstructionImpl::iNOP:
      anOstream << "NOP";
      break;
    case TraceInstructionImpl::iLOAD:
      anOstream << "Load " << insn.theAddress;
      break;
    case TraceInstructionImpl::iSTORE:
      anOstream << "Store " << insn.theAddress;
      break;
    default:
      //Something goes here
      break;
  }
  return anOstream;
}

typedef uint64_t native;

typedef struct traceStruct {
  uint32_t seg24pc;       // top 24 bit (out of 52 bit) virtual PC adderss
  uint32_t pc;            // bottom 32 bit virtual PC address
  uint32_t seg24ea;       // top 24 bit (out of 52 bit) virtual address of operand
  uint32_t ea;            // bottom 32 bit virtual address of operand
  uint32_t length;        // size of data operand in byte
  uint32_t image;         // real PPC instruction image
} traceSet;

template <class Cfg>
class TraceFeederComponent : public FlexusComponentBase<TraceFeederComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(TraceFeederComponent, Cfg);

public:
  TraceFeederComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , count(0)
  {}

  //Total number of instructions read
  int64_t count;

  ~TraceFeederComponent() {
    if (pipeIn) {
      pclose(pipeIn);
    }
    //std::cout << "Trace instructions read: " << count << std::endl;
  }

  //Initialization
  void initialize() {
    pipeIn = popen("gzip -d -c outtrace3.gz", "r");
    if (pipeIn == nullptr) {
      DBG_(Crit, ( << "Error: could not open trace file!" ) );
      exit(-1);
    }
  }

private:
  FILE * pipeIn;

  uint32_t byteReorder(uint32_t value) {
    uint32_t ret = (value >> 24) & 0x000000ff;
    ret |= (value >> 8) & 0x0000ff00;
    ret |= (value << 8) & 0x00ff0000;
    ret |= (value << 24) & 0xff000000;
    return ret;
  }

  intrusive_ptr<TraceInstructionImpl> next_instruction() {
    // To consturct 52-bit virtual address,
    // strip the high NIBBLE (not a byte) of EA/PC.
    //
    // ex)  52 bit virtual address = (seg24pc) | (pc: 27-0 bit)
    //                               (seg24ea) | (ea:
    //
    //     if (Seg24 = 0x654321, EA = 0xA2345678)
    //       52-bit virtual address = 0x6543212345678
    traceSet rec;
    native pc = 0;
    native ea = 0;
    intrusive_ptr<TraceInstructionImpl> new_instruction = new TraceInstructionImpl();
    //int32_t is_mem = 0;

    // search for a memory address
    while (!ea) {
      if (fread(&rec, sizeof(traceSet), 1, pipeIn) == 1) {
        count++;
        pc = byteReorder(rec.pc) & 0x0fffffff;
        pc |= (uint64_t)byteReorder(rec.seg24pc) << 28;
        ea = byteReorder(rec.ea) & 0x0fffffff;
        ea |= (uint64_t)byteReorder(rec.seg24ea) << 28;
      } else {
        DBG_(Crit, ( << "Error: could not read trace file!" ) );
        throw Flexus::Core::FlexusException();
      }
    }

    new_instruction->theAddress = (TraceInstructionImpl::MemoryAddress)ea;
    new_instruction->theOpCode = TraceInstructionImpl::iLOAD;
    // do the intelligent decode stuff here!

    return new_instruction;
  }

public:
  //Ports
  struct FeederCommandIn : public PushInputPort< FeederCommand >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aFeeder, FeederCommand aCommand) {
      //Feeder commands are currently ignored by the TraceFeeder.  It always returns the
      //next instruction in the trace.
    }

  };

  struct InstructionOutputPort : public PullOutputPort< InstructionTransport >, AlwaysAvailable {

    FLEXUS_WIRING_TEMPLATE
    static InstructionTransport pull(self & aFeeder) {
      InstructionTransport aTransport;
      aTransport.set(ArchitecturalInstructionTag, aFeeder.next_instruction());
      return aTransport;
    }
  };

  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(TraceFeederComponentConfiguration);

}  //End Namespace nTraceFeeder

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TraceFeeder

#define DBG_Reset
#include DBG_Control()
