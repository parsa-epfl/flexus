#include <core/debug/debug.hpp>

#define FLEXUS_internal_COMP_DEBUG_SEV DBG_internal_Sev_to_int(Dev)

#include <boost/test/included/unit_test_framework.hpp>

namespace Flexus {
namespace Core {
void Break() {}
}
}

using namespace boost::unit_test_framework;

test_suite*
init_unit_test_suite( int32_t argc, char * argv[] ) {
  test_suite * test = BOOST_TEST_SUITE( "Flexus core unit tests" );

  DBG_( Dev, Core() ( << "Beginning Regression tests." ) );

  return test;
}

