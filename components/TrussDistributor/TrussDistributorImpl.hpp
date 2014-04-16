#include <memory>

#include <deque>

#define FLEXUS_BEGIN_COMPONENT TrussDistributor
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories TrussDistributor, Distributor
#define DBG_SetDefaultOps AddCat(TrussDistributor | Distributor)
#include DBG_Control()

namespace nTrussDistributor {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

typedef uint64_t CycleCount;

typedef std::pair<CycleCount, boost::intrusive_ptr<ArchitecturalInstruction> > InstructionCyclePair;

template <class Cfg>
class TrussDistributorComponent : public FlexusComponentBase< TrussDistributorComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(nTrussDistributor::TrussDistributorComponent, Cfg);

public:
  TrussDistributorComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  std::deque< InstructionCyclePair > theSlaveBuffers[cfg_t::NumProcs_t::value][cfg_t::NumReplicas_t::value];

  void initialize() { }

  //Ports
public:
  struct InstructionIn : public PullInputPortArray< InstructionTransport, cfg_t::NumProcs_t::value > { };

  struct InstructionOut : public PullOutputPortArray < InstructionTransport , (1 + cfg_t::NumReplicas_t::value)*(cfg_t::NumProcs_t::value) > , AvailabilityComputedOnRequest {
    FLEXUS_WIRING_TEMPLATE
  static InstructionTransport pull(self & aDistributor, index_t anIndex) {
    // This function is called when a node's fetch component wants an instruction
    // If this is a master node, we get the instruction from the feeder and
    //   also copy it into two buffers
    // If this is a slave node, we get the instruction from the appropriate buffer

    InstructionTransport anInsn;

    boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
    int32_t proc = theTrussManager->getProcIndex(anIndex);
    CycleCount cycle = Flexus::Core::theFlexus->cycleCount();

    if (theTrussManager->isMasterNode(anIndex)) {
      DBG_Assert(FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc).available() );
      //Fetch it from the feeder
      FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc) >> anInsn;

      // buffer it for the slave nodes
      for (int32_t i = 0; i < cfg_t::NumReplicas_t::value; i++) {
        aDistributor.theSlaveBuffers[proc][i].push_back(InstructionCyclePair(cycle, anInsn[ArchitecturalInstructionTag]->createShadow()));
      }

    } else {
      int32_t idx = theTrussManager->getSlaveIndex(anIndex);
      //DBG_(Crit, (<< "idx = " << idx << " -- queue size: " << aDistributor.theSlaveBuffers[proc][idx].size()));
      boost::intrusive_ptr<ArchitecturalInstruction> aSlave = aDistributor.theSlaveBuffers[proc][idx].front().second;
      anInsn.set(ArchitecturalInstructionTag, aSlave);
      aDistributor.theSlaveBuffers[proc][idx].pop_front();
    }

    DBG_(Iface, ( << "done pulling from Distributor [" << anIndex << "]: " << *anInsn[ArchitecturalInstructionTag] ) );

    return anInsn;
  }

  FLEXUS_WIRING_TEMPLATE
  static bool available(self & aDistributor, index_t anIndex) {
    boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
    int32_t proc = theTrussManager->getProcIndex(anIndex);
    if (theTrussManager->isMasterNode(anIndex)) {
      return FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc).available();
    } else {
      int32_t idx = theTrussManager->getSlaveIndex(anIndex);
      if (!aDistributor.theSlaveBuffers[proc][idx].empty()) {
        CycleCount cycle = aDistributor.theSlaveBuffers[proc][idx].front().first;
        DBG_Assert(Flexus::Core::theFlexus->cycleCount() <= cycle + theTrussManager->getFixedDelay(),
                   ( << "Instruction not distributed correctly! cycle: " << cycle + theTrussManager->getFixedDelay()));
        return (Flexus::Core::theFlexus->cycleCount() == cycle + theTrussManager->getFixedDelay());
      } else {
        return false;
      }
    }// master/slave
  }
  };

  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

private:
  //Implementation of the FetchDrive drive interface
  FLEXUS_WIRING_TEMPLATE void doFetch() {
  }
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussDistributorComponentConfiguration,
                                        FLEXUS_STATIC_PARAMETER( NumProcs, int, "Number of Processors", "procs", 2)
                                        FLEXUS_STATIC_PARAMETER( NumReplicas, int, "Number of Replicas", "replicas", 1)
                                       );

}//End namespace nTrussDistributor

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussDistributor

#define DBG_Reset
#include DBG_Control()
