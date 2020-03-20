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
#ifndef FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED

#include <set>

#include <core/debug/category.hpp>
#include <core/debug/field.hpp>
#include <core/debug/severity.hpp>

namespace Flexus {
namespace Dbg {

class Entry {
  std::set<Field> theFields;
  std::set<int> theCategories;
  Severity theSeverity;

public:
  Entry(Severity aSeverity, char const *aFile, int64_t aLine, char const *aFunction,
        int64_t aGlobalCount, int64_t aCycleCount);
  Entry &set(std::string const &aFieldName);
  Entry &set(std::string const &aFieldName, std::string const &aFieldValue);
  Entry &set(std::string const &aFieldName, int64_t aFieldValue);
  Entry &append(std::string const &aFieldName, std::string const &aFieldValue);
  Entry &addCategory(Category *aCategory);
  Entry &addCategories(CategoryHolder const &aCategoryHolder);
  std::string get(std::string aFieldName) const;
  int64_t getNumeric(std::string aFieldName) const;
  bool exists(std::string aFieldName) const;
  bool hasCategory(Category const *aCategory) const;
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED
