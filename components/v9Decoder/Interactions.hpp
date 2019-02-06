#ifndef FLEXUS_v9DECODER_INTERACTIONS_HPP_INCLUDED
#define FLEXUS_v9DECODER_INTERACTIONS_HPP_INCLUDED

namespace nv9Decoder {

using Flexus::SharedTypes::VirtualMemoryAddress;

struct BranchInteraction : public nuArch::Interaction {
  VirtualMemoryAddress theTarget;
  BranchInteraction( VirtualMemoryAddress aTarget);
  void operator() (boost::intrusive_ptr<nuArch::Instruction> anInstruction, nuArch::uArch & aCore);
  void describe(std::ostream & anOstream) const;
  boost::optional< uint64_t> npc() {
    return boost::optional<uint64_t>(theTarget);
  }
};

nuArch::Interaction * reinstateInstructionInteraction();
nuArch::Interaction * annulInstructionInteraction();
nuArch::Interaction * branchInteraction( VirtualMemoryAddress aTarget );

} //nv9Decoder

#endif //FLEXUS_v9DECODER_INTERACTIONS_HPP_INCLUDED
