#include <set>
#include <string>

#include <boost/optional.hpp>
#include <core/boost_extensions/lexical_cast.hpp>
#include <map>
#include <utility>
#include <vector>

#include <core/debug/entry.hpp>

namespace Flexus {
namespace Dbg {

Entry::Entry(Severity aSeverity, char const *aFile, int64_t aLine, char const *aFunction,
             int64_t aGlobalCount, int64_t aCycleCount)
    : theSeverity(aSeverity) {
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

Entry &Entry::set(std::string const &aFieldName) {
  theFields.insert(aFieldName);
  return *this;
}
Entry &Entry::set(std::string const &aFieldName, std::string const &aFieldValue) {
  std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
  iter->setValue(aFieldValue);
  return *this;
}
Entry &Entry::set(std::string const &aFieldName, int64_t aFieldValue) {
  std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
  iter->setValue(aFieldValue);
  return *this;
}
Entry &Entry::append(std::string const &aFieldName, std::string const &aFieldValue) {
  std::set<Field>::iterator iter(theFields.insert(aFieldName).first);
  iter->setValue(iter->value() + aFieldValue);
  return *this;
}
Entry &Entry::addCategory(Category *aCategory) {
  if (theCategories.insert(aCategory->number()).second) {
    set("Categories", get("Categories") + ' ' + aCategory->name());
  }
  return *this;
}

Entry &Entry::addCategories(CategoryHolder const &aCategoryHolder) {
  CategoryHolder::const_iterator iter(aCategoryHolder.begin());
  while (iter != aCategoryHolder.end()) {
    if (theCategories.insert((*iter)->number()).second) {
      set("Categories", get("Categories") + ' ' + (*iter)->name());
    }
    ++iter;
  }
  return *this;
}

std::string Entry::get(std::string aFieldName) const {
  std::set<Field>::const_iterator iter(theFields.find(aFieldName));
  if (iter != theFields.end()) {
    return iter->value();
  } else {
    return "<undefined>";
  }
}

int64_t Entry::getNumeric(std::string aFieldName) const {
  std::set<Field>::const_iterator iter = theFields.find(aFieldName);
  if (iter != theFields.end()) {
    return iter->numericValue();
  } else {
    return 0;
  }
}

bool Entry::exists(std::string aFieldName) const {
  return (theFields.find(aFieldName) != theFields.end());
}

bool Entry::hasCategory(Category const *aCategory) const {
  return (theCategories.find(aCategory->number()) != theCategories.end());
}

} // namespace Dbg
} // namespace Flexus
