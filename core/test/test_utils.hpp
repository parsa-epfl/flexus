#ifndef FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED
#define FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/test/unit_test.hpp>

#include <core/debug/debug.hpp>

#include <core/component.hpp>
#include <core/configuration.hpp>
#include <core/metaprogram.hpp>
#include <core/ports.hpp>
#include <core/simulator_layout.hpp>
#include <core/wiring.hpp>

using namespace boost::unit_test_framework;
using namespace Flexus::Core;

// V2 versions of macros
#define FLEXUS_TEST_IO_LIST(COUNT, ...) mpl::vector<__VA_ARGS__>
#define FLEXUS_TEST_IO_LIST_EMPTY mpl::vector<>
#define FLEXUS_TEST_WIRING template <class Handle, class Wiring>
#define FLEXUS_TEST_CHANNEL(COMPONENT, PORT)                                                       \
  Wiring::channel(static_cast<Handle *>(0), static_cast<PORT *>(0), COMPONENT.flexusIndex())
#define FLEXUS_TEST_CHANNEL_ARRAY(COMPONENT, PORT, IDX)                                            \
  Wiring::channel_array(static_cast<Handle *>(0), static_cast<PORT *>(0),                          \
                        (COMPONENT).flexusIndex(), IDX)
#define FLEXUS_TEST_DRIVE_LIST(COUNT, ...) mpl::vector<__VA_ARGS__>
#define FLEXUS_TEST_DRIVE_LIST_EMPTY mpl::vector<>

#endif // FLEXUS_CORE_TEST_TEST_UTILS_HPP_INCLUDED
