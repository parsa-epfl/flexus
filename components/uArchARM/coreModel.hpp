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
#include <components/CommonQEMU/Slices/Translation.hpp>
#include <core/qemu/mmuRegisters.h>

namespace nuArchARM {

enum eLoseWritePermission {
  eLosePerm_Invalidate
  , eLosePerm_Downgrade
  , eLosePerm_Replacement
};

struct armState {
  uint64_t theGlobalRegs[32];
  uint64_t thePC;
  uint64_t theFPRegs[64];
  uint32_t theFPSR;
  uint32_t theFPCR;
  uint32_t thePSTATE;
};
struct CoreModel : public uArchARM {
  static CoreModel * construct(uArchOptions_t options
                                // Msutherl, removed
                               //, std::function< void (Flexus::Qemu::Translation &) > translate
                               , std::function< int() > advance
                               , std::function< void(eSquashCause) > squash
                               , std::function< void(VirtualMemoryAddress) > redirect
                               , std::function< void(int, int) > change_mode
                               , std::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
                               , std::function< void( bool )> signalStoreForwardingHit
                              );

  //Interface to mircoArch
  virtual void initializeRegister(mapped_reg aRegister, register_value aValue) = 0;
  virtual register_value readArchitecturalRegister( reg aRegister, bool aRotate ) = 0;
//  virtual int32_t selectedRegisterSet() const = 0;
  virtual void setRoundingMode(uint32_t aRoundingMode) = 0;

  virtual void getARMState( armState & aState) = 0;
  virtual void restoreARMState( armState & aState) = 0;

  virtual void setPC( uint64_t aPC) = 0;
  virtual uint64_t pc() const = 0;
  virtual void dumpActions() = 0;
  virtual void reset() = 0;
  virtual void resetARM() = 0;

  virtual int32_t availableROB() const = 0;
  virtual bool isSynchronized() const = 0;
  virtual bool isStalled() const = 0;
  virtual int32_t iCount() const = 0;
  virtual bool isQuiesced() const = 0;
  virtual void dispatch(boost::intrusive_ptr<Instruction>) = 0;

  virtual void skipCycle() = 0;
  virtual void cycle(eExceptionType aPendingInterrupt) = 0;

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


  // MMU and Multi-stage translation, now in CoreModel, not QEMU MAI
  // - Msutherl: Oct'18
  virtual bool IsTranslationEnabledAtEL(uint8_t & anEL) = 0;
  virtual void translate(boost::intrusive_ptr<Translation>& aTr) = 0;
  virtual void intermediateTranslationStep(boost::intrusive_ptr<Translation>& aTr) = 0; // TODO: this func, permissions check etc.
  virtual void InitMMU( std::shared_ptr<mmu_regs_t> regsFromQemu ) = 0;

};

struct ResynchronizeWithQemuException {
  bool expected;
  ResynchronizeWithQemuException(bool was_expected = false )  : expected(was_expected) {}
};

} //nuArchARM

#endif //FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
