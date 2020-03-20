set(KeenKraken_REQUIRED_COMPONENTS
  CommonQEMU 
  DecoupledFeederQEMU 
  FastCache 
  FastCMPCache 
  FastMemoryLoopback 
  MagicBreakQEMU 
  BPWarm 
  TraceTrackerQEMU 
  MMU
)

set(SUPPORTS_QEMU true)
set(DEFAULT_TARGET qemu)
set(SUPPORTS_X86 false)
set(SUPPORTS_V9 false)
set(SUPPORTS_ARM true)
set(DEFAULT_PLATFORM arm)