#ifndef FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED

#include <core/debug/category.hpp>
#include <core/debug/field.hpp>
#include <core/debug/severity.hpp>
#include <set>

namespace Flexus {
namespace Dbg {

class Entry
{
    std::set<Field> theFields;
    std::set<int> theCategories;
    Severity theSeverity;

  public:
    Entry(Severity aSeverity,
          char const* aFile,
          int64_t aLine,
          char const* aFunction,
          int64_t aGlobalCount,
          int64_t aCycleCount);
    Entry& set(std::string const& aFieldName);
    Entry& set(std::string const& aFieldName, std::string const& aFieldValue);
    Entry& set(std::string const& aFieldName, int64_t aFieldValue);
    Entry& append(std::string const& aFieldName, std::string const& aFieldValue);
    Entry& addCategory(Category* aCategory);
    Entry& addCategories(CategoryHolder const& aCategoryHolder);
    std::string get(std::string aFieldName) const;
    int64_t getNumeric(std::string aFieldName) const;
    bool exists(std::string aFieldName) const;
    bool hasCategory(Category const* aCategory) const;
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_ENTRY_HPP_INCLUDED
