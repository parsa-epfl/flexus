#ifndef FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED
#define FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED

#include <bitset>
#include <list>

#include <boost/function.hpp>
#include <core/debug/debug.hpp>
#include <core/metaprogram.hpp>
#include <boost/variant.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <boost/none.hpp>

#include "RegisterType.hpp"
#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <core/simics/sparcmmu.hpp>

namespace Flexus {
namespace SharedTypes {
struct BranchFeedback;
}
}

namespace nuArch {

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
} //nuArch

namespace boost {
inline void intrusive_ptr_add_ref(nuArch::SemanticAction * p) {
  p->addRef();
}

inline void intrusive_ptr_release(nuArch::SemanticAction * p) {
  p->releaseRef();
}
} //boost

namespace nuArch {

struct uArch;
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

struct uArch;
struct Interaction;

enum eConsistencyModel {
  kSC
  , kTSO
  , kRMO
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
  boost::function< boost::tuple< PhysicalMemoryAddress, bool, bool> ( VirtualMemoryAddress, int32_t ) >
  translate;
  boost::function< PhysicalMemoryAddress ( VirtualMemoryAddress ) >
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
  virtual void connectuArch(uArch & uArch) = 0;
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

  virtual void setMMU(Flexus::Simics::MMU::mmu_t m) = 0;
  virtual boost::optional<Flexus::Simics::MMU::mmu_t> getMMU() const = 0;

  virtual eInstructionClass instClass() const = 0;
  virtual eInstructionCode instCode() const = 0;
  virtual void changeInstCode(eInstructionCode ) = 0;

  virtual bool redirectNPC(VirtualMemoryAddress anNPC, boost::optional<VirtualMemoryAddress> anNPCReg = boost::none) = 0; //returns true if this actually changed the NPC of the instruction
  virtual VirtualMemoryAddress pc() const = 0;
  virtual VirtualMemoryAddress npc() const = 0;
  virtual VirtualMemoryAddress npcReg() const = 0;
  virtual bool isPriv() const = 0;
  virtual bool isTrap() const = 0;

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
  virtual int32_t getASI() const = 0;

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
  boost::function < void() > squash;
  boost::function < void() > satisfy;
};

struct Interaction : public boost::counted_base {
  virtual void operator() (boost::intrusive_ptr<Instruction> anInstruction, nuArch::uArch & aCore) {
    DBG_Assert(false);
  }
  virtual void describe(std::ostream & anOstream) const {
    DBG_Assert(false);
  }
  virtual boost::optional< uint64_t> npc() {
    return boost::none;
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
struct unmapped_reg : public reg_ {};
struct mapped_reg : public reg_ {};

inline mapped_reg rReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = rRegisters;
  ret_val.theIndex = anIndex;
  return ret_val;
}

inline mapped_reg fReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = fRegisters;
  ret_val.theIndex = anIndex;
  return ret_val;
}

inline mapped_reg ccReg(uint32_t anIndex) {
  mapped_reg ret_val;
  ret_val.theType = ccBits;
  ret_val.theIndex = anIndex;
  return ret_val;
}

typedef boost::variant< uint64_t , std::bitset<8> > register_value;

struct uArch {

  virtual ~uArch() {}
  virtual mapped_reg map( unmapped_reg aReg ) {
    DBG_Assert( false );
    return mapped_reg();
  }
  virtual std::pair<mapped_reg, mapped_reg> create( unmapped_reg aReg ) {
    DBG_Assert( false );
    return std::make_pair(mapped_reg(), mapped_reg());
  }
  virtual void free( mapped_reg aReg ) {
    DBG_Assert( false );
  }
  virtual void restore( unmapped_reg aName, mapped_reg aReg ) {
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
  //virtual void disconnectRegister( mapped_reg aReg, boost::intrusive_ptr< Instruction > inst) { DBG_Assert( false ); }

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

  virtual void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB ) {
    DBG_Assert(false);
  }
  virtual void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB, InstructionDependance const & aDependance ) {
    DBG_Assert(false);
  }
  virtual void eraseLSQ( boost::intrusive_ptr< Instruction > anInsn ) {
    DBG_Assert(false);
  }
  virtual void resolveVAddr( boost::intrusive_ptr< Instruction > anInsn, VirtualMemoryAddress theAddr, int32_t theASI ) {
    DBG_Assert(false);
  }
  virtual void updateStoreValue( boost::intrusive_ptr< Instruction > anInsn, uint64_t aValue, boost::optional<uint64_t> anExtendedValue = boost::none ) {
    DBG_Assert(false);
  }
  virtual void annulStoreValue( boost::intrusive_ptr< Instruction > anInsn ) {
    DBG_Assert(false);
  }
  virtual void updateCASValue( boost::intrusive_ptr< Instruction > anInsn, uint64_t aValue, uint64_t aCMPValue ) {
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
  virtual void accessMem( PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn ) {
    DBG_Assert(false);
  }
  virtual uint64_t retrieveLoadValue( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t retrieveExtendedLoadValue( boost::intrusive_ptr<Instruction> aCorrespondingInstruction) {
    DBG_Assert(false);
    return 0;
  }
  virtual bool checkStoreRetirement( boost::intrusive_ptr<Instruction> aStore) {
    DBG_Assert(false);
    return true;
  }

  virtual void takeTrap(boost::intrusive_ptr<Instruction> anInstruction, int32_t aTrapNum) {
    DBG_Assert(false);
  }
  virtual void popTL() {
    DBG_Assert(false);
  }

  virtual uint32_t saveWindow() {
    DBG_Assert(false);
    return 0;
  }
  virtual void saveWindowPriv() {
    DBG_Assert(false);
  }
  virtual void saved() {
    DBG_Assert(false);
  }
  virtual uint32_t restoreWindow() {
    DBG_Assert(false);
    return 0;
  }
  virtual void restoreWindowPriv() {
    DBG_Assert(false);
  }
  virtual void restored() {
    DBG_Assert(false);
  }

  virtual uint32_t getRoundingMode() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getTPC(uint32_t aTL) {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getTNPC(uint32_t aTL) {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getTSTATE(uint32_t aTL) {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getTT(uint32_t aTL) {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getTBA() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getPSTATE() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getFPRS() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getTL() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getPIL() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getCWP() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getCLEANWIN() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getOTHERWIN() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getWSTATE() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getCANSAVE() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint32_t getCANRESTORE() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getVER() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getTICK() {
    DBG_Assert(false);
    return 0;
  }
  virtual uint64_t getSTICK() {
    DBG_Assert(false);
    return 0;
  }

  virtual void setFPRS(uint64_t aValue) {
    DBG_Assert(false);
  }

  virtual uint64_t readFSR() {
    DBG_Assert(false);
    return 0;
  }
  virtual void writeFSR(uint64_t aValue) {
    DBG_Assert(false);
  }

  virtual void writePR(uint32_t aPR, uint64_t aVal) {
    DBG_Assert(false);
  }
  virtual uint64_t readPR(uint32_t aPR) {
    DBG_Assert(false);
    return 0;
  }
  virtual std::string prName(uint32_t aPR) {
    DBG_Assert(false);
    return "";
  }

  virtual void bypass(mapped_reg aReg, register_value aValue) {
    DBG_Assert(false);
  }
  virtual void connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst, boost::function<bool(register_value)> ) {
    DBG_Assert(false);
  }
  //virtual void disconnectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst) { DBG_Assert(false); }

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

} //nuArch

namespace Flexus {
namespace SharedTypes {
typedef nuArch::Instruction Instruction;

using nuArch::kResynchronize;
using nuArch::kException;

} //Flexus
} //SharedTypes

#endif //FLEXUS_uARCH_uARCHINTERFACES_HPP_INCLUDED
