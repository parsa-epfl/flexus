#include <zlib.h>
#include <list>
#include <fstream>

#include "common.hpp"
#include "trace.hpp"

namespace trace {

struct FileRecord {
  std::string theBaseName;
  int32_t theNumL2s;
  std::vector<gzFile> theFiles;
  std::vector<int>    theFileNo;
  std::vector<long>   theFileLengths;

  bool theReadOnlyFlag;

  static const int32_t K = 1024;
  static const int32_t M = 1024 * K;
  static const int32_t kMaxFileSize = 512 * M;

  FileRecord(std::string const & aName, int32_t aNumL2s, const bool aReadOnlyFlag)
    : theBaseName(aName)
    , theNumL2s(aNumL2s)
    , theFiles(0)
    , theReadOnlyFlag(aReadOnlyFlag) {
    theFileNo.resize(theNumL2s, 0);
    theFileLengths.resize(theNumL2s, 0);
  }
  ~FileRecord() {
    for (unsigned ii = 0; ii < theFiles.size(); ii++) {
      gzclose(theFiles[ii]);
    }
  }

  void init() {
    theFiles.resize(theNumL2s, 0);
    for (int32_t ii = 0; ii < theNumL2s; ii++) {
      theFiles[ii] = gzopen( nextFile(ii).c_str(), (theReadOnlyFlag ? "r" : "w") );
    }
  }
  void flush() {
    for (int32_t ii = 0; ii < theNumL2s; ii++) {
      switchFile(ii);
    }
  }
  std::string nextFile(int32_t aCpu) {
    return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".trace." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
  }
  gzFile switchFile(int32_t aCpu) {
    gzclose(theFiles[aCpu]);
    std::string new_file = nextFile(aCpu);
    theFiles[aCpu] = gzopen( new_file.c_str(), (theReadOnlyFlag ? "r" : "w") );
    theFileLengths[aCpu] = 0;
    DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    return theFiles[aCpu];
  }
  int32_t readFile(uint8_t * aBuffer, int32_t aLength, int32_t aCpu) {
    int32_t ret = gzread(theFiles[aCpu], aBuffer, aLength);
    if (ret == 0) {
      if (switchFile(aCpu) == NULL) {
        return 0;
      }
      ret = gzread(theFiles[aCpu], aBuffer, aLength);
    }
    return ret;
  }
  void writeFile(uint8_t const * aBuffer, int32_t aLength, int32_t aCpu) {
    gzwrite(theFiles[aCpu], const_cast<uint8_t *>(aBuffer), aLength);

    theFileLengths[aCpu] += aLength;
    if (theFileLengths[aCpu] > kMaxFileSize) {
      switchFile(aCpu);
    }
  }
  std::string getName( int32_t aCpu ) {
    return (theBaseName + boost::lexical_cast<std::string>(aCpu) + ".trace." + boost::lexical_cast<std::string>(theFileNo[aCpu]) + ".gz" );
  }
};

FileRecord * theL2Accesses = 0;
FileRecord * theOffChipAccesses = 0;

uint8_t theBuffer[256];

Flexus::Stat::StatCounter statL2Accesses("sys-trace-L2Accesses");
Flexus::Stat::StatCounter statOCAccesses("sys-trace-OffChipAccesses");

void initialize( std::string const & aPath, const bool aReadOnlyFlag ) {
  theL2Accesses = new FileRecord( aPath + "/L2_trace", 1, aReadOnlyFlag);
  theL2Accesses->init();
  theOffChipAccesses = new FileRecord( aPath + "/OC_trace", 1, aReadOnlyFlag);
  theOffChipAccesses->init();
}

void flushFiles() {
  DBG_(Dev, ( << "Switching all output files" ) );
  if (theL2Accesses) {
    theL2Accesses->flush();
  }
  if (theOffChipAccesses) {
    theOffChipAccesses->flush();
  }
}

void addL2Access(TraceData const & theRecord) {
  DBG_(Verb, ( << theRecord ));

  ++statL2Accesses;

  DBG_Assert( theRecord.theFillLevel <= Flexus::SharedTypes::eCore);
  DBG_Assert( theRecord.theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord.theNode << "] 0x" << std::hex << theRecord.theAddress
              << " PC:" << theRecord.thePC << std::dec << ( theRecord.theOS ? "s " : "u " ) << theRecord.theFillLevel ));

  theL2Accesses->writeFile(reinterpret_cast<const uint8_t *>(&theRecord), sizeof(TraceData), 0);
}

void addOCAccess(TraceData const & theRecord) {
  DBG_(Verb, ( << theRecord ));

  ++statOCAccesses;

  DBG_Assert( theRecord.theFillLevel <= Flexus::SharedTypes::eCore);
  DBG_Assert( theRecord.theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord.theNode << "] 0x" << std::hex << theRecord.theAddress
              << " PC:" << theRecord.thePC << std::dec << ( theRecord.theOS ? "s " : "u " ) << theRecord.theFillLevel ));

  theOffChipAccesses->writeFile(reinterpret_cast<const uint8_t *>(&theRecord), sizeof(TraceData), 0);
}

int32_t getL2Access( TraceData * theRecord) {
  int32_t ret = theL2Accesses->readFile(reinterpret_cast<uint8_t *>(theRecord), sizeof(TraceData), 0);

  if (ret != 0) {
    DBG_(Verb, ( << *theRecord ));

    ++statL2Accesses;

    DBG_Assert( theRecord->theFillLevel <= Flexus::SharedTypes::eCore, ( << *theRecord) );
    DBG_Assert( theRecord->theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord->theNode << "] 0x" << std::hex << theRecord->theAddress
                << " PC:" << theRecord->thePC << std::dec << ( theRecord->theOS ? " s " : " u " ) << theRecord->theFillLevel ));
  }
  return ret;
}

int32_t getOCAccess( TraceData * theRecord ) {
  int32_t ret = theOffChipAccesses->readFile(reinterpret_cast<uint8_t *>(theRecord), sizeof(TraceData), 0);

  if (ret != 0) {
    DBG_(Verb, ( << *theRecord ));

    ++statOCAccesses;

    DBG_Assert( theRecord->theFillLevel <= Flexus::SharedTypes::eCore );
    DBG_Assert( theRecord->theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord->theNode << "] 0x" << std::hex << theRecord->theAddress
                << " PC:" << theRecord->thePC << std::dec << ( theRecord->theOS ? "s " : "u " ) << theRecord->theFillLevel ));
  }
  return ret;
}

} //end namespace trace

