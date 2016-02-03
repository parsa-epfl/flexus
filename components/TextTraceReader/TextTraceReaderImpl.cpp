#include <components/TextTraceReader/TextTraceReader.hpp>

#include <core/types.hpp>
#include <core/stats.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>

#include <boost/bind.hpp>
#include <unordered_map>

#include <fstream>

#include <stdlib.h> // for random()

#define DBG_DefineCategories TextTraceReader
#define DBG_SetDefaultOps AddCat(TextTraceReader) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT TextTraceReader
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTextTraceReader {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(TextTraceReader) {
  FLEXUS_COMPONENT_IMPL( TextTraceReader );

  // some bookkeeping vars
  uint64_t     theNumRequests;
  uint64_t     theMaxReferences;

  std::ifstream fin;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(TextTraceReader)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void finalize( void ) {
    //theStats->update();
  }

  void initialize(void) {

    theNumRequests = 0;

    fin.open(cfg.TraceFile.c_str(), std::ifstream::in);

    DBG_Assert( fin.good() && fin.is_open(), ( << "Failed to open trace '" << cfg.TraceFile << "'" ));

    theMaxReferences = cfg.MaxReferences;

    DBG_( Dev, ( << "MaxReferences = " << theMaxReferences ));

  }

  void drive( interface::TextTraceReaderDrive const &) {

    DBG_Assert( fin.good() );

    MemoryMessage aMessage(MemoryMessage::NumMemoryMessageTypes);

    uint32_t addr, pc;
    int32_t core;
    bool priv;
    std::string type;
    bool success = true;

    fin >> std::hex >> addr;
    success &= !fin.eof();
    fin >> std::dec >> core;
    success &= !fin.eof();
    fin >> type;
    success &= !fin.eof();
    fin >> std::hex >> pc;
    success &= !fin.eof();
    fin >> std::boolalpha >> priv;

    if (!success) {
      DBG_(Crit, ( << "Reached EOF, terminating simulation") );
      Flexus::Core::theFlexus->terminateSimulation();
    }

    aMessage.address() = PhysicalMemoryAddress(addr);
    aMessage.coreIdx() = core;
    aMessage.pc() = VirtualMemoryAddress(pc);
    aMessage.priv() = priv;

    if (type == "R") {
      aMessage.type() = MemoryMessage::ReadReq;
    } else if (type == "F") {
      aMessage.type() = MemoryMessage::FetchReq;
    } else if (type == "W") {
      aMessage.type() = MemoryMessage::WriteReq;
    } else if (type == "U") {
      aMessage.type() = MemoryMessage::UpgradeReq;
    } else if (type == "EC") {
      aMessage.type() = MemoryMessage::EvictClean;
    } else if (type == "ED") {
      aMessage.type() = MemoryMessage::EvictDirty;
    } else if (type == "EW") {
      aMessage.type() = MemoryMessage::EvictWritable;
    } else {
      DBG_Assert(false, ( << "Unknown Message type " << type ));
    }

    if (aMessage.type() == MemoryMessage::FetchReq) {
      FLEXUS_CHANNEL_ARRAY( FetchRequest_Out, aMessage.coreIdx()) << aMessage;
    } else {
      FLEXUS_CHANNEL_ARRAY( Request_Out, aMessage.coreIdx()) << aMessage;
    }

    theNumRequests++;
    if (theMaxReferences > 0 && theNumRequests >= theMaxReferences) {
      DBG_(Crit, ( << "terminating simulation"));
      Flexus::Core::theFlexus->terminateSimulation();
    }
    if (((theNumRequests / 10000000) > 0 ) && (theNumRequests % 10000000 == 0))
      DBG_(Crit, ( << " req " <<  std::dec << theNumRequests));

    if (fin.eof() || fin.bad()) {
      DBG_(Crit, ( << "Reached EOF or bad file, terminating simulation"));
      Flexus::Core::theFlexus->terminateSimulation();
    }

  }

private:

  void saveState(std::string const & aDirName) { }

  void loadState( std::string const & aDirName ) { }

};  // end class TraceParsingComponent

}  // end Namespace nTraceParsingComponent

FLEXUS_COMPONENT_INSTANTIATOR( TextTraceReader, nTextTraceReader);

FLEXUS_PORT_ARRAY_WIDTH( TextTraceReader, Request_Out)      {
  return (cfg.CMPWidth);
}
FLEXUS_PORT_ARRAY_WIDTH( TextTraceReader, FetchRequest_Out) {
  return (cfg.CMPWidth);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TextTraceReader

#define DBG_Reset
#include DBG_Control()
