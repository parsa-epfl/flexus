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
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <core/debug/category.hpp>
#include <core/debug/entry.hpp>
#include <core/debug/field.hpp>
#include <core/debug/severity.hpp>
#include <core/debug/target.hpp>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#ifndef FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED

namespace Flexus {
namespace Dbg {

struct At
{
    int64_t theCycle;
    Action* theAction;

    At(int64_t aCycle, Action* anAction)
      : theCycle(aCycle)
      , theAction(anAction)
    {
    }

    friend bool operator<(At const& aLeft, At const& aRight) { return aLeft.theCycle > aRight.theCycle; }
};

class Debugger
{
    std::vector<Target*> theTargets; // Owns all targets
    std::map<std::string, bool*> theCategories;
    std::map<std::string, std::vector<bool*>> theComponents;

    int64_t theCount;
    uint64_t* theCycleCount;

    std::priority_queue<At> theAts; // Owns all targets

  public:
    static Debugger* theDebugger;
    Severity theMinimumSeverity;

    Debugger()
      : theCount(0)
      , theCycleCount(0)
      , theMinimumSeverity(SevDev)
    {
    }

    ~Debugger();
    void initialize();
    void addFile(std::string const&);
    static void constructDebugger();

    int64_t count() { return ++theCount; }

    int64_t cycleCount()
    {
        if (theCycleCount)
            return *theCycleCount;
        else
            return 0;
    }

    void connectCycleCount(uint64_t* aCount) { theCycleCount = aCount; }

    void process(Entry const& anEntry);
    void printConfiguration(std::ostream& anOstream);
    void add(Target* aTarget);
    void registerCategory(std::string const& aCategory, bool* aSwitch);
    bool setAllCategories(bool aValue);
    bool setCategory(std::string const& aCategory, bool aValue);
    void listCategories(std::ostream& anOstream);
    void registerComponent(std::string const& aComponent, uint32_t anIndex, bool* aSwitch);
    bool setAllComponents(int32_t anIndex, bool aValue);
    bool setComponent(std::string const& aCategory, int32_t anIndex, bool aValue);
    void listComponents(std::ostream& anOstream);
    void addAt(int64_t aCycle, Action* anAction);
    void checkAt();
    void doNextAt();
    void reset();

    void setMinSev(Severity aSeverity) { theMinimumSeverity = aSeverity; }
};

struct DebuggerConstructor
{
    DebuggerConstructor() { Debugger::constructDebugger(); }
};

} // namespace Dbg
} // namespace Flexus

namespace {
Flexus::Dbg::DebuggerConstructor construct_debugger;
}

#endif // FLEXUS_CORE_DEBUG_DEBUGGER_HPP_INCLUDED
