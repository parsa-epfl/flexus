#include <components/DecoupledFeederV/DecoupledFeederV.hpp>

#include <components/DecoupledFeederV/SimicsTracer.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <core/simics/simics_interface.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/mai_api.hpp>

#include <functional>

#define DBG_DefineCategories Feeder
#define DBG_SetDefaultOps AddCat(Feeder)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT DecoupledFeederV
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDecoupledFeederV {

using namespace Flexus;

class FLEXUS_COMPONENT(DecoupledFeederV) {
  FLEXUS_COMPONENT_IMPL( DecoupledFeederV );

  //The Simics objects (one for each processor) for getting trace data
  int32_t theNumCPUs;
  int32_t theCMPWidth;
  SimicsTracerManager * theTracer;
  int64_t * theLastICounts;
  Stat::StatCounter ** theICounts;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeederV)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    theTracer = SimicsTracerManager::construct(theNumCPUs
                , std::bind( &DecoupledFeederVComponent::toL1D, this, _1, _2)
                , std::bind( &DecoupledFeederVComponent::toL1I, this, _1, _2, _3)
                , std::bind( &DecoupledFeederVComponent::toDMA, this, _1, _2)
                , std::bind( &DecoupledFeederVComponent::toNAW, this, _1, _2)
                                              );
  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    //Disable cycle-callback
    Flexus::Simics::theSimicsInterface->disableCycleHook();

    theTracer->setSimicsQuantum(cfg.SimicsQuantum);
    if (cfg.TrackIFetch) {
      theTracer->enableInstructionTracing();
    }

    if (cfg.SystemTickFrequency > 0.0) {
      theTracer->setSystemTick(cfg.SystemTickFrequency);
    }

    theLastICounts = new int64_t [theNumCPUs];
    theICounts = new Stat::StatCounter * [theNumCPUs];
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      theICounts[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-feeder-ICount" );
      theLastICounts[i] = Simics::Processor::getProcessor(i)->stepCount();
    }

    thePeriodicHap = new periodic_hap_t(this, cfg.HousekeepingPeriod);
    theFlexus->advanceCycles(0);

    theCMPWidth = cfg.CMPWidth;
    if (theCMPWidth == 0) {
      theCMPWidth = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    }
  }

  void finalize(void) {}

  std::pair< uint64_t, uint32_t> theFetchInfo;

  void toL1D(int32_t anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToL1D, anIndex ) << aMessage;
  }

  void toNAW(int32_t anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToNAW, anIndex / theCMPWidth ) << aMessage;
  }

  void toDMA(int32_t aVM, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToDMA, aVM ) << aMessage;
  }

  void toL1I(int32_t anIndex, MemoryMessage & aMessage, uint32_t anOpcode) {
    FLEXUS_CHANNEL_ARRAY( ToL1I, anIndex ) << aMessage;
    theFetchInfo.first = aMessage.pc();
    theFetchInfo.second = anOpcode;
    FLEXUS_CHANNEL_ARRAY( ToBPred, anIndex ) << theFetchInfo;
  }

  void updateInstructionCounts() {
    //Count instructions
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      int64_t temp = Simics::Processor::getProcessor(i)->stepCount() ;
      *(theICounts[i]) += temp - theLastICounts[i];
      theLastICounts[i] = temp;
    }
  }

  void doHousekeeping() {
    updateInstructionCounts();
    theTracer->updateStats();

    theFlexus->advanceCycles(cfg.HousekeepingPeriod);
    theFlexus->invokeDrives();
  }

  //void OnPeriodicEvent(Simics::API::conf_object_t * ignored, int64_t aPeriod) {
  void OnPeriodicEvent(Simics::API::conf_object_t * ignored, long long aPeriod) {
    doHousekeeping();
  }

  typedef Simics::HapToMemFnBinding<Simics::HAPs::Core_Periodic_Event, self, &self::OnPeriodicEvent> periodic_hap_t;
  periodic_hap_t * thePeriodicHap;

};  // end class DecoupledFeederV

}  // end Namespace nDecoupledFeederV

FLEXUS_COMPONENT_INSTANTIATOR( DecoupledFeederV, nDecoupledFeederV);
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeederV, ToL1D ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeederV, ToL1I ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeederV, ToBPred ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeederV, ToNAW ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeederV, ToDMA ) {
  return Flexus::Simics::ProcessorMapper::numVMs();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeederV

#define DBG_Reset
#include DBG_Control()
