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
#include <core/debug/debugger.hpp>
#include <iomanip>
#include <iostream>
#include <map>
#include <utility>

namespace DBG_Cats {
bool Core_debug_enabled = true;
Flexus::Dbg::Category Core("Core", &Core_debug_enabled);
bool Assert_debug_enabled = true;
Flexus::Dbg::Category Assert("Assert", &Assert_debug_enabled);
bool Stats_debug_enabled = true;
Flexus::Dbg::Category Stats("Stats", &Stats_debug_enabled);
} // namespace DBG_Cats

namespace Flexus {
namespace Dbg {

Debugger::~Debugger() {
  for (auto *aTarget : theTargets) {
    delete aTarget;
  } // Clean up all pointers owned by theTargets
}

void Debugger::process(Entry const &anEntry) {
  for (auto *aTarget : theTargets) {
    aTarget->process(anEntry);
  }
}

void Debugger::printConfiguration(std::ostream &anOstream) {
  for (auto *aTarget : theTargets) {
    aTarget->printConfiguration(anOstream, std::string());
  }
}

void Debugger::add(Target *aTarget) { // Ownership assumed by Debugger
  theTargets.push_back(aTarget);      // Ownership assumed by theTargets
}

void Debugger::registerCategory(std::string const &aCategory, bool *aSwitch) {
  theCategories.insert(std::make_pair(aCategory, aSwitch));
}

bool Debugger::setAllCategories(bool aValue) {
  std::map<std::string, bool *>::iterator iter, end;
  iter = theCategories.begin();
  end = theCategories.end();
  while (iter != end) {
    if (iter->first != "Assert" && iter->second != 0) {
      *(iter->second) = aValue;
    }
    ++iter;
  }
  return true;
}

bool Debugger::setCategory(std::string const &aCategory, bool aValue) {
  if (aCategory == "all") {
    return setAllCategories(aValue);
  }
  std::map<std::string, bool *>::iterator iter;
  iter = theCategories.find(aCategory);
  if (iter == theCategories.end()) {
    return false; // no such category
  }
  *(iter->second) = aValue;
  return true;
}

void Debugger::listCategories(std::ostream &anOstream) {
  std::map<std::string, bool *>::iterator iter, end;
  iter = theCategories.begin();
  end = theCategories.end();
  while (iter != end) {
    if (iter->second != 0) {
      if (*iter->second) {
        anOstream << "   " << std::left << std::setw(40) << iter->first << "[enabled]\n";
      } else {
        anOstream << "   " << std::left << std::setw(40) << iter->first << "[disabled]\n";
      }
    }
    ++iter;
  }
}

void Debugger::registerComponent(std::string const &aComponent, uint32_t anIndex, bool *aSwitch) {
  std::map<std::string, std::vector<bool *>>::iterator iter;
  iter = theComponents.find(aComponent);
  if (iter == theComponents.end()) {
    bool ignored;
    std::tie(iter, ignored) =
        theComponents.insert(std::make_pair(aComponent, std::vector<bool *>(anIndex + 1, 0)));
  } else if (iter->second.size() < anIndex + 1) {
    iter->second.resize(anIndex + 1, 0);
  }
  (iter->second)[anIndex] = aSwitch;
}

bool Debugger::setAllComponents(int32_t anIndex, bool aValue) {
  std::map<std::string, std::vector<bool *>>::iterator iter, end;
  iter = theComponents.begin();
  end = theComponents.end();
  while (iter != end) {
    if (anIndex == -1) {
      for (unsigned i = 0; i < iter->second.size(); ++i) {
        *((iter->second)[i]) = aValue;
      }
    } else if (static_cast<unsigned>(anIndex) < iter->second.size()) {
      *((iter->second)[anIndex]) = aValue;
    } // Ignores out of raange indices
    ++iter;
  }
  return true;
}

bool Debugger::setComponent(std::string const &aCategory, int32_t anIndex, bool aValue) {
  if (aCategory == "all") {
    return setAllComponents(anIndex, aValue);
  }
  std::map<std::string, std::vector<bool *>>::iterator iter;
  iter = theComponents.find(aCategory);
  if (iter == theComponents.end()) {
    return false; // no such component
  }
  if (anIndex == -1) {
    for (unsigned i = 0; i < iter->second.size(); ++i) {
      *((iter->second)[i]) = aValue;
    }
  } else if (static_cast<unsigned>(anIndex) < iter->second.size()) {
    *((iter->second)[anIndex]) = aValue;
  } else {
    return false; // anIndex out of range
  }
  return true;
}

void Debugger::listComponents(std::ostream &anOstream) {
  std::map<std::string, std::vector<bool *>>::iterator iter, end;
  iter = theComponents.begin();
  end = theComponents.end();
  anOstream << "   " << std::left << std::setw(40) << "Component"
            << "0-------8-------F\n";
  while (iter != end) {
    if (iter->second.size() > 0) {
      anOstream << "   " << std::setw(40) << iter->first;
      for (unsigned i = 0; i < iter->second.size(); ++i) {
        if ((iter->second)[i] != 0) {
          if (*((iter->second)[i])) {
            anOstream << "X";
          } else {
            anOstream << ".";
          }
        }
      }
      anOstream << "\n";
    }
    ++iter;
  }
}

void Debugger::addAt(int64_t aCycle,
                     Action *anAction) { // Ownership assumed by Debugger
  theAts.push(At(aCycle, anAction));
}

void Debugger::checkAt() {
  if (!theAts.empty() && theAts.top().theCycle == cycleCount()) {
    doNextAt();
  }
}

void Debugger::doNextAt() {
  Entry entry = Entry(
      /*Severity*/ Flexus::Dbg::SevCrit, /*File*/ "", /*Line*/ 0,
      /*Function*/ "",
      /*GlobalCount*/ Flexus::Dbg::Debugger::theDebugger->count(),
      /*CycleCount*/ Flexus::Dbg::Debugger::theDebugger->cycleCount());
  theAts.top().theAction->process(entry);
  delete theAts.top().theAction;
  theAts.pop();
}

void Debugger::reset() {
  for (auto *aTarget : theTargets) {
    delete aTarget;
  }
  theTargets.clear();
}

Debugger *Debugger::theDebugger((Debugger *)0);

void Debugger::constructDebugger() {
  if (theDebugger == 0) {
    theDebugger = new Debugger();
    theDebugger->initialize();
  }
}

Category::Category(std::string const &aName, bool *aSwitch, bool anIsDynamic)
    : theName(aName), theIsDynamic(anIsDynamic) {
  theNumber = CategoryMgr::categoryMgr().addCategory(*this);
  if (!anIsDynamic) {
    Debugger::theDebugger->registerCategory(aName, aSwitch);
  }
}

std::string const &toString(Severity aSeverity) {
  static std::string theStaticSeverities[] = {"Invocations", "VeryVerbose", "Verbose",  "Interface",
                                              "Trace",       "Development", "Critical", "Temp"};
  return theStaticSeverities[aSeverity];
}

} // namespace Dbg
} // namespace Flexus
