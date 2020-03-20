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

#include <components/MTManager/MTManagerComponent.hpp>

#define DBG_DefineCategories MTMgr
#define DBG_SetDefaultOps AddCat(MTMgr)
#include DBG_Control()

#include <core/flexus.hpp>
#include <core/stats.hpp>

#define FLEXUS_BEGIN_COMPONENT MTManagerComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/MTManager/MTManager.hpp>

namespace nMTManager {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

struct ScheduleState {
  uint32_t theRoundRobinPtr;
  uint32_t theThreadThisCycle;
  uint64_t theLastScheduledCycle;
  ScheduleState() : theRoundRobinPtr(0), theThreadThisCycle(0), theLastScheduledCycle(0) {
  }
};

MTManager *theMTManager = 0;

class FLEXUS_COMPONENT(MTManagerComponent), public MTManager {
  FLEXUS_COMPONENT_IMPL(MTManagerComponent);

  std::vector<ScheduleState> theFAGSchedules;
  std::vector<ScheduleState> theFSchedules;
  std::vector<ScheduleState> theDSchedules;
  std::vector<ScheduleState> theEXSchedules;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MTManagerComponent) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
    theMTManager = this;
  }

  bool isQuiesced() const {
    return true;
  }

  void initialize() {
    theFAGSchedules.resize(cfg.Cores);
    theFSchedules.resize(cfg.Cores);
    theDSchedules.resize(cfg.Cores);
    theEXSchedules.resize(cfg.Cores);
  }

  void finalize() {
  }

  int32_t getICount(int32_t aCore, int32_t aThread) {
    int32_t sum = 0;
    int32_t count = 0;
    FLEXUS_CHANNEL_ARRAY(FAQ_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    FLEXUS_CHANNEL_ARRAY(FIQ_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    FLEXUS_CHANNEL_ARRAY(ROB_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    return sum;
  }

  bool isFAGStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(FAGStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isFStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(FStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isDStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(DStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isEXStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(EXStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  void scheduleICount(int32_t aCore, ScheduleState & aSchedule,
                      bool (nMTManager::MTManagerComponentComponent::*isStalled)(int, int)) {
    std::vector<uint32_t> icount(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      icount[i] = getICount(aCore, i);
    }
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    uint32_t min_icount = static_cast<uint32_t>(-1);
    int32_t td = selected_td;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      DBG_(Iface, (<< "   Td[" << td << "] icount: " << icount[td] << " stalled? "
                   << (this->*isStalled)(aCore, td)));
      if (icount[td] < min_icount && !(this->*isStalled)(aCore, td)) {
        selected_td = td;
        min_icount = icount[td];
      }
      td = (td + 1) % cfg.Threads;
    }
    aSchedule.theThreadThisCycle = selected_td;
    DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleExecuteRoundRobin(int32_t aCore, ScheduleState & aSchedule) {
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (isEXStalled(aCore, selected_td)) {
        DBG_(Iface, (<< "   Td[" << selected_td << "] stalled"));
        selected_td = (selected_td + 1) % cfg.Threads;
      } else {
        DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
        break;
      }
    }
    aSchedule.theThreadThisCycle = selected_td;
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleDispatchRoundRobin(int32_t aCore, ScheduleState & aSchedule) {
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (isDStalled(aCore, selected_td)) {
        DBG_(Iface, (<< "   Td[" << selected_td << "] stalled"));
        selected_td = (selected_td + 1) % cfg.Threads;
      } else {
        DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
        break;
      }
    }
    aSchedule.theThreadThisCycle = selected_td;
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleStrictRoundRobin(ScheduleState & aSchedule) {
    aSchedule.theThreadThisCycle = aSchedule.theRoundRobinPtr;
    aSchedule.theRoundRobinPtr++;
    if (aSchedule.theRoundRobinPtr == cfg.Threads) {
      aSchedule.theRoundRobinPtr = 0;
    }
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  uint32_t scheduleThread(std::vector<ScheduleState> & aSchedules, char const *aResource,
                          bool (nMTManager::MTManagerComponentComponent::*isStalled)(int, int),
                          uint32_t aCoreIndex) {
    DBG_Assert(aCoreIndex < cfg.Cores);
    ScheduleState &sched = aSchedules[aCoreIndex];
    if (cfg.FrontEndPolicy == kFE_ICount) {
      DBG_(Iface, (<< "Core[" << aCoreIndex << "] Schedule " << aResource << " via ICount "));
      scheduleICount(aCoreIndex, sched, isStalled);
    } else {
      DBG_(Iface, (<< "Core[" << aCoreIndex << "] Schedule " << aResource << " via RoundRobin "));
      scheduleStrictRoundRobin(sched);
    }
    DBG_(Iface,
         (<< "Core[" << aCoreIndex << "] " << aResource << ": " << sched.theThreadThisCycle));
    return sched.theThreadThisCycle;
  }

  uint32_t scheduleFAGThread(uint32_t aCoreIndex) {
    return scheduleThread(theFAGSchedules, "FAG",
                          &nMTManager::MTManagerComponentComponent::isFAGStalled, aCoreIndex);
  }
  uint32_t scheduleFThread(uint32_t aCoreIndex) {
    return scheduleThread(theFSchedules, "F", &nMTManager::MTManagerComponentComponent::isFStalled,
                          aCoreIndex);
  }

  bool runThisEX(uint32_t anIndex) {
    uint32_t core_index = anIndex / cfg.Threads;
    uint32_t thread_index = anIndex % cfg.Threads;
    DBG_Assert(core_index < cfg.Cores);
    ScheduleState &sched = theEXSchedules[core_index];

    if (sched.theLastScheduledCycle != Flexus::Core::theFlexus->cycleCount()) {
      if (cfg.BackEndPolicy == kBE_SmartRoundRobin) {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule EX via SmartRoundRobin"));
        scheduleExecuteRoundRobin(core_index, sched);
      } else {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule EX via StrictRoundRobin"));
        scheduleStrictRoundRobin(sched);
      }
      DBG_(Verb, (<< "Core[" << core_index << "] EX: " << sched.theThreadThisCycle));
    }
    return sched.theThreadThisCycle == thread_index;
  }

  bool runThisD(uint32_t anIndex) {
    uint32_t core_index = anIndex / cfg.Threads;
    uint32_t thread_index = anIndex % cfg.Threads;
    DBG_Assert(core_index < cfg.Cores);
    ScheduleState &sched = theDSchedules[core_index];

    if (sched.theLastScheduledCycle != Flexus::Core::theFlexus->cycleCount()) {
      if (cfg.BackEndPolicy == kBE_SmartRoundRobin) {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule D via SmartRoundRobin"));
        // scheduleThread(  theDSchedules, "D",
        // &nMTManager::MTManagerComponentComponent::isDStalled, core_index);
        scheduleDispatchRoundRobin(core_index, sched);
      } else {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule D via StrictRoundRobin"));
        scheduleStrictRoundRobin(sched);
      }
    }
    return sched.theThreadThisCycle == thread_index;
  }
};

MTManager *MTManager::get() {
  return theMTManager;
}

} // end namespace nMTManager

FLEXUS_COMPONENT_INSTANTIATOR(MTManagerComponent, nMTManager);

FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, EXStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, FAGStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, FStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, DStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, FAQ_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, FIQ_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH(MTManagerComponent, ROB_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MTManagerComponent

#define DBG_Reset
#include DBG_Control()
