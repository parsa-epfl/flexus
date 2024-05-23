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
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <core/debug/entry.hpp>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Flexus {
namespace Dbg {

Entry::Entry(Severity aSeverity,
             char const* aFile,
             int64_t aLine,
             char const* aFunction,
             int64_t aGlobalCount,
             int64_t aCycleCount)
  : theSeverity(aSeverity)
{
    set("Severity", toString(theSeverity));
    set("SeverityNumeric", theSeverity);
    std::string path(aFile);
    set("FilePath", path);
    std::string::size_type last_slash(path.rfind('/'));
    if (last_slash == std::string::npos) {
        set("File", path);
    } else {
        set("File", std::string(path, last_slash + 1));
    }
    set("Line", aLine);
    set("Function", std::string(aFunction) + "()");
    set("GlobalCount", aGlobalCount);
    set("Cycles", aCycleCount);
    set("Categories", "");
}

Entry&
Entry::set(std::string const& aFieldName)
{
    theFields.insert(aFieldName);
    return *this;
}
Entry&
Entry::set(std::string const& aFieldName, std::string const& aFieldValue)
{
    std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
    iter->setValue(aFieldValue);
    return *this;
}
Entry&
Entry::set(std::string const& aFieldName, int64_t aFieldValue)
{
    std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
    iter->setValue(aFieldValue);
    return *this;
}
Entry&
Entry::append(std::string const& aFieldName, std::string const& aFieldValue)
{
    std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
    iter->setValue(iter->value() + aFieldValue);
    return *this;
}
Entry&
Entry::addCategory(Category* aCategory)
{
    if (theCategories.insert(aCategory->number()).second) {
        set("Categories", get("Categories") + ' ' + aCategory->name());
    }
    return *this;
}

Entry&
Entry::addCategories(CategoryHolder const& aCategoryHolder)
{
    CategoryHolder::const_iterator iter(aCategoryHolder.begin());
    while (iter != aCategoryHolder.end()) {
        if (theCategories.insert((*iter)->number()).second) {
            set("Categories", get("Categories") + ' ' + (*iter)->name());
        }
        ++iter;
    }
    return *this;
}

std::string
Entry::get(std::string aFieldName) const
{
    std::set<Field>::const_iterator iter(theFields.find(aFieldName));
    if (iter != theFields.end()) {
        return iter->value();
    } else {
        return "<undefined>";
    }
}

int64_t
Entry::getNumeric(std::string aFieldName) const
{
    std::set<Field>::const_iterator iter = theFields.find(aFieldName);
    if (iter != theFields.end()) {
        return iter->numericValue();
    } else {
        return 0;
    }
}

bool
Entry::exists(std::string aFieldName) const
{
    return (theFields.find(aFieldName) != theFields.end());
}

bool
Entry::hasCategory(Category const* aCategory) const
{
    return (theCategories.find(aCategory->number()) != theCategories.end());
}

} // namespace Dbg
} // namespace Flexus
