#ifndef FLEXUS_uFETCH_TYPES_HPP_INCLUDED
#define FLEXUS_uFETCH_TYPES_HPP_INCLUDED

#include <list>
#include <iostream>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/FillLevel.hpp>

namespace Flexus {
namespace SharedTypes {

using boost::counted_base;
using Flexus::SharedTypes::VirtualMemoryAddress;
using Flexus::SharedTypes::TransactionTracker;

enum eBranchType {
  kNonBranch
  , kConditional
  , kUnconditional
  , kCall
  , kReturn
  , kLastBranchType
};
std::ostream & operator << (std::ostream & anOstream, eBranchType aType);

enum eDirection {
  kStronglyTaken
  , kTaken                //Bimodal
  , kNotTaken             //gShare
  , kStronglyNotTaken
};
std::ostream & operator << (std::ostream & anOstream, eDirection aDir);

struct BPredState : boost::counted_base {
  eBranchType thePredictedType;
  VirtualMemoryAddress thePredictedTarget;
  eDirection thePrediction;
  eDirection theBimodalPrediction;
  eDirection theMetaPrediction;
  eDirection theGSharePrediction;
  uint32_t theGShareShiftReg;
  uint32_t theSerial;
};

struct FetchAddr {
  Flexus::SharedTypes::VirtualMemoryAddress theAddress;
  boost::intrusive_ptr<BPredState> theBPState;
  FetchAddr(Flexus::SharedTypes::VirtualMemoryAddress anAddress)
    : theAddress(anAddress)
  { }
};

struct FetchCommand : boost::counted_base {
  std::list< FetchAddr > theFetches;
};

struct BranchFeedback : boost::counted_base {
  VirtualMemoryAddress thePC;
  eBranchType theActualType;
  eDirection theActualDirection;
  VirtualMemoryAddress theActualTarget;
  boost::intrusive_ptr<BPredState> theBPState;
};

typedef int64_t Opcode;

struct FetchedOpcode {
  VirtualMemoryAddress thePC;
  VirtualMemoryAddress theNextPC;
  Opcode theOpcode;
  boost::intrusive_ptr<BPredState> theBPState;
  boost::intrusive_ptr<TransactionTracker> theTransaction;

  FetchedOpcode( VirtualMemoryAddress anAddr
                 , VirtualMemoryAddress aNextAddr
                 , Opcode anOpcode
                 , boost::intrusive_ptr<BPredState> aBPState
                 , boost::intrusive_ptr<TransactionTracker> aTransaction
               )
    : thePC(anAddr)
    , theNextPC(aNextAddr)
    , theOpcode(anOpcode)
    , theBPState(aBPState)
    , theTransaction(aTransaction)
  { }
};

struct FetchBundle : public boost::counted_base {
  std::list< FetchedOpcode > theOpcodes;
  std::list< tFillLevel > theFillLevels;
};

typedef boost::intrusive_ptr<FetchBundle> pFetchBundle;

struct CPUState {
  int32_t theTL;
  int32_t thePSTATE;
};

} // end namespace SharedTypes
} // end namespace Flexus

#endif //FLEXUS_uFETCH_TYPES_HPP_INCLUDED

