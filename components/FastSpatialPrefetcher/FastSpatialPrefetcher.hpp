#include <core/simulator_layout.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT SpatialPrefetcher
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "c-level", Flexus::SharedTypes::eL1 )
  PARAMETER( PrefetchLevel, Flexus::SharedTypes::tFillLevel, "Cache level to prefetch into", "pref-level", Flexus::SharedTypes::eL1 )
  PARAMETER( BufferPrefetching, bool, "Prefetch into local buffer (SVB)", "svb", false )
  PARAMETER( UsageEnable, bool, "Enable usage stats", "usage", false )
  PARAMETER( RepetEnable, bool, "Enable repetition stats", "repet", false )
  PARAMETER( TimeRepetEnable, bool, "Enable time rep tracking", "time", false )
  PARAMETER( BufFetchEnable, bool, "Enable local buffer fetching", "buf-fetch", false )
  PARAMETER( PrefetchEnable, bool, "Enable prefetching", "enable", false )
  PARAMETER( ActiveEnable, bool, "Enable active group tracking", "active", false )
  PARAMETER( OrderEnable, bool, "Enable group ordering", "ordering", false )
  PARAMETER( StreamEnable, bool, "Enable streaming", "streaming", false )
  PARAMETER( BlockSize, long, "Cache block size", "bsize", 64 )
  PARAMETER( SgpBlocks, long, "Blocks per SPG", "sgp-blocks", 64 )
  PARAMETER( RepetType, long, "Repet type", "repet-type", 1 )
  PARAMETER( RepetFills, bool, "Record fills for repet", "repet-fills", false )
  PARAMETER( SparseOpt, bool, "Enable sparse optimization", "sparse", false )
  PARAMETER( PhtSize, long, "Size of PHT (entries, 0 = infinite)", "pht-size", 256 )
  PARAMETER( PhtAssoc, long, "Assoc of PHT (0 = fully-assoc)", "pht-assoc", 0 )
  PARAMETER( PcBits, long, "Number of PC bits to use", "pc-bits", 30 )
  PARAMETER( CptType, long, "Current Pattern Table type", "cpt-type", 1 )
  PARAMETER( CptSize, long, "Size of CPT (entries, 0 = infinite)", "cpt-size", 256 )
  PARAMETER( CptAssoc, long, "Assoc of CPT (0 = fully-assoc)", "cpt-assoc", 0 )
  PARAMETER( CptSparse, bool, "Allow unbounded sparse in CPT", "cpt-sparse", false )
  PARAMETER( FetchDist, bool, "Track buffetch dist-to-use", "fetch-dist", false )
  PARAMETER( StreamWindow, long, "Size of stream window", "window", 0 )
  PARAMETER( StreamDense, bool, "Do not use order for dense groups", "str-dense", false )
  PARAMETER( BufSize, long, "Size of stream value buffer", "buf-size", 0 )
  PARAMETER( StreamDescs, long, "Number of stream descriptors", "str-descs", 0 )
  PARAMETER( GhbEnable, bool, "Enable GHB (PC/DC)", "ghb", false )
  PARAMETER( GhbSize, long, "Size of GHB", "ghb-size", 256 )
  PARAMETER( CptFilter, long, "Size of filter table (entries, 0 = infinite)", "cpt-filt", 0 )
);

COMPONENT_INTERFACE(
  PORT(PushInput, MemoryMessage, RequestInL1)
  PORT(PushOutput, MemoryMessage, RequestOutL1)
  PORT(PushInput, MemoryMessage, SnoopInL1)
  PORT(PushOutput, MemoryMessage, SnoopOutL1)
  PORT(PushInput, MemoryMessage, RequestInL2)
  PORT(PushOutput, MemoryMessage, RequestOutL2)
  PORT(PushInput, MemoryMessage, SnoopInL2)
  PORT(PushOutput, MemoryMessage, SnoopOutL2)
  DRIVE(PrefetchDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SpatialPrefetcher
