#include <iostream>
#include <list>

#include <components/CommonQEMU/Slices/FillType.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator << (std::ostream & anOstream, tFillType aType) {
  const char * const name[] = {
    "eCold"
    , "eReplacement"
    , "eCoherence"
    , "eDGP"
    , "eDMA"
    , "ePrefetch"
    , "eFetch"
    , "eNAW"
    , "eDataPrivate"
    , "eDataSharedRO"
    , "eDataSharedRW"
  };
  anOstream << name[aType];
  return anOstream;
}

} //End SharedTypes
} //End Flexus
