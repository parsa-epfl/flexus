#ifndef FLEXUS_CORE_DEBUG_SEVERITY_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_SEVERITY_HPP_INCLUDED

namespace Flexus {
namespace Dbg {

enum Severity {
  SevInv   = 0
  , SevVVerb
  , SevVerb
  , SevIface
  , SevTrace
  , SevDev
  , SevCrit
  , SevTmp
  , NumSev
};

std::string const & toString(Severity aSeverity);

} //Dbg
} //Flexus

#endif //FLEXUS_CORE_DEBUG_SEVERITY_HPP_INCLUDED

