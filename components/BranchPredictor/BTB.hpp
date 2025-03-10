#ifndef FLEXUS_BTB
#define FLEXUS_BTB

#include "BTBSet.hpp"
#include "components/uFetch/uFetchTypes.hpp"
#include "core/checkpoint/json.hpp"
#include "core/types.hpp"

#include <vector>

using json = nlohmann::json;
using namespace Flexus::SharedTypes;

class BTB
{
  private:
    std::vector<BTBSet> theBTB; // Array of Sets make up a cache
    uint64_t theIndexMask;

  public:
    uint32_t theBTBSets;
    uint32_t theBTBAssoc;

  private:
    BTB() = default;

  public:
    BTB(int32_t aBTBSets, int32_t aBTBAssoc);
    /* The PC is assumed to be word aligned so the index into the Set Associative structure is */
    /* INDEX_MASK anded with the PC >> 2 */
    uint32_t index(VirtualMemoryAddress anAddress);
    // Whether the BTB contains target for anAddress
    bool contains(VirtualMemoryAddress anAddress);
    // The kind of branch corresponding to the address: conditional, direct, indirect, return, etc
    eBranchType type(VirtualMemoryAddress anAddress);
    // Target address of the branch
    boost::optional<VirtualMemoryAddress> target(VirtualMemoryAddress anAddress);
    // Update or add a new entry to the BTB
    bool update(VirtualMemoryAddress aPC, eBranchType aType, VirtualMemoryAddress aTarget);

    json saveState() const;
    void loadState(json checkpoint);
};

#endif