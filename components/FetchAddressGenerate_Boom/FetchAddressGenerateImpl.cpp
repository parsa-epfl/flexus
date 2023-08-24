//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#include <components/FetchAddressGenerate_Boom/FetchAddressGenerate.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FetchAddressGenerate
#define DBG_SetDefaultOps AddCat(FetchAddressGenerate)
#include DBG_Control

#include <core/flexus.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

#include <components/CommonQEMU/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>
#include <components/CommonQEMU/PerfectBranchDecode.hpp>

#define PerfectBranchDetection

namespace nFetchAddressGenerate {

using namespace Flexus;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(FetchAddressGenerate) {
  FLEXUS_COMPONENT_IMPL(FetchAddressGenerate);

  std::vector<MemoryAddress> thePC;
  // std::vector<MemoryAddress> theNextPC;
  std::vector<MemoryAddress> theRedirectPC;
  // std::vector<MemoryAddress> theRedirectNextPC;
  std::vector<bool> theRedirect;
  boost::scoped_ptr<BranchPredictor> theBranchPredictor;
  uint32_t theCurrentThread;
  std::vector<uint64_t> theNextSerial;
  boost::intrusive_ptr<BPredState> squashedBPState; // Rakesh
  std::vector<bool> isRedirect;
  std::vector<bool> RASreconstructed;
  std::vector<bool> theBTBMiss;
  std::vector<bool> theLateBTBMiss;
  std::vector<int> thePredecodeCyclesLeft;
  std::vector<bool> lastTranslationFailed;
  std::vector<VirtualMemoryAddress> theBTBFetchedBlock;
  std::list<VirtualMemoryAddress> theBTBPredecBuff;
  std::vector<eSquashCause> squashReason;
  std::map<VirtualMemoryAddress, VirtualMemoryAddress> missMap;
  Stat::StatCounter theNextReturnAddr;
  Stat::StatCounter theBTBMissOnCall;
  Stat::StatCounter theBTBMisses;
  Stat::StatCounter theBTBHits;
  Stat::StatCounter theBTBFillsL1;
  Stat::StatCounter theBTBFillsL2;

  Stat::StatCounter theCondPredicts;
  Stat::StatCounter theTageOverrides;

  VirtualMemoryAddress BTBMissPrefetchAddr;
  int32_t BTBMissPrefetchesIssued;

  int32_t theBTBPredecBuffSize;
  int64_t theBlockMask;
  uint32_t ICacheLineSize;
  bool BTBPrefilled;
  bool missUnderBTBMiss;
  std::list<VirtualMemoryAddress> BTBMissFetchBlocks;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS),
        theNextReturnAddr(statName() + "-NextCorrectReturnAddr"),
        theBTBMissOnCall(statName() + "-BTBMissOnCall"), theBTBMisses(statName() + "-BTBMisses"),
        theBTBHits(statName() + "-BTBHits"), theBTBFillsL1(statName() + "-BTBFillsL1"),
        theBTBFillsL2(statName() + "-BTBFillsL2"), theCondPredicts(statName() + "-CondPredicts"),
        theTageOverrides(statName() + "-TageOverrides") {
  }

  void initialize() {
    thePC.resize(cfg.Threads);
    // theNextPC.resize(cfg.Threads);
    theRedirectPC.resize(cfg.Threads);
    // theRedirectNextPC.resize(cfg.Threads);
    theRedirect.resize(cfg.Threads);
    theNextSerial.resize(cfg.Threads);
    isRedirect.resize(cfg.Threads);
    RASreconstructed.resize(cfg.Threads);
    theBTBMiss.resize(cfg.Threads);
    lastTranslationFailed.resize(cfg.Threads);
    thePredecodeCyclesLeft.resize(cfg.Threads);
    theBTBFetchedBlock.resize(cfg.Threads);
    squashReason.resize(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + i);
      thePC[i] = cpu->getPC();
      // theNextPC[i] = thePC[i] + 4;
      theRedirectPC[i] = MemoryAddress(0);
      // theRedirectNextPC[i] = MemoryAddress(0);
      theRedirect[i] = false;
      isRedirect[i] = false;
      RASreconstructed[i] = false;
      theBTBMiss[i] = false;
      lastTranslationFailed[i] = false;
      thePredecodeCyclesLeft[i] = 0;
      squashReason[i] = kResynchronize; // initialization with random value
      theBTBFetchedBlock[i] = MemoryAddress(0);
      // DBG_( Dev, Comp(*this) ( << "Thread[" << flexusIndex() << "." << i << "] connected to " <<
      // ((static_cast<Flexus::Qemu::API::conf_object_t *>(cpu))->name ) << " Initial PC: " <<
      // thePC[i] ) );
    }
    theCurrentThread = cfg.Threads;
    theBranchPredictor.reset(BranchPredictor::combining(statName(), flexusIndex(), cfg.EnableRAS,
                                                        cfg.EnableTCE, cfg.EnableTrapRet,
                                                        cfg.BTBSets, cfg.BTBWays));

    squashedBPState = 0;
    BTBMissPrefetchesIssued = 0;

    BTBPrefilled = false;
    missUnderBTBMiss = false;

    theBTBPredecBuffSize = 8; // Fixme: Should be a parameter.
    theBTBPredecBuff.resize(theBTBPredecBuffSize);

    ICacheLineSize = 64; // Bytes. Fixme: make it parameter consistent with other places where block
                         // size is needed
    theBlockMask = ~(ICacheLineSize - 1);
  }

  void finalize() {
  }

  bool isQuiesced() const {
    // the FAG is always quiesced.
    return true;
  }

  void saveState(std::string const &aDirName) {
    theBranchPredictor->saveState(aDirName);
  }

  void loadState(std::string const &aDirName) {
    theBranchPredictor->loadState(aDirName);
  }

public:
  // RedirectIn
  //----------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex,
            Flexus::SharedTypes::VirtualMemoryAddress &aRedirect) {
    theRedirectPC[anIndex] = aRedirect;
    theRedirect[anIndex] = true;
    isRedirect[anIndex] = true;
    //    DBG_(Tmp, Comp(*this)( << std::endl<< std::endl<< std::endl << "redirecting pc to " <<
    //    std::hex << aRedirect.first << " and " << aRedirect.second));
    squashedBPState = 0;
  }

  // MissPairIn
  //----------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(MissPairIn);
  void push(interface::MissPairIn const &, index_t anIndex,
            std::pair<MemoryAddress, MemoryAddress> &aMissPair) {
    std::map<MemoryAddress, MemoryAddress>::iterator it = missMap.find(aMissPair.first);
    if (it != missMap.end()) {
      //		  DBG_(Tmp, ( << "Updating miss pair pc " << aMissPair.first << " to missed
      // block " << aMissPair.second));
      it->second = aMissPair.second;
    } else {
      //		  DBG_(Tmp, ( << "Inserting miss pair pc " << aMissPair.first << " missed
      // block " << aMissPair.second));
      missMap.insert(aMissPair);
    }
  }

  // BTBReplyIn
  //----------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BTBReplyIn);
  void push(interface::BTBReplyIn const &, index_t anIndex, bool &isMissed) {
    if (isMissed) {
      // DBG_(Iface, ( << "L1 Miss "));
      theBTBFillsL2++;
    } else {
      // DBG_(Iface, ( << "L1 Hit "));
      theBTBFillsL1++;
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BTBMissFetchReplyIn);
  void push(interface::BTBMissFetchReplyIn const &, index_t anIndex, bool &isHit) {
    if(BTBMissFetchBlocks.empty()){
      theBTBMiss[anIndex] = false;
      //std::cout << "ReplyIn, BTBMissFetchBlocks empty, clock: " << theFlexus->cycleCount() << "\n";
    }
    else{
      //std::cout << "ReplyIn, FAG BTB miss request for: " << (BTBMissFetchBlocks.front() >> 6) << ", clock: " << theFlexus->cycleCount() << "\n";
      FLEXUS_CHANNEL_ARRAY(BTBRequestOut, anIndex) << BTBMissFetchBlocks.front();
      BTBMissFetchBlocks.pop_front();
    }
  }

  // SquashIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashIn);
  void push(interface::SquashIn const &, index_t anIndex, eSquashCause &aReason) {
    DBG_Assert(FLEXUS_CHANNEL(uArchHalted).available());
    bool cpu_in_halt = false;
    FLEXUS_CHANNEL(uArchHalted) >> cpu_in_halt;
    if (cpu_in_halt) {
      return;
    }

    DBG_(Iface, Comp(*this)(<< std::endl
                                     << std::endl
                                     << std::endl
                                     << "Squash cpu " << flexusIndex()
                                     << "Resetting branch history reason " << aReason << std::endl
                                     << std::endl
                                     << std::endl));

    squashReason[anIndex] = aReason;
    if (!squashedBPState) {
      DBG_(Iface, (<< "WARNING! Squash with no BPstate to revert to"));
    } else {
      if (aReason == kBranchMispredict) {
        theBranchPredictor->resetUpdateState(squashedBPState);
      } else {
        theBranchPredictor->resetState(squashedBPState);
      }
    }

    if (RASreconstructed[anIndex] == false) {
      //		  DBG_( Tmp, ( << "Reset RAS "));
      theBranchPredictor->resetRAS();
    }
    RASreconstructed[anIndex] = false;
    theBTBMiss[anIndex] = false;
    thePredecodeCyclesLeft[anIndex] = 0;
    BTBMissPrefetchesIssued = 0;
    BTBPrefilled = false;
    missUnderBTBMiss = false;

    BTBMissFetchBlocks.clear();

    //	    if (aReason == kBranchMispredict && squashedBPState->thePredictedType == kNonBranch &&
    //(squashedBPState->theActualType == kCall || squashedBPState->theActualType == kIndirectCall)) {
    //	    	DBG_Assert( squashedBPState->theActualDirection != kStronglyTaken, ( <<
    //"theActualDirection not updated.  Cannot be kStronglyTaken" ) ); 	    	DBG_( Tmp, ( <<
    //"pushing to RAS " << squashedBPState->pc + 8));
    //	    	theBranchPredictor->pushReturnAddresstoRAS(squashedBPState->pc + 8);
    //	    	squashedBPState->callUpdatedRAS = true;
    //	    	theBTBMissOnCall++;
    //	    }
  }

  // SquashBranchIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SquashBranchIn);
  void push(interface::SquashBranchIn const &, index_t anIndex,
            boost::intrusive_ptr<BPredState> &aBPState) {

    DBG_(Iface,
         (<< std::endl
          << std::endl
          << std::endl
          << "Branch Mispredicted " << aBPState->thePredictedType << " "
          << aBPState->returnPopRASTwice << " serial " << aBPState->theSerial << std::hex << " pc "
          << aBPState->pc << " "
          << Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(aBPState->pc)
          << std::endl
          << std::endl
          << std::endl));
    squashedBPState = aBPState;
  }

  // TrapStateIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(TrapStateIn);
  void push(interface::TrapStateIn const &, index_t anIndex,
            boost::intrusive_ptr<TrapState> &aTrapState) {
    //	  DBG_(Tmp, ( << std::endl<< std::endl << std::endl << "Trap State Received " << std::endl
    //<< std::endl<< std::endl) );
    theBranchPredictor->resetTrapState(aTrapState);
  }

  // RASOpsIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RASOpsIn);
  void push(interface::RASOpsIn const &, index_t anIndex,
            std::list<boost::intrusive_ptr<BPredState>> &theRASops) {
    //	  DBG_( Tmp, ( << " reconstruct RAS " ));
    theBranchPredictor->reconstructRAS(theRASops);
    RASreconstructed[anIndex] = true;
    //	  squashedBPState = aBPState;
    //	  DBG_( Tmp, ( << " Instance More After " << aBPState->theRefCount));
  }

  // SpecialCallIn
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(SpecialCallIn);
  void push(interface::SpecialCallIn const &, index_t anIndex,
            boost::intrusive_ptr<BPredState> &aBPState) {
    assert(0);
  }

  // BranchFeedbackIn
  //----------------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BranchFeedbackIn);
  void push(interface::BranchFeedbackIn const &, index_t anIndex,
            boost::intrusive_ptr<BranchFeedback> &aFeedback) {
    //	  DBG_(Tmp, ( << std::endl<< std::endl<< std::endl<< "Sending branch feedback " << std::endl
    //<< std::endl<< std::endl) );
    theBranchPredictor->feedback(*aFeedback, flexusIndex());
  }

  // Drive Interfaces
  //----------------
  // The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
  // passing each instruction to its FetchOut port.
  void drive(interface::FAGDrive const &) {
    int32_t td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFAGThread(flexusIndex());
    }
    doAddressGen(td);
  }

private:
  VirtualMemoryAddress issueWrongPathPrefetch(VirtualMemoryAddress target, FetchAddr faddr) {
    VirtualMemoryAddress prefetechBlock;
    VirtualMemoryAddress targetBlock;
    VirtualMemoryAddress currentBlock = VirtualMemoryAddress(faddr.theAddress & theBlockMask);
    if (faddr.theBPState->thePrediction <= kTaken && target != 0) {
      targetBlock = VirtualMemoryAddress(target & theBlockMask);
      prefetechBlock = currentBlock + 64;
    } else if (faddr.theBPState->theNextPredictedTarget != 0) {
      targetBlock = VirtualMemoryAddress(faddr.theBPState->theNextPredictedTarget & theBlockMask);
      prefetechBlock = targetBlock;
    } else {
      targetBlock = currentBlock;
    }

    //		DBG_(Tmp, ( << "currBlock " << std::hex << currentBlock << " target " << targetBlock
    //<< " prefetch " << prefetechBlock << " decision " << faddr.theBPState->thePrediction << " addr
    //"
    // << target));
    if (targetBlock != currentBlock && targetBlock != currentBlock + 64) {
      return prefetechBlock;
    } else {
      return VirtualMemoryAddress(0);
    }
  }

#ifdef PerfectBranchDetection
  Flexus::Core::index_t getSystemWidth() {
    return 1;
    // TODO
    // int cpu_count = 0;
    // int client_cpu_count = 0;
    // added by PLotfi to support SPECweb2009 workloads
    // in SPECweb2009 in addition to client and server machines, a third group of machines exists to
    // represent backend. The name of these mahines starts with "besim"
    // int besim_cpu_count = 0;
    // end PLotfi
    /* MARK: This was removed because libqflex does not support this API
       Flexus::Qemu::API::conf_object_t * queue = Flexus::Qemu::API::SIM_next_queue(NULL);
       while (queue != NULL) {
       if (std::strstr(queue->name, "client") != 0) {
       ++client_cpu_count;
       } else if (std::strstr(queue->name, "besim") != 0) { // added by PLotfi
       ++besim_cpu_count;
       } else {
       ++cpu_count;
       }
       queue = Flexus::Qemu::API::SIM_next_queue(queue);
       }
       return cpu_count;
       */
  }

  PhysicalMemoryAddress getPhysicalAddress(VirtualMemoryAddress prefetch_addr) {
    PhysicalMemoryAddress paddr =
        Qemu::Processor::getProcessor(flexusIndex())->translateVirtualAddress(prefetch_addr);
    if (!paddr) {
      //			DBG_(Tmp, ( << "sys width " << std::dec << getSystemWidth()));
      for (uint16_t i = 0; i < getSystemWidth(); i++) {
        paddr = Qemu::Processor::getProcessor(i)->translateVirtualAddress(prefetch_addr);
        if (paddr) {
          break;
        }
      }
    }
    return paddr;
  }

  bool trackSpecialCall(VirtualMemoryAddress vpc) {
    PhysicalMemoryAddress paddr = getPhysicalAddress(vpc);
    uint32_t opcode = 0;
    if (paddr) {
      opcode = Flexus::Qemu::Processor::getProcessor(flexusIndex())->fetchInstruction(vpc);
      if (opcode) {
        uint32_t op = opcode >> 30;
        if (op == 2) {
          uint32_t op3 = (opcode >> 19) & 0x3F;
          if (op3 == 0x3D) { // Its a restore instruction
            return true;
          } else if (op3 == 0x2 || op3 == 0x12) {
            uint32_t rd = (opcode >> 25) & 0x1F;
            if (rd == 15) {
              DBG_(Iface,
                   (<< "Decoding a special call " << vpc << " "
                    << Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(vpc)));
              return true;
            }
          }
        }
      }
    }
    return false;
  }

#if 0
        bool preFillBTB (index_t anIndex, VirtualMemoryAddress vpc) {
            if (thePredecodeCyclesLeft[anIndex]) {
                thePredecodeCyclesLeft[anIndex]--;
                return false;
            }
            /*if (theLateBTBMiss[anIndex]) {
              assert (theBTBMissCyclesLeft[anIndex] >= 0 && theBTBMissCyclesLeft[anIndex] < 3);
              if (theBTBMissCyclesLeft[anIndex] == 0) {
              theLateBTBMiss[anIndex] = false;
              theBTBMiss[anIndex] = true;
              return false;
              } else {
              theBTBMissCyclesLeft[anIndex]--;
              }
              return true;
              }*/

            bool BTBHit = true;
            PhysicalMemoryAddress paddr = getPhysicalAddress(vpc);
            int64_t op_code = 0;
            if (paddr) {
                lastTranslationFailed[anIndex] = false;
                op_code = Flexus::Qemu::Processor::getProcessor(flexusIndex())->readPAddr(paddr, 4/*Number of bytes*/);
                if (op_code) {
                    std::pair<eBranchType, VirtualMemoryAddress> aPair = targetDecode(op_code);
                    eBranchType branchType = aPair.first;
                    if (branchType != kNonBranch) {
                        //    			  DBG_(Tmp, ( << "cpu " << flexusIndex() << " Branch detected " << vpc));
                        if (theBranchPredictor->isBranchInsn( vpc ) == false) {
                            theBTBMisses++;
                            //    				  DBG_(Tmp, ( << "cpu " << flexusIndex() << " BTB Miss " << vpc));
                            VirtualMemoryAddress target = VirtualMemoryAddress(0);;
                            if (cfg.MagicBTypeDetect == false || /*branchType == kConditional || branchType == kUnconditional || branchType == kCall*/branchType != kReturn) {
                                bool branchTaken = true;
                                //            			  if (cfg.MagicBTypeDetect && branchType == kConditional) {
                                //        				  don't used it being used for something else.
                                //            				  branchTaken = theBranchPredictor->conditionalTaken(vpc);
                                //            			  }
                                //						  DBG_(Tmp, ( << "BTB Miss " << vpc << " taken " << branchTaken));
                                if (branchTaken) {
                                    if ((vpc & theBlockMask) != theBTBFetchedBlock[anIndex]) {
                                        //Fetch block from Icache
                                        if (! cfg.PerfectBTB) {
                                            FLEXUS_CHANNEL_ARRAY(BTBRequestOut, anIndex) << vpc;
                                            theBTBFetchedBlock[anIndex] = VirtualMemoryAddress(vpc & theBlockMask);
                                            if (theBTBMiss[anIndex]) {
                                                //										  DBG_(Tmp, ( << "L1 Miss "));
                                                theBTBFillsL2++;
                                            } else {
                                                //										  DBG_(Tmp, ( << "L1 Hit "));
                                                theBTBFillsL1++;
                                            }
                                            //									  return false;
                                            BTBHit = false;
                                        }
                                    }
                                }
                                if (branchType == kConditional || branchType == kUnconditional || branchType == kCall) {
                                    target = vpc;
                                    target += aPair.second;
                                }
                            } /*else if (branchType == kIndirectReg || branchType == kIndirectCall) {
                                theBTBMissCyclesLeft[anIndex] = 2;
                                }*/
                            //UPdate BTB

                            bool specialCall = false;
                            if (branchType == kCall || branchType == kIndirectCall) { //If the next instruction restores the register window, it is a special call
                                specialCall = trackSpecialCall(vpc + 4);
                            }
                            //    				  DBG_(Tmp, ( << "Target " << target ));
                            theBranchPredictor->updateBTB(vpc, branchType, target, specialCall, 0 /*Fixme: pass the size of the basic block*/);
                        } else {
                            theBTBHits++;
                        }
                        //    			  DBG_(Tmp, ( << "Enqueued Branch addr " << vpc << " opcode " << std::hex << op_code) );
                    }
                }
            } else {
                lastTranslationFailed[anIndex] = true;
                //    	  DBG_(Tmp, ( << "cpu " << flexusIndex() << " translation failed pc " << vpc ) );
            }
            //      return true;
            return BTBHit;
        }
#endif

  bool isSpecialCall(VirtualMemoryAddress vpc) {
    PhysicalMemoryAddress paddr = getPhysicalAddress(vpc);
    if (paddr) {
      uint32_t opcode = Flexus::Qemu::Processor::getProcessor(flexusIndex())->fetchInstruction(vpc);
      if (opcode) {
        uint32_t op = (opcode >> 30) & 3;
        if (op == 2) {
          uint32_t op3 = (opcode >> 19) & 0x3F;
          if (op3 == 0x2 || op3 == 0x12) { // Move to %o7
            uint32_t rd = (opcode >> 25) & 0x1F;
            if (rd == 15) {
              DBG_(Iface, (<< " Detected move to o7  "));
              return true;
            }
          } else if (op3 == 0x3D) { // Restore
            DBG_(Iface, (<< " Detected restore: "));
            return true;
          }
        }
      }
    }
    return false;
  }

  std::pair<bool, BTBEntry> preFillBBTB(index_t anIndex, VirtualMemoryAddress vpc) {
    //std::cout << "\n\n\npreFillBBTB, clock: " << theFlexus->cycleCount() << "\n";
    BTBEntry aBBTBEntry(vpc, kNonBranch, VirtualMemoryAddress(0), 0, kTaken, false);
    int BBSize = 0;
    eBranchType branchType = kNonBranch;
    lastTranslationFailed[anIndex] = false;
    bool isPredecodeBuffHit = false;

    while (branchType == kNonBranch && (BBSize < 100)) {
      DBG_(Iface, (<< "Predec PC " << vpc));
      PhysicalMemoryAddress paddr = getPhysicalAddress(vpc);
      if (paddr) {
        int64_t op_code =
            Flexus::Qemu::Processor::getProcessor(flexusIndex())->fetchInstruction(vpc);
        if (op_code) {
          BBSize++;
          std::pair<eBranchType, VirtualMemoryAddress> aPair = targetDecode(op_code);
          branchType = aPair.first;
          DBG_(Iface,
               (<< "Type " << branchType << " "
                << Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(vpc)));
          if (branchType != kNonBranch) {
            aBBTBEntry.theBranchType = branchType;
            aBBTBEntry.theBBsize = BBSize;

            if (branchType == kConditional) {
              aBBTBEntry.theBranchDirection = theBranchPredictor->conditionalTaken(vpc);
              DBG_(Iface, (<< "Prediction " << aBBTBEntry.theBranchDirection));
            }

            if (branchType == kConditional || branchType == kUnconditional || branchType == kCall ||
                branchType == kIndirectReg || branchType == kIndirectCall) {
              aBBTBEntry.theTarget = vpc;
              aBBTBEntry.theTarget += aPair.second;
            }

            // Number of cache blocks needed to predecode the requested basic block
            uint64_t firstBlock = aBBTBEntry.thePC & theBlockMask;
            uint64_t lastBlock =
                (aBBTBEntry.thePC + (aBBTBEntry.theBBsize - 1) * 4 /*Inst size in bytes*/) &
                theBlockMask;
            int numCacheBlocks = 1 /*Curret Block*/ + (lastBlock - firstBlock) / ICacheLineSize;
            boost::intrusive_ptr<RecordedMisses> blockAddresses(new RecordedMisses());
            bool predecCached = true;

            //std::cout << "numCacheBlocks: " << numCacheBlocks << "\n";

            DBG_(Iface,
                 (<< "First address " << aBBTBEntry.thePC << " last addr "
                  << aBBTBEntry.thePC + (aBBTBEntry.theBBsize - 1) * 4 << " 1st block " << std::hex
                  << firstBlock << " last bloc " << lastBlock << " num blocks " << numCacheBlocks));

            assert(BTBMissFetchBlocks.empty());

            for (int i = 0; i < numCacheBlocks; i++) {
              uint64_t blockAddr = firstBlock + (i * ICacheLineSize);
              blockAddresses->theMisses.push_back(VirtualMemoryAddress(blockAddr));
              BTBMissFetchBlocks.push_back(VirtualMemoryAddress(blockAddr));

              if (std::find(theBTBPredecBuff.begin(), theBTBPredecBuff.end(),
                            VirtualMemoryAddress(blockAddr)) == theBTBPredecBuff.end()) {
                predecCached = false;
                DBG_(Iface, (<< "block not found " << VirtualMemoryAddress(blockAddr)));
              } else {
                DBG_(Iface, (<< "block found " << VirtualMemoryAddress(blockAddr)));
              }
            }

            if (predecCached == false) {
              //std::cout << "not predecCached\n";
              if (theBTBMiss[anIndex]) {
                if (!cfg.EnableBTBPrefill) {
                  assert(0);
                }
                DBG_(Iface, (<< "MissUnderMiss "));
                missUnderBTBMiss = true;
                sendprefetch(anIndex, aBBTBEntry.thePC);
                return std::make_pair(false, aBBTBEntry);
                ;
              }

              if (!((branchType == kConditional && aBBTBEntry.theBranchDirection >= kNotTaken))) {
                missUnderBTBMiss = true;
                DBG_(Iface, (<< "Early missUnderMiss "));
              }

              theBTBPredecBuff.insert(theBTBPredecBuff.end(), blockAddresses->theMisses.begin(),
                                      blockAddresses->theMisses.end());

              while (theBTBPredecBuff.size() > 8) {
                theBTBPredecBuff.pop_front();
              }

              DBG_(Iface, (<< "Sending req to L1 "));
              assert(thePredecodeCyclesLeft[anIndex] == 0);

              //std::cout << "FAG: FetchCritical request for: " << (BTBMissFetchBlocks.front() >> 6) << "\n";
              FLEXUS_CHANNEL_ARRAY(BTBRequestOut, anIndex) << BTBMissFetchBlocks.front();
              BTBMissFetchBlocks.pop_front();

              theBTBMiss[anIndex] = true;
              thePredecodeCyclesLeft[anIndex] += (1+numCacheBlocks); // assuming a single cycle per block for decoding!!!!

              BTBPrefilled = true;
              DBG_(Iface, (<< "Prefilling " << std::hex << vpc));

            } else {
              //std::cout << "predecCached\n";
              BTBMissFetchBlocks.clear();
              DBG_(Iface, (<< "PredecBuff hit "));
              isPredecodeBuffHit = true;
            }

            // Update the BBTB
            theBranchPredictor->updateBBTB(aBBTBEntry);
          }
          vpc += 4;
        } else {
          aBBTBEntry.thePC = VirtualMemoryAddress(0);
          isPredecodeBuffHit = true; // If translation fails, consider it a hit and proceed
                                     // instruction by instruction
          lastTranslationFailed[anIndex] = true;
          break;
        }
      } else {
        aBBTBEntry.thePC = VirtualMemoryAddress(0);
        isPredecodeBuffHit =
            true; // If translation fails, consider it a hit and proceed instruction by instruction
        lastTranslationFailed[anIndex] = true;
        break;
      }
    }
    return std::make_pair(isPredecodeBuffHit, aBBTBEntry);
  }
#endif

  void genSingleAddr(index_t anIndex, boost::intrusive_ptr<FetchCommand> fetch) {
    FetchAddr faddr(thePC[anIndex], theNextSerial[anIndex]);
    if (isRedirect[anIndex]) {
      faddr.wasRedirected = true;
      faddr.redirectionCause = squashReason[anIndex];
      isRedirect[anIndex] = false;
    }
    theBranchPredictor->checkPointBPState(faddr);
    thePC[anIndex] += 4;
    // theNextPC[anIndex] = thePC[anIndex] + 4;

    DBG_(Iface,
         Comp(*this)(<< "cpu " << flexusIndex() << " Serial " << faddr.theBPState->theSerial
                     << " Enqueue address " << faddr.theAddress << " "
                     << Flexus::Qemu::Processor::getProcessor(flexusIndex())
                            ->disassemble(faddr.theAddress)));

    fetch->theFetches.push_back(faddr);
  }

  void sendprefetch(index_t anIndex, VirtualMemoryAddress prefetchAddr) {
    DBG_(Iface, (<< "Prefetch for " << prefetchAddr));
    boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
    FetchAddr faddr(prefetchAddr, theNextSerial[anIndex]);
    faddr.redirectionCause = squashReason[anIndex];
    fetch->theFetches.push_back(faddr);
    FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex) << fetch;
  }

  // Implementation of the FetchDrive drive interface
  void doAddressGen(index_t anIndex) {
    //  DBG_(Tmp, ( << std::endl<< std::endl<< std::endl<< "Entering FAG Number: " << anIndex <<
    //  std::endl << std::endl<< std::endl) );
    //	    DBG_(Tmp, ( << std::endl<< "Entering FAG Number: " << anIndex << std::endl <<
    // std::endl));

    if (theFlexus->quiescing()) {
      // We halt address generation when we are trying to quiesce Flexus
      return;
    }

    DBG_Assert(FLEXUS_CHANNEL(uArchHalted).available());
    bool cpu_in_halt = false;
    FLEXUS_CHANNEL(uArchHalted) >> cpu_in_halt;
    if (cpu_in_halt) {
      return;
    }

    if (theBTBMiss[anIndex] /*&& missUnderBTBMiss*/) {
      // Activate prefetching from victimBTB but only if its an L1 miss.
      // If this prefetching is enabled, it will mess-up some of the prefetching related stats.
      if (BTBMissPrefetchesIssued < cfg.BlocksOnBTBMiss) {
        if (BTBMissPrefetchesIssued == 0) {
          //				DBG_(Tmp, ( << "Discont lookup " << std::hex <<
          //(thePC[anIndex]
          //&
          //(~63))));
          BTBMissPrefetchAddr =
              theBranchPredictor->getNextPrefetchAddr(VirtualMemoryAddress(thePC[anIndex] & (~63)));
        } else {
          //				DBG_(Tmp, ( << "Discont lookup " << BTBMissPrefetchAddr));
          BTBMissPrefetchAddr = theBranchPredictor->getNextPrefetchAddr(BTBMissPrefetchAddr);
        }
        BTBMissPrefetchesIssued++;
        sendprefetch(anIndex, BTBMissPrefetchAddr);
      }
      //    	DBG_(Tmp, ( << "going back "));
      return;
    }

    /*This should always be checked after the above theBTBMiss conditional*/
    if (thePredecodeCyclesLeft[anIndex] /*&& missUnderBTBMiss*/) {
      //std::cout << "predecoding, clock: " << theFlexus->cycleCount() << "\n";
      thePredecodeCyclesLeft[anIndex]--;
      if (thePredecodeCyclesLeft[anIndex] == 0) {
        missUnderBTBMiss = false;
      }
      //		  DBG_(Tmp, ( << "going back "));
      return;
    }

    BTBMissPrefetchesIssued = 0;

    if (theRedirect[anIndex]) {
      thePC[anIndex] = theRedirectPC[anIndex];
      // theNextPC[anIndex] = theRedirectNextPC[anIndex];
      theRedirect[anIndex] = false;
      DBG_(Iface, Comp(*this)(<< "Redirect Thread[" << anIndex << "] " << thePC[anIndex]));
      //      DBG_(Tmp, ( << "Redirect to " << std::hex << thePC[anIndex]));
    }
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex).available());
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex).available()); // Rakesh
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(RecordedMissOut, anIndex).available()); // Rakesh
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
    int32_t available_faq = 0;
    FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;

    if (available_faq == 0) {
      //    	DBG_(Tmp, ( << "No space in Q "));
      return;
    }

    boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());

    /*
       if (theNextPC[anIndex] != thePC[anIndex] + 4) {
       DBG_(Iface, ( << "NPC not PC+4"));
       genSingleAddr(anIndex, fetch);

       FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
       FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex) << fetch;
       fetch->theFetches.clear();
       }
       */

    BTBEntry aBTBEntry = theBranchPredictor->access_BBTB(thePC[anIndex]);

    if (cfg.EnableBTBPrefill) {
      if (aBTBEntry.thePC == 0) {
        theBTBMisses++;
        DBG_(Iface, (<< "BTB Miss " << thePC[anIndex]));
        std::pair<bool, BTBEntry> preFill = preFillBBTB(anIndex, thePC[anIndex]);

        if (preFill.first == false) {
          return; // BTB Miss being prefilled
        } else {
          aBTBEntry = preFill.second;
        }
      }
    }

    if (aBTBEntry.thePC == 0) {
      theBTBMisses++;
      DBG_(Iface, (<< "Could not prefill BTB Miss " << cfg.InsnOnBTBMiss));
      if (cfg.EnableBTBPrefill) {
        assert(lastTranslationFailed[anIndex] == true);
      }
      for (int i = 0; i < cfg.InsnOnBTBMiss; i++) {
        genSingleAddr(anIndex, fetch);
      }
    } else {
      theBTBHits++;
      DBG_(Iface, (<< "BTB Hit " << thePC[anIndex]));
      DBG_(Iface,
           (<< std::endl
            << std::endl
            << " BB start: " << std::hex << thePC[anIndex] << " end "
            << (thePC[anIndex] + (aBTBEntry.theBBsize * 4)) << " size " << aBTBEntry.theBBsize
            << " target " << aBTBEntry.theTarget << " type " << aBTBEntry.theBranchType << " pred "
            << aBTBEntry.theBranchDirection << std::endl
            << std::endl));

      int32_t max_addrs = aBTBEntry.theBBsize;
      VirtualMemoryAddress bbTarget;
      VirtualMemoryAddress branchAddress;

      branchAddress =
          thePC[anIndex] +
          ((max_addrs - 1) * 4); // no delay slot in ARM64, branch is always max_addrs-1 * 4

      DBG_(Iface, (<< "AvailableFAQ " << available_faq << " max addr " << max_addrs));

      while (max_addrs > 0) {
        FetchAddr faddr(thePC[anIndex], theNextSerial[anIndex]);
        if (isRedirect[anIndex]) {
          faddr.wasRedirected = true;
          faddr.redirectionCause = squashReason[anIndex];
          isRedirect[anIndex] = false;
        }

        if (thePC[anIndex] == branchAddress) {
          bbTarget = theBranchPredictor->predictBranch(faddr, aBTBEntry);
          faddr.theBPState->BTBPreFilled = BTBPrefilled;
          if (aBTBEntry.theBranchType == kConditional) {
            theCondPredicts++;
            if (faddr.theBPState->bimodalPrediction == false) {
              if (((aBTBEntry.theBranchDirection >= kNotTaken) &&
                   (faddr.theBPState->thePrediction <= kTaken)) ||
                  ((aBTBEntry.theBranchDirection <= kTaken) &&
                   (faddr.theBPState->thePrediction >= kNotTaken))) {
                //						  DBG_(Tmp, ( << "Different
                // predictions"));
                thePredecodeCyclesLeft[anIndex] =
                    1; // Tage prediction overrode BTB, so we need one bubble cycle
                theTageOverrides++;
              } else {
                //					  DBG_(Tmp, ( << "Tage 2-bit same"));
              }
            } else {
              //    			  DBG_(Tmp, ( << "It was not a Tage history prediction"));
            }
          }

          if (bbTarget == 0) {
            thePC[anIndex] += 4; // Could not get the target, so fetch sequentially after the branch
          } else {
            thePC[anIndex] = bbTarget;
          }

          DBG_(Iface,
               (<< "target " << bbTarget << " dir " << faddr.theBPState->thePrediction));
          DBG_(Iface,
               Comp(*this)(<< "cpu " << flexusIndex() << " Serial " << faddr.theBPState->theSerial
                           << " Enqueue Branch address " << faddr.theAddress << " predicted type "
                           << faddr.theBPState->thePredictedType << " "
                           << Flexus::Qemu::Processor::getProcessor(flexusIndex())
                                  ->disassemble(faddr.theAddress)));
        } else {
          thePC[anIndex] += 4;

          theBranchPredictor->checkPointBPState(faddr);
          DBG_(Iface,
               Comp(*this)(<< "cpu " << flexusIndex() << " Serial " << faddr.theBPState->theSerial
                           << " Enqueue address " << faddr.theAddress << " "
                           << Flexus::Qemu::Processor::getProcessor(flexusIndex())
                                  ->disassemble(faddr.theAddress)));
        }

        fetch->theFetches.push_back(faddr);
        //std::cout << "pc: " << faddr.theAddress << "\t" << Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(faddr.theAddress) << "\n";
        max_addrs--;
      }
    }

    DBG_(Iface, (<< "Fetch command size: " << fetch->theFetches.size()));

    BTBPrefilled = false;
    if (fetch->theFetches.size() > 0) {
      // Send it to FetchOut
      FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
      FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex) << fetch;
    }
  }

#if 0
        //Implementation of the FetchDrive drive interface
        void doAddressGen(index_t anIndex) {

            if (theFlexus->quiescing()) {
                //We halt address generation when we are trying to quiesce Flexus
                return;
            }

            if (theBTBMiss[anIndex]) {
                //Activate prefetching from victimBTB but only if its an L1 miss.
                //If this prefetching is enabled, it will mess-up some of the prefetching related stats.
                /*if (BTBMissPrefetchesIssued < 4) {
                  if (BTBMissPrefetchesIssued == 0) {
                  BTBMissPrefetchAddr = theBranchPredictor->getNextPrefetchAddr(VirtualMemoryAddress(thePC[anIndex] & (~63)));
                  } else {
                  BTBMissPrefetchAddr = theBranchPredictor->getNextPrefetchAddr(BTBMissPrefetchAddr);
                  }
                  BTBMissPrefetchesIssued++;
                  boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
                  FetchAddr faddr(BTBMissPrefetchAddr, theNextSerial[anIndex]);
                  faddr.redirectionCause = squashReason[anIndex];
                  fetch->theFetches.push_back( faddr);
                  FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex) << fetch;
                  }*/
                return;
            }
            BTBMissPrefetchesIssued = 0;

            //    DBG_(Tmp, ( << std::endl<< std::endl<< std::endl<< "Entering FAG Number: " << anIndex << std::endl << std::endl<< std::endl) );

            if (theRedirect[anIndex]) {
                thePC[anIndex] = theRedirectPC[anIndex];
                theNextPC[anIndex] = theRedirectNextPC[anIndex];
                theRedirect[anIndex] = false;
                DBG_(Iface, Comp(*this) ( << "Redirect Thread[" << anIndex << "] " << thePC[anIndex]) );
                //      DBG_(Tmp, ( << "Redirect to " << std::hex << thePC[anIndex]));
            }
            DBG_Assert( FLEXUS_CHANNEL_ARRAY( FetchAddrOut, anIndex).available() );
            DBG_Assert( FLEXUS_CHANNEL_ARRAY( PrefetchAddrOut, anIndex).available() );	//Rakesh
            DBG_Assert( FLEXUS_CHANNEL_ARRAY( RecordedMissOut, anIndex).available() );	//Rakesh
            DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex).available() );
            int32_t available_faq = 0;
            FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex) >> available_faq;

            int32_t max_addrs = cfg.MaxFetchAddress;
            if (max_addrs > available_faq) {
                max_addrs = available_faq;
            }

            //    DBG_(Tmp, ( << "AvailableFAQ " << available_faq << " max addr " << max_addrs << " limit " << cfg.MaxFetchAddress) );

            int32_t max_predicts = cfg.MaxBPred;

            boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
            boost::intrusive_ptr<RecordedMisses> recordedMisses = new RecordedMisses();
            while ( max_addrs > 0 ) {
                FetchAddr faddr(thePC[anIndex], theNextSerial[anIndex]);

#ifdef PerfectBranchDetection
                if (cfg.EnableBTBPrefill && !preFillBTB(anIndex, faddr.theAddress)) {
                    break;
                }
#endif

                if(isRedirect[anIndex]){
                    faddr.wasRedirected = true;
                    faddr.redirectionCause = squashReason[anIndex];
                    isRedirect[anIndex] = false;
                }

                //Advance the PC
                if ( theBranchPredictor->isBranch( faddr/*.theAddress*/ ) ) {
                    -- max_predicts;
                    if (max_predicts == 0) {
                        //        	DBG_(Tmp, ( << "Reached branch prediction limit "));
                        if (max_addrs > 2)
                            max_addrs = 2; //This instruction + the next one (as the instruction following branch has to be always executed)
                        //          break;
                    }
                    thePC[anIndex] = theNextPC[anIndex];
                    VirtualMemoryAddress target = theNextPC[anIndex] = theBranchPredictor->predict( faddr );

                    if (target == 0) {
                        theNextPC[anIndex] = thePC[anIndex] + 4;
                    } else if (faddr.theBPState->thePredictedType == kRetry ) {
                        if(!cfg.EnableTrapRet) assert(0);
                        thePC[anIndex] = target;
                        if (faddr.theBPState->theNextPredictedTarget != 0) {
                            theNextPC[anIndex] = faddr.theBPState->theNextPredictedTarget;
                        } else {
                            theNextPC[anIndex] = thePC[anIndex] + 4;
                        }
                    } else if (faddr.theBPState->thePredictedType == kDone) {
                        if(!cfg.EnableTrapRet) assert(0);
                        thePC[anIndex] = target;
                        theNextPC[anIndex] = target + 4;
                    }

                    DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
                    //        DBG_(Tmp, Comp(*this) ( << "cpu " << flexusIndex() << " Serial " << faddr.theBPState->theSerial << " Enqueue Branch address " << faddr.theAddress << " predicted type " << faddr.theBPState->thePredictedType << " "<< Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(faddr.theAddress) << " predictions left " << max_predicts));
                    fetch->theFetches.push_back( faddr);

                    /*if (faddr.theBPState->thePredictedType == kConditional) {
                      if (faddr.theBPState->bimodalPrediction) {
                      assert(faddr.theBPState->saturationCounter >= 0 && faddr.theBPState->saturationCounter < 4);
                      if (faddr.theBPState->saturationCounter > 0 && faddr.theBPState->saturationCounter < 3) {
                      VirtualMemoryAddress prefetechBlock = issueWrongPathPrefetch(target, faddr);
                      if (prefetechBlock) {
                    //						DBG_(Tmp, ( << "Bimod issuing pfetch " << prefetechBlock));
                    recordedMisses->theMisses.push_back(prefetechBlock);
                    recordedMisses->theMisses.push_back(prefetechBlock + 64);
                    }
                    }
                    } else {
                    assert(faddr.theBPState->saturationCounter >= 0 && faddr.theBPState->saturationCounter < 8);
                    if (faddr.theBPState->saturationCounter > 0 && faddr.theBPState->saturationCounter < 7) {
                    VirtualMemoryAddress prefetechBlock = issueWrongPathPrefetch(target, faddr);
                    if (prefetechBlock) {
                    //						DBG_(Tmp, ( << "Tage issuing pfetch " << prefetechBlock));
                    recordedMisses->theMisses.push_back(prefetechBlock);
                    recordedMisses->theMisses.push_back(prefetechBlock + 64);
                    }
                    }
                    }
                    } else if (faddr.theBPState->thePredictedType == kCall || faddr.theBPState->thePredictedType == kIndirectCall) {
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01001300));
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01001340));
                    } else if (faddr.theBPState->thePredictedType == kReturn) {
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01001b00));
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01001b40));
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01005b00));
                    recordedMisses->theMisses.push_back(VirtualMemoryAddress(0x01005b40));
                    }*/
#if 0
                    std::map<VirtualMemoryAddress, VirtualMemoryAddress>::iterator it = missMap.find(faddr.theAddress);
                    if ( it != missMap.end()) {
                        recordedMisses->theMisses.push_back(it->second);
                        //        	DBG_(Tmp, ( << "Found recorded miss " << it->first << " missed block " << it->second));
                    }
#endif
                    //        -- max_predicts;
                } else {
                    thePC[anIndex] += 4;
                    DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
                    //        DBG_(Tmp, Comp(*this) ( << "cpu " << flexusIndex() << " Serial " << faddr.theBPState->theSerial << " Enqueue address " << faddr.theAddress << " " << Flexus::Qemu::Processor::getProcessor(flexusIndex())->disassemble(faddr.theAddress)));
                    fetch->theFetches.push_back( faddr );
                }

                faddr.theBPState->translationFailed = lastTranslationFailed[anIndex];
                --max_addrs;
            }

            if (fetch->theFetches.size() > 0) {
                //Send it to FetchOut
                FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
                FLEXUS_CHANNEL_ARRAY(PrefetchAddrOut, anIndex) << fetch;
            }
            if (recordedMisses->theMisses.size() > 0) {
                FLEXUS_CHANNEL_ARRAY(RecordedMissOut, anIndex) << recordedMisses;
            }
        }
#endif
public:
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) {
    int32_t available_faq = 0;
    DBG_Assert(FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex).available());
    FLEXUS_CHANNEL_ARRAY(AvailableFAQ, anIndex) >> available_faq;
    return available_faq == 0;
  }
};
} // End namespace nFetchAddressGenerate

FLEXUS_COMPONENT_INSTANTIATOR(FetchAddressGenerate, nFetchAddressGenerate);

FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, RedirectIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BranchFeedbackIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, FetchAddrOut) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, PrefetchAddrOut) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BTBRequestOut) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, RecordedMissOut) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, SquashIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, SquashBranchIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, TrapStateIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, SpecialCallIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, RASOpsIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, MissPairIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BTBReplyIn) { // Rakesh
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, BTBMissFetchReplyIn) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, AvailableFAQ) {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH(FetchAddressGenerate, Stalled) {
  return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

#define DBG_Reset
#include DBG_Control
