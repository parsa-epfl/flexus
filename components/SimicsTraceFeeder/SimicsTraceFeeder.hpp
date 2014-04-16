#define FLEXUS_BEGIN_COMPONENT SimicsTraceFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define SimicsTraceFeeder_IMPLEMENTATION (<components/SimicsTraceFeeder/SimicsTraceFeederImpl.hpp>)

#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif

namespace Flexus {
namespace SimicsTraceFeeder {

// Debug Setting for Instruction data type
typedef Flexus::Debug::Debug InstructionDebugSetting;

using boost::counted_base;
using SharedTypes::PhysicalMemoryAddress;
using namespace Flexus::MemoryPool;

class SimicsX86MemoryOpImpl;
struct SimicsX86MemoryOp : public counted_base {
  FLEXUS_DECLARE_DEBUG(InstructionDebugSetting, "SimicsX86MemoryOp" );

  // x86 is 32-bit, so logical and physical addresses are 32-bit
  typedef int32_t RegisterName;
  typedef int32_t * RegisterName_iterator;

  // data word is also 32-bit
  typedef uint32_t data_word;

  enum eOpType {
    Read,
    Write
  };

  bool isMemory() const;
  bool isLoad() const;
  bool isStore() const;
  bool isBranch() const;
  bool isInterrupt() const;
  bool requiresSync() const;
  RegisterName_iterator inputRegisters_begin() const;
  RegisterName_iterator inputRegisters_end() const;
  RegisterName_iterator outputRegisters_begin() const;
  RegisterName_iterator outputRegisters_end() const;
  PhysicalMemoryAddress physicalMemoryAddress() const;
  void execute();
  void perform();
  void commit();
  void squash();
  void release();
  bool isValid() const;
  bool isFetched() const;
  bool isDecoded() const;
  bool isExecuted() const;
  bool isPerformed() const;
  bool isCommitted() const;
  bool isSquashed() const;
  bool isExcepted() const;
  bool wasTaken() const;
  bool wasNotTaken() const;

  bool canExecute() const;
  bool canPerform() const;

  virtual ~SimicsX86MemoryOp() {}
protected:
  SimicsX86MemoryOp(SimicsX86MemoryOpImpl * anImpl): theImpl(anImpl) {}
private:
  friend class SimicsX86MemoryOpImpl;
  template <class , class> friend class SimicsTraceFeederComponent;

  SimicsX86MemoryOpImpl * getImpl();
  // Note: raw ptr as a MemoryOp does not have any ownership
  // over its impl.  In fact, the reverse is true - the MemoryOp is
  // a sub-object of the Impl.
  SimicsX86MemoryOpImpl * theImpl;
};  // end struct SimicsX86MemoryOp

} // end namespace SimicsTraceFeeder

namespace SharedTypes {

#define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
typedef SimicsTraceFeeder::SimicsX86MemoryOp ArchitecturalInstruction;

} // end namespace SharedTypes
} // end namespace Flexus

namespace Flexus {
namespace SimicsTraceFeeder {
typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_InstructionTransport_Typemap,
std::pair <
Flexus::SharedTypes::ArchitecturalInstructionTag_t
,   Flexus::SharedTypes::ArchitecturalInstruction
>
>::type InstructionTransport_Typemap;

#undef FLEXUS_PREVIOUS_InstructionTransport_Typemap
#define FLEXUS_PREVIOUS_InstructionTransport_Typemap Flexus::SimicsTraceFeeder::InstructionTransport_Typemap
} // end namespace SimicsTraceFeeder
} // end namespace Flexus

#ifdef FLEXUS_FeederCommand_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FeederCommand data type"
#endif

namespace Flexus {
namespace SimicsTraceFeeder {

using boost::intrusive_ptr;

class Command {
public:
  enum eCommands {
    Oracle_NextInstruction  // for compatibility with BWFetch component
  };
  typedef Core::FlexusException FeederException;
public:
  Command() {}
  static Command OracleNextInstruction() {
    return Command();
  }
  static Command TakeException(boost::intrusive_ptr<Flexus::SharedTypes::ArchitecturalInstruction>) {
    return Command();
  }
  friend std::ostream & operator << (std::ostream &, Command const &);

  // Default copy, assignment, and destructor
  eCommands getCommand() const;
};  // end class Command

} // end namespace SimicsTraceFeeder

namespace SharedTypes {

#define FLEXUS_FeederCommand_TYPE_PROVIDED
typedef SimicsTraceFeeder::Command FeederCommand;

} // end namespace SharedTypes
} // end namespace Flexus

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SimicsTraceFeeder
