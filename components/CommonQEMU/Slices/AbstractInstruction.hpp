#ifndef FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED
#define FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <ostream>

namespace Flexus {
namespace SharedTypes {

struct AbstractInstruction : public boost::counted_base
{
    boost::intrusive_ptr<TransactionTracker> theFetchTransaction;

  protected:
    tFillLevel theInsnSourceLevel;
    bool theExclusive;

  public:
    virtual bool isExclusive() const { return theExclusive; }
    virtual void setExclusive() { theExclusive = true; }
    virtual void describe(std::ostream& anOstream) const;
    virtual bool resync() const             = 0;
    virtual void forceResync(bool r = true) = 0;
    virtual bool haltDispatch() const;
    virtual void setFetchTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction)
    {
        theFetchTransaction = aTransaction;
    }
    virtual boost::intrusive_ptr<TransactionTracker> getFetchTransactionTracker() const { return theFetchTransaction; }
    AbstractInstruction()
      : theInsnSourceLevel(eL1I)
      , theExclusive(false)
    {
    }
    virtual ~AbstractInstruction() {}

    virtual void setSourceLevel(tFillLevel aLevel) { theInsnSourceLevel = aLevel; }
    virtual tFillLevel sourceLevel() const { return theInsnSourceLevel; }
    virtual uint32_t getOpcode() = 0;
};

enum eSquashCause
{
    kResynchronize    = 1,
    kBranchMispredict = 2,
    kException        = 3,
    kInterrupt        = 4,
    kFailedSpec       = 5
};

std::ostream&
operator<<(std::ostream& anOstream, eSquashCause aCause);

std::ostream&
operator<<(std::ostream& anOstream, AbstractInstruction const& anInstruction);

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_SLICES_ABSTRACTINSTRUCTION_HPP_INCLUDED