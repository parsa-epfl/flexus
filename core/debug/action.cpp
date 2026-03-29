#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <core/debug/action.hpp>
#include <core/debug/debugger.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>
#include <core/target.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

namespace Flexus {

namespace Dbg {

void
CompoundAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    for (auto* anAction : theActions) {
        anAction->printConfiguration(anOstream, anIndent);
    }
}

void
CompoundAction::process(Entry const& anEntry)
{
    for (auto* anAction : theActions) {
        anAction->process(anEntry);
    }
}

CompoundAction::~CompoundAction()
{
    for (auto* anAction : theActions) {
        delete anAction;
    }
}

void
CompoundAction::add(std::unique_ptr<Action> anAction)
{
    theActions.push_back(anAction.release()); // Ownership assumed by theActions
}

void
ConsoleLogAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "log console";
    theFormat->printConfiguration(anOstream, "");
    anOstream << ";\n";
}

void
ConsoleLogAction::process(Entry const& anEntry)
{
    theFormat->format(std::cerr, anEntry);
    std::cerr.flush();
}

class StreamManager
{
    typedef std::map<std::string, std::ofstream*> stream_map;
    stream_map theStreams;

  public:
    std::ostream& getStream(std::string const& aFile)
    {
        std::pair<stream_map::iterator, bool> insert_result;
        insert_result = theStreams.insert(std::make_pair(aFile, static_cast<std::ofstream*>(0)));
        if (insert_result.second) {
            (*insert_result.first).second = new std::ofstream(aFile.c_str());
            std::cout << "Opening debug output file: " << aFile << "\n";
        }
        return *(*insert_result.first).second;
    }

    ~StreamManager()
    {
        stream_map::iterator iter = theStreams.begin();
        while (iter != theStreams.end()) {
            (*iter).second->close();
            delete (*iter).second;
            ++iter;
        }
    }
};

// The StreamManager instance
std::unique_ptr<StreamManager> theStreamManager;

// StreamManager accessor
inline StreamManager&
streamManager()
{
    if (theStreamManager.get() == 0) { theStreamManager.reset(new StreamManager()); }
    return *theStreamManager;
}

FileLogAction::FileLogAction(std::string aFilename, Format* aFormat)
  : theFilename(aFilename)
  , theOstream(streamManager().getStream(aFilename))
  , theFormat(aFormat) {};

void
FileLogAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "log " << theFilename;
    theFormat->printConfiguration(anOstream, "");
    anOstream << ";\n";
}

void
FileLogAction::process(Entry const& anEntry)
{
    theFormat->format(theOstream, anEntry);
    theOstream.flush();
}

void
AbortAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "abort ;";
}

void
AbortAction::process(Entry const& anEntry)
{
    std::abort();
}

void
BreakAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "break ;";
}

void
BreakAction::process(Entry const& anEntry)
{
    DBG_Assert(false, (<< " Should be unreachable"));
}

void
PrintStatsAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "print-stats ;";
}

void
PrintStatsAction::process(Entry const& anEntry)
{
    Flexus::Stat::getStatManager()->printMeasurement("all", std::cout);
}

SeverityAction::SeverityAction(uint32_t aSeverity)
  : theSeverity(aSeverity) {};

void
SeverityAction::printConfiguration(std::ostream& anOstream, std::string const& anIndent)
{
    anOstream << anIndent << "set-global-severity " << toString(Severity(theSeverity)) << " ;";
}

void
SeverityAction::process(Entry const& anEntry)
{
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Severity(theSeverity));
}

} // namespace Dbg
} // namespace Flexus