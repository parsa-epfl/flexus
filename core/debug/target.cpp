#include <core/debug/action.hpp>
#include <core/debug/entry.hpp>
#include <core/debug/filter.hpp>

#include <core/debug/target.hpp>

namespace Flexus {
namespace Dbg {

Target::Target(std::string const &aName, Filter *aFilter, Action *anAction)
    : theName(aName), theFilter(aFilter), theAction(anAction) {
}

void Target::process(Entry const &anEntry) {
  if (theFilter->match(anEntry) == Filter::Include) {
    theAction->process(anEntry);
  }
}

Filter &Target::filter() {
  return *theFilter;
}
void Target::setFilter(std::unique_ptr<Filter> aFilter) {
  theFilter = std::move(aFilter);
}

Action &Target::action() {
  return *theAction;
}
void Target::setAction(std::unique_ptr<Action> anAction) {
  theAction = std::move(anAction);
}

void Target::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  anOstream << anIndent << "target \"" << theName << "\" {\n";
  anOstream << anIndent << "  filter {\n";

  theFilter->printConfiguration(anOstream, anIndent + "    ");

  anOstream << anIndent << "  }\n";
  anOstream << anIndent << "  action {\n";

  theAction->printConfiguration(anOstream, anIndent + "    ");

  anOstream << anIndent << "  }\n";
  anOstream << anIndent << "}\n";
}

} // namespace Dbg
} // namespace Flexus
