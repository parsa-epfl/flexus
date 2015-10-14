#ifndef FLEXUS_DECOUPLED_FEEDER_QEMU_TRACER_HPP_INCLUDED
#define FLEXUS_DECOUPLED_FEEDER_QEMU_TRACER_HPP_INCLUDED

#include <components/CommonQEMU/Slices/MemoryMessage.hpp>

namespace nDecoupledFeeder {

using Flexus::SharedTypes::MemoryMessage;

struct QemuTracerManager {
  static QemuTracerManager * construct(
				   int32_t aNumCPUs
				 , std::function< void(int, MemoryMessage &) > toL1D
				 , std::function< void(int, MemoryMessage &, uint32_t) > toL1I
				 , std::function< void(MemoryMessage &) > toDMA
				 , std::function< void(int, MemoryMessage &) > toNAW
//				 , bool aWhiteBoxDebug
//				 , int32_t aWhiteBoxPeriod
				 , bool aSendNonAllocatingStores
				);
  virtual ~QemuTracerManager()  {}
  virtual void setQemuQuantum(int64_t aQuantum)  = 0;
  virtual void enableInstructionTracing() = 0;
  virtual void setSystemTick(double aTickFreq) = 0;
  virtual void updateStats() = 0;
};

}

#endif //FLEXUS_DECOUPLED_FEEDER_QEMU_TRACER_HPP_INCLUDED
