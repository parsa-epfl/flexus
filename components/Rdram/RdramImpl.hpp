#include <iostream.h>
#include "RdramMagic.hpp"
#include "RdramBank.hpp"
#include "RdramBlackbox.hpp"

#define VERBOSE1  // for debugging purpose

#define FLEXUS_BEGIN_COMPONENT Rdram
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace Flexus {
namespace Rdram {

using namespace Flexus::Core;
using namespace Flexus::Debug;
using namespace Flexus::SharedTypes;

using Flexus::Debug::end;

class Rdram_SimicsObject_Impl  {
  RdramBlackbox * theBlackBox; //Non-owning pointer
public:
  //Initialize theBlackBox to zero
  Rdram_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) : theBlackBox(0) {
  }

  //Passed in by the RdramComponent's initialize call
  void setBlackBox(RdramBlackbox * aBlackBox) {
    theBlackBox = aBlackBox;
  }

  //Print the black boxes' statistics
  void printStats() {
    if (theBlackBox)
      theBlackBox->puke();
  }
};

class Rdram_SimicsObject : public Simics::AddInObject <Rdram_SimicsObject_Impl> {
  typedef Simics::AddInObject<Rdram_SimicsObject_Impl> base;
public:
  static const Simics::Persistence  class_persistence = Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "RDRAM";
  }
  static std::string classDescription() {
    return "PowerTap RDRAM simulation";
  }

  Rdram_SimicsObject() : base() { }
  Rdram_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  Rdram_SimicsObject(Rdram_SimicsObject_Impl * anImpl) : base(anImpl) {}

  struct PrintStatsCommand : public Simics::CommandT < PrintStatsCommand, Rdram_SimicsObject, Rdram_SimicsObject_Impl, void ( *)() , & Rdram_SimicsObject_Impl::printStats > {
  static std::string commandName() {
    return "command_printStats";
  }
  static std::string commandDescription() {
    return "PrintStats command hook";
  }
  };

  typedef mpl::list < PrintStatsCommand >::type Commands;
};

FLEXUS_COMPONENT class RdramComponent {
  FLEXUS_COMPONENT_IMPLEMENTATION(RdramComponent);

private:

  ////////////////////////////////////////////////////////////////////
  // CFC
  int64_t cyclesElapsed;

  MemoryTransport ack;  // reply message

  std::auto_ptr<RdramBlackbox> theBlackbox; // Deleted automagically
  //  when the rdram goes away

  //The Simics object for getting trace data
  Rdram_SimicsObject theRdramObject;

  //
  ////////////////////////////////////////////////////////////////////

public:
  // Initialization
  void initialize() {
    cout << endl;
    cout << "Initializing RDRAM component... " << endl;
    cout << "PowerTap (C) 2003 Carnegie Mellon University" << endl;
    cout << "Chi Chen and Anand Eswaran" << endl;
    cout << endl;

    cyclesElapsed = 0;
    theBlackbox = std::auto_ptr<RdramBlackbox>(new RdramBlackbox());

    Simics::Factory<Rdram_SimicsObject> rdram_factory;
    theRdramObject = rdram_factory.create("rdram");
    theRdramObject->setBlackBox(theBlackbox.get());

  }  // initialize

  // Print out statistics
  void printStats() {
    theBlackbox->puke();
  }

  // Ports
  struct RdramOut : public PushOutputPort< MemoryTransport > { };

  struct RdramIn : public PushInputPort<MemoryTransport>, AvailabilityDeterminedByInputs {
    typedef FLEXUS_IO_LIST ( 1, Availability<RdramOut> ) Inputs;
    typedef FLEXUS_IO_LIST ( 1, Value<RdramOut>  ) Outputs;

    /*
     * RDRAM memory requests received here
     */
    FLEXUS_WIRING_TEMPLATE
    static void push(self & aRdram, MemoryTransport aMessageTransport) {
      intrusive_ptr<MemoryMessage> msg(aMessageTransport[MemoryMessageTag]);
      FLEXUS_MBR_DEBUG() << "Rdram received: " << *msg << "\n";

      MemoryTransport transport;
      intrusive_ptr<MemoryMessage> reply = new MemoryMessage();
      reply->address() = msg->address();
      RdramPhysicalMemoryAddress memoryAddress = msg->address();
      switch (msg->type()) {
        case MemoryMessage::REQ_FILL:

          aRdram.theBlackbox->accessStatic(memoryAddress, RDRAM_READ);
          reply->type() = MemoryMessage::RESP_FILL_ACK;
          break;

        case MemoryMessage::REQ_WRITEBACK:

          aRdram.theBlackbox->accessStatic(memoryAddress, RDRAM_WRITE);
          reply->type() = MemoryMessage::RESP_WRITEBACK_ACK;
          break;

        default:
          FLEXUS_MBR_DEBUG() << "Don't know how to handle message." << end;
          return;
      }

      transport.set(MemoryMessageTag, reply);
      FLEXUS_MBR_ASSERT(( FLEXUS_CHANNEL(aRdram, RdramOut).available() ));
      FLEXUS_MBR_DEBUG() << "Sending reply: " << *reply << end;
      FLEXUS_CHANNEL( aRdram, RdramOut) << transport;

    }  // push

    FLEXUS_WIRING_TEMPLATE
    static bool available(self & aRdram) {
      return FLEXUS_CHANNEL(aRdram, RdramOut).available();
    }
  };

  // Rdram Interfaces
  struct RdramDrive {
    typedef FLEXUS_IO_LIST( 2, Availability<RdramOut> , Value<RdramIn> ) Inputs;
    typedef FLEXUS_IO_LIST( 1, Value<RdramOut> ) Outputs;

    /**
     * DOCYCLE -- function that is invoked every cycle
     */
    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aRdram) {
      aRdram.theBlackbox->doServe();

#ifdef VERBOSE
      if (aRdram.cyclesElapsed >= RDRAM_SPILL_INTERVAL) {
        aRdram.theBlackbox->printStates();
        aRdram.cyclesElapsed = 0;
      } else {
        aRdram.cyclesElapsed++;
      }
#endif

    }  // doCycle
  };

  // Declare the list of Drive interfaces
  //typedef end_of_list DriveInterfaces;
  typedef FLEXUS_DRIVE_LIST ( 1, RdramDrive) DriveInterfaces;

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(RdramConfiguration);

} //End Namespace Rdram
} //End Namespace Flexus

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Rdram

