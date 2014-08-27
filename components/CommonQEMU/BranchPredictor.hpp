#ifndef FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
#define FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED

#include <iostream>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uFetch/uFetchTypes.hpp>

namespace Flexus {
namespace SharedTypes {

struct BranchPredictor {
  static BranchPredictor * combining(std::string const & aName, uint32_t anIndex);
  virtual bool isBranch( VirtualMemoryAddress anAddress) = 0;
  virtual void feedback( BranchFeedback const & aFeedback) = 0;
  virtual VirtualMemoryAddress predict( FetchAddr & aFetchAddr) = 0;
  virtual ~BranchPredictor() {}
  virtual void loadState( std::string const & aDirName) = 0;
  virtual void saveState( std::string const & aDirName) const = 0;
};

struct FastBranchPredictor {
  static FastBranchPredictor * combining(std::string const & aName, uint32_t anIndex);
  //virtual void feedback( BranchFeedback const & aFeedback) = 0;
  virtual void predict( VirtualMemoryAddress anAddress, BPredState & aBPState) = 0;
  virtual void feedback( VirtualMemoryAddress anAddress,  eBranchType anActualType, eDirection anActualDirection, VirtualMemoryAddress anActualAddress, BPredState & aBPState) = 0;
  virtual ~FastBranchPredictor() {}
  virtual void loadState( std::string const & aDirName) = 0;
  virtual void saveState( std::string const & aDirName) const = 0;
};

} //SharedTypes
} //Flexus

#endif //FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
