set(TanglyKraken_REQUIRED_COMPONENTS
  CommonQEMU
  uFetch
  Decoder
  uArch
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
set(DEFAULT_PLATFORM riscv)
