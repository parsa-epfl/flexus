#include "profile.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <boost/date_time/posix_time/posix_time.hpp>

namespace nProfile {

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

boost::posix_time::ptime last_reset(boost::posix_time::second_clock::local_time());

void ProfileManager::reset() {
  std::vector< Profiler *>::iterator iter, end;
  for (iter = theProfilers.begin(), end = theProfilers.end(); iter != end; ++iter) {
    (*iter)->reset();
  }

  last_reset = boost::posix_time::second_clock::local_time();
  theStartTime = rdtsc();
}

void ProfileManager::report(std::ostream & out) {
  std::vector< Profiler *>::iterator iter, end;
  int32_t i = 0;

  float program_time = programTime() / 100 ;

  out << "Ticks Since Reset: " << program_time << std::endl << std::endl;
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  out << "Wall Clock Since Reset: " << boost::posix_time::to_simple_string( boost::posix_time::time_period(last_reset, now).length() ) << std::endl << std::endl;

  out << "Worst sources, by Self Time \n";
  std::sort( theProfilers.begin(), theProfilers.end(), &sortSelfTime);

  out << std::setiosflags( std::ios::fixed) << std::setprecision(2);
  out << "File                          " << " ";
  out << "Function/Name                 " << " ";
  out << "Self Time  " << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200; ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + boost::lexical_cast<std::string>((*iter)->line());
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
    std::string file_line = (*iter)->file() + ":" + boost::lexical_cast<std::string>((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->totalTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->totalTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

}

} //nProfile
