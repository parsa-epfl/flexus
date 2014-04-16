#include <components/VMMapper/VMMapper.hpp>

#include <core/types.hpp>
#include <core/stats.hpp>

#include <core/simics/mai_api.hpp>

#include <boost/bind.hpp>
#include <ext/hash_map>

#include <fstream>

#include <components/Common/Util.hpp>

#include <stdlib.h> // for random()

#define DBG_DefineCategories VMMapper
#define DBG_SetDefaultOps AddCat(VMMapper) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT VMMapper
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nVMMapper {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

using namespace nCommonUtil;

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(VMMapper) {
  FLEXUS_COMPONENT_IMPL( VMMapper );

  uint64_t theLowMask;
  uint64_t theHighMask;
  int32_t theVMIndexShift;
  int32_t theVMIndexWidth;
  int32_t * theCoreMap;
  int32_t * theReverseCoreMap;

  int32_t theCoresPerVM;
  int32_t theTotalNumCores;
  int32_t theNumVMs;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(VMMapper)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void finalize( void ) {
  }

  void initialize(void) {
    // Where are we going to put the vm_id in the address
    // PhysicalMemoryAddress is 64-bits, but in our simulations, we only use 32-bits of it
    // so shift it 32-bits
    theVMIndexShift = cfg.PhysicalAddrBits;
    theLowMask = (1ULL << theVMIndexShift) - 1;
    theHighMask = ~theLowMask;
    theVMIndexWidth = log_base2(Flexus::Simics::ProcessorMapper::numVMs());
  }

  bool available( interface::FrontSideIn_Snoop const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(BackSideOut_Snoop, anIndex).available();
  }

  void push( interface::FrontSideIn_Snoop const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    if ( vm == Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(anIndex)) {
      fixMessage(aMessage[MemoryMessageTag], vm);
    }
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received Snoop Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    //FLEXUS_CHANNEL_ARRAY(BackSideOut_Snoop, aMessage[MemoryMessageTag]->coreIdx()) << aMessage;
    FLEXUS_CHANNEL_ARRAY(BackSideOut_Snoop, anIndex) << aMessage;
  }

  bool available( interface::FrontSideIn_Request const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(BackSideOut_Request, anIndex).available();
  }

  void push( interface::FrontSideIn_Request const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    // This line might break some simlators that expect core to be 0
    aMessage[MemoryMessageTag]->coreIdx() = anIndex;

    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    fixMessage(aMessage[MemoryMessageTag], vm);
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received Request Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(BackSideOut_Request, anIndex) << aMessage;
  }

  bool available( interface::FrontSideIn_Prefetch const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(BackSideOut_Prefetch, anIndex).available();
  }

  void push( interface::FrontSideIn_Prefetch const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    //aMessage[MemoryMessageTag]->coreIdx() = anIndex;
    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    fixMessage(aMessage[MemoryMessageTag], vm);
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received Prefetch Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(BackSideOut_Prefetch, anIndex) << aMessage;
  }

  bool available( interface::BackSideIn const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(FrontSideOut, anIndex).available();
  }

  void push( interface::BackSideIn const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    // TODO: Fix this so that only messages from same VM are un-mapped
    if (vm == Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(anIndex)) {
      undoMapping(aMessage[MemoryMessageTag], vm);
    }
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received Back Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    //FLEXUS_CHANNEL_ARRAY(FrontSideOut, aMessage[MemoryMessageTag]->coreIdx()) << aMessage;
    FLEXUS_CHANNEL_ARRAY(FrontSideOut, anIndex) << aMessage;
  }

  bool available( interface::BackSideIn_D const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(FrontSideOut_D, anIndex).available();
  }

  void push( interface::BackSideIn_D const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    // TODO: Fix this so that only messages from same VM are un-mapped
    if (vm == Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(anIndex)) {
      undoMapping(aMessage[MemoryMessageTag], vm);
    }
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received BackD Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    //FLEXUS_CHANNEL_ARRAY(FrontSideOut_D, aMessage[MemoryMessageTag]->coreIdx()) << aMessage;
    FLEXUS_CHANNEL_ARRAY(FrontSideOut_D, anIndex) << aMessage;
  }

  bool available( interface::BackSideIn_I const &,
                  index_t anIndex ) {
    return FLEXUS_CHANNEL_ARRAY(FrontSideOut_I, anIndex).available();
  }

  void push( interface::BackSideIn_I const &,
             index_t         anIndex,
             MemoryTransport & aMessage ) {
    int32_t vm = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(getSource(aMessage, anIndex));
    // TODO: Fix this so that only messages from same VM are un-mapped
    if (vm == Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(anIndex)) {
      undoMapping(aMessage[MemoryMessageTag], vm);
    }
    DBG_(Trace, Addr(aMessage[MemoryMessageTag]->address()) ( << "VMMapper: VM " << vm << " received BackI Message " << *(aMessage[MemoryMessageTag]) << " from core " << anIndex << " mapping to core " << aMessage[MemoryMessageTag]->coreIdx() ));
    //FLEXUS_CHANNEL_ARRAY(FrontSideOut_I, aMessage[MemoryMessageTag]->coreIdx()) << aMessage;
    FLEXUS_CHANNEL_ARRAY(FrontSideOut_I, anIndex) << aMessage;
  }

private:

  void saveState(std::string const & aDirName) { }

  void loadState( std::string const & aDirName ) { }

#if 0
  inline int32_t getVMIndex(int32_t core) {
    return (core / theCoresPerVM);
  }
#endif

  inline PhysicalMemoryAddress remapAddress(PhysicalMemoryAddress addr, int32_t vm) {
    return PhysicalMemoryAddress(((addr & theHighMask) << theVMIndexWidth) | (addr & theLowMask) | ((uint64_t)vm << theVMIndexShift));
  }

  inline PhysicalMemoryAddress unmapAddress(PhysicalMemoryAddress addr, int32_t vm) {
    return PhysicalMemoryAddress(((addr >> theVMIndexWidth) & theHighMask) | (addr & theLowMask));
  }

  void fixMessage(boost::intrusive_ptr<MemoryMessage> aMessage, int32_t vm) {
    aMessage->address() = remapAddress(aMessage->address(), vm);
  }

  void undoMapping(boost::intrusive_ptr<MemoryMessage> aMessage, int32_t vm) {
    aMessage->address() = unmapAddress(aMessage->address(), vm);
  }

  inline int32_t getSource(MemoryTransport & transport, index_t anIndex) {
    if (transport[TransactionTrackerTag] != 0) {
      return transport[TransactionTrackerTag]->initiator().get();
    } else {
      return anIndex;
    }
  }

};  // end class VMMapper

}  // end Namespace nVMMapper

FLEXUS_COMPONENT_INSTANTIATOR( VMMapper, nVMMapper);

FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideIn_Snoop )      {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideIn_Request ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideIn_Prefetch ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideOut )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideOut_D )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, FrontSideOut_I )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideOut_Snoop )  {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideOut_Request ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideOut_Prefetch ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideIn )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideIn_D )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( VMMapper, BackSideIn_I )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT VMMapper

#define DBG_Reset
#include DBG_Control()
