/* Authors: Andr� Seznec and Pierre Michaud, January 2006

Code is essentially derived  from the tagged PPM predictor simulator from Pierre Michaud and the
OGEHL predictor simulator from Andr� Seznec

*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <bitset>
#include <string>
#include <iostream>
#include <inttypes.h>

#include <core/types.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include <core/checkpoint/json.hpp>
using json = nlohmann::json;

#define TAGE

#define ASSERT(cond)                                                                               \
  if (!(cond)) {                                                                                   \
    printf("assert line %d\n", __LINE__);                                                          \
    exit(EXIT_FAILURE);                                                                            \
  }

// the predictor features NHIST tagged components + a base bimodal component
// a tagged entry in tagged table Ti (0<= i< NHIST)  features a TBITS-(i+ (NHIST & 1))/2 tag, a
// CBITS prediction counter and a 2-bit useful counter. Tagged components feature 2**LOGG entries on
// the bimodal table: hysteresis is shared among 4 counters: total size 5*2**(LOGB-2)
// Remark a contrario from JILP paper, T0 is the table with the longest history, Sorry !

// 32KB
//#define BIG
#ifdef BIG
#define LOGB 14         // bimodal size
#define NHIST 12        // number of tables
#define TBITS 15        // tag bits
#define LOGG (LOGB - 4) // geo-table size
//#define LOGG 15//geo-table size
#define MAXHIST 640
#define MINHIST 4
#endif

//#define FIVECOMPONENT
#ifdef FIVECOMPONENT
// a 64 Kbits predictor with 4 tagged tables
#define LOGB 13
#define NHIST 4
#define TBITS 9
#define LOGG (LOGB - 3)
#endif

//#define FOURTEENCOMPONENT
#ifdef FOURTEENCOMPONENT
// a 64,5 Kbits predictor with 13 tagged tables
#define LOGB 13
#define NHIST 13
#define TBITS 15
#define LOGG (LOGB - 5)
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
//#define INITRAND
class folded_history {
public:
  unsigned comp;
  int CLENGTH;
  int OLENGTH;
  int OUTPOINT;

  folded_history() {
  }

  void init(int original_length, int compressed_length) {
    comp = 0;
    OLENGTH = original_length;
    CLENGTH = compressed_length;
    OUTPOINT = OLENGTH % CLENGTH;
    ASSERT(OLENGTH < MAXHIST);
  }

  void update(history_t h) {
    ASSERT((comp >> CLENGTH) == 0);
    comp = (comp << 1) | h[0];
    comp ^= h[OLENGTH] << OUTPOINT;
    comp ^= (comp >> CLENGTH);
    comp &= (1 << CLENGTH) - 1;
  }
};

// all the predictor is there

class PREDICTOR {
public:
  // bimodal table entry
  class bentry {
  public:
    int8_t hyst;
    int8_t pred;

    bentry() {

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
  class gentry {
  public:
    int8_t ctr;
    uint16_t tag;
    int8_t ubit;

    gentry() {

      ctr = 0;
      tag = 0;
      ubit = 0;

#ifdef INITRAND
      ctr = (random() & ((1 << CBITS) - 1)) - (1 << (CBITS - 1));
      tag = 0;
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
  int phist_runahead;
  int phist_retired;
  // use a path history as for the OGEHL predictor
  history_t ghist;
  history_t ghist_runahead;
  history_t ghist_retired;
  folded_history ch_i[NHIST];
  folded_history ch_t[2][NHIST];
  folded_history ch_i_runahead[NHIST];
  folded_history ch_t_runahead[2][NHIST];
  bentry *btable;
  gentry *gtable[NHIST];
  // used for storing the history lengths
  int m[NHIST];
  int Seed;
  PREDICTOR() {
    int STORAGESIZE = 0;

    PWIN = 0; // EPFL change
    Seed = 0;
    TICK = 0;

    phist = 0;
    phist_runahead = 0;
    phist_retired = 0;

    ghist = 0;
    ghist_runahead = 0;
    ghist_retired = 0;
    DBG_(Tmp, (<< " ghist ini: " << ghist));
    // computes the geometric history lengths
    m[0] = MAXHIST - 1;
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

      ch_i[i].init(m[i], (LOGG));
      ch_i_runahead[i].init(m[i], (LOGG));
      STORAGESIZE += (1 << LOGG) * (5 + TBITS - ((i + (NHIST & 1)) / 2));
    }
    fprintf(stderr, "\n");
    STORAGESIZE += (1 << LOGB) + /*(1 << (LOGB - 2))*/ (1 << LOGB);
    fprintf(stderr, "NHIST= %d; MINHIST= %d; MAXHIST= %d; STORAGESIZE= %d bits\n", NHIST, MINHIST,
            MAXHIST - 1, STORAGESIZE);

    for (int i = 0; i < NHIST; i++) {
      ch_t[0][i].init(ch_i[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2));
      ch_t[1][i].init(ch_i[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2) - 1);
      ch_t_runahead[0][i].init(ch_i_runahead[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2));
      ch_t_runahead[1][i].init(ch_i_runahead[i].OLENGTH, TBITS - ((i + (NHIST & 1)) / 2) - 1);
    }

    btable = new bentry[1 << LOGB];
    for (int i = 0; i < NHIST; i++) {
      gtable[i] = new gentry[1 << (LOGG)];
    }
  }

  ~PREDICTOR() {
  }

  // index function for the bimodal table

  int bindex(address_t pc) {

    return (pc & ((1 << (LOGB)) - 1));
  }

  // indexes to the different tables are computed only once  and store in GI and BI
  int GI[NHIST];
  int BI;

  // index function for the global tables:
  // includes path history as in the OGEHL predictor
  // F serves to mix path history
  int F(int A, int size, int bank) {
    int A1, A2;

    A = A & ((1 << size) - 1);
    A1 = (A & ((1 << LOGG) - 1));
    A2 = (A >> LOGG);
    A2 = ((A2 << bank) & ((1 << LOGG) - 1)) + (A2 >> (LOGG - bank));
    A = A1 ^ A2;
    A = ((A << bank) & ((1 << LOGG) - 1)) + (A >> (LOGG - bank));
    return (A);
  }
  int gindex(address_t pc, int bank, bool is_runahead) {
    int index;

    if (is_runahead) {
      if (m[bank] >= 16)
        index = pc ^ (pc >> ((LOGG - (NHIST - bank - 1)))) ^ ch_i_runahead[bank].comp ^
                F(phist_runahead, 16, bank);

      else
        index = pc ^ (pc >> (LOGG - NHIST + bank + 1)) ^ ch_i_runahead[bank].comp ^
                F(phist_runahead, m[bank], bank);

      return (index & ((1 << (LOGG)) - 1));

    } else {
      if (m[bank] >= 16)
        index = pc ^ (pc >> ((LOGG - (NHIST - bank - 1)))) ^ ch_i[bank].comp ^ F(phist, 16, bank);

      else
        index = pc ^ (pc >> (LOGG - NHIST + bank + 1)) ^ ch_i[bank].comp ^ F(phist, m[bank], bank);

      return (index & ((1 << (LOGG)) - 1));
    }
  }

  //  tag computation
  uint16_t gtag(address_t pc, int bank, bool is_runahead) {

    if (is_runahead) {
      int tag = pc ^ ch_t_runahead[0][bank].comp ^ (ch_t_runahead[1][bank].comp << 1);
      return (tag & ((1 << (TBITS - ((bank + (NHIST & 1)) / 2))) - 1));
    } else {
      int tag = pc ^ ch_t[0][bank].comp ^ (ch_t[1][bank].comp << 1);
      return (tag & ((1 << (TBITS - ((bank + (NHIST & 1)) / 2))) - 1));
    }
    // does not use the same length for all the components
  }

  // up-down saturating counter
  void ctrupdate(int8_t &ctr, bool taken, int nbits) {
    if (taken) {
      if (ctr < ((1 << (nbits - 1)) - 1)) {
        ctr++;
      }
    } else {
      if (ctr > -(1 << (nbits - 1))) {
        ctr--;
      }
    }
  }

  void reset_runahead_history() {

    for (int i = 0; i < NHIST; i++) {
      ch_i_runahead[i].comp = ch_i[i].comp;
      ch_t_runahead[0][i].comp = ch_t[0][i].comp;
      ch_t_runahead[1][i].comp = ch_t[1][i].comp;
    }
    phist_runahead = phist;
    ghist_runahead = std::bitset<MAXHIST>(ghist.to_string());
  }

  eDirection isCondTaken(uint64_t instruction_addr) {

    address_t pc = instruction_addr >> 2;

    // computes the table addresses
    int GI[NHIST];
    int BI;

    for (int i = 0; i < NHIST; i++)
      GI[i] = gindex(pc, i, 0);
    BI = bindex(pc);

    int bank = NHIST;

    for (int i = 0; i < NHIST; i++) {
      if (gtable[i][GI[i]].tag == gtag(pc, i, 0)) {
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

  int altbank;
  // prediction given by longest matching global history
  // altpred contains the alternate prediction
  bool read_prediction(address_t pc, int &bank, bool &altpred, BPredState &aBPState) {

    bank = NHIST;
    altbank = NHIST;

    {
      for (int i = 0; i < NHIST; i++) {
        if (gtable[i][GI[i]].tag == gtag(pc, i, aBPState.is_runahead)) {
          bank = i;
          break;
        }
      }
      for (int i = bank + 1; i < NHIST; i++) {
        if (gtable[i][GI[i]].tag == gtag(pc, i, aBPState.is_runahead)) {
          altbank = i;
          break;
        }
      }
      if (bank < NHIST) {
        if (altbank < NHIST)
          altpred = (gtable[altbank][GI[altbank]].ctr >= 0);
        else
          altpred = getbim(pc);
        // if the entry is recognized as a newly allocated entry and
        // counter PWIN is negative use the alternate prediction
        // see section 3.2.4
        //	  if ((PWIN < 0) || (abs (2 * gtable[bank][GI[bank]].ctr + 1) != 1)
        //	      || (gtable[bank][GI[bank]].ubit != 0))
        //
        //	    return (gtable[bank][GI[bank]].ctr >= 0);
        //	  else
        //	    return (altpred);
        //	  DBG_(Tmp, ( << "Tage history prediciton"));
        aBPState.bimodalPrediction = false;
        aBPState.saturationCounter =
            gtable[bank][GI[bank]].ctr + 4 /*To make the value positive (0 and 7) */;
        return (gtable[bank][GI[bank]].ctr >= 0);

      } else {
        altpred = getbim(pc);

        //	  DBG_(Tmp, ( << "Tage base prediciton"));
        aBPState.bimodalPrediction = true;
        aBPState.saturationCounter = getSatCounter();
        return altpred;
      }
    }
  }

  void update_retired_history(eBranchType theBranchType, bool taken, uint64_t instruction_addr) {
    ghist_retired = (ghist_retired << 1);
    if ((!(theBranchType == kConditional)) | (taken))
      ghist_retired |= (history_t)1;

    phist_retired = (phist_retired << 1) + (instruction_addr >> 2 & 1);
    phist_retired = (phist_retired & ((1 << 16) - 1));
  }

  void checkpoint_history(BPredState &aBPState) {
    // Save a checkpoint. We never reload a checkpoint from runahead state.

    if (aBPState.is_runahead) {
      assert(0);
      aBPState.bank = bank;
      aBPState.pred_taken = pred_taken;
      aBPState.alttaken = alttaken;
      aBPState.BI = BI;

      for (int i = 0; i < NHIST; i++) {
        aBPState.GI[i] = GI[i];
        aBPState.ch_i[i] = ch_i_runahead[i].comp;
        aBPState.ch_t[0][i] = ch_t_runahead[0][i].comp;
        aBPState.ch_t[1][i] = ch_t_runahead[1][i].comp;
      }
      aBPState.phist = phist_runahead;
      aBPState.ghist = std::bitset<MAXHIST>(ghist_runahead.to_string());
    } else {
      aBPState.bank = bank;
      aBPState.pred_taken = pred_taken;
      aBPState.alttaken = alttaken;
      aBPState.BI = BI;

      for (int i = 0; i < NHIST; i++) {
        aBPState.GI[i] = GI[i];
        aBPState.ch_i[i] = ch_i[i].comp;
        aBPState.ch_t[0][i] = ch_t[0][i].comp;
        aBPState.ch_t[1][i] = ch_t[1][i].comp;
      }
      aBPState.phist = phist;
      aBPState.ghist = std::bitset<MAXHIST>(ghist.to_string());
    }
  }

  void update_history(BPredState &aBPState, bool taken, uint64_t instruction_addr) {

    // Update the state
    if (aBPState.is_runahead) {
      assert(0);
      ghist_runahead = (ghist_runahead << 1);
      if ((!(aBPState.thePredictedType == kConditional)) | (taken))
        ghist_runahead |= (history_t)1;

      phist_runahead = (phist_runahead << 1) + (instruction_addr >> 2 & 1);
      phist_runahead = (phist_runahead & ((1 << 16) - 1));
      for (int i = 0; i < NHIST; i++) {
        ch_i_runahead[i].update(ghist_runahead);
        ch_t_runahead[0][i].update(ghist_runahead);
        ch_t_runahead[1][i].update(ghist_runahead);
      }

    } else {
      ghist = (ghist << 1);
      if ((!(aBPState.thePredictedType == kConditional)) | (taken))
        ghist |= (history_t)1;

      phist = (phist << 1) + (instruction_addr >> 2 & 1);
      phist = (phist & ((1 << 16) - 1));
      for (int i = 0; i < NHIST; i++) {
        ch_i[i].update(ghist);
        ch_t[0][i].update(ghist);
        ch_t[1][i].update(ghist);
      }
    }
  }

  // PREDICTION
  bool pred_taken, alttaken;
  int bank;
  bool get_prediction(uint64_t instruction_addr, BPredState &aBPState) {
    aBPState.saturationCounter = -1;
    if (aBPState.thePredictedType == kConditional) {

      address_t pc = instruction_addr >> 2;
      // computes the table addresses
      for (int i = 0; i < NHIST; i++)
        GI[i] = gindex(pc, i, aBPState.is_runahead);
      BI = bindex(pc);

      pred_taken = read_prediction(pc, bank, alttaken, aBPState);
      //	std::cout << "Tage Predict " << std::hex << instruction_addr <<std::endl;
      // bank contains the number of the matching table, NHIST if no match
      // pred_taken is the prediction
      // alttaken is the alternate prediction
      //	if (printLog)
      //		std::cout << "Predicting pc " << std::hex << instruction_addr << " pred " <<
      // pred_taken << " bank " << bank << " alttaken " << alttaken << std::endl; 	DBG_( Tmp,
      // ( << " TAGE prdict: " << std::hex << instruction_addr << " " << pred_taken));
    }

    // Save the checkpoint
    checkpoint_history(aBPState);

    // Speculatively update the history
    update_history(aBPState, pred_taken, instruction_addr);
    //	std::cout << std::endl<< std::endl<< std::endl<< std::endl << "Tage predict " << pred_taken
    //<< " cond? " << aBPState.is_conditional << std::endl<< std::endl<< std::endl<< std::endl<<
    // std::endl;

    return pred_taken;
  }
  bool getbim(address_t pc) {

    return (btable[BI].pred > 0);
  }

  int8_t getSatCounter() {
    return (btable[BI].pred << 1) + btable[BI].hyst;
  }
  // update  the bimodal predictor
  void baseupdate(address_t pc, bool Taken) {
    // just a normal 2-bit counter apart that hysteresis is shared
    if (Taken == getbim(pc)) {

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
        if (inter < 3)
          inter += 1;
      } else {
        if (inter > 0)
          inter--;
      }

      btable[BI].pred = inter >> 1;
      btable[BI].hyst = (inter & 1); // EPFL change
      //	btable[BI >> 2].hyst = (inter & 1);
    }
  }
  // just building our own simple pseudo random number generator based on linear feedback shift
  // register
  //  int Seed;

  int MYRANDOM() {
    Seed = ((1 << 2 * NHIST) + 1) * Seed + 0xf3f531;
    Seed = (Seed & ((1 << (2 * (NHIST))) - 1));
    return (Seed);
  }

  void confirm_state(BPredState &aBPState) {
    if (ghist_retired != aBPState.ghist) {
      DBG_(Tmp,
           (<< " Ghist is different retired: " << ghist_retired << " carried " << aBPState.ghist));
      assert(0);
    } else if (phist_retired != aBPState.phist) {
      DBG_(Tmp,
           (<< " Phist is different retired: " << phist_retired << " carried " << aBPState.phist));
      assert(0);
    }
  }

  void restore_retired_state() {

    phist = phist_retired;
    ghist = std::bitset<MAXHIST>(ghist_retired.to_string());
    for (int i = 0; i < NHIST; i++) {
      ch_i[i].update(ghist);
      ch_t[0][i].update(ghist);
      ch_t[1][i].update(ghist);
    }
  }

  void restore_state(BPredState &aBPState) {

    bank = aBPState.bank;
    pred_taken = aBPState.pred_taken;
    alttaken = aBPState.alttaken;
    BI = aBPState.BI;

    for (int i = 0; i < NHIST; i++) {
      GI[i] = aBPState.GI[i];
    }
  }

  void restore_all_state(BPredState &aBPState) {

    bank = aBPState.bank;
    pred_taken = aBPState.pred_taken;
    alttaken = aBPState.alttaken;
    BI = aBPState.BI;

    for (int i = 0; i < NHIST; i++) {
      GI[i] = aBPState.GI[i];
      ch_i[i].comp = aBPState.ch_i[i];
      ch_t[0][i].comp = aBPState.ch_t[0][i];
      ch_t[1][i].comp = aBPState.ch_t[1][i];
    }
    phist = aBPState.phist;
    ghist = std::bitset<MAXHIST>(aBPState.ghist.to_string());
  }

  // PREDICTOR UPDATE
  void update_predictor(uint64_t instruction_addr, BPredState &aBPState, bool taken) {

    //	  std::cout << std::endl<< std::endl<< std::endl<< std::endl << "Tage update " << taken <<
    // std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
    DBG_(Iface, (<< " TAGE feedback: " << std::hex << instruction_addr));
    if (aBPState.thePredictedType == kConditional) {
      int phist_back;
      history_t ghist_back;
      folded_history ch_i_back[NHIST];
      folded_history ch_t_back[2][NHIST];

      /*Save the current history*/
      phist_back = phist;
      ghist_back = std::bitset<MAXHIST>(ghist.to_string());
      for (int i = 0; i < NHIST; i++) {
        ch_i_back[i].comp = ch_i[i].comp;
        ch_t_back[0][i].comp = ch_t[0][i].comp;
        ch_t_back[1][i].comp = ch_t[1][i].comp;
      }
      /*Done*/
      /*Restore the history when the branch was predicted*/
      restore_all_state(aBPState);

      //    	if (printLog) {
      //			std::cout << "UPdate pc " << std::hex << instruction_addr << " pred
      //" << aBPState.pred_taken << " outcome " << taken << std::endl;
      // std::cout << "phist " << std::hex << aBPState.phist << " ghist " << aBPState.ghist << " BI
      // "
      // << aBPState.BI << " bank
      //"<< aBPState.bank << " altpred "<< aBPState.alttaken << std::endl;
      //    	}
      //

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

          if (loctaken == taken)
            ALLOC = false;
          // if the provider component  was delivering the correct prediction; no need to allocate a
          // new entry
          // even if the overall prediction was false

          // see section 3.2.4
          if (loctaken != alttaken) {
            if (alttaken == taken) {

              if (PWIN < 7)
                PWIN++;
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
          if (gtable[i][GI[i]].ubit < min)
            min = gtable[i][GI[i]].ubit;
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
          int Y = NRAND & ((1 << (bank - 1)) - 1);
          int X = bank - 1;
          while ((Y & 1) != 0) {
            X--;
            Y >>= 1;
          }

          for (int i = X; i >= 0; i--)

          {
            int T = i;
            if ((gtable[T][GI[T]].ubit == min)) {
              //		    		std::cout << "Bank alloc " << T << std::endl;

              gtable[T][GI[T]].tag = gtag(pc, T, 0 /*Not from runahead path*/);
              gtable[T][GI[T]].ctr = (taken) ? 0 : -1;
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
        if ((X & 1) == 0)
          X = 2;
        for (int i = 0; i < NHIST; i++)
          for (int j = 0; j < (1 << LOGG); j++)
            gtable[i][j].ubit = gtable[i][j].ubit & X;
      }

      // update the counter that provided the prediction, and only this counter
      if (bank < NHIST) {

        ctrupdate(gtable[bank][GI[bank]].ctr, taken, CBITS);
      } else {
        baseupdate(pc, taken);
      }
      // update the ubit counter
      if ((pred_taken != alttaken)) {
        ASSERT(bank < NHIST);

        if (pred_taken == taken) {

          if (gtable[bank][GI[bank]].ubit < 3)
            gtable[bank][GI[bank]].ubit++;

        } else {
          if (gtable[bank][GI[bank]].ubit > 0)
            gtable[bank][GI[bank]].ubit--;
        }
      }

      /* On a wrong prediction, update the history with correct values.
       * It could be a mis-prediction due to target miss in BTB and not because of wrong direction.
       * Therefore, we use "is_mispredict" variable instead of comparing the predicted and actual
       * direction. Moved to "feedback" function in BranchPredictor.cpp
       */
      //	if(is_mispredict) {
      //		update_history(aBPState, taken, instruction_addr);
      //	}
      /*Restore the current history*/
      phist = phist_back;
      ghist = std::bitset<MAXHIST>(ghist_back.to_string());
      for (int i = 0; i < NHIST; i++) {
        ch_i[i].comp = ch_i_back[i].comp;
        ch_t[0][i].comp = ch_t_back[0][i].comp;
        ch_t[1][i].comp = ch_t_back[1][i].comp;
      }

      /*Done*/
    }
    // update global history and cyclic shift registers
    // use also history on unconditional branches as for OGEHL predictors.

    //    ghist = (ghist << 1);
    //    if ((!br->is_conditional) | (taken))
    //      ghist |= (history_t) 1;
    //
    //    phist = (phist << 1) + (br->instruction_addr & 1);
    //    phist = (phist & ((1 << 16) - 1));
    //    for (int i = 0; i < NHIST; i++)
    //      {
    //	ch_i[i].update (ghist);
    //	ch_t[0][i].update (ghist);
    //	ch_t[1][i].update (ghist);
    //      }
  }

  json saveState() const {
    json checkpoint;

    checkpoint["PWIN"]    = PWIN;
    checkpoint["TICK"]    = TICK;
    checkpoint["Seed"]    = Seed;
    checkpoint["phist"]   = phist;
    checkpoint["ghist"]   = ghist.to_string();
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

        gtable_json["ctr"] = (int)gtable[i][j].ctr;
        gtable_json["tag"] = (int)gtable[i][j].tag;
        gtable_json["ubit"] = (int)gtable[i][j].ubit;

        gtable_array_json.push_back(gtable_json);
      }

      checkpoint["gtable"].push_back(gtable_array_json);
    };

    for (int i = 0; i < NHIST; i++) {
      json ch_i_json;

      ch_i_json["comp"] = (uint32_t)ch_i[i].comp;
      ch_i_json["c_length"] = (uint32_t)ch_i[i].CLENGTH;
      ch_i_json["o_length"] = (uint32_t)ch_i[i].OLENGTH;

      checkpoint["ch_i"].push_back(ch_i_json);
    }


    for (int j = 0; j < 2; j++) {
      json ch_t_array_json;

      for (int i = 0; i < NHIST; i++) {
        json ch_t_json;

        ch_t_json["comp"] = (uint32_t)ch_t[j][i].comp;
        ch_t_json["o_length"] = (uint32_t)ch_t[j][i].OLENGTH;
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

  void loadState(std::istream &s) {
    int logb, nhist, logg, tbits, maxhist, minhist, cbits;
    int a, b, c;
    s >> PWIN;
    s >> TICK;
    s >> Seed;
    s >> phist;
    s >> ghist;

    DBG_(Tmp, (<< " ghist reloaded: " << ghist));
    phist_retired = phist;
    ghist_retired = std::bitset<MAXHIST>(ghist.to_string());

    s >> logb;
    s >> nhist;
    s >> logg;
    s >> tbits;
    s >> maxhist;
    s >> minhist;
    s >> cbits;

    assert(LOGB == logb);
    assert(NHIST == nhist);
    assert(LOGG == logg);
    assert(TBITS == tbits);
    assert(MAXHIST == maxhist);
    assert(MINHIST == minhist);
    assert(CBITS == cbits);

    // bimodal table

    for (int i = 0; i < (1 << LOGB); i++) {
      //			s >> btable[i].hyst >> btable[i].pred;
      s >> a >> b;
      btable[i].hyst = a;
      btable[i].pred = b;
    }

    for (int i = 0; i < NHIST; i++) {
      for (int j = 0; j < (1 << LOGG); j++) {
        //			s >> gtable[i][j].ctr >>  gtable[i][j].tag >> gtable[i][j].ubit;
        s >> a >> b >> c;
        gtable[i][j].ctr = a;
        gtable[i][j].tag = b;
        gtable[i][j].ubit = c;
      }
    }

    for (int i = 0; i < NHIST; i++) {
      s >> ch_i[i].comp >> ch_i[i].CLENGTH >> ch_i[i].OLENGTH >> ch_i[i].OUTPOINT;
    }

    for (int i = 0; i < NHIST; i++) {
      s >> ch_t[0][i].comp >> ch_t[0][i].CLENGTH >> ch_t[0][i].OLENGTH >> ch_t[0][i].OUTPOINT;
    }

    for (int i = 0; i < NHIST; i++) {
      s >> ch_t[1][i].comp >> ch_t[1][i].CLENGTH >> ch_t[1][i].OLENGTH >> ch_t[1][i].OUTPOINT;
    }

    for (int i = 0; i < NHIST; i++) {
      s >> m[i];
    }
  }

  void print_state(std::ostream &s) const {
    s << PWIN << "\n";
    s << TICK << "\n";
    s << Seed << "\n";
    s << phist << "\n";
    s << ghist.to_string() << "\n";
    s << LOGB << "\n";
    s << NHIST << "\n";
    s << LOGG << "\n";
    s << TBITS << "\n";
    s << MAXHIST << "\n";
    s << MINHIST << "\n";
    s << CBITS << "\n";

    s << "\n btable";

    // bimodal table
    for (int i = 0; i < (1 << LOGB); i++) {
      s << (int)btable[i].hyst << " " << (int)btable[i].pred << "\n";
    }
    s << "\n gtable";

    for (int i = 0; i < NHIST; i++) {
      for (int j = 0; j < (1 << LOGG); j++) {
        s << (int)gtable[i][j].ctr << " " << (uint32_t)gtable[i][j].tag << " "
          << (int)gtable[i][j].ubit << "\n";
      }
      s << "\n next gtable";
    }

    s << "\n ch_i";

    for (int i = 0; i < NHIST; i++) {
      s << (uint32_t)ch_i[i].comp << " " << ch_i[i].CLENGTH << " " << ch_i[i].OLENGTH << " "
        << ch_i[i].OUTPOINT << "\n";
    }
    s << "\n ch_t1";

    for (int i = 0; i < NHIST; i++) {
      s << (uint32_t)ch_t[0][i].comp << " " << ch_t[0][i].CLENGTH << " " << ch_t[0][i].OLENGTH
        << " " << ch_t[0][i].OUTPOINT << "\n";
    }
    s << "\n ch_t2";

    for (int i = 0; i < NHIST; i++) {
      s << (uint32_t)ch_t[1][i].comp << " " << ch_t[1][i].CLENGTH << " " << ch_t[1][i].OLENGTH
        << " " << ch_t[1][i].OUTPOINT << "\n";
    }
    s << "\n m";

    for (int i = 0; i < NHIST; i++) {
      s << m[i] << " ";
    };
    s << "\n Done";
  }
};
} // namespace SharedTypes
} // namespace Flexus
#endif // PREDICTOR_H_SEEN