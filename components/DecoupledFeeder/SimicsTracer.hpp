#ifndef FLEXUS_DECOUPLED_FEEDER_SIMICS_TRACER_HPP_INCLUDED
#define FLEXUS_DECOUPLED_FEEDER_SIMICS_TRACER_HPP_INCLUDED

#include <components/Common/Slices/MemoryMessage.hpp>

namespace nDecoupledFeeder {

using Flexus::SharedTypes::MemoryMessage;

struct SimicsTracerManager {
  static SimicsTracerManager * construct(int32_t aNumCPUs
                                         , boost::function< void(int, MemoryMessage &) > toL1D
                                         , boost::function< void(int, MemoryMessage &, uint32_t) > toL1I
                                         , boost::function< void(MemoryMessage &) > toDMA
                                         , boost::function< void(int, MemoryMessage &) > toNAW
                                         , bool aWhiteBoxDebug
                                         , int32_t aWhiteBoxPeriod
                                         , bool aSendNonAllocatingStores
                                        );
  virtual ~SimicsTracerManager()  {}
  virtual void setSimicsQuantum(int64_t aQuantum)  = 0;
  virtual void enableInstructionTracing() = 0;
  virtual void setSystemTick(double aTickFreq) = 0;
  virtual void updateStats() = 0;
};

}

#endif //FLEXUS_DECOUPLED_FEEDER_SIMICS_TRACER_HPP_INCLUDED
