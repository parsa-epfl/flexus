#ifndef FLEXUS_uFETCH_TYPES_HPP_INCLUDED
#define FLEXUS_uFETCH_TYPES_HPP_INCLUDED

#include "components/CommonQEMU/Translation.hpp"

#include <unordered_map>
#include <unordered_set>

using boost::counted_base;
using Flexus::SharedTypes::TransactionTracker;
using Flexus::SharedTypes::Translation;
using Flexus::SharedTypes::VirtualMemoryAddress;

namespace Flexus::SharedTypes {

// =========== TYPEDEF ===================
typedef uint32_t Opcode;

struct FetchBundle;   // Forward declaration
struct FetchedOpcode; // Forward declaration
typedef boost::intrusive_ptr<FetchBundle> pFetchBundle;
typedef std::shared_ptr<FetchedOpcode> opcodeQIterator;
typedef std::unordered_map<TranslationPtr,
                           opcodeQIterator,            // K/V
                           TranslationPtrHasher,       // Custom hash object
                           TranslationPtrEqualityCheck // Custom equality comparison
                           >
  BijectionMapType_t;

typedef std::unordered_set<TranslationPtr,             // Key
                           TranslationPtrHasher,       // Custom hash
                           TranslationPtrEqualityCheck // Custom equality
                           >
  ExpectedTranslation_t;

// =========== ENUM ===================
enum eBranchType
{
    kNonBranch,
    kConditional,
    kUnconditional,
    kCall,
    kIndirectReg,
    kIndirectCall,
    kReturn,
    kLastBranchType,
    kBarrier
};

enum eDirection
{
    kStronglyTaken,
    kTaken,
    kNotTaken,
    kStronglyNotTaken
};

// =========== STRUCT ===================
struct BPredState : boost::counted_base
{
    eBranchType thePredictedType;
    eBranchType theActualType;

    VirtualMemoryAddress pc;
    VirtualMemoryAddress thePredictedTarget;
    VirtualMemoryAddress theNextPredictedTarget;

    eDirection thePrediction;
    eDirection theActualDirection;

    // TODO: Fix magic values
    unsigned ch_i[15];
    unsigned ch_t[2][15];

    int bank;
    int BI;
    int GI[15]; // 15 is random, upper bound on #tables?
    int altbank;
    int PWIN;
    int phist;

    std::bitset<131> ghist; // Fixme: replace 131 with a correct macro

    uint32_t last_miss_distance;
    VirtualMemoryAddress ICache_miss_address;

    bool caused_ICache_miss;
    bool pred_taken;
    bool alttaken;
    bool is_runahead;       // 1: if it is prediction from runahead path
    bool bimodalPrediction; // Is the final prediction from bimoal (in case of Tage)
    bool returnUsedRAS;     // Did the return instruction used RAS to get the return address
    bool returnPopRASTwice;
    bool callUpdatedRAS;
    bool detectedSpecialCall;
    bool haltDispatch;
    bool translationFailed;
    bool BTBPreFilled;
    bool causedSquash;
    bool hasRetired;
    // xExceptionSource exceptionSource;    // ! Probably needed in the future

    int8_t saturationCounter; // What is the counter value
    uint32_t theTL;
    uint32_t theBBSize;
    uint32_t theSerial;
};

struct BranchFeedback : boost::counted_base
{
    VirtualMemoryAddress thePC;
    eBranchType theActualType;
    eDirection theActualDirection;
    VirtualMemoryAddress theActualTarget;
    boost::intrusive_ptr<BPredState> theBPState;
};

struct FetchAddr
{
    VirtualMemoryAddress theAddress;
    boost::intrusive_ptr<BPredState> theBPState;
    FetchAddr(VirtualMemoryAddress anAddress)
      : theAddress(anAddress)
      , theBPState(new BPredState())
    {
    }
};

struct FetchCommand : boost::counted_base
{
    std::list<FetchAddr> theFetches;
};

struct FetchedOpcode
{
    VirtualMemoryAddress thePC;
    Opcode theOpcode;
    boost::intrusive_ptr<BPredState> theBPState;
    boost::intrusive_ptr<TransactionTracker> theTransaction;

    FetchedOpcode(Opcode anOpcode)
      : theOpcode(anOpcode)
    {
    }

    FetchedOpcode(VirtualMemoryAddress anAddr,
                  Opcode anOpcode,
                  boost::intrusive_ptr<BPredState> aBPState,
                  boost::intrusive_ptr<TransactionTracker> aTransaction)
      : thePC(anAddr)
      , theOpcode(anOpcode)
      , theBPState(aBPState)
      , theTransaction(aTransaction)
    {
    }
};

struct FetchBundle : public boost::counted_base
{
    std::list<std::shared_ptr<FetchedOpcode>> theOpcodes;
    std::list<std::shared_ptr<tFillLevel>>
      theFillLevels; // Level in memory hierarchy from where the instruction was fetched
    int32_t coreID;

    void updateOpcode(VirtualMemoryAddress anAddress, std::shared_ptr<FetchedOpcode> it, Opcode anOpcode)
    {
        DBG_AssertSev(Crit,
                      it->thePC == anAddress,
                      (<< "ERROR: FetchedOpcode iterator did not match!! Iterator PC " << it->thePC
                       << ", translation returned vaddr " << anAddress));
        it->theOpcode = anOpcode;
    }

    void clear()
    {
        theOpcodes.clear();
        theFillLevels.clear();
    }
};

} // FLEXUS::SHAREDTYPES

#endif
