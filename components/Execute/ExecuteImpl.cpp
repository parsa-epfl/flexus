#include <components/Execute/Execute.hpp>

#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

#include <deque>

#include <components/Common/StandardDeviation.hpp>
#include <components/Common/TimeBreakdown.hpp>

#include <core/stats.hpp>
#include <core/simics/simics_interface.hpp>

#define DBG_DefineCategories Execute
#define DBG_SetDefaultOps AddCat(Execute)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT Execute
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nExecute {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

using std::deque;
using boost::intrusive_ptr;

using Flexus::SharedTypes::PhysicalMemoryAddress;

class ExecuteStateImpl : public ExecuteState {
  bool theIsIssued;
  bool theIsInitiated;
  bool theIsPerformed;
  bool theIsComplete;

  bool theIsStorePrefetched;
  bool theIsStorePrefetchedDone;
  bool theSkipStorePrefetchDone;

  uint64_t theDispatchTimestamp;
  intrusive_ptr<ArchitecturalInstruction> theInstruction;

  nTimeBreakdown::eCycleClasses theStallClass;

public:
  ExecuteStateImpl(intrusive_ptr<ArchitecturalInstruction> anInstruction)
    : theIsIssued(false)
    , theIsInitiated(false)
    , theIsComplete(false)

    // JWK: st-prefetch
    , theIsStorePrefetched(false)
    , theIsStorePrefetchedDone(false)
    , theSkipStorePrefetchDone(false)

    , theDispatchTimestamp(0)
    , theInstruction(anInstruction)
    , theStallClass(nTimeBreakdown::kUnknown)
  {}

  void setStallClass(nTimeBreakdown::eCycleClasses aClass) {
    theStallClass = aClass;
  }

  nTimeBreakdown::eCycleClasses stallClass() {
    return theStallClass;
  }

  bool isStorePrefetched() const {
    return theIsStorePrefetched;
  }
  void storePrefetched() {
    theIsStorePrefetched = true;
  }

  bool isStorePrefetchedDone() const {
    return theIsStorePrefetchedDone;
  }
  void storePrefetchedDone() {
    theIsStorePrefetchedDone = true;
  }

  bool isSkipStorePrefetch() const {
    return theSkipStorePrefetchDone;
  }
  void skipStorePrefetch() {
    DBG_Assert(!isStorePrefetched() && !isStorePrefetchedDone());
    storePrefetched();
    storePrefetchedDone();
    theSkipStorePrefetchDone = true;
  }

  bool isIssued() const {
    return theIsIssued;
  }
  void issue() {
    theIsIssued = true;
    theInstruction->execute();
  }

  bool isInitiated() const {
    return theIsInitiated;
  }
  void initiate() {
    theIsInitiated = true;
  }

  bool isPerformed() const {
    return theInstruction->isPerformed();
  }
  void perform() {
    theInstruction->perform();
  }

  bool isComplete() const {
    return theIsComplete;
  }
  void complete() {
    theIsComplete = true;
  }

  uint64_t getDispatchTimestamp(void) const   {
    return theDispatchTimestamp;
  }
  void               setDispatchTimestamp(void)         {
    theDispatchTimestamp = Flexus::Core::theFlexus->cycleCount();
    return;
  }

  ArchitecturalInstruction & instruction() {
    return * theInstruction;
  }
  intrusive_ptr<ArchitecturalInstruction> instructionPtr() {
    return theInstruction;
  }
};

class FLEXUS_COMPONENT(Execute) {
  FLEXUS_COMPONENT_IMPL(Execute);

  std::deque < intrusive_ptr<ExecuteStateImpl> > theROB;
  std::deque < intrusive_ptr<ExecuteStateImpl> > theLSQ;
  std::deque < intrusive_ptr<ExecuteStateImpl> > theSB;
  std::map < Flexus::SharedTypes::PhysicalMemoryAddress, intrusive_ptr<ExecuteStateImpl> > theSBMap;

  deque < MemoryTransport > theMemoryRequests;

  std::deque < intrusive_ptr<ExecuteStateImpl> >::iterator theSBprefetch_iter;
  bool         wait_SB_empty;
  nTimeBreakdown::eCycleClasses theWaitSBEmptyCause;

  unsigned theIssuedMemOps;

  nTimeBreakdown::TimeBreakdown theTimeBreakdown;
  int32_t kUser;
  int32_t kTrap;
  int32_t kSystem;

  uint32_t theMaxSpinLoads;
  bool theSpinning;
  uint32_t theSpinDetectCount;
  std::set<PhysicalMemoryAddress> theSpinMemoryAddresses;
  std::set<VirtualMemoryAddress> theSpinPCs;

  //Stat::StatInstanceCounter<uint64_t> theLoadTimeHistogram;
  //Stat::StatInstanceCounter<uint64_t> theStoreTimeHistogram;
  //Stat::StatInstanceCounter<uint64_t> theStorePrefetchTimeHistogram;

  //Stat::StatInstanceCounter<uint64_t> theCmpxTimeHistogram;

  //Commits and SpinCommits are mutually exclusive
  Stat::StatCounter theCommits;
  Stat::StatCounter theCommits_User;
  Stat::StatCounter theCommits_Trap;
  Stat::StatCounter theCommits_System;
  Stat::StatCounter theSpinCommits;
  Stat::StatCounter theSpinCommits_User;
  Stat::StatCounter theSpinCommits_Trap;
  Stat::StatCounter theSpinCommits_System;

#if FLEXUS_TARGET_IS(x86)
  //Add this counters to correctly count I-Count for x86 instructions
  Stat::StatCounter theCommits_x86;
  Stat::StatCounter theCommits_User_x86;
  Stat::StatCounter theCommits_System_x86;
  Stat::StatCounter theSpinCommits_x86;
  Stat::StatCounter theSpinCommits_User_x86;
  Stat::StatCounter theSpinCommits_System_x86;
  Stat::StatCounter theDispatchedInsns_x86;
  Stat::StatCounter theHardwareWalkInstrs;
#endif

  uint64_t theCommitCount;

  Stat::StatLog2Histogram theCommitsBetweenMisses_Coherence;
  Stat::StatCounter theMisses_Coherence;
  uint64_t theLastCommitCount_Coherence;

  Stat::StatLog2Histogram theCommitsBetweenMisses_Remote;
  Stat::StatCounter theMisses_Remote;
  uint64_t theLastCommitCount_Remote;

  Stat::StatLog2Histogram theCommitsBetweenMisses_OffChip;
  Stat::StatCounter theMisses_OffChip;
  uint64_t theLastCommitCount_OffChip;

  Stat::StatCounter theTotalMemOps;
  Stat::StatMax theSBMaxSize;
  Stat::StatCounter theSBAccesses;
  Stat::StatCounter theSBHits;
  Stat::StatCounter theSBPartialMisses;
  Stat::StatCounter theSBMisses;
  Stat::StatCounter theMEMBARs;
  Stat::StatCounter theDispatchedInsns;
  Stat::StatCounter theSnoops;
  Stat::StatCounter theNOPs;
  Stat::StatCounter theNonSyncStores;
  Stat::StatCounter theNonSyncLoads;
  Stat::StatCounter theSyncStores;
  Stat::StatCounter theSyncLoads;
  Stat::StatCounter theLoads;
  Stat::StatCounter theStores;
  Stat::StatCounter theRMWs;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(Execute)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )

    , theTimeBreakdown(statName() + "-TB")

    , theSpinning(false)
    , theSpinDetectCount(0)

    //, theLoadTimeHistogram("Load Histogram", this)
    //, theStoreTimeHistogram("Store Histogram", this)
    //, theStorePrefetchTimeHistogram("Store Prefetch Histogram", this)
    //, theCmpxTimeHistogram("Cmpx Histogram", this)

    , theCommits("Commits", this)
    , theCommits_User("Commits:User", this)
    , theCommits_Trap("Commits:Trap", this)
    , theCommits_System("Commits:System", this)
    , theSpinCommits("SpinCommits", this)
    , theSpinCommits_User("SpinCommits:User", this)
    , theSpinCommits_Trap("SpinCommits:Trap", this)
    , theSpinCommits_System("SpinCommits:System", this)
#if FLEXUS_TARGET_IS(x86)
    , theCommits_x86("Commits_x86", this)
    , theCommits_User_x86("Commits:User_x86", this)
    , theCommits_System_x86("Commits:System_x86", this)
    , theSpinCommits_x86("SpinCommits_x86", this)
    , theSpinCommits_User_x86("SpinCommits:User_x86", this)
    , theSpinCommits_System_x86("SpinCommits:System_x86", this)
    , theDispatchedInsns_x86("Dispatched x86 Instructions", this )
    , theHardwareWalkInstrs("Hardware Walk Memory Accesses", this)
#endif
    , theCommitCount(0)
    , theCommitsBetweenMisses_Coherence("CommitsPerMiss:Coherence", this)
    , theMisses_Coherence("Misses:Coherence", this)
    , theLastCommitCount_Coherence(0)
    , theCommitsBetweenMisses_Remote("CommitsPerMiss:Remote", this)
    , theMisses_Remote("Misses:Remote", this)
    , theLastCommitCount_Remote(0)
    , theCommitsBetweenMisses_OffChip("CommitsPerMiss:OffChip", this)
    , theMisses_OffChip("Misses:OffChip", this)
    , theLastCommitCount_OffChip(0)

    , theTotalMemOps("Memory Operations", this)
    , theSBMaxSize("SB Max Size", this)
    , theSBAccesses("SB Accesses", this)
    , theSBHits("SB Hits", this)
    , theSBPartialMisses("SB Partial Misses", this)
    , theSBMisses("SB Misses", this)

    , theMEMBARs("MEMBARs", this)
    , theDispatchedInsns("Dispatched Insns", this)
    , theSnoops("Snoops", this)
    , theNOPs("NOPs", this)
    , theNonSyncStores("Non-Sync Stores", this)
    , theNonSyncLoads("Non-Sync Loads", this)
    , theSyncStores("Sync Stores", this)
    , theSyncLoads("Sync Loads", this)
    , theLoads("Reads", this)
    , theStores("Writes", this)
    , theRMWs("RMWs", this)

  {
    kUser = theTimeBreakdown.addClass("User");
    kTrap = theTimeBreakdown.addClass("Trap");
    kSystem = theTimeBreakdown.addClass("System");

    theMaxSpinLoads = cfg.MaxSpinLoads;
  }

  bool isQuiesced() const {
    return theROB.empty() && theSB.empty() && theLSQ.empty() && theMemoryRequests.empty();
  }

  //Initialization
  void initialize() {
    DBG_( Dev, Comp(*this) ( << "EX initalized" ));
    if (cfg.SequentialConsistency) {
      //Under SC, the instruction isn't complete until it is performed.
      DBG_( Dev, Comp(*this) ( << "Sequential Consistency Enabled" ));
    } else {
      DBG_Assert( FLEXUS_TARGET_IS(v9), ( << "Total Store Order consistency is supported for v9 target only" ));
    }

    wait_SB_empty = false;
    theWaitSBEmptyCause = nTimeBreakdown::kUnknown;

    if ((cfg.SequentialConsistency && cfg.StorePrefetching)) {
      // can't turn SC and TSO-prefetching at the same time.
      DBG_Assert(false);
    }

  }

  //Ports
  //=====
  //ExecuteIn
  //---------
  bool available(interface::ExecuteIn const &) {
    return (
             theROB.size() < cfg.ROBSize
             && theLSQ.size() < cfg.LSQSize
             && (  ( cfg.SBSize == 0) || ( theSB.size() < cfg.SBSize ) )
           );
  }
  void push(interface::ExecuteIn const &, InstructionTransport & anInstruction) {
    DBG_( Verb, Comp(*this) ( << "EX received instr " << *anInstruction[ArchitecturalInstructionTag] ));

    dispatch(anInstruction);
    theDispatchedInsns++;

#if FLEXUS_TARGET_IS(x86)
    if (anInstruction[ArchitecturalInstructionTag]->getIfPart() == Normal)
      ++theDispatchedInsns_x86;
#endif
  }

  //ExecuteMemReply
  //---------------
  FLEXUS_PORT_ALWAYS_AVAILABLE(ExecuteMemReply);
  void push(interface::ExecuteMemReply const &, MemoryTransport & aMessage) {
    intrusive_ptr<MemoryMessage> msg(aMessage[MemoryMessageTag]);
    if (msg->isRequest()) {
      theSnoops++;
      DBG_( Iface, Comp(*this) ( << "EX received mem request " << *aMessage[MemoryMessageTag] ) Addr( aMessage[MemoryMessageTag]->address() ) );
      theMemoryRequests.push_back(aMessage);
    } else {
      DBG_Assert( (   (msg->type() == MemoryMessage::LoadReply)
                      ||  (msg->type() == MemoryMessage::StoreReply)
                      ||  (msg->type() == MemoryMessage::StorePrefetchReply)
                      ||  (msg->type() == MemoryMessage::RMWReply)
                      ||  (msg->type() == MemoryMessage::CmpxReply)
                  ), Comp(*this) );
      DBG_( Iface, Comp(*this) ( << "EX received Reply " << *aMessage[MemoryMessageTag] ) Addr( aMessage[MemoryMessageTag]->address() ) );

      if (aMessage[TransactionTrackerTag] && !aMessage[TransactionTrackerTag]->completionCycle()) {
        aMessage[TransactionTrackerTag]->complete();
      }

      DBG_Assert(aMessage[ExecuteStateTag]);
      //We must explicitly downcast the ExecuteState slice since we don't expose
      //the true type of the ExecuteState in the transport object.
      ExecuteStateImpl & exec_state(static_cast<ExecuteStateImpl &>(*aMessage[ExecuteStateTag]));

      if (msg->type() == MemoryMessage::StorePrefetchReply) {
        if (exec_state.isStorePrefetched() && !exec_state.isStorePrefetchedDone()) {

          DBG_Assert(!exec_state.isInitiated()
                     || (exec_state.instruction().isRmw() && exec_state.isInitiated()));
          DBG_Assert(exec_state.isIssued());
          DBG_Assert(exec_state.isComplete()
                     || (exec_state.instruction().isRmw() && !exec_state.isComplete() /* sync RMW */)
                     || (exec_state.instruction().isStore() && exec_state.instruction().isSync() && !exec_state.isComplete() /* sync Store */)
                    ); // tso + stprefetching + sb : instantaneous complete
          DBG_Assert(!exec_state.isPerformed());
          // should be prefetched already, and now it should be the time to mark it as done.
          exec_state.storePrefetchedDone();
          return;
        } else {
          // got a StorePrefetchReply without being issued???
          DBG_Assert(0, Comp(*this));
        }
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      //Inform the Consort that we are completing read
      if (aMessage[MemoryMessageTag]->type() == MemoryMessage::LoadReply || aMessage[MemoryMessageTag]->type() == MemoryMessage::RMWReply || aMessage[MemoryMessageTag]->type() == MemoryMessage::CmpxReply ) {
        intrusive_ptr<PredictorMessage> pmsg (new PredictorMessage(PredictorMessage::eReadNonPredicted, flexusIndex(), aMessage[MemoryMessageTag]->address()));
        PredictorTransport pt;
        pt.set(PredictorMessageTag, pmsg);
        if ((aMessage[MemoryMessageTag]->type() == MemoryMessage::RMWReply || aMessage[MemoryMessageTag]->type() == MemoryMessage::CmpxReply)  && aMessage[TransactionTrackerTag]) {
          aMessage[TransactionTrackerTag]->setSpeculativeAtomicLoad(true);
        }
        pt.set(TransactionTrackerTag, aMessage[TransactionTrackerTag]);
        FLEXUS_CHANNEL(ToConsort) << pt;
      }
      /* CMU-ONLY-BLOCK-END */

      DBG_Assert( exec_state.isInitiated());

      //Cause Simics to perform the memory operation
      DBG_( Verb, Comp(*this) ( << "Performing: " << exec_state.instruction() ));
      exec_state.perform();

      //If the instruction isn't released yet, account for stall cycles
      if (! exec_state.isComplete()) {

        //The instruction wasn't released, so it incurred stall cycles
        if (exec_state.instruction().isLoad()) {

          if ( aMessage[TransactionTrackerTag] && aMessage[TransactionTrackerTag]->fillLevel()) {
            switch ( *aMessage[TransactionTrackerTag]->fillLevel() ) {
              case eL1:
                exec_state.setStallClass(nTimeBreakdown::kLoad_L1);
                break;
              case eL2:
                if (  aMessage[TransactionTrackerTag]->fillType() &&
                      *aMessage[TransactionTrackerTag]->fillType() == eCoherence )
                  exec_state.setStallClass(nTimeBreakdown::kLoad_L2_Coherence);
                else
                  exec_state.setStallClass(nTimeBreakdown::kLoad_L2);
                break;
              case ePrefetchBuffer:
                exec_state.setStallClass(nTimeBreakdown::kLoad_PB);
                DBG_( Iface, ( << statName() << " PB-HIT @" << aMessage[MemoryMessageTag]->address() ) );
                break;
              case eLocalMem:
                //Update miss distance histogram
                theCommitsBetweenMisses_OffChip << (theCommitCount - theLastCommitCount_OffChip);
                ++theMisses_OffChip;
                theLastCommitCount_OffChip = theCommitCount;

                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Local_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Local_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Local_DGP);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eCoherence) {
                    //Update miss distance histogram
                    theCommitsBetweenMisses_Coherence << (theCommitCount - theLastCommitCount_Coherence);
                    ++theMisses_Coherence;
                    theLastCommitCount_Coherence = theCommitCount;

                    exec_state.setStallClass(nTimeBreakdown::kLoad_Local_Coherence);

                    DBG_( Iface, ( << statName() << " COHERENCE-MISS @" << aMessage[MemoryMessageTag]->address() ) );

                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case eRemoteMem:
                //Update miss distance histogram
                theCommitsBetweenMisses_OffChip << (theCommitCount - theLastCommitCount_OffChip);
                theLastCommitCount_OffChip = theCommitCount;
                ++theMisses_OffChip;

                theCommitsBetweenMisses_Remote << (theCommitCount - theLastCommitCount_Remote);
                theLastCommitCount_Remote = theCommitCount;
                ++theMisses_Remote;

                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_DGP);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eCoherence) {
                    //Update miss distance histogram
                    theCommitsBetweenMisses_Coherence << (theCommitCount - theLastCommitCount_Coherence);
                    ++theMisses_Coherence;
                    theLastCommitCount_Coherence = theCommitCount;

                    DBG_( Trace, ( << statName() << " COHERENCE-MISS @" << aMessage[MemoryMessageTag]->address()) );

                    if (aMessage[TransactionTrackerTag]->previousState()) {
                      if (*aMessage[TransactionTrackerTag]->previousState() == eShared) {
                        exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_Coherence_Shared);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eModified) {
                        exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_Coherence_Modified);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eInvalid) {
                        exec_state.setStallClass(nTimeBreakdown::kLoad_Remote_Coherence_Invalid);
                      } else {
                        DBG_(Dev, ( << "Invalid previous state: " << *aMessage[TransactionTrackerTag]->previousState() ) );
                        DBG_Assert(false);
                      }
                    }
                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case ePeerL1Cache:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kLoad_PeerL1Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kLoad_PeerL1Cache_Coherence_Modified );
                break;

              case ePeerL2:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kLoad_PeerL2Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kLoad_PeerL2Cache_Coherence_Modified );
                break;

              default:
                DBG_(Dev, ( << "Invalid fill level: " << *aMessage[TransactionTrackerTag]->fillLevel() ) );
                DBG_Assert(false);
            }
          }

        } else if (exec_state.instruction().isStore()) {

          if ( aMessage[TransactionTrackerTag] && aMessage[TransactionTrackerTag]->fillLevel()) {
            switch ( *aMessage[TransactionTrackerTag]->fillLevel() ) {
              case eL1:
                exec_state.setStallClass(nTimeBreakdown::kStore_L1);
                break;
              case ePrefetchBuffer:
                exec_state.setStallClass(nTimeBreakdown::kStore_PB);
                break;
              case eL2:
                if (  aMessage[TransactionTrackerTag]->fillType() &&
                      *aMessage[TransactionTrackerTag]->fillType() == eCoherence )
                  exec_state.setStallClass(nTimeBreakdown::kStore_L2_Coherence);
                else
                  exec_state.setStallClass(nTimeBreakdown::kStore_L2);
                break;
              case eLocalMem:
                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Local_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Local_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Local_DGP);
                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case eRemoteMem:
                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Remote_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Remote_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kStore_Remote_DGP);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eCoherence) {
                    if (aMessage[TransactionTrackerTag]->previousState()) {
                      if (*aMessage[TransactionTrackerTag]->previousState() == eShared) {
                        exec_state.setStallClass(nTimeBreakdown::kStore_Remote_Coherence_Shared);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eModified) {
                        exec_state.setStallClass(nTimeBreakdown::kStore_Remote_Coherence_Modified);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eInvalid) {
                        exec_state.setStallClass(nTimeBreakdown::kStore_Remote_Coherence_Invalid);
                      } else {
                        DBG_(Dev, ( << "Invalid previous state: " << *aMessage[TransactionTrackerTag]->previousState() ) );
                        DBG_Assert(false);
                      }
                    }
                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case eDirectory: /* RMW and Store can be satisfied by Upgrade from Directory */
              case ePeerL1Cache:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kStore_PeerL1Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kStore_PeerL1Cache_Coherence_Modified );
                break;
              case ePeerL2:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kStore_PeerL2Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kStore_PeerL2Cache_Coherence_Modified );
                break;
              default:
                DBG_(Dev, ( << "Invalid fill level: " << *aMessage[TransactionTrackerTag]->fillLevel() ) );
                DBG_Assert(false);
            }
          }

        } else { /*RMW*/
          DBG_Assert( exec_state.instruction().isRmw());

          if ( aMessage[TransactionTrackerTag] && aMessage[TransactionTrackerTag]->fillLevel()) {
            switch ( *aMessage[TransactionTrackerTag]->fillLevel() ) {
              case eL1:
                exec_state.setStallClass(nTimeBreakdown::kAtomic_L1);
                break;
              case ePrefetchBuffer:
                exec_state.setStallClass(nTimeBreakdown::kAtomic_PB);
                break;
              case eL2:
                if (  aMessage[TransactionTrackerTag]->fillType() &&
                      *aMessage[TransactionTrackerTag]->fillType() == eCoherence )
                  exec_state.setStallClass(nTimeBreakdown::kAtomic_L2_Coherence);
                else
                  exec_state.setStallClass(nTimeBreakdown::kAtomic_L2);
                break;
              case eLocalMem:
                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Local_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Local_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Local_DGP);
                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case eRemoteMem:
                if (aMessage[TransactionTrackerTag]->fillType()) {
                  if (*aMessage[TransactionTrackerTag]->fillType() == eCold) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_Cold);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eReplacement) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_Replacement);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eDGP) {
                    exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_DGP);
                  } else if (*aMessage[TransactionTrackerTag]->fillType() == eCoherence) {
                    if (aMessage[TransactionTrackerTag]->previousState()) {
                      if (*aMessage[TransactionTrackerTag]->previousState() == eShared) {
                        exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_Coherence_Shared);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eModified) {
                        exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_Coherence_Modified);
                      } else if (*aMessage[TransactionTrackerTag]->previousState() == eInvalid) {
                        exec_state.setStallClass(nTimeBreakdown::kAtomic_Remote_Coherence_Invalid);
                      } else {
                        DBG_(Dev, ( << "Invalid previous state: " << *aMessage[TransactionTrackerTag]->previousState() ) );
                        DBG_Assert(false);
                      }
                    }
                  } else {
                    DBG_(Dev, ( << "Invalid fill type: " << *aMessage[TransactionTrackerTag]->fillType() ) );
                    DBG_Assert(false);
                  }
                }
                break;
              case eDirectory: /* RMW and Store can be satisfied by Upgrade from Directory */
              case ePeerL1Cache:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kAtomic_PeerL1Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kAtomic_PeerL1Cache_Coherence_Modified );
                break;
              case ePeerL2:
                if (  aMessage[TransactionTrackerTag]->previousState() &&
                      *aMessage[TransactionTrackerTag]->previousState() == eShared )
                  exec_state.setStallClass ( nTimeBreakdown::kAtomic_PeerL2Cache_Coherence_Shared );
                else
                  exec_state.setStallClass ( nTimeBreakdown::kAtomic_PeerL2Cache_Coherence_Modified );
                break;
              default:
                DBG_(Dev, ( << "Invalid fill level: " << *aMessage[TransactionTrackerTag]->fillLevel() ) );
                DBG_Assert(false);
            }
          }

        }  // end RMW

        exec_state.complete();

        DBG_( Verb, Comp(*this) ( << "Releasing:" << exec_state.instruction() ));
        exec_state.instruction().release();
      }

    }
  }

  //Drive Interfaces
  void drive(interface::ExecuteDrive const &) {
    DBG_( VVerb, Comp(*this) ( << "ExecuteDrive" ));
    doExecute();
  }
  void drive(interface::CommitDrive const &) {
    DBG_( VVerb, Comp(*this) ( << "CommitDrive" ));
    doCommit();
  }

private:
  void dispatch(InstructionTransport & anInstruction) {

    intrusive_ptr<ExecuteStateImpl> insn_state(new ExecuteStateImpl(anInstruction[ArchitecturalInstructionTag]));
    theROB.push_back(insn_state);
    if (insn_state->instruction().isMemory()) {
      theLSQ.push_back(insn_state);

      // set timestamp at initiation time
      insn_state->setDispatchTimestamp();
    }

    DBG_Assert(insn_state->instruction().canExecute());

    DBG_( Verb, Comp(*this) ( << "Executing: " << insn_state->instruction() ));
    insn_state->issue();

    if (insn_state->instruction().isPerformed()) {
      DBG_( Verb, Comp(*this) ( << "Completing: " << insn_state->instruction() ));

      theNOPs++;

      insn_state->setStallClass(nTimeBreakdown::kDataflow);

      //For non-memory instructions, executing also performs the
      //instruction.   We mark these completed immediately.  This
      //will also mark memory instructions that access IO locations
      //as issued/complete
      insn_state->complete();
    }

  }

  void doExecute() {
    DBG_( VVerb, Comp(*this) ( << "Entered doExecute()" ));

    sendMemoryReplies();

    theIssuedMemOps = 0;

    initiateMemoryOperations();
    issueStores();

    DBG_( VVerb, Comp(*this) ( << "Leaving doExecute()" ));
  }

  void spinDetect(intrusive_ptr<ExecuteStateImpl> & insn_state) {
    //Spin detection / control
    if (! insn_state->instruction().isMemory()) {
      return; //Only evaluate spin state on memory ops
    }

    if (theSpinning) {
      //Check leave conditions
      bool leave = false;

      if (!leave && !insn_state->instruction().isLoad() ) {
        //Leave on non-load memory op
        leave = true;
      }

      if (!leave && insn_state->instruction().isSync()) {
        //Leave on sychronizing mem-op
        leave = true;
      }

      if ( !leave && theSpinMemoryAddresses.count(insn_state->instruction().physicalMemoryAddress()) == 0) {
        leave = true;
      }
      if (!leave && theSpinPCs.count(insn_state->instruction().virtualInstructionAddress()) == 0) {
        leave = true;
      }

      ++theSpinDetectCount;

      if (leave) {
        std::string spin_string = "Addrs {";
        std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
        while (ma_iter != theSpinMemoryAddresses.end()) {
          spin_string += boost::lexical_cast<std::string>(*ma_iter) + ",";
          ++ma_iter;
        }
        spin_string += "} PCs {";
        std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
        while (va_iter != theSpinPCs.end()) {
          spin_string += boost::lexical_cast<std::string>(*va_iter) + ",";
          ++va_iter;
        }
        spin_string += "}";
        DBG_( Iface, Comp(*this) ( << " leaving spin mode. " << spin_string ) );

        theSpinning = false;
        theSpinDetectCount = 0;
        theSpinMemoryAddresses.clear();
        theSpinPCs.clear();
      }
    } else {
      //Check all reset conditions
      bool reset = false;

      if (!reset && !insn_state->instruction().isLoad() ) {
        reset = true;
      }

      if (!reset && insn_state->instruction().isSync()) {
        reset = true;
      }

      theSpinMemoryAddresses.insert(insn_state->instruction().physicalMemoryAddress());
      if (!reset && theSpinMemoryAddresses.size() > theMaxSpinLoads) {
        reset = true;
      }

      theSpinPCs.insert(insn_state->instruction().virtualInstructionAddress());
      if (!reset && theSpinPCs.size() > theMaxSpinLoads) {
        reset = true;
      }

      ++theSpinDetectCount;
      if (reset) {
        theSpinDetectCount = 0;
        theSpinMemoryAddresses.clear();
        theSpinPCs.clear();
      } else {
        //Check condition for entering spin mode
        if (theSpinDetectCount > 24 ) {
          std::string spin_string = "Addrs {";
          std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
          while (ma_iter != theSpinMemoryAddresses.end()) {
            spin_string += boost::lexical_cast<std::string>(*ma_iter) + ",";
            ++ma_iter;
          }
          spin_string += "} PCs {";
          std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
          while (va_iter != theSpinPCs.end()) {
            spin_string += boost::lexical_cast<std::string>(*va_iter) + ",";
            ++va_iter;
          }
          spin_string += "}";
          DBG_( Iface, Comp(*this) ( << " entering spin mode. " << spin_string ) );

          theSpinning = true;
        }
      }
    }
  }

  void doCommit() {
    DBG_( VVerb, Comp(*this) ( << "Comitting instructions" ));

    if (! theROB.empty()) {
      intrusive_ptr<ExecuteStateImpl> & insn_state = theROB.front();
      DBG_( VVerb, Comp(*this) ( << "doCommit - insn_state->isComplete: " << insn_state->isComplete()
                                 << " instruction: " << insn_state->instruction()));

      // We need to wait for SB becomes empty for this IO instruction becomes released ( InProgress->InProgress)
      if (insn_state->instruction().isIO() && !theSB.empty()) {
        theTimeBreakdown.stall(nTimeBreakdown::kSideEffect_SBDrain);

        DBG_( Verb, Comp(*this) ( << "Not-releasing (IO && !SB.empty) :" << insn_state->instruction() ));
      } else if (insn_state->isComplete()) {

        //Determine if we are/are not spinning
        spinDetect(insn_state);

        bool system = insn_state->instruction().isPriv();
        bool trap = false;
#if FLEXUS_TARGET_IS(v9)
        if ((insn_state->instruction().virtualInstructionAddress() >= 0x1000000)
            && (insn_state->instruction().virtualInstructionAddress() < 0x1008000)) {
          trap = true;
          system = false;
        }
#endif

#if FLEXUS_TARGET_IS(x86)
        bool advance_x86_counters = (insn_state->instruction().getIfPart() == Normal);
#endif

        if (theSpinning) {
          ++theSpinCommits;
#if FLEXUS_TARGET_IS(x86)
          if (advance_x86_counters)
            ++theSpinCommits_x86;
#endif
          if (system) {
            ++theSpinCommits_System;
#if FLEXUS_TARGET_IS(x86)
            if (advance_x86_counters)
              ++theSpinCommits_System_x86;
#endif
            theTimeBreakdown.spin(kSystem, 1, !theSB.empty());
          } else {
            if (trap) {
              ++theSpinCommits_Trap;
              theTimeBreakdown.spin(kTrap, 1, !theSB.empty());
            } else {
              ++theSpinCommits_User;
#if FLEXUS_TARGET_IS(x86)
              if (advance_x86_counters)
                ++theSpinCommits_User_x86;
#endif
              theTimeBreakdown.spin(kUser, 1, !theSB.empty());
            }
          }

        } else {
          ++theCommits;
#if FLEXUS_TARGET_IS(x86)
          if (advance_x86_counters)
            ++theCommits_x86;
#endif
          if (system) {
            ++theCommits_System;
#if FLEXUS_TARGET_IS(x86)
            if (advance_x86_counters)
              ++theCommits_System_x86;
#endif
            theTimeBreakdown.commit(kSystem, insn_state->stallClass(), 1);
          } else if (trap) {
            ++theCommits_Trap;
            theTimeBreakdown.commit(kTrap, insn_state->stallClass(), 1);
          } else {
            ++theCommits_User;
#if FLEXUS_TARGET_IS(x86)
            if (advance_x86_counters)
              ++theCommits_User_x86;
#endif
            theTimeBreakdown.commit(kUser, insn_state->stallClass(), 1);
          }
        }
        ++theCommitCount;

#if FLEXUS_TARGET_IS(x86)
        if (insn_state->instruction().getIfPart() == Hardware_Walk)
          theHardwareWalkInstrs++;
#endif

        DBG_( Verb, Comp(*this) ( << "Comitting:" << insn_state->instruction() ));
        insn_state->instruction().commit();
        if (insn_state->instruction().canRelease() && ! insn_state->instruction().isReleased()) {
          DBG_( Verb, Comp(*this) ( << "Releasing:" << insn_state->instruction() ));
          insn_state->instruction().release();
        }
        //Also remove from the head of the LSQ if present
        if (!theLSQ.empty() && (theROB.front() == theLSQ.front())) {
          theLSQ.pop_front();
        }
        theROB.pop_front();

      } else {
        //Account for possible stalls (other than accumulated stall cycles) when ROB is non-empty
        if ( ( cfg.SBSize > 0) && ( theSB.size() >= cfg.SBSize )  ) {
          theTimeBreakdown.stall(nTimeBreakdown::kStore_BufferFull);
        }
      }
    } else {
      //ROB empty
      theTimeBreakdown.stall(nTimeBreakdown::kEmptyROB_Unknown);
    }

    DBG_( VVerb, Comp(*this) ( << "Expunging performed stores" ));

    while (! theSB.empty() && theSB.front()->isPerformed()) {
      DBG_( Verb, Comp(*this) ( << "Expunging:" << theSB.front()->instruction() ));
      std::map< PhysicalMemoryAddress, boost::intrusive_ptr<ExecuteStateImpl> >::iterator iter =
        theSBMap.find( PhysicalMemoryAddress( theSB.front()->instruction().physicalMemoryAddress() & ~7LL ) ) ;
      if (iter != theSBMap.end()) {
        if ( iter->second.get() ==  theSB.front().get()) {
          DBG_( VVerb, Comp(*this) ( << "Removing from SBMap: " << theSB.front()->instruction() ));
          theSBMap.erase(iter);
        }
      }

      theSB.pop_front();
    }

    if (wait_SB_empty) {
      if (theSB.size() > 1) {
        theTimeBreakdown.stall(theWaitSBEmptyCause);
      } else if (theSB.size() == 0) {
        wait_SB_empty = false;  // Re-issuing instructions after this sync instruction is retired
      }
    }

    DBG_( VVerb, Comp(*this) ( << "Done committing instructions & expunging stores." ));
  }

  void initiateMemoryOperations() {

    DBG_( VVerb, Comp(*this) ( << "Initiating memory operations." ));
    //Issue up to MemoryWidth memory operations from LSQ
    deque< intrusive_ptr<ExecuteStateImpl> >::iterator lsq_iter;

    if (cfg.StorePrefetching && wait_SB_empty) {
      //Can't initiate memory operations when we are waiting for the SB
      //to drain
      return;
    }

    //Iterate over the LSQ from the beginning, up to MemoryWidth instructions
    for (lsq_iter = theLSQ.begin(); (theIssuedMemOps < cfg.MemoryWidth) && (lsq_iter != theLSQ.end()); ++lsq_iter) {
      intrusive_ptr<ExecuteStateImpl> & insn_state = *lsq_iter;

      //Don't try to reissue instructions that are already issued
      if (!insn_state->isInitiated() ) {

        //Only consider memory operations where the instruction has been issued
        //and the memory op can be performed
        if (insn_state->isIssued() && insn_state->instruction().canPerform() ) {

          //See if it is a store that can go in the store buffer
          if (insn_state->instruction().isStore() && ! insn_state->instruction().isSync() ) {

            //Store
            theNonSyncStores++;
            theStores++;

            // Enter the store in the SB and mark the instruction complete.
            // The store will be initiated when we next process the SB.
            theSB.push_back(insn_state);

            // JWK: st-prefetch
            if (cfg.StorePrefetching && (theSB.size() == 1)) {
              theSBprefetch_iter = theSB.begin();
              (*theSBprefetch_iter)->skipStorePrefetch(); // you don't want to prefetch a store at head
            }
            //////////

            theSBMap.insert( std::make_pair(insn_state->instruction().physicalMemoryAddress() & ~7ULL, insn_state) ) ;
            DBG_( VVerb, Comp(*this) ( << "Adding to SBMap: " << theSB.front()->instruction() ));

            // measure SB size
            theSBMaxSize << theSB.size();

            if (! cfg.SequentialConsistency) {
              //Under SC, the instruction isn't complete until it is performed.
              insn_state->complete();
            }

            // JWK: st-prefetch_check
          } else if (cfg.StorePrefetching && (insn_state->instruction().isRmw() || (insn_state->instruction().isSync() && insn_state->instruction().isStore()))) {
            //RMWs and synchronized stores under store prefetching

            if (insn_state->instruction().isRmw()) {
              theRMWs++;
              theWaitSBEmptyCause = nTimeBreakdown::kAtomic_SBDrain;
            }
            if (insn_state->instruction().isStore()) {
              theSyncStores++;
              theStores++;
              theWaitSBEmptyCause = nTimeBreakdown::kSideEffect_SBDrain;
            }

            if ( !(!insn_state->isStorePrefetched() && !insn_state->isStorePrefetchedDone() && !insn_state->isSkipStorePrefetch())) {
              DBG_Assert(!insn_state->isStorePrefetched() && !insn_state->isStorePrefetchedDone() && !insn_state->isSkipStorePrefetch());
            }

            // Enter the store in the SB and mark the instruction complete.
            // The store will be initiated when we next process the SB.
            theSB.push_back(insn_state);

            if (cfg.StorePrefetching > 0 && (theSB.size() == 1)) {
              theSBprefetch_iter = theSB.begin();
              (*theSBprefetch_iter)->skipStorePrefetch();
            }

            theSBMap.insert( std::make_pair(insn_state->instruction().physicalMemoryAddress() & ~7ULL, insn_state) ) ;
            // measure SB size
            theSBMaxSize << theSB.size();

            wait_SB_empty = true;   // prevent inssuing later instructions after this sync. instruction is executed.

          } else if (insn_state->instruction().isMEMBAR()) {

            // A MEMBAR may not execute until the store buffer is empty
            if (! theSB.empty()) {
              DBG_(VVerb, ( << "MEMBAR must wait for SB to empty.") );
              break;
            }
            insn_state->setStallClass(nTimeBreakdown::kMEMBAR);

            theMEMBARs++;

            insn_state->perform();
            insn_state->complete();

          } else {

            //Load or RMW or sync'd store (block store)
            bool hit_in_sb = false;

            if (insn_state->instruction().isSync()) {
              //Note - only uncached loads will enter this if branch under StorePrefetching

              //We will only initiate an RMW and block stores if the store
              //buffer is empty. This is neccessary to guarantee TSO
              if (! theSB.empty()) {
                break;
              }

              if (insn_state->instruction().isRmw()) {
                theRMWs++;
              } else if (insn_state->instruction().isLoad()) {
                theSyncLoads++;
                theLoads++;
              } else if (insn_state->instruction().isStore()) {
                theSyncStores++;
                theStores++;
              } else {
                DBG_Assert(false, ( << "Unknown sync instruction type") );
              }

            } else {
              //Check for the block in SB, if allowed
              if (insn_state->instruction().isLoad() && insn_state->instruction().size() <= 8) {
                theSBAccesses++;
                theNonSyncLoads++;
                theLoads++;

                PhysicalMemoryAddress aligned( insn_state->instruction().physicalMemoryAddress() & ~7 );
                std::map < PhysicalMemoryAddress, intrusive_ptr<ExecuteStateImpl> >::iterator iter = theSBMap.find( aligned ) ;
                if ( iter != theSBMap.end() ) {
                  //Possible hit in the SB.  Check the DoubleWord to see if it
                  //can satisfy this request.
                  int32_t size = insn_state->instruction().size();
                  uint32_t offset = insn_state->instruction().physicalMemoryAddress() & 7;
                  Flexus::SharedTypes::DoubleWord const & dw = iter->second->instruction().data();
                  if ( dw.isValid(size, offset) ) {
                    DBG_( VVerb, Comp(*this) ( << "SB Hit!: " << insn_state->instruction() << " and " << iter->second->instruction()  ));
                    insn_state->setStallClass(nTimeBreakdown::kLoad_Forwarded);
                    hit_in_sb = true;
                    theSBHits++;
                  } else {
                    DBG_( VVerb, Comp(*this) ( << "SB Partial Miss: " << insn_state->instruction() << " and " << iter->second->instruction()  ));
                    theSBPartialMisses++;
                  }
                } else {
                  theSBMisses++;
                }
              }
            }

            if ( hit_in_sb ) {

              insn_state->perform();
              insn_state->complete();

            } else {
              //Missed in the SB.  Need to issue to memory
              if ( FLEXUS_CHANNEL( ExecuteMemRequest).available() ) {
                DBG_( VVerb, Comp(*this) ( << "Initiating memory op: " << insn_state->instruction()  ));

                insn_state->initiate();

                issueMemoryOperation(insn_state);

                theIssuedMemOps++;

              } else {
                break; //Break if the channel isn't available
              }
            }

          }
        } else {
          break; //Break on first non-issued instruction
        }
      }
    }

    DBG_( VVerb, Comp(*this) ( << "Done issuing memory operations." ));
  }

  void issueStores() {
    //Only the head of the SB is eligible to be initiated.
    if (! theSB.empty() && theIssuedMemOps < cfg.MemoryWidth && FLEXUS_CHANNEL( ExecuteMemRequest).available()) {

      if (!cfg.StorePrefetching) {
        // no store prefetching
        if (! theSB.front()->isInitiated()) {
          theSB.front()->initiate();
          issueMemoryOperation(theSB.front());
          theIssuedMemOps++;
        }
      } else {
        // prefetching case
        // STORE-PREFETCH /////
        // JWK: SB drain from the head (re-issue)
        if (theSB.front()->isSkipStorePrefetch()) {
          if (!theSB.front()->isInitiated()) {
            // already prefetched, prefetch-done, real store not issued yet
            theSB.front()->initiate();
            issueMemoryOperation(theSB.front());
            theIssuedMemOps++;
          } else {
            // didn't need prefetch from start, and already issued to the memory
          }
        } else if (theSB.front()->isStorePrefetched()) {
          if (theSB.front()->isStorePrefetchedDone()) {
            if (!theSB.front()->isInitiated() ) {
              // already prefetched, prefetch-done, real store not issued yet
              theSB.front()->initiate();
              issueMemoryOperation(theSB.front());
              theIssuedMemOps++;
            } else {
              // already prefetched, prefetch-done, real store issued
            }
          } else {
            // already prefetched, should wait until this is completed.. then, re-issue
            DBG_Assert(!theSB.front()->isInitiated());
          }
        } else {
          // prefetch not skippable, didn't start prefetch yet..
          // Now, prefetch SB_header should be pointing this entry!!

          //DBG_Assert(theSBprefetch_iter == theSB.begin());
          // Then, let's make this one prefetch-skippable !
          DBG_Assert(!theSB.front()->isStorePrefetched());
          DBG_Assert(!theSB.front()->isStorePrefetchedDone());
          (*theSBprefetch_iter)->skipStorePrefetch();
          theSBprefetch_iter++;

          // this new store in the head of SB can be issued
          DBG_Assert(!theSB.front()->isInitiated());
          theSB.front()->initiate();
          issueMemoryOperation(theSB.front());
          theIssuedMemOps++;
        }
        // Now, prefetch cadidate-stores as int64_t as we have enough left-over bandwidth

        /*
                      if ((theSBprefetch_iter != theSB.end()) && (*theSBprefetch_iter)->isSkipStorePrefetch()) {
          DBG_Assert(theSBprefetch_iter == theSB.begin());
          theSBprefetch_iter++;
                      }
                      */

        // Commented routine above is broken.. Instead, simple fast-forwarding...
        theSBprefetch_iter = theSB.begin();
        while  ((theSBprefetch_iter != theSB.end()) && ((*theSBprefetch_iter)->isSkipStorePrefetch() || (*theSBprefetch_iter)->isStorePrefetched())) {
          theSBprefetch_iter++;
        }

        while ((theIssuedMemOps < cfg.MemoryWidth) && FLEXUS_CHANNEL( ExecuteMemRequest).available() /* && (theNumStorePrefetched < cfg.StorePrefetching)*/
               && (theSBprefetch_iter != theSB.end()) && (!(*theSBprefetch_iter)->isSkipStorePrefetch()) ) {

          if ( !(*theSBprefetch_iter)->isInitiated()) {
            DBG_Assert(!((*theSBprefetch_iter)->isStorePrefetched()) && !((*theSBprefetch_iter)->isStorePrefetchedDone()));
            //(*theSBprefetch_iter)->initiate();
            issueMemoryOperation(*theSBprefetch_iter);
            theIssuedMemOps++;

            /*       theNumStorePrefetched++;*/

            theSBprefetch_iter++;
            DBG_Assert(&(*theSBprefetch_iter) != 0);

          } else {
            // already initiated
            DBG_Assert(0);
          }
        }
      }
    }
  }

  void issueMemoryOperation(intrusive_ptr<ExecuteStateImpl> & anInstructionState) {
    MemoryTransport mem_op;
    intrusive_ptr<MemoryMessage> operation;

    bool store_to_be_prefetched = cfg.StorePrefetching && !anInstructionState->isSkipStorePrefetch()
                                  && !anInstructionState->isStorePrefetched() && !anInstructionState->isStorePrefetchedDone();

    if (anInstructionState->instruction().isLoad()) {
      DBG_( VVerb, Comp(*this) ( << "Issuing load to memory system: " << anInstructionState->instruction() ));

      operation = MemoryMessage::newLoad(anInstructionState->instruction().physicalMemoryAddress(), VirtualMemoryAddress( anInstructionState->instruction().virtualInstructionAddress() ));

    } else if (anInstructionState->instruction().isStore() || anInstructionState->instruction().isRmw()) {
      if (!store_to_be_prefetched) {
        if (anInstructionState->instruction().isRmw()) {
          DBG_( Iface, Comp(*this) ( << "Issuing RMW to memory system: " << anInstructionState->instruction() ));
          operation = MemoryMessage::newRMW(anInstructionState->instruction().physicalMemoryAddress(), VirtualMemoryAddress( anInstructionState->instruction().virtualInstructionAddress() ), DataWord(0) );
        } else {
          DBG_( VVerb, Comp(*this) ( << "Issuing Store to memory system: " << anInstructionState->instruction() ));
          operation = MemoryMessage::newStore(anInstructionState->instruction().physicalMemoryAddress(), VirtualMemoryAddress( anInstructionState->instruction().virtualInstructionAddress() ), DataWord(0) );
        }
      } else {
        DBG_( VVerb, Comp(*this) ( << "Issuing Store/RMW Prefetch to memory system: " << anInstructionState->instruction() ));
        operation = MemoryMessage::newStorePrefetch(anInstructionState->instruction().physicalMemoryAddress(), VirtualMemoryAddress( anInstructionState->instruction().virtualInstructionAddress() ), DataWord(0) );
      }
    } else {
      DBG_Assert(false, ( << "Got a memory operation that is neither a load or a store"));
      //Don't know what to do
    }

    if (anInstructionState->instruction().isPriv()) {
      operation->setPriv();
    }
    operation->reqSize() = 8; //Needs to be non-zero for the cache to work.

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress(anInstructionState->instruction().physicalMemoryAddress());
    tracker->setInitiator(flexusIndex());
    tracker->setSource("Execute");
    if (anInstructionState->instruction().isLoad() || anInstructionState->instruction().isSync()) {
      tracker->setCriticalPath(true);
    }
    tracker->setOS(anInstructionState->instruction().isPriv());
    tracker->setDelayCause(name(), "Issue");

    mem_op.set(MemoryMessageTag, operation);
    mem_op.set(ExecuteStateTag, anInstructionState);
    mem_op.set(TransactionTrackerTag, tracker);

    theTotalMemOps++;

    // Indicate that we made forward progress.
    // The SB can be tremendously big since it is infinite. Until it is drained,
    // a fetch can't happen, leading to the possibility of the watchdog timer to expire.
    // However, issuing requests from the SB should be considered forward progress.
    Flexus::Core::theFlexus->watchdogReset(flexusIndex());

    DBG_( Iface, Comp(*this) ( << "EX Issuing memory request: " << *operation ) Addr(anInstructionState->instruction().physicalMemoryAddress()));
    FLEXUS_CHANNEL( ExecuteMemRequest) << mem_op;

    if (store_to_be_prefetched) {
      anInstructionState->storePrefetched(); // makring this store prefetched
    }

  }

  void sendMemoryReplies() {
    DBG_( VVerb, Comp(*this) ( << "Sending inv/dg replies to memory system: " ));

    while ( ! theMemoryRequests.empty() && FLEXUS_CHANNEL( ExecuteMemSnoop).available()) {
      MemoryTransport request = theMemoryRequests.front();
      theMemoryRequests.pop_front();
      intrusive_ptr<MemoryMessage> request_msg(request[MemoryMessageTag]);
      intrusive_ptr<MemoryMessage> reply_msg;
      switch (request_msg->type()) {
        case MemoryMessage::Invalidate:
          reply_msg = new MemoryMessage(MemoryMessage::InvalidateAck, request_msg->address());
          break;
        case MemoryMessage::Downgrade:
          reply_msg = new MemoryMessage(MemoryMessage::DowngradeAck, request_msg->address());
          break;
        case MemoryMessage::Probe:
          reply_msg = new MemoryMessage(MemoryMessage::ProbedNotPresent, request_msg->address());
          break;
        case MemoryMessage::ReturnReq:
          reply_msg = new MemoryMessage(MemoryMessage::ReturnReply, request_msg->address());
          break;
        default:
          DBG_( Crit, Comp(*this) ( << "Received a request from memory that is not Invalidate, Downgrade, or Probe: " << *request_msg ) Addr(request_msg->address()) );
          DBG_Assert( 0 );
      }
      DBG_( Iface, Comp(*this) ( << "EX Issuing memory reply: " << *reply_msg) Addr(request_msg->address()));
      request.set(MemoryMessageTag, reply_msg);
      FLEXUS_CHANNEL(ExecuteMemSnoop) << request;
    }
  }

};

}  //End Namespace nExecute

FLEXUS_COMPONENT_INSTANTIATOR( Execute, nExecute );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Execute

#define DBG_Reset
#include DBG_Control()

