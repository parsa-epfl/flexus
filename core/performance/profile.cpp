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
#include "profile.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

// #ifndef __STDC_CONSTANT_MACROS
// #define __STDC_CONSTANT_MACROS
// #endif
// #include <boost/date_time/posix_time/posix_time.hpp>

namespace nProfile {

using namespace std::chrono;

ProfileManager *theProfileManager = 0;
Profiler *theProfileTOS = 0;

ProfileManager *ProfileManager::profileManager() {
  if (theProfileManager == 0) {
    theProfileManager = new ProfileManager();
  }
  return theProfileManager;
}

bool sortTotalTime(Profiler *left, Profiler *right) {
  return left->totalTime() > right->totalTime();
}

bool sortSelfTime(Profiler *left, Profiler *right) {
  return left->selfTime() > right->selfTime();
}

std::string rightmost(std::string const &aString, uint32_t N) {
  std::string retval(aString);
  if (retval.length() > N) {
    retval.erase(0, retval.length() - N);
  } else if (retval.length() < N) {
    retval.append(N - retval.length(), ' ');
  }
  return retval;
}

std::string leftmost(std::string const &aString, uint32_t N) {
  std::string retval(aString.substr(0, N));
  if (retval.length() < N) {
    retval.append(N - retval.length(), ' ');
  }
  return retval;
}

system_clock::time_point last_reset(system_clock::now());

void ProfileManager::reset() {
  std::vector<Profiler *>::iterator iter, end;
  for (iter = theProfilers.begin(), end = theProfilers.end(); iter != end; ++iter) {
    (*iter)->reset();
  }

  last_reset = system_clock::now();
  theStartTime = rdtsc();
}

void ProfileManager::report(std::ostream &out) {
  std::vector<Profiler *>::iterator iter, end;
  int32_t i = 0;

  float program_time = programTime() / 100;

  out << "Ticks Since Reset: " << program_time << std::endl << std::endl;
  system_clock::time_point now(system_clock::now());
  out << "Wall Clock Since Reset: " << duration_cast<microseconds>(now - last_reset).count() << "us"
      << std::endl
      << std::endl;

  out << "Worst sources, by Self Time \n";
  std::sort(theProfilers.begin(), theProfilers.end(), &sortSelfTime);

  out << std::setiosflags(std::ios::fixed) << std::setprecision(2);
  out << "File                          "
      << " ";
  out << "Function/Name                 "
      << " ";
  out << "Self Time  "
      << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200;
       ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + std::to_string((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->selfTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->selfTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

  out << "Worst sources, sorted by Total Time \n";
  std::sort(theProfilers.begin(), theProfilers.end(), &sortTotalTime);

  out << std::setiosflags(std::ios::fixed) << std::setprecision(2);
  out << "File                          "
      << " ";
  out << "Function/Name                 "
      << " ";
  out << "Total Time "
      << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200;
       ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + std::to_string((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->totalTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->totalTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";
}

} // namespace nProfile
