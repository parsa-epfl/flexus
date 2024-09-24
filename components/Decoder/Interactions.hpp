
#ifndef FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED

#include "components/uArch/uArchInterfaces.hpp"
#include "core/types.hpp"

namespace nDecoder {

using Flexus::SharedTypes::VirtualMemoryAddress;

struct BranchInteraction : public nuArch::Interaction
{

    VirtualMemoryAddress theTarget;
    BranchInteraction(VirtualMemoryAddress aTarget);
    void operator()(boost::intrusive_ptr<nuArch::Instruction> anInstruction, nuArch::uArch& aCore);
    void describe(std::ostream& anOstream) const;
    //  boost::optional< uint64_t> npc() {
    //    return boost::optional<uint64_t>(theTarget);
    //  }
};

nuArch::Interaction*
reinstateInstructionInteraction();
nuArch::Interaction*
annulInstructionInteraction();
nuArch::Interaction*
branchInteraction(VirtualMemoryAddress aTarget);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED