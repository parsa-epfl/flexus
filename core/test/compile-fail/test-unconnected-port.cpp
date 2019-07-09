#include <list>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/test/unit_test.hpp>

#define FLEXUS__CORE_TEST UnconnectedPortTest

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/debug.hpp>
#include <core/metaprogram.hpp>
#include <core/ports.hpp>
#include <core/preprocessor/simulator_layout.hpp>
#include <core/wiring.hpp>

using namespace boost::unit_test_framework;
using namespace Flexus::Core;

FLEXUS_COMPONENT class WiringSource {
  FLEXUS_COMPONENT_IMPLEMENTATION(WiringSource);

public:
  void initialize() {
  }

  struct PushOut : public PushOutputPort<const int32_t> {};

  struct PushOutDrive {
    typedef mpl::vector<Availability<PushOut>> Inputs;
    typedef mpl::vector<Value<PushOut>> Outputs;

    template <class Handle, class Wiring> static void doCycle(self &theComponent) {
      BOOST_CHECK((Wiring::available(Handle(), PushOut())));
      Wiring::channel(Handle(), PushOut()) << 1;
    }
  };

  typedef mpl::list<PushOutDrive> DriveInterfaces;
};
FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(WiringSourceCfgTempl);

FLEXUS_COMPONENT class WiringSink {
  FLEXUS_COMPONENT_IMPLEMENTATION(WiringSink);

public:
  std::list<int> theList;
  void initialize() {
  }

  struct PushIn : public PushInputPort<const int32_t>, AlwaysAvailable {
    template <class Handle, class Wiring>
    static void push(self &theComponent, const int32_t anInt) {
      theComponent.theList.push_back(anInt);
    }
  };

  struct CheckDrive {
    typedef mpl::vector<Value<PushIn>> Inputs;

    typedef mpl::vector<> Outputs;

    template <class Handle, class Wiring> static void doCycle(self &theComponent) {
      BOOST_CHECK(theComponent.theList.front() == 1);
      theComponent.theList.pop_front();
    }
  };

  typedef mpl::list<CheckDrive> DriveInterfaces;
};
FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(WiringSinkCfgTempl);

namespace FLEXUS__CORE_TEST {
typedef WiringSourceCfgTempl<> WiringSourceCfg_t;
WiringSourceCfg_t WiringSourceCfg("wiring-test");

typedef WiringSinkCfgTempl<> WiringSinkCfg_t;
WiringSinkCfg_t WiringSinkCfg("wiring-test");

FLEXUS_INSTANTIATE_COMPONENT(WiringSource, WiringSourceCfg_t, WiringSourceCfg, NoDebug, theSource);
FLEXUS_INSTANTIATE_COMPONENT(WiringSink, WiringSinkCfg_t, WiringSinkCfg, NoDebug, theSink);
} // namespace FLEXUS__CORE_TEST

#include FLEXUS_BEGIN_COMPONENT_REGISTRATION_SECTION()
FLEXUS__CORE_TEST::theSink, FLEXUS__CORE_TEST::theSource
#include FLEXUS_END_COMPONENT_REGISTRATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_CODE_GENERATION_SECTION()

    void
    testUnconnectedPort() {

  // Initialize components
  BOOST_CHECKPOINT("About to test wiring");
  UnconnectedPortTest::Wiring::theDrive.doCycle();

  BOOST_CHECK_MESSAGE(true, "Wiring test complete");
}

test_suite *unconnected_port_test_suite() {
  test_suite *test = BOOST_TEST_SUITE("Unconnected Port unit test");

  test->add(BOOST_TEST_CASE(&testUnconnectedPort));

  return test;
}
