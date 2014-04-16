#define FLEXUS_BEGIN_COMPONENT SimicsMemory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace Flexus {
namespace SimicsMemory {

using namespace Flexus::Core;
using namespace Flexus::Debug;
using namespace Flexus::SharedTypes;

using boost::intrusive_ptr;
using Flexus::Debug::end;

FLEXUS_COMPONENT class SimicsMemoryComponent {
  FLEXUS_COMPONENT_IMPLEMENTATION(SimicsMemoryComponent);

  // stats
  int32_t loadsTranslated;
  int32_t fillsReceived;
  int32_t storesTranslated;
  int32_t writebacksReceived;

public:
  ~SimicsMemoryComponent() {
    FLEXUS_MBR_DEBUG() << "\n"
                       << "Loads Translated:    " << loadsTranslated
                       << "\n"
                       << "Fills Received:      " << fillsReceived
                       << "\n"
                       << "Stores Translated:   " << storesTranslated
                       << "\n"
                       << "WriteBacks Received: " << writebacksReceived
                       << end;
  }

  // Initialization
  void initialize() {
    loadsTranslated = 0;
    fillsReceived = 0;
    storesTranslated = 0;
    writebacksReceived = 0;

    return;
  }

  // Ports
  struct MemHarnessCpuReply : public PushOutputPort<MemoryTransport> { };
  struct MemHarnessCpuRequest : public PushInputPort<MemoryTransport>, AvailabilityDeterminedByInputs {
    template <template <class Port> class Wiring>
    static void push(self & aMemHarness, MemoryTransport aMessage) {
      aMemHarness.translateCpuReqToMemReq<Wiring>(aMessage);
    }

    template <template <class Port> class Wiring>
    static bool available(self & aMemHarness) {
      return aMemHarness.available();
    }
  };

  // Drive Interfaces
  struct MemHarnessDrive {
    typedef FLEXUS_IO_LIST(1, Availability<MemHarnessCpuRequest>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<MemHarnessCpuReply>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & theComponent) {
      // do nothing - everything is port (push) driven
      // theComponent.doEvent<Wiring>();
    }
  };

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;
  //typedef FLEXUS_MAKE_TYPELIST (1, (MemHarnessDrive)) DriveInterfaces;

private:
  bool available() {
    return true;
  }

  FLEXUS_WIRING_TEMPLATE
  void translateCpuReqToMemReq(MemoryTransport aMessage) {

    intrusive_ptr<MemoryMessage> theMsg = new MemoryMessage();
    theMsg->theAddress = aMessage[MemoryMessageTag]->theAddress;
    theMsg->theData = aMessage[MemoryMessageTag]->theData;

    if (aMessage[MemoryMessageTag]->theOpCode == MemoryMessage::mLOAD) {
      loadsTranslated ++;
      theMsg->theOpCode = MemoryMessage::mRdReq;
    } else if (aMessage[MemoryMessageTag]->theOpCode == MemoryMessage::mSTORE) {
      storesTranslated ++;
      theMsg->theOpCode = MemoryMessage::mWB;
    }

    MemoryTransport aTransport;
    aTransport.set(MemoryMessageTag, theMsg);
    //if( Wiring<MemHarnessMemRequest>::available() ) {
    //  Wiring<MemHarnessMemRequest>::channel() << aTransport;
    //}

    return;
  }

  FLEXUS_WIRING_TEMPLATE
  void translateMemReplyToCpuReply(MemoryTransport aMessage) {

    intrusive_ptr<MemoryMessage> theMsg = new MemoryMessage();
    theMsg->theAddress = aMessage[MemoryMessageTag]->theAddress;
    theMsg->theData = aMessage[MemoryMessageTag]->theData;

    if (aMessage[MemoryMessageTag]->theOpCode == MemoryMessage::mFill) {
      fillsReceived ++;
      theMsg->theOpCode = MemoryMessage::mLOAD;
    } else if (aMessage[MemoryMessageTag]->theOpCode == MemoryMessage::mWBAck) {
      writebacksReceived ++;
      theMsg->theOpCode = MemoryMessage::mSTORE;
    }

    MemoryTransport aTransport;
    aTransport.set(MemoryMessageTag, theMsg);
    if (Wiring<MemHarnessCpuReply>::available() ) {
      Wiring<MemHarnessCpuReply>::channel() << aTransport;
    }

    return;
  }

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(SimicsMemoryComponentConfiguration);

} // End Namespace SimicsMemory
} // End Namespace Flexus

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SimicsMemory
