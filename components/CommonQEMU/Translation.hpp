#ifndef FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED
#define FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>


namespace Flexus {
namespace SharedTypes {

static uint64_t translationID;

struct Translation : public boost::counted_base
{

    enum eTranslationType
    {
        eLoad,
        eStore,
        eFetch
    };

    enum eTLBstatus
    {
        kTLBunresolved,
        kTLBmiss,
        kTLBhit
    };

    enum eTLBtype
    {
        kINST,
        kDATA,
        kNONE
    };

    Translation()
      : theVaddr(-1)
      , thePaddr(-1)
      , theTLBstatus(kTLBunresolved)
      , theTLBtype(kNONE)
      , theReady(false)
      , theWaiting(false)
      , theDone(false)
      , theCurrentTranslationLevel(0)
      , rawTTEValue(0)
      , theID(translationID++)
      , theAnnul(false)
      , theTimeoutCounter(0)
      , thePageFault(false)
      , inTraceMode(false)
      , theASID(0)
      , theNG(true)

    {
    }
    ~Translation() {}
    Translation(const Translation& aTr)
    {
        theVaddr          = aTr.theVaddr;
        thePSTATE         = aTr.thePSTATE;
        theType           = aTr.theType;
        thePaddr          = aTr.thePaddr;
        theException      = aTr.theException;
        theTTEEntry       = aTr.theTTEEntry;
        theTLBstatus      = aTr.theTLBstatus;
        theTLBtype        = aTr.theTLBtype;
        theTimeoutCounter = aTr.theTimeoutCounter;
        theReady          = aTr.theReady;
        theID             = aTr.theReady;
        theWaiting        = aTr.theReady;
        thePageFault      = aTr.thePageFault;
        trace_addresses   = aTr.trace_addresses;
        inTraceMode       = aTr.inTraceMode;
        theNG             = aTr.theNG;
        theASID           = aTr.theASID;
    }

    Translation& operator=(Translation& rhs)
    {
        theVaddr          = rhs.theVaddr;
        thePSTATE         = rhs.thePSTATE;
        theType           = rhs.theType;
        thePaddr          = rhs.thePaddr;
        theException      = rhs.theException;
        theTTEEntry       = rhs.theTTEEntry;
        theTLBstatus      = rhs.theTLBstatus;
        theTLBtype        = rhs.theTLBtype;
        theTimeoutCounter = rhs.theTimeoutCounter;
        theReady          = rhs.theReady;
        theID             = rhs.theID;
        theWaiting        = rhs.theReady;
        thePageFault      = rhs.thePageFault;
        trace_addresses   = rhs.trace_addresses;
        inTraceMode       = rhs.inTraceMode;
        theNG             = rhs.theNG;
        theASID           = rhs.theASID;

        return *this;
    }

    VirtualMemoryAddress theVaddr;
    PhysicalMemoryAddress thePaddr;

    int thePSTATE;
    uint32_t theIndex;
    eTranslationType theType;
    eTLBstatus theTLBstatus;
    eTLBtype theTLBtype;
    int theException;
    uint64_t theTTEEntry;

    bool theReady;   // ready for translation - step i
    bool theWaiting; // in memory - step ii
    bool theDone;    // all done step iii
    uint8_t theCurrentTranslationLevel;
    uint64_t rawTTEValue;
    uint64_t theID;
    bool theAnnul;
    uint64_t theTimeoutCounter;
    bool thePageFault;
    std::queue<PhysicalMemoryAddress> trace_addresses;
    bool inTraceMode;
    uint16_t theASID; // Address Space Identifier
    bool theNG; // non-global bit

    boost::intrusive_ptr<AbstractInstruction> theInstruction;

    void setData() { theTLBtype = kDATA; }
    void setInstr() { theTLBtype = kINST; }
    eTLBtype type() const { return theTLBtype; }
    bool isInstr() const { return type() == kINST; }
    bool isData() const { return type() == kDATA; }
    eTLBstatus status() const { return theTLBstatus; }
    bool isMiss() const { return status() == kTLBmiss; }
    bool isUnresuloved() { return status() == kTLBunresolved; }
    void setHit() { theTLBstatus = kTLBhit; }
    void setMiss() { theTLBstatus = kTLBmiss; }
    bool isHit() const { return theTLBstatus == kTLBhit; }
    bool isPagefault() { return thePageFault; }
    void setPagefault(bool p = true) { thePageFault = p; }

    void setInstruction(boost::intrusive_ptr<AbstractInstruction> anInstruction) { theInstruction = anInstruction; }
    boost::intrusive_ptr<AbstractInstruction> getInstruction() const { return theInstruction; }

    bool isWaiting() const { return theWaiting; }
    void toggleWaiting() { theWaiting = !theWaiting; }

    bool isReady() const { return theReady; }
    void toggleReady()
    {
        theReady = !theReady;
        DBG_(VVerb,
             (<< "toggling translation readiness (" << theReady << ") for " << theVaddr
              << " -- translation ID: " << theID));
    }

    bool isDone() const { return theDone; }
    void setDone(bool d = true) { theDone = d; }

    bool isAnnul() const { return theAnnul; }
    void setAnnul() { theAnnul = true; }

    void setNG(bool ng) { theNG = ng; }
    void setASID(uint16_t asid) { theASID = asid; }

    Translation& operator++(int)
    {
        if (++theTimeoutCounter > 10000) { DBG_(Crit, (<< "Translation timeout " << theVaddr << " id:" << theID)); }
        return *this;
    }
};

typedef boost::intrusive_ptr<Translation> TranslationPtr;

// MARK: Specialization of hasher and equality operator for translation object
struct TranslationPtrHasher
{
    std::size_t operator()(const TranslationPtr& transPtr) const { return transPtr->theID; }
};

struct TranslationPtrEqualityCheck
{
    bool operator()(const TranslationPtr& lhs, const TranslationPtr& rhs) const
    {
        return (lhs->theID == rhs->theID) && (lhs->theVaddr == rhs->theVaddr);
    }
};

} // namespace SharedTypes
} // namespace Flexus
#endif // FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED