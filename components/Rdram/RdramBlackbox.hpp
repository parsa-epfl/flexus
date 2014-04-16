/*! \file RdramBlackbox.hpp
 * \brief
 *
 *  Brief description goes here.
 *
 * Revision History:
 *     cfchen      23 Apr 03 - Initial Revision
 */

#include <deque>
#include <vector>
#include <iterator>
#include "RdramMagic.hpp"  // for rdram_bank_t and other strange things
// in this file  ;-)

namespace Flexus {
namespace Rdram {

using namespace Flexus::Typelist;
using Flexus::Debug::Debug;

class RdramBlackbox {

public:

  int64_t accesses;  // accesses = reads + writes
  int64_t reads;
  int64_t writes;

  int64_t delay;  // total latency in cycles
  int64_t currentCycle;  // total cycles during the simulation
  // without taking into account the
  // RDRAM latencies

  int32_t lastBank;  // index to the most recently accessed bank
  rdram_bank_t banks[RDRAM_NUM_BANKS];

  /*
   * Constructor
   */
  RdramBlackbox() {

    currentCycle = 0;
    delay = 0;

    accesses = 0;
    reads = 0;
    writes = 0;

    lastBank = -1;

    // initialize banks
    memset(banks, 0, (RDRAM_NUM_BANKS * sizeof(rdram_bank_t)));
    for (int32_t i = 0; i < RDRAM_NUM_BANKS; i++) {
      banks[i].id = i;
      banks[i].lastType = -1;
      banks[i].state = ACTIVE;
    }
  }  // Constructor

  /*
   * Destructor
   */
  ~RdramBlackbox() {
    puke();
  }

  /*
   * Banks are set to a static state and remain at the state
   * through out the runtime
   */
  int32_t accessStatic(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
    rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);

    if (cmd == RDRAM_READ) {
      reads++;
    } else {
      writes++;
    }
    accesses++;

    int32_t latency = 0;
    switch (pbank->state) {
      case ACTIVE:
        if (pbank->ready > currentCycle) {
          latency += RDRAM_ACTIVE_ACCESS;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          // no transition cycles as the bank must be in transistion state
          pbank->ready += RDRAM_ACTIVE_ACCESS;
        } else {
          latency += RDRAM_ACTIVE_ACCESS;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          pbank->ready = (currentCycle + latency);
        }
        break;

      case STANDBY:
        if (pbank->ready > currentCycle) {
          latency += RDRAM_ACTIVE_ACCESS;
          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          // no transition cycles as the bank must be in transistion state
          pbank->ready += RDRAM_ACTIVE_ACCESS;
        } else {
          latency += RDRAM_ACTIVE_ACCESS;
          latency += RDRAM_STANDBY_TO_ACTIVE;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          pbank->s2aCycles += RDRAM_STANDBY_TO_ACTIVE;
          pbank->ready = (currentCycle + latency);
        }
        break;

      case NAP:
        if (pbank->ready > currentCycle) {
          latency += RDRAM_ACTIVE_ACCESS;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          // no transition cycles as the bank must be in transistion state
          pbank->ready += RDRAM_ACTIVE_ACCESS;
        } else {
          latency += RDRAM_ACTIVE_ACCESS;
          latency += RDRAM_NAP_TO_ACTIVE;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          pbank->n2aCycles += RDRAM_NAP_TO_ACTIVE;
          pbank->ready = (currentCycle + latency);
        }
        break;

      case POWERDOWN:
        if (pbank->ready > currentCycle) {
          latency += RDRAM_ACTIVE_ACCESS;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          // no transition cycles as the bank must be in transistion state
          pbank->ready += RDRAM_ACTIVE_ACCESS;
        } else {
          latency += RDRAM_ACTIVE_ACCESS;
          latency += RDRAM_POWERDOWN_TO_ACTIVE;

          pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
          pbank->p2aCycles += RDRAM_POWERDOWN_TO_ACTIVE;
          pbank->ready = (currentCycle + latency);
        }
        break;

      default:
        assert(0);
        break;
    }

    delay = latency;

    return latency;
  }  // accessStatic

  /*
   * prints out statistics out to STDOUT
   */
  void puke() {
    cout << endl;
    cout << "RDRAM statistics:" << endl;
    cout << "Cycles elapsed: " << currentCycle << endl;
    cout << "Number of accesses: " << accesses << endl;
    cout << "Number of reads: " << reads << endl;
    cout << "Number of writes: " << writes << endl;

    int32_t numberActiveBanks = 0;
    int32_t numberStandbyBanks = 0;
    int32_t numberNapBanks = 0;
    int32_t numberPowerdownBanks = 0;

    int64_t activeCycles = 0;
    int64_t standbyCycles = 0;
    int64_t napCycles = 0;
    int64_t powerdownCycles = 0;

    for (int32_t i = 0; i < RDRAM_NUM_BANKS; i++) {

      switch (banks[i].state) {
        case ACTIVE:
          numberActiveBanks++;
          activeCycles += banks[i].activeCycles;
          standbyCycles += banks[i].standbyCycles;
          napCycles += banks[i].napCycles;
          powerdownCycles += banks[i].powerdownCycles;
          break;

        case STANDBY:
          numberStandbyBanks++;
          activeCycles += banks[i].activeCycles;
          standbyCycles += banks[i].standbyCycles;
          napCycles += banks[i].napCycles;
          powerdownCycles += banks[i].powerdownCycles;
          break;

        case NAP:
          numberNapBanks++;
          activeCycles += banks[i].activeCycles;
          standbyCycles += banks[i].standbyCycles;
          napCycles += banks[i].napCycles;
          powerdownCycles += banks[i].powerdownCycles;
          break;

        case POWERDOWN:
        default:
          numberPowerdownBanks++;
          activeCycles += banks[i].activeCycles;
          standbyCycles += banks[i].standbyCycles;
          napCycles += banks[i].napCycles;
          powerdownCycles += banks[i].powerdownCycles;
          break;

      }  // switch

    }  // for

    int64_t total_engy = RDRAM_ACTIVE_POWER * activeCycles +
                         RDRAM_STANDBY_POWER * standbyCycles +
                         RDRAM_NAP_POWER * napCycles +
                         RDRAM_POWERDOWN_POWER * powerdownCycles;

    cout << "Number of banks in ACTIVE state: " << numberActiveBanks << endl;
    cout << "Number of banks in STANDBY state: " << numberStandbyBanks << endl;
    cout << "Number of banks in NAP state: " << numberNapBanks << endl;
    cout << "Number of banks in POWERDOWN: " << numberPowerdownBanks << endl;
    cout << "Energy (nJ): " << total_engy << endl;
    cout << endl;
  }  // puke

  /*
   * doServe
   */
  inline
  int32_t doServe() {
    currentCycle++;
    /*
          if(1 == outstanding) {
            assert(busy >= 0);

            if(busy > 0) {
              busy--;
            }

            if(0 == busy) {
              outstanding = 0;
              return 1;
            }
          }
     */
    return 0;
  }  // doServe

#ifdef USED  // used for debugging purpose only
  /*
   * accessDebug -- constant access latency for debugging purpose
   */
  int32_t accessDebug(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
    rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);

    if (cmd == RDRAM_READ) {
      reads++;
    } else {
      writes++;
    }
    accesses++;

    lastBank = pbank->id;
    pbank->lastAccessed = currentCycle;
    pbank->ready = (currentCycle + 6);

    outstanding = 1;
    busy = 6;
    return 6;
  }  // accessDebug

  /*
   * Print out bank state/status
   */
  void printStates() {
    cout << endl;
    cout << "RDRAM statistics:" << endl;
    cout << "Cycles elapsed: " << currentCycle << endl;
    cout << "Number of accesses: " << accesses << endl;
    cout << "Number of reads: " << reads << endl;
    cout << "Number of writes: " << writes << endl;

    int32_t numberActiveBanks = 0;
    int32_t numberStandbyBanks = 0;
    int32_t numberNapBanks = 0;
    int32_t numberPowerdownBanks = 0;
    for (int32_t i = 0; i < RDRAM_NUM_BANKS; i++) {
      int64_t idleInterval = (currentCycle - banks[i].lastAccessed);
      // has the bank been standing by or napping or off?
      if (idleInterval > RDRAM_THRESHOLD) {
        switch (idleInterval / RDRAM_THRESHOLD) {
          case 1:  // STANDBY STATE
            numberStandbyBanks++;
            banks[i].activeCycles += RDRAM_THRESHOLD;
            banks[i].standbyCycles += (idleInterval - RDRAM_THRESHOLD);

            break;
          case 2:  // NAP STATE
            numberNapBanks++;
            banks[i].activeCycles += RDRAM_THRESHOLD;
            banks[i].standbyCycles += RDRAM_THRESHOLD;
            banks[i].napCycles += (idleInterval - 2 * RDRAM_THRESHOLD);

            break;
          case 3:  // POWERDOWN STATE
          default:
            numberPowerdownBanks++;
            banks[i].activeCycles += RDRAM_THRESHOLD;
            banks[i].standbyCycles += RDRAM_THRESHOLD;
            banks[i].napCycles += RDRAM_THRESHOLD;
            banks[i].powerdownCycles += (idleInterval - 3 * RDRAM_THRESHOLD);

            break;
        }  // switch
      } else {
        numberActiveBanks++;
        banks[i].activeCycles += idleInterval;
      }
    }  // for

    cout << "Number of banks in ACTIVE state: " << numberActiveBanks << endl;
    cout << "Number of banks in STANDBY state: " << numberStandbyBanks << endl;
    cout << "Number of banks in NAP state: " << numberNapBanks << endl;
    cout << "Number of banks in POWERDOWN: " << numberPowerdownBanks << endl;
    cout << endl;

  }  // printStates

  /**
   * RDRAM bank access function -- only works if the banks are initialized
   * to ACTIVE state
   */
  int32_t access(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
    if (cmd == RDRAM_READ) {
      reads++;
    } else {
      writes++;
    }
    accesses++;

    rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);
    int32_t latency = 0;
    int64_t idleInterval = (currentCycle - pbank->lastAccessed);

    // has the bank been standing by or napping or off?
    if (idleInterval > RDRAM_THRESHOLD) {
      switch (idleInterval / RDRAM_THRESHOLD) {
        case 1:  // STANDBY STATE
          latency += RDRAM_STANDBY_TO_ACTIVE;
          latency += RDRAM_ACTIVE_ACCESS;
          pbank->ready =
            (currentCycle + RDRAM_STANDBY_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

          pbank->activeCycles += RDRAM_THRESHOLD;
          pbank->standbyCycles += (idleInterval - RDRAM_THRESHOLD);

          break;
        case 2:  // NAP STATE
          latency += RDRAM_NAP_TO_ACTIVE;
          latency += RDRAM_ACTIVE_ACCESS;
          pbank->ready =
            (currentCycle + RDRAM_NAP_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

          pbank->activeCycles += RDRAM_THRESHOLD;
          pbank->standbyCycles += RDRAM_THRESHOLD;
          pbank->napCycles += (idleInterval - 2 * RDRAM_THRESHOLD);

          break;
        case 3:  // POWERDOWN STATE
        default:
          latency += RDRAM_POWERDOWN_TO_ACTIVE;
          latency += RDRAM_ACTIVE_ACCESS;
          pbank->ready =
            (currentCycle + RDRAM_POWERDOWN_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

          pbank->activeCycles += RDRAM_THRESHOLD;
          pbank->standbyCycles += RDRAM_THRESHOLD;
          pbank->napCycles += RDRAM_THRESHOLD;
          pbank->powerdownCycles += (idleInterval - 3 * RDRAM_THRESHOLD);

          break;
      }  // switch

    } else {  // bank in ACTIVE state
      if (currentCycle < pbank->ready) {
        latency += (pbank->ready - currentCycle + RDRAM_ACTIVE_ACCESS);
        pbank->ready += RDRAM_ACTIVE_ACCESS;
      } else {
        latency += RDRAM_ACTIVE_ACCESS;
        pbank->ready = (currentCycle + RDRAM_ACTIVE_ACCESS);
      }

    }

    lastBank = pbank->id;
    pbank->lastAccessed = currentCycle;

    outstanding = 1;
    busy = latency;

    return latency;
  }  // access
#endif

};  // end class RdramBank

}  // end namespace Rdram
}  // end namespace Flexus

