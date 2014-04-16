#define FLEXUS_BEGIN_COMPONENT RandomFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#ifdef FLEXUS_FeederCommand_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FeederCommand data type"
#endif

namespace Flexus {
namespace RandomFeeder {

struct Command {

};

} //End SimicsFeeder
namespace SharedTypes {

#define FLEXUS_FeederCommand_TYPE_PROVIDED
typedef Flexus::RandomFeeder::Command FeederCommand;

} //End SharedTypes
} //End Flexus

#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif

namespace Flexus {
namespace RandomFeeder {

using boost::counted_base;
using namespace Flexus::Core;
using namespace Flexus::MemoryPool;

struct FakeInstruction : public counted_base, UseMemoryPool<FakeInstruction, LocalResizing > {

  //enumerated op code type
  enum eOpCode { iNOP, iLOAD, iSTORE };
  eOpCode theOpCode;

  //Only defined if eOpCode is iLOAD or iSTORE
  typedef int32_t address_type;
  address_type theAddress;

};

} //End RandomFeeder
namespace SharedTypes {

#define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
typedef RandomFeeder :: FakeInstruction ArchitecturalInstruction;

} //End SharedTypes
} //End Flexus

namespace Flexus {
namespace SimicsFeeder {
typedef Flexus::Typelist::append <
FLEXUS_PREVIOUS_InstructionTransport_Typemap
, Flexus::Typelist::pair <
Flexus::SharedTypes::ArchitecturalInstructionTag_t
, Flexus::SharedTypes::ArchitecturalInstruction
>
>::list InstructionTransport_Typemap;
#undef FLEXUS_PREVIOUS_InstructionTransport_Typemap
#define FLEXUS_PREVIOUS_InstructionTransport_Typemap Flexus::SimicsFeeder::InstructionTransport_Typemap
} //End SimicsFeeder
} //End Flexus

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT RandomFeeder

