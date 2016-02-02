#include <components/DecoupledFeederQEMU/DecoupledFeeder.hpp>

#include <components/DecoupledFeederQEMU/QemuTracer.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/qemu/api_wrappers.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <functional>

#define DBG_DefineCategories Feeder
#define DBG_SetDefaultOps AddCat(Feeder)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDecoupledFeeder {

using namespace Flexus;
using namespace Flexus::Qemu;
extern "C" {
    void houseKeeping(void*,void*,void*);
}
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
                , [this](int32_t x, MemoryMessage& y){ this->toL1D(x,y); } //std::bind( &DecoupledFeederComponent::toL1D, this, _1, _2)
                , [this](int32_t x, MemoryMessage& y, auto dummy){ this->modernToL1I(x,y); } //std::bind( &DecoupledFeederComponent::modernToL1I, this, _1, _2)
                , [this](MemoryMessage& x){ this->toDMA(x); } //std::bind( &DecoupledFeederComponent::toDMA, this, _1)
                , [this](int32_t x, MemoryMessage& y){ this->toNAW(x,y); } //std::bind( &DecoupledFeederComponent::toNAW, this, _1, _2)
                //, cfg.WhiteBoxDebug
                //, cfg.WhiteBoxPeriod
                , cfg.SendNonAllocatingStores
			  );
    printf("Is the FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder) run?\n");
    size_t i;
    Flexus::SharedTypes::MemoryMessage msg(MemoryMessage::LoadReq);
//    DecoupledFeederComponent::toL1D((int32_t) 0, msg); 
    
  //  printf("toL1D %p\n", DecoupledFeederComponent::toL1D);

  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    //Disable cycle-callback
    //Flexus::Qemu::theQemuInterface->disableCycleHook();
    printf("Decoupled feeder intialized? START...");
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
      theLastICounts[i] = Qemu::API::QEMU_get_instruction_count(i);
    }

	//TODO fix this with actual QEMU_insert_callback.
    //thePeriodicHap = new periodic_hap_t(this, cfg.HousekeepingPeriod);
    Qemu::API::QEMU_insert_callback(QEMUFLEX_GENERIC_CALLBACK, Qemu::API::QEMU_periodic_event,(void*)this, (void*)&houseKeeping);
    theFlexus->advanceCycles(0);
    theCMPWidth = cfg.CMPWidth;
    if (theCMPWidth == 0) {
      theCMPWidth = Qemu::API::QEMU_get_num_cores();
    }
    printf("Hello decouple feeder!\n");
  }

  void finalize(void) {}

  std::pair< uint64_t, uint32_t> theFetchInfo;

  void toL1D(int32_t anIndex, MemoryMessage & aMessage) {
  //  printf("toL1D interface entry!\n");
    FLEXUS_CHANNEL_ARRAY( ToL1D, anIndex ) << aMessage;
  }

  void toNAW(int32_t anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToNAW, anIndex / theCMPWidth ) << aMessage;
  }

  void toDMA(MemoryMessage & aMessage) {
    FLEXUS_CHANNEL( ToDMA ) << aMessage;
  }
  /*
  void toL1I(int32_t anIndex, MemoryMessage & aMessage, uint32_t anOpcode) {
    FLEXUS_CHANNEL_ARRAY( ToL1I, anIndex ) << aMessage;
    theFetchInfo.first = aMessage.pc();
    theFetchInfo.second = anOpcode;
    FLEXUS_CHANNEL_ARRAY( ToBPred, anIndex ) << theFetchInfo;
  }
  */
  void modernToL1I(int32_t anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToL1I, anIndex ) << aMessage;

    pc_type_annul_triplet thePCTypeAndAnnulTriplet;
    thePCTypeAndAnnulTriplet.first = aMessage.pc();

    std::pair< uint32_t, uint32_t> theTypeAndAnnulPair;
    theTypeAndAnnulPair.first = (uint32_t)aMessage.branchType();
    theTypeAndAnnulPair.second = (uint32_t)aMessage.branchAnnul();

    thePCTypeAndAnnulTriplet.second = theTypeAndAnnulPair;

    FLEXUS_CHANNEL_ARRAY( ToBPred, anIndex ) << thePCTypeAndAnnulTriplet;
  }
  void updateInstructionCounts() {
    //Count instructions
    //FIXME Currently Does nothing since step_count ha not been implemented
    for (int32_t i = 0; i < theNumCPUs; ++i) {
	  int64_t temp = Qemu::API::QEMU_get_instruction_count(i);
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
extern "C" {
void houseKeeping(void* obj, void * ign, void* ign2){
    static_cast<DecoupledFeederComponent*>(obj)->doHousekeeping();
}
}
}  // end Namespace nDecoupledFeeder

//extern "C" {
//void houseKeeping(void* obj, void * ign, void* ign2){
//    printf("Is houseKeeping being run?  1\n");
//        nDecoupledFeeder::houseKeep(obj);
//}
//}
FLEXUS_COMPONENT_INSTANTIATOR( DecoupledFeeder, nDecoupledFeeder);
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1D ) {
  //  printf("DecoupldFeeder 1\n");
    return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1I ) {
 //   printf("DecoupldFeeder 2\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToBPred ) {
 //   printf("DecoupldFeeder 3\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToNAW ) {
 //   printf("DecoupldFeeder 4\n");
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder

#define DBG_Reset
#include DBG_Control()
