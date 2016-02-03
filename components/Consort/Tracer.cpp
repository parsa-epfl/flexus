#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <core/boost_extensions/lexical_cast.hpp>
#include <core/simics/configuration_api.hpp>

#include <core/flexus.hpp>

#include "Tracer.hpp"

#include <zlib.h>

namespace nConsort  {

using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
using Flexus::SharedTypes::PhysicalMemoryAddress;

typedef PhysicalMemoryAddress MemoryAddress;
typedef uint64_t Time;

struct TraceManagerImpl : public TraceManager {

  static const int32_t kNumProcs = 16;
  static const int32_t kBlockSize = 64;

  // output files
  struct OutfileRecord {
    std::string theBaseName;
    gzFile * theFiles;
    int32_t      theFileNo[kNumProcs];
    int64_t     theFileLengths[kNumProcs];

    static const int32_t K = 1024;
    static const int32_t M = 1024 * K;
    static const int32_t kMaxFileSize = 512 * M;

    OutfileRecord(char * aName)
      : theBaseName(aName)
      , theFiles(0) {
      int32_t ii;
      for (ii = 0; ii < kNumProcs; ii++) {
        theFileNo[ii] = 0;
        theFileLengths[ii] = 0;
      }
    }
    ~OutfileRecord() {
      if (theFiles != 0) {
        int32_t ii;
        for (ii = 0; ii < kNumProcs; ii++) {
          gzclose(theFiles[ii]);
        }
        delete [] theFiles;
      }
    }

    void init() {
      int32_t ii;
      theFiles = new gzFile[kNumProcs];
      for (ii = 0; ii < kNumProcs; ii++) {
        theFiles[ii] = gzopen( nextFile(ii).c_str(), "w" );
      }
    }
    void flush() {
      int32_t ii;
      for (ii = 0; ii < kNumProcs; ii++) {
        switchFile(ii);
      }
    }
    std::string nextFile(int32_t aCpu) {
      return ( theBaseName + std::to_string(aCpu) + ".sordtrace64." + std::to_string(theFileNo[aCpu]++) + ".gz" );
    }
    void switchFile(int32_t aCpu) {
      DBG_Assert(theFiles);
      gzclose(theFiles[aCpu]);
      std::string new_file = nextFile(aCpu);
      theFiles[aCpu] = gzopen( new_file.c_str(), "w" );
      theFileLengths[aCpu] = 0;
      DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    }
    void writeFile(uint8_t * aBuffer, int32_t aLength, int32_t aCpu) {
      DBG_Assert(theFiles);
      gzwrite(theFiles[aCpu], aBuffer, aLength);

      theFileLengths[aCpu] += aLength;
      if (theFileLengths[aCpu] > kMaxFileSize) {
        switchFile(aCpu);
      }
    }
  };

  OutfileRecord * Upgrades;
  OutfileRecord * Consumptions;

  // file I/O buffer
  uint8_t theBuffer[64];

  TraceManagerImpl(Flexus::Simics::API::conf_object_t * /*ignored*/ )
    : Upgrades(0)
    , Consumptions(0)
  { }

  void start() {
    DBG_(Dev, ( << "Begin tracing upgrades and consumptions") );
    Upgrades = new OutfileRecord("upgrade");
    Consumptions = new OutfileRecord("consumer");
    Upgrades->init();
    Consumptions->init();
  }

  void finish() {
    DBG_(Dev, ( << "Stop tracing upgrades and consumptions") );
    delete Upgrades;
    Upgrades = 0;
    delete Consumptions;
    Consumptions = 0;
  }

  void upgrade(uint32_t producer, MemoryAddress address) {
    if (! Upgrades) {
      return;
    }

    MemoryAddress pc(0);
    bool priv(false);
    Time time = currentTime();

    DBG_(VVerb, (  << "upgrade: node=" << producer
                   << " address=" << address
                   << " time=" << time
                ) );

    int32_t offset = 0;
    int32_t len;

    int64_t addr = address & ~63LL;
    len = sizeof(int64_t);
    memcpy(theBuffer + offset, &addr, len);
    offset += len;

    addr = pc;
    len = sizeof(int64_t);
    memcpy(theBuffer + offset, &addr, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer + offset, &time, len);
    offset += len;

    char priv_c = priv;
    len = sizeof(char);
    memcpy(theBuffer + offset, &priv_c, len);
    offset += len;

    Upgrades->writeFile(theBuffer, offset, producer);
  }

  void consumption(uint32_t consumer, MemoryAddress address) {
    if (! Consumptions) {
      return;
    }

    MemoryAddress pc(0);
    int32_t producer(0);
    uint32_t productionTime(0);
    bool priv(false);
    Time consumptionTime = currentTime();

    DBG_(VVerb, (  << "consumption: address=" << address
                   << " consumer=" << consumer
                   << " consumption time=" << consumptionTime
                   << " producer=" << producer
                   << " production time=" << productionTime
                ) );

    int32_t offset = 0;
    int32_t len;

    int64_t addr = address & ~63LL;
    len = sizeof(int64_t);
    memcpy(theBuffer + offset, &addr, len);
    offset += len;

    addr = pc;
    len = sizeof(int64_t);
    memcpy(theBuffer + offset, &addr, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer + offset, &consumptionTime, len);
    offset += len;

    char prod_c = producer;
    len = sizeof(char);
    memcpy(theBuffer + offset, &prod_c, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer + offset, &productionTime, len);
    offset += len;

    char priv_c = priv;
    len = sizeof(char);
    memcpy(theBuffer + offset, &priv_c, len);
    offset += len;

    Consumptions->writeFile(theBuffer, offset, consumer);

  }

  //--- Utility functions ------------------------------------------------------
  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(kBlockSize - 1) );
  }

  Time currentTime() {
    return theFlexus->cycleCount();
  }
};

class TraceManager_SimicsObject : public Flexus::Simics::AddInObject <TraceManagerImpl> {
  typedef Flexus::Simics::AddInObject<TraceManagerImpl> base;
public:
  static const Flexus::Simics::Persistence  class_persistence = Flexus::Simics::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "TraceManager";
  }
  static std::string classDescription() {
    return "TraceManager object";
  }

  TraceManager_SimicsObject() : base() { }
  TraceManager_SimicsObject(Flexus::Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
  TraceManager_SimicsObject(TraceManagerImpl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

    aClass.addCommand
    ( & TraceManagerImpl::start
      , "start"
      , "begin tracing"
    );

    aClass.addCommand
    ( & TraceManagerImpl::finish
      , "finish"
      , "stop tracing"
    );

  }

};

Flexus::Simics::Factory<TraceManager_SimicsObject> theTraceManagerFactory;

TraceManager_SimicsObject theRealTraceManager;

struct StaticInit {
  StaticInit() {
    std::cerr << "Creating MRC trace manager\n";
    theRealTraceManager = theTraceManagerFactory.create("trace-mgr");
    theTraceManager = & theRealTraceManager;
  }
} StaticInit_;

TraceManager * theTraceManager = 0;

} //End Namespace nConsort

