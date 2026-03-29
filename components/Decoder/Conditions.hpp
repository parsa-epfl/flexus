
#ifndef FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED
#define FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED

#include "OperandMap.hpp"
#include "SemanticInstruction.hpp"

namespace nDecoder {
using namespace nuArch;

enum eCondCode
{
    kCBZ_,
    kCBNZ_,
    kTBZ_,
    kTBNZ_,
    kBCOND_,
};

struct Condition
{
    virtual ~Condition() {}
    virtual bool operator()(std::vector<Operand> const& operands) = 0;
    virtual char const* describe() const                          = 0;
    void setInstruction(SemanticInstruction* anInstruction) { theInstruction = anInstruction; }

  protected:
    SemanticInstruction* theInstruction;
};

std::unique_ptr<Condition>
condition(eCondCode aCond);

} // namespace narmDecoder

#endif // FLEXUS_ARMDECODER_CONDITIONS_HPP_INCLUDED