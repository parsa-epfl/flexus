#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT MagicBreak
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define MagicBreak_IMPLEMENTATION    (<components/MagicBreak/MagicBreakImpl.hpp>)

COMPONENT_PARAMETERS(
  PARAMETER( EnableIterationCounts, bool, "Enable Iteration Counts", "iter", false )
  PARAMETER( TerminateOnMagicBreak, int, "Terminate simulation on a specific magic breakpoint", "stop_on_magic", -1 )
  PARAMETER( TerminateOnIteration, int, "Terminate simulation when CPU 0 reaches iteration.  -1 disables", "end_iter", -1 )
  PARAMETER( CheckpointOnIteration, bool, "Checkpoint simulation when CPU 0 reaches each iteration.", "ckpt_iter", false )
  PARAMETER( TerminateOnTransaction, int, "Terminate simulation after ## transactions.  -1 disables", "end_trans", -1 )
  PARAMETER( EnableTransactionCounts, bool, "Enable Transaction Counts", "trans", false )
  PARAMETER( TransactionType, int, "Workload type.  0=TPCC/JBB  1=WEB", "trans_type", 0 )
  PARAMETER( TransactionStatsInterval, int, "Statistics interval on ## transactions.  -1 disables", "stats_trans", -1 )
  PARAMETER( CheckpointEveryXTransactions, int, "Quiesce and save every X transactions. -1 disables", "ckpt_trans", -1 )
  PARAMETER( FirstTransactionIs, int, "Transaction number for first transaction.", "first_trans", 0 )
  PARAMETER( CycleMinimum, uint64_t, "Minimum number of cycles to run when TerminateOnTransaction is enabled.", "min_cycle", 0 )
  PARAMETER( StopCycle, uint64_t, "Cycle on which to halt simulation.", "stop_cycle", 0 )
  PARAMETER( CkptCycleInterval, uint64_t, "# of cycles between checkpoints.", "ckpt_cycle", 0 )
  PARAMETER( CkptCycleName, uint32_t, "Base cycle # from which to build checkpoint names.", "ckpt_cycle_name", 0 )
);

COMPONENT_INTERFACE(
  DRIVE( TickDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MagicBreak

