set(REQUIRED_COMPONENTS
  CommonQEMU  
  MemoryLoopback 
  MemoryMap
)
#MemoryMap 
#  CMPCache 
#  MMU
#)

set(SUPPORTS_SIMICS false)
set(SUPPORTS_QEMU true)
set(DEFAULT_TARGET qemu)
set(SUPPORTS_X86 false)
set(SUPPORTS_V9 false)
set(SUPPORTS_ARM true)
set(DEFAULT_PLATFORM arm)
