
#ifndef FLEXUS_DECODER_VALIDATIONS_HPP_INCLUDED
#define FLEXUS_DECODER_VALIDATIONS_HPP_INCLUDED

#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace nDecoder {

struct validateXRegister
{
    uint32_t theReg;
    eOperandCode theOperandCode;
    SemanticInstruction* theInstruction;
    bool the_64;

    validateXRegister(uint32_t aReg, eOperandCode anOperand, SemanticInstruction* anInstruction, bool is_64)
      : theReg(aReg)
      , theOperandCode(anOperand)
      , theInstruction(anInstruction)
      , the_64(is_64)
    {
    }

    bool operator()();
};

struct validatePC
{
    SemanticInstruction* theInstruction;
    bool thePreValidation;

    validatePC(SemanticInstruction* anInstruction, bool prevalidation = false)
      : theInstruction(anInstruction)
      , thePreValidation(prevalidation)
    {
    }

    bool operator()();
};

struct validateMemory
{
    eOperandCode theAddressCode;
    eOperandCode theValueCode;
    nuArch::eSize theSize;
    SemanticInstruction* theInstruction;

    validateMemory(eOperandCode anAddressCode,
                   eOperandCode aValueCode,
                   nuArch::eSize aSize,
                   SemanticInstruction* anInstruction)
      : theAddressCode(anAddressCode)
      , theValueCode(aValueCode)
      , theSize(aSize)
      , theInstruction(anInstruction)
    {
    }

    bool operator()();
};

} // namespace nDecoder

#endif // FLEXUS_DECODER_VALIDATIONS_HPP_INCLUDED
