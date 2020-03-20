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
#include <algorithm>

#include <core/boost_extensions/lexical_cast.hpp>

#include <core/debug/category.hpp>
#include <core/debug/entry.hpp>

#include <core/debug/filter.hpp>

namespace Flexus {
namespace Dbg {

typedef Filter::MatchResult MatchResult;

void CompoundFilter::destruct() {
  std::for_each(theFilters.begin(), theFilters.end(), [](Filter *aFilter) {
    delete aFilter;
  }); // Clean up all pointers owned by theActions
}

// Returns Exclude if any filter returns exclude.
// Returns Include if at least one filter returns Include and none return
// Exclude Otherwise, returns NoMatch
MatchResult CompoundFilter::match(Entry const &anEntry) {
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

void CompoundFilter::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  std::for_each(theFilters.begin(), theFilters.end(), [&anOstream, &anIndent](Filter *aFilter) {
    aFilter->printConfiguration(anOstream, anIndent);
  });
};

void CompoundFilter::add(Filter *aFilter) {
  theFilters.push_back(aFilter); // Ownership assumed by theFilters
}

MatchResult IncludeFilter::match(Entry const &anEntry) {
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

void IncludeFilter::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  std::vector<Filter *>::iterator iter(theFilters.begin());
  if (iter != theFilters.end()) {
    (*iter)->printConfiguration(anOstream, anIndent + "+ ");
    ++iter;
    std::for_each(iter, theFilters.end(), [&anOstream, &anIndent](Filter *aFilter) {
      return aFilter->printConfiguration(anOstream, anIndent + "& ");
    });
    // std::for_each(iter, theFilters.end(),
    // std::bind(&Filter::printConfiguration, std::_1, std::var(anOstream),
    // anIndent + "& ") );
  }
  anOstream << anIndent << ";\n";
};

MatchResult ExcludeFilter::match(Entry const &anEntry) {
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

void ExcludeFilter::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  std::vector<Filter *>::iterator iter(theFilters.begin());
  if (iter != theFilters.end()) {
    (*iter)->printConfiguration(anOstream, anIndent + "- ");
    ++iter;
    std::for_each(iter, theFilters.end(), [&anOstream, &anIndent](Filter *aFilter) {
      return aFilter->printConfiguration(anOstream, anIndent + "& ");
    });
    // std::for_each(iter, theFilters.end(),
    // std::bind(&Filter::printConfiguration, std::_1, std::var(anOstream),
    // anIndent + "& ") );
  }
  anOstream << anIndent << ";\n";
};

ExistsFilter::ExistsFilter(std::string const &aField) : SimpleFilter(aField) {
}

MatchResult ExistsFilter::match(Entry const &anEntry) {
  if (anEntry.exists(theField)) {
    return Include;
  } else {
    return NoMatch;
  }
}

void ExistsFilter::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  anOstream << anIndent << '{' << theField << "} exists\n";
};

CategoryFilter::CategoryFilter(std::string const &aCategory)
    : theCategory(&CategoryMgr::categoryMgr().category(aCategory)) {
}

MatchResult CategoryFilter::match(Entry const &anEntry) {
  if (anEntry.hasCategory(theCategory)) {
    return Include;
  } else {
    return NoMatch;
  }
}

void CategoryFilter::printConfiguration(std::ostream &anOstream, std::string const &anIndent) {
  anOstream << anIndent << theCategory->name() << "\n";
};

} // namespace Dbg
} // namespace Flexus
