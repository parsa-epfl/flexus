#define USE_CMU_TRACE_FORMAT

namespace nVirtutechTraceReader {

typedef uint32_t Time;
typedef int64_t Address;

struct TraceEntry {
  TraceEntry()
  {}
  TraceEntry(bool isWrite, Address anAddr, Address aPC, int32_t aNode, bool isPriv)
    : write(isWrite)
    , address(anAddr)
    , pc(aPC)
    , node(aNode)
    , priv(isPriv)
  {}

  bool write;         // true=>write  false=>read
  Address address;    // address of data reference
  Address pc;         // PC of instruction that caused reference
  int32_t node;           // node of read or write
  bool priv;          // privileged instruction?

  friend std::ostream & operator << (std::ostream & anOstream, const TraceEntry & anEntry) {
    if (anEntry.write) {
      anOstream << "write";
    } else {
      anOstream << "read ";
    }
    anOstream << " address=" << std::hex << anEntry.address
              << " pc=" << anEntry.pc
              << " node=" << anEntry.node;
    if (anEntry.priv) {
      anOstream << " privileged";
    } else {
      anOstream << " non-priv";
    }
    return anOstream;
  }

};

class VirtutechTraceReader {
  FILE * myTraceFile;

public:
  VirtutechTraceReader()
  {}

  void init() {
#ifdef USE_CMU_TRACE_FORMAT
    myTraceFile = fopen("tracedata", "r");
    DBG_Assert(myTraceFile, ( << "could not open file \"tracedata\"." ) );
#endif
  }

  void next(TraceEntry & entry) {
    // read until we find an appropriate trace entry
    char buf[256];

#ifdef USE_CMU_TRACE_FORMAT
    int32_t node;
    uint32_t pc;
    uint32_t addr;
    uint32_t data;
    int32_t write;
    int32_t pid;
    int32_t priv;

    if (fgets(buf, 256, myTraceFile) == 0) {
      // EOF
      DBG_(Crit, ( << "end of trace file" ) );
      throw Flexus::Core::FlexusException();
    }
    sscanf(buf, "%d %x %x %x %d %d %d", &node, &pc, &addr, &data, &write, &pid, &priv);

    entry.node = node;
    entry.pc = pc;
    entry.address = addr;
    entry.write = (bool)write;
    entry.priv = (bool)priv;
#endif  // USE_CMU_TRACE_FORMAT
  }

};  // end class VirtutechTraceReader

}  // end namespace nVirtutechTraceReader
