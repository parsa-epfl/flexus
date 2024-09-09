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

#include "CoreModel/PSTATE.hpp"
#include "CoreModel/SCTLR_EL.hpp"
#include "RegisterType.hpp"
#include "components/CommonQEMU/Slices/AbstractInstruction.hpp"
#include "components/CommonQEMU/Slices/MemOp.hpp"
#include "components/CommonQEMU/Slices/TransactionTracker.hpp"
#include "components/uFetch/uFetchTypes.hpp"
#include "core/debug/debug.hpp"
#include "core/qemu/mai_api.hpp"
#include "core/target.hpp"
#include "core/types.hpp"

#include <bitset>
#include <boost/none.hpp>
#include <boost/variant.hpp>
#include <functional>
#include <list>

using namespace Flexus::SharedTypes;

namespace nuArch {

class SysRegInfo; // MARK: forward declare

struct SemanticAction
{
    virtual bool cancelled() const { return false; }
    virtual void create() {}
    // evaluate returns true if further actions should be stalled (for in-order
    // execution)
    virtual void evaluate() {}
    virtual void describe(std::ostream& anOstream) const {}
    virtual void satisfyDependants() {}
    virtual void squashDependants() {}
    virtual void addRef() {}
    virtual void releaseRef() {}
    virtual int64_t instructionNo() const { return 0; }
    virtual ~SemanticAction() {}
};

inline void
intrusive_ptr_add_ref(nuArch::SemanticAction* p)
{
    p->addRef();
}

inline void
intrusive_ptr_release(nuArch::SemanticAction* p)
{
    p->releaseRef();
}

} // namespace nuArch

namespace nuArch {

struct uArch;

using namespace Flexus::SharedTypes;

enum eResourceStatus
{
    kReady,
    kUnmapped,
    kNotReady,
    kLastStatus
};

std::ostream&
operator<<(std::ostream& anOstream, eResourceStatus aStatus);

inline std::ostream&
operator<<(std::ostream& anOstream, SemanticAction const& anAction)
{
    anAction.describe(anOstream);
    return anOstream;
}

struct uArch;
struct Interaction;

enum eConsistencyModel
{
    kSC,
    kTSO,
    kRMO
};

enum eMemoryAccess
{
    /* Used to indicate the type of accesses on which ordering
       is to be ensured.  Modeled after SPARC barriers.

       This is of the form MO_A_B where A is before B in program order.
    */
    kMO_LD_LD = 0x01,
    kMO_ST_LD = 0x02,
    kMO_LD_ST = 0x04,
    kMO_ST_ST = 0x08,
    kMO_ALL   = 0x0F, /* OR of the above */

    /* Used to indicate the kind of ordering which is to be ensured by the
       instruction.  These types are derived from x86/aarch64 instructions.
       It should be noted that these are different from C11 semantics.  */
    kBAR_LDAQ = 0x10, /* Following ops will not come forward */
    kBAR_STRL = 0x20, /* Previous ops will not be delayed */
    kBAR_SC   = 0x30, /* No ops cross barrier; OR of the above */
};

enum eFakeEffectCAS
{
    kAlwaysStore   = 0x0DEADBEEF,
    kNeverStore    = 0x1DEADBEEF,
    kCheckAndStore = 0x2DEADBEEF,
    kMaxStore      = 0x3DEADBEEF,
};

enum eExclusiveMonitorCode
{
    kMonitorSet         = 0,
    kMonitorUnset       = 1,
    kMonitorDoesntExist = 2,
};

enum eExceptionType
{
    kException_UNCATEGORIZED,        //= 0x00,
    kException_WFX_TRAP,             //= 0x01,
    kException_CP15RTTRAP,           //= 0x03,
    kException_CP15RRTTRAP,          //= 0x04,
    kException_CP14RTTRAP,           //= 0x05,
    kException_CP14DTTRAP,           //= 0x06,
    kException_ADVSIMDFPACCESSTRAP,  //= 0x07,
    kException_FPIDTRAP,             //= 0x08,
    kException_CP14RRTTRAP,          //= 0x0c,
    kException_ILLEGALSTATE,         //= 0x0e,
    kException_AA32_SVC,             //= 0x11,
    kException_AA32_HVC,             //= 0x12,
    kException_AA32_SMC,             //= 0x13,
    kException_AA64_SVC,             //= 0x15,
    kException_AA64_HVC,             //= 0x16,
    kException_AA64_SMC,             //= 0x17,
    kException_SYSTEMREGISTERTRAP,   //= 0x18,
    kException_INSNABORT,            //= 0x20,
    kException_INSNABORT_SAME_EL,    //= 0x21,
    kException_PCALIGNMENT,          //= 0x22,
    kException_DATAABORT,            //= 0x24,
    kException_DATAABORT_SAME_EL,    //= 0x25,
    kException_SPALIGNMENT,          //= 0x26,
    kException_AA32_FPTRAP,          //= 0x28,
    kException_AA64_FPTRAP,          //= 0x2c,
    kException_SERROR,               //= 0x2f,
    kException_BREAKPOINT,           //= 0x30,
    kException_BREAKPOINT_SAME_EL,   //= 0x31,
    kException_SOFTWARESTEP,         //= 0x32,
    kException_SOFTWARESTEP_SAME_EL, //= 0x33,
    kException_WATCHPOINT,           //= 0x34,
    kException_WATCHPOINT_SAME_EL,   //= 0x35,
    kException_AA32_BKPT,            //= 0x38,
    kException_VECTORCATCH,          //= 0x3a,
    kException_AA64_BKPT,            //= 0x3c,
    kException_IRQ,
    kException_None, //= 0xff,
};

std::ostream&
operator<<(std::ostream& anOstream, eExceptionType aCode);

enum ePrivRegs
{
    kPSTATE,
    kSCTLR_EL,
    kNZCV,
    kDAIF,
    kFPCR,
    kFPSR,
    kDCZID_EL0,
    kDC_ZVA,
    kCURRENT_EL,
    kELR_EL1,
    kSPSR_EL1,
    kSP_EL0,
    kSP_EL1,
    kSPSel,
    kSPSR_IRQ,
    kSPSR_ABT,
    kSPSR_UND,
    kSPSR_FIQ,
    kTPIDR_EL0,
    kAbstractSysReg, /* Msutherl: Blanket type for all registers to represent as hashed/encoded
                        5-tuple which are then read through QEMU */
    kLastPrivReg
};

/* Access rights:
 * We define bits for Read and Write access for what rev C of the v7-AR ARM ARM
 * defines as PL0 (user), PL1 (fiq/irq/svc/abt/und/sys, ie privileged), and
 * PL2 (hyp). The other level which has Read and Write bits is Secure PL1
 * (ie any of the privileged modes in Secure state, or Monitor mode).
 * If a register is accessible in one privilege level it's always accessible
 * in higher privilege levels too. Since "Secure PL1" also follows this rule
 * (ie anything visible in PL2 is visible in S-PL1, some things are only
 * visible in S-PL1) but "Secure PL1" is a bit of a mouthful, we bend the
 * terminology a little and call this PL3.
 * In AArch64 things are somewhat simpler as the PLx bits line up exactly
 * with the ELx exception levels.
 *
 * If access permissions for a register are more complex than can be
 * described with these bits, then use a laxer set of restrictions, and
 * do the more restrictive/complex check inside a helper function.
 */

enum eAccessRight
{
    kPL3_R  = 0x80,
    kPL3_W  = 0x40,
    kPL2_R  = (0x20 | kPL3_R),
    kPL2_W  = (0x10 | kPL3_W),
    kPL1_R  = (0x08 | kPL2_R),
    kPL1_W  = (0x04 | kPL2_W),
    kPL0_R  = (0x02 | kPL1_R),
    kPL0_W  = (0x01 | kPL1_W),
    kPL3_RW = (kPL3_R | kPL3_W),
    kPL2_RW = (kPL2_R | kPL2_W),
    kPL1_RW = (kPL1_R | kPL1_W),
    kPL0_RW = (kPL0_R | kPL0_W),
};

/* ARMCPRegInfo type field bits. If the SPECIAL bit is set this is a
 * special-behaviour cp reg and bits [15..8] indicate what behaviour
 * it has. Otherwise it is a simple cp reg, where CONST indicates that
 * TCG can assume the value to be constant (ie load at translate time)
 * and 64BIT indicates a 64 bit wide coprocessor register. SUPPRESS_TB_END
 * indicates that the TB should not be ended after a write to this register
 * (the default is that the TB ends after cp writes). OVERRIDE permits
 * a register definition to override a previous definition for the
 * same (cp, is64, crn,crm,opc1,opc2) tuple: either the new or the
 * old must have the OVERRIDE bit set.
 *
 * ALIAS indicates that this register is an alias view of some underlying
 * state which is also visible via another register, and that the other
 * register is handling migration and reset; registers marked ALIAS will not be
 * migrated but may have their state set by syncing of register state from KVM.
 * NO_RAW indicates that this register has no underlying state and does not
 * support raw access for state saving/loading; it will not be used for either
 * migration or KVM state synchronization. (Typically this is for "registers"
 * which are actually used as instructions for cache maintenance and so on.)
 * IO indicates that this register does I/O and therefore its accesses
 * need to be surrounded by gen_io_start()/gen_io_end(). In particular,
 * registers which implement clocks or timers require this.
 */

enum eRegInfo
{
    kARM_SPECIAL         = 1,
    kARM_CONST           = 2,
    kARM_64BIT           = 4,
    kARM_SUPPRESS_TB_END = 8,
    kARM_OVERRIDE        = 16,
    kARM_ALIAS           = 32,
    kARM_IO              = 64,
    kARM_NO_RAW          = 128,
    kARM_NOP             = (kARM_SPECIAL | (1 << 8)),
    kARM_WFI             = (kARM_SPECIAL | (2 << 8)),
    kARM_NZCV            = (kARM_SPECIAL | (3 << 8)),
    kARM_CURRENTEL       = (kARM_SPECIAL | (4 << 8)),
    kARM_DC_ZVA          = (kARM_SPECIAL | (5 << 8)),
    kARM_LAST_SPECIAL    = kARM_DC_ZVA,
    /* Mask of only the flag bits in a type field */
    kARM_FLAG_MASK = 0xff,
};

#define EL0 0
#define EL1 1
#define EL2 2
#define EL3 3

enum eAccessResult
{
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

    kACCESS_LAST,
};

enum eCacheType
{
    kInstructionCache,
    kDataCache,
};

enum eCachePoint
{
    kPoC, // Point of Coherency (PoC). For a particular address, the PoC is the
          // point at which all observers, for example, cores, DSPs, or DMA
          // engines, that can access memory, are guaranteed to see the same copy
          // of a memory location. Typically, this is the main external system
          // memory.

    kPoU, // Point of Unification (PoU). The PoU for a core is the point at which
          // the instruction and data caches and translation table walks of the
          // core are guaranteed to see the same copy of a memory location. For
          // example, a unified level 2 cache would be the point of unification in
          // a system with Harvard level 1 caches and a TLB for caching
          // translation table entries. If no external cache is present, main
          // memory would be the Point of Unification.
};

enum eShareableDomain
{
    kNonShareable, // This represents memory accessible only by a single processor
                   // or other agent, so memory accesses never need to be
                   // synchronized with other processors. This domain is not
                   // typically used in SMP systems.

    KInnerShareable, // This represents a shareability domain that can be shared
                     // by multiple processors, but not necessarily all of the
                     // agents in the system. A system might have multiple Inner
                     // Shareable domains. An operation that affects one Inner
                     // Shareable domain does not affect other Inner Shareable
                     // domains in the system. An example of such a domain might
                     // be a quad-core Cortex-A57 cluster.

    kOuterShareable, // An outer shareable (OSH) domain re-orderis shared by
                     // multiple agents and can consist of one or more inner
                     // shareable domains. An operation that affects an outer
                     // shareable domain also implicitly affects all inner
                     // shareable domains inside it. However, it does not
                     // otherwise behave as an inner shareable operation.

    kFullSystem, // An operation on the full system (SY) affects all observers in
                 // the system.
};

enum eInstructionClass
{
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
{
    codeBlackBox,
    codeNOP,
    codeMAGIC,
    // ALU
    codeALU,
    codeRev,
    codeMul,
    codeDiv,
    codeRDPR,
    codeWRPR,
    // FP
    codeFP,
    codeALIGN,
    // Memory
    codeLoad,
    codeLDEX,
    codeLDAQ,
    codeLDP,
    codeLoadFP,
    codeLDD,
    codeLDALU,
    codeStore,
    codeSTEX,
    codeSTP,
    codeSTRL,
    codeStoreFP,
    codeSTD,
    // Atomics
    codeCAS,
    codeCASP,
    // Branches
    codeBranchUnconditional,
    codeBranchConditional,
    codeBranchFPConditional,
    codeCALL,
    codeRETURN,
    codeBranchIndirectReg,
    codeBranchIndirectCall,
    // MEMBARs
    codeMEMBARSync,
    codeMEMBARStLd,
    codeMEMBARStSt,
    codeMEMBARLdSt,
    codeMEMBARLdLd,
    // Unsupported Instructions
    codeRDPRUnsupported,
    codeWRPRUnsupported,
    codeRETRYorDONE,
    // Codes that are not used by the decode component, but are used in
    // the instruction counter
    codeExceptionUnsupported,
    codeException,
    codeSideEffectLoad,
    codeSideEffectStore,
    codeSideEffectAtomic,
    codeDeviceAccess,
    codeMMUAccess,
    codeITLBMiss,
    codeCLREX,
    codeHaltedState,
    // End of Enum
    codeLastCode
};

typedef enum eMemOp
{
    kMemOp_LOAD,
    kMemOp_STORE,
    kMemOp_PREFETCH,
} eMemOp;

typedef enum eAccType
// Special cases
{
    kAccType_NORMAL,
    kAccType_VEC, // Normal loads and stores
    kAccType_STREAM,
    kAccType_VECSTREAM, // Streaming loads and stores
    kAccType_ATOMIC,
    kAccType_ATOMICRW, // Atomic loads and stores
    kAccType_ORDERED,
    kAccType_ORDEREDRW,      // Load-Acquire and Store-Release
    kAccType_LIMITEDORDERED, // Load-LOAcquire and Store-LORelease
    kAccType_UNPRIV,         // Load and store unprivileged
    kAccType_IFETCH,         // Instruction fetch
    kAccType_PTW,            // Page table walk
    // Other operations
    kAccType_DC,    // Data cache maintenance
    kAccType_IC,    // Instruction cache maintenance
    kAccType_DCZVA, // DC ZVA instructions
    kAccType_AT,    // Address translation
    kLastAccType,   // last one for debugging
} eAccType;

std::ostream&
operator<<(std::ostream& anOstream, eInstructionCode aCode);

// Options to the OoO core structure are added here.  Do not forget to
// copy from the Cfg object to this structure in uArchImpl.cpp
struct uArchOptions_t
{
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
    std::function<std::tuple<PhysicalMemoryAddress, bool, bool>(VirtualMemoryAddress, int32_t)> translate;
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

struct Instruction : public Flexus::SharedTypes::AbstractInstruction
{
    virtual void connectuArch(uArch& uArch) = 0;
    virtual void doDispatchEffects()        = 0; // used
    virtual void squash()                   = 0;
    virtual void pageFault(bool p = true)   = 0;
    virtual bool isPageFault() const        = 0;
    virtual void doRescheduleEffects()      = 0;
    virtual void doRetirementEffects()      = 0; // used
    virtual void checkTraps()               = 0; // used
    virtual void doCommitEffects()          = 0; // used

    virtual void annul()            = 0;
    virtual bool isAnnulled() const = 0;
    virtual bool isSquashed() const = 0;
    virtual void reinstate()        = 0; // reverse of annul

    //  virtual void setMMU(Flexus::Qemu::MMU::mmu_t m) = 0;
    //  virtual boost::optional<Flexus::Qemu::MMU::mmu_t> getMMU() const = 0;

    virtual eInstructionClass instClass() const   = 0;
    virtual std::string instClassName() const     = 0;
    virtual eInstructionCode instCode() const     = 0;
    virtual void changeInstCode(eInstructionCode) = 0;

    virtual void redirectPC(VirtualMemoryAddress aPC) = 0;

    virtual VirtualMemoryAddress pc() const                                             = 0;
    virtual VirtualMemoryAddress pcNext() const                                         = 0;
    virtual bool isPriv() const                                                         = 0;
    virtual void makePriv()                                                             = 0;
    virtual bool isTrap() const                                                         = 0;
    virtual bool preValidate()                                                          = 0;
    virtual bool advancesSimics() const                                                 = 0;
    virtual bool postValidate()                                                         = 0;
    virtual bool resync() const                                                         = 0;
    virtual void forceResync(bool r = true)                                             = 0;
    virtual void setClass(eInstructionClass anInstructionClass, eInstructionCode aCode) = 0;

    virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) = 0;
    virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const            = 0;
    virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction)    = 0;
    virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const               = 0;

    virtual bool mayRetire() const                        = 0;
    virtual bool mayCommit() const                        = 0;
    virtual bool isResolved() const                       = 0;
    virtual void setResolved(bool value = true)           = 0;
    virtual void raise(eExceptionType anException)        = 0;
    virtual eExceptionType willRaise() const              = 0;
    virtual void setWillRaise(eExceptionType anException) = 0;
    virtual int64_t sequenceNo() const                    = 0;
    virtual bool isComplete() const                       = 0;

    virtual void describe(std::ostream& anOstream) const = 0;

    virtual void overrideSimics()           = 0;
    virtual bool willOverrideSimics() const = 0;
    virtual bool isMicroOp() const          = 0;

    virtual void setAccessAddress(PhysicalMemoryAddress anAddress) = 0;
    virtual PhysicalMemoryAddress getAccessAddress() const         = 0;
    virtual void setMayCommit(bool aMayCommit)                     = 0;
    virtual void resolveSpeculation()                              = 0;
    virtual void restoreOriginalInstCode()                         = 0;

    virtual void setPreceedingInstruction(boost::intrusive_ptr<Instruction> aPredecessor) = 0;
    virtual bool hasExecuted() const                                                      = 0;
    //  virtual int32_t getASI() const = 0;
    //    virtual void setXRegister(uint32_t aReg, uint64_t aVal) = 0;
    //    virtual uint64_t getXRegister(uint32_t aReg) = 0;

    virtual bool hasCheckpoint() const        = 0;
    virtual void setHasCheckpoint(bool aCkpt) = 0;

    virtual void setRetireStallCycles(uint64_t aDelay) = 0;
    virtual uint64_t retireStallCycles() const         = 0;

    virtual void setCanRetireCounter(const uint32_t numCycles) = 0;
    virtual void decrementCanRetireCounter()                   = 0;
    virtual bool usesIntAlu() const                            = 0;
    virtual bool usesIntMult() const                           = 0;
    virtual bool usesIntDiv() const                            = 0;
    virtual bool usesFpAdd() const                             = 0;
    virtual bool usesFpCmp() const                             = 0;
    virtual bool usesFpCvt() const                             = 0;
    virtual bool usesFpMult() const                            = 0;
    virtual bool usesFpDiv() const                             = 0;
    virtual bool usesFpSqrt() const                            = 0;

    virtual void setUsesFpAdd()  = 0;
    virtual void setUsesFpCmp()  = 0;
    virtual void setUsesFpCvt()  = 0;
    virtual void setUsesFpMult() = 0;
    virtual void setUsesFpDiv()  = 0;
    virtual void setUsesFpSqrt() = 0;
};

struct InstructionDependance
{
    boost::intrusive_ptr<Instruction> instruction; // For lifetime control
    std::function<void()> squash;
    std::function<void()> satisfy;
};

struct Interaction : public boost::counted_base
{
    virtual void operator()(boost::intrusive_ptr<Instruction> anInstruction, nuArch::uArch& aCore)
    {
        DBG_Assert(false);
    }
    virtual void describe(std::ostream& anOstream) const { DBG_Assert(false); }

    virtual bool annuls() { return false; }
    virtual ~Interaction() {}
};

inline std::ostream&
operator<<(std::ostream& anOstream, Interaction const& anInteraction)
{
    anInteraction.describe(anOstream);
    return anOstream;
}

struct reg_
{
    eRegisterType theType;
    uint32_t theIndex;
    bool operator<(reg_ const& aRight) const
    {
        if (theType == aRight.theType) {
            return theIndex < aRight.theIndex;
        } else {
            return theType < aRight.theType;
        }
    }
    bool operator==(reg_ const& aRight) const { return (theType == aRight.theType) && (theIndex == aRight.theIndex); }
    friend std::ostream& operator<<(std::ostream& anOstream, reg_ const& aReg)
    {
        anOstream << aReg.theType << "[" << aReg.theIndex << "]";
        return anOstream;
    }
};
struct reg : public reg_
{};
struct mapped_reg : public reg_
{};

inline reg
xRegArch(uint32_t anIndex)
{
    reg ret_val;
    ret_val.theType  = xRegisters;
    ret_val.theIndex = anIndex;
    return ret_val;
}

inline reg
vRegArch(uint32_t anIndex)
{
    reg ret_val;
    ret_val.theType  = vRegisters;
    ret_val.theIndex = anIndex;
    return ret_val;
}

inline reg
ccRegArch(uint32_t anIndex)
{
    reg ret_val;
    ret_val.theType  = ccBits;
    ret_val.theIndex = anIndex;
    return ret_val;
}

inline mapped_reg
xReg(uint32_t anIndex)
{
    mapped_reg ret_val;
    ret_val.theType  = xRegisters;
    ret_val.theIndex = anIndex;
    return ret_val;
}
inline mapped_reg
vReg(uint32_t anIndex)
{
    mapped_reg ret_val;
    ret_val.theType  = vRegisters;
    ret_val.theIndex = anIndex;
    return ret_val;
}

inline mapped_reg
ccReg(uint32_t anIndex)
{
    mapped_reg ret_val;
    ret_val.theType  = ccBits;
    ret_val.theIndex = anIndex;
    return ret_val;
}

using register_value  = boost::variant<int64_t, uint64_t, bits> ;

struct uArch
{

    virtual ~uArch() {}
    virtual mapped_reg map(reg aReg)
    {
        DBG_Assert(false);
        return mapped_reg();
    }
    virtual std::pair<mapped_reg, mapped_reg> create(reg aReg)
    {
        DBG_Assert(false);
        return std::make_pair(mapped_reg(), mapped_reg());
    }
    virtual void free(mapped_reg aReg) { DBG_Assert(false); }
    virtual void restore(reg aName, mapped_reg aReg) { DBG_Assert(false); }
    virtual void create(boost::intrusive_ptr<SemanticAction> anAction) { DBG_Assert(false); }
    virtual void reschedule(boost::intrusive_ptr<SemanticAction> anAction) { DBG_Assert(false); }
    virtual eResourceStatus requestRegister(mapped_reg aRegister, InstructionDependance const& aDependance)
    {
        DBG_Assert(false);
        return kNotReady;
    }
    virtual eResourceStatus requestRegister(mapped_reg aRegister)
    {
        DBG_Assert(false);
        return kNotReady;
    }
    virtual register_value readRegister(mapped_reg aRegister)
    {
        DBG_Assert(false);
        return (uint64_t)0ULL;
    }
    virtual void squashRegister(mapped_reg aRegister) { DBG_Assert(false); }
    virtual void writeRegister(mapped_reg aRegister, register_value aValue, bool isW = false) { DBG_Assert(false); }
    virtual void copyRegValue(mapped_reg aSource, mapped_reg aDest) { DBG_Assert(false); }
    virtual void satisfy(InstructionDependance const& aDep) { DBG_Assert(false); }
    virtual void squash(InstructionDependance const& aDep) { DBG_Assert(false); }
    virtual void satisfy(std::list<InstructionDependance>& dependances) { DBG_Assert(false); }
    virtual void squash(std::list<InstructionDependance>& dependances) { DBG_Assert(false); }
    virtual void applyToNext(boost::intrusive_ptr<Instruction> anInsn, boost::intrusive_ptr<Interaction> anInteraction)
    {
        DBG_Assert(false);
    }
    virtual void deferInteraction(boost::intrusive_ptr<Instruction> anInsn,
                                  boost::intrusive_ptr<Interaction> anInteraction)
    {
        DBG_Assert(false);
    }
    virtual bool squashFrom(boost::intrusive_ptr<Instruction> anInsn)
    {
        DBG_Assert(false);
        return false;
    }
    virtual void redirectFetch(VirtualMemoryAddress anAddress) { DBG_Assert(false); }
    virtual void insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                           eOperation anOperation,
                           eSize aSize,
                           bool aBypassSB,
                           eAccType type)
    {
        DBG_Assert(false);
    }
    virtual void insertLSQ(boost::intrusive_ptr<Instruction> anInsn,
                           eOperation anOperation,
                           eSize aSize,
                           bool aBypassSB,
                           InstructionDependance const& aDependance,
                           eAccType type)
    {
        DBG_Assert(false);
    }
    virtual void eraseLSQ(boost::intrusive_ptr<Instruction> anInsn) { DBG_Assert(false); }
    virtual void resolveVAddr(boost::intrusive_ptr<Instruction> anInsn, VirtualMemoryAddress theAddr)
    {
        DBG_Assert(false);
    }
    virtual void translate(boost::intrusive_ptr<Instruction> anInsn) { DBG_Assert(false); }
    virtual void resolvePAddr(boost::intrusive_ptr<Instruction> anInsn, PhysicalMemoryAddress theAddr)
    {
        DBG_Assert(false);
    }
    virtual void updateStoreValue(boost::intrusive_ptr<Instruction> anInsn,
                                  bits aValue,
                                  boost::optional<bits> anExtendedValue = boost::none)
    {
        DBG_Assert(false);
    }
    virtual void annulStoreValue(boost::intrusive_ptr<Instruction> anInsn) { DBG_Assert(false); }
    virtual void updateCASValue(boost::intrusive_ptr<Instruction> anInsn, bits aValue, bits aCMPValue)
    {
        DBG_Assert(false);
    }
    virtual void retireMem(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) { DBG_Assert(false); }
    virtual void checkPageFault(boost::intrusive_ptr<Instruction> anInsn) { DBG_Assert(false); }
    virtual void commitStore(boost::intrusive_ptr<Instruction> aCorrespondingInstruction) { DBG_Assert(false); }
    virtual uint32_t currentEL()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void invalidateCache(eCacheType aType) { DBG_Assert(false); }
    virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress) { DBG_Assert(false); }
    virtual void invalidateCache(eCacheType aType, VirtualMemoryAddress anAddress, uint32_t aSize)
    {
        DBG_Assert(false);
    }
    virtual eAccessResult accessZVA()
    {
        DBG_Assert(false);
        return kACCESS_OK;
    }
    virtual uint32_t readDCZID_EL0()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void SystemRegisterTrap(uint32_t no) { DBG_Assert(false); }
    virtual bool _SECURE()
    {
        DBG_Assert(false);
        return false;
    }
    virtual void accessMem(PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn)
    {
        DBG_Assert(false);
    }
    virtual bits retrieveLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction)
    {
        DBG_Assert(false);
        return bits(0);
    }
    virtual void setLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction, bits aCorrespondingValue)
    {
        DBG_Assert(false);
    }
    virtual bits retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction> aCorrespondingInstruction)
    {
        DBG_Assert(false);
        return bits(0);
    }
    virtual bool checkStoreRetirement(boost::intrusive_ptr<Instruction> aStore)
    {
        DBG_Assert(false);
        return false;
    }
    virtual uint32_t getRoundingMode()
    {
        DBG_Assert(false);
        return 0;
    }

    virtual void setSP_el(uint8_t anEL, uint64_t aVal) { DBG_Assert(false); }
    virtual uint64_t getSP_el(uint8_t anEL)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setSPSR_el(uint8_t anEL, uint64_t aVal) { DBG_Assert(false); }
    virtual uint64_t getSPSR_el(uint8_t anEL)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setSPSel(uint32_t aVal) { DBG_Assert(false); }
    virtual uint32_t getSPSel()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual uint32_t getPSTATE()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setPSTATE(uint32_t aPSTATE) { DBG_Assert(false); }
    virtual uint64_t getTPIDR(uint8_t anEL)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual uint64_t getFPSR()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setFPSR(uint64_t aValue) { DBG_Assert(false); }
    virtual uint64_t getFPCR()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setFPCR(uint64_t aValue) { DBG_Assert(false); }
    virtual uint64_t readFPCR()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void writeFPCR(uint32_t aValue) { DBG_Assert(false); }

    virtual SCTLR_EL _SCTLR(uint32_t anELn)
    {
        DBG_Assert(false);
        return SCTLR_EL(0);
    }

    virtual void setSCTLR_EL(uint8_t anId, uint64_t aSCTLR) { DBG_Assert(false); }
    virtual uint64_t getSCTLR_EL(uint8_t anId)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setELR_el(uint8_t anEL, uint64_t aVal) { DBG_Assert(false); }
    virtual uint64_t getELR_el(uint8_t anEL)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setHCREL2(uint64_t aSCTLR) { DBG_Assert(false); }
    virtual uint64_t getHCREL2()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual uint32_t getDCZID_EL0()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual void setDCZID_EL0(uint32_t aDCZID_EL0) { DBG_Assert(false); }
    virtual bool isAARCH64()
    {
        DBG_Assert(false);
        return false;
    }
    virtual void setAARCH64(bool aMode) { DBG_Assert(false); }
    virtual void setException(Flexus::Qemu::API::exception_t anEXP) { DBG_Assert(false); }
    virtual void setDAIF(uint32_t aDAIF) { DBG_Assert(false); }
    virtual Flexus::Qemu::API::exception_t getException()
    {
        DBG_Assert(false);
        return Flexus::Qemu::API::exception_t();
    }
    virtual uint64_t getSP()
    {
        DBG_Assert(false);
        return 0;
    }
    virtual uint64_t getXRegister(uint32_t aReg)
    {
        DBG_Assert(false);
        return 0;
    }
    virtual PSTATE _PSTATE()
    {
        DBG_Assert(false);
        return PSTATE(0);
    }

    virtual void setXRegister(uint32_t aReg, uint64_t aVal) { DBG_Assert(false); }
    virtual void bypass(mapped_reg aReg, register_value aValue) { DBG_Assert(false); }
    virtual void connectBypass(mapped_reg aReg,
                               boost::intrusive_ptr<Instruction> inst,
                               std::function<bool(register_value)>)
    {
        DBG_Assert(false);
    }
    virtual eConsistencyModel consistencyModel() const
    {
        DBG_Assert(false);
        return kSC;
    }
    virtual bool speculativeConsistency() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool sbEmpty() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool sbFull() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool mayRetire_MEMBARStLd() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool mayRetire_MEMBARStSt() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool mayRetire_MEMBARSync() const
    {
        DBG_Assert(false);
        return false;
    }
    virtual void branchFeedback(boost::intrusive_ptr<BranchFeedback> feedback) { DBG_Assert(false); }
    virtual void takeTrap(boost::intrusive_ptr<Instruction> anInstruction, eExceptionType aTrapType)
    {
        DBG_Assert(false);
    }

    virtual void clearExclusiveLocal() { DBG_Assert(false); }
    virtual void clearExclusiveGlobal() { DBG_Assert(false); }

    virtual void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker)
    {
        DBG_Assert(false);
    }
    virtual void markExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker)
    {
        DBG_Assert(false);
    }
    virtual void markExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize, uint64_t marker) { DBG_Assert(false); }

    virtual int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        DBG_Assert(false);
        return false;
    }
    virtual int isExclusiveGlobal(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        DBG_Assert(false);
        return false;
    }
    virtual int isExclusiveVA(VirtualMemoryAddress anAddress, eSize aSize)
    {
        DBG_Assert(false);
        return false;
    }
    virtual bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction)
    {
        DBG_Assert(false);
        return false;
    }
    virtual uint64_t readUnhashedSysReg(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t crn, uint8_t crm)
    {
        DBG_Assert(false);
        return 0;
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
