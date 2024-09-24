
#include "Unallocated.hpp"

namespace nDecoder {
using namespace nuArch;

struct BlackBoxInstruction : public ArchInstruction
{

    BlackBoxInstruction(VirtualMemoryAddress aPC,
                        Opcode anOpcode,
                        boost::intrusive_ptr<BPredState> aBPState,
                        uint32_t aCPU,
                        int64_t aSequenceNo)
      : ArchInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo)
    {
        setClass(clsSynchronizing, codeBlackBox);
        forceResync();
    }

    virtual bool mayRetire() const { return true; }

    virtual bool postValidate() { return false; }

    virtual void describe(std::ostream& anOstream) const
    {
        ArchInstruction::describe(anOstream);
        anOstream << instCode();
    }
};

archinst
blackBox(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return archinst(new BlackBoxInstruction(aFetchedOpcode.thePC,
                                            aFetchedOpcode.theOpcode,
                                            aFetchedOpcode.theBPState,
                                            aCPU,
                                            aSequenceNo));
}

archinst
grayBox(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo, eInstructionCode aCode)
{
    archinst inst(new BlackBoxInstruction(aFetchedOpcode.thePC,
                                          aFetchedOpcode.theOpcode,
                                          aFetchedOpcode.theBPState,
                                          aCPU,
                                          aSequenceNo));
    inst->forceResync();
    inst->setClass(clsSynchronizing, aCode);
    return inst;
}

archinst
nop(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    SemanticInstruction* inst(new SemanticInstruction(aFetchedOpcode.thePC,
                                                      aFetchedOpcode.theOpcode,
                                                      aFetchedOpcode.theBPState,
                                                      aCPU,
                                                      aSequenceNo));
    inst->setClass(clsComputation, codeNOP);
    return inst;
}

archinst
unallocated_encoding(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    /* Unallocated and reserved encodings are uncategorized */
}

archinst
unsupported_encoding(archcode const& aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo)
{
    DECODER_TRACE;
    /* Unallocated and reserved encodings are uncategorized */
    return blackBox(aFetchedOpcode, aCPU, aSequenceNo);
    // throw Exception;
}

} // namespace nDecoder