#include <algorithm>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include <core/boost_extensions/lexical_cast.hpp>

#include <core/debug/entry.hpp>
#include <core/debug/category.hpp>

#include <core/debug/filter.hpp>

namespace Flexus {
namespace Dbg {
using boost::lambda::_1;
using boost::lambda::bind;
using boost::lambda::var;
using boost::lambda::delete_ptr;

typedef Filter::MatchResult MatchResult;

void CompoundFilter::destruct() {
  std::for_each(theFilters.begin(), theFilters.end(), bind( delete_ptr(), _1)); //Clean up all pointers owned by theActions
}

//Returns Exclude if any filter returns exclude.
//Returns Include if at least one filter returns Include and none return Exclude
//Otherwise, returns NoMatch
MatchResult CompoundFilter::match(Entry const & anEntry) {
  MatchResult result = NoMatch;
  std::vector<Filter *>::iterator iter = theFilters.begin();
  while (iter != theFilters.end()) {
    MatchResult test = (*iter)->match(anEntry);
    if (test == Exclude) {
      result = Exclude;
      break;
    } else if (test == Include) {
      result = Include;
    }
    ++iter;
  }
  return result;
}

void CompoundFilter::printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
  std::for_each(theFilters.begin(), theFilters.end(), bind(&Filter::printConfiguration, _1, var(anOstream), anIndent) );
};

void CompoundFilter::add(Filter * aFilter) {
  theFilters.push_back(aFilter); //Ownership assumed by theFilters
}

MatchResult IncludeFilter::match(Entry const & anEntry) {
  MatchResult result(Include);
  std::vector<Filter *>::iterator iter(theFilters.begin());
  while (iter != theFilters.end() && result == Include) {
    result = (*iter)->match(anEntry);
    ++iter;
  }
  if (result != Include) {
    result = NoMatch;
  }
  return result;
}

void IncludeFilter::printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
  std::vector<Filter *>::iterator iter(theFilters.begin());
  if (iter != theFilters.end()) {
    (*iter)->printConfiguration(anOstream, anIndent + "+ ");
    ++iter;
    std::for_each(iter, theFilters.end(), bind(&Filter::printConfiguration, _1, var(anOstream), anIndent + "& ") );
  }
  anOstream << anIndent << ";\n";
};

MatchResult ExcludeFilter::match(Entry const & anEntry) {
  MatchResult result(Include);
  std::vector<Filter *>::iterator iter = theFilters.begin();
  while (iter != theFilters.end() && result == Include) {
    result = (*iter)->match(anEntry);
    ++iter;
  }
  if (result == Include) {
    result = Exclude;
  } else {
    result = NoMatch;
  }
  return result;
}

void ExcludeFilter::printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
  std::vector<Filter *>::iterator iter(theFilters.begin());
  if (iter != theFilters.end()) {
    (*iter)->printConfiguration(anOstream, anIndent + "- ");
    ++iter;
    std::for_each(iter, theFilters.end(), bind(&Filter::printConfiguration, _1, var(anOstream), anIndent + "& ") );
  }
  anOstream << anIndent << ";\n";
};

ExistsFilter::ExistsFilter(std::string const & aField)
  : SimpleFilter(aField)
{}

MatchResult ExistsFilter::match(Entry const & anEntry) {
  if (anEntry.exists(theField)) {
    return Include;
  } else {
    return NoMatch;
  }
}

void ExistsFilter::printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
  anOstream << anIndent << '{' << theField << "} exists\n";
};

CategoryFilter::CategoryFilter(std::string const & aCategory)
  : theCategory(& CategoryMgr::categoryMgr().category(aCategory))
{ }

MatchResult CategoryFilter::match(Entry const & anEntry) {
  if (anEntry.hasCategory(theCategory)) {
    return Include;
  } else {
    return NoMatch;
  }
}

void CategoryFilter::printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
  anOstream << anIndent << theCategory->name() << "\n";
};

} //Dbg
} //Flexus

