#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT v9Decoder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/uFetch/uFetchTypes.hpp>
#include <components/Common/Slices/AbstractInstruction.hpp>

typedef std::pair < int32_t /*# of instruction */ , bool /* core synchronized? */ > dispatch_status;

COMPONENT_PARAMETERS(
  PARAMETER( FIQSize, uint32_t, "Fetch instruction queue size", "fiq", 32 )
  PARAMETER( DispatchWidth, uint32_t, "Maximum dispatch per cycle", "dispatch", 8 )
  PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )
);

COMPONENT_INTERFACE(
  PORT( PushInput, pFetchBundle, FetchBundleIn)
  PORT( PullOutput, int32_t, AvailableFIQOut)
  PORT( PullInput, dispatch_status, AvailableDispatchIn)
  PORT( PushOutput, boost::intrusive_ptr< AbstractInstruction>, DispatchOut)
  PORT( PushOutput, eSquashCause, SquashOut)
  PORT( PushInput, eSquashCause, SquashIn)
  PORT( PullOutput, int32_t, ICount)
  PORT( PullOutput, bool, Stalled)

  PORT( PushOutput, int64_t, DispatchedInstructionOut) // Send instruction word to Power Tracker

  DRIVE( DecoderDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT v9Decoder

