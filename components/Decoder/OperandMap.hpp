
#ifndef FLEXUS_DECODER_OPERANDMAP_HPP_INCLUDED
#define FLEXUS_DECODER_OPERANDMAP_HPP_INCLUDED

#include "OperandCode.hpp"

#include <components/uArch/uArchInterfaces.hpp>

using namespace nuArch;

namespace nDecoder {

using operand_typelist_1 = mpl::push_front<register_value::types, mapped_reg>::type ;
using operand_typelist = mpl::push_front<operand_typelist_1, reg>::type;
using Operand = boost::make_variant_over<operand_typelist>::type;

class OperandMap
{

    typedef std::map<eOperandCode, Operand> operand_map;
    operand_map theOperandMap;

  public:
    template<class T>
    T& operand(eOperandCode anOperandId)
    {
        return boost::get<T>(theOperandMap[anOperandId]);
    }

    Operand& operand(eOperandCode anOperandId) { return theOperandMap[anOperandId]; }

    template<class T>
    void set(eOperandCode anOperandId, T aT)
    {
        theOperandMap[anOperandId] = aT;
    }

    void set(eOperandCode anOperandId, Operand const& aT) { theOperandMap[anOperandId] = aT; }

    bool hasOperand(eOperandCode anOperandId) const { return (theOperandMap.find(anOperandId) != theOperandMap.end()); }

    struct operandPrinter
    {
        std::ostream& theOstream;
        operandPrinter(std::ostream& anOstream)
          : theOstream(anOstream)
        {
        }
        void operator()(operand_map::value_type const& anEntry)
        {
            theOstream << "\t   " << anEntry.first << " = " << anEntry.second << "\n";
        }
    };

    void dump(std::ostream& anOstream) const
    {
        std::for_each(theOperandMap.begin(), theOperandMap.end(), operandPrinter(anOstream));
    }
};

} // namespace nDecoder

#endif // FLEXUS_DECODER_OPERANDMAP_HPP_INCLUDED
