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

#ifndef FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
#define FLEXUS_uARCH_COREMODEL_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <components/CommonQEMU/Slices/PredictorMessage.hpp> /* CMU-ONLY */
#include <components/uFetch/uFetchTypes.hpp>

namespace nuArchARM {

enum eLoseWritePermission { eLosePerm_Invalidate, eLosePerm_Downgrade, eLosePerm_Replacement };

struct armState {
  uint64_t theCCRegs;
  uint64_t theGlobalRegs[32];
  uint64_t thePC;
  uint64_t theFPRegs[64];
  uint32_t theFPSR;
  uint32_t theFPCR;
  uint32_t thePSTATE;
};
struct CoreModel : public uArchARM {
  static CoreModel *construct(uArchOptions_t options
                              // Msutherl, removed
                              //, std::function< void (Flexus::Qemu::Translation &) > translate
                              ,
                              std::function<int(bool)> advance,
                              std::function<void(eSquashCause)> squash,
                              std::function<void(VirtualMemoryAddress)> redirect,
                              std::function<void(int, int)> change_mode,
                              std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                              std::function<void(bool)> signalStoreForwardingHit,
                              std::function<void(int32_t)> mmuResync);

  // Interface to mircoArch
  virtual void initializeRegister(mapped_reg aRegister, register_value aValue) = 0;
  virtual register_value readArchitecturalRegister(reg aRegister, bool aRotate) = 0;
  //  virtual int32_t selectedRegisterSet() const = 0;
  virtual void setRoundingMode(uint32_t aRoundingMode) = 0;

  virtual void getARMState(armState &aState) = 0;
  virtual void restoreARMState(armState &aState) = 0;

  virtual void setPC(uint64_t aPC) = 0;
  virtual uint64_t pc() const = 0;
  virtual void dumpActions() = 0;
  virtual void reset() = 0;
  virtual void resetARM() = 0;

  virtual int32_t availableROB() const = 0;
  virtual bool isSynchronized() const = 0;
  virtual bool isStalled() const = 0;
  virtual bool isHalted() const = 0;
  virtual int32_t iCount() const = 0;
  virtual bool isQuiesced() const = 0;
  virtual void dispatch(boost::intrusive_ptr<Instruction>) = 0;

  virtual void skipCycle() = 0;
  virtual void cycle(eExceptionType aPendingInterrupt) = 0;
  virtual void issueMMU(TranslationPtr aTranslation) = 0;

  virtual bool checkValidatation() = 0;
  virtual void pushMemOp(boost::intrusive_ptr<MemOp>) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual TranslationPtr popTranslation() = 0;
  virtual void pushTranslation(TranslationPtr aTranslation) = 0;
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;

  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;

  virtual void loseWritePermission(eLoseWritePermission aReason,
                                   PhysicalMemoryAddress anAddress) = 0;

  // MMU and Multi-stage translation, now in CoreModel, not QEMU MAI
  // - Msutherl: Oct'18
  //  virtual void translate(boost::intrusive_ptr<Translation>& aTr) = 0;
};

struct ResynchronizeWithQemuException {
  bool expected;
  ResynchronizeWithQemuException(bool was_expected = false) : expected(was_expected) {
  }
};

} // namespace nuArchARM

#endif // FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
