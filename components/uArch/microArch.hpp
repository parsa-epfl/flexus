#ifndef FLEXUS_uARCH_microARCH_HPP_INCLUDED
#define FLEXUS_uARCH_microARCH_HPP_INCLUDED

#include <boost/function.hpp>
#include <memory>

#include <components/Common/Slices/MemOp.hpp>
#include <components/Common/Slices/PredictorMessage.hpp> /* CMU-ONLY */

namespace Flexus {
namespace SharedTypes {
struct BranchFeedback;
}
}

namespace nuArch {

using Flexus::SharedTypes::VirtualMemoryAddress;

struct microArch {
  static std::shared_ptr<microArch>
  construct( uArchOptions_t options
             , boost::function< void(eSquashCause)> squash
             , boost::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> redirect
             , boost::function< void(int, int)> changeState
             , boost::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
             , boost::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> notifyTMS /* CMU-ONLY */
             , boost::function< void(bool) > aStoreForwardingHitFunction
           );

  virtual int32_t availableROB() = 0;
  virtual bool isSynchronized() = 0;
  virtual bool isQuiesced() = 0;
  virtual bool isStalled() = 0;
  virtual int32_t iCount() = 0;
  virtual void dispatch(boost::intrusive_ptr< AbstractInstruction >) = 0;
  virtual void skipCycle() = 0;
  virtual void cycle() = 0;
  virtual void pushMemOp(boost::intrusive_ptr< MemOp >) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual ~microArch() {}
  virtual void testCkptRestore() = 0;
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;
  virtual void pregs() = 0;
  virtual void pregsAll() = 0;
  virtual void resynchronize() = 0;
  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;
  virtual void writePermissionLost(PhysicalMemoryAddress anAddress) = 0;

};

} //nuArch

#endif //FLEXUS_uARCH_microARCH_HPP_INCLUDED
