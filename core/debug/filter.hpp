#ifndef FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED

#include <core/debug/entry.hpp>
#include <core/debug/category.hpp>

#include <core/boost_extensions/lexical_cast.hpp>

namespace Flexus {
namespace Dbg {

struct Filter {
  enum MatchResult {
    Include
    , Exclude
    , NoMatch
  };

  virtual MatchResult match(Entry const & anEntry) = 0;
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent) = 0;
  virtual ~Filter() {}
};

struct CompoundFilter : public Filter {
protected:
  std::vector<Filter *> theFilters;

public:
  void destruct();
  virtual ~CompoundFilter() {
    destruct(); //Clean up all pointers owned by theActions
  }

  //Returns Exclude if any filter returns exclude.
  //Returns Include if at least one filter returns Include and none return Exclude
  //Otherwise, returns NoMatch
  virtual MatchResult match(Entry const & anEntry);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
  void add(Filter * aFilter);
};

struct IncludeFilter : public CompoundFilter {
  //returns Include if all filters return Include, otherwise, returns NoMatch
  virtual MatchResult match(Entry const & anEntry);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
};

struct ExcludeFilter : public CompoundFilter {
  //returns Exclude if all filters return Include, otherwise, returns NoMatch
  virtual MatchResult match(Entry const & anEntry);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
};

struct SimpleFilter : public Filter {
protected:
  std::string theField;

public:
  SimpleFilter(std::string const & aField)
    : theField(aField)
  {}

  virtual ~SimpleFilter() { }
};

template <class ValueType, class Operation>
class OpFilter : public SimpleFilter {
  typename ValueType::type theValue;
public:
  OpFilter(std::string const & aField, typename ValueType::type const & aValue)
    : SimpleFilter(aField)
    , theValue(aValue)
  {}

  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent) {
    anOstream << anIndent << '{' << theField << "} " << Operation().toString() << ' ' << ValueType().toString(theValue) << "\n";
  };

  virtual MatchResult match(Entry const & anEntry) {
    Operation op;
    if ( op( ValueType().extract(anEntry, theField), theValue  ) ) {
      return Include;
    } else {
      return NoMatch;
    }
  }
};

struct String {
  typedef std::string type;
  std::string extract(Entry const & anEntry, std::string const & aField) {
    return anEntry.get(aField);
  }
  std::string toString(std::string const & aValue) {
    return std::string("\"") + aValue + '"';
  }
};
struct LongLong {
  typedef int64_t type;
  int64_t extract(Entry const & anEntry, std::string const & aField) {
    return anEntry.getNumeric(aField);
  }
  std::string toString(int64_t const & aValue) {
    return std::to_string(aValue);
  }
};

struct OpEqual {
  std::string const & toString() {
    static std::string str("==");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs == rhs;
  }
};

struct OpNotEqual {
  std::string const & toString() {
    static std::string str("!=");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs != rhs;
  }
};

struct OpLessThan {
  std::string const & toString() {
    static std::string str("<");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs < rhs;
  }
};

struct OpLessEqual {
  std::string const & toString() {
    static std::string str("<=");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs <= rhs;
  }
};

struct OpGreaterThan {
  std::string const & toString() {
    static std::string str(">");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs > rhs;
  }
};

struct OpGreaterEqual {
  std::string const & toString() {
    static std::string str(">=");
    return str;
  }
  template <class T>
  bool operator()(T const & lhs, T const & rhs) {
    return lhs >= rhs;
  }
};

struct ExistsFilter : public SimpleFilter {
  ExistsFilter(std::string const & aField);
  virtual MatchResult match(Entry const & anEntry);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
};

class CategoryFilter : public Filter {
  Category const * theCategory;
public:
  CategoryFilter(std::string const & aCategory);
  virtual MatchResult match(Entry const & anEntry);
  virtual void printConfiguration(std::ostream & anOstream, std::string const & anIndent);
};

} //Dbg
} //Flexus

#endif //FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED

