#define FLEXUS_BEGIN_COMPONENT TraceFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define TraceFeeder_IMPLEMENTATION (<components/TraceFeeder/TraceFeederImpl.hpp>)

#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif

namespace Flexus {
namespace SharedTypes {

// tag for extracting the arch inst part of an InstructionTransport
struct ArchitecturalInstructionTag_t {} ArchitecturalInstructionTag;

} // namespace SharedTypes
} // namespace Flexus

namespace nTraceFeeder {

using boost::counted_base;
using Flexus::SharedTypes::PhysicalMemoryAddress;

//Debug Setting for Instruction data type
typedef Flexus::Debug::Debug InstructionDebugSetting;

class TraceInstructionImpl;
struct TraceInstruction : public counted_base {
  FLEXUS_DECLARE_DEBUG(InstructionDebugSetting, "TraceInstruction" );

  //Need to create a register type once this is useful
  typedef int32_t RegisterName;
  typedef int32_t * RegisterName_iterator;

  bool isMemory() const;
  bool isLoad() const;
  bool isStore() const;
  bool isRmw() const;
  bool isSync() const;
  bool isMEMBAR() const;
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
  bool isReleased() const;
  bool wasTaken() const;
  bool wasNotTaken() const;

  bool canExecute() const;
  bool canPerform() const;
  bool canRelease() const;

protected:
  TraceInstruction(TraceInstructionImpl * anImpl): theImpl(anImpl) {}
private:
  //Note: raw ptr as an Instruction does not have any ownership
  //over its impl.  In fact, the reverse is true - the Instruction is
  //a sub-object of the Impl.
  TraceInstructionImpl * theImpl;
};

std::ostream & operator << (std::ostream & anOstream, TraceInstruction const & anInstruction);

} //End nTraceFeeder
namespace Flexus {
namespace SharedTypes {

#define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
typedef nTraceFeeder::TraceInstruction ArchitecturalInstruction;

} //End SharedTypes
} //End Flexus

#ifdef FLEXUS_FeederCommand_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FeederCommand data type"
#endif

namespace nTraceFeeder {

struct TraceCommand {
  typedef Flexus::Core::FlexusException FeederException;
  static TraceCommand OracleNextInstruction() {
    return TraceCommand();
  }
  static TraceCommand TakeException(boost::intrusive_ptr<Flexus::SharedTypes::ArchitecturalInstruction>) {
    return TraceCommand();
  }
  TraceCommand const & command() const {
    return *this;
  }
  //Default copy, assignment, and destructor
};

} //End nTraceFeeder
namespace Flexus {
namespace SharedTypes {

#define FLEXUS_FeederCommand_TYPE_PROVIDED
typedef nTraceFeeder::TraceCommand FeederCommand;

} //End SharedTypes
} //End Flexus

namespace nTraceFeeder {
typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_InstructionTransport_Typemap,
std::pair <
Flexus::SharedTypes::ArchitecturalInstructionTag_t
, Flexus::SharedTypes::ArchitecturalInstruction
>
>::type InstructionTransport_Typemap;

#undef FLEXUS_PREVIOUS_InstructionTransport_Typemap
#define FLEXUS_PREVIOUS_InstructionTransport_Typemap nTraceFeeder::InstructionTransport_Typemap
} //End nTraceFeeder

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TraceFeeder

