
#ifndef FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED
#define FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED

#include "components/uArch/uArchInterfaces.hpp"
#include "core/types.hpp"

namespace nDecoder {

using Flexus::SharedTypes::VirtualMemoryAddress;

struct BranchInteraction : public nuArch::Interaction
{
    boost::intrusive_ptr<nuArch::Instruction> theIssuer;
    
    BranchInteraction(boost::intrusive_ptr<nuArch::Instruction> anIssuer);
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
branchInteraction(boost::intrusive_ptr<nuArch::Instruction> anIssuer);

} // namespace nDecoder

#endif // FLEXUS_armDECODER_INTERACTIONS_HPP_INCLUDED