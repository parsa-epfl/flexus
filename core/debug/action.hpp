#ifndef FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED

#include <core/debug/entry.hpp>
#include <core/debug/format.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace Flexus {
namespace Dbg {

class Action
{
  public:
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) = 0;
    virtual void process(Entry const& anEntry)                                            = 0;
    virtual ~Action() {}
};

class CompoundAction : public Action
{
    std::vector<Action*> theActions; // These actions are owned by the pointers in theActions
  public:
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
    virtual ~CompoundAction();
    void add(std::unique_ptr<Action> anAction);
};

class ConsoleLogAction : public Action
{
    std::unique_ptr<Format> theFormat;

  public:
    ConsoleLogAction(Format* aFormat)
      : theFormat(aFormat) {};
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

class FileLogAction : public Action
{
    std::string theFilename;
    std::ostream& theOstream;
    std::unique_ptr<Format> theFormat;

  public:
    FileLogAction(std::string aFilename, Format* aFormat);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

struct AbortAction : public Action
{
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

struct BreakAction : public Action
{
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

struct PrintStatsAction : public Action
{
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

struct SeverityAction : public Action
{
    uint32_t theSeverity;

  public:
    SeverityAction(uint32_t aSeverity);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    virtual void process(Entry const& anEntry);
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_ACTION_HPP_INCLUDED
