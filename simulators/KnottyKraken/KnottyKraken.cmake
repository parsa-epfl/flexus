set(KnottyKraken_REQUIRED_COMPONENTS
  CommonQEMU 
  uFetch 
  armDecoder 
  uArchARM 
  FetchAddressGenerate 
  Cache
  MemoryLoopback 
  MemoryMap 
  MagicBreakQEMU 
  CMPCache 
  MultiNic 
  NetShim 
  TraceTrackerQEMU 
  MTManager 
  SplitDestinationMapper 
  MMU
)

set(SUPPORTS_STANDALONE false)
set(SUPPORTS_SIMICS false)
set(SUPPORTS_QEMU true)
set(DEFAULT_TARGET qemu)
set(SUPPORTS_X86 false)
set(SUPPORTS_V9 false)
set(SUPPORTS_ARM true)
set(DEFAULT_PLATFORM arm)