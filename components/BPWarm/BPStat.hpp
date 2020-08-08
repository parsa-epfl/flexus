/*
 * BPstat.hpp
 *
 *  Created on: 8 Dec 2015
 *      Author: rakesh
 */

#ifndef COMPONENTS_BPWARM_BPSTAT_HPP_
#define COMPONENTS_BPWARM_BPSTAT_HPP_

#include <components/uFetch/uFetchTypes.hpp>
#include <components/CommonQEMU/PerfectBranchDecode.hpp>

#define UNRESOLVED_BRANCH_ARRAY_SIZE 1001
#define CACHE_BLOCK_ADDRESS(X) X >> 6
#define MAX_OFFSET_BITS 50

enum ICacheMissType {	//Rakesh
	None
    , SeqMisses
    , CondForwardBranchMisses
    , CondBackwardBrancheMisses
    , UncondForwardBranchMisses
    , UncondBackwardBrancheMisses
	, CallMisses
    , ReturnMissesMisses
    , IndirectBranchMisses
    , OtherMisses
	, OtherSpecialMisses1
	, OtherSpecialMisses2
};

struct BranchResolve {
	bool is_pred_correct;
	bool is_direction_correct;
	bool is_target_correct;
};

struct BPWarm_stats {
    Stat::StatCounter theBP_PredMispredict;
    Stat::StatCounter theBP_CallCorrect;
    Stat::StatCounter theBP_CallMispredict;
    Stat::StatCounter theBP_RetCorrect;
    Stat::StatCounter theBP_RetMispredict;
    Stat::StatCounter theBP_IndCorrect;
    Stat::StatCounter theBP_IndMispredict;
    Stat::StatCounter theBP_UncondCorrect;
    Stat::StatCounter theBP_UncondMispredict;
    Stat::StatCounter theBP_CondCorrect;
    Stat::StatCounter theBP_CondMispredict_Dir;
    Stat::StatCounter theBP_CondMispredict_Target;
	Stat::StatCounter * seq_block_run[10];
	Stat::StatCounter * last_miss_distance[UNRESOLVED_BRANCH_ARRAY_SIZE];

    Stat::StatCounter theBP_IcacheMisses;
    Stat::StatCounter theBP_IcacheSeqMisses;
    Stat::StatCounter theBP_IcacheCondForwardBranchMisses;
    Stat::StatCounter theBP_IcacheCondBackwardBrancheMisses;
    Stat::StatCounter theBP_IcacheUncondForwardBranchMisses;
    Stat::StatCounter theBP_IcacheUncondBackwardBrancheMisses;
    Stat::StatCounter theBP_IcacheCallMisses;
    Stat::StatCounter theBP_IcacheReturnMissesMisses;
    Stat::StatCounter theBP_IcacheIndirectBranchMisses;
    Stat::StatCounter theBP_IcacheOtherMisses;
    Stat::StatCounter theBP_IcacheOtherSpecialMisses1;
    Stat::StatCounter theBP_IcacheOtherSpecialMisses2;

    Stat::StatCounter * theBP_IForwardTaken[10];
    Stat::StatCounter * theBP_IBackwardTaken[10];
    Stat::StatCounter * theBP_IIndForwardTaken[10];
    Stat::StatCounter * theBP_IIndBackwardTaken[10];

    Stat::StatCounter theBP_ICallInsn;
    Stat::StatCounter theBP_IRetInsn;
    Stat::StatCounter theBP_IindirectBranchInsn;

    Stat::StatCounter theBP_ICorrect;
    Stat::StatCounter theBP_IMispredict;

    uint64_t insn_count;
    uint64_t last_icache_block_address;
    uint64_t first_icache_block_address;	//First icache block in current sequential run
    uint64_t num_sequential_icache_blocks;

    Stat::StatCounter statBigVirtualPC; //PC greater that 0xFFFFFFFF

    Stat::StatCounter * CondBranchOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * UnCondBranchOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * CallOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * IBranchOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * ICallOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * ReturnOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * OtherOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * OverallOffset[MAX_OFFSET_BITS];

    Stat::StatCounter * TakenBranchOffset[MAX_OFFSET_BITS];
    Stat::StatCounter * TakenOverallOffset[MAX_OFFSET_BITS];

    Stat::StatCounter * theBP_CondBranchDist[10];
    Stat::StatCounter * theBP_TakenCondBranchDist[10];

    Stat::StatCounter theBP_TotalCondBranches;
    Stat::StatCounter theBP_TakenCondBranches;

    Stat::StatInstanceCounter<int64_t> offsetFreq;
    Stat::StatInstanceCounter<int64_t> targetFreq;


	BPWarm_stats(std::string const & theName, int unres_branches)
	: theBP_PredMispredict				(theName + "-BP_D_PredMispredict")
	, theBP_CallCorrect					(theName + "-BP_D_CallCorrect")
	, theBP_CallMispredict				(theName + "-BP_D_CallMispredict")
	, theBP_RetCorrect 					(theName + "-BP_D_RetCorrect")
	, theBP_RetMispredict				(theName + "-BP_D_RetMispredict")
	, theBP_IndCorrect					(theName + "-BP_D_IndCorrect")
	, theBP_IndMispredict				(theName + "-BP_D_IndMispredict")
	, theBP_UncondCorrect				(theName + "-BP_D_UncondCorrect")
	, theBP_UncondMispredict			(theName + "-BP_D_UncondMispredict")
	, theBP_CondCorrect					(theName + "-BP_D_CondCorrect")
	, theBP_CondMispredict_Dir			(theName + "-BP_D_CondMispredict_Dir")
	, theBP_CondMispredict_Target		(theName + "-BP_D_CondMispredict_Target")
    , theBP_IcacheMisses               	( theName + "-BP_Icache:Misses" )
	, theBP_IcacheSeqMisses            	( theName + "-BP_Icache_Seq:Misses" )
	, theBP_IcacheCondForwardBranchMisses  	( theName + "-BP_Icache_CondForwardBranch:Misses" )
	, theBP_IcacheCondBackwardBrancheMisses	( theName + "-BP_Icache_CondBackwardBranch:Misses" )
  	, theBP_IcacheUncondForwardBranchMisses	( theName + "-BP_Icache_UncondForwardBranch:Misses" )
  	, theBP_IcacheUncondBackwardBrancheMisses ( theName + "-BP_Icache_UncondBackwardBranch:Misses" )
	, theBP_IcacheCallMisses           	( theName + "-BP_Icache_Call:Misses" )
	, theBP_IcacheReturnMissesMisses   	( theName + "-BP_Icache_Return:Misses" )
	, theBP_IcacheIndirectBranchMisses 	( theName + "-BP_Icache_IndirectBranches:Misses" )
	, theBP_IcacheOtherMisses          	( theName + "-BP_Icache_Other:Misses" )
    , theBP_IcacheOtherSpecialMisses1  	( theName + "-BP_Icache_OtherSpecial1:Misses" )
    , theBP_IcacheOtherSpecialMisses2  	( theName + "-BP_Icache_OtherSpecial2:Misses" )
	, theBP_ICallInsn           		( theName + "-BP_ICall:Insn" )
  	, theBP_IRetInsn   				 	( theName + "-BP_IReturn:Insn" )
  	, theBP_IindirectBranchInsn 		( theName + "-BP_IindirectBranche:Insn" )
	, theBP_ICorrect		   			( theName + "-BP_ICorrect:Branch" )
	, theBP_IMispredict   			 	( theName + "-BP_IMispredict:Branch" )
    , statBigVirtualPC					( theName + "-BP_BigVirtualPC" )
    , theBP_TotalCondBranches ( theName + "-CondBranches" )
    , theBP_TakenCondBranches ( theName + "-TakenCondBranches" )
	, offsetFreq ( theName  + "-offsetFreq" )
	, targetFreq ( theName  + "-targetFreq" )
    {
		char stat_name[50];
		for (int i = 0; i < 10; i++) {
    		sprintf(stat_name, "-Seq_run:Blocks%d", i);
			seq_block_run[i] = new Stat::StatCounter(theName + stat_name);
		}
		for (int i = 0; i <= unres_branches; i++) {
    		sprintf(stat_name, "-last_miss_distance:Branches%d", i);
    		last_miss_distance[i] = new Stat::StatCounter(theName + stat_name);
		}
  		char stat_name1[50];
  		char stat_name2[50];
  		char stat_name3[50];
  		char stat_name4[50];
    	for (int i = 0; i < 10; i++) {
    		sprintf(stat_name1, "-BP_IForward:Blocks%d", i);
    		sprintf(stat_name2, "-BP_IBackward:Blocks%d", i);
    		sprintf(stat_name3, "-BP_IIndForward:Blocks%d", i);
    		sprintf(stat_name4, "-BP_IIndBackward:Blocks%d", i);
    		theBP_IForwardTaken[i] = new Stat::StatCounter(theName + stat_name1);
    	    theBP_IBackwardTaken[i] = new Stat::StatCounter(theName + stat_name2);
    	    theBP_IIndForwardTaken[i] = new Stat::StatCounter(theName + stat_name3);
    	    theBP_IIndBackwardTaken[i] = new Stat::StatCounter(theName + stat_name4);
    	}

    	for (int i = 0; i < 10; i++) {
    		sprintf(stat_name1, "-BP_CondBranchDist:Blocks%d", i);
    		sprintf(stat_name2, "-BP_TakenCondBranchDist:Blocks%d", i);
    		theBP_CondBranchDist[i] = new Stat::StatCounter(theName + stat_name1);
    		theBP_TakenCondBranchDist[i] = new Stat::StatCounter(theName + stat_name2);
    	}

    	insn_count = 0;
        last_icache_block_address = 0;
        first_icache_block_address = 0;
        num_sequential_icache_blocks = 0;

		for (int i = 0; i < MAX_OFFSET_BITS; i++) {
			sprintf(stat_name, "-CondBranchOffset%d", i);
			CondBranchOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-UncondBranchOffset%d", i);
			UnCondBranchOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-CallOffset%d", i);
			CallOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-IBranchOffset%d", i);
			IBranchOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-ICallOffset%d", i);
			ICallOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-ReturnOffset%d", i);
			ReturnOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-OtherOffset%d", i);
			OtherOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-OverallOffset%d", i);
			OverallOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-TakenBranchOffset%d", i);
			TakenBranchOffset[i] = new Stat::StatCounter(theName + stat_name);
			sprintf(stat_name, "-TakenOverallOffset%d", i);
			TakenOverallOffset[i] = new Stat::StatCounter(theName + stat_name);
		}


	}

    void offsetStats(VirtualMemoryAddress anAddress,  eBranchType anActualType, eDirection anActualDirection, VirtualMemoryAddress anActualTarget, uint32_t anOpcode) {
      if (anAddress >= 0xFFFFFFFF) {
    	  statBigVirtualPC++;
      }

      std::pair<eBranchType, VirtualMemoryAddress> aPair = targetDecode(anOpcode);
      int64_t disp_decoded = abs(aPair.second);
      int64_t disp = disp_decoded;
      int64_t target = (int64_t)anAddress + (disp_decoded << 2);
//      DBG_( Tmp, ( << " disp: " << std::hex << disp << " target " << target));
      if (aPair.first != anActualType) {
    	  DBG_( Tmp, ( << " actualType: "<< anActualType << " decoded type " <<aPair.first));
    	  assert(0);
      }

      if (disp == 0) {
		  if (anActualDirection <= kTaken) {
			  disp = abs((int64_t)anActualTarget - (int64_t)anAddress);
			  disp = disp >> 2; //because of instruction alignment
			  target = anActualTarget;
//			  DBG_( Tmp, ( << " New disp: " << std::hex << disp << " target " << target));
		  } else {
			  assert(anActualType == kConditional);
		  }
      }

//      if (anActualDirection <= kTaken) {
//		  offsetFreq << std::make_pair(disp, 1);
//		  targetFreq << std::make_pair(anActualTarget, 1);
//      }

//      DBG_( Tmp, ( << " target: " <<std::hex << anActualTarget << " pc: " << anAddress << " disp " << std::hex << disp << " dispDec " << disp_decoded));
      if(disp == 0) {
//    	  DBG_( Tmp, ( << " pc: " << anAddress << " btype " << anActualType << " opcode " << std::hex << anOpcode));
    	  assert(anActualType == kConditional);
      }

  	  int numBitsRequired = 0;
  	  while (disp) {
  		  disp = disp >> 1;
  		  numBitsRequired++;
  	  }
  	  numBitsRequired++; //Sign bit
//  	  DBG_( Tmp, ( << " Number of bits required are: " << std::dec << numBitsRequired));
  	  if (numBitsRequired >= MAX_OFFSET_BITS) {
  		numBitsRequired = MAX_OFFSET_BITS - 1;
  	  }

	  int64_t branch_block = CACHE_BLOCK_ADDRESS(anAddress);
	  int64_t current_block = CACHE_BLOCK_ADDRESS(target);
	  int jumpDist = abs(current_block - branch_block);
	  if (jumpDist > 9) {
		  jumpDist = 9;
	  }
//	  DBG_( Tmp, ( << "current block " << std::hex << (current_block << 6) << " prev block " << (branch_block << 6) << " diff " << std::dec << jumpDist << " btype " << anActualType));


	  (* OverallOffset[numBitsRequired])++;
	  if (anActualDirection <= kTaken) {
		  (* TakenOverallOffset[numBitsRequired])++;
	  }

      switch (anActualType) {
        case kConditional:
          theBP_TotalCondBranches++;
          (* theBP_CondBranchDist[jumpDist])++;
          (* CondBranchOffset[numBitsRequired])++;
          if (anActualDirection <= kTaken) {
        	  theBP_TakenCondBranches++;
        	  (* theBP_TakenCondBranchDist[jumpDist])++;
        	  (* TakenBranchOffset[numBitsRequired])++;
          }
          break;
        case kUnconditional:
          (* UnCondBranchOffset[numBitsRequired])++;
          break;
        case kCall:
          (* CallOffset[numBitsRequired])++;
          break;
        case kJmplCall:
          (* ICallOffset[numBitsRequired])++;
          break;
        case kReturn:
          (* ReturnOffset[numBitsRequired])++;
          break;
        case kJmpl:
          (* IBranchOffset[numBitsRequired])++;
          break;
        default:
          (* OtherOffset[numBitsRequired])++;
      	  assert(anActualType == kRetry || anActualType == kDone);
          break;
      }


    }

	/* Stuff for Attributing I-cache misses to different categories */
	  ICacheMissType Imiss_by_TakenBranch(eBranchType theFetchType, VirtualMemoryAddress theFetchAddress, BPredState theFetchState, bool is_mispredict) {
		BranchResolve branch_resolve;

		VirtualMemoryAddress ICache_miss_address = theFetchState.ICache_miss_address;
		int64_t branch_block = CACHE_BLOCK_ADDRESS(theFetchAddress);
		int64_t current_block = CACHE_BLOCK_ADDRESS(ICache_miss_address);
		int num_blocks = 9;
//		if (current_block - branch_block == 0){
//			DBG_( Tmp, ( << "current block " << std::hex << current_block << " prev block " << branch_block));
//			assert(current_block - branch_block != 0);
//		}

		if (abs(current_block - branch_block) < 9)
			num_blocks = abs(current_block - branch_block);

		branch_resolve = Imiss_BP_resolution(theFetchType, ICache_miss_address, theFetchState);

		if (branch_resolve.is_direction_correct && branch_resolve.is_target_correct && branch_resolve.is_pred_correct)
			assert(is_mispredict == 0);
		else if (is_mispredict == 0) {
			DBG_( Tmp, ( << " Problem is " << branch_resolve.is_direction_correct << " " <<  branch_resolve.is_target_correct << " " << branch_resolve.is_pred_correct));
			assert(is_mispredict == 1);
		}

		if(!is_mispredict) {
			(* last_miss_distance[theFetchState.last_miss_distance])++;
		}

	  	switch (theFetchType) {
	  	case  kConditional:
	  		if (branch_resolve.is_pred_correct) {
	  			if (branch_resolve.is_direction_correct) {
	  				if (branch_resolve.is_target_correct) {
	  					theBP_CondCorrect++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss:Correct" ));
	  				} else {
	  					theBP_CondMispredict_Target++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss:TragetWrong" ));
	  				}
	  			} else {
	  				theBP_CondMispredict_Dir++;
	//  				DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss:DirectionWrong" ));
	  			}
	  		} else {
	  			theBP_PredMispredict++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss:PredWrong" ));
	  		}

	  		if (ICache_miss_address > theFetchAddress) {
	  			(* theBP_IForwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Forward Branch block " << branch_block << " Current block " << current_block << " Diff " << current_block - branch_block));
	  			return CondForwardBranchMisses;
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss: forward"));
	  		} else {
	  			(* theBP_IBackwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Backward Branch block " << branch_block << " Current block " << current_block << " Diff " << branch_block - current_block));
	  			return CondBackwardBrancheMisses;
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Conditional Miss: backward"));
	  		}
	  		break;
	  	case kUnconditional:
	  		if (branch_resolve.is_pred_correct) {
	  			if (branch_resolve.is_direction_correct) {
	  				if (branch_resolve.is_target_correct) {
	  					theBP_UncondCorrect++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Unconditional Miss:Correct" ));
	  				} else {
	  					theBP_UncondMispredict++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Unconditional Miss:TargetWrong" ));
	  				}
	  			} else {
	  				assert(0);
	  			}
	  		} else {
	  			theBP_PredMispredict++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Unconditional Miss:PredWrong" ));
	  		}

	  		if (ICache_miss_address > theFetchAddress) {
	  			(* theBP_IForwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Forward Branch block " << branch_block << " Current block " << current_block << " Diff " << current_block - branch_block));
	  			return UncondForwardBranchMisses;
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Unconditional Miss: forward"));
	  		} else {
	  			(* theBP_IBackwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Backward Branch block " << branch_block << " Current block " << current_block << " Diff " << branch_block - current_block));
	  			return UncondBackwardBrancheMisses;
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Unconditional Miss: backward"));
	  		}
	  		break;
	  	case kCall:
	  	case kJmplCall:
	  		if (branch_resolve.is_pred_correct) {
	  			if (branch_resolve.is_direction_correct) {
	  				if (branch_resolve.is_target_correct) {
	  					theBP_CallCorrect++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Call Miss:Correct" ));
	  				} else {
	  					theBP_CallMispredict++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Call Miss:TargetWrong" ));
	  				}
	  			} else {
	  				assert(0);
	  			}
	  		} else {
	  			theBP_PredMispredict++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Call Miss:PredWrong" ));
	  		}

	  		return CallMisses;
	  		if (ICache_miss_address > theFetchAddress) {
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Call Miss: forward"));
	  		} else {
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Call Miss: backward"));
	  		}
	  		break;
	  	case kJmpl:
	  		if (branch_resolve.is_pred_correct) {
	  			if (branch_resolve.is_direction_correct) {
	  				if (branch_resolve.is_target_correct) {
	  					theBP_IndCorrect++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ind Miss:Correct" ));
	  				} else {
	  					theBP_IndMispredict++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ind Miss:TargetWrong" ));
	  				}
	  			} else {
	  				assert(0);
	  			}
	  		} else {
	  			theBP_PredMispredict++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ind Miss:PredWrong" ));
	  		}

	  		if (ICache_miss_address > theFetchAddress) {
	  			(* theBP_IIndForwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Forward Indirect Branch block " << branch_block << " Current block " << current_block << " Diff " << current_block - branch_block));
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Jmpl Miss: forward"));
	  		} else {
	  			(* theBP_IIndBackwardTaken[num_blocks])++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << std::hex << " Backward Indirect Branch block " << branch_block << " Current block " << current_block << " Diff " << branch_block - current_block));
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Jmpl Miss: backward"));
	  		}
	  		return IndirectBranchMisses;
	  		break;
	  	case kReturn:
	  		if (branch_resolve.is_pred_correct) {
	  			if (branch_resolve.is_direction_correct) {
	  				if (branch_resolve.is_target_correct) {
	  					theBP_RetCorrect++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ret Miss:Correct" ));
	  				} else {
	  					theBP_RetMispredict++;
	//  					DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ret Miss:TargetWrong" ));
	  				}
	  			} else {
	  				assert(0);
	  			}
	  		} else {
	  			theBP_PredMispredict++;
	//  			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Ret Miss:PredWrong" ));
	  		}

	  		return ReturnMissesMisses;
	  		if (ICache_miss_address > theFetchAddress) {
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Return Miss: forward"));
	  		} else {
	//    			DBG_( Tmp, Addr(aMessage.address()) ( << " Received on Port InsnMissIn[" << anIndex << "] Return Miss: backward"));
	  		}
	  		break;
	  	default:
	  		DBG_Assert(false);
	  	}
	  	return None;
	  }

      void BP_IcacheMiss_Update (ICacheMissType missType) {
    	  theBP_IcacheMisses++;
    	  switch(missType) {
    	  case 	None:
    		  assert(0);
    		  break;
    	  case SeqMisses:
    		  theBP_IcacheSeqMisses++;
    		  break;
    	  case CondForwardBranchMisses:
    		  theBP_IcacheCondForwardBranchMisses++;
    		  break;
    	  case CondBackwardBrancheMisses:
    		  theBP_IcacheCondBackwardBrancheMisses++;
    		  break;
    	  case UncondForwardBranchMisses:
    		  theBP_IcacheUncondForwardBranchMisses++;
    		  break;
    	  case UncondBackwardBrancheMisses:
    		  theBP_IcacheUncondBackwardBrancheMisses++;
    		  break;
    	  case CallMisses:
    		  theBP_IcacheCallMisses++;
    		  break;
    	  case ReturnMissesMisses:
    		  theBP_IcacheReturnMissesMisses++;
    		  break;
    	  case IndirectBranchMisses:
    		  theBP_IcacheIndirectBranchMisses++;
    		  break;
    	  case OtherMisses:
    		  theBP_IcacheOtherMisses++;
    		  break;
    	  case OtherSpecialMisses1:
    		  theBP_IcacheOtherSpecialMisses1++;
    		  break;
    	  case OtherSpecialMisses2:
    		  theBP_IcacheOtherSpecialMisses2++;
    		  break;
    	  default:
    		  assert(0);
    	  }
      }

      void icache_sequential_run_test (uint64_t pc) {
    	insn_count++;
    	uint64_t icache_block_address = CACHE_BLOCK_ADDRESS(pc);
    	if (icache_block_address > last_icache_block_address || icache_block_address < first_icache_block_address)
    	{
    		if (icache_block_address == last_icache_block_address + 1) {
    			num_sequential_icache_blocks++;
    			last_icache_block_address = icache_block_address;
    //			DBG_( Tmp, ( << " Next seq block: " << std::hex << icache_block_address << " last block " << last_icache_block_address << " num seq blocks " << num_sequential_icache_blocks));
    		}
    		else if ((icache_block_address < last_icache_block_address) && (icache_block_address >= first_icache_block_address)) {
    //			DBG_( Tmp, ( << " Prev seq block: " << std::hex << icache_block_address << " last block " << last_icache_block_address));
    			assert(0);
    		} else {
    //			DBG_( Tmp, ( << " Total num seq blocks " << num_sequential_icache_blocks));
    			if (num_sequential_icache_blocks > 9) {
    				num_sequential_icache_blocks = 9;
    			}
    			(* seq_block_run[num_sequential_icache_blocks])++;
    //			DBG_( Tmp, ( << " Discont block: " << std::hex << icache_block_address << " last block " << last_icache_block_address << " num seq blocks " << num_sequential_icache_blocks));
    			first_icache_block_address = icache_block_address;
    			last_icache_block_address = icache_block_address;
    //			if (icache_block_address == 0x40231){
    //				DBG_( Tmp, ( << " Bad block: " << std::dec << bcount++));
    //			}
    //			if (num_sequential_icache_blocks == 0) {
    //				std::map<uint64_t, uint64_t>::iterator it = stats0.find(icache_block_address);
    //				if (it == stats0.end()) {
    //					stats0[icache_block_address] = 1;
    //				} else {
    //					it->second++;
    //				}
    //			}
    			num_sequential_icache_blocks = 0;
    		}
    	}
    //	if (insn_count == 1000000)
    //	{
    //		std::multimap<uint64_t, uint64_t> reverse_stats0;
    //		for (std::map<uint64_t, uint64_t> ::iterator it = stats0.begin(); it != stats0.end(); it++) {
    //			reverse_stats0.insert(std::make_pair(it->second, it->first));
    //		}
    //		for (std::multimap<uint64_t, uint64_t> ::iterator it = reverse_stats0.begin(); it != reverse_stats0.end(); it++) {
    //			DBG_( Tmp, ( << std::hex << it->second << " for " << std::dec << it->first));
    //		}
    //	}
      }

      void BP_Insn_Update (int insnType) {
    	  switch(insnType) {
    	  case 0:
    		  theBP_IRetInsn++;
    		  break;
    	  case 1:
    		  theBP_IindirectBranchInsn++;
    		  break;
    	  case 2:
    		  theBP_ICallInsn++;
    		  break;
    	  default:
    		  assert(0);
    	  }
      }

      BranchResolve Imiss_BP_resolution(eBranchType anActualType, uint32_t anActualTarget, BPredState & aBPState) {
    	BranchResolve branch_resovle = {true, true, true};
    	eDirection anActualDirection = kTaken;
        if (anActualType != aBPState.thePredictedType) {
//        	DBG_( Tmp, ( << " Imispredict: wrong type" ));
        	branch_resovle.is_pred_correct = false;
        	theBP_IMispredict++;
        } else {
          if ( anActualType  == kConditional) {
            if (( aBPState.thePrediction >= kNotTaken ) && ( anActualDirection >= kNotTaken )) {
            	//Correct but should never enter here as the branch in not taken and are counting correctly predicted taken branches
            	theBP_ICorrect++;
            	assert(0);
            } else if (( aBPState.thePrediction <= kTaken ) && ( anActualDirection <= kTaken ) ) {
              if (anActualTarget == static_cast<uint32_t>(aBPState.thePredictedTarget)) {
//            	  DBG_( Tmp, ( << " Icorrpredict: cond taken" ));
            	  theBP_ICorrect++;
              } else {
//            	  DBG_( Tmp, ( << " Imispredict: cond taken wrong address" ));
            	  branch_resovle.is_target_correct = false;
            	  theBP_IMispredict++;
              }
            } else {
//            	DBG_( Tmp, ( << " Imispredict: cond taken wrong direction" ));
            	branch_resovle.is_direction_correct = false;
            	theBP_IMispredict++;
            }
          } else {
            //Unconditinal
            if (anActualTarget == static_cast<uint32_t>(aBPState.thePredictedTarget)) {
//            	DBG_( Tmp, ( << " Icorrpredict: Uncond" ));
            	theBP_ICorrect++;
            } else {
//            	DBG_( Tmp, ( << " Imispredict: Uncond" ));
            	branch_resovle.is_target_correct = false;
            	theBP_IMispredict++;
            }
          }
        }
        return branch_resovle;
      }



};




#endif /* COMPONENTS_BPWARM_BPSTAT_HPP_ */
