#ifndef FLEXUS_uFETCH_TYPES_HPP_INCLUDED
#define FLEXUS_uFETCH_TYPES_HPP_INCLUDED

#include "components/CommonQEMU/Translation.hpp"
#include "core/types.hpp"

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
    VirtualMemoryAddress theActualTarget;

    eDirection thePrediction;
    eDirection theActualDirection;

    bool theTageHistoryValid;
    int phist;
    std::bitset<131> ghist; // Fixme: replace 131 with a correct macro
    unsigned ch_i[15];
    unsigned ch_t[2][15];

    bool theTagePredictionValid;
    int GI[15]; // 15 is random, upper bound on #tables?
    int BI;
    int bank;
    int altbank;
    bool pred_taken;
    bool alt_pred;

    uint32_t last_miss_distance;
    VirtualMemoryAddress ICache_miss_address;

    bool caused_ICache_miss;

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

    BPredState() {
      thePredictedType = kNonBranch;
      theActualType = kNonBranch;

      pc = VirtualMemoryAddress(0);
      thePredictedTarget = VirtualMemoryAddress(0);
      theActualTarget = VirtualMemoryAddress(0);

      thePrediction = kNotTaken;
      theActualDirection = kNotTaken;

      // There is no need to initialize the rest of the variables

      theTageHistoryValid = false;
      theTagePredictionValid = false;
    }
};

struct BPredRedictRequest : boost::counted_base
{
  VirtualMemoryAddress theTarget;
  boost::intrusive_ptr<BPredState> theBPState; // this might be NULL. If so, no history update is needed.
  bool theInsertNewHistory;                    // If true, insert a new history when recovering from a misprediction
};

struct FetchAddr
{
    VirtualMemoryAddress theAddress;
    boost::intrusive_ptr<BPredState> theBPState;
    FetchAddr(VirtualMemoryAddress anAddress)
      : theAddress(anAddress)
      , theBPState(new BPredState())
    {
      theBPState->theActualTarget = (uint64_t)anAddress + 4;
      theBPState->pc = anAddress;
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
