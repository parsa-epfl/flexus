#ifndef FLEXUS_CORE_DEBUG_FORMAT_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_FORMAT_HPP_INCLUDED

#include <core/debug/entry.hpp>
#include <iostream>
#include <string>

namespace Flexus {
namespace Dbg {

struct Format
{
    virtual void format(std::ostream& anOstream, Entry const& anEntry) const                    = 0;
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) const = 0;
    virtual ~Format() {}
};

class StaticFormat : public Format
{
    std::string theValue;

  public:
    StaticFormat(std::string const& aValue);
    virtual void format(std::ostream& anOstream, Entry const& anEntry) const;
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) const;
};

class FieldFormat : public Format
{
    std::string theField;

  public:
    FieldFormat(std::string const& aField);
    virtual void format(std::ostream& anOstream, Entry const& anEntry) const;
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) const;
};

class CompoundFormat : public Format
{
    std::vector<Format*> theFormats; // These actions are owned by the pointers in theFormats
  public:
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) const;
    virtual void format(std::ostream& anOstream, Entry const& anEntry) const;
    virtual ~CompoundFormat() { destruct(); }
    void destruct();

    // Assumes ownership of the passed in Format
    void add(Format* aFormat);
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_FORMAT_HPP_INCLUDED
