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
#define FLEXUS__CORE_TEST IncompatiblePortTest

#include <list>

#include <core/test/test_utils.hpp>

struct Payload {};

FLEXUS_COMPONENT class WiringSource {
  FLEXUS_COMPONENT_IMPLEMENTATION(WiringSource);

public:
  void initialize() {
  }

  struct MismatchOut : public PushOutputPort<const Payload> {};

  struct TestDrive {
    typedef FLEXUS_TEST_IO_LIST(1, Availability<MismatchOut>) Inputs;
    typedef FLEXUS_TEST_IO_LIST(1, Value<MismatchOut>) Outputs;

    FLEXUS_TEST_WIRING
    static void doCycle(self &theComponent) {
      BOOST_CHECK((FLEXUS_TEST_CHANNEL_AVAILABLE(MismatchOut)));
      FLEXUS_TEST_CHANNEL(MismatchOut) << Payload();
    }
  };

  typedef FLEXUS_TEST_DRIVE_LIST(1, TestDrive) DriveInterfaces;
};
FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(WiringSourceCfgTempl);

FLEXUS_COMPONENT class WiringSink {
  FLEXUS_COMPONENT_IMPLEMENTATION(WiringSink);

public:
  void initialize() {
  }

  struct MismatchOut : public PushOutputPort<const Payload> {};

  struct CheckDrive {
    typedef FLEXUS_TEST_IO_LIST(1, Availability<MismatchOut>) Inputs;
    typedef FLEXUS_TEST_IO_LIST(1, Value<MismatchOut>) Outputs;

    FLEXUS_TEST_WIRING
    static void doCycle(self &theComponent) {
    }
  };

  typedef FLEXUS_TEST_DRIVE_LIST(1, CheckDrive) DriveInterfaces;
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
FLEXUS__CORE_TEST::theSink,
    FLEXUS__CORE_TEST::theSource
#include FLEXUS_END_COMPONENT_REGISTRATION_SECTION()

#include FLEXUS_BEGIN_COMPONENT_WIRING_SECTION()

    FROM(FLEXUS__CORE_TEST::theSource, MismatchOut) TO(FLEXUS__CORE_TEST::theSink, MismatchOut)

#include FLEXUS_END_COMPONENT_WIRING_SECTION()

#include FLEXUS_CODE_GENERATION_SECTION()

        void testIncompatiblePort() {

  // Initialize components
  BOOST_CHECKPOINT("About to test wiring");
  FLEXUS__CORE_TEST::Wiring::theDrive.doCycle();

  BOOST_CHECK_MESSAGE(true, "Wiring test complete");
}

test_suite *incompatible_port_test_suite() {
  test_suite *test = BOOST_TEST_SUITE("Mismatched Payload unit test");

  test->add(BOOST_TEST_CASE(&testIncompatiblePort));

  return test;
}
