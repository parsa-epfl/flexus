//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED

#include <boost/lexical_cast.hpp>
#include <core/debug/category.hpp>
#include <core/debug/entry.hpp>

namespace Flexus {
namespace Dbg {

struct Filter
{
    enum MatchResult
    {
        Include,
        Exclude,
        NoMatch
    };

    virtual MatchResult match(Entry const& anEntry)                                       = 0;
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent) = 0;
    virtual ~Filter() {}
};

struct CompoundFilter : public Filter
{
  protected:
    std::vector<Filter*> theFilters;

  public:
    void destruct();
    virtual ~CompoundFilter()
    {
        destruct(); // Clean up all pointers owned by theActions
    }

    // Returns Exclude if any filter returns exclude.
    // Returns Include if at least one filter returns Include and none return
    // Exclude Otherwise, returns NoMatch
    virtual MatchResult match(Entry const& anEntry);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
    void add(Filter* aFilter);
};

struct IncludeFilter : public CompoundFilter
{
    // returns Include if all filters return Include, otherwise, returns NoMatch
    virtual MatchResult match(Entry const& anEntry);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
};

struct ExcludeFilter : public CompoundFilter
{
    // returns Exclude if all filters return Include, otherwise, returns NoMatch
    virtual MatchResult match(Entry const& anEntry);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
};

struct SimpleFilter : public Filter
{
  protected:
    std::string theField;

  public:
    SimpleFilter(std::string const& aField)
      : theField(aField)
    {
    }

    virtual ~SimpleFilter() {}
};

template<class ValueType, class Operation>
class OpFilter : public SimpleFilter
{
    typename ValueType::type theValue;

  public:
    OpFilter(std::string const& aField, typename ValueType::type const& aValue)
      : SimpleFilter(aField)
      , theValue(aValue)
    {
    }

    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent)
    {
        anOstream << anIndent << '{' << theField << "} " << Operation().toString() << ' '
                  << ValueType().toString(theValue) << "\n";
    };

    virtual MatchResult match(Entry const& anEntry)
    {
        Operation op;
        if (op(ValueType().extract(anEntry, theField), theValue)) {
            return Include;
        } else {
            return NoMatch;
        }
    }
};

struct String
{
    typedef std::string type;
    std::string extract(Entry const& anEntry, std::string const& aField) { return anEntry.get(aField); }
    std::string toString(std::string const& aValue) { return std::string("\"") + aValue + '"'; }
};
struct LongLong
{
    typedef int64_t type;
    int64_t extract(Entry const& anEntry, std::string const& aField) { return anEntry.getNumeric(aField); }
    std::string toString(int64_t const& aValue) { return std::to_string(aValue); }
};

struct OpEqual
{
    std::string const& toString()
    {
        static std::string str("==");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs == rhs;
    }
};

struct OpNotEqual
{
    std::string const& toString()
    {
        static std::string str("!=");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs != rhs;
    }
};

struct OpLessThan
{
    std::string const& toString()
    {
        static std::string str("<");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs < rhs;
    }
};

struct OpLessEqual
{
    std::string const& toString()
    {
        static std::string str("<=");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs <= rhs;
    }
};

struct OpGreaterThan
{
    std::string const& toString()
    {
        static std::string str(">");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs > rhs;
    }
};

struct OpGreaterEqual
{
    std::string const& toString()
    {
        static std::string str(">=");
        return str;
    }
    template<class T>
    bool operator()(T const& lhs, T const& rhs)
    {
        return lhs >= rhs;
    }
};

struct ExistsFilter : public SimpleFilter
{
    ExistsFilter(std::string const& aField);
    virtual MatchResult match(Entry const& anEntry);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
};

class CategoryFilter : public Filter
{
    Category const* theCategory;

  public:
    CategoryFilter(std::string const& aCategory);
    virtual MatchResult match(Entry const& anEntry);
    virtual void printConfiguration(std::ostream& anOstream, std::string const& anIndent);
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_FILTER_HPP_INCLUDED
