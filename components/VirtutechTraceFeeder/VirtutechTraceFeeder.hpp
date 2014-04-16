#define FLEXUS_BEGIN_COMPONENT VirtutechTraceFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define VirtutechTraceFeeder_IMPLEMENTATION (<components/VirtutechTraceFeeder/VirtutechTraceFeederImpl.hpp>)

namespace nExecute {
typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct Instruction {
  MemoryAddress pc;
  bool priv;

  MemoryAddress physicalInstructionAddress() {
    return pc;
  }
  bool isPriv() {
    return priv;
  }
  uint32_t opcode() {
    return 0;
  }
};

struct ExecuteState : boost::counted_base {
  ExecuteState(MemoryAddress pc, bool priv) {
    theInst.pc = pc;
    theInst.priv = priv;
  }
  Instruction & instruction() {
    return theInst;
  }
  Instruction theInst;
};
} //namespace nExecute

namespace Flexus {
namespace SharedTypes {

struct ExecuteStateTag_t {} ExecuteStateTag;

typedef nExecute :: ExecuteState ExecuteState;

} //namespace SharedTypes
} //namespace Flexus

namespace nExecute {
typedef boost::mpl::push_front <
FLEXUS_PREVIOUS_MemoryTransport_Typemap,
std::pair <
Flexus::SharedTypes::ExecuteStateTag_t
,   Flexus::SharedTypes::ExecuteState
>
>::type MemoryTransport_Typemap;

#undef FLEXUS_PREVIOUS_MemoryTransport_Typemap
#define FLEXUS_PREVIOUS_MemoryTransport_Typemap nExecute::MemoryTransport_Typemap

} //namespace nExecute

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT VirtutechTraceFeeder
