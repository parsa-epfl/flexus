#include <core/stats.hpp>
#include <components/FastMemoryLoopback/FastMemoryLoopback.hpp>
#include <components/FastMemoryLoopback/MemStats.hpp>


#define FLEXUS_BEGIN_COMPONENT FastMemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FastMemoryLoopback
#define DBG_SetDefaultOps AddCat(FastMemoryLoopback)
#include DBG_Control()

namespace nFastMemoryLoopback {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

class FLEXUS_COMPONENT(FastMemoryLoopback) {
  FLEXUS_COMPONENT_IMPL( FastMemoryLoopback );

  MemStats *theStats;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastMemoryLoopback)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  // Initialization
  void initialize() {
	  theStats = new MemStats(statName());
  }
  
  void finalize() {}

  // Ports
  FLEXUS_PORT_ALWAYS_AVAILABLE(FromCache);
  void push(interface::FromCache const &, MemoryMessage & message) {
   
    DBG_(Iface, Addr(message.address()) ( << "request received: " << message ) );
	switch (message.type()) {
      case MemoryMessage::LoadReq:
      case MemoryMessage::FetchReq:
        message.type() = MemoryMessage::MissReply;
        message.fillLevel() = eLocalMem;
        (theStats->theReadRequests_stat)++;
        break;
      case MemoryMessage::ReadReq:
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:
        message.type() = MemoryMessage::MissReply;
        message.fillLevel() = eLocalMem;
        (theStats->theReadRequests_stat)++;
        break;
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:      
        (theStats->theWriteRequests_stat)++;
      break;
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        message.type() = MemoryMessage::MissReplyWritable;
        message.fillLevel() = eLocalMem;        
        (theStats->theUpgradeRequest_stat)++;
        break;
      case MemoryMessage::EvictDirty:
          (theStats->theEvictDirtys_stat)++;     
               
          break;
      case MemoryMessage::EvictWritable:
    	  (theStats->theEvictWritables_stat)++;    	  
    	  break;
      case MemoryMessage::EvictClean:
        // no response necessary (and no state to track here)
    	  (theStats->theEvictCleans_stat)++;    	  
          break;
      case MemoryMessage::NonAllocatingStoreReq:
        message.type() = MemoryMessage::NonAllocatingStoreReply;
        message.fillLevel() = eLocalMem;
        (theStats->theNonAllocatingStoreReq_stat)++;        
        break;
      default:
        DBG_Assert(false, ( << "unknown request received: " << message));
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(DMA);
  void push(interface::DMA const &, MemoryMessage & message) {
    DBG_( Iface, Addr(message.address()) ( << "DMA " << message ));
    if (message.type() == MemoryMessage::WriteReq) {
      (theStats->theWriteDMA_stat)++;      
      DBG_( Trace, Addr(message.address()) ( << "Sending Invalidate in response to DMA " << message));
      message.type() = MemoryMessage::Invalidate;
      FLEXUS_CHANNEL( ToCache ) << message;
    } else {
      (theStats->theReadDMA_stat)++;      
      DBG_Assert( message.type() == MemoryMessage::ReadReq );
      message.type() = MemoryMessage::Downgrade;
      FLEXUS_CHANNEL( ToCache ) << message;
    }

  }

  void drive( interface::UpdateStatsDrive const &) {
    theStats->update();
  }

};

}//End namespace nFastMemoryLoopback

FLEXUS_COMPONENT_INSTANTIATOR( FastMemoryLoopback, nFastMemoryLoopback);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastMemoryLoopback

#define DBG_Reset
#include DBG_Control()
