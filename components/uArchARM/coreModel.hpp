// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
#define FLEXUS_uARCH_COREMODEL_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <components/uFetch/uFetchTypes.hpp>
#include <components/CommonQEMU/Slices/PredictorMessage.hpp> /* CMU-ONLY */

namespace Flexus {
namespace Qemu {
struct Translation;
} //Qemu
} //Flexus

namespace nuArchARM {

enum eLoseWritePermission {
  eLosePerm_Invalidate
  , eLosePerm_Downgrade
  , eLosePerm_Replacement
};

struct armState {
  uint64_t theGlobalRegs[32];
  uint64_t theWindowRegs[128];
  uint64_t thePC;
  uint64_t theFPRegs[64];
  uint64_t theFPSR;
  uint64_t theFPCR;
  uint64_t thePSTATE;
  uint64_t theASI;
};
struct CoreModel : public uArchARM {
  static CoreModel * construct(uArchOptions_t options
                               , std::function< void (Flexus::Qemu::Translation &, bool) > translate
                               , std::function< int(bool) > advance
                               , std::function< void(eSquashCause) > squash
                               , std::function< void(VirtualMemoryAddress) > redirect
                               , std::function< void(int, int) > change_mode
                               , std::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
                               , std::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> notifyTMS /* CMU-ONLY */
                               , std::function< void( bool )> signalStoreForwardingHit
                              );

  //Interface to mircoArch
  virtual void initializeRegister(mapped_reg aRegister, register_value aValue) = 0;
  virtual register_value readArchitecturalRegister( reg aRegister, bool aRotate ) = 0;
//  virtual int32_t selectedRegisterSet() const = 0;
  virtual void setRoundingMode(uint32_t aRoundingMode) = 0;


  virtual void setPSTATE( uint64_t aPSTATE) = 0;
  virtual uint64_t getPSTATE( ) = 0;
  virtual void setSP( uint64_t aSP, unsigned idx) = 0;
  virtual uint64_t getSP(unsigned idx) = 0;
  virtual void setEL( uint64_t aEL, unsigned idx) = 0;
  virtual uint64_t getEL(unsigned idx) = 0;
  virtual void setSPSR_EL( uint64_t aSPSR, unsigned idx) = 0;
  virtual uint64_t getSPSR_EL(unsigned idx) = 0;
  virtual void setFPCR( uint64_t anFPCR) = 0;
  virtual void setSPSR(uint64_t anSPSR) = 0;
  virtual uint64_t getSPSR() = 0;
  virtual void setCurrentEL(uint64_t anEL) = 0;
  virtual uint64_t getCurrentEL() = 0;

  virtual void getARMState( armState & aState) = 0;
  virtual void restoreARMState( armState & aState) = 0;

  virtual void setPC( uint64_t aPC) = 0;
  virtual uint64_t pc() const = 0;
  virtual void dumpActions() = 0;
  virtual void reset() = 0;
  virtual void resetARM() = 0;

  virtual bool getSuccess() = 0;
  virtual void setSuccess(bool val) = 0;

  virtual int32_t availableROB() const = 0;
  virtual bool isSynchronized() const = 0;
  virtual bool isStalled() const = 0;
  virtual int32_t iCount() const = 0;
  virtual bool isQuiesced() const = 0;
  virtual void dispatch(boost::intrusive_ptr<Instruction>) = 0;

  virtual void skipCycle() = 0;
  virtual void cycle(int32_t aPendingInterrupt) = 0;

  virtual void pushMemOp(boost::intrusive_ptr< MemOp >) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;

  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;

  virtual void loseWritePermission( eLoseWritePermission aReason, PhysicalMemoryAddress anAddress) = 0;

};

struct ResynchronizeWithSimicsException {
  bool expected;
  ResynchronizeWithSimicsException(bool was_expected = false )  : expected(was_expected) {}
};

} //nuArchARM

#endif //FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
