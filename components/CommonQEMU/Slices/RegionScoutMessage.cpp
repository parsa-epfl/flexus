#include <iostream>
#include <list>

#include <components/CommonQEMU/Slices/RegionScoutMessage.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator << (std::ostream & s, RegionScoutMessage::RegionScoutMessageType const & aType) {
  char const * message_types[] = {
    "Region Probe",
    "Region State Probe",
    "Region Miss Reply",
    "Region Hit Reply",
    "Region Is Shared",
    "Region Not Shared",
    "Region Global Miss",
    "Region Partial Miss",
    "Region Probe Owner",
    "Region Owner Reply",
    "Region Set Owner",
    "Region Evict",
    "Block Probe",
    "BlockScout Probe",
    "Block Miss Reply",
    "Block Hit Reply",
    "Set Tag Probe",
    "RVA Set Tag Probe",
    "Set Tag Reply",
    "RVA Set Tag Reply"
  };
  s << message_types[aType];
  return s;
}

std::ostream & operator << (std::ostream & s, RegionScoutMessage const & aMsg) {
  s << aMsg.type() << " for Region " << std::hex << aMsg.region() << std::dec;
  return s;
}

} //End SharedTypes
} //End Flexus

