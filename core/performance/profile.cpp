#include "profile.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

#include <boost/lexical_cast.hpp>

// #ifndef __STDC_CONSTANT_MACROS
// #define __STDC_CONSTANT_MACROS
// #endif
// #include <boost/date_time/posix_time/posix_time.hpp>

namespace nProfile {

using namespace std::chrono;

ProfileManager * theProfileManager = 0;
Profiler * theProfileTOS = 0;

ProfileManager * ProfileManager::profileManager() {
  if (theProfileManager == 0) {
    theProfileManager = new ProfileManager();
  }
  return theProfileManager;
}

bool sortTotalTime( Profiler * left, Profiler * right) {
  return left->totalTime() > right->totalTime();
}

bool sortSelfTime( Profiler * left, Profiler * right) {
  return left->selfTime() > right->selfTime();
}

std::string rightmost( std::string const & aString, uint32_t N) {
  std::string retval(aString);
  if (retval.length() > N) {
    retval.erase(0, retval.length() - N);
  } else if (retval.length() < N) {
    retval.append(N - retval.length(), ' ');
  }
  return retval;
}

std::string leftmost( std::string const & aString, uint32_t N) {
  std::string retval(aString.substr(0, N));
  if (retval.length() < N) {
    retval.append(N - retval.length(), ' ');
  }
  return retval;
}

system_clock::time_point last_reset(system_clock::now());

void ProfileManager::reset() {
  std::vector< Profiler *>::iterator iter, end;
  for (iter = theProfilers.begin(), end = theProfilers.end(); iter != end; ++iter) {
    (*iter)->reset();
  }

  last_reset = system_clock::now();
  theStartTime = rdtsc();
}

void ProfileManager::report(std::ostream & out) {
  std::vector< Profiler *>::iterator iter, end;
  int32_t i = 0;

  float program_time = programTime() / 100 ;

  out << "Ticks Since Reset: " << program_time << std::endl << std::endl;
  system_clock::time_point now(system_clock::now());
  out << "Wall Clock Since Reset: " << duration_cast<microseconds>(now - last_reset).count() << "us" << std::endl << std::endl;

  out << "Worst sources, by Self Time \n";
  std::sort( theProfilers.begin(), theProfilers.end(), &sortSelfTime);

  out << std::setiosflags( std::ios::fixed) << std::setprecision(2);
  out << "File                          " << " ";
  out << "Function/Name                 " << " ";
  out << "Self Time  " << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200; ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + std::to_string((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->selfTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->selfTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

  out << "Worst sources, sorted by Total Time \n";
  std::sort( theProfilers.begin(), theProfilers.end(), &sortTotalTime);

  out << std::setiosflags( std::ios::fixed) << std::setprecision(2);
  out << "File                          " << " ";
  out << "Function/Name                 " << " ";
  out << "Total Time " << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200; ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + std::to_string((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->totalTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->totalTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

}

} //nProfile
