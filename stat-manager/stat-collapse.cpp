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
  std::cout << "Usage: stat-collapse <start msmt cycle> <end msmt cycle> "
               "<input stat file> <output stat file>"
            << std::endl;
}

void help(std::string const &command) {
  usage();
}

void loadDatabase(std::string const &aName) {
  size_t loc = aName.rfind(".gz");

  try {
    std::ifstream in(aName.c_str(), std::ios::binary);
    if (in) {
      if (loc == std::string::npos) {
        getStatManager()->load(in);
        in.close();
      } else {
        boost::iostreams::filtering_istream inGzip;
        inGzip.push(boost::iostreams::gzip_decompressor());
        inGzip.push(in);
        getStatManager()->load(inGzip);
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

void reduceSum(std::string const &aMeasurementRestriction) {
  getStatManager()->reduce(eReduction::eSum, aMeasurementRestriction, "selection", std::cout);
}

void save(std::string const &aFilename, std::string const &aMeasurementRestriction) {
  getStatManager()->saveMeasurements(aMeasurementRestriction, aFilename);
}

std::string region_string;

void computeRegions(int64_t aStartInsn, int64_t anEndInsn) {
  std::stringstream result;
  getStatManager()->format("Region 000", "{sys-cycles}", result);
  std::string out = result.str();
  // Strip trailing cr
  if (out[out.length() - 1] == '\n') {
    out = out.substr(0, out.length() - 1);
  }
  int32_t region = boost::lexical_cast<int64_t>(out);
  int32_t start_region = aStartInsn / region;
  int32_t end_region = anEndInsn / region;
  std::cout << "Start region: " << start_region << std::endl;
  std::cout << "End region: " << end_region << std::endl;
  std::stringstream region_strstream;
  region_strstream << "Region (";
  region_strstream << std::setw(3) << std::setfill('0') << start_region;
  for (++start_region; start_region <= end_region; ++start_region) {
    region_strstream << '|' << std::setw(3) << std::setfill('0') << start_region;
  }
  region_strstream << ')';
  region_string = region_strstream.str();

  StatAnnotation *regions = new StatAnnotation("reduce-SelectedRegions");
  (*regions) << region_string;
}

void processCmdLine(int32_t aCount, char **anArgList) {

  if (aCount < 4) {
    usage();
    std::exit(-1);
  }

  int64_t start_insn = boost::lexical_cast<int64_t>(anArgList[1]);
  int64_t end_insn = boost::lexical_cast<int64_t>(anArgList[2]);
  if (start_insn >= end_insn) {
    std::cout << "Start instruction must exceed end instruction\n";
    std::exit(-1);
  }
  std::string input_file = anArgList[3];
  std::string output_file = anArgList[4];

  theCommands.emplace_back([&input_file]() {
    return loadDatabase(input_file);
  }); // ll::bind( &loadDatabase, input_file ) );
  theCommands.emplace_back([&start_insn, &end_insn]() {
    return computeRegions(start_insn, end_insn);
  }); // ll::bind( &computeRegions, start_insn, end_insn ) );
  theCommands.emplace_back([]() {
    return reduceSum(region_string);
  }); // ll::bind( &reduceSum, ll::var(region_string) ) );
  theCommands.emplace_back([&output_file]() {
    return save(output_file, "selection");
  }); // ll::bind( &save, output_file, "selection" ) );
}

int32_t main(int32_t argc, char **argv) {

  getStatManager()->initialize();

  processCmdLine(argc, argv);

  while (!theCommands.empty()) {
    theCommands.front()();
    theCommands.pop_front();
  }
}
