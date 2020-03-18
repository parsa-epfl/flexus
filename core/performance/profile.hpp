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
#ifndef FLEXUS_PROFILE_HPP_INCLUDED
#define FLEXUS_PROFILE_HPP_INCLUDED

#include <string>
#include <vector>

#include <boost/preprocessor/cat.hpp>

namespace nProfile {

#ifndef X86_64
inline int64_t rdtsc() {
  int64_t tsc;
  __asm__ __volatile__("rdtsc" : "=A"(tsc));
  return tsc;
}
#else
inline int64_t rdtsc() {
  int64_t tsca, tscd;
  __asm__ __volatile__("rdtsc" : "=A"(tsca), "=D"(tscd));
  return (tscd << 32) | (tsca & 0xffffffff);
}
#endif

/*
inline int32_t prid() {
  int64_t id = 0;
  __asm__ __volatile__ ( "cpuid" : "=b" (id) : "a" (1) ) ;
  return id >> 31;
}
*/

class Timer;
class ManualTimer;
class Profiler;

class ProfileManager {
  std::vector<Profiler *> theProfilers;
  int64_t theStartTime;

public:
  ProfileManager() {
    theStartTime = rdtsc();
  }
  void addProfiler(Profiler *aProfiler) {
    theProfilers.push_back(aProfiler);
  }
  inline int64_t programTime() {
    return (rdtsc() - theStartTime) / 1000;
  }
  void report(std::ostream &);
  void reset();
  static ProfileManager *profileManager();
};

extern Profiler *theProfileTOS;

class Profiler {
  std::string theFn;
  std::string theFile;
  int64_t theLine;
  int64_t theTimeIn;
  int64_t theTimeAccum;
  int64_t theTimeAccumChildren;
  friend class Timer;
  friend class ManualTimer;

public:
  std::string const &name() const {
    return theFn;
  }
  std::string const &file() const {
    return theFile;
  }
  int64_t line() const {
    return theLine;
  }
  int64_t totalTime() const {
    return theTimeAccum / 1000;
  }
  int64_t selfTime() const {
    return (theTimeAccum - theTimeAccumChildren) / 1000;
  }

  Profiler(std::string const &aFn, std::string const &aFile, int64_t aLine)
      : theFn(aFn), theFile(aFile), theLine(aLine), theTimeIn(0), theTimeAccum(0),
        theTimeAccumChildren(0) {
    ProfileManager::profileManager()->addProfiler(this);
  }

  void reset() {
    theTimeIn = 0;
    theTimeAccum = 0;
    theTimeAccumChildren = 0;
  }
};

class Timer {
  Profiler &theProfiler;
  Profiler *theParent;

public:
  inline Timer(Profiler &aProfiler) : theProfiler(aProfiler) {
    theParent = theProfileTOS;
    theProfileTOS = &theProfiler;
    theProfiler.theTimeIn = rdtsc();
  }
  inline ~Timer() {
    if (theProfiler.theTimeIn != 0) {
      int64_t delta = rdtsc() - theProfiler.theTimeIn;
      if (delta > 0) {
        theProfiler.theTimeAccum += delta;
        if (theParent) {
          theParent->theTimeAccumChildren += delta;
        }
      }
    }
    theProfileTOS = theParent;
    theProfiler.theTimeIn = 0;
  }
};

class ManualTimer {
  Profiler &theProfiler;

public:
  inline ManualTimer(Profiler &aProfiler) : theProfiler(aProfiler) {
  }

  inline void start() {
    theProfiler.theTimeIn = rdtsc();
  }

  inline void stop() {
    if (theProfiler.theTimeIn != 0) {
      int64_t delta = rdtsc() - theProfiler.theTimeIn;
      if (delta > 0) {
        theProfiler.theTimeAccum += delta;
      }
    }
    theProfiler.theTimeIn = 0;
  }
};

#undef PROFILING_ENABLED
#ifdef PROFILING_ENABLED

#define FLEXUS_PROFILE()                                                                           \
  static nProfile::Profiler BOOST_PP_CAT(profiler, __LINE__)(__FUNCTION__, __FILE__, __LINE__);    \
  nProfile::Timer BOOST_PP_CAT(timer, __LINE__)(BOOST_PP_CAT(profiler, __LINE__)) /**/

#define FLEXUS_PROFILE_N(name)                                                                     \
  static nProfile::Profiler BOOST_PP_CAT(profiler, __LINE__)(name, __FILE__, __LINE__);            \
  nProfile::Timer BOOST_PP_CAT(timer, __LINE__)(BOOST_PP_CAT(profiler, __LINE__)) /**/

#else

#define FLEXUS_PROFILE() while (false)
#define FLEXUS_PROFILE_N(name) while (false)

#endif

} // namespace nProfile

#endif // FLEXUS_PROFILE_HPP_INCLUDED
