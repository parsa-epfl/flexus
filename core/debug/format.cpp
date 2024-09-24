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
