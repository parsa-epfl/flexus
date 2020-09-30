#ifndef FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
#define FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED

#include <iostream>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uFetch/uFetchTypes.hpp>

namespace Flexus {
namespace SharedTypes {

struct BranchPredictor {
  static BranchPredictor *combining(std::string const &aName, uint32_t anIndex, bool RAS, bool TCE,
                                    bool TrapRet);
  static BranchPredictor *combining(std::string const &aName, uint32_t anIndex, bool RAS, bool TCE,
                                    bool TrapRet, uint32_t aBTBSets, uint32_t aBTBWays);
  virtual bool isBranch(FetchAddr &aFetchAddr) = 0;
  virtual bool isBranchInsn(VirtualMemoryAddress aFetchAddr) = 0;
  virtual void feedback(BranchFeedback &aFeedback, int flexusIndex) = 0;
  virtual void resetRAS() = 0;
  virtual void reconstructRAS(std::list<boost::intrusive_ptr<BPredState>> &theRASops) = 0;
  virtual void pushReturnAddresstoRAS(VirtualMemoryAddress retAddress) = 0;
  virtual void resetState(boost::intrusive_ptr<BPredState>) = 0;
  virtual void resetUpdateState(boost::intrusive_ptr<BPredState> aBPState) = 0;
  virtual void resetTrapState(boost::intrusive_ptr<TrapState> aTrapState) = 0;
  virtual VirtualMemoryAddress predict(FetchAddr &aFetchAddr) = 0;
  virtual BTBEntry access_BBTB(VirtualMemoryAddress anAddress) = 0;
  virtual VirtualMemoryAddress predictBranch(FetchAddr &aFetch, BTBEntry aBTBEntry) = 0;
  virtual void checkPointBPState(FetchAddr &aFetch) = 0;
  virtual eDirection conditionalTaken(VirtualMemoryAddress anAddress) = 0;
  virtual VirtualMemoryAddress getNextPrefetchAddr(VirtualMemoryAddress anAddress) = 0;
  virtual void updateBTB(VirtualMemoryAddress pc, eBranchType branchType,
                         VirtualMemoryAddress target, bool specialCall, int BBsize) = 0;
  virtual void updateBBTB(BTBEntry anEntry) = 0;
  virtual ~BranchPredictor() {
  }
  virtual void loadState(std::string const &aDirName) = 0;
  virtual void saveState(std::string const &aDirName) const = 0;
};

struct FastBranchPredictor {
  static FastBranchPredictor *combining(std::string const &aName, uint32_t anIndex);
  static FastBranchPredictor *combining(std::string const &aName, uint32_t anIndex,
                                        uint32_t aBTBSets, uint32_t aBTBWays);
  // virtual void feedback( BranchFeedback const & aFeedback) = 0;
  virtual void predict(VirtualMemoryAddress anAddress, BPredState &aBPState) = 0;
  virtual void runahead_predict(VirtualMemoryAddress anAddress, BPredState &aBPState) = 0;
  virtual void reset_runahead_history() = 0;
  virtual bool feedback(VirtualMemoryAddress anAddress, eBranchType anActualType,
                        eDirection anActualDirection, VirtualMemoryAddress anActualAddress,
                        BPredState &aBPState, int aBBsize) = 0;
  virtual void updateBB_BTB(VirtualMemoryAddress anAddress, eBranchType anActualType,
                            eDirection anActualDirection, VirtualMemoryAddress anActualAddress,
                            BPredState &aBPState, int aBBsize) = 0;
  virtual void predict_BBTB(VirtualMemoryAddress anAddress, BPredState &aBPState) = 0;
  virtual void updateBBTB(BTBEntry anEntry) = 0;
  virtual eDirection conditionalTaken(VirtualMemoryAddress anAddress) = 0;
  virtual ~FastBranchPredictor() {
  }
  virtual void loadState(std::string const &aDirName) = 0;
  virtual void saveState(std::string const &aDirName) const = 0;
};

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
