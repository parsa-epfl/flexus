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
set(DEFAULT_PLATFORM aarch64)
