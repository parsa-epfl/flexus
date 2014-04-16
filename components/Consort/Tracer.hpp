namespace nConsort {

using Flexus::SharedTypes::PhysicalMemoryAddress;

typedef PhysicalMemoryAddress MemoryAddress;
typedef uint64_t Time;

struct TraceManager {
  virtual void start() = 0;
  virtual void finish() = 0;
  virtual void upgrade(uint32_t producer, MemoryAddress address) = 0;
  virtual void consumption(uint32_t consumer, MemoryAddress address) = 0;
  virtual ~TraceManager() {}
};

extern TraceManager * theTraceManager;

} //End Namespace nConsort

