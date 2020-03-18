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
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#include <functional>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <core/stats.hpp>

namespace Flexus {
namespace Core {
void Break() {
}
} // namespace Core
} // namespace Flexus

using namespace Flexus::Stat;

std::deque<std::function<void()>> theCommands;

void usage() {
  std::cout << "Usage: stat-sample <output stat file> <input stat files>* " << std::endl;
}

void help(std::string const &command) {
  usage();
}

void loadDatabaseFile(std::istream &anIstream, std::string const &aPrefix, bool aFirst) {
  if (aFirst) {
    getStatManager()->load(anIstream);
  } else {
    getStatManager()->loadMore(anIstream, aPrefix);
  }
}

void loadDatabase(std::string const &aName, std::string const &aPrefix, bool aFirst) {
  size_t loc = aName.rfind(".gz");

  try {
    std::ifstream in(aName.c_str(), std::ios::binary);
    if (in) {
      if (loc == std::string::npos) {
        loadDatabaseFile(in, aPrefix, aFirst);
        in.close();
      } else {
        boost::iostreams::filtering_istream inGzip;
        inGzip.push(boost::iostreams::gzip_decompressor());
        inGzip.push(in);
        loadDatabaseFile(inGzip, aPrefix, aFirst);
        inGzip.reset();
      }
    } else {
      std::cout << "Cannot open stats database " << aName << std::endl;
      std::exit(-1);
    }
  } catch (...) {
    std::cout << "Unable to load stats from database " << aName << std::endl;
    std::exit(-1);
  }
}

void reduceSum() {
  getStatManager()->reduce(eReduction::eSum, ".*selection", "sum", std::cout);
}

void reduceAvg() {
  getStatManager()->reduce(eReduction::eAverage, ".*selection", "avg", std::cout);
}

void reduceStdev(std::string const &aMsmt) {
  getStatManager()->reduce(eReduction::eStdDev, ".*selection", aMsmt, std::cout);
}

void reduceCount() {
  getStatManager()->reduce(eReduction::eCount, ".*selection", "count", std::cout);
}

void reduceNodes() {
  getStatManager()->reduceNodes(".*selection");
}

void save(std::string const &aFilename, std::string const &aMeasurementRestriction) {
  getStatManager()->saveMeasurements(aMeasurementRestriction, aFilename);
}

std::string region_string;

void processCmdLine(int32_t aCount, char **anArgList) {

  if (aCount < 2) {
    usage();
    std::exit(-1);
  }

  std::string output_file = anArgList[1];
  std::string first_file = anArgList[2];

  theCommands.push_back([&first_file]() {
    return loadDatabase(first_file, "", true);
  }); // ll::bind( &loadDatabase, first_file, std::string(""), true ) );
  for (int32_t i = 3; i < aCount; ++i) {
    std::stringstream prefix;
    prefix << std::setw(2) << std::setfill('0') << (i - 1) << '-';
    theCommands.push_back([i, &anArgList, &prefix]() {
      return loadDatabase(std::string(anArgList[i]), prefix.str(), false);
    }); // ll::bind( &loadDatabase, std::string(anArgList[i]), prefix.str(),
        // false) );
  }
  theCommands.push_back([]() { return reduceSum(); }); // ll::bind( &reduceSum ) );
  theCommands.push_back([]() { return reduceAvg(); }); // ll::bind( &reduceAvg ) );
  theCommands.push_back([]() {
    return reduceStdev("pernode-stdev");
  }); // ll::bind( &reduceStdev, "pernode-stdev"  ) );
  theCommands.push_back([]() { return reduceCount(); }); // ll::bind( &reduceCount) );
  theCommands.push_back([]() { return reduceNodes(); }); // ll::bind( &reduceNodes ) );
  theCommands.push_back(
      []() { return reduceStdev("stdev"); }); // ll::bind( &reduceStdev, "stdev" ) );
  theCommands.push_back([&output_file]() {
    return save(output_file, "(sum|count|avg|stdev|pernode-stdev)");
  }); // ll::bind( &save, output_file, "(sum|count|avg|stdev|pernode-stdev)" )
      // );
}

int32_t main(int32_t argc, char **argv) {

  getStatManager()->initialize();

  processCmdLine(argc, argv);

  while (!theCommands.empty()) {
    theCommands.front()();
    theCommands.pop_front();
  }
}
