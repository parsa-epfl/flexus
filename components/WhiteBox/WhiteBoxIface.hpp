#include <core/boost_extensions/intrusive_ptr.hpp>

#include <list>

namespace nWhiteBox {

struct CPUState {
  uint64_t theThread;
  uint64_t thePC;
  uint32_t theTrap;
  std::list<uint64_t> theBackTrace;
  std::string theExecName;
};

std::ostream & operator<< (std::ostream & anOstream, CPUState & entry);

struct WhiteBox : public boost::counted_base {
  virtual uint64_t get_thread_t( int32_t aCPU ) = 0;
  virtual uint64_t get_idle_thread_t( int32_t aCPU ) = 0;
  virtual int64_t getPID( int32_t aCPU) = 0;
  virtual int32_t getPendingTrap( int32_t aCPU ) = 0;
  virtual void printPIDS( ) = 0;
  virtual uint64_t pc(int32_t aCPU) = 0;
  virtual void fillBackTrace(int32_t aCPU, std::list<uint64_t> & aTrace) = 0;
  virtual bool getState(int32_t aCPU, CPUState & aState) = 0;

  virtual ~WhiteBox() {};
  static boost::intrusive_ptr<WhiteBox> getWhiteBox();
};

} //namespace nMagicBreak

