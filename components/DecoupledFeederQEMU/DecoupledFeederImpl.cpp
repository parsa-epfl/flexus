#include <components/DecoupledFeederQEMU/DecoupledFeeder.hpp>

#include <components/DecoupledFeederQEMU/QemuTracer.hpp>
#include <core/qemu/api_wrappers.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#define DBG_DefineCategories Feeder
#define DBG_SetDefaultOps AddCat(Feeder)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDecoupledFeeder {

using namespace Flexus;
using namespace Flexus::Qemu;

class FLEXUS_COMPONENT(DecoupledFeeder) {
  FLEXUS_COMPONENT_IMPL( DecoupledFeeder );

  //The Qemu objects (one for each processor) for getting trace data
  int32_t theNumCPUs;
  int32_t theCMPWidth;
  QemuTracerManager * theTracer;
  int64_t * theLastICounts;
  Stat::StatCounter ** theICounts;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    theTracer = QemuTracerManager::construct(theNumCPUs
                , boost::bind( &DecoupledFeederComponent::toL1D, this, _1, _2)
                , boost::bind( &DecoupledFeederComponent::toL1I, this, _1, _2, _3)
                , boost::bind( &DecoupledFeederComponent::toDMA, this, _1)
                , boost::bind( &DecoupledFeederComponent::toNAW, this, _1, _2)
                //, cfg.WhiteBoxDebug
                //, cfg.WhiteBoxPeriod
                , cfg.SendNonAllocatingStores
			  );

  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    //Disable cycle-callback
    //Flexus::Qemu::theQemuInterface->disableCycleHook();

    //theTracer->setQemuQuantum(cfg.QemuQuantum);
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
	  Qemu::API::conf_object_t *cpu = Qemu::API::QEMU_get_cpu_by_index(i);
	  theLastICounts[i] = Qemu::API::QEMU_step_count(cpu);
    }

	//TODO fix this with actual QEMU_insert_callback.
    //thePeriodicHap = new periodic_hap_t(this, cfg.HousekeepingPeriod);
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

  void toDMA(MemoryMessage & aMessage) {
    FLEXUS_CHANNEL( ToDMA ) << aMessage;
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
	  Qemu::API::conf_object_t *curr_cpu = Qemu::API::QEMU_get_cpu_by_index(i);
	  int64_t temp = Qemu::API::QEMU_step_count(curr_cpu);
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

  void OnPeriodicEvent(Qemu::API::conf_object_t * ignored, long long aPeriod) {
    doHousekeeping();
  }

  //typedef Qemu::HapToMemFnBinding<Qemu::HAPs::Core_Periodic_Event, self, &self::OnPeriodicEvent> periodic_hap_t;
  //periodic_hap_t * thePeriodicHap;

};  // end class DecoupledFeeder

}  // end Namespace nDecoupledFeeder

FLEXUS_COMPONENT_INSTANTIATOR( DecoupledFeeder, nDecoupledFeeder);
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1D ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1I ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToBPred ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToNAW ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder

#define DBG_Reset
#include DBG_Control()
