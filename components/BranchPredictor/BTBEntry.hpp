#ifndef FLEXUS_BTB_ENTRY
#define FLEXUS_BTB_ENTRY

#include "components/uFetch/uFetchTypes.hpp"
#include "core/types.hpp"

using namespace Flexus::SharedTypes;

class BTBEntry
{
  public:
    bool valid;
    VirtualMemoryAddress thePC;
    mutable eBranchType theBranchType;
    mutable VirtualMemoryAddress theTarget;

    BTBEntry(VirtualMemoryAddress aPC, eBranchType aType, VirtualMemoryAddress aTarget)
      : valid(true)
      , thePC(aPC)
      , theBranchType(aType)
      , theTarget(aTarget)
    {
    }

    BTBEntry()
      : valid(false)
    {
    }
};

#endif