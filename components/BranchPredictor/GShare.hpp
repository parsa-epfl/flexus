#ifndef FLEXUS_BRANCHPREDICTOR_GSHARE_HPP_INCLUDED
#define FLEXUS_BRANCHPREDICTOR_GSHARE_HPP_INCLUDED

#include <components/uFetch/uFetchTypes.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Flexus::SharedTypes {

// A direction predictor with one untagged table of 2-bit saturating counters.
// History is speculative; counter training is a separate, commit-time operation.
class GShare
{
  private:
    uint64_t theHistoryMask;
    uint32_t theIndexMask;
    uint64_t theHistory = 0;
    std::vector<uint8_t> theCounters;

    static uint64_t historyMask(uint32_t historyLength)
    {
        if (historyLength > 64) {
            throw std::invalid_argument("GShare history length must be between 0 and 64 bits");
        }
        // Avoid shifting a 64-bit value by 64. Length zero gives a PC-only predictor.
        return historyLength == 64 ? std::numeric_limits<uint64_t>::max()
                                   : (uint64_t{1} << historyLength) - 1;
    }

    static uint32_t indexMask(uint32_t tableSize)
    {
        if (tableSize == 0 || (tableSize & (tableSize - 1)) != 0) {
            throw std::invalid_argument("GShare table size must be a positive power of two entries");
        }
        return tableSize - 1;
    }

  public:
    GShare(uint32_t historyLength, uint32_t tableSize)
      : theHistoryMask(historyMask(historyLength))
      , theIndexMask(indexMask(tableSize))
      , theCounters(tableSize, 1) // 0/1 predict not taken; 2/3 predict taken.
    {
    }

    void checkpointHistory(BPredState& aBPState) const
    {
        aBPState.theGShareHistory = theHistory;
        aBPState.theGShareHistoryValid = true;
    }

    bool get_prediction(uint64_t instruction_addr, BPredState& aBPState)
    {
        assert(aBPState.thePredictedType == kConditional);

        // Ignore AArch64 instruction-alignment bits. Longer histories contribute
        // only the low index-width bits; there is no tag or history folding.
        aBPState.theGShareIndex = ((instruction_addr >> 2) ^ theHistory) & theIndexMask;
        const uint8_t counter = theCounters[aBPState.theGShareIndex];
        aBPState.saturationCounter = static_cast<int8_t>(counter);
        aBPState.pred_taken = counter >= 2;
        aBPState.theGSharePredictionValid = true;

        update_history(aBPState, aBPState.pred_taken, instruction_addr);
        return aBPState.pred_taken;
    }

    void update_history(const BPredState&, bool taken, uint64_t)
    {
        // The wrapper passes the predicted/actual conditional direction, and
        // taken for other recognized branches, just as for the TAGE history.
        theHistory = ((theHistory << 1) | static_cast<uint64_t>(taken)) & theHistoryMask;
    }

    void restore_history(const BPredState& aBPState)
    {
        if (aBPState.theGShareHistoryValid) {
            theHistory = aBPState.theGShareHistory & theHistoryMask;
        }
    }

    void update_predictor(uint64_t, const BPredState& aBPState, bool taken)
    {
        if (aBPState.thePredictedType != kConditional || !aBPState.theGSharePredictionValid) {
            return;
        }

        // Train the entry selected at lookup, even when younger predictions
        // have changed history. Training must not append history a second time.
        assert(aBPState.theGShareIndex < theCounters.size());
        uint8_t& counter = theCounters[aBPState.theGShareIndex];
        if (taken) {
            if (counter < 3) ++counter;
        } else if (counter > 0) {
            --counter;
        }
    }

    void reset()
    {
        theHistory = 0;
        std::fill(theCounters.begin(), theCounters.end(), uint8_t{1});
    }
};

} // namespace Flexus::SharedTypes

#endif // FLEXUS_BRANCHPREDICTOR_GSHARE_HPP_INCLUDED
