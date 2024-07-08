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

#ifndef FLEXUS_UARCH_uARCHINTERFACES_HPP_INCLUDED
#define FLEXUS_UARCH_uARCHINTERFACES_HPP_INCLUDED

#include <bitset>
#include <list>

#include <boost/none.hpp>
#include <boost/variant.hpp>
#include <core/debug/debug.hpp>
#include <core/metaprogram.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <functional>

#include "RegisterType.hpp"
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>

#include <core/qemu/mai_api.hpp>

namespace Flexus {
namespace SharedTypes {
struct BranchFeedback;
}
} // namespace Flexus

namespace nuArch {

using namespace Flexus::SharedTypes;

struct SemanticAction {
  virtual bool cancelled() const {
    return false;
  }
  virtual void create() {
  }
  // evaluate returns true if further actions should be stalled (for in-order
  // execution)
  virtual void evaluate() {
  }
  virtual void describe(std::ostream &anOstream) const {
  }
  virtual void satisfyDependants() {
  }
  virtual void squashDependants() {
  }
  virtual void addRef() {
  }
  virtual void releaseRef() {
  }
  virtual int64_t instructionNo() const {
    return 0;
  }
  virtual ~SemanticAction() {
  }
};

inline void intrusive_ptr_add_ref(nuArch::SemanticAction *p) {
  p->addRef();
}

inline void intrusive_ptr_release(nuArch::SemanticAction *p) {
  p->releaseRef();
}

} // namespace nuArch

namespace nuArch {

struct uArch;
using Flexus::SharedTypes::PhysicalMemoryAddress;
using Flexus::SharedTypes::VirtualMemoryAddress;

enum eResourceStatus { kReady, kUnmapped, kNotReady, kLastStatus };

std::ostream &operator<<(std::ostream &anOstream, eResourceStatus aStatus);

inline std::ostream &operator<<(std::ostream &anOstream, SemanticAction const &anAction) {
  anAction.describe(anOstream);
  return anOstream;
}

struct uArch;
struct Interaction;

enum eConsistencyModel { kSC, kTSO, kRMO };

enum eExclusiveMonitorCode {
  kMonitorSet = 0,
  kMonitorUnset = 1,
  kMonitorDoesntExist = 2,
};

enum eExceptionType {
  kException_InstrAddrMisaligned = 0,
  kException_InstrAccessFault    = 1,
  kException_IllegalInstr        = 2,
  kException_Breakpoint          = 3,
  kException_LoadAddrMisaligned  = 4,
  kException_LoadAccessFault     = 5,
  kException_StoreAddrMisaligned = 6,
  kException_StoreAccessFault    = 7,
  kException_ECallFromUMode      = 8,
  kException_ECallFromSMode      = 9,
  kException_ECallFromMMode      = 11,
  kException_InstrPageFault      = 12,
  kException_LoadPageFault       = 13,
  kException_StorePageFault      = 15,
  kException_SModeSoftwareInt    = 0x80000000 + 1,
  kException_MModeSoftwareInt    = 0x80000000 + 3,
  kException_SModeTimerInt       = 0x80000000 + 5,
  kException_MModeTimerInt       = 0x80000000 + 7,
  kException_SModeExternalInt    = 0x80000000 + 9,
  kException_MModeExternalInt    = 0x80000000 + 11,
  kException_None                = 0xffffffff
};

std::ostream &operator<<(std::ostream &anOstream, eExceptionType aCode);

enum ePrivRegs {
  kAbstractSysReg, /* Msutherl: Blanket type for all registers to represent as hashed/encoded
                      5-tuple which are then read through QEMU */
  kLastPrivReg
};

enum eCacheType {
  kInstructionCache,
  kDataCache,
};

enum eInstructionClass {
  clsLoad,
  clsStore,
  clsAtomic,
  clsBranch,
  clsMEMBAR,
  clsComputation,
  clsSynchronizing
};

enum eInstructionCode
// Special cases
{ codeBlackBox,
  codeALU,
  codeLoad,
  codeStore,
  codeRMW,
  codeBranchUnconditional,
  codeBranchConditional,
  codeCALL,
  codeRETURN,
  codeBranchIndirectReg,
  codeBranchIndirectCall,
  codeMEMBARSync,
  codeException,
  codeHaltedState,
  codeLastCode };

typedef enum eMemOp {
  kMemOp_LOAD,
  kMemOp_STORE,
  kMemOp_PREFETCH,
} eMemOp;

typedef enum eAccType
// Special cases
{ kAccType_NORMAL,
  kAccType_ATOMIC,
  kAccType_ATOMICRW,  // Atomic loads and stores
  kAccType_ORDERED,
  kAccType_ORDEREDRW, // Load-Acquire and Store-Release
} eAccType;

std::ostream &operator<<(std::ostream &anOstream, eInstructionCode aCode);

// Options to the OoO core structure are added here.  Do not forget to
// copy from the Cfg object to this structure in uArchImpl.cpp
struct uArchOptions_t {
  std::string name;
  uint32_t node;
  int32_t ROBSize;
  int32_t SBSize;
  bool NAWBypassSB;
  bool NAWWaitAtSync;
  int32_t retireWidth;
  int32_t numMemoryPorts;
  int32_t numSnoopPorts;
  int32_t numStorePrefetches;
  bool prefetchEarly;
  bool spinControlEnabled;
  eConsistencyModel consistencyModel;
  uint32_t coherenceUnit;
  std::function<std::tuple<PhysicalMemoryAddress, bool, bool>(VirtualMemoryAddress, int32_t)>
      translate;
  std::function<PhysicalMemoryAddress(VirtualMemoryAddress)> translatePC;
  bool speculativeOrder;
  bool speculateOnAtomicValue;
  bool speculateOnAtomicValuePerfect;
  int32_t speculativeCheckpoints;
  int32_t checkpointThreshold;
  bool breakOnResynchronize;
  bool validateMMU;
  bool earlySGP;              /* CMU-ONLY */
  bool trackParallelAccesses; /* CMU-ONLY */
  bool inOrderMemory;
  bool inOrderExecute;
  uint32_t onChipLatency;
  uint32_t offChipLatency;

  uint32_t numIntAlu;
  uint32_t intAluOpLatency;
  uint32_t intAluOpPipelineResetTime;

  uint32_t numIntMult;
  uint32_t intMultOpLatency;
  uint32_t intMultOpPipelineResetTime;
  uint32_t intDivOpLatency;
  uint32_t intDivOpPipelineResetTime;

  uint32_t numFpAlu;
  uint32_t fpAddOpLatency;
  uint32_t fpAddOpPipelineResetTime;
  uint32_t fpCmpOpLatency;
  uint32_t fpCmpOpPipelineResetTime;
  uint32_t fpCvtOpLatency;
  uint32_t fpCvtOpPipelineResetTime;

  uint32_t numFpMult;
  uint32_t fpMultOpLatency;
  uint32_t fpMultOpPipelineResetTime;
  uint32_t fpDivOpLatency;
  uint32_t fpDivOpPipelineResetTime;
  uint32_t fpSqrtOpLatency;
  uint32_t fpSqrtOpPipelineResetTime;
};

struct Instruction : public Flexus::SharedTypes::AbstractInstruction {
  virtual void connectuArch(uArch &uArch) = 0;
  virtual void doDispatchEffects() = 0; // used
  virtual void squash() = 0;
  virtual void pageFault(bool p = true) = 0;
  virtual bool isPageFault() const = 0;
  virtual void doRescheduleEffects() = 0;
  virtual void doRetirementEffects() = 0; // used
  virtual void checkTraps() = 0;          // used
  virtual void doCommitEffects() = 0;     // used

  virtual void annul() = 0;
  virtual bool isAnnulled() const = 0;
  virtual bool isSquashed() const = 0;
  virtual void reinstate() = 0; // reverse of annul

  //  virtual void setMMU(Flexus::Qemu::MMU::mmu_t m) = 0;
  //  virtual boost::optional<Flexus::Qemu::MMU::mmu_t> getMMU() const = 0;

  virtual eInstructionClass instClass() const = 0;
  virtual std::string instClassName() const = 0;
  virtual eInstructionCode instCode() const = 0;
  virtual void changeInstCode(eInstructionCode) = 0;

  virtual void redirectPC(VirtualMemoryAddress aPC) = 0;

  virtual VirtualMemoryAddress pc() const = 0;
  virtual VirtualMemoryAddress pcNext() const = 0;
  virtual bool isPriv() const = 0;
  virtual void makePriv() = 0;
  virtual bool isTrap() const = 0;
  virtual bool preValidate() = 0;
  virtual bool advancesSimics() const = 0;
  virtual bool postValidate() = 0;
  virtual bool resync() const = 0;
  virtual void forceResync(bool r = true) = 0;

  virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) = 0;
  virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const = 0;
  virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) = 0;
  virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const = 0;

  virtual bool mayRetire() const = 0;
  virtual bool mayCommit() const = 0;
  virtual bool isResolved() const = 0;
  virtual void setResolved(bool value = true) = 0;
  virtual void raise(eExceptionType anException) = 0;
  virtual eExceptionType willRaise() const = 0;
  virtual void setWillRaise(eExceptionType anException) = 0;
  virtual int64_t sequenceNo() const = 0;
  virtual bool isComplete() const = 0;

  virtual void setOperand(uint32_t idx, uint64_t val) = 0;

  virtual void describe(std::ostream &anOstream) const = 0;

  virtual void overrideSimics() = 0;
  virtual bool willOverrideSimics() const = 0;
  virtual bool isMicroOp() const = 0;

  virtual void setAccessAddress(PhysicalMemoryAddress anAddress) = 0;
  virtual PhysicalMemoryAddress getAccessAddress() const = 0;
  virtual void setMayCommit(bool aMayCommit) = 0;
  virtual void resolveSpeculation() = 0;
  virtual void restoreOriginalInstCode() = 0;

  virtual void setPreceedingInstruction(boost::intrusive_ptr<Instruction> aPredecessor) = 0;
  virtual bool hasExecuted() const = 0;

  virtual bool hasCheckpoint() const = 0;
  virtual void setHasCheckpoint(bool aCkpt) = 0;

  virtual void setRetireStallCycles(uint64_t aDelay) = 0;
  virtual uint64_t retireStallCycles() const = 0;

  virtual void setCanRetireCounter(const uint32_t numCycles) = 0;
  virtual void decrementCanRetireCounter() = 0;
  virtual bool usesIntAlu() const = 0;
  virtual bool usesIntMult() const = 0;
  virtual bool usesIntDiv() const = 0;
  virtual bool usesFpAdd() const = 0;
  virtual bool usesFpCmp() const = 0;
  virtual bool usesFpCvt() const = 0;
  virtual bool usesFpMult() const = 0;
  virtual bool usesFpDiv() const = 0;
  virtual bool usesFpSqrt() const = 0;

  virtual void setUsesFpAdd() = 0;
  virtual void setUsesFpCmp() = 0;
  virtual void setUsesFpCvt() = 0;
  virtual void setUsesFpMult() = 0;
  virtual void setUsesFpDiv() = 0;
  virtual void setUsesFpSqrt() = 0;
};

struct InstructionDependance {
  boost::intrusive_ptr<Instruction> instruction; // For lifetime control
  std::function<void()> squash;
  std::function<void()> satisfy;
};

struct Interaction : public boost::counted_base {
  virtual void operator()(boost::intrusive_ptr<Instruction> anInstruction,
                          nuArch::uArch &aCore) {
    DBG_Assert(false);
  }
  virtual void describe(std::ostream &anOstream) const {
    DBG_Assert(false);
  }

  virtual bool annuls() {
    return false;
  }
  virtual ~Interaction() {
  }
};

inline std::ostream &operator<<(std::ostream &anOstream, Interaction const &anInteraction) {
  anInteraction.describe(anOstream);
  return anOstream;
}

struct reg_ {
  eRegisterType theType;
  uint32_t theIndex;
  bool operator<(reg_ const &aRight) const {
    if (theType == aRight.theType) {
      return theIndex < aRight.theIndex;
    } else {
      return theType < aRight.theType;
    }
  }
  bool operator==(reg_ const &aRight) const {
    return (theType == aRight.theType) && (theIndex == aRight.theIndex);
  }
  friend std::ostream &operator<<(std::ostream &anOstream, reg_ const &aReg) {
    anOstream << aReg.theType << "[" << aReg.theIndex << "]";
    return anOstream;
  }
};
struct reg : public reg_ {};
struct mapped_reg : public reg_ {};

inline mapped_reg xReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = xRegisters;
  ret_val.theIndex = anIndex;
  return ret_val;
}
inline mapped_reg vReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = vRegisters;
  ret_val.theIndex = anIndex;
  return ret_val;
}

typedef boost::variant<int64_t, uint64_t, bits> register_value;

struct uArch {

  virtual ~uArch() {
  }
  virtual mapped_reg map(reg aReg) {
    DBG_Assert(false);
    return mapped_reg();
  }
  virtual std::pair<mapped_reg, mapped_reg> create(reg aReg) {
    DBG_Assert(false);
    return std::make_pair(mapped_reg(), mapped_reg());
  }
  virtual void free(mapped_reg aReg) {
    DBG_Assert(false);
  }
  virtual void restore(reg aName, mapped_reg aReg) {
    DBG_Assert(false);
  }
  virtual void create(boost::intrusive_ptr<SemanticAction> anAction) {
    DBG_Assert(false);
  }
  virtual void reschedule(boost::intrusive_ptr<SemanticAction> anAction) {
    DBG_Assert(false);
  }
  virtual eResourceStatus requestRegister(mapped_reg aRegister,
                                          InstructionDependance const &aDependance) {
    DBG_Assert(false);
    return kNotReady;
  }
  virtual eResourceStatus requestRegister(mapped_reg aRegister) {
    DBG_Assert(false);
    return kNotReady;
  }
  virtual register_value readRegister(mapped_reg aRegister) {
    DBG_Assert(false);
    return (uint64_t)0ULL;
  }
  virtual void squashRegister(mapped_reg aRegister) {
    DBG_Assert(false);
  }
  virtual void writeRegister(mapped_reg aRegister, register_value aValue) {
    DBG_Assert(false);
  }
  virtual void copyRegValue(mapped_reg aSource, mapped_reg aDest) {
    DBG_Assert(false);
  }
  virtual void satisfy(InstructionDependance const &aDep) {
    DBG_Assert(false);
  }
  virtual void squash(InstructionDependance const &aDep) {
    DBG_Assert(false);
  }
  virtual void satisfy(std::list<InstructionDependance> &dependances) {
    DBG_Assert(false);
  }
  virtual void squash(std::list<InstructionDependance> &dependances) {
    DBG_Assert(false);
  }
  virtual void applyToNext(boost::intrusive_ptr<Instruction> anInsn,
                           boost::intrusive_ptr<Interaction> anInteraction) {
    DBG_Assert(false);
  }
  virtual void deferInteraction(boost::intrusive_ptr<Instruction> anInsn,
                                boost::intrusive_ptr<Interaction> anInteraction) {
    DBG_Assert(false);
  }
  virtual bool squashFrom(boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
    return false;
  }
  virtual void redirectFetch(VirtualMemoryAddress anAddress) {
    DBG_Assert(false);
  }
  virtual void insertLSQ(boost::intrusive_ptr<Instruction> anInsn, eOperation anOperation,
                         eSize aSize, bool aBypassSB, eAccType type) {
    DBG_Assert(false);
  }
  virtual void insertLSQ(boost::intrusive_ptr<Instruction> anInsn, eOperation anOperation,
                         eSize aSize, bool aBypassSB, InstructionDependance const &aDependance,
                         eAccType type) {
    DBG_Assert(false);
  }
  virtual void eraseLSQ(boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
  }
  virtual void resolveVAddr(boost::intrusive_ptr<Instruction> anInsn,
                            VirtualMemoryAddress theAddr) {
    DBG_Assert(false);
  }
  virtual void translate(boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
  }
  virtual void resolvePAddr(boost::intrusive_ptr<Instruction> anInsn,
                            PhysicalMemoryAddress theAddr) {
    DBG_Assert(false);
  }
  virtual void updateStoreValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue,
                                boost::optional<bits> anExtendedValue = boost::none) {
    DBG_Assert(false);
  }
  virtual void annulStoreValue(boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
  }
  virtual bits updateRMWValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue,
                              eOperation anOperation, eRMWOperation anRMW) {
    DBG_Assert(false);
    return 0;
  }
  virtual void retireMem(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
  }
  virtual void checkPageFault(boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
  }
  virtual void commitStore(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
  }
  virtual void invalidateCache(eCacheType aType) {
    DBG_Assert(false);
  }
  virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress) {
    DBG_Assert(false);
  }
  virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize) {
    DBG_Assert(false);
  }
  virtual void SystemRegisterTrap(uint32_t no) {
    DBG_Assert(false);
  }
  virtual void accessMem(PhysicalMemoryAddress anAddress,
                         boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
  }
  virtual bits retrieveLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return bits(0);
  }
  virtual void setLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction,
                            bits aCorrespondingValue) {
    DBG_Assert(false);
  }
  virtual bits
  retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return bits(0);
  }
  virtual bool checkStoreRetirement(boost::intrusive_ptr<Instruction> aStore) {
    DBG_Assert(false);
    return false;
  }
  virtual uint32_t getRoundingMode() {
    DBG_Assert(false);
    return 0;
  }
  virtual void setException(Flexus::Qemu::API::exception_t anEXP) {
    DBG_Assert(false);
  }
  virtual Flexus::Qemu::API::exception_t getException() {
    DBG_Assert(false);
    return Flexus::Qemu::API::exception_t();
  }
  virtual void bypass(mapped_reg aReg, register_value aValue) {
    DBG_Assert(false);
  }
  virtual void connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst,
                             std::function<bool(register_value)>) {
    DBG_Assert(false);
  }
  virtual eConsistencyModel consistencyModel() const {
    DBG_Assert(false);
    return kSC;
  }
  virtual bool speculativeConsistency() const {
    DBG_Assert(false);
    return false;
  }
  virtual bool sbEmpty() const {
    DBG_Assert(false);
    return false;
  }
  virtual bool sbFull() const {
    DBG_Assert(false);
    return false;
  }
  virtual bool mayRetire_MEMBARStLd() const {
    DBG_Assert(false);
    return false;
  }
  virtual bool mayRetire_MEMBARStSt() const {
    DBG_Assert(false);
    return false;
  }
  virtual bool mayRetire_MEMBARSync() const {
    DBG_Assert(false);
    return false;
  }
  virtual void branchFeedback(boost::intrusive_ptr<BranchFeedback> feedback) {
    DBG_Assert(false);
  }
  virtual void takeTrap(boost::intrusive_ptr<Instruction> anInstruction, eExceptionType aTrapType) {
    DBG_Assert(false);
  }

  virtual void clearExclusiveLocal() {
    DBG_Assert(false);
  }
  virtual void clearExclusiveGlobal() {
    DBG_Assert(false);
  }

  virtual void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker) {
    DBG_Assert(false);
  }
  virtual void markExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker) {
    DBG_Assert(false);
  }
  virtual void markExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize, uint64_t marker) {
    DBG_Assert(false);
  }

  virtual int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize) {
    DBG_Assert(false);
    return false;
  }
  virtual int isExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize) {
    DBG_Assert(false);
    return false;
  }
  virtual int isExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize) {
    DBG_Assert(false);
    return false;
  }
  virtual bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction) {
    DBG_Assert(false);
    return false;
  }

  virtual void tlbi(uint64_t addr, uint64_t asid) {
    DBG_Assert(false);
  }
};

static const VirtualMemoryAddress kUnresolved(-1);

} // namespace nuArch

namespace Flexus {
namespace SharedTypes {
typedef nuArch::Instruction Instruction;

using nuArch::kException;
using nuArch::kResynchronize;

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED
