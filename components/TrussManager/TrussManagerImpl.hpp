#define FLEXUS_BEGIN_COMPONENT TrussManager
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories TrussMgr
#define DBG_SetDefaultOps AddCat(TrussMgr)
#include DBG_Control()

#include <boost/shared_array.hpp>
#include <iomanip>

namespace nTrussManager {

using namespace Flexus;

using SharedTypes::node_id_t;
using namespace Core;

template <class TrussManagerComponent>
struct TrussManagerImpl : public TrussManager {
  TrussManagerComponent & theTrussManager;
  node_id_t theRequestingComponent;
  bool theRequestingComponent_isValid;

  TrussManagerImpl(TrussManagerComponent & aTrussManager)
    : theTrussManager(aTrussManager)
    , theRequestingComponent_isValid(false)
  {}

  TrussManagerImpl(TrussManagerComponent & aTrussManager, node_id_t aRequestingComponent)
    : theTrussManager(aTrussManager)
    , theRequestingComponent(aRequestingComponent)
    , theRequestingComponent_isValid(true)
  {}

  virtual bool isMasterNode(node_id_t aNode) {
    return theTrussManager.isMasterNode(aNode);
  }
  virtual bool isSlaveNode(node_id_t aNode) {
    return theTrussManager.isSlaveNode(aNode);
  }
  virtual int32_t getSlaveIndex(node_id_t aNode) {
    return theTrussManager.getSlaveIndex(aNode);
  }
  virtual node_id_t getMasterIndex(node_id_t aNode) {
    return theTrussManager.getMasterIndex(aNode);
  }
  virtual node_id_t getProcIndex(node_id_t aNode) {
    return theTrussManager.getProcIndex(aNode);
  }
  virtual node_id_t getSlaveNode(node_id_t aNode, int32_t slave_idx) {
    return theTrussManager.getSlaveNode(aNode, slave_idx);
  }
  virtual int32_t getFixedDelay() {
    return theTrussManager.getFixedDelay();
  }

};

//Initialized upon construction of the TrussManagerComponent
class TrussManagerFactory;
TrussManagerFactory * theTrussManagerFactory = 0;

//The TrussManagerFactory knows how to create TrussManager objects
struct TrussManagerFactory {
  TrussManagerFactory() {
    DBG_Assert( (theTrussManagerFactory == 0) );
    theTrussManagerFactory = this;
  }
  virtual boost::intrusive_ptr<TrussManager> createTrussManager() = 0;
  virtual boost::intrusive_ptr<TrussManager> createTrussManager(node_id_t aRequestingNode) = 0;
};

inline boost::intrusive_ptr<TrussManager> TrussManager::getTrussManager() {
  DBG_Assert(theTrussManagerFactory != 0);
  return theTrussManagerFactory->createTrussManager();
}
inline boost::intrusive_ptr<TrussManager> TrussManager::getTrussManager(node_id_t aRequestingNode) {
  DBG_Assert(theTrussManagerFactory != 0);
  return theTrussManagerFactory->createTrussManager(aRequestingNode);
}

template <class Cfg>
class TrussManagerComponent : public FlexusComponentBase<TrussManagerComponent, Cfg> , TrussManagerFactory {
  FLEXUS_COMPONENT_IMPL(nTrussManager::TrussManagerComponent, Cfg);

public:
  TrussManagerComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {    }

  // Initialization
  void initialize() {
  }

  virtual ~TrussManagerComponent() {} //Eliminate warning about virtual destructor

  virtual boost::intrusive_ptr<TrussManager> createTrussManager() {
    return boost::intrusive_ptr<TrussManager>(new TrussManagerImpl<self>(*this));
  }

  virtual boost::intrusive_ptr<TrussManager> createTrussManager(node_id_t aRequestingNode) {
    return boost::intrusive_ptr<TrussManager> (new TrussManagerImpl<self>(*this, aRequestingNode));
  }

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

  bool isMasterNode(node_id_t aNode) {
    return (aNode < (unsigned)cfg_t::NumProcs_t::value);
  }
  bool isSlaveNode(node_id_t aNode)  {
    return (aNode >= (unsigned)cfg_t::NumProcs_t::value);
  }
  int32_t getSlaveIndex(node_id_t aNode) {
    DBG_Assert(isSlaveNode(aNode));
    return (aNode / (cfg_t::NumProcs_t::value) - 1);
  }
  node_id_t getMasterIndex(node_id_t aNode) {
    return (aNode - (1 + getSlaveIndex(aNode)) * (cfg_t::NumProcs_t::value));
  }
  node_id_t getProcIndex(node_id_t aNode) {
    return (isSlaveNode(aNode) ? getMasterIndex(aNode) : aNode);
  }
  node_id_t getSlaveNode(node_id_t aNode, int32_t slave_idx) {
    DBG_Assert(isMasterNode(aNode));
    return (aNode + (1 + slave_idx) * (cfg_t::NumProcs_t::value));
  }
  int32_t getFixedDelay() {
    return cfg.FixedDelay.value;
  }

  static const int32_t num_nodes = cfg_t::NumProcs_t::value * (1 + cfg_t::NumReplicas_t::value);

};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussManagerConfiguration,
                                        FLEXUS_STATIC_PARAMETER( NumProcs, int, "Number of Processors", "procs", 2)
                                        FLEXUS_STATIC_PARAMETER( NumReplicas, int, "Number of Replicas", "replicas", 1)
                                        FLEXUS_PARAMETER( FixedDelay, int, "Delay between M/S", "delay", 100)
                                       );

} //End Namespace nTrussManager

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussManager

#define DBG_Reset
#include DBG_Control()

