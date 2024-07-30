
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Transports/MemoryTransport.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT uFetch
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( FAQSize, uint32_t, "Fetch address queue size", "faq", 32 )
  PARAMETER( MaxFetchLines, uint32_t, "Max i-cache lines fetched per cycle", "flines", 2 )
  PARAMETER( MaxFetchInstructions, uint32_t, "Max instructions fetched per cycle", "finst", 10 )
  PARAMETER( ICacheLineSize, uint64_t, "Icache line size in bytes", "iline", 64 )
  PARAMETER( PerfectICache, bool, "Use a perfect ICache", "perfect", true )
  PARAMETER( PrefetchEnabled, bool, "Enable Next-line Prefetcher", "prefetch", true )
  PARAMETER( CleanEvict, bool, "Enable eviction messages", "clean_evict", false)
  PARAMETER( Size, int, "ICache size in bytes", "size", 65536 )
  PARAMETER( Associativity, int, "ICache associativity", "associativity", 4 )
  PARAMETER( MissQueueSize, uint32_t, "Maximum size of the fetch miss queue", "miss_queue_size", 4 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this uFetch", "threads", 1 )
  PARAMETER( SendAcks, bool, "Send acknowledgements when we received data", "send_acks", false )
  PARAMETER( UseReplyChannel, bool, "Send replies on Reply Channel and only Evicts on Snoop Channel", "use_reply_channel", false )
  PARAMETER( EvictOnSnoop, bool, "Send evicts on Snoop Channel (otherwise use Request Channel)", "evict_on_snoop", true )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<FetchCommand>, FetchAddressIn )
  DYNAMIC_PORT_ARRAY( PushInput, eSquashCause, SquashIn )
  PORT( PushInput, MemoryTransport, FetchMissIn )
  DYNAMIC_PORT_ARRAY( PullOutput, int, AvailableFAQOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFIQ )
  DYNAMIC_PORT_ARRAY( PullOutput, int, ICount)
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  DYNAMIC_PORT_ARRAY( PushOutput, pFetchBundle, FetchBundleOut )

  PORT( PushOutput, MemoryTransport, FetchMissOut )
  PORT( PushOutput, MemoryTransport, FetchSnoopOut )
  PORT( PushOutput, MemoryTransport, FetchReplyOut )

  PORT( PushOutput, TranslationPtr , iTranslationOut )
  PORT( PushInput,  TranslationPtr , iTranslationIn )

  PORT( PushOutput, bool, InstructionFetchSeen ) // Notify PowerTracker when an instruction is fetched.
  PORT( PushOutput, bool, ClockTickSeen )        // Notify PowerTracker when the clock in this core ticks. This goes here just because uFetch is driven first and it's convenient.


  DYNAMIC_PORT_ARRAY( PushInput, int, ResyncIn )

  DRIVE( uFetchDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT uFetch
// clang-format on
