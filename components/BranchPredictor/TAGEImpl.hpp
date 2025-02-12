/* Authors: Andr� Seznec and Pierre Michaud, January 2006

Code is essentially derived  from the tagged PPM predictor simulator from Pierre Michaud and the
OGEHL predictor simulator from Andr� Seznec

*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include "core/debug/debug.hpp"
#include <bitset>
#include <cmath>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/checkpoint/json.hpp>
#include <core/types.hpp>
#include <cstdlib>
#include <inttypes.h>
using json = nlohmann::json;

#define TAGE

#define ASSERT(cond)                                                                                                   \
    if (!(cond)) {                                                                                                     \
        printf("assert line %d\n", __LINE__);                                                                          \
        abort();                                                                                            \
    }

// the predictor features NHIST tagged components + a base bimodal component
// a tagged entry in tagged table Ti (0<= i< NHIST)  features a TBITS-(i+ (NHIST & 1))/2 tag, a
// CBITS prediction counter and a 2-bit useful counter. Tagged components feature 2**LOGG entries on
// the bimodal table: hysteresis is shared among 4 counters: total size 5*2**(LOGB-2)
// Remark a contrario from JILP paper, T0 is the table with the longest history, Sorry !

// 32KB
// #define BIG
#ifdef BIG
#define LOGB  14         // bimodal size
#define NHIST 12         // number of tables
#define TBITS 15         // tag bits
#define LOGG  (LOGB - 4) // geo-table size
// #define LOGG 15//geo-table size
#define MAXHIST 640
#define MINHIST 4
#endif

// #define FIVECOMPONENT
#ifdef FIVECOMPONENT
// a 64 Kbits predictor with 4 tagged tables
#define LOGB  13
#define NHIST 4
#define TBITS 9
#define LOGG  (LOGB - 3)
#endif

// #define FOURTEENCOMPONENT
#ifdef FOURTEENCOMPONENT
// a 64,5 Kbits predictor with 13 tagged tables
#define LOGB  13
#define NHIST 13
#define TBITS 15
#define LOGG  (LOGB - 5)
#endif

// bits per counter in the global history tables
#ifndef CBITS
#define CBITS 3
#endif

// the default predictor
// by default a 63.5  Kbits predictor, featuring 7 tagged components and a base bimodal component:
// NHIST = 7, LOGB =13, LOGG=9, CBITS=3
// 10 Kbits for the bimodal table.
// 8.5 Kbits for T0
// 8 Kbits  for T1 and T2
// 7.5 Kbits for T3 and T4
// 7 Kbits for T5 and T6

#ifndef LOGB
#define LOGB 13
#endif

#ifndef NHIST
#define NHIST 7
#endif
// base 2 logarithm of number of entries  on each tagged component
#ifndef LOGG
#define LOGG (LOGB - 4)
#endif

// Total width of an entry in the tagged table with the longest history length
#ifndef TBITS
#define TBITS 12
#endif

// AS: we use Geometric history length
// AS: maximum global history length used and minimum history length
#ifndef MAXHIST
#define MAXHIST 131
#define MINHIST 5
#endif

typedef uint64_t address_t;
typedef std::bitset<MAXHIST> history_t;

namespace Flexus {
namespace SharedTypes {

// this is the cyclic shift register for folding
// a long global history into a smaller number of bits
// #define INITRAND
class folded_history
{
  public:
    unsigned comp;
    int CLENGTH;
    int OLENGTH;
    int OUTPOINT;

    folded_history() {}

    void init(int original_length, int compressed_length)
    {
        comp     = 0;
        OLENGTH  = original_length;
        CLENGTH  = compressed_length;
        OUTPOINT = OLENGTH % CLENGTH;
        ASSERT(OLENGTH < MAXHIST);
    }

    void update(history_t h)
    {
        ASSERT((comp >> CLENGTH) == 0);
        comp = (comp << 1) | h[0];
        comp ^= h[OLENGTH] << OUTPOINT;
        comp ^= (comp >> CLENGTH);
        comp &= (1 << CLENGTH) - 1;
    }
};

// all the predictor is there

class PREDICTOR
{
  public:
    // bimodal table entry
    class bentry
    {
      public:
        int8_t hyst;
        int8_t pred;

        bentry()
        {

            pred = 0;
            //    hyst = 0;
            hyst = 1;

#ifdef INITRAND

            pred = random() & 1;
            hyst = random() & 1;
#endif
        }
    };

    // global table entry
    class gentry
    {
      public:
        int8_t ctr;
        uint16_t tag;
        int8_t ubit;

        gentry()
        {

            ctr  = 0;
            tag  = 0;
            ubit = 0;

#ifdef INITRAND
            ctr  = (random() & ((1 << CBITS) - 1)) - (1 << (CBITS - 1));
            tag  = 0;
            ubit = (random() & 3);

#endif
        }
    };

    // predictor storage data
    int PWIN;
    // 4 bits to determine whether newly allocated entries should be considered as
    // valid or not for delivering  the prediction
    int TICK;
    int phist;
    // use a path history as for the OGEHL predictor
    history_t ghist;
    history_t ghist_retired;
    folded_history ch_i[NHIST];
    folded_history ch_t[2][NHIST];
    bentry* btable;
    gentry* gtable[NHIST];
    // used for storing the history lengths
    int m[NHIST];
    int Seed;
    PREDICTOR()
    {
        int STORAGESIZE = 0;

        PWIN = 0; // EPFL change
        Seed = 0;
        TICK = 0;

        phist          = 0;

        ghist          = 0;
        ghist_retired  = 0;
        // computes the geometric history lengths
        m[0]         = MAXHIST - 1;
        m[NHIST - 1] = MINHIST;
        for (int i = 1; i < NHIST - 1; i++) {
            m[NHIST - 1 - i] = (int)(((double)MINHIST * pow((double)(MAXHIST - 1) / (double)MINHIST,
                                                            (double)(i) / (double)((NHIST - 1)))) +
                                     0.5);
        }

        fprintf(stderr, "History Series:");
        STORAGESIZE = 0;

        for (int i = NHIST - 1; i >= 0; i--) {

            fprintf(stderr, "%d ", m[i]);

            ch_i[i].init(m[i], LOGG);
            STORAGESIZE += (1 << LOGG) * (5 + TBITS - ((i + (NHIST & 1)) / 2));
        }
        fprintf(stderr, "\n");
        STORAGESIZE += (1 << LOGB) + /*(1 << (LOGB - 2))*/ (1 << LOGB);
        fprintf(stderr,
                "NHIST= %d; MINHIST= %d; MAXHIST= %d; STORAGESIZE= %d bits\n",
                NHIST,
                MINHIST,
                MAXHIST - 1,
                STORAGESIZE);

        for (int i = 0; i < NHIST; i++) {
            ch_t[0][i].init(ch_i[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2));
            ch_t[1][i].init(ch_i[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2) - 1);
        }

        btable = new bentry[1 << LOGB];
        for (int i = 0; i < NHIST; i++) {
            gtable[i] = new gentry[1 << (LOGG)];
        }
    }

    ~PREDICTOR() {}

    // index function for the bimodal table

    int bindex(address_t pc) { return (pc & ((1 << (LOGB)) - 1)); }

    // index function for the global tables:
    // includes path history as in the OGEHL predictor
    // F serves to mix path history
    int F(int A, int size, int bank)
    {
        int A1, A2;

        A  = A & ((1 << size) - 1);
        A1 = (A & ((1 << LOGG) - 1));
        A2 = (A >> LOGG);
        A2 = ((A2 << bank) & ((1 << LOGG) - 1)) + (A2 >> (LOGG - bank));
        A  = A1 ^ A2;
        A  = ((A << bank) & ((1 << LOGG) - 1)) + (A >> (LOGG - bank));
        return (A);
    }
    int gindex(address_t pc, int bank)
    {
        int index;

        if (m[bank] >= 16)
            index = pc ^ (pc >> ((LOGG - (NHIST - bank - 1)))) ^ ch_i[bank].comp ^ F(phist, 16, bank);

        else
            index = pc ^ (pc >> (LOGG - NHIST + bank + 1)) ^ ch_i[bank].comp ^ F(phist, m[bank], bank);

        return (index & ((1 << (LOGG)) - 1));
    }

    //  tag computation
    uint16_t gtag(address_t pc, int bank)
    {

        int tag = pc ^ ch_t[0][bank].comp ^ (ch_t[1][bank].comp << 1);
        return (tag & ((1 << (TBITS - ((bank + (NHIST & 1)) / 2))) - 1));
        // does not use the same length for all the components
    }

    // up-down saturating counter
    void ctrupdate(int8_t& ctr, bool taken, int nbits)
    {
        if (taken) {
            if (ctr < ((1 << (nbits - 1)) - 1)) { ctr++; }
        } else {
            if (ctr > -(1 << (nbits - 1))) { ctr--; }
        }
    }

    eDirection isCondTaken(uint64_t instruction_addr)
    {

        address_t pc = instruction_addr >> 2;

        // computes the table addresses
        int GI[NHIST];
        int BI;

        for (int i = 0; i < NHIST; i++)
            GI[i] = gindex(pc, i);
        BI = bindex(pc);

        int bank = NHIST;

        for (int i = 0; i < NHIST; i++) {
            if (gtable[i][GI[i]].tag == gtag(pc, i)) {
                bank = i;
                break;
            }
        }

        if (bank < NHIST) {

            //		  return (gtable[bank][GI[bank]].ctr >= 0);
            //	    	DBG_( Tmp, ( << " Tage predict: " << (int)gtable[bank][GI[bank]].ctr));
            int8_t ctr = gtable[bank][GI[bank]].ctr;
            if (ctr == 3 || ctr == 2) {
                return kStronglyTaken;
            } else if (ctr == 1 || ctr == 0) {
                return kTaken;
            } else if (ctr == -1 || ctr == -2) {
                return kNotTaken;
            } else {
                return kStronglyNotTaken;
                assert(ctr == -3 || ctr == -4);
            }

        } else {
            //		  return btable[BI].pred > 0;
            //	    	  DBG_( Tmp, ( << " Base predict: " << (int)btable[BI].pred << " " <<
            //(int)btable[BI].hyst));
            int8_t ctr = ((btable[BI].pred << 1) + btable[BI].hyst);
            if (ctr == 3) {
                return kStronglyTaken;
            } else if (ctr == 2) {
                return kTaken;
            } else if (ctr == 1) {
                return kNotTaken;
            } else {
                return kStronglyNotTaken;
                assert(ctr == 0);
            }
        }
        return kNotTaken; // Mark: Added
    }

    // prediction given by longest matching global history
    // altpred contains the alternate prediction
    bool read_prediction(address_t pc, int& bank, bool& altpred, BPredState& aBPState)
    {
        aBPState.bank = NHIST;
        aBPState.altbank = NHIST;

        {
            for (int i = 0; i < NHIST; i++) {
                if (gtable[i][aBPState.GI[i]].tag == gtag(pc, i)) {
                    bank = i;
                    break;
                }
            }
            for (int i = bank + 1; i < NHIST; i++) {
                if (gtable[i][aBPState.GI[i]].tag == gtag(pc, i)) {
                    aBPState.altbank = i;
                    break;
                }
            }
            if (bank < NHIST) {
                if (aBPState.altbank < NHIST)
                    altpred = (gtable[aBPState.altbank][aBPState.GI[aBPState.altbank]].ctr >= 0);
                else
                    altpred = getbim(pc, aBPState.BI);
                aBPState.bimodalPrediction = false;
                aBPState.saturationCounter = gtable[bank][aBPState.GI[bank]].ctr + 4 /*To make the value positive (0 and 7) */;
                return (gtable[bank][aBPState.GI[bank]].ctr >= 0);

            } else {
                altpred = getbim(pc, aBPState.BI);

                aBPState.bimodalPrediction = true;
                aBPState.saturationCounter = getSatCounter(aBPState.BI);
                return altpred;
            }
        }
    }

    void checkpointHistory(BPredState& aBPState) const
    {
        // This checkpoint only saves the global history and path history
        DBG_Assert(aBPState.theTageHistoryValid == false);
        aBPState.phist = phist;
        aBPState.ghist = std::bitset<MAXHIST>(ghist.to_string());

        // Checkpoint ch_i and ch_t. They are the function of the global history.
        for (int i = 0; i < NHIST; i++) {
            aBPState.ch_i[i] = ch_i[i].comp;
            aBPState.ch_t[0][i] = ch_t[0][i].comp;
            aBPState.ch_t[1][i] = ch_t[1][i].comp;
        }

        aBPState.theTageHistoryValid = true;
    }

    void update_history(const BPredState& aBPState, bool taken, uint64_t instruction_addr)
    {
        // TODO: Check whether this function is called for non-conditional branches.
        // Update the state
        ghist = (ghist << 1);
        if ((!(aBPState.thePredictedType == kConditional)) | (taken)) ghist |= (history_t)1;

        phist = (phist << 1) + (instruction_addr >> 2 & 1);
        phist = (phist & ((1 << 16) - 1));
        for (int i = 0; i < NHIST; i++) {
            ch_i[i].update(ghist);
            ch_t[0][i].update(ghist);
            ch_t[1][i].update(ghist);
        }
    }

    // PREDICTION
    bool get_prediction(uint64_t instruction_addr, BPredState& aBPState)
    {
        aBPState.saturationCounter = -1;
        if (aBPState.thePredictedType == kConditional) {

            address_t pc = instruction_addr >> 2;
            // computes the table addresses
            for (int i = 0; i < NHIST; i++)
                aBPState.GI[i] = gindex(pc, i);
            aBPState.BI = bindex(pc);

            aBPState.pred_taken = read_prediction(pc, aBPState.bank, aBPState.alt_pred, aBPState);

            update_history(aBPState, aBPState.pred_taken, instruction_addr);

            aBPState.theTagePredictionValid = true;

            return aBPState.pred_taken;
        }

        // No way to predict the direction of a non-conditional branch
        DBG_Assert(false);
        __builtin_unreachable();
    }

    bool getbim(address_t pc, int BI) { return (btable[BI].pred > 0); }

    int8_t getSatCounter(int BI) { return (btable[BI].pred << 1) + btable[BI].hyst; }
    // update  the bimodal predictor
    void baseupdate(address_t pc, bool Taken, int BI)
    {
        // just a normal 2-bit counter apart that hysteresis is shared
        if (Taken == getbim(pc, BI)) {

            if (Taken) {
                if (btable[BI].pred)
                    //	      btable[BI >> 2].hyst = 1;
                    btable[BI].hyst = 1; // EPFL change
            } else {
                if (!btable[BI].pred)
                    //	      btable[BI >> 2].hyst = 0;
                    btable[BI].hyst = 0; // EPFL change
            }
        } else {
            //	int inter = (btable[BI].pred << 1) + btable[BI >> 2].hyst;
            int inter = (btable[BI].pred << 1) + btable[BI].hyst; // EPFL change
            if (Taken) {
                if (inter < 3) inter += 1;
            } else {
                if (inter > 0) inter--;
            }

            btable[BI].pred = inter >> 1;
            btable[BI].hyst = (inter & 1); // EPFL change
                                           //	btable[BI >> 2].hyst = (inter & 1);
        }
    }
    // just building our own simple pseudo random number generator based on linear feedback shift
    // register
    //  int Seed;

    int MYRANDOM()
    {
        Seed = ((1 << 2 * NHIST) + 1) * Seed + 0xf3f531;
        Seed = (Seed & ((1 << (2 * (NHIST))) - 1));
        return (Seed);
    }

    void restore_history(const BPredState& aBPState)
    {

        DBG_Assert(aBPState.theTageHistoryValid);

        for (int i = 0; i < NHIST; i++) {
            ch_i[i].comp    = aBPState.ch_i[i];
            DBG_Assert((ch_i[i].comp >> ch_i[i].CLENGTH) == 0);
            ch_t[0][i].comp = aBPState.ch_t[0][i];
            DBG_Assert((ch_t[0][i].comp >> ch_t[0][i].CLENGTH) == 0);
            ch_t[1][i].comp = aBPState.ch_t[1][i];
            DBG_Assert((ch_t[1][i].comp >> ch_t[0][i].CLENGTH) == 0);
        }

        phist = aBPState.phist;
        ghist = std::bitset<MAXHIST>(aBPState.ghist.to_string());
    }

    // PREDICTOR UPDATE
    void update_predictor(uint64_t instruction_addr, const BPredState& aBPState, bool taken)
    {

        //	  std::cout << std::endl<< std::endl<< std::endl<< std::endl << "Tage update " << taken <<
        // std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
        DBG_(VVerb, (<< " TAGE feedback: " << std::hex << instruction_addr));
        if (aBPState.thePredictedType == kConditional) {
            int phist_back;
            history_t ghist_back;
            folded_history ch_i_back[NHIST];
            folded_history ch_t_back[2][NHIST];

            /*Save the current history*/
            phist_back = phist;
            ghist_back = std::bitset<MAXHIST>(ghist.to_string());
            for (int i = 0; i < NHIST; i++) {
                ch_i_back[i].comp    = ch_i[i].comp;
                ch_t_back[0][i].comp = ch_t[0][i].comp;
                ch_t_back[1][i].comp = ch_t[1][i].comp;
            }
            /*Done*/
            /*Restore the history when the branch was predicted*/
            restore_history(aBPState);

            // GI, BI, bank, altbank, pred_taken, alt_pred
            int GI[NHIST];
            int BI;
            int bank;
            bool alt_pred;
            bool pred_taken;

            if (aBPState.theTagePredictionValid) {
                for (int i = 0; i < NHIST; i++)
                    GI[i] = aBPState.GI[i];
                BI        = aBPState.BI;
                bank      = aBPState.bank;
                alt_pred  = aBPState.alt_pred;
                pred_taken = aBPState.pred_taken;
            } else {
                // We need to recompute the indices.
                DBG_Assert(aBPState.thePredictedType != kConditional);
                for (int i = 0; i < NHIST; i++)
                    GI[i] = gindex(instruction_addr >> 2, i);
                BI = bindex(instruction_addr >> 2);
                bank = NHIST;
                alt_pred = aBPState.thePrediction == kTaken;
                pred_taken = aBPState.thePrediction == kTaken;
            }

            address_t pc = instruction_addr >> 2;

            /* in a real processor, it is not necessary to re-read the predictor at update
             * it suffices to propagate the prediction along with the branch instruction
             * We only check if the direction was correct and not the target as Tage only prediction the
             * direction. If the direction was correct but not the target, Tage did its job and we do not
             * was to allocate another entry.
             */
            bool ALLOC = ((pred_taken != taken) & (bank > 0));

            if (bank < NHIST) {
                bool loctaken = (gtable[bank][GI[bank]].ctr >= 0);
                bool PseudoNewAlloc =
                  (abs(2 * gtable[bank][GI[bank]].ctr + 1) == 1) && (gtable[bank][GI[bank]].ubit == 0);
                // is entry "pseudo-new allocated"

                if (PseudoNewAlloc) {

                    if (loctaken == taken) ALLOC = false;
                    // if the provider component  was delivering the correct prediction; no need to allocate a
                    // new entry
                    // even if the overall prediction was false

                    // see section 3.2.4
                    if (loctaken != alt_pred) {
                        if (alt_pred == taken) {

                            if (PWIN < 7) PWIN++;
                        }

                        else if (PWIN > -8)
                            PWIN--;
                    }
                }
            }

            // try to allocate a  new entries only if prediction was wrong
            if (ALLOC) {

                assert(pred_taken != taken);
                // is there some "unuseful" entry to allocate
                int8_t min = 3;
                for (int i = 0; i < bank; i++) {
                    if (gtable[i][GI[i]].ubit < min) min = gtable[i][GI[i]].ubit;
                }

                if (min > 0) {

                    // NO UNUSEFUL ENTRY TO ALLOCATE: age all possible targets, but do not allocate
                    for (int i = bank - 1; i >= 0; i--) {
                        gtable[i][GI[i]].ubit--;
                    }

                } else {
                    // YES: allocate one entry, but apply some randomness
                    // bank I is twice more probable than bank I-1

                    int NRAND = MYRANDOM();
                    int Y     = NRAND & ((1 << (bank - 1)) - 1);
                    int X     = bank - 1;
                    while ((Y & 1) != 0) {
                        X--;
                        Y >>= 1;
                    }

                    for (int i = X; i >= 0; i--)

                    {
                        int T = i;
                        if (gtable[T][GI[T]].ubit == min) {
                            //		    		std::cout << "Bank alloc " << T << std::endl;

                            gtable[T][GI[T]].tag  = gtag(pc, T);
                            gtable[T][GI[T]].ctr  = (taken) ? 0 : -1;
                            gtable[T][GI[T]].ubit = 0;
                            break;
                        }
                    }
                }
            }

            // periodic reset of ubit: reset is not complete but bit by bit
            TICK++;
            if ((TICK & ((1 << 18) - 1)) == 0) {
                int X = (TICK >> 18) & 1;
                if ((X & 1) == 0) X = 2;
                for (int i = 0; i < NHIST; i++)
                    for (int j = 0; j < (1 << LOGG); j++)
                        gtable[i][j].ubit = gtable[i][j].ubit & X;
            }

            // update the counter that provided the prediction, and only this counter
            if (bank < NHIST) {
                ctrupdate(gtable[bank][GI[bank]].ctr, taken, CBITS);
            } else {
                baseupdate(pc, taken, BI);
            }
            // update the ubit counter
            if ((pred_taken != alt_pred)) {
                ASSERT(bank < NHIST);

                if (pred_taken == taken) {

                    if (gtable[bank][GI[bank]].ubit < 3) gtable[bank][GI[bank]].ubit++;

                } else {
                    if (gtable[bank][GI[bank]].ubit > 0) gtable[bank][GI[bank]].ubit--;
                }
            }

            /*Restore the current history*/
            phist = phist_back;
            ghist = std::bitset<MAXHIST>(ghist_back.to_string());
            for (int i = 0; i < NHIST; i++) {
                ch_i[i].comp    = ch_i_back[i].comp;
                ch_t[0][i].comp = ch_t_back[0][i].comp;
                ch_t[1][i].comp = ch_t_back[1][i].comp;
            }

            /*Done*/
        }
    }

    json saveState() const
    {
        json checkpoint;

        checkpoint["PWIN"]    = PWIN;
        checkpoint["TICK"]    = TICK;
        checkpoint["SEED"]    = Seed;
        checkpoint["PHIST"]   = phist;
        checkpoint["GHIST"]   = ghist.to_string();
        checkpoint["LOGB"]    = LOGB;
        checkpoint["NHIST"]   = NHIST;
        checkpoint["LOGG"]    = LOGG;
        checkpoint["TBITS"]   = TBITS;
        checkpoint["MAXHIST"] = MAXHIST;
        checkpoint["MINHIST"] = MINHIST;
        checkpoint["CBITS"]   = CBITS;

        // bimodal table
        for (int i = 0; i < (1 << LOGB); i++) {
            json btable_json;

            btable_json["hyst"] = (int)btable[i].hyst;
            btable_json["pred"] = (int)btable[i].pred;

            checkpoint["btable"].push_back(btable_json);
        }

        for (int i = 0; i < NHIST; i++) {
            json gtable_array_json;

            for (int j = 0; j < (1 << LOGG); j++) {
                json gtable_json;

                gtable_json["ctr"]  = (int)gtable[i][j].ctr;
                gtable_json["tag"]  = (int)gtable[i][j].tag;
                gtable_json["ubit"] = (int)gtable[i][j].ubit;

                gtable_array_json.push_back(gtable_json);
            }

            checkpoint["gtable"].push_back(gtable_array_json);
        };

        for (int i = 0; i < NHIST; i++) {
            json ch_i_json;

            ch_i_json["comp"]     = (uint32_t)ch_i[i].comp;
            ch_i_json["c_length"] = (uint32_t)ch_i[i].CLENGTH;
            ch_i_json["o_length"] = (uint32_t)ch_i[i].OLENGTH;

            checkpoint["ch_i"].push_back(ch_i_json);
        }

        for (int j = 0; j < 2; j++) {
            json ch_t_array_json;

            for (int i = 0; i < NHIST; i++) {
                json ch_t_json;

                ch_t_json["comp"]      = (uint32_t)ch_t[j][i].comp;
                ch_t_json["o_length"]  = (uint32_t)ch_t[j][i].OLENGTH;
                ch_t_json["out_point"] = (uint32_t)ch_t[j][i].OUTPOINT;

                ch_t_array_json.push_back(ch_t_json);
            }

            checkpoint["ch_t"].push_back(ch_t_array_json);
        }

        for (int i = 0; i < NHIST; i++) {
            checkpoint["m"].push_back(m[i]);
        };

        return checkpoint;
    }

    void loadState(json checkpoint)
    {

        DBG_Assert(LOGB == checkpoint["LOGB"]);
        DBG_Assert(NHIST == checkpoint["NHIST"]);
        DBG_Assert(LOGG == checkpoint["LOGG"]);
        DBG_Assert(TBITS == checkpoint["TBITS"]);
        DBG_Assert(MAXHIST == checkpoint["MAXHIST"]);
        DBG_Assert(MINHIST == checkpoint["MINHIST"]);
        DBG_Assert(CBITS == checkpoint["CBITS"]);

        PWIN  = checkpoint["PWIN"];
        TICK  = checkpoint["TICK"];
        Seed  = checkpoint["SEED"];
        phist = checkpoint["PHIST"];
        ghist = history_t(checkpoint["GHIST"].get<std::string>());

        // bimodal table
        for (int i = 0; i < (1 << LOGB); i++) {
            btable[i].hyst = checkpoint["btable"][i]["hyst"];
            btable[i].pred = checkpoint["btable"][i]["pred"];
        }

        for (int i = 0; i < NHIST; i++) {
            for (int j = 0; j < (1 << LOGG); j++) {
                gtable[i][j].ctr  = checkpoint["gtable"][i][j]["ctr"];
                gtable[i][j].tag  = checkpoint["gtable"][i][j]["tag"];
                gtable[i][j].ubit = checkpoint["gtable"][i][j]["ubit"];
            }
        };

        for (int i = 0; i < NHIST; i++) {
            ch_i[i].comp    = checkpoint["ch_i"][i]["comp"];
            ch_i[i].CLENGTH = checkpoint["ch_i"][i]["c_length"];
            ch_i[i].OLENGTH = checkpoint["ch_i"][i]["o_length"];
        }

        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < NHIST; i++) {
                ch_t[j][i].comp     = checkpoint["ch_t"][j][i]["comp"];
                ch_t[j][i].OLENGTH  = checkpoint["ch_t"][j][i]["o_length"];
                ch_t[j][i].OUTPOINT = checkpoint["ch_t"][j][i]["out_point"];
            }
        }

        for (int i = 0; i < NHIST; i++) {
            m[i] = checkpoint["m"][i];
        }
    };
}; // namespace SharedTypes
} // namespace Flexus
}
#endif // PREDICTOR_H_SEEN
