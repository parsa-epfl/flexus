#define FLEXUS_BEGIN_COMPONENT VirtutechTraceFeederComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories VirtutechTraceFeeder, Feeder
#define DBG_SetDefaultOps AddCat(VirtutechTraceFeeder | Feeder)
#include DBG_Control()

#include "VirtutechTraceReader.hpp"

namespace nVirtutechTraceFeeder {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::intrusive_ptr;

typedef nVirtutechTraceReader::TraceEntry TraceEntry;
typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;

template <class Cfg>
class VirtutechTraceFeederComponent : public FlexusComponentBase<VirtutechTraceFeederComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(VirtutechTraceFeederComponent, Cfg);

public:
  VirtutechTraceFeederComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  ~VirtutechTraceFeederComponent() {
  }

  // Initialization
  void initialize() {
    myTraceReader.init();
  }

  //Ports
  struct ToMemory : public PushOutputPortArray<MemoryTransport, NUM_PROCS> { };

  struct FromMemory : public PushInputPortArray<MemoryTransport, NUM_PROCS>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self & aFeeder, index_t anIndex, MemoryTransport transport) {
      // do absolutely nothing!
    }
  };

  //Drive Interfaces
  struct VirtutechTraceFeederDrive {
    typedef FLEXUS_IO_LIST( 1, Availability<ToMemory> ) Inputs;
    typedef FLEXUS_IO_LIST( 1, Value<ToMemory> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aFeeder) {
      TraceEntry nextEntry;
      aFeeder.myTraceReader.next(nextEntry);
      DBG_(VVerb, ( << "got: " << nextEntry ) );

      // create the transport to pass to the memory
      MemoryTransport trans;

      intrusive_ptr<MemoryMessage> msg;
      if (nextEntry.write) {
        msg = new MemoryMessage(MemoryMessage::WriteReq, MemoryAddress(nextEntry.address));
      } else {
        msg = new MemoryMessage(MemoryMessage::ReadReq, MemoryAddress(nextEntry.address));
      }
      trans.set(MemoryMessageTag, msg);

      intrusive_ptr<ExecuteState> exec( new ExecuteState(MemoryAddress(nextEntry.pc), nextEntry.priv) );
      trans.set(ExecuteStateTag, exec);

      FLEXUS_CHANNEL_ARRAY(aFeeder, ToMemory, nextEntry.node) << trans;
    }

  };  // end VirtutechTraceFeederDrive

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, VirtutechTraceFeederDrive) DriveInterfaces;

private:

  // the trace reader
  nVirtutechTraceReader::VirtutechTraceReader myTraceReader;

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(VirtutechTraceFeederConfiguration);

} //End Namespace nVirtutechTraceFeeder

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT VirtutechTraceFeederComponent

#define DBG_Reset
#include DBG_Control()

