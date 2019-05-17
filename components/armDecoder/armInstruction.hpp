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


#ifndef FLEXUS_armDECODER_armINSTRUCTION_HPP_INCLUDED
#define FLEXUS_armDECODER_armINSTRUCTION_HPP_INCLUDED

#include <sstream>
#include <list>

#include <components/CommonQEMU/Slices/FillLevel.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <components/uArchARM/coreModel.hpp>

namespace narmDecoder {

using Flexus::SharedTypes::Opcode;
using Flexus::SharedTypes::VirtualMemoryAddress;

using namespace nuArchARM;

std::pair< boost::intrusive_ptr<AbstractInstruction>, bool> decode( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop );


class armInstruction : public nuArchARM::Instruction {
protected:
  VirtualMemoryAddress thePC;
  std::vector<uint32_t> theInstruction;
  VirtualMemoryAddress thePCReg;
  Opcode theOpcode;
  boost::intrusive_ptr<BPredState> theBPState;
  uint32_t theCPU;
  int64_t theSequenceNo;
  uArchARM * theuArch;
  eExceptionType theRaisedException;
  bool theResync;
  eExceptionType theWillRaise;
  bool theAnnulled;
  bool theRetired;
  bool theSquashed;
  bool theExecuted;
  bool thePageFault;
  eInstructionClass theInstructionClass;
  eInstructionCode theInstructionCode;
  eInstructionCode theOriginalInstructionCode;
  boost::intrusive_ptr<TransactionTracker> theTransaction;
  boost::intrusive_ptr<TransactionTracker> thePrefetchTransaction;
  boost::intrusive_ptr< nuArchARM::Instruction >  thePredecessor;
//  int32_t theASI;
  bool theHaltDispatch;
  bool theHasCheckpoint;
  uint64_t theRetireStallCycles;
  bool theMayCommit;
  bool theResolved;
//  boost::optional<Flexus::Qemu::MMU::mmu_t> theMMU;

  bool theUsesIntAlu;
  bool theUsesIntMult;
  bool theUsesIntDiv;
  bool theUsesFpAdd;
  bool theUsesFpCmp;
  bool theUsesFpCvt;
  bool theUsesFpMult;
  bool theUsesFpDiv;
  bool theUsesFpSqrt;
  tFillLevel theInsnSourceLevel;
  bool thePriv;

public:

  virtual bool usesIntAlu() const;
  virtual bool usesIntMult() const;
  virtual bool usesIntDiv() const;
  virtual bool usesFpAdd() const;
  virtual bool usesFpCmp() const;
  virtual bool usesFpCvt() const;
  virtual bool usesFpMult() const;
  virtual bool usesFpDiv() const;
  virtual bool usesFpSqrt() const;

  virtual void setCanRetireCounter(const uint32_t numCycles) {}
  virtual void decrementCanRetireCounter() {}

  virtual void connectuArch(uArchARM & auArch) {
    theuArch = &auArch;
  };

  virtual void doDispatchEffects();
  virtual void squash() {}
  virtual void pageFault() {thePageFault = true;}
  virtual bool isPageFault() const {return thePageFault;}
  virtual void doRescheduleEffects() {}
  virtual void doRetirementEffects() {}
  virtual void checkTraps() {}
  virtual void doCommitEffects() {}
  virtual void annul() {
    theAnnulled = true;
  }
  virtual void reinstate() {
    theAnnulled = false;
  }

  virtual bool preValidate() {
    return true;
  }
  virtual bool advancesSimics() const {
    return true;
  }
  virtual bool postValidate() {
    return true;
  }

  virtual bool mayRetire() const {
    return false;
  }
  virtual void resolveSpeculation() {
    theMayCommit = true;
  }
  virtual void setMayCommit(bool aMayCommit) {
    theMayCommit = false;
  }
  virtual bool mayCommit() const {
    return theMayCommit;
  }

  virtual bool isResolved() const{
      return theResolved;
  }

  virtual void setResolved(){
      DBG_Assert(!theResolved);
      theResolved = true;
  }

  virtual eExceptionType willRaise() const {
    return theWillRaise;
  }
  virtual void setWillRaise(eExceptionType aSetting);

  virtual eExceptionType raised() {
    return theRaisedException;
  }
  virtual void raise(eExceptionType anException) {
    theRaisedException = anException;
  }
  virtual bool resync() const {
    return theResync;
  }
  virtual void forceResync() {
    theResync = true;
  }

  virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) {
    theTransaction = aTransaction;
  }
  virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const {
    return theTransaction;
  }
  virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) {
    thePrefetchTransaction = aTransaction;
  }
  virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const {
    return thePrefetchTransaction;
  }

  virtual bool willOverrideSimics() const {
    return false;
  }

  virtual void describe(std::ostream & anOstream) const;
  virtual std::string disassemble() const;
  virtual void overrideSimics() {}
  virtual int64_t sequenceNo() const {
    return theSequenceNo;
  }
  virtual bool isAnnulled() const {
    return theAnnulled;
  }
  bool isSquashed() const {
    return theSquashed;
  }
  bool isRetired() const {
    return theRetired;
  }
  virtual bool isComplete() const {
    return isRetired() || isSquashed();
  }

//  virtual void setMMU(Flexus::Qemu::MMU::mmu_t m) {
//    theMMU = m;
//  }
//  virtual boost::optional<Flexus::Qemu::MMU::mmu_t> getMMU() const {
//    return theMMU;
//  }


  std::string printInstClass() const {
    switch (theInstructionClass)
    {
    case clsLoad:
        return " {clsLoad} ";
    case clsStore:
        return " {clsStore} ";
    case clsAtomic:
        return " {clsAtomic} ";
    case clsBranch:
        return " {clsBranch} ";
    case clsMEMBAR:
        return " {clsMEMBAR} ";
    case clsComputation:
        return " {clsComputation} ";
    case clsSynchronizing:
        return "{clsSynchronizing}";
    default:
        return "";
    }

  }

  virtual eInstructionClass instClass() const {
    return theInstructionClass;
  }

  virtual std::string instClassName() const {
      switch (theInstructionClass) {
      case clsLoad:
          return "clsLoad";

      case clsStore:
          return "clsStore";

      case clsAtomic:
          return "clsAtomic";

      case clsBranch:
          return "clsBranch";

      case clsMEMBAR:
          return "clsMEMBAR";

      case clsComputation:
          return "clsComputation";

      case clsSynchronizing:
          return "clsSynchronizing";

      default:
          assert(false);
          break;
      }
  }

  virtual eInstructionCode instCode() const {
    return theInstructionCode;
  }
  virtual eInstructionCode originalInstCode() const {
    return theOriginalInstructionCode;
  }

  virtual void restoreOriginalInstCode() {
    DBG_ ( Trace, ( << "Restoring instruction code from " << theInstructionCode
                    << " to " << theOriginalInstructionCode << ": " << *this ) );
    theInstructionCode = theOriginalInstructionCode;
  }

  virtual void changeInstCode(eInstructionCode aCode) {
    theInstructionCode = aCode ;
  }

  void setClass( eInstructionClass anInstructionClass, eInstructionCode aCode) {
    theInstructionClass = anInstructionClass;
    theOriginalInstructionCode = theInstructionCode = aCode;
    DECODER_DBG(*this);

  }

//  Opcode opcode() const ;
//  void setASI( int32_t anASI) {
//    theASI = anASI;
//  }
//  int32_t getASI() const {
//    return theASI;
//  }

  uint32_t cpu() {
    return theCPU;
  }
  virtual bool isMicroOp() const {
    return false;
  }

  std::string identify() const {
    std::stringstream id;
    id << "CPU[" << std::setfill('0') << std::setw(2) << theCPU << "]#" << theSequenceNo;
    return id.str();
  }
  virtual ~armInstruction() {
    DBG_( VVerb, ( << identify() << " destroyed") );
  }

  virtual bool redirectPC(VirtualMemoryAddress anPCReg) {
    bool ret_val = (anPCReg != thePCReg);
    thePCReg = anPCReg;
    return ret_val;
  }

  virtual VirtualMemoryAddress pc() const {
    return thePC;
  }

  virtual VirtualMemoryAddress pcOrig() const {
    return (thePCReg == thePC) ? (thePC + 4) : thePCReg;
  }

  virtual bool isPriv() const {
    return thePriv;
  }
  virtual void makePriv() {
    thePriv = true;
  }

  virtual bool isTrap() const {
    return theRaisedException != kException_None;
  }
  virtual boost::intrusive_ptr<BPredState> bpState() const {
    return theBPState;
  }
  bool isBranch() const {
    return theInstructionClass == clsBranch;
  }
  virtual void setAccessAddress(PhysicalMemoryAddress anAddress) { }
  virtual PhysicalMemoryAddress getAccessAddress() const {
    return PhysicalMemoryAddress(0) ;
  }

  virtual void setPreceedingInstruction(boost::intrusive_ptr< Instruction > aPredecessor) {
    thePredecessor = aPredecessor;
  }
  virtual bool hasExecuted() const {
    return theExecuted;
  }
  void setExecuted(bool aVal) {
    theExecuted = aVal;
  }
  bool hasPredecessorExecuted() {
    if (thePredecessor) {
      return thePredecessor->hasExecuted();
    } else {
      return true;
    }
  }

  uArchARM * core() {
    return theuArch;
  }

  bool haltDispatch() const {
    return theHaltDispatch;
  }
  void setHaltDispatch() {
    theHaltDispatch = true;
  }

  bool hasCheckpoint() const {
    return theHasCheckpoint;
  }
  void setHasCheckpoint(bool aCkpt) {
    theHasCheckpoint = aCkpt;
  }

  void setRetireStallCycles(uint64_t aDelay) {
    theRetireStallCycles = aDelay;
  }
  uint64_t retireStallCycles() const {
    return theRetireStallCycles;
  }

  void setSourceLevel(tFillLevel aLevel) {
    theInsnSourceLevel = aLevel;
  }
  tFillLevel sourceLevel() const {
    return theInsnSourceLevel;
  }

protected:
  armInstruction(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> bp_state, uint32_t aCPU, int64_t aSequenceNo)
    : thePC(aPC)
    , thePCReg(aPC)
    , theOpcode(anOpcode)
    , theBPState(bp_state)
    , theCPU(aCPU)
    , theSequenceNo(aSequenceNo)
    , theuArch(0)
    , theRaisedException(kException_None)
    , theResync(false)
    , theWillRaise(kException_None)
    , theAnnulled(false)
    , theRetired(false)
    , theSquashed(false)
    , theExecuted(true)
    , thePageFault(false)
    , theInstructionClass(clsSynchronizing)
    , theHaltDispatch(false)
    , theHasCheckpoint(false)
    , theRetireStallCycles(0)
    , theMayCommit(true)
    , theResolved(false)
    , theInsnSourceLevel(eL1I)
    , thePriv(false)
    , theUsesIntAlu(true)
    , theUsesIntMult(false)
    , theUsesIntDiv(false)
    , theUsesFpAdd(false)
    , theUsesFpCmp(false)
    , theUsesFpCvt(false)
    , theUsesFpMult(false)
    , theUsesFpDiv(false)
    , theUsesFpSqrt(false)
  {
  }

  // So that armDecoder can send opcodes out to PowerTracker
public:
  Opcode getOpcode() {
    return theOpcode;
  }
};

typedef boost::intrusive_ptr<armInstruction>  arminst;
typedef Flexus::SharedTypes::FetchedOpcode armcode;

} //armDecoder

#endif //FLEXUS_armDECODER_armINSTRUCTION_HPP_INCLUDED
