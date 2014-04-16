#ifndef FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED
#define FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED

#include <ostream>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#include <components/Common/Slices/FillLevel.hpp>

namespace Flexus {
namespace SharedTypes {

struct AbstractInstruction  : public boost::counted_base {
  boost::intrusive_ptr<TransactionTracker> theFetchTransaction;
protected:
  tFillLevel theInsnSourceLevel;

public:

  virtual void describe(std::ostream & anOstream) const;
  virtual bool haltDispatch() const;
  virtual void setFetchTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) {
    theFetchTransaction = aTransaction;
  }
  virtual boost::intrusive_ptr<TransactionTracker> getFetchTransactionTracker() const {
    return theFetchTransaction;
  }
  AbstractInstruction() : theInsnSourceLevel(eL1I) {}
  virtual ~AbstractInstruction() {}

  virtual void setSourceLevel(tFillLevel aLevel) {
    theInsnSourceLevel = aLevel;
  }
  virtual tFillLevel sourceLevel() const {
    return theInsnSourceLevel;
  }

};

enum eSquashCause {
  kResynchronize = 1
  , kBranchMispredict = 2
  , kException = 3
  , kInterrupt = 4
  , kFailedSpec = 5
};

std::ostream & operator << ( std::ostream & anOstream, eSquashCause aCause );

std::ostream & operator << (std::ostream & anOstream, AbstractInstruction const & anInstruction);

} //Flexus
} //SharedTypes

#endif //FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED
