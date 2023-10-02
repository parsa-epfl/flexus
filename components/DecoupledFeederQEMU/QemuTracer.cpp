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
// FIXME: note commented out all uses of WhiteBox

#include <set>

// For debug only
#include <iostream>

#include <core/boost_extensions/padded_string_cast.hpp>

#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <core/qemu/api_wrappers.hpp>
#include <core/qemu/configuration_api.hpp>

#include <components/DecoupledFeederQEMU/QemuTracer.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include <core/stats.hpp>
namespace Stat = Flexus::Stat;
// was only in namespace nDecoupledFeeder

namespace nDecoupledFeeder {

using namespace Flexus::SharedTypes;

namespace Qemu = Flexus::Qemu;
namespace API = Qemu::API;

std::set<API::conf_object_t *> theTimingModels;
const uint32_t kLargeMemoryQueue = 16000;

struct TracerStats {
  Stat::StatCounter theMemOps_stat;
  Stat::StatCounter theIOOps_stat;
  Stat::StatCounter theUncacheableOps_stat;
  Stat::StatCounter theFetches_stat;
  Stat::StatCounter theLoadExOps_stat;
  Stat::StatCounter theStoreExOps_stat;
  Stat::StatCounter theRMWOps_stat;
  Stat::StatCounter theCASOps_stat;
  Stat::StatCounter theLoadOps_stat;
  Stat::StatCounter theStoreOps_stat;
  Stat::StatCounter thePrefetchOps_stat;

  int64_t theIOOps;
  int64_t theUncacheableOps;
  int64_t theFetches;
  int64_t theLoadExOps;
  int64_t theStoreExOps;
  int64_t theRMWOps;
  int64_t theCASOps;
  int64_t theLoadOps;
  int64_t theStoreOps;
  int64_t thePrefetchOps;

  TracerStats(std::string const &aName)
      : theMemOps_stat(aName + "MemOps"), theIOOps_stat(aName + "IOOps"),
        theUncacheableOps_stat(aName + "UncacheableOps"), theFetches_stat(aName + "Fetches"),
        theLoadExOps_stat(aName + "LoadExOps"), theStoreExOps_stat(aName + "StoreExOps"),
        theRMWOps_stat(aName + "RMWOps"), theCASOps_stat(aName + "CASOps"),
        theLoadOps_stat(aName + "LoadOps"), theStoreOps_stat(aName + "StoreOps"),
        thePrefetchOps_stat(aName + "PrefetchOps")

        ,
        theIOOps(0), theUncacheableOps(0), theFetches(0), theLoadExOps(0), theStoreExOps(0),
        theRMWOps(0), theCASOps(0), theLoadOps(0), theStoreOps(0), thePrefetchOps(0) {
  }

  void update() {
    theMemOps_stat += theLoadOps + theStoreOps + theCASOps + theLoadExOps + theStoreExOps +
                      theRMWOps + thePrefetchOps;
    theIOOps_stat += theIOOps;
    theUncacheableOps_stat += theUncacheableOps;
    theFetches_stat += theFetches;
    theLoadExOps_stat += theLoadExOps;
    theStoreExOps_stat += theStoreExOps;
    theRMWOps_stat += theRMWOps;
    theCASOps_stat += theCASOps;
    theLoadOps_stat += theLoadOps;
    theStoreOps_stat += theStoreOps;
    thePrefetchOps_stat += thePrefetchOps;
    theIOOps = 0;
    theUncacheableOps = 0;
    theFetches = 0;
    theLoadExOps = 0;
    theStoreExOps = 0;
    theRMWOps = 0;
    theCASOps = 0;
    theLoadOps = 0;
    theStoreOps = 0;
    thePrefetchOps = 0;
  }
};

#define IS_PRIV(mem_trans) (!mem_trans->arm_specific.user)

class QemuTracerImpl {
  API::conf_object_t *theUnderlyingObject;
  API::conf_object_t *theCPU;
  int32_t theIndex;
  API::conf_object_t *thePhysMemory;
  API::conf_object_t *thePhysIO;

  TracerStats *theUserStats;
  TracerStats *theOSStats;
  TracerStats *theBothStats;
  MemoryMessage theMemoryMessage;

  uint64_t instr_num;

public:
  std::function<void(int, MemoryMessage &)> toL1D;
  std::function<void(int, MemoryMessage &)> toL1I;
  std::function<void(int, MemoryMessage &)> toNAW;

  //  bool theWhiteBoxDebug;
  //  int32_t  theWhiteBoxPeriod;
  bool theSendNonAllocatingStores;

public:
  QemuTracerImpl(API::conf_object_t *anUnderlyingObject)
      : theUnderlyingObject(anUnderlyingObject), theMemoryMessage(MemoryMessage::LoadReq) {
  }

  API::conf_object_t *cpu() const {
    return theCPU;
  }

  // Initialize the tracer to the desired CPU
  void init(API::conf_object_t *aCPU, index_t anIndex,
            std::function<void(int, MemoryMessage &)> aToL1D,
            std::function<void(int, MemoryMessage &)> aToL1I,
            std::function<void(int, MemoryMessage &)> aToNAW,
            //          , bool aWhiteBoxDebug
            //        , int32_t aWhiteBoxPeriod
            bool aSendNonAllocatingStores

  ) {
    theCPU = aCPU;
    theIndex = anIndex;
    instr_num = 0;
    toL1D = aToL1D;
    toL1I = aToL1I;
    toNAW = aToNAW;
    //   theWhiteBoxDebug = false;
    //   theWhiteBoxPeriod = aWhiteBoxPeriod;
    theSendNonAllocatingStores = aSendNonAllocatingStores;
    theUserStats = new TracerStats(boost::padded_string_cast<2, '0'>(theIndex) + "-feeder-User:");
    theOSStats = new TracerStats(boost::padded_string_cast<2, '0'>(theIndex) + "-feeder-OS:");
    theBothStats = new TracerStats(boost::padded_string_cast<2, '0'>(theIndex) + "-feeder-");
    thePhysIO = 0;
  }

  void updateStats() {
    theUserStats->update();
    theOSStats->update();
    theBothStats->update();
  }

  API::cycles_t insn_fetch(Qemu::API::memory_transaction_t *mem_trans) {
    instr_num++;
    const int32_t k_no_stall = 0;
    theMemoryMessage.address() = PhysicalMemoryAddress(mem_trans->s.physical_address);
    theMemoryMessage.pc() = VirtualMemoryAddress(mem_trans->s.pc);
    theMemoryMessage.targetpc() = VirtualMemoryAddress(mem_trans->s.logical_address);
    theMemoryMessage.opcode() = mem_trans->s.opcode;
    theMemoryMessage.type() = MemoryMessage::FetchReq;
    theMemoryMessage.priv() = IS_PRIV(mem_trans);
    theMemoryMessage.reqSize() = 4;
    theMemoryMessage.timeStamp() = instr_num;

    eBranchType branchTypeTable[API::QEMU_BRANCH_TYPE_COUNT] = {
        kNonBranch,   kConditional,  kUnconditional, kCall,
        kIndirectReg, kIndirectCall, kReturn,kLastBranchType};
    theMemoryMessage.branchType() = branchTypeTable[mem_trans->s.branch_type];
    theMemoryMessage.branchAnnul() = (mem_trans->s.annul != 0);

    theMemoryMessage.tl() = 0;

    IS_PRIV(mem_trans) ? theOSStats->theFetches++ : theUserStats->theFetches++;
    theBothStats->theFetches++;

    // TODO FIXME
    DBG_(VVerb, (<< "sending fetch[" << theIndex << "] op: " << theMemoryMessage.opcode()
                 << " message: " << theMemoryMessage));
    toL1I(theIndex, theMemoryMessage);
    return k_no_stall; // Never stalls
  }

  API::cycles_t trace_mem_hier_operate(API::memory_transaction_t *mem_trans) {
    const int32_t k_no_stall = 0;
    if (mem_trans->io) {
      // Count data accesses
      IS_PRIV(mem_trans) ? theOSStats->theIOOps++ : theUserStats->theIOOps++;
      theBothStats->theIOOps++;
      return k_no_stall; // Not a memory operation
    }

    if (mem_trans->s.type == API::QEMU_Trans_Instr_Fetch) {
      return insn_fetch(mem_trans);
    }

    // Minimal memory message implementation
    theMemoryMessage.address() = PhysicalMemoryAddress(mem_trans->s.physical_address);
    theMemoryMessage.pc() = VirtualMemoryAddress(mem_trans->s.logical_address);
    theMemoryMessage.priv() = IS_PRIV(mem_trans);
    theMemoryMessage.coreIdx() = theIndex;

    // Set the type field of the memory operation
    if (mem_trans->s.atomic) {
      theMemoryMessage.type() = MemoryMessage::RMWReq;
      switch (mem_trans->s.type) {
      case API::QEMU_Trans_Load:
        IS_PRIV(mem_trans) ? theOSStats->theLoadExOps++ : theUserStats->theLoadExOps++;
        theBothStats->theLoadExOps++;
        break;
      case API::QEMU_Trans_Store:
        IS_PRIV(mem_trans) ? theOSStats->theStoreExOps++ : theUserStats->theStoreExOps++;
        theBothStats->theStoreExOps++;
        break;
      default:
        DBG_(Crit, (<< "unhandled transaction type.  Transaction follows:"));
        debugTransaction(mem_trans);
        DBG_Assert(false);
        break;
      }
    } else {
      switch (mem_trans->s.type) {
      case API::QEMU_Trans_Load:
        theMemoryMessage.type() = MemoryMessage::LoadReq;
        if (IS_PRIV(mem_trans)) {
          theOSStats->theLoadOps++;
        } else {
          theUserStats->theLoadOps++;
        }
        theBothStats->theLoadOps++;
        break;
      case API::QEMU_Trans_Store:
        theMemoryMessage.type() = MemoryMessage::StoreReq;
        IS_PRIV(mem_trans) ? theOSStats->theStoreOps++ : theUserStats->theStoreOps++;
        theBothStats->theStoreOps++;
        break;
      case API::QEMU_Trans_Prefetch:
        // Prefetches are currently mapped as loads
        theMemoryMessage.type() = MemoryMessage::LoadReq;
        IS_PRIV(mem_trans) ? theOSStats->thePrefetchOps++ : theUserStats->thePrefetchOps++;
        theBothStats->thePrefetchOps++;
        break;
      case API::QEMU_Trans_Instr_Fetch:
        DBG_Assert(false);
      case API::QEMU_Trans_Cache:
        // We don't really support these operations
        return k_no_stall;
      default:
        DBG_(Crit, (<< "unhandled transaction type.  Transaction follows:"));
        debugTransaction(mem_trans);
        DBG_Assert(false);
        break;
      }
    }

    toL1D(theIndex, theMemoryMessage);
    if (theIndex != 0) {
      // std::cout<<"index : "<<theIndex<<std::endl;
    }
    return k_no_stall; // Never stalls
  }

  // Useful debugging stuff for tracing every instruction
  void debugTransaction(Qemu::API::memory_transaction_t *mem_trans) {
    API::logical_address_t pc_logical = API::qemu_callbacks.QEMU_get_program_counter(theCPU);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    API::physical_address_t pc =
        API::qemu_callbacks.QEMU_logical_to_physical(theCPU, API::QEMU_DI_Instruction, pc_logical);
    unsigned opcode = 0;

    DBG_(VVerb, SetNumeric((ScaffoldIdx)theIndex)(<< "Mem Hier Instr: " << opcode << " logical pc: "
                                                  << &std::hex << pc_logical << " pc: " << pc));
#pragma GCC diagnostic pop

    if (API::qemu_callbacks.QEMU_mem_op_is_data(&mem_trans->s)) {
      if (API::qemu_callbacks.QEMU_mem_op_is_write(&mem_trans->s)) {
        DBG_(VVerb,
             SetNumeric((ScaffoldIdx)theIndex)(
                 << "  Write v@" << &std::hex << mem_trans->s.logical_address << " p@"
                 << mem_trans->s.physical_address << &std::dec << " size=" << mem_trans->s.size
                 << " type=" << mem_trans->s.type << (mem_trans->s.atomic ? " atomic" : "")
                 << (mem_trans->s.may_stall ? "" : " no-stall")
                 << (mem_trans->s.inquiry ? " inquiry" : "")
                 << (mem_trans->s.speculative ? " speculative" : "")
                 << (mem_trans->s.ignore ? " ignore" : "")
                 << (IS_PRIV(mem_trans) ? " priv" : " user")));
      } else {
        if (mem_trans->s.type == API::QEMU_Trans_Prefetch) {
          DBG_(VVerb,
               SetNumeric((ScaffoldIdx)theIndex)(
                   << "  Prefetch v@" << &std::hex << mem_trans->s.logical_address << " p@"
                   << mem_trans->s.physical_address << &std::dec << " size=" << mem_trans->s.size
                   << " type=" << mem_trans->s.type << (mem_trans->s.atomic ? " atomic" : "")
                   << (mem_trans->s.may_stall ? "" : " no-stall")
                   << (mem_trans->s.inquiry ? " inquiry" : "")
                   << (mem_trans->s.speculative ? " speculative" : "")
                   << (mem_trans->s.ignore ? " ignore" : "")
                   << (IS_PRIV(mem_trans) ? " priv" : " user")));
        } else {
          DBG_(VVerb,
               SetNumeric((ScaffoldIdx)theIndex)(
                   << "  Read v@" << &std::hex << mem_trans->s.logical_address << " p@"
                   << mem_trans->s.physical_address << &std::dec << " size=" << mem_trans->s.size
                   << " type=" << mem_trans->s.type << (mem_trans->s.atomic ? " atomic" : "")
                   << (mem_trans->s.may_stall ? "" : " no-stall")
                   << (mem_trans->s.inquiry ? " inquiry" : "")
                   << (mem_trans->s.speculative ? " speculative" : "")
                   << (mem_trans->s.ignore ? " ignore" : "")
                   << (IS_PRIV(mem_trans) ? " priv" : " user")));
        }
      }
    }
  }

}; // class QemuTracerImpl

class QemuTracer : public Qemu::AddInObject<QemuTracerImpl> {
  typedef Qemu::AddInObject<QemuTracerImpl> base;

public:
  static const Qemu::Persistence class_persistence = Qemu::Session;
  static std::string className() {
    return "DecoupledFeeder";
  }
  static std::string classDescription() {
    return "Flexus's decoupled feeder.";
  }

  QemuTracer() : base() {
  }
  QemuTracer(Qemu::API::conf_object_t *aQemuObject) : base(aQemuObject) {
  }
  QemuTracer(QemuTracerImpl *anImpl) : base(anImpl) {
  }
};

class DMATracerImpl {
  API::conf_object_t *theUnderlyingObject;

  MemoryMessage theMemoryMessage;
  std::function<void(MemoryMessage &)> toDMA;

public:
  DMATracerImpl(API::conf_object_t *anUnderlyingObjec)
      : theUnderlyingObject(anUnderlyingObjec), theMemoryMessage(MemoryMessage::WriteReq) {
  }

  // Initialize the tracer to the desired CPU
  void init(std::function<void(MemoryMessage &)> aToDMA) {
    toDMA = aToDMA;
  }

  void updateStats() {
  }

  API::cycles_t dma_mem_hier_operate(API::memory_transaction_t *mem_trans) {

    const int32_t k_no_stall = 0;
    // debugTransaction(mem_trans);
    if (API::qemu_callbacks.QEMU_mem_op_is_write(&mem_trans->s)) {
      DBG_(Verb, (<< "DMA To Mem: " << std::hex << mem_trans->s.physical_address << std::dec
                  << " Size: " << mem_trans->s.size));
      theMemoryMessage.type() = MemoryMessage::WriteReq;
    } else {
      DBG_(Verb, (<< "DMA From Mem: " << std::hex << mem_trans->s.physical_address << std::dec
                  << " Size: " << mem_trans->s.size));
      theMemoryMessage.type() = MemoryMessage::ReadReq;
    }

    theMemoryMessage.address() = PhysicalMemoryAddress(mem_trans->s.physical_address);
    theMemoryMessage.reqSize() = mem_trans->s.size;
    toDMA(theMemoryMessage);
    return k_no_stall; // Never stalls
  }

}; // class DMATracerImpl

class DMATracer : public Qemu::AddInObject<DMATracerImpl> {
  typedef Qemu::AddInObject<DMATracerImpl> base;

public:
  static const Qemu::Persistence class_persistence = Qemu::Session;
  static std::string className() {
    return "DMATracer";
  }
  static std::string classDescription() {
    return "Flexus's DMA tracer.";
  }

  DMATracer() : base() {
  }
  DMATracer(Qemu::API::conf_object_t *aQemuObject) : base(aQemuObject) {
  }
  DMATracer(DMATracerImpl *anImpl) : base(anImpl) {
  }
};

class QemuTracerManagerImpl : public QemuTracerManager {
  int32_t theNumCPUs;
  QemuTracer *theTracers; // QEMU Components that have support for callbacks
  DMATracer theDMATracer;
  std::function<void(int, MemoryMessage &)> toL1D;
  std::function<void(int, MemoryMessage &)> toL1I;
  std::function<void(MemoryMessage &)> toDMA;
  std::function<void(int, MemoryMessage &)> toNAW;

  //  bool theWhiteBoxDebug;
  //  int32_t  theWhiteBoxPeriod;
  bool theSendNonAllocatingStores;

public:
  QemuTracerManagerImpl(int32_t aNumCPUs, std::function<void(int, MemoryMessage &)> aToL1D,
                        std::function<void(int, MemoryMessage &)> aToL1I,
                        std::function<void(MemoryMessage &)> aToDMA,
                        std::function<void(int, MemoryMessage &)> aToNAW,
                        //	  , bool aWhiteBoxDebug
                        //	  , int32_t aWhiteBoxPeriod
                        bool aSendNonAllocatingStores)
      : theNumCPUs(aNumCPUs), toL1D(aToL1D), toL1I(aToL1I), toDMA(aToDMA),
        toNAW(aToNAW),
        //  , theWhiteBoxDebug(aWhiteBoxDebug)
        //  , theWhiteBoxPeriod(aWhiteBoxPeriod)
        theSendNonAllocatingStores(aSendNonAllocatingStores) {
    DBG_(Dev, (<< "Initializing QemuTracerManager."));
    theNumCPUs = aNumCPUs;

    // Dump translation caches so that helpers get regenerated with new insertions
    Qemu::API::qemu_callbacks.QEMU_flush_tb_cache();

    // Flexus::SharedTypes::MemoryMessage msg(MemoryMessage::LoadReq);
    // toL1D((int32_t) 0, msg);
    createTracers();
    createDMATracer();
    DBG_(Dev, (<< "Done initializing QemuTracerManager."));
  }

  virtual ~QemuTracerManagerImpl() {
  }

  void setQemuQuantum(int64_t aSwitchTime) {
  }

  void setSystemTick(double aTickFreq) {
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      API::conf_object_t *cpu = Qemu::API::qemu_callbacks.QEMU_get_cpu_by_index(i);
      API::qemu_callbacks.QEMU_set_tick_frequency(cpu, aTickFreq);
    }
  }

  void updateStats() {
    for (int32_t i = 0; i < theNumCPUs; ++i) {
      theTracers[i]->updateStats();
    }
  }

  void enableInstructionTracing() {
    // DBG_Assert(false, (<< "Enabling instruction tracing is done by default"));
  }

private:

  void createTracers() {
    theTracers = new QemuTracer[theNumCPUs];

    // Create QemuTracer Factory
    Qemu::Factory<QemuTracer> tracer_factory;
    // registerTimingInterface();

    // Create QemuTracer Objects
    // FIXME I believe this had been used incorrectly as the end point of the
    // inner for loop. In the simics it was being used, but not sure why or how
    // it works.
    for (int32_t ii = 0; ii < theNumCPUs; ++ii) {
      std::string feeder_name("flexus-feeder");
      if (theNumCPUs > 1) {
        feeder_name += '-' + boost::padded_string_cast<2, '0'>(ii);
      }
      theTracers[ii] = tracer_factory.create(feeder_name);

      API::conf_object_t *cpu = API::qemu_callbacks.QEMU_get_cpu_by_index(ii);
      theTracers[ii]->init(cpu, ii, toL1D, toL1I, toNAW,
                           //			  , theWhiteBoxDebug
                           //			  , theWhiteBoxPeriod
                           theSendNonAllocatingStores);
    }
    // Flexus::SharedTypes::MemoryMessage msg(MemoryMessage::LoadReq);
    // theTracers[0]->toL1D((int32_t) 0, msg);
    // Should not be here, need to also change the function
    //    registerTimingInterface(&theTracers[ii]);

    // Register memory interface: Insert QEMU callbacks for instruction fetch and data access
    Flexus::Qemu::API::qflex_sim_callbacks.trace_mem = new Flexus::Qemu::API::callback_t[theNumCPUs];
    for (int i = 0; i < theNumCPUs; i++) {
      Flexus::Qemu::API::qflex_sim_callbacks.trace_mem[i].obj = (void *) (theTracers + i);
      Flexus::Qemu::API::qflex_sim_callbacks.trace_mem[i].fn = (void *) &TraceMemHierOperate;
    }
  }

  void createDMATracer(void) {
    // Create QemuTracer Factory
    Qemu::Factory<DMATracer> tracer_factory;
    // API::conf_class_t *trace_class = tracer_factory.getQemuClass();

    std::string tracer_name("dma-tracer");
    theDMATracer = tracer_factory.create(tracer_name);
    DBG_(Crit, (<< "Connecting to DMA memory map"));
    theDMATracer->init(toDMA);

    // Register DMA interface: Insert QEMU callbacks for DMA accesses
    for (int i = 0; i < theNumCPUs; i++) {
      Flexus::Qemu::API::qflex_sim_callbacks.trace_mem_dma.obj = (void *) &theDMATracer;
      Flexus::Qemu::API::qflex_sim_callbacks.trace_mem_dma.fn = (void *) &DMAMemHierOperate;
    }
  }
};

QemuTracerManager *
QemuTracerManager::construct(int32_t aNumCPUs, std::function<void(int, MemoryMessage &)> toL1D,
                             std::function<void(int, MemoryMessage &)> toL1I,
                             std::function<void(MemoryMessage &)> toDMA,
                             std::function<void(int, MemoryMessage &)> toNAW
                             //    , bool aWhiteBoxDebug
                             //    , int32_t aWhiteBoxPeriod
                             ,
                             bool aSendNonAllocatingStores) {
  return new QemuTracerManagerImpl(aNumCPUs, toL1D, toL1I, toDMA, toNAW,
                                   /* aWhiteBoxDebug, aWhiteBoxPeriod,*/ aSendNonAllocatingStores);
}

void TraceMemHierOperate(void *obj, API::memory_transaction_t *mem_trans) {
  // Thread safe as is, no need to add muteces
  QemuTracer *tracer = reinterpret_cast<QemuTracer *>(obj);
  (*tracer)->trace_mem_hier_operate(mem_trans);
};
void DMAMemHierOperate(void *obj, API::memory_transaction_t *mem_trans) {
  DMATracerImpl *tracer = reinterpret_cast<DMATracerImpl *>(obj);
  tracer->dma_mem_hier_operate(mem_trans);
};

} // namespace nDecoupledFeeder
