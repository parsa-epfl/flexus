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


#ifndef FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED
#define FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED

#include <bitset>
#include <list>

#include <functional>
#include <core/debug/debug.hpp>
#include <core/metaprogram.hpp>
#include <boost/variant.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <boost/none.hpp>

#include "RegisterType.hpp"
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>
#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include "CoreModel/SCTLR_EL.hpp"
//#include "systemRegister.hpp"

#include <core/qemu/mai_api.hpp>

#define EL0 0
#define EL1 1
#define EL2 2
#define EL3 3

namespace Flexus {
namespace SharedTypes {
struct BranchFeedback;
}
}

namespace nuArchARM {

//struct SysRegInfo;

using namespace Flexus::SharedTypes;

struct SemanticAction {
  virtual bool cancelled() const {
    return false;
  }
  virtual void create() {}
  //evaluate returns true if further actions should be stalled (for in-order execution)
  virtual void evaluate() {}
  virtual void describe(std::ostream & anOstream) const {}
  virtual void satisfyDependants() {}
  virtual void squashDependants() {}
  virtual void addRef() {}
  virtual void releaseRef() {}
  virtual int64_t instructionNo() const {
    return 0;
  }
  virtual ~SemanticAction() {}
};

inline void intrusive_ptr_add_ref(nuArchARM::SemanticAction * p) {
  p->addRef();
}

inline void intrusive_ptr_release(nuArchARM::SemanticAction * p) {
  p->releaseRef();
}

} //nuArchARM

namespace nuArchARM {

struct uArchARM;
using Flexus::SharedTypes::VirtualMemoryAddress;
using Flexus::SharedTypes::PhysicalMemoryAddress;

enum eResourceStatus {
  kReady
  , kUnmapped
  , kNotReady
  , kLastStatus
};

std::ostream & operator << ( std::ostream & anOstream, eResourceStatus aStatus);

inline std::ostream & operator << ( std::ostream & anOstream, SemanticAction const & anAction) {
  anAction.describe( anOstream );
  return anOstream;
}

struct uArchARM;
struct Interaction;

enum eConsistencyModel {
  kSC
  , kTSO
  , kRMO
};


enum eAccessResult {
        /* Access is permitted */
        kACCESS_OK = 0,
        /* Access fails due to a configurable trap or enable which would
         * result in a categorized exception syndrome giving information about
         * the failing instruction (ie syndrome category 0x3, 0x4, 0x5, 0x6,
         * 0xc or 0x18). The exception is taken to the usual target EL (EL1 or
         * PL1 if in EL0, otherwise to the current EL).
         */
       kACCESS_TRAP = 1,
        /* Access fails and results in an exception syndrome 0x0 ("uncategorized").
         * Note that this is not a catch-all case -- the set of cases which may
         * result in this failure is specifically defined by the architecture.
         */
       kACCESS_TRAP_UNCATEGORIZED = 2,
        /* AskACCESS_TRAP, but for traps directly to EL2 or EL3 */
       kACCESS_TRAP_EL2 = 3,
       kACCESS_TRAP_EL3 = 4,
        /* AskACCESS_UNCATEGORIZED, but for traps directly to EL2 or EL3 */
       kACCESS_TRAP_UNCATEGORIZED_EL2 = 5,
       kACCESS_TRAP_UNCATEGORIZED_EL3 = 6,
        /* Access fails and results in an exception syndrome for an FP access,
         * trapped directly to EL2 or EL3
         */
       kACCESS_TRAP_FP_EL2 = 7,
       kACCESS_TRAP_FP_EL3 = 8,
};

enum eCacheType {
    kInstructionCache,
    kDataCache,
};


enum eCachePoint {
    kPoC, // Point of Coherency (PoC). For a particular address, the PoC is the point at which all observers,
          // for example, cores, DSPs, or DMA engines, that can access memory, are guaranteed to see the same copy
          // of a memory location. Typically, this is the main external system memory.

    kPoU, // Point of Unification (PoU). The PoU for a core is the point at which the instruction and data caches and
          // translation table walks of the core are guaranteed to see the same copy of a memory location. For example,
          // a unified level 2 cache would be the point of unification in a system with Harvard level 1 caches and a TLB
          // for caching translation table entries. If no external cache is present, main memory would be the Point of Unification.
};

enum eShareableDomain {
  kNonShareable,   // This represents memory accessible only by a single processor or other agent,
                    // so memory accesses never need to be synchronized with other processors.
                    // This domain is not typically used in SMP systems.

  KInnerShareable, // This represents a shareability domain that can be shared by multiple processors,
                    // but not necessarily all of the agents in the system. A system might have multiple Inner Shareable domains.
                    // An operation that affects one Inner Shareable domain does not affect other Inner Shareable domains in the system.
                    // An example of such a domain might be a quad-core Cortex-A57 cluster.

  kOuterShareable, // An outer shareable (OSH) domain re-orderis shared by multiple agents and can consist of one or more inner shareable domains.
                    // An operation that affects an outer shareable domain also implicitly affects all inner shareable domains inside it. However,
                    // it does not otherwise behave as an inner shareable operation.

  kFullSystem,     // An operation on the full system (SY) affects all observers in the system.
};

enum eInstructionClass {
  clsLoad
  , clsStore
  , clsAtomic
  , clsBranch
  , clsMEMBAR
  , clsComputation
  , clsSynchronizing
};

enum eInstructionCode
//Special cases
{
  codeBlackBox
  , codeITLBMiss
  , codeNOP
  , codeMAGIC
  //ALU
  , codeSETHI
  , codeALU
  , codeMul
  , codeDiv
  , codeRDPR
  , codeWRPR
  //Register Window Manipulation
  , codeRestore
  , codeSave
  , codeRestored
  , codeSaved
  , codeFLUSHW
  //FP
  , codeFP
  , codeALIGN
  //Memory
  , codeLoad
  , codeLoadFP
  , codeLDD
  , codeStore
  , codeStoreFP
  , codeSTD
  //Atomics
  , codeCAS
  , codeSWAP
  , codeLDSTUB
  //Branches
  , codeBranchUnconditional
  , codeBranchConditional
  , codeBranchFPConditional
  , codeCALL
  , codeRETURN
  //MEMBARs
  , codeMEMBARSync
  , codeMEMBARStLd
  , codeMEMBARStSt
  //Unsupported Instructions
  , codeRDPRUnsupported
  , codeWRPRUnsupported
  , codePOPCUnsupported
  , codeRETRYorDONE

  //Codes that are not used by the decode component, but are used in
  //the instruction counter
  , codeExceptionUnsupported
  , codeException
  , codeSideEffectLoad
  , codeSideEffectStore
  , codeSideEffectAtomic

  , codeDeviceAccess
  , codeMMUAccess
  , codeTcc

  //End of Enum
  , codeLastCode
};

typedef enum eMemOp
{
    kMemOp_LOAD,
    kMemOp_STORE,
    kMemOp_PREFETCH,
}eMemOp;

typedef enum eAccType
//Special cases
{
    kAccType_NORMAL, kAccType_VEC,           // Normal loads and stores
    kAccType_STREAM, kAccType_VECSTREAM,     // Streaming loads and stores
    kAccType_ATOMIC, kAccType_ATOMICRW,      // Atomic loads and stores
    kAccType_ORDERED, kAccType_ORDEREDRW,    // Load-Acquire and Store-Release
    kAccType_LIMITEDORDERED,                // Load-LOAcquire and Store-LORelease
    kAccType_UNPRIV,                        // Load and store unprivileged
    kAccType_IFETCH,                        // Instruction fetch
    kAccType_PTW,                           // Page table walk
    // Other operations
    kAccType_DC,                            // Data cache maintenance
    kAccType_IC,                            // Instruction cache maintenance
    kAccType_DCZVA,                         // DC ZVA instructions
    kAccType_AT,                            // Address translation
    kLastAccType,                           // last one for debugging
}eAccType;

std::ostream & operator << ( std::ostream & anOstream, eInstructionCode aCode);

// Options to the OoO core structure are added here.  Do not forget to
// copy from the Cfg object to this structure in uArchImpl.cpp
struct uArchOptions_t {
  std::string   name;
  uint32_t  node;
  int32_t           ROBSize;
  int32_t           SBSize;
  bool          NAWBypassSB;
  bool          NAWWaitAtSync;
  int32_t           retireWidth;
  int32_t           numMemoryPorts;
  int32_t           numSnoopPorts;
  int32_t           numStorePrefetches;
  bool          prefetchEarly;
  bool          spinControlEnabled;
  eConsistencyModel
  consistencyModel;
  uint32_t  coherenceUnit;
  std::function< std::tuple< PhysicalMemoryAddress, bool, bool> ( VirtualMemoryAddress, int32_t ) >
  translate;
  std::function< PhysicalMemoryAddress ( VirtualMemoryAddress ) >
  translatePC;
  bool          speculativeOrder;
  bool          speculateOnAtomicValue;
  bool          speculateOnAtomicValuePerfect;
  int32_t           speculativeCheckpoints;
  int32_t           checkpointThreshold;
  bool          breakOnResynchronize;
  bool          validateMMU;
  bool          earlySGP;   /* CMU-ONLY */
  bool          trackParallelAccesses; /* CMU-ONLY */
  bool          inOrderMemory;
  bool          inOrderExecute;
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
  virtual void connectuArch(uArchARM & uArchARM) = 0;
  virtual void doDispatchEffects() = 0;     //used
  virtual void squash() = 0;
  virtual void doRescheduleEffects() = 0;
  virtual void doRetirementEffects() = 0;       //used
  virtual void checkTraps() = 0;       //used
  virtual void doCommitEffects() = 0;       //used

  virtual void annul() = 0;
  virtual bool isAnnulled() const = 0;
  virtual bool isSquashed() const = 0;
  virtual void reinstate() = 0; //reverse of annul

//  virtual void setMMU(Flexus::Qemu::MMU::mmu_t m) = 0;
//  virtual boost::optional<Flexus::Qemu::MMU::mmu_t> getMMU() const = 0;

  virtual eInstructionClass instClass() const = 0;
  virtual std::string instClassName() const = 0;
  virtual eInstructionCode instCode() const = 0;
  virtual void changeInstCode(eInstructionCode ) = 0;

  virtual VirtualMemoryAddress pc() const = 0;

  virtual bool preValidate() = 0;
  virtual bool advancesSimics() const = 0;
  virtual bool postValidate() = 0;
  virtual bool resync() const = 0;
  virtual void forceResync() = 0;

  virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) = 0;
  virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const = 0;
  virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) = 0;
  virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const = 0;

  virtual bool mayRetire() const = 0;
  virtual bool mayCommit() const = 0;
  virtual void raise(int32_t anException)  = 0;
  virtual int32_t willRaise() const = 0;
  virtual void setWillRaise(int32_t anException) = 0;
  virtual int64_t sequenceNo() const = 0;
  virtual bool isComplete() const = 0;

  virtual void describe(std::ostream & anOstream) const = 0;

  virtual void overrideSimics() = 0;
  virtual bool willOverrideSimics() const = 0;
  virtual bool isMicroOp() const = 0;

  virtual void setAccessAddress(PhysicalMemoryAddress anAddress) = 0;
  virtual PhysicalMemoryAddress getAccessAddress() const = 0;
  virtual void setMayCommit(bool aMayCommit) = 0;
  virtual void resolveSpeculation() = 0;
  virtual void restoreOriginalInstCode() = 0;

  virtual void setPreceedingInstruction(boost::intrusive_ptr< Instruction > aPredecessor) = 0;
  virtual bool hasExecuted() const = 0;
//  virtual int32_t getASI() const = 0;
//    virtual void setXRegister(uint32_t aReg, uint64_t aVal) = 0;
//    virtual uint64_t getXRegister(uint32_t aReg) = 0;

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

};

struct InstructionDependance {
  boost::intrusive_ptr< Instruction > instruction; //For lifetime control
  std::function < void() > squash;
  std::function < void() > satisfy;
};

struct Interaction : public boost::counted_base {
  virtual void operator() (boost::intrusive_ptr<Instruction> anInstruction, nuArchARM::uArchARM & aCore) {
    DBG_Assert(false);
  }
  virtual void describe(std::ostream & anOstream) const {
    DBG_Assert(false);
  }

  virtual bool annuls() {
    return false;
  }
  virtual ~Interaction() {}
};

inline std::ostream & operator << ( std::ostream & anOstream, Interaction const & anInteraction) {
  anInteraction.describe( anOstream );
  return anOstream;
}

struct reg_ {
  eRegisterType theType;
  uint32_t theIndex;
  bool operator < ( reg_ const & aRight) const {
    if (theType == aRight.theType) {
      return theIndex < aRight.theIndex;
    } else {
      return theType < aRight.theType;
    }
  }
  bool operator == ( reg_ const & aRight) const {
    return (theType == aRight.theType) && (theIndex == aRight.theIndex);
  }
  friend std::ostream & operator << (std::ostream & anOstream, reg_ const & aReg) {
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
inline mapped_reg ccReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = ccBits;
  ret_val.theIndex = anIndex;
  return ret_val;
}

typedef boost::variant< uint64_t , bits > register_value;

struct uArchARM {

    virtual ~uArchARM() {}
    virtual mapped_reg map( reg aReg ) {
    DBG_Assert( false );
    return mapped_reg();
    }
    virtual std::pair<mapped_reg, mapped_reg> create( reg aReg ) {
      DBG_Assert( false );
      return std::make_pair(mapped_reg(), mapped_reg());
    }
    virtual void free( mapped_reg aReg ) {
    DBG_Assert( false );
    }
    virtual void restore( reg aName, mapped_reg aReg ) {
    DBG_Assert( false );
    }
    virtual void create( boost::intrusive_ptr<SemanticAction> anAction) {
    DBG_Assert( false );
    }
    virtual void reschedule( boost::intrusive_ptr<SemanticAction> anAction) {
    DBG_Assert( false );
    }
    virtual eResourceStatus requestRegister( mapped_reg aRegister, InstructionDependance const & aDependance) {
    DBG_Assert( false );
    return kNotReady;
    }
    virtual eResourceStatus requestRegister( mapped_reg aRegister) {
    DBG_Assert( false );
    return kNotReady;
    }
    virtual register_value readRegister( mapped_reg aRegister ) {
    DBG_Assert( false );
    return 0ULL;
    }
    virtual void squashRegister( mapped_reg aRegister) {
    DBG_Assert( false );
    }
    virtual void writeRegister( mapped_reg aRegister, register_value aValue ) {
    DBG_Assert( false );
    }
    virtual void copyRegValue( mapped_reg aSource, mapped_reg aDest ) {
    DBG_Assert( false );
    }
    virtual void satisfy( InstructionDependance const & aDep) {
    DBG_Assert(false);
    }
    virtual void squash( InstructionDependance const & aDep) {
    DBG_Assert(false);
    }
    virtual void satisfy( std::list<InstructionDependance> & dependances) {
    DBG_Assert(false);
    }
    virtual void squash( std::list<InstructionDependance> & dependances) {
    DBG_Assert(false);
    }
    virtual void applyToNext( boost::intrusive_ptr< Instruction > anInsn, boost::intrusive_ptr<Interaction> anInteraction) {
    DBG_Assert(false);
    }
    virtual void deferInteraction( boost::intrusive_ptr< Instruction > anInsn, boost::intrusive_ptr<Interaction> anInteraction) {
    DBG_Assert(false);
    }
    virtual bool squashAfter( boost::intrusive_ptr< Instruction > anInsn) {
    DBG_Assert(false);
    return false;
    }
    virtual void redirectFetch( VirtualMemoryAddress anAddress ) {
    DBG_Assert(false);
    }
    virtual void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB, eAccType type ) {
    DBG_Assert(false);
    }
    virtual void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB, InstructionDependance const & aDependance , eAccType type) {
    DBG_Assert(false);
    }
    virtual void eraseLSQ( boost::intrusive_ptr< Instruction > anInsn ) {
    DBG_Assert(false);
    }
    virtual void resolveVAddr( boost::intrusive_ptr< Instruction > anInsn, VirtualMemoryAddress theAddr ) {
    DBG_Assert(false);
    }
    virtual void updateStoreValue( boost::intrusive_ptr< Instruction > anInsn, bits aValue, boost::optional<uint64_t> anExtendedValue = boost::none ) {
    DBG_Assert(false);
    }
    virtual void annulStoreValue( boost::intrusive_ptr< Instruction > anInsn ) {
    DBG_Assert(false);
    }
    virtual void updateCASValue( boost::intrusive_ptr< Instruction > anInsn, bits aValue, bits aCMPValue ) {
    DBG_Assert(false);
    }
    virtual void retireMem( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    }
    virtual void checkTranslation( boost::intrusive_ptr<Instruction> anInsn) {
    DBG_Assert(false);
    }
    virtual void commitStore( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    }

    virtual uint32_t currentEL() {
        DBG_Assert(false);
        return 0;
    }

    virtual void invalidateCache(eCacheType aType, eShareableDomain aDomain, eCachePoint aPoint){
        DBG_Assert(false);
    }

    virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, eCachePoint aPoint){
        DBG_Assert(false);
    }

    virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize, eCachePoint aPoint){
        DBG_Assert(false);
    }

    virtual eAccessResult accessZVA(){
        DBG_Assert(false);
        return kACCESS_OK;
    }


    virtual uint32_t readDCZID_EL0(){
        DBG_Assert(false);
        return 0;
    }

    virtual SCTLR_EL _SCTLR(uint32_t anELn){
        DBG_Assert(false);
        return SCTLR_EL(0);
    }

//    virtual SysRegInfo* getSysRegInfo(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t CRn, uint8_t CRm, bool hasCP = false){
//        DBG_Assert(false);
//        return nullptr;
//    }

//    virtual void initSystemRegisters(std::multimap<int, SysRegInfo*> * aMap){
//        DBG_Assert(false);
//    }


    virtual uint32_t increaseEL(){
        DBG_Assert(false);
        return 0;
    }

    virtual uint32_t decreaseEL(){
    DBG_Assert(false);
    return 0;
    }


    virtual void accessMem( PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn ) {
    DBG_Assert(false);
    }
    virtual bits retrieveLoadValue( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return bits(0);
    }
    virtual bits retrieveExtendedLoadValue( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return bits(0);
    }
    virtual bool checkStoreRetirement( boost::intrusive_ptr<Instruction> aStore) {
    DBG_Assert(false);
    return true;
    }
    virtual uint32_t getRoundingMode()              { DBG_Assert(false); return 0; }

    virtual uint32_t getPSTATE()                    { DBG_Assert(false); return 0; }
    virtual void setPSTATE(uint32_t aPSTATE)        { DBG_Assert(false); }

    virtual uint32_t getFPSR()                      { DBG_Assert(false); return 0; }
    virtual void setFPSR(uint32_t aValue)           { DBG_Assert(false); }
    virtual uint32_t getFPCR()                      { DBG_Assert(false); return 0; }
    virtual void setFPCR(uint32_t aValue)           { DBG_Assert(false); }
    virtual uint32_t readFPCR()                     { DBG_Assert(false); return 0; }
    virtual void writeFPCR(uint32_t aValue)         { DBG_Assert(false); }

    virtual void setSCTLR_EL( uint64_t* aSCTLR)        { DBG_Assert(false); }
    virtual uint64_t* getSCTLR_EL()                     { DBG_Assert(false); return nullptr; }

    virtual void setHCREL2( uint64_t aSCTLR)        { DBG_Assert(false); }
    virtual uint64_t getHCREL2()                     { DBG_Assert(false); return 0; }

//    virtual uint32_t getDCZID_EL0();
//    virtual void setDCZID_EL0(uint32_t aDCZID_EL0);

    virtual bool isAARCH64(){ DBG_Assert(false); return false; }
    virtual void setAARCH64(bool aMode){ DBG_Assert(false);}

    virtual void setException( Flexus::Qemu::API::exception_t anEXP)  { DBG_Assert(false); }
    virtual Flexus::Qemu::API::exception_t getException()  { DBG_Assert(false); }

    virtual uint64_t getSP()            { DBG_Assert(false); return 0; }

    virtual uint64_t getXRegister(uint32_t aReg) { DBG_Assert(false); return 0; }
    virtual void setXRegister(uint32_t aReg, uint64_t aVal) { DBG_Assert(false); }

    virtual void writePR(uint32_t aPR, uint64_t aVal) { DBG_Assert(false); }
    virtual void updatePSTATEbits(uint64_t mask) { DBG_Assert(false); }
    virtual uint64_t readPR(uint32_t aPR) { DBG_Assert(false); return 0; }
    virtual std::string prName(uint32_t aPR) { DBG_Assert(false); return 0; }
    virtual void bypass(mapped_reg aReg, register_value aValue) { DBG_Assert(false); }
    virtual void connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst, std::function<bool(register_value)> ) {
    DBG_Assert(false);
    }
    virtual eConsistencyModel consistencyModel( ) const {
    DBG_Assert(false);
    return kSC;
    }
    virtual bool speculativeConsistency( ) const {
    DBG_Assert(false);
    return false;
    }
    virtual bool sbEmpty( ) const {
    DBG_Assert(false);
    return true;
    }
    virtual bool sbFull( ) const {
    DBG_Assert(false);
    return false;
    }
    virtual bool mayRetire_MEMBARStLd( ) const {
    DBG_Assert(false);
    return false;
    }
    virtual bool mayRetire_MEMBARStSt( ) const {
    DBG_Assert(false);
    return false;
    }
    virtual bool mayRetire_MEMBARSync( ) const {
    DBG_Assert(false);
    return false;
    }
    virtual void branchFeedback( boost::intrusive_ptr<BranchFeedback> feedback ) {
    DBG_Assert(false);
    }

};

static const PhysicalMemoryAddress kInvalid(0);
static const VirtualMemoryAddress kUnresolved(-1);

} //nuArchARM

namespace Flexus {
namespace SharedTypes {
typedef nuArchARM::Instruction Instruction;

using nuArchARM::kResynchronize;
using nuArchARM::kException;

} //Flexus
} //SharedTypes

#endif //FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED