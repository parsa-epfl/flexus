#include "coreModelImpl.hpp"
#include <core/debug/severity.hpp>
#include "../ValueTracker.hpp"

#include <components/Common/Slices/FillLevel.hpp>
#include <components/Common/TraceTracker.hpp>

//#include <components/WhiteBox/WhiteBoxIface.hpp>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nv9Decoder {
extern uint32_t theInsnCount;
}

namespace nuArch {

void CoreImpl::cycle(int32_t aPendingInterrupt) {
  FLEXUS_PROFILE();

  if (aPendingInterrupt != thePendingInterrupt) {
    if (aPendingInterrupt == 0) {
      DBG_(Iface, ( << theName << " Interrupt: " << std::hex << thePendingInterrupt << std::dec << " pending for " << (theFlexus->cycleCount() - theInterruptReceived) ) );
    } else {
      theInterruptReceived = theFlexus->cycleCount();
    }
    thePendingInterrupt = aPendingInterrupt;
  }

  if (nv9Decoder::theInsnCount > 10000ULL && ( static_cast<uint64_t>(theFlexus->cycleCount() - theLastGarbageCollect) > 1000ULL - nv9Decoder::theInsnCount / 100))  {
    FLEXUS_PROFILE_N("CoreImpl::cycle() collect-dead-dependencies");
    DBG_( Iface, ( << theName << "Garbage-collect count before clean:  " << nv9Decoder::theInsnCount ) );
    theBypassNetwork.collectAll();
    DBG_( Iface, ( << theName << "Garbage-collect between bypass and registers:  " << nv9Decoder::theInsnCount ) );
    theRegisters.collectAll();
    DBG_( Iface, ( << theName << "Garbage-collect count after clean:  " << nv9Decoder::theInsnCount ) );
    theLastGarbageCollect = theFlexus->cycleCount();

    if ( nv9Decoder::theInsnCount > 1000000 ) {
      DBG_( Crit, ( << theName << "Garbage-collect detects too many live instructions.  Forcing resynchronize.") );
      ++theResync_GarbageCollect;
      throw ResynchronizeWithSimicsException();
    }
  }

  DBG_( Verb, ( << theName << " Core Status {" << theFlexus->cycleCount() << "}" << (theSpinning ? " <spinning>" : "" ) << (theIsSpeculating ? " <speculating>" : "" ) <<  "\n========================================================" ) );

  //Update TICK, STICK
  ++theTICK;
  --theSTICK_tillIncrement;
  if (theSTICK_tillIncrement <= 0) {
    ++theSTICK;
    theSTICK_tillIncrement = theSTICK_interval;
  }

  if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 3) {
    dumpROB();
    dumpSRB();
    dumpMemQueue();
    dumpMSHR();
    dumpCheckpoints();
    dumpSBPermissions();
    //dumpActions();
  }

  if (theIsSpeculating) {
    DBG_Assert( theCheckpoints.size() > 0);
    DBG_Assert( ! theSRB.empty() );
    DBG_Assert( theSBCount + theSBNAWCount > 0 );
    DBG_Assert( theOpenCheckpoint != 0);
    DBG_Assert( theCheckpoints.find( theOpenCheckpoint ) != theCheckpoints.end() );

#ifdef VALIDATE_STORE_PREFETCHING
    //Ensure that the required blocks are somewhere
    std::map< boost::intrusive_ptr< Instruction >, Checkpoint >::iterator ckpt = theCheckpoints.begin();
    while (ckpt != theCheckpoints.end()) {
      //Walk through the required permissions, make sure the block is somewhere
      std::map< PhysicalMemoryAddress, boost::intrusive_ptr< Instruction > >::iterator iter, end;
      iter = ckpt->second.theRequiredPermissions.begin();
      end = ckpt->second.theRequiredPermissions.end();
      while (iter != end) {
        if (! ckpt->second.theHeldPermissions.count( iter->first ) > 0) {
          //permission not held
          if (! theOutstandingStorePrefetches.count( iter->first ) > 0 ) {
            if (! theWaitingStorePrefetches.count( iter->first ) > 0 ) {
              if (! theBlockedStorePrefetches.count( iter->first ) > 0 ) {
                Flexus::Core::theFlexus->setDebug("verb");
                dumpCheckpoints();
                dumpMSHR();
                dumpSBPermissions();
                DBG_Assert( false, ( << theName << " required store address is lost: " << iter->first << " required by " << *iter->second ) );
              } else {
                bool found = false;
                MSHRs_t::iterator miter, mend;
                for ( miter = theMSHRs.begin(), mend = theMSHRs.end(); miter != mend; ++miter) {
                  if ((miter->first & ~63) == iter->first) {
                    found = true;
                    break;
                  }
                }
                if (! found) {
                  Flexus::Core::theFlexus->setDebug("verb");
                  dumpCheckpoints();
                  dumpMSHR();
                  dumpSBPermissions();
                  DBG_Assert( false, ( << theName << " blocked prefetch for: " << iter->first << " has no obvious blocking MSHR. ") );
                }
              }
            } else {
              DBG_Assert( ! theWaitingStorePrefetches[iter->first].empty() );
              if ( theMemoryPortArbiter.theStorePrefetchRequests.get<by_insn>().find( *theWaitingStorePrefetches[iter->first].begin()) == theMemoryPortArbiter.theStorePrefetchRequests.get<by_insn>().end() ) {
                Flexus::Core::theFlexus->setDebug("verb");
                dumpCheckpoints();
                dumpMSHR();
                dumpSBPermissions();
                DBG_Assert( false, ( << theName << " no queued prefetch request for: " << **theWaitingStorePrefetches[iter->first].begin()) );

              }
            }
          }
        }
        ++iter;
      }
      ++ckpt;
    }
#endif //VALIDATE STORE PREFETCHING
  } else {
    DBG_Assert( theCheckpoints.size() == 0);
    DBG_Assert( theSRB.empty() );
    DBG_Assert( theOpenCheckpoint == 0);
  }

  DBG_Assert((theSBCount + theSBNAWCount) >= 0);
  DBG_Assert((static_cast<int> (theSBLines_Permission.size())) <= theSBCount + theSBNAWCount );

  DBG_( Verb, ( << "*** Prepare *** " ) );

  processMemoryReplies();
  prepareCycle();

  DBG_( Verb, ( << "*** Eval *** " ) );
  evaluate();

  DBG_( Verb, ( << "*** Issue Mem *** " ) );

  issuePartialSnoop();
  issueStore();
  issueAtomic();
  issueAtomicSpecWrite();
  issueSpecial();
  valuePredictAtomic();
  checkExtraLatencyTimeout();
  resolveCheckpoint();

  DBG_( Verb, ( << "*** Arb *** " ) );
  arbitrate();

  //Retire instruction from the ROB to the SRB
  retire();

  if (theRetireCount > 0) {
    theIdleThisCycle = false;
  }

  //Do cycle accounting
  completeAccounting();

  if (theTSOBReplayStalls > 0) {
    -- theTSOBReplayStalls;
  }

  while (! theBranchFeedback.empty()) {
    feedback_fn(theBranchFeedback.front());
    DBG_( Verb, ( << " Sent Branch Feedback") );
    theBranchFeedback.pop_front();
    theIdleThisCycle = false;
  }

  if (theSquashRequested) {
    DBG_( Verb, ( << " Core triggering Squash: " << theSquashReason) );
    doSquash();
    squash_fn(theSquashReason);
    theSquashRequested = false;
    theIdleThisCycle = false;
  }

  //Commit instructions the SRB and compare to simics
  commit();

  handleTrap();
  handlePopTL();

  if (theRedirectRequested) {
    DBG_( Verb, ( << " Core triggering Redirect to " << theRedirectPC ) );
    redirect_fn(theRedirectPC, theRedirectNPC);
    thePC = theRedirectPC;
    theNPC = boost::none;
    theRedirectRequested = false;
    theIdleThisCycle = false;
  }

  DBG_( Verb, ( << theName << " End Cycle {" << theFlexus->cycleCount() << "}\n========================================================\n" ) );

  if (theIdleThisCycle) {
    ++theIdleCycleCount;
  } else {
    theIdleCycleCount = 0;
    DBG_( VVerb, ( << theName << " Non-Idle" ) );
  }
  theIdleThisCycle = true;

  rob_t::iterator i;
  for (i = theROB.begin(); i != theROB.end(); ++i) {
    i->get()->decrementCanRetireCounter();
  }

}

void CoreImpl::prepareCycle() {
  FLEXUS_PROFILE();
  thePreserveInteractions = false;
  if (! theRescheduledActions.empty() || ! theActiveActions.empty()) {
    theIdleThisCycle = false;
  }

  std::swap( theRescheduledActions, theActiveActions );
}

void CoreImpl::evaluate() {
  FLEXUS_PROFILE();
  while (! theActiveActions.empty()) {
    theActiveActions.top()->evaluate();
    theActiveActions.pop();
  }
}

void CoreImpl::arbitrate() {
  FLEXUS_PROFILE();
  theMemoryPortArbiter.arbitrate();
}

void CoreImpl::satisfy( InstructionDependance  const & aDep) {
  aDep.satisfy();
}
void CoreImpl::squash( InstructionDependance  const & aDep) {
  aDep.squash();
}
void CoreImpl::satisfy( std::list<InstructionDependance > & dependances) {
  std::list<InstructionDependance >::iterator iter = dependances.begin();
  std::list<InstructionDependance >::iterator end = dependances.end();
  while ( iter != end) {
    iter->satisfy();
    ++iter;
  }
}
void CoreImpl::squash( std::list<InstructionDependance > & dependances) {
  std::list<InstructionDependance >::iterator iter = dependances.begin();
  std::list<InstructionDependance >::iterator end = dependances.end();
  while ( iter != end) {
    iter->squash();
    ++iter;
  }
}

void CoreImpl::spinDetect( memq_t::index<by_insn>::type::iterator iter) {
  if (theSpinning) {
    //Check leave conditions
    bool leave = false;

    if (!leave && iter->theOperation != kLoad) {
      leave = true;
    }

    if (!leave && iter->isAbnormalAccess()) {
      leave = true;
    }

    if (!leave && theSpinMemoryAddresses.count(iter->thePaddr) == 0) {
      leave = true;
    }

    if (!leave && theSpinPCs.count(iter->theInstruction->pc()) == 0) {
      leave = true;
    }

    ++theSpinDetectCount;

    if (leave) {
      std::string spin_string = "Addrs {";
      std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
      while (ma_iter != theSpinMemoryAddresses.end()) {
        spin_string += std::to_string(*ma_iter) + ",";
        ++ma_iter;
      }
      spin_string += "} PCs {";
      std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
      while (va_iter != theSpinPCs.end()) {
        spin_string += std::to_string(*va_iter) + ",";
        ++va_iter;
      }
      spin_string += "}";
      DBG_( Iface, ( << theName << " leaving spin mode. " << spin_string ) );

      theSpinning = false;
      theSpinCount += theSpinDetectCount;
      theSpinCycles += theFlexus->cycleCount() - theSpinStartCycle;
      theSpinRetireSinceReset = 0;
      theSpinDetectCount = 0;
      theSpinMemoryAddresses.clear();
      theSpinPCs.clear();
    }
  } else {
    //Check all reset conditions
    bool reset = false;

    if (!reset && iter->theOperation != kLoad) {
      reset = true;
    }

    if (!reset && iter->isAbnormalAccess()) {
      reset = true;
    }

    theSpinMemoryAddresses.insert(iter->thePaddr);
    if (!reset && theSpinMemoryAddresses.size() > 3) {
      reset = true;
    }

    theSpinPCs.insert(iter->theInstruction->pc());
    if (!reset && theSpinPCs.size() > 3) {
      reset = true;
    }

    ++theSpinDetectCount;
    if (reset) {
      theSpinRetireSinceReset = 0;
      theSpinDetectCount = 0;
      theSpinMemoryAddresses.clear();
      theSpinPCs.clear();
    } else {
      //Check condition for entering spin mode
      if (theSpinDetectCount > 24 || theSpinRetireSinceReset > 99) {
        std::string spin_string = "Addrs {";
        std::set<PhysicalMemoryAddress>::iterator ma_iter = theSpinMemoryAddresses.begin();
        while (ma_iter != theSpinMemoryAddresses.end()) {
          spin_string += std::to_string(*ma_iter) + ",";
          ++ma_iter;
        }
        spin_string += "} PCs {";
        std::set<VirtualMemoryAddress>::iterator va_iter = theSpinPCs.begin();
        while (va_iter != theSpinPCs.end()) {
          spin_string += std::to_string(*va_iter) + ",";
          ++va_iter;
        }
        spin_string += "}";
        DBG_( Iface, ( << theName << " entering spin mode. " << spin_string ) );

        theSpinning = true;
        theSpinStartCycle = theFlexus->cycleCount();
      }
    }
  }
}

void CoreImpl::checkTranslation( boost::intrusive_ptr<Instruction> anInsn) {
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(  iter != theMemQueue.get<by_insn>().end());

  if (!anInsn->isAnnulled() && !anInsn->isSquashed()) {
    Flexus::Qemu::Translation xlat;
    xlat.theVaddr = iter->theVaddr;
    xlat.theASI = iter->theASI;
    xlat.theTL = getTL();
    xlat.thePSTATE = getPSTATE() ;
    xlat.theType = ( iter->isStore() ? Flexus::Qemu::Translation::eStore :  Flexus::Qemu::Translation::eLoad) ;
    translate(xlat, true);
    iter->theException = xlat.theException;
    if (iter->theException != 0 ) {
      DBG_(Verb, ( <<  theName << " Taking MMU exception: " << iter->theException << " "  << *iter ) );
      takeTrap(anInsn, iter->theException);
      anInsn->setAccessAddress(PhysicalMemoryAddress(0));
    }
  }
}

void CoreImpl::createCheckpoint( boost::intrusive_ptr<Instruction> anInsn) {
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ckpt;
  bool is_new;
  std::tie(ckpt, is_new) = theCheckpoints.insert( std::make_pair( anInsn, Checkpoint() ) );
  anInsn->setHasCheckpoint(true);
  getv9State( ckpt->second.theState );
  Flexus::Qemu::Processor::getProcessor( theNode )->ckptMMU();
  DBG_(Verb, ( << theName << "Collecting checkpoint for " << *anInsn) );
  DBG_Assert(is_new);

  theMaxSpeculativeCheckpoints << theCheckpoints.size();
  DBG_Assert( theAllowedSpeculativeCheckpoints >= 0);
  DBG_Assert( theAllowedSpeculativeCheckpoints == 0 || (static_cast<int> ( theCheckpoints.size())) <= theAllowedSpeculativeCheckpoints );
  theOpenCheckpoint = anInsn;
  theRetiresSinceCheckpoint = 0;

  startSpeculating();
}

void CoreImpl::freeCheckpoint( boost::intrusive_ptr<Instruction> anInsn) {
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ckpt = theCheckpoints.find( anInsn );
  DBG_Assert( ckpt != theCheckpoints.end() );
  theCheckpoints.erase(ckpt);
  anInsn->setHasCheckpoint(false);
  Flexus::Qemu::Processor::getProcessor( theNode )->releaseMMUCkpt();
  if (theOpenCheckpoint == anInsn) {
    theOpenCheckpoint = 0;
  }
  checkStopSpeculating();
}

void CoreImpl::requireWritePermission( memq_t::index<by_insn>::type::iterator aWrite) {
  PhysicalMemoryAddress addr( aWrite->thePaddr & ~( theCoherenceUnit - 1) );
  if (aWrite->thePaddr > 0) {
    std::map<PhysicalMemoryAddress, std::pair<int, bool> >::iterator sbline;
    bool is_new;
    DBG_( Iface, ( << theName << "requireWritePermission : " << *aWrite ) );
    std::tie(sbline, is_new) = theSBLines_Permission.insert( std::make_pair( addr, std::make_pair(1, false) ) );
    if ( is_new ) {
      ++theSBLines_Permission_falsecount;
      DBG_Assert( theSBLines_Permission_falsecount >= 0);
      DBG_Assert( theSBLines_Permission_falsecount <= (static_cast<int> (theSBLines_Permission.size())) );
    } else {
      sbline->second.first++;
    }
    if (! sbline->second.second) {
      requestStorePrefetch(aWrite);
    }
    if (theOpenCheckpoint) {
      if (theConsistencyModel == kSC || aWrite->isAtomic()) {
        theCheckpoints[theOpenCheckpoint].theRequiredPermissions.insert( std::make_pair(addr, aWrite->theInstruction) );
        if (sbline->second.second) {
          theCheckpoints[theOpenCheckpoint].theHeldPermissions.insert( addr );
        }
      }
    }
  }
}

void CoreImpl::unrequireWritePermission( PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr( static_cast<uint64_t>(anAddress) & ~( theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::pair<int, bool> >::iterator sbline = theSBLines_Permission.find( addr );
  if ( sbline != theSBLines_Permission.end()) {
    --sbline->second.first;
    DBG_Assert(sbline->second.first >= 0);
    if (sbline->second.first == 0) {
      DBG_(Iface, ( << theName << " Stop tracking: " << anAddress ) );
      if (! sbline->second.second) {
        --theSBLines_Permission_falsecount;
        DBG_Assert( theSBLines_Permission_falsecount >= 0 );
      }

      theSBLines_Permission.erase(sbline);
    }
  }
}

void CoreImpl::acquireWritePermission( PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr( static_cast<uint64_t>(anAddress) & ~( theCoherenceUnit - 1));
  std::map<PhysicalMemoryAddress, std::pair<int, bool> >::iterator sbline = theSBLines_Permission.find( addr );
  if ( sbline != theSBLines_Permission.end()) {
    if (! sbline->second.second) {
      DBG_( VVerb, ( << theName << " Acquired write permission for address in SB: " << addr ));
      --theSBLines_Permission_falsecount;
      DBG_Assert( theSBLines_Permission_falsecount >= 0 );
    }
    sbline->second.second = true;
    std::map< boost::intrusive_ptr< Instruction >, Checkpoint >::iterator ckpt = theCheckpoints.begin();
    while (ckpt != theCheckpoints.end()) {
      if (ckpt->second.theRequiredPermissions.find( addr ) != ckpt->second.theRequiredPermissions.end() ) {
        ckpt->second.theHeldPermissions.insert( addr );
      }
      ++ckpt;
    }
  }
}

void CoreImpl::loseWritePermission( eLoseWritePermission aReason, PhysicalMemoryAddress anAddress) {
  PhysicalMemoryAddress addr( anAddress & ~( theCoherenceUnit - 1) );
  std::map<PhysicalMemoryAddress, std::pair<int, bool> >::iterator sbline = theSBLines_Permission.find( addr );
  if ( sbline != theSBLines_Permission.end()) {
    if (sbline->second.second) {
      switch ( aReason) {
        case eLosePerm_Invalidate:
          DBG_( Verb, ( << theName << " Invalidate removed write permission for line in store buffer:" << addr ));
          ++theInv_HitSB;
          break;
        case eLosePerm_Downgrade:
          DBG_( Verb, ( << theName << " Downgrade removed write permission for line in store buffer: " << addr));
          ++theDG_HitSB;
          break;
        case eLosePerm_Replacement:
          DBG_( Verb, ( << theName << " Replacement removed write permission for line in store buffer:" << addr ));
          ++theRepl_HitSB;
          break;
      }
      ++theSBLines_Permission_falsecount;
      DBG_Assert( theSBLines_Permission_falsecount >= 0);
      DBG_Assert( theSBLines_Permission_falsecount <= (static_cast<int> (theSBLines_Permission.size())) );
    }
    sbline->second.second = false;

    std::map< boost::intrusive_ptr< Instruction >, Checkpoint >::iterator ckpt = theCheckpoints.begin();
    bool requested = false;
    while (ckpt != theCheckpoints.end()) {
      if (ckpt->second.theHeldPermissions.count( addr ) > 0) {
        ckpt->second.theHeldPermissions.erase( addr );
        ckpt->second.theLostPermissionCount++;

        if (ckpt->second.theLostPermissionCount > 5) {
          //Abort if lost permission count exceeds threshold
          if (! theAbortSpeculation || theViolatingInstruction->sequenceNo() > ckpt->second.theRequiredPermissions[addr]->sequenceNo()) {
            theAbortSpeculation = true;
            theViolatingInstruction = ckpt->second.theRequiredPermissions[addr];
          }
        } else if (! requested) {
          //otherwise, re-request permission
          requestStorePrefetch(ckpt->second.theRequiredPermissions[addr]);
        }
      }
      ++ckpt;
    }
  }
}

void CoreImpl::retireMem( boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  DBG_Assert(  iter != theMemQueue.get<by_insn>().end());

  //Spin detection / control
  if (! anInsn->isAnnulled() ) {
    spinDetect(iter);
  }

  DBG_Assert(  iter->theQueue == kLSQ );

  if (iter->theOperation == kCAS || iter->theOperation == kRMW ) {
    if (!iter->isAbnormalAccess() && iter->thePaddr != 0) {
      PhysicalMemoryAddress block_addr( static_cast<uint64_t>(iter->thePaddr) & ~( theCoherenceUnit - 1) );
      addSLATEntry( block_addr, anInsn );

      //TRACE TRACKER : Notify trace tracker of store
      //uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/, anInsn->isPriv(), 0 );
      /* CMU-ONLY-BLOCK-BEGIN */
      if (theTrackParallelAccesses ) {
        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr, iter->theParallelAddresses);
        /*
                  DBG_( Dev,  ( << theName << " CAS/RMW " << std::hex << iter->thePaddr << std::dec << " parallel set follows " ) );
                  std::set< PhysicalMemoryAddress >::iterator pal_iter = iter->theParallelAddresses.begin();
                  std::set< PhysicalMemoryAddress >::iterator pal_end = iter->theParallelAddresses.end();
                  while (pal_iter != pal_end ) {
                    DBG_ ( Dev, ( << "    " << std::hex << *pal_iter << std::dec ) );
                    ++pal_iter;
                  }
         */
      }
      /* CMU-ONLY-BLOCK-END */
    }

    if (iter->theOperation == kRMW) {
      ++theAtomicVal_RMWs;
      DBG_Assert(iter->theExtendedValue);
      if (*iter->theExtendedValue == 0) {
        ++theAtomicVal_RMWs_Zero;
      } else {
        ++theAtomicVal_RMWs_NonZero;
      }
    } else { /* CAS */
      ++theAtomicVal_CASs;
      DBG_Assert(iter->theExtendedValue, ( << *iter ) );
      DBG_Assert(iter->theCompareValue, ( << *iter ) );
      if (*iter->theExtendedValue == 0) {
        ++theAtomicVal_CASs_Zero;
      } else {
        ++theAtomicVal_CASs_NonZero;
      }
      if (*iter->theExtendedValue == *iter->theCompareValue) {
        ++theAtomicVal_CASs_Match;
        if (theAtomicVal_LastCASMismatch) {
          ++theAtomicVal_CASs_MatchAfterMismatch;
        }
        theAtomicVal_LastCASMismatch = false;
      } else {
        ++theAtomicVal_CASs_Mismatch;
        if (theAtomicVal_LastCASMismatch) {
          ++theAtomicVal_CASs_MismatchAfterMismatch;
        }
        theAtomicVal_LastCASMismatch = true;
      }
    }

    if ( iter->theStoreComplete == true) {
      //Completed non-speculatively
      eraseLSQ( anInsn ); //Will setAccessAddress
    } else {
      //Speculatively retiring an atomic
      DBG_Assert( theSpeculativeOrder );
      DBG_(Trace, ( << theName << " Spec-Retire Atomic: " << *iter ) );

      theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      --theLSQCount;
      DBG_Assert( theLSQCount >= 0);
      ++theSBCount;

      iter->theIssued = false;

      DBG_Assert( iter->thePaddr != kUnresolved );
      DBG_Assert( ! iter->isAbnormalAccess() );
      iter->theInstruction->setAccessAddress( iter->thePaddr );

      createCheckpoint( iter->theInstruction );
      requireWritePermission( iter );

    }
  } else if (iter->theOperation == kLoad ) {
    DBG_Assert(theAllowedSpeculativeCheckpoints >= 0);
    bool speculate = false;
    if (!iter->isAbnormalAccess() && ( (iter->thePaddr & ~(theCoherenceUnit - 1)) != 0 ) ) {
      if ( consistencyModel() == kSC && theSpeculativeOrder) {
        if (    (!isSpeculating() && !sbEmpty()) //mandatory checkpoint
                || (  isSpeculating()  //optional checkpoint
                      && theCheckpointThreshold > 0
                      && theRetiresSinceCheckpoint > theCheckpointThreshold
                      && ( theAllowedSpeculativeCheckpoints == 0 || (static_cast<int> (theCheckpoints.size())) < theAllowedSpeculativeCheckpoints)
                   )
                || (  isSpeculating()  //Checkpoint may not require more than 15 stores
                      && theCheckpoints[theOpenCheckpoint].theRequiredPermissions.size() > 15
                      && ( theAllowedSpeculativeCheckpoints == 0 || (static_cast<int> (theCheckpoints.size())) < theAllowedSpeculativeCheckpoints)
                   )
           ) {
          speculate = true;
          //Must enter speculation mode to retire load past store
          DBG_(Verb, ( << theName << " Entering SC speculation on: " << *iter ) );
          iter->theInstruction->setMayCommit(false);

          //Move the MEMBAR to the SSB and update counts
          theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
          --theLSQCount;
          DBG_Assert( theLSQCount >= 0);
          ++theSBNAWCount;
          iter->theBypassSB = true;
          createCheckpoint( iter->theInstruction );
        }
      }

      PhysicalMemoryAddress block_addr( static_cast<uint64_t>(iter->thePaddr) & ~( theCoherenceUnit - 1) );
      addSLATEntry( block_addr, anInsn );

      //TRACE TRACKER : Notify trace tracker of load commit
      //uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.access(theNode, eCore, iter->thePaddr, anInsn->pc(), false, false, false, anInsn->isPriv(), 0) ;
      theTraceTracker.commit(theNode, eCore, iter->thePaddr, anInsn->pc(), 0 );
      /* CMU-ONLY-BLOCK-BEGIN */
      if (theTrackParallelAccesses ) {
        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr, iter->theParallelAddresses);
      }
      /* CMU-ONLY-BLOCK-END */
    }
    if (! speculate) {
      eraseLSQ( anInsn ); //Will setAccessAddress
    }
  } else if (iter->theOperation == kStore) {
    if (anInsn->isAnnulled() || iter->isAbnormalAccess()) {
      if (! anInsn->isAnnulled()  && iter->theMMU) {
        DBG_Assert(mmuASI(iter->theASI));
        DBG_( Verb, ( << theName << " MMU write: " << *iter ) );
        Flexus::Qemu::Processor::getProcessor( theNode )->mmuWrite(iter->theVaddr, iter->theASI, *iter->theValue );
      }
      eraseLSQ( anInsn );
    } else {
      //See if this store is to the same cache block as the preceding store
      memq_t::index<by_seq>::type::iterator pred = theMemQueue.project<by_seq>(iter);
      if (pred != theMemQueue.begin()) {
        --pred;
        DBG_Assert( pred->theQueue != kLSQ );
        PhysicalMemoryAddress pred_aligned( pred->thePaddr & ~( theCoherenceUnit - 1) );
        PhysicalMemoryAddress iter_aligned( iter->thePaddr & ~( theCoherenceUnit - 1) );
        if (iter_aligned != 0 && pred_aligned == iter_aligned && pred->theOperation == kStore ) {
          //Can coalesce this with preceding SB enry
          ++theCoalescedStores;
          iter->theBypassSB;
        }
      }

      theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      --theLSQCount;
      DBG_Assert( theLSQCount >= 0);

      if (iter->theBypassSB) {
        ++theSBNAWCount;
      } else {
        ++theSBCount;
      }

      //TRACE TRACKER : Notify trace tracker of store
      //uint64_t logical_timestamp = theCommitNumber + theSRB.size();
      theTraceTracker.store(theNode, eCore, iter->thePaddr, anInsn->pc(), false /*unknown*/, anInsn->isPriv(), 0 );
      /* CMU-ONLY-BLOCK-BEGIN */
      if (theTrackParallelAccesses ) {
        theTraceTracker.parallelList(theNode, eCore, iter->thePaddr, iter->theParallelAddresses);
        /*
                  DBG_( Dev,  ( << theName << " SR " << std::hex << iter->thePaddr << std::dec << " parallel set follows " ) );
                  std::set< PhysicalMemoryAddress >::iterator pal_iter = iter->theParallelAddresses.begin();
                  std::set< PhysicalMemoryAddress >::iterator pal_end = iter->theParallelAddresses.end();
                  while (pal_iter != pal_end ) {
                    DBG_ ( Dev, ( << "    " << std::hex << *pal_iter << std::dec ) );
                    ++pal_iter;
                  }
        */
      }
      /* CMU-ONLY-BLOCK-END */

      requireWritePermission( iter );

    }
  } else if (iter->theOperation == kMEMBARMarker) {
    //Retiring a MEMBAR Marker
    if (sbEmpty()) {
      DBG_( Verb, ( << theName << " Non-Spec-Retire MEMBAR: " << *iter) );
      iter->theInstruction->resolveSpeculation();
      eraseLSQ( anInsn );
    } else {
      DBG_(Verb, ( << theName << " Spec-Retire MEMBAR: " << *iter ) );

      //!sbFull() should be in the retirement constraint for MEMBARs
      DBG_Assert( ! sbFull() );

      //Move the MEMBAR to the SSB and update counts
      theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSSB );
      --theLSQCount;
      DBG_Assert( theLSQCount >= 0);
      ++theSBCount;

      if (theConsistencyModel == kSC) {
        if (! isSpeculating()) {
          createCheckpoint( iter->theInstruction );
        } else {
          iter->theInstruction->resolveSpeculation(); //Will commit with its atomic sequence
        }
      } else if (theConsistencyModel == kTSO ) {
        createCheckpoint( iter->theInstruction );
      }

    }
  } else {
    DBG_Assert( false, ( << "Tried to retire non-load or non-store: " << *iter ) );
  }
}

bool CoreImpl::mayRetire_MEMBARStSt( ) const {
  if (consistencyModel() != kRMO) {
    return true;
  } else {
    return sbEmpty();
  }
}

bool CoreImpl::mayRetire_MEMBARStLd( ) const {
  switch (consistencyModel()) {
    case kSC:
      if (theSpeculativeOrder) {
        return ! sbFull();
      } else {
        return sbEmpty();
      }
    case kTSO:
      DBG_Assert( theAllowedSpeculativeCheckpoints >= 0);
      if (theSpeculativeOrder) {
        return ! sbFull() && ( theAllowedSpeculativeCheckpoints == 0 ? true : (static_cast<int> (theCheckpoints.size())) < theAllowedSpeculativeCheckpoints );
      } else {
        return sbEmpty();
      }
    case kRMO:
      DBG_Assert( theAllowedSpeculativeCheckpoints >= 0);
      if (theSpeculativeOrder) {
        return ! sbFull() && ( theAllowedSpeculativeCheckpoints == 0 ? true : (static_cast<int> (theCheckpoints.size())) < theAllowedSpeculativeCheckpoints );
      } else {
        return sbEmpty();
      }
  }

  return true;
}

bool CoreImpl::mayRetire_MEMBARSync( ) const {
  if (theSBNAWCount > 0 && theNAWWaitAtSync) {
    return false;
  }
  return mayRetire_MEMBARStLd();
}

void CoreImpl::commitStore( boost::intrusive_ptr<Instruction> anInsn) {
  FLEXUS_PROFILE();
  memq_t::index<by_insn>::type::iterator iter = theMemQueue.get<by_insn>().find(anInsn);
  if ( iter != theMemQueue.get<by_insn>().end()) {
    DBG_Assert( iter->theOperation == kStore );
    DBG_Assert( iter->theQueue == kSSB );
    DBG_Assert ( ! anInsn->isAnnulled() );
    DBG_Assert ( ! anInsn->isSquashed() );
    DBG_Assert( ! iter->isAbnormalAccess() );
    DBG_Assert( iter->theValue );
    DBG_Assert( iter->thePaddr != 0 );

    uint64_t value = *iter->theValue;
    if (iter->theInverseEndian) {
      value = Flexus::Qemu::endianFlip(value, iter->theSize);
      DBG_(Verb, ( << theName << " Inverse endian store: " << *iter << " inverted value: " <<  std::hex << value << std::dec ) );
    }
    ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theNode)).store( theNode, iter->thePaddr, iter->theSize, value);
    theMemQueue.get<by_insn>().modify( iter, [](auto& x){ x.theQueue = kSB; });//ll::bind( &MemQueueEntry::theQueue, ll::_1 ) = kSB );
  }
}

void CoreImpl::retire() {
  FLEXUS_PROFILE();
  bool stop_retire = false;
  theRetireCount = 0;
  while (   ! theROB.empty()
            && ! stop_retire
        ) {
    if (! theROB.front()->mayRetire() ) {
      break;
    }

    if (theTSOBReplayStalls > 0) {
      break;
    }

    //Check for sufficient speculative checkpoints
    DBG_Assert( theAllowedSpeculativeCheckpoints >= 0);
    if (    theIsSpeculating
            && theAllowedSpeculativeCheckpoints > 0
            && theROB.front()->instClass() == clsAtomic
            && !theROB.front()->isAnnulled()
            && (static_cast<int> (theCheckpoints.size())) >= theAllowedSpeculativeCheckpoints
       ) {
      break;
    }

    if (thePendingInterrupt && theIsSpeculating) {
      // stop retiring so we can stop speculating and handle the interrupt
      break;
    }

    if ( ( theROB.front()->resync() || (theROB.front() == theInterruptInstruction) ) && theIsSpeculating ) {
      //Do not retire sync instructions or interrupts while speculating
      break;
    }

    //Stop retirement for the cycle if we retire an instruction that
    //requires resynchronization
    if ( theROB.front()->resync()) {
      stop_retire = true;
    }

    //FOR in-order SMS Experiments only - not normal in-order
    //Under theInOrderMemory, we do not allow stores or atomics to retire
    //unless the OoO core is fully idle.  This is not neccessary for loads -
    //they will only issue when they reach the MemQueue head and the core is
    //fully stalled
    //if ( theInOrderMemory && (theROB.front()->instClass() == clsStore || theROB.front()->instClass() == clsAtomic ) && ( !isFullyStalled() || theRetireCount > 0)) {
    //break;
    //}

    theSpinRetireSinceReset++;

    DBG_( Iface, ( << theName << " Retire:" << *theROB.front() ) );
    if (!acceptInterrupt()) {
      theROB.front()->checkTraps();  // take traps only if we don't take interrupt
    }
    if (thePendingTrap != 0) {
      theROB.front()->changeInstCode(codeException);
      DBG_( Verb, ( << theName << " Trap raised by " << *theROB.front() ));
      stop_retire = true;
    } else {
      theROB.front()->doRetirementEffects();
      if (thePendingTrap != 0) {
        theROB.front()->changeInstCode(codeException);
        DBG_( Verb, ( << theName << " Trap raised by " << *theROB.front() ));
        stop_retire = true;
      }
    }

    if (theValidateMMU) {
      // save MMU with this instruction so we can check at commit
      theROB.front()->setMMU( Flexus::Qemu::Processor::getProcessor( theNode )->getMMU() );
    }

    accountRetire( theROB.front() );

    if ( theSquashRequested && (theROB.begin() == theSquashInstruction) ) {
      if (!theSquashInclusive) {
        ++theSquashInstruction;
        theSquashInclusive = true;
      }
      stop_retire = true;
    }

    ++theRetireCount;
    if (theRetireCount >= theRetireWidth) {
      stop_retire = true;
    }

    //Value prediction enabled after forward progress is made
    theValuePredictInhibit = false;

    thePC = theROB.front()->npc();
    //DBG_(Dev, ( << theName << " PC: " << std::hex << thePC << std::dec ) );

    theSRB.push_back(theROB.front());
    if ( thePendingTrap == 0) {
      theROB.pop_front(); //Need to squash and retire instructions that cause traps
    }
  }
}

void CoreImpl::doAbortSpeculation() {
  DBG_Assert( theIsSpeculating );

  //Need to treat ROB head as if it retired to account for stall cycles
  //properly
  if (! theROB.empty() ) {
    accountRetire( theROB.front() );
  }

  //Ensure that the violating instruction is in the SRB.
  DBG_Assert(! theSRB.empty());
  rob_t::index<by_insn>::type::iterator iter = theSRB.get<by_insn>().find( theViolatingInstruction );
  if (iter == theSRB.get<by_insn>().end() ) {
    DBG_Assert( false, ( << " Violating instruction is not in SRB: " << *theViolatingInstruction ) );
  }

  //Locate the instruction that caused the violation and the nearest
  //preceding checkpoint
  rob_t::iterator srb_iter = theSRB.project<0>(iter);
  rob_t::iterator srb_begin = theSRB.begin();
  rob_t::iterator srb_ckpt = srb_begin;
  int32_t saved_discard_count = 0;
  int32_t ckpt_discard_count = 0;
  int32_t remaining_checkpoints = 0;
  bool checkpoint_found = false;
  bool done  = false;
  do {
    if (! checkpoint_found) {
      if ((*srb_iter)->hasCheckpoint()) {
        srb_ckpt = srb_iter;
        checkpoint_found = true;
      } else {
        ++ckpt_discard_count;
      }
    } else {
      ++saved_discard_count;
      if ((*srb_iter)->hasCheckpoint()) {
        ++remaining_checkpoints;
      }
    }
    if (srb_iter == srb_begin) {
      done = true;
    } else {
      --srb_iter;
    }
  } while (! done);

  //Account for location of checkpoint, track discarded instructions
  int32_t required = (theSRB.size() - saved_discard_count - ckpt_discard_count);
  DBG_Assert( required >= 0);
  theSavedDiscards += saved_discard_count;
  theSavedDiscards_Histogram << saved_discard_count;
  theRequiredDiscards += required;
  theRequiredDiscards_Histogram << required;
  theNearestCkptDiscards += ckpt_discard_count;
  theNearestCkptDiscards_Histogram << ckpt_discard_count;

  //Determine which checkpoint we rolled back to.  1 is the eldest,
  //2 is the next, and so on
  theRollbackToCheckpoint << std::make_pair( remaining_checkpoints + 1LL, 1LL);

  //Determine how many checkpoints were crossed.  Minimum is 1 (rolling back
  //to the most recently created checkpoint.
  int32_t num_ckpts_discarded = theCheckpoints.size() - remaining_checkpoints;
  theCheckpointsDiscarded << std::make_pair( static_cast<int64_t>(num_ckpts_discarded), 1LL);

  //Find the checkpoint corresponding to srb_ckpt
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ckpt;
  ckpt = theCheckpoints.find( *srb_ckpt );
  int64_t ckpt_seq_num = (*srb_ckpt)->sequenceNo();
  DBG_Assert( ckpt != theCheckpoints.end() );

  DBG_(Dev, ( << theName << " Aborting Speculation.  Violating Instruction: " <<  *theViolatingInstruction << " Roll back to: " << **srb_ckpt << " Required: " << required << " Ckpt: " << ckpt_discard_count << " Saved: " << saved_discard_count ));

  //Wipe uArch and LSQ
  resetv9();

  cleanMSHRS( ckpt_seq_num );

  //Restore checkpoint
  restorev9State( ckpt->second.theState);

  // restore MMU
  Flexus::Qemu::Processor::getProcessor( theNode )->rollbackMMUCkpts(num_ckpts_discarded - 1);

  //Clean up SRB and SSB.
  theSRB.erase( srb_ckpt, theSRB.end());
  srb_ckpt = theSRB.end(); //srb_ckpt is no longer valid;
  int32_t remaining_ssb = clearSSB( ckpt_seq_num );

  //redirect fetch
  squash_fn(kFailedSpec);
  theRedirectRequested = true;
  theRedirectPC = VirtualMemoryAddress(ckpt->second.theState.thePC);
  theRedirectNPC = VirtualMemoryAddress(ckpt->second.theState.theNPC);

  //Clean up SLAT
  SpeculativeLoadAddressTracker::iterator slat_iter = theSLAT.begin();
  SpeculativeLoadAddressTracker::iterator slat_item;
  SpeculativeLoadAddressTracker::iterator slat_end = theSLAT.end();
  while (slat_iter != slat_end) {
    slat_item = slat_iter;
    ++slat_iter;
    if (slat_item->second->sequenceNo() >= ckpt_seq_num) {
      theSLAT.erase(slat_item);
    }
  }

  //Clean up checkpoints
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ck_iter = theCheckpoints.begin();
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ck_item;
  std::map< boost::intrusive_ptr< Instruction >, Checkpoint>::iterator ck_end = theCheckpoints.end();
  theOpenCheckpoint = 0;
  while (ck_iter != ck_end) {
    ck_item = ck_iter;
    ++ck_iter;
    if (ck_item->first->sequenceNo() >= ckpt_seq_num) {
      theCheckpoints.erase(ck_item);
    } else {
      if (! theOpenCheckpoint || theOpenCheckpoint->sequenceNo() < ck_item->first->sequenceNo() ) {
        theOpenCheckpoint = ck_item->first;
      }
    }
  }
  theRetiresSinceCheckpoint = 0;

  //Stop speculating
  theAbortSpeculation = false;

  theEmptyROBCause = kFailedSpeculation;
  theIsSpeculating = ! theCheckpoints.empty();
  accountAbortSpeculation(ckpt_seq_num);

  //Charge stall cycles for TSOB replay
  if (remaining_ssb > 0) {
    theTSOBReplayStalls = remaining_ssb / theNumMemoryPorts;
  }

}

void CoreImpl::commit() {
  FLEXUS_PROFILE();

  if (theAbortSpeculation) {
    doAbortSpeculation();
  }

  while (! theSRB.empty() && (theSRB.front()->mayCommit() || theSRB.front()->isSquashed()) ) {

    if (theSRB.front()->hasCheckpoint()) {
      freeCheckpoint( theSRB.front() );
    }

    DBG_( Iface, ( << theName << " FinalCommit:" << *theSRB.front() << " NPC=" << theSRB.front()->npc()) );
    if (! theSRB.front()->willRaise()) {
      theSRB.front()->doCommitEffects();
      DBG_( VVerb, ( <<  theName << " commit effects complete" ) );
    }

    commit( theSRB.front() );
    DBG_( VVerb, ( <<  theName << " committed in Simics" ) );

    if (theValidateMMU) {
      DBG_Assert(theSRB.front()->getMMU(), ( << theName << " instruction does not have MMU: " << *theSRB.front()));
      DBG_(Verb, ( << theName << " comparing MMU state after: " << *theSRB.front()));
      Flexus::Qemu::MMU::mmu_t mmu = *(theSRB.front()->getMMU());
      if (! Flexus::Qemu::Processor::getProcessor( theNode )->validateMMU(&mmu)) {
        DBG_(Crit, ( << theName << " MMU mismatch after FinalCommit of: " << *theSRB.front()));
        Flexus::Qemu::Processor::getProcessor( theNode )->dumpMMU(&mmu);
      }
    }

    theSRB.pop_front();
  }
}

void CoreImpl::commit( boost::intrusive_ptr< Instruction > anInstruction ) {
  FLEXUS_PROFILE();

  //Detect kernel-panic
  if (  ( anInstruction->pc() == 0x1000d31c )
        && ( anInstruction->instClass() == clsBranch )
     ) {
    ++theKernelPanicCount;
    if ( theKernelPanicCount == 100) {
      DBG_Assert(false, ( << theName << " Appears to be in the kernel panic loop at v:0x1000d31c" ) );
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  if (theBBVTracker) {
    Flexus::Qemu::Translation xlat;
    xlat.theVaddr = anInstruction->pc();
    xlat.theASI = 0x80;
    xlat.theTL = getTL();
    xlat.thePSTATE = getPSTATE();
    xlat.theType = Flexus::Qemu::Translation::eFetch;
    translate(xlat, false);
    theBBVTracker->commitInsn( xlat.thePaddr, anInstruction->instClass() == clsBranch );
  }
  /* CMU-ONLY-BLOCK-END */

  bool validation_passed = true;
  int32_t raised = 0;
  bool resync_accounted = false;
  if (anInstruction->advancesSimics()) {
    validation_passed &= anInstruction->preValidate();
    DBG_( Verb, Condition(!validation_passed) ( << *anInstruction << "Prevalidation failure." ) );
    bool take_interrupt = theInterruptSignalled && (anInstruction == theInterruptInstruction);
    theInterruptSignalled = false;
    theInterruptInstruction = 0;
    raised = advance_fn(take_interrupt);
    if ( raised != 0) {
      if ( anInstruction->willRaise() != raised) {
        DBG_( Dev, ( << *anInstruction << " Core did not predict correct exception for this instruction raised=0x" << std::hex << raised << " will_raise=0x" << anInstruction->willRaise() << std::dec ) );
        if (anInstruction->instCode() != codeITLBMiss) {
          anInstruction->changeInstCode(codeExceptionUnsupported);
        }
        anInstruction->forceResync();
        resync_accounted = true;
        if (raised < 0x400) {
          ++theResync_UnexpectedException;
        } else {
          ++theResync_Interrupt;
        }
      } else {
        DBG_( Verb, ( << *anInstruction << " Core correctly identified raise=0x" << std::hex << raised << std::dec) );
      }
      anInstruction->raise(raised);
    } else if (anInstruction->willRaise()) {
      DBG_(Crit, ( << *anInstruction << " DANGER:  Core predicted exception: " << std::hex << anInstruction->willRaise() << " but simics says no exception"));
      //DBG_Assert(false, (<< *anInstruction << " Core predicted exception: " << std::hex << anInstruction->willRaise() << " but simics says no exception"));
    }
  }

  accountCommit(anInstruction, raised);

  if (anInstruction->resync()) {
    DBG_( Dev, ( << "Forced Resync:" << *anInstruction ) );
    //Subsequent Empty ROB stalls (until next dispatch) are the result of a
    //synchronizing instruction.
    theEmptyROBCause = kSync;
    if (! resync_accounted) {
      accountResyncReason(anInstruction);
    }
    throw ResynchronizeWithSimicsException(true);
  }
  validation_passed &= anInstruction->postValidate();
  if (! validation_passed ) {
    DBG_( Dev, ( << "Failed Validation\n" << std::internal << *anInstruction << std::left ) );
    //Subsequent Empty ROB stalls (until next dispatch) are the result of a
    //modelling error resynchronization instruction.
    theEmptyROBCause = kResync;
    ++theResync_FailedValidation;
    throw ResynchronizeWithSimicsException();
  }

}

bool CoreImpl::squashAfter( boost::intrusive_ptr< Instruction > anInsn) {
  if ( !theSquashRequested || (anInsn->sequenceNo() <= (*theSquashInstruction)->sequenceNo()) ) {
    theSquashRequested = true;
    theSquashReason = kBranchMispredict;
    theEmptyROBCause = kMispredict;
    theSquashInstruction = theROB.project<0>( theROB.get<by_insn>().find(anInsn) );
    theSquashInclusive = false;
    return true;
  }
  return false;
}

void CoreImpl::redirectFetch( VirtualMemoryAddress anAddress ) {
  theRedirectRequested = true;
  theRedirectPC = anAddress;
  theRedirectNPC = anAddress + 4;
}

void CoreImpl::branchFeedback( boost::intrusive_ptr<BranchFeedback> feedback ) {
  theBranchFeedback.push_back( feedback );
}

void CoreImpl::doSquash() {
  FLEXUS_PROFILE();
  if (theSquashInstruction != theROB.end()) {

    //There is at least one instruction in the ROB
    rob_t::iterator erase_iter = theSquashInstruction;
    if (!theSquashInclusive) {
      ++erase_iter;
    }

    {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() squash");
      rob_t::reverse_iterator iter = theROB.rbegin();
      rob_t::reverse_iterator end = boost::make_reverse_iterator( erase_iter );
      while ( iter != end) {
        (*iter)->squash();
        ++iter;
      }
    }

    if ( ! thePreserveInteractions ) {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() clean-interactions");
      //Discard any defferred interactions
      theDispatchInteractions.clear();
    }

    {
      FLEXUS_PROFILE_N("CoreImpl::doSquash() ROB-clear");
      theROB.erase( erase_iter, theROB.end());
    }
  }
}

void CoreImpl::startSpeculating() {
  if ( ! isSpeculating() ) {
    DBG_( Iface, ( << theName << " Starting Speculation" ) );
    theIsSpeculating = true;
    theAbortSpeculation = false;
    accountStartSpeculation();
  }
}

void CoreImpl::checkStopSpeculating() {
  if (      isSpeculating()
            && ! theAbortSpeculation
            &&   theCheckpoints.size() == 0
     ) {
    DBG_( Iface, ( << theName << " Ending Successful Speculation" ) );
    theIsSpeculating = false;
    accountEndSpeculation();
  } else {
    DBG_( Iface, ( << theName << " continuing speculation checkpoints: " << theCheckpoints.size() << " Abort pending? " <<  theAbortSpeculation  ) );
  }
}

void CoreImpl::takeTrap(boost::intrusive_ptr<Instruction> anInstruction, int32_t aTrapNum) {
  DBG_( Verb,  ( << theName << " Take trap: " << std::hex << aTrapNum << std::dec ) );

  //Ensure ROB is not empty
  DBG_Assert ( ! theROB.empty() );
  //Only ROB head should raise
  DBG_Assert ( anInstruction == theROB.front() );

  //Clear ROB
  theSquashRequested = true;
  theSquashReason = kException;
  theEmptyROBCause = kRaisedException;
  theSquashInstruction = theROB.begin();
  theSquashInclusive = true;

  //Record the pending trap
  thePendingTrap = aTrapNum;
  theTrapInstruction = anInstruction;

  anInstruction->setWillRaise( aTrapNum );
  anInstruction->raise(aTrapNum);
}

void CoreImpl::popTL() {
  thePopTLRequested = true;
}

void CoreImpl::handlePopTL() {
  if ( ! thePopTLRequested) {
    return;
  }
  thePopTLRequested = false;
  int32_t tl = getTL();
  if (tl <= 0 || tl > 5) {
    return;
  }
  DBG_( Verb,  ( << theName << " Pop trap. TL=" << tl) );
  uint64_t tstate = getTSTATE(tl);
  uint64_t cwp = tstate & 0x7;
  uint64_t asi = ( tstate >> 24) & 0xFF;
  uint64_t ccr = ( tstate >> 32) & 0xFF;
  uint64_t pstate = ( tstate >> 8) & 0xFFF;

  setTL( tl - 1);
  DBG_( Verb,  ( << theName << "    TL: " << std::hex << getTL() ) );

  setPSTATE(pstate);
  //setAG(pstate & 1);
  //setMG(pstate & 0x400ULL);
  //setIG(pstate & 0x800ULL);
  DBG_( Verb,  ( << theName << "    PSTATE: " << std::hex << getPSTATE() ) );

  setASI(asi);
  DBG_( Verb,  ( << theName << "    ASI: " << std::hex << getASI() ) );

  setCWP(cwp);
  DBG_( Verb,  ( << theName << "    CWP: " << std::hex << getCWP() ) );

  setCCR(ccr);
  DBG_( Verb,  ( << theName << "    CCR: " << std::hex << getCCR() ) );
}

bool CoreImpl::acceptInterrupt() {
  if (    (thePendingInterrupt > 0)   //Interrupt is pending
          && ! theInterruptSignalled       //Already accepted the interrupt
          && ! theROB.empty()              //Need an instruction to take the interrupt on
          && ! theROB.front()->isAnnulled()//Do not take interrupts on annulled instructions
          && ! theROB.front()->isSquashed()//Do not take interrupts on squashed instructions
          && ! theROB.front()->isMicroOp() //Do not take interrupts on squashed micro-ops
          && ! theIsSpeculating            //Do not take interrupts while speculating
          &&   (getPSTATE() & 0x2)         //PSTATE.IE = 1
     ) {
    //Interrupt was signalled this cycle.  Clear the ROB
    theInterruptSignalled = true;

    DBG_(Verb, ( << theName << " Accepting interrupt " << std::hex << thePendingInterrupt << std::dec << " on instruction " << *theROB.front()) );
    theInterruptInstruction = theROB.front();
    takeTrap( theInterruptInstruction, thePendingInterrupt );

    return true;
  }
  return false;
}

void CoreImpl::handleTrap() {
  if (thePendingTrap == 0) {
    return;
  }
  DBG_( Verb,  ( << theName << " Handling trap: " << std::hex << thePendingTrap << std::dec << " raised by: " << *theTrapInstruction ) );
  if (! theROB.empty()) {
    DBG_( Crit,  ( << theName << " ROB non-empty in handle trap.  Resynchronize instead.") );
    theEmptyROBCause = kResync;
    ++theResync_FailedHandleTrap;
    throw ResynchronizeWithSimicsException();
  }

  //Increment trap level
  DBG_Assert( getTL() + 1 < 5, ( << theName << " Trap when TL == 5.  Processor enters RED_state.  Unsupported." ) );
  setTL( getTL() + 1);
  DBG_( Verb,  ( << theName << "    TL: " << std::hex << getTL() ) );

  //Save state
  uint64_t ccr = getCCR();
  DBG_( Verb,  ( << theName << "    ccr: " << std::hex << ccr ) );
  uint64_t asi = getASI();
  uint64_t pstate = getPSTATE();
  uint64_t cwp = getCWP();
  uint64_t tstate = (ccr << 32) | (asi << 24) | (pstate << 8) | cwp;
  setTSTATE( tstate, getTL());
  DBG_( Verb,  ( << theName << "    TSTATE: " << std::hex << getTSTATE(getTL()) ) );
  setTPC( ( pstate & 8/*AM*/ ?  (theTrapInstruction->pc() & 0xFFFFFFFFULL) : theTrapInstruction->pc()) , getTL());
  DBG_( Verb,  ( << theName << "    TPC: " << std::hex << getTPC(getTL()) ) );
  setTNPC( ( pstate & 8/*AM*/ ?  (theTrapInstruction->npc() & 0xFFFFFFFFULL) : theTrapInstruction->npc()), getTL());
  DBG_( Verb,  ( << theName << "    TNPC: " << std::hex << getTNPC(getTL()) ) );

  //Record Trap Type
  setTT(thePendingTrap, getTL());
  DBG_( Verb,  ( << theName << "    TT: " << std::hex << getTT(getTL()) ) );

  //Update PSTATE
  //Mask off PSTATE bits that are always turned off
  //PSTATE.RED = 0
  //PSTATE.AM = 0 (address masking is turned off)
  //PSTATE.IE = 0 (interrupts are disabled)
  pstate &= ~0x2A;

  //Turn on PSTATE bits that are always on
  //PSTATE.PEF = 1 (FPU is present)
  //PSTATE.PRIV = 1 (the processor enters privileged mode)
  pstate |= 0x14;

  //PSTATE.CLE = PSTATE.TLE (set endian mode for traps)
  pstate |= ((pstate << 1) & 0x200ULL);

  //if (TT[TL] one of ( fast_instruction_access_MMU_miss , fast_data_access_MMU_miss, and fast_data_access_protection - 0x64 to 0x6F )
  pstate &= ~0xC01; //Shut off all register sets
  if (thePendingTrap >= 0x64 && thePendingTrap <= 0x6F) {
    //PSTATE.AG = 0 (global registers are replaced with alternate globals)
    pstate |= 0x400ULL;
  } else if ( thePendingTrap == 0x60 ) {
    //else if TT[TL] is (interrupt_vector 0x60 )
    pstate |= 0x800ULL;
  } else {
    pstate |= 1;
  }
  //PSTATE.MM is unchanged
  //PSTATE.TLE is unchanged
  setPSTATE(pstate);
  DBG_( Verb,  ( << theName << "    PSTATE: " << std::hex << getPSTATE() ) );

  //Register window trap processing
  DBG_( Verb,  ( << theName << "    Previous CWP: " << std::hex << getCWP() ) );
  if (thePendingTrap == 0x24) {
    //If TT[TL] = 0x24 //(a clean_window trap),
    //CWP = CWP + 1.
    theWindowMap.incrementCWP();
    theArchitecturalWindowMap.incrementCWP();
  } else if ( thePendingTrap >= 0x80 && thePendingTrap <= 0xBF) {
    //If 0x80 <= TT[TL] <= 0xBF //(window spill trap)
    //CWP = CWP + CANSAVE + 2.
    setCWP( (getCWP() + getCANSAVE() + 2) & 0x7 );
  } else if ( thePendingTrap >= 0xC0 && thePendingTrap <= 0xFF) {
    theWindowMap.decrementCWP();
    theArchitecturalWindowMap.decrementCWP();
  }
  DBG_( Verb,  ( << theName << "    CWP: " << std::hex << getCWP() ) );

  //Redirect Execution
  //PC  = TBA<63:15> | (TL > 0) | TT[TL] | 0 0000
  //nPC = TBA<63:15> | (TL > 0) | TT[TL] | 0 0100
  uint64_t vector_address = getTBA() & 0xFFFFC000ULL;
  vector_address |= ( getTL() > 1 ? 0x4000 : 0) | ( (thePendingTrap ) << 5);
  redirectFetch( VirtualMemoryAddress(vector_address) );
  DBG_( Verb,  ( << theName << "    new PC: " << VirtualMemoryAddress(vector_address) ) );

  thePendingTrap = 0;
  theTrapInstruction = 0;
}

void CoreImpl::valuePredictAtomic() {
  FLEXUS_PROFILE();
  if (theSpeculateOnAtomicValue && theLSQCount > 0) {
    memq_t::index<by_queue>::type::iterator lsq_head = theMemQueue.get<by_queue>().lower_bound( std::make_tuple( kLSQ ) );
    if (     (   lsq_head->isAtomic() )
             &&   ( ! lsq_head->isAbnormalAccess() )
             &&   ( ! lsq_head->theExtendedValue )
             &&   ( ! lsq_head->thePartialSnoop )
             &&   (   (lsq_head->theOperation == kCAS) ? (!! lsq_head->theCompareValue) : true )
             &&   (   lsq_head->theInstruction == theROB.front() )
             &&   ( ! theValuePredictInhibit )  //Disables value prediction for one instruction after a rollback
             &&   ( lsq_head->status() == kIssuedToMemory )
       ) {
      DBG_(Iface, ( << theName  << " Value-predicting " << *lsq_head ) );
      ++theValuePredictions;

      if (theSpeculateOnAtomicValuePerfect) {
	lsq_head->theExtendedValue = ValueTracker::valueTracker(Flexus::Qemu::ProcessorMapper::mapFlexusIndex2VM(theNode)).load( theNode, lsq_head->thePaddr, lsq_head->theSize);
      } else {
        if (lsq_head->theOperation == kCAS) {
          lsq_head->theExtendedValue = lsq_head->theCompareValue;
        } else {
          lsq_head->theExtendedValue = 0;
        }
      }
      if (lsq_head->theDependance) {
        lsq_head->theDependance->satisfy();
      }
      lsq_head->theSpeculatedValue = true;
    }
  }
}

bool CoreImpl::isIdleLoop() {
  return false;		//ALEX - we don't currently have WhiteBox in QEMU
  /*uint64_t idle = nWhiteBox::WhiteBox::getWhiteBox()->get_idle_thread_t(theNode);
  unmapped_reg reg;
  reg.theType = rRegisters;
  reg.theIndex = 7;  // %g7 holds current thread
  uint64_t ct = boost::get<uint64_t>(readArchitecturalRegister(reg, false));

  DBG_(Verb, ( << theName << " isIdleLoop: idle=0x" << std::hex << idle << "== ct=0x" << ct << " : " << (ct == idle)));

  return ((idle != 0) && (ct == idle));*/
}

uint64_t CoreImpl::pc() const {
  if (! theROB.empty()) {
    if (theROB.front()->isAnnulled() || theROB.front()->isMicroOp() ) {
      return theROB.front()->npc();
    } else {
      return theROB.front()->pc();
    }
  } else {
    return thePC;
  }
}

uint64_t CoreImpl::npc() const {
  if (! theROB.empty()) {
    if (theROB.front()->isAnnulled() || theROB.front()->isMicroOp() ) {
      DBG_(Verb, ( << "NPC= front->npc + 4" ) );
      return theROB.front()->npc() + 4;
    } else {
      DBG_(Verb, ( << "NPC= front->npcReg" ) );
      return theROB.front()->npcReg();
    }
  } else if (theNPC) {
    DBG_(Verb, ( << "NPC= *NPC" ) );
    return *theNPC;
  } else {
    if (!theDispatchInteractions.empty()) {
      std::list< boost::intrusive_ptr< Interaction > >::const_iterator iter, end;
      iter = theDispatchInteractions.begin();
      end = theDispatchInteractions.end();
      uint64_t npc = 0;
      bool found = false;
      while (iter != end)  {
        DBG_(Verb, ( << "interaction: " << **iter) );
        if ((*iter)->npc()) {
          npc = *((*iter)->npc());
          found = true;
        } else if ((*iter)->annuls()) {
          found = false;
          break;
        }
        ++iter;
      }
      if (found) {
        DBG_(Verb, ( << "NPC= from interaction " ) );
        return npc;
      }
    }
  }
  DBG_(Verb, ( << "NPC= PC + 4" ) );
  return thePC + 4;
}

} //nuArch
