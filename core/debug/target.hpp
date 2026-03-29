#ifndef FLEXUS_CORE_DEBUG_TARGET_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_TARGET_HPP_INCLUDED

#include <core/debug/action.hpp>
#include <core/debug/entry.hpp>
#include <core/debug/filter.hpp>
#include <memory>
#include <string>

namespace Flexus {
namespace Dbg {

class Target
{
    std::string theName;
    std::unique_ptr<Filter> theFilter;
    std::unique_ptr<Action> theAction;

  public:
    Target(std::string const& aName, Filter* aFilter, Action* anAction);
    void process(Entry const& anEntry);
    Filter& filter();
    void setFilter(std::unique_ptr<Filter> aFilter);
    Action& action();
    void setAction(std::unique_ptr<Action> anAction);
    void printConfiguration(std::ostream& anOstream, std::string const& anIndent = std::string(""));
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_TARGET_HPP_INCLUDED
