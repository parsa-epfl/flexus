//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

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
