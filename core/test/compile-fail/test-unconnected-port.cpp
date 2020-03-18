//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
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
