#include <components/TraceVMMapper/TraceVMMapper.hpp>

#include <core/types.hpp>
#include <core/stats.hpp>

#include <core/simics/mai_api.hpp>

#include <boost/bind.hpp>
#include <unordered_map>

#include <fstream>

#include <stdlib.h> // for random()

#include <components/Common/Util.hpp>
using nCommonUtil::log_base2;

#define DBG_DefineCategories TraceVMMapper
#define DBG_SetDefaultOps AddCat(TraceVMMapper) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT TraceVMMapper
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTraceVMMapper {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(TraceVMMapper) {
  FLEXUS_COMPONENT_IMPL( TraceVMMapper );

  int64_t theLowMask;
  int64_t theHighMask;
  int32_t theVMIndexWidth;
  int32_t theVMIndexShift;
  int32_t theVMXORShift;
  int32_t * theCoreMap;
  int32_t * theReverseCoreMap;

  int32_t theCoresPerVM;
  int32_t theTotalNumCores;
  int32_t theNumVMs;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(TraceVMMapper)
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
    theVMIndexShift = cfg.VMIndexShift;
    theLowMask = (1LL << cfg.VMIndexShift) - 1;
    theHighMask = ~theLowMask;
    theVMIndexWidth = log_base2(Flexus::Simics::ProcessorMapper::numVMs());
    theVMXORShift = cfg.VMXORShift;
    DBG_Assert( theVMIndexShift > (theVMXORShift + theVMIndexWidth));
    if (cfg.VMXORShift < 0) {
      theVMXORShift = 64;
    }

  }

  bool available( interface::RequestIn const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::RequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    fixMessage(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received Request Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(RequestOut, aMessage.coreIdx()) << aMessage;
  }

  bool available( interface::FetchRequestIn const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::FetchRequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    fixMessage(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received FetchRequest Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(FetchRequestOut, aMessage.coreIdx()) << aMessage;
  }

  bool available( interface::SnoopIn const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::SnoopIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    undoMapping(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received Snoop Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(SnoopOut, aMessage.coreIdx()) << aMessage;
  }

  bool available( interface::SnoopIn_I const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::SnoopIn_I const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    PhysicalMemoryAddress orig_addr = aMessage.address();
    undoMapping(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received SnoopI Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(SnoopOut_I, aMessage.coreIdx()) << aMessage;
    aMessage.address() = orig_addr;
  }

  bool available( interface::SnoopIn_D const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::SnoopIn_D const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    PhysicalMemoryAddress orig_addr = aMessage.address();
    undoMapping(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received SnoopD Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(SnoopOut_D, aMessage.coreIdx()) << aMessage;
    aMessage.address() = orig_addr;
  }

  bool available( interface::NonAllocateWrite_In const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::NonAllocateWrite_In const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    fixMessage(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received NonAllocateWrite Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL_ARRAY(NonAllocateWrite_Out, aMessage.coreIdx()) << aMessage;
  }

  bool available( interface::DMA_In const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::DMA_In const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    fixMessageVM(aMessage, anIndex);
    DBG_(Iface, Addr(aMessage.address()) ( << "TraceVMMapper: received DMA Message " << aMessage << " from core " << anIndex << " mapping to core " << aMessage.coreIdx() ));
    FLEXUS_CHANNEL(DMA_Out) << aMessage;
  }

private:

  void saveState(std::string const & aDirName) { }

  void loadState( std::string const & aDirName ) { }

#if 0
  inline int32_t getVMIndex(int32_t core) {
    return (core / theCoresPerVM);
  }
#endif

  inline PhysicalMemoryAddress remapAddress(PhysicalMemoryAddress addr, int32_t core) {
    uint64_t VM_index = Flexus::Simics::ProcessorMapper::mapFlexusIndex2VM(core);
    return PhysicalMemoryAddress((((addr & theHighMask) << theVMIndexWidth) | (VM_index << theVMIndexShift) | (addr & theLowMask)) ^ (VM_index << theVMXORShift));
  }

  inline PhysicalMemoryAddress remapAddressVM(PhysicalMemoryAddress addr, int32_t vm) {
    return PhysicalMemoryAddress((((addr & theHighMask) << theVMIndexWidth) | (vm << theVMIndexShift) | (addr & theLowMask)) ^ (vm << theVMXORShift));
  }

  inline PhysicalMemoryAddress unmapAddress(PhysicalMemoryAddress addr, int32_t core) {
    int32_t vm = (addr >> theVMIndexShift) & ((1 << theVMIndexWidth) - 1);
    return PhysicalMemoryAddress( ((((int64_t)addr >> theVMIndexWidth) & theHighMask) | (addr & theLowMask)) ^ (vm << theVMXORShift));
  }

  inline int32_t remapCoreVM(int32_t vm) {
    return vm;
  }

  inline int32_t remapCore(int32_t core) {
    //return theCoreMap[core];
    return core;
  }

  inline int32_t unmapCore(int32_t core) {
    //return theReverseCoreMap[core];
    return core;
  }

  void fixMessage(MemoryMessage & aMessage, int32_t core) {
    aMessage.address() = remapAddress(aMessage.address(), core);
    //aMessage->physical_pc() = remapAddress(aMessage->physical_pc(), core);
    aMessage.coreIdx() = remapCore(core);
  }

  void fixMessageVM(MemoryMessage & aMessage, int32_t vm) {
    aMessage.address() = remapAddressVM(aMessage.address(), vm);
    //aMessage->physical_pc() = remapAddress(aMessage->physical_pc(), core);
    aMessage.coreIdx() = remapCoreVM(vm);
  }

  void undoMapping(MemoryMessage & aMessage, int32_t core) {
    aMessage.coreIdx() = unmapCore(core);
    aMessage.address() = unmapAddress(aMessage.address(), core);
    //aMessage->physical_pc() = unmapAddress(aMessage->physical_pc(), core);
  }

};  // end class TraceVMMapper

}  // end Namespace nTraceVMMapper

FLEXUS_COMPONENT_INSTANTIATOR( TraceVMMapper, nTraceVMMapper);

FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, RequestIn )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, FetchRequestIn )  {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopIn )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopIn_I )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopIn_D )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, NonAllocateWrite_In ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, DMA_In )    {
  return Flexus::Simics::ProcessorMapper::numVMs();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, RequestOut )   {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, FetchRequestOut )  {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopOut )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopOut_D )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, SnoopOut_I )    {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( TraceVMMapper, NonAllocateWrite_Out ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TraceVMMapper

#define DBG_Reset
#include DBG_Control()
