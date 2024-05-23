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
#include <algorithm>
#include <core/debug/format.hpp>
#include <iostream>
#include <string>

namespace Flexus {
namespace Dbg {

void
translateEscapes(std::string& aString)
{
    std::string clean_value(aString);
    std::string::size_type loc(0);
    while ((loc = clean_value.find('\\', loc)) != std::string::npos) {
        switch (clean_value.at(loc + 1)) {
            case '\'': clean_value.replace(loc, 2, "\'"); break;
            case '"': clean_value.replace(loc, 2, "\""); break;
            case '?': clean_value.replace(loc, 2, "\?"); break;
            case '\\': clean_value.replace(loc, 2, "\\"); break;
            case 'a': clean_value.replace(loc, 2, "\a"); break;
            case 'b': clean_value.replace(loc, 2, "\b"); break;
            case 'f': clean_value.replace(loc, 2, "\b"); break;
            case 'n': clean_value.replace(loc, 2, "\n"); break;
            case 'r': clean_value.replace(loc, 2, "\r"); break;
            case 't': clean_value.replace(loc, 2, "\t"); break;
            default:
                // Do nothing
                break;
        }
        ++loc;
    }

    aString.swap(clean_value);
}

StaticFormat::StaticFormat(std::string const& aValue)
  : theValue(aValue)
{
    // Replace all escape sequences in the format.  This functionality is broken
    // in the Spirit parser
    translateEscapes(theValue);
}

void
StaticFormat::format(std::ostream& anOstream, Entry const& anEntry) const
{
    anOstream << theValue;
}

void
StaticFormat::printConfiguration(std::ostream& anOstream, std::string const& anIndent) const
{
    std::string clean_value(theValue);
    std::string::size_type loc(0);
    while ((loc = clean_value.find('\n', loc)) != std::string::npos) {
        clean_value.replace(loc, 1, "\\n");
    }
    anOstream << anIndent << " \"" << clean_value << '\"';
}

FieldFormat::FieldFormat(std::string const& aField)
  : theField(aField)
{
}

void
FieldFormat::format(std::ostream& anOstream, Entry const& anEntry) const
{
    anOstream << anEntry.get(theField);
}
void
FieldFormat::printConfiguration(std::ostream& anOstream, std::string const& anIndent) const
{
    anOstream << anIndent << " {" << theField << '}';
}

void
CompoundFormat::destruct()
{
    for (auto* aFormat : theFormats) {
        delete aFormat;
    } // Clean up all pointers owned by theFormats
}

// Assumes ownership of the passed in Format
void
CompoundFormat::add(Format* aFormat)
{
    theFormats.push_back(aFormat); // Ownership assumed by theFormats
}

void
CompoundFormat::printConfiguration(std::ostream& anOstream, std::string const& anIndent) const
{
    for (auto* aFormat : theFormats) {
        aFormat->printConfiguration(anOstream, anIndent);
    }
}

void
CompoundFormat::format(std::ostream& anOstream, Entry const& anEntry) const
{
    for (auto* aFormat : theFormats) {
        aFormat->format(anOstream, anEntry);
    }
}

} // namespace Dbg
} // namespace Flexus
