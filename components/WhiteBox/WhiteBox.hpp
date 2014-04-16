#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT WhiteBox
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define WhiteBox_IMPLEMENTATION    (<components/WhiteBox/WhiteBoxImpl.hpp>)

COMPONENT_NO_PARAMETERS ;

COMPONENT_EMPTY_INTERFACE ;

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT WhiteBox

