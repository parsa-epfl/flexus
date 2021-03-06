#include <core/simulator_layout.hpp>

#include <components/uFetch/uFetchTypes.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER(MaxBPred, uint32_t, "Max branches predicted per cycle", "bpreds", 2)
    PARAMETER(Threads, uint32_t, "Number of threads under control of this FAG", "threads",1)
    PARAMETER(EnableRAS, bool, "Enable Return Address Stack", "enableRAS", false)
    PARAMETER(EnableTCE, bool, "Enable Tail Call Elimination(TCE)", "enableTCE", false)
    PARAMETER(EnableTrapRet, bool, "Redirect fetch on trap returns", "enableTrapRet",false)
    PARAMETER(EnableBTBPrefill, bool, "Predecode and fill BTB on a BTB Miss", "enableBTBPrefill", false)
    PARAMETER(MagicBTypeDetect, bool, "Tells the type of branch without decoding it", "magicBTypeDetect", false)
    PARAMETER(PerfectBTB, bool,"BTB detect all the branch instructions but not the targets for indirect branches","perfectBTB", false)
    PARAMETER(BlocksOnBTBMiss, int, "Number of next blocks to be prefetched at BTB miss","blocksOnBTBMiss", 0)
    PARAMETER(InsnOnBTBMiss, int, "Number of next consecutive instruction addresses to be enqueued on a BTB miss","insnOnBTBMiss", 1)
    PARAMETER(BTBSets, uint32_t, "Number of sets in the BTB", "btbsets", 512)
    PARAMETER(BTBWays, uint32_t, "Number of ways in the BTB", "btbways", 4)
    );

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput, Flexus::SharedTypes::VirtualMemoryAddress, RedirectIn)
  DYNAMIC_PORT_ARRAY(PushInput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackIn)
  DYNAMIC_PORT_ARRAY(PushOutput, boost::intrusive_ptr<FetchCommand>, FetchAddrOut)
  DYNAMIC_PORT_ARRAY(PushOutput, boost::intrusive_ptr<FetchCommand>, PrefetchAddrOut)
  DYNAMIC_PORT_ARRAY(PushOutput, boost::intrusive_ptr<RecordedMisses>,RecordedMissOut)
  DYNAMIC_PORT_ARRAY(PullInput, int, AvailableFAQ)
  DYNAMIC_PORT_ARRAY(PullOutput, bool, Stalled)
  DYNAMIC_PORT_ARRAY(PushInput, eSquashCause, SquashIn) // Rakesh
  DYNAMIC_PORT_ARRAY(PushInput, boost::intrusive_ptr<BPredState>, SquashBranchIn)
  DYNAMIC_PORT_ARRAY(PushInput, std::list<boost::intrusive_ptr<BPredState>>, RASOpsIn)
  DYNAMIC_PORT_ARRAY(PushInput, boost::intrusive_ptr<BPredState>,SpecialCallIn)
  DYNAMIC_PORT_ARRAY(PushInput, vaddr_pair, MissPairIn)
  DYNAMIC_PORT_ARRAY(PushInput, bool, BTBReplyIn)
  DYNAMIC_PORT_ARRAY(PushInput, bool, BTBMissFetchReplyIn) //Ali
  DYNAMIC_PORT_ARRAY(PushOutput, VirtualMemoryAddress, BTBRequestOut)
  DYNAMIC_PORT_ARRAY(PushInput, boost::intrusive_ptr<TrapState>, TrapStateIn)
  // add for qflex
  PORT(PullInput, bool, uArchHalted)

  DRIVE(FAGDrive)
  );

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate
// clang-format on
