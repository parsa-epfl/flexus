#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>
#include <cstdlib>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace ll = boost::lambda;
using ll::_1;
using ll::_2;
using ll::_3;

#include <core/stats.hpp>

namespace Flexus {
namespace Core {
void Break() {}
} }

using namespace Flexus::Stat;

std::deque< boost::function< void () > > theCommands;

void usage() {
  std::cout << "Usage: stat-collapse <start msmt cycle> <end msmt cycle> <input stat file> <output stat file>" << std::endl;
}

void help(std::string const & command) {
  usage();
}

void loadDatabase( std::string const & aName) {
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

void reduceSum(std::string const & aMeasurementRestriction) {
  getStatManager()->reduce(eSum, aMeasurementRestriction, "selection", std::cout);
}

void save(std::string const & aFilename, std::string const & aMeasurementRestriction) {
  getStatManager()->saveMeasurements(aMeasurementRestriction, aFilename);
}

std::string region_string;

void computeRegions(int64_t aStartInsn, int64_t anEndInsn) {
  std::stringstream result;
  getStatManager()->format("Region 000", "{sys-cycles}", result);
  std::string out = result.str();
  //Strip trailing cr
  if (out[out.length()-1] == '\n') {
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
  for ( ++ start_region ; start_region <= end_region ; ++ start_region) {
    region_strstream << '|' << std::setw(3) << std::setfill('0') << start_region;
  }
  region_strstream << ')';
  region_string = region_strstream.str();

  StatAnnotation * regions = new StatAnnotation("reduce-SelectedRegions");
  (*regions) << region_string;
}

void processCmdLine(int32_t aCount, char ** anArgList) {

  if (aCount < 4) {
    usage();
    std::exit(-1);
  }

  int64_t start_insn = boost::lexical_cast<int64_t>( anArgList[1] );
  int64_t end_insn   = boost::lexical_cast<int64_t>( anArgList[2] );
  if (start_insn >= end_insn) {
    std::cout << "Start instruction must exceed end instruction\n";
    std::exit(-1);
  }
  std::string input_file = anArgList[3];
  std::string output_file = anArgList[4];

  theCommands.push_back(  ll::bind( &loadDatabase, input_file ) );
  theCommands.push_back(  ll::bind( &computeRegions, start_insn, end_insn ) );
  theCommands.push_back(  ll::bind( &reduceSum, ll::var(region_string) ) );
  theCommands.push_back(  ll::bind( &save, output_file, "selection" ) );

}

int32_t main(int32_t argc, char ** argv) {

  getStatManager()->initialize();

  processCmdLine(argc, argv);

  while (! theCommands.empty() ) {
    theCommands.front()();
    theCommands.pop_front();
  }

}
