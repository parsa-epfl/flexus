#ifndef _RDRAM_MAGIC_H_
#define _RDRAM_MAGIC_H_

/*
 * some convenience micros
 */
#define RDRAM_NUM_BANKS 16
#define RDRAM_SPILL_INTERVAL 5000000
#define RDRAM_ADDR2BANK(banks, paddr)   \
       (&(banks)[( (paddr)>>7 ) & (RDRAM_NUM_BANKS-1)])
// assuming 128B L2 cache line

/*
 * RDRAM timing micros
 */
#define RDRAM_RC       2  // interval between commands to the same bank
#define RDRAM_PACKET   0  // length of ROWA/ROWR/COLC/COLM/COLX packet
#define RDRAM_RR       2  // RAS-to-RAS time to different banks of same device
#define RDRAM_RP       2  // interval between PRER command and next ROWA command
#define RDRAM_CBUB1    2  // bubble between READ and WRITE command
#define RDRAM_CBUB2    2  // bubble between WRITE and READ command
#define RDRAM_RCD      2  // RAS-to-CAS delay
#define RDRAM_CAC      2  // CAS-to-Q delay
#define RDRAM_CWD      2  // CAS write delay

#define RDRAM_ACTIVE_ACCESS           1
#define RDRAM_STANDBY_TO_ACTIVE       6
#define RDRAM_NAP_TO_ACTIVE          20
#define RDRAM_POWERDOWN_TO_ACTIVE   600

#define RDRAM_THRESHOLD  500  // number of cycles has to elapse before
// a bank power state transition

/*
 * RDRAM BANK power
 */
#define RDRAM_ACTIVE_POWER     300
#define RDRAM_STANDBY_POWER    180
#define RDRAM_NAP_POWER        30
#define RDRAM_POWERDOWN_POWER  3

#define RDRAM_STANDBY_TO_ACTIVE_POWER    240
#define RDRAM_NAP_TO_ACTIVE_POWER        165
#define RDRAM_POWERDOWN_TO_ACTIVE_POWER  152

/*
 * RDRAM control commands in micros
 */
#define RDRAM_READ  0
#define RDRAM_WRITE 1

/*
 * RDRAM bank states
 */
enum rdram_bank_state_t {
  ACTIVE,
  STANDBY,
  NAP,
  POWERDOWN
};

typedef unsigned RdramPhysicalMemoryAddress;
typedef int64_t RdramCycles;
typedef char RdramCommand;

/*
 * RDRAM bank
 */
typedef struct rdram_bank {
  int32_t id;        // bank ID
  int32_t lastType;  // last transanction type (READ/WRITE)
  enum rdram_bank_state_t state;  // current state

  int64_t accesses;
  int64_t reads;
  int64_t writes;

  RdramCycles lastAccessed;
  RdramCycles ready;

  RdramCycles s2aCycles;
  RdramCycles n2aCycles;
  RdramCycles p2aCycles;

  RdramCycles activeCycles;  // cycles in ACTIVE state
  RdramCycles standbyCycles;  // cycles in STANDBY state
  RdramCycles napCycles;  // cycles in NAP state
  RdramCycles powerdownCycles;  // cycles in POWERDOWN state
} rdram_bank_t;

#endif  // _RDRAM_MAGIC_H_

