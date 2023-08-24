#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()


namespace std {
template <> struct hash<VirtualMemoryAddress> {
  std::size_t operator()(const VirtualMemoryAddress &anAddress) const {
    return ((hash<uint64_t>()(uint64_t(anAddress))));
  }
};

} // namespace std



using namespace Flexus::SharedTypes;
struct MMUConfiguration_struct {
  std::string theConfigName_;
  MMUConfiguration_struct(std::string
    const & aConfigName): theConfigName_(aConfigName) {}
  std::string
  const & name() const {
    return theConfigName_;
  }

  template < class Parameter > struct get;
  int Cores;
  struct Cores_param {};
  size_t iTLBSize;
  struct iTLBSize_param {};
  size_t dTLBSize;
  struct dTLBSize_param {};
  bool PerfectTLB;
  struct PerfectTLB_param {};
};

template < > struct MMUConfiguration_struct::get < MMUConfiguration_struct::Cores_param > {
  static std::string name() {
    return "Cores";
  }
  static std::string description() {
    return "Number of cores";
  }
  static std::string cmd_line_switch() {
    return "cores";
  }

  typedef int type;static type defaultValue() {
    return 1;
  }
  static type MMUConfiguration_struct:: * member() {
    return & MMUConfiguration_struct::Cores;
  }
};
template < > struct MMUConfiguration_struct::get < MMUConfiguration_struct::iTLBSize_param > {
  static std::string name() {
    return "iTLBSize";
  }
  static std::string description() {
    return "Size of the Instruction TLB";
  }
  static std::string cmd_line_switch() {
    return "itlbsize";
  }
  typedef size_t type;static type defaultValue() {
    return 64;
  }
  static type MMUConfiguration_struct:: * member() {
    return & MMUConfiguration_struct::iTLBSize;
  }
};
template < > struct MMUConfiguration_struct::get < MMUConfiguration_struct::dTLBSize_param > {
  static std::string name() {
    return "dTLBSize";
  }
  static std::string description() {
    return "Size of the Data TLB";
  }
  static std::string cmd_line_switch() {
    return "dtlbsize";
  }
  typedef size_t type;static type defaultValue() {
    return 64;
  }
  static type MMUConfiguration_struct:: * member() {
    return & MMUConfiguration_struct::dTLBSize;
  }
};
template < > struct MMUConfiguration_struct::get < MMUConfiguration_struct::PerfectTLB_param > {
  static std::string name() {
    return "PerfectTLB";
  }
  static std::string description() {
    return "TLB never misses";
  }
  static std::string cmd_line_switch() {
    return "perfecttlb";
  }
  typedef bool type;static type defaultValue() {
    return false;
  }
  static type MMUConfiguration_struct:: * member() {
    return & MMUConfiguration_struct::PerfectTLB;
  }
};
struct MMUConfiguration {
  typedef MMUConfiguration_struct cfg_struct_;
  cfg_struct_ theCfg_;
  std::string name() {
    return theCfg_.theConfigName_;
  }
  MMUConfiguration_struct & cfg() {
    return theCfg_;
  }
  MMUConfiguration(const std::string & aConfigName): theCfg_(aConfigName), Cores(theCfg_), iTLBSize(theCfg_), dTLBSize(theCfg_), PerfectTLB(theCfg_) {}
  Flexus::Core::aux_::DynamicParameter < MMUConfiguration_struct, MMUConfiguration_struct::Cores_param > Cores;
  Flexus::Core::aux_::DynamicParameter < MMUConfiguration_struct, MMUConfiguration_struct::iTLBSize_param > iTLBSize;
  Flexus::Core::aux_::DynamicParameter < MMUConfiguration_struct, MMUConfiguration_struct::dTLBSize_param > dTLBSize;
  Flexus::Core::aux_::DynamicParameter < MMUConfiguration_struct, MMUConfiguration_struct::PerfectTLB_param > PerfectTLB;
};;


struct MMUJumpTable;


struct MMUInterface: public Flexus::Core::ComponentInterface {
  typedef MMUConfiguration_struct configuration;
  typedef MMUJumpTable jump_table;
  static MMUInterface * instantiate(configuration & , jump_table & , Flexus::Core::index_t, Flexus::Core::index_t);
  BOOST_PP_IIF_1 /* this is for trace*/
  ((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))
  ((Drive, MMUDrive, "MMUDrive", xx, 0))
  
  (nil)(BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC, BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY)
  
  (FLEXUS_IFACE_GENERATE, MMUInterface, ((PushInputDynArray, iRequestIn, "iRequestIn", TranslationPtr, 0))
  ((PushInputDynArray, dRequestIn, "dRequestIn", TranslationPtr, 0))
  ((PushInputDynArray, ResyncIn, "ResyncIn", int, 0))
  ((PushOutputDynArray, iTranslationReply, "iTranslationReply", TranslationPtr, 0))
  ((PushOutputDynArray, dTranslationReply, "dTranslationReply", TranslationPtr, 0))
  ((PushOutputDynArray, MemoryRequestOut, "MemoryRequestOut", TranslationPtr, 0))
  ((PushInputDynArray, TLBReqIn, "TLBReqIn", TranslationPtr, 0))
  /* this is for trace*/ 
  ((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))
  ((Drive, MMUDrive, "MMUDrive", xx, 0)))
};

struct MMUJumpTable {
  typedef MMUInterface iface;
  BOOST_PP_IIF_1 /* this is for trace*/((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0))(nil)(BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC, BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY)(FLEXUS_IFACE_JUMPTABLE, x, ((PushInputDynArray, iRequestIn, "iRequestIn", TranslationPtr, 0))((PushInputDynArray, dRequestIn, "dRequestIn", TranslationPtr, 0))((PushInputDynArray, ResyncIn, "ResyncIn", int, 0))((PushOutputDynArray, iTranslationReply, "iTranslationReply", TranslationPtr, 0))((PushOutputDynArray, dTranslationReply, "dTranslationReply", TranslationPtr, 0))((PushOutputDynArray, MemoryRequestOut, "MemoryRequestOut", TranslationPtr, 0))((PushInputDynArray, TLBReqIn, "TLBReqIn", TranslationPtr, 0)) /* this is for trace*/ ((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0))) MMUJumpTable() {
    BOOST_PP_IIF_1 /* this is for trace*/((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0))(nil)(BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC, BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY)(FLEXUS_IFACE_JUMPTABLE_INIT, x, ((PushInputDynArray, iRequestIn, "iRequestIn", TranslationPtr, 0))((PushInputDynArray, dRequestIn, "dRequestIn", TranslationPtr, 0))((PushInputDynArray, ResyncIn, "ResyncIn", int, 0))((PushOutputDynArray, iTranslationReply, "iTranslationReply", TranslationPtr, 0))((PushOutputDynArray, dTranslationReply, "dTranslationReply", TranslationPtr, 0))((PushOutputDynArray, MemoryRequestOut, "MemoryRequestOut", TranslationPtr, 0))((PushInputDynArray, TLBReqIn, "TLBReqIn", TranslationPtr, 0)) /* this is for trace*/ ((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0)))
  }
  void check(std::string
    const & anInstance) {
    BOOST_PP_IIF_1 /* this is for trace*/((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0))(nil)(BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC, BOOST_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY)(FLEXUS_IFACE_JUMPTABLE_CHECK, x, ((PushInputDynArray, iRequestIn, "iRequestIn", TranslationPtr, 0))((PushInputDynArray, dRequestIn, "dRequestIn", TranslationPtr, 0))((PushInputDynArray, ResyncIn, "ResyncIn", int, 0))((PushOutputDynArray, iTranslationReply, "iTranslationReply", TranslationPtr, 0))((PushOutputDynArray, dTranslationReply, "dTranslationReply", TranslationPtr, 0))((PushOutputDynArray, MemoryRequestOut, "MemoryRequestOut", TranslationPtr, 0))((PushInputDynArray, TLBReqIn, "TLBReqIn", TranslationPtr, 0)) /* this is for trace*/ ((PushOutputDynArray, ResyncOut, "ResyncOut", int, 0))((Drive, MMUDrive, "MMUDrive", xx, 0)))
  }
};




namespace nMMU {
using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
uint64_t PAGEMASK;
class MMUComponent : public Flexus::Core::FlexusComponentBase<MMUCo
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()


namespace std {
template <> struct hash<VirtualMemoryAddress> {
  std::size_t operator()(const VirtualMemoryAddress &anAddress) const {
    return ((hash<uint64_t>()(uint64_t(anAddress))));
  }
};
} // namespace std
namespace nMMU {
using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
uint64_t PAGEMASK;
class MMUComponent : public Flexus::Core::FlexusComponentBase<MMUComponent, MMUConfiguration, MMUInterface> {
  typedef Flexus::Core::FlexusComponentBase<MMUComponent, MMUConfiguration, MMUInterface> base; typedef base::cfg_t cfg_t; static std::string componentType() { return "MMU"; } public: using base::flexusIndex; using base::flexusWidth; using base::name; using base::statName; using base::cfg; using base::interface; using interface::get_channel; using interface::get_channel_array; private: typedef base::self self;
private:
private:
  struct TLBentry {
    friend class boost::serialization::access;
    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theRate;
      ar &theVaddr;
      ar &thePaddr;
    }
    TLBentry() { 
    }
    TLBentry(VirtualMemoryAddress anAddress) : theRate(0), theVaddr(anAddress) {
    }
    uint64_t theRate;
    VirtualMemoryAddress theVaddr;
    PhysicalMemoryAddress thePaddr;
    bool operator==(const TLBentry &other) const {
      return (theVaddr == other.theVaddr);
    }
    TLBentry &operator=(const PhysicalMemoryAddress &other) {
      PhysicalMemoryAddress otherAligned(other & PAGEMASK);
      thePaddr = otherAligned;
      return *this;
    }
  };
  struct TLB {
    friend class boost::serialization::access;
    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theTLB;
      ar &theSize;
    }
    std::pair<bool, PhysicalMemoryAddress> lookUp(const VirtualMemoryAddress &anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      std::pair<bool, PhysicalMemoryAddress> ret{false, PhysicalMemoryAddress(0)};
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        iter->second.theRate++;
        if (iter->second.theVaddr == anAddressAligned) {
          iter->second.theRate = 0;
          ret.first = true;
          ret.second = iter->second.thePaddr;
        }
      }
      return ret;
    }
    TLBentry &operator[](VirtualMemoryAddress anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      auto iter = theTLB.find(anAddressAligned);
      if (iter == theTLB.end()) {
        size_t s = theTLB.size();
        if (s == theSize) {
          evict();
        }
        std::pair<tlbIterator, bool> result;
        result = theTLB.insert({anAddressAligned, TLBentry(anAddressAligned)});
        (static_cast <bool> (result.second) ? void (0) : __assert_fail ("result.second", "components/MMU/MMUImpl.cpp", 167, __extension__ __PRETTY_FUNCTION__));
        iter = result.first;
      }
      return iter->second;
    }
    void resize(size_t aSize) {
      theTLB.reserve(aSize);
      theSize = aSize;
    }
    size_t capacity() {
      return theSize;
    }
    void clear() {
      theTLB.clear();
    }
    size_t size() {
      return theTLB.size();
    }
  private:
    void evict() {
      auto res = theTLB.begin();
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        if (iter->second.theRate > res->second.theRate) {
          res = iter;
        }
      }
      theTLB.erase(res);
    }
    size_t theSize;
    std::unordered_map<VirtualMemoryAddress, TLBentry> theTLB;
    typedef std::unordered_map<VirtualMemoryAddress, TLBentry>::iterator tlbIterator;
  };
  std::unique_ptr<PageWalk> thePageWalker;
  TLB theInstrTLB;
  TLB theDataTLB;
  std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
  std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
  std::set<VirtualMemoryAddress> alreadyPW;
  std::vector<boost::intrusive_ptr<Translation>> standingEntries;
  std::shared_ptr<Flexus::Qemu::Processor> theCPU;
  std::shared_ptr<mmu_t> theMMU;
  bool theMMUInitialized;
public:
  MMUComponent (cfg_t & aCfg, MMUInterface::jump_table & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth) : base(aCfg, aJumpTable, anIndex, aWidth) {
  }
  bool isQuiesced() const {
    return false;
  }
  void saveState(std::string const &aDirName) {
    std::ofstream iFile(aDirName + "/" + statName() + "-itlb", std::ofstream::out);
    std::ofstream dFile(aDirName + "/" + statName() + "-dtlb", std::ofstream::out);
    boost::archive::text_oarchive ioarch(iFile);
    boost::archive::text_oarchive doarch(dFile);
    ioarch << theInstrTLB;
    doarch << theDataTLB;
    iFile.close();
    dFile.close();
  }
  void loadState(std::string const &aDirName) {
    std::ifstream iFile(aDirName + "/" + statName() + "-itlb", std::ifstream::in);
    std::ifstream dFile(aDirName + "/" + statName() + "-dtlb", std::ifstream::in);
    boost::archive::text_iarchive iiarch(iFile);
    boost::archive::text_iarchive diarch(dFile);
    size_t iSize = theInstrTLB.capacity();
    size_t dSize = theDataTLB.capacity();
    iiarch >> theInstrTLB;
    diarch >> theDataTLB;
    if (iSize != theInstrTLB.capacity()) {
      size_t old_size = theInstrTLB.capacity();
      theInstrTLB.clear();
      theInstrTLB.resize(iSize);
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 257, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Changing iTLB capacity from " << old_size << " to " << iSize).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    }
    if (dSize != theDataTLB.capacity()) {
      size_t old_size = theDataTLB.capacity();
      theDataTLB.clear();
      theDataTLB.resize(dSize);
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 263, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Changing dTLB capacity from " << old_size << " to " << dSize).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    }
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 265, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Size - iTLB:" << theInstrTLB.capacity() << ", dTLB:" << theDataTLB.capacity()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    iFile.close();
    dFile.close();
  }
  // Initialization
  void initialize() {
    theCPU = std::make_shared<Flexus::Qemu::Processor>(
        Flexus::Qemu::Processor::getProcessor((int)flexusIndex()));
    thePageWalker.reset(new PageWalk(flexusIndex()));
    thePageWalker->setMMU(theMMU);
    theMMUInitialized = false;
    theInstrTLB.resize(cfg.iTLBSize);
    theDataTLB.resize(cfg.dTLBSize);
  }
  void finalize() {
  }
  // MMUDrive
  //----------
  void drive(interface::MMUDrive const &) {
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && (*this).debugEnabled()) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 288, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("CompName", (*this).statName()) .set("CompIndex", (*this).flexusIndex()) .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "MMUDrive").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    busCycle();
    thePageWalker->cycle();
    processMemoryRequests();
  }
  void busCycle() {
    while (!theLookUpEntries.empty()) {
      TranslationPtr item = theLookUpEntries.front();
      theLookUpEntries.pop();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 300, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Processing lookup entry for " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->isInstr() != item->isData())) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 301, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->isInstr() != item->isData())"); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 303, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      std::pair<bool, PhysicalMemoryAddress> entry =
          (item->isInstr() ? theInstrTLB : theDataTLB)
              .lookUp((VirtualMemoryAddress)(item->theVaddr));
      if (cfg.PerfectTLB) {
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        entry.first = true;
        entry.second = perfectPaddr;
        if (perfectPaddr == qemuFaultAddress)
          item->setPagefault();
      }
      if (entry.first) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 320, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is a Hit " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        // item exists so mark hit
        item->setHit();
        if(entry.second == (qemuFaultAddress & PAGEMASK))
          item->thePaddr = qemuFaultAddress;
        else
          item->thePaddr = (PhysicalMemoryAddress)(entry.second | (item->theVaddr & ~(PAGEMASK)));
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        // DBG_Assert(item->thePaddr == perfectPaddr, (<< "Translation mismatch. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr));
        if(item->thePaddr != perfectPaddr){
          do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 335, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. Correcting address forcefully. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
          item->thePaddr = perfectPaddr;
          (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] = (PhysicalMemoryAddress)(item->thePaddr);
        }
        if (item->isInstr())
          get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
        else
          get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
      } else {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 345, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is a miss " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) == alreadyPW.end()) {
          // mark miss
          item->setMiss();
          if (thePageWalker->push_back(item)) {
            alreadyPW.insert(pageAddr);
            thePageWalkEntries.push(item);
          } else {
            PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
                *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
                item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
                item->theVaddr));
            item->setHit();
            item->thePaddr = perfectPaddr;
            if (item->isInstr())
              get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
            else
              get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
          }
        } else {
          standingEntries.push_back(item);
        }
      }
    }
    while (!thePageWalkEntries.empty()) {
      TranslationPtr item = thePageWalkEntries.front();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 375, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Processing PW entry for " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      if (item->isAnnul()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 378, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item was annulled " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        thePageWalkEntries.pop();
      } else if (item->isDone()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 381, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item was Done translationg " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 383, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Translation is Done " << item->theVaddr << " -- " << item->thePaddr << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
        thePageWalkEntries.pop();
        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) != alreadyPW.end()) {
          alreadyPW.erase(pageAddr);
          for (auto it = standingEntries.begin(); it != standingEntries.end();) {
            if (((*it)->theVaddr & pageAddr) == pageAddr && item->isInstr() == (*it)->isInstr()) {
              theLookUpEntries.push(*it);
              standingEntries.erase(it);
            } else {
              ++it;
            }
          }
        }
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->isInstr() != item->isData())) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 399, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->isInstr() != item->isData())"); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 3 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(3), "components/MMU/MMUImpl.cpp", 400, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        if (!item->isPagefault()) {
          (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] =
              (PhysicalMemoryAddress)(item->thePaddr);
        }
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 410, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        if (item->isInstr())
          get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
        else
          get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
      } else {
        break;
      }
    }
  }
  void processMemoryRequests() {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 422, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    while (thePageWalker->hasMemoryRequest()) {
      TranslationPtr tmp = thePageWalker->popMemoryRequest();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 425, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Sending a Memory Translation request to Core ready(" << tmp->isReady() << ")  " << tmp->theVaddr << " -- " << tmp->thePaddr << "  -- ID " << tmp->theID).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      get_channel(interface::MemoryRequestOut(), jump_table_.wire_available_MemoryRequestOut, jump_table_.wire_manip_MemoryRequestOut, flexusIndex()) << tmp;
    }
  }
  void resyncMMU(int anIndex) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 433, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 434, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Resynchronizing MMU").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    static bool optimize = false;
    theMMUInitialized = optimize;
    if (!theMMUInitialized) {
      theMMU.reset(new mmu_t());
      theMMUInitialized = true;
    }
    theMMU->initRegsFromQEMUObject(getMMURegsFromQEMU(anIndex));
    theMMU->setupAddressSpaceSizesAndGranules();
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran0->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 445, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran0->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG0 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran1->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 446, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran1->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG1 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
    if (thePageWalker) {
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 449, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Annulling all PW entries").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      thePageWalker->annulAll();
    }
    if (thePageWalkEntries.size() > 0) {
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 453, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting PageWalk Entries").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      while (!thePageWalkEntries.empty()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 455, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting MMU PageWalk Entrie " << thePageWalkEntries.front()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        thePageWalkEntries.pop();
      }
    }
    alreadyPW.clear();
    standingEntries.clear();
    if (theLookUpEntries.size() > 0) {
      while (!theLookUpEntries.empty()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 464, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting MMU Lookup Entrie " << theLookUpEntries.front()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        theLookUpEntries.pop();
      }
    }
    get_channel(interface::ResyncOut(), jump_table_.wire_available_ResyncOut, jump_table_.wire_manip_ResyncOut, flexusIndex()) << anIndex;
  }
  // Msutherl: Fetch MMU's registers
  std::shared_ptr<mmu_regs_t> getMMURegsFromQEMU(uint8_t anIndex) {
    std::shared_ptr<mmu_regs_t> mmu_obj(new mmu_regs_t());
    mmu_obj->SCTLR[0] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(0);
    mmu_obj->SCTLR[1] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(1);
    mmu_obj->SCTLR[2] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(2);
    mmu_obj->SCTLR[3] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(3);
    mmu_obj->TCR[0] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 0);
    mmu_obj->TCR[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 1);
    mmu_obj->TCR[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 2);
    mmu_obj->TCR[3] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 3);
    // mmu_obj->TTBR1[EL0] = Qemu::API::qemu_callbacks.QEMU_read_register(
    //     *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, EL0);
    mmu_obj->TTBR0[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 1);
    mmu_obj->TTBR1[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, -1, 1);
    mmu_obj->TTBR0[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 2);
    mmu_obj->TTBR1[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, -1, 2);
    mmu_obj->TTBR0[3] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 3);
    // mmu_obj->TTBR1[EL3] = Qemu::API::qemu_callbacks.QEMU_read_register(
    //     *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, EL3);
    mmu_obj->ID_AA64MMFR0_EL1 = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_ID_AA64MMFR0, -1, -1);
    return mmu_obj;
  }
  bool IsTranslationEnabledAtEL(uint8_t &anEL) {
    return true; // theCore->IsTranslationEnabledAtEL(anEL);
  }
  bool available(interface::ResyncIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
    resyncMMU(aResync);
  }
  bool available(interface::iRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::iRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 524, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Instruction RequestIn" << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }
  bool available(interface::dRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::dRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 535, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Data RequestIn" << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }
  void sendTLBresponse(TranslationPtr aTranslation) {
    PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
        *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
        aTranslation->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
        aTranslation->theVaddr));
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(aTranslation->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 548, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(aTranslation->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << aTranslation->theVaddr << ", PA:" << aTranslation->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (aTranslation->isInstr()) {
      get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << aTranslation;
    } else {
      get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << aTranslation;
    }
  }
  bool available(interface::TLBReqIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::TLBReqIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
        *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
        aTranslate->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
        aTranslate->theVaddr));
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(aTranslate->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 565, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(aTranslate->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << aTranslate->theVaddr << ", PA:" << aTranslate->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (!cfg.PerfectTLB &&
        (aTranslate->isInstr() ? theInstrTLB : theDataTLB).lookUp(aTranslate->theVaddr).first ==
            false) {
      if (!theMMUInitialized) {
        theMMU.reset(new mmu_t());
        theMMU->initRegsFromQEMUObject(getMMURegsFromQEMU((int)flexusIndex()));
        theMMU->setupAddressSpaceSizesAndGranules();
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran0->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 573, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran0->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG0 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran1->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 574, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran1->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG1 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
        thePageWalker->setMMU(theMMU);
        theMMUInitialized = true;
      }
      (aTranslate->isInstr() ? theInstrTLB : theDataTLB)[aTranslate->theVaddr] =
          aTranslate->thePaddr;
      thePageWalker->push_back_trace(aTranslate,
                                     Flexus::Qemu::Processor::getProcessor((int)flexusIndex()));
    }
  }
};
} // End Namespace nMMU
MMUInterface * MMUInterface::instantiate( MMUConfiguration::cfg_struct_ &aCfg, MMUInterface::jump_table &aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth) { return new nMMU::MMUComponent(aCfg, aJumpTable, anIndex, aWidth); } struct eat_semicolon_;
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::dRequestIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::iRequestIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::ResyncIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::iTranslationReply const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::dTranslationReply const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::MemoryRequestOut const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::TLBReqIn const &) {
  return (cfg.Cores);
}

    friend class boost::serialization::access;
    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theRate;
      ar &theVaddr;
      ar &thePaddr;
    }
    TLBentry() {
    }
    TLBentry(VirtualMemoryAddress anAddress) : theRate(0), theVaddr(anAddress) {
    }
    uint64_t theRate;
    VirtualMemoryAddress theVaddr;
    PhysicalMemoryAddress thePaddr;
    bool operator==(const TLBentry &other) const {
      return (theVaddr == other.theVaddr);
    }
    TLBentry &operator=(const PhysicalMemoryAddress &other) {
      PhysicalMemoryAddress otherAligned(other & PAGEMASK);
      thePaddr = otherAligned;
      return *this;
    }
  };
  struct TLB {
    friend class boost::serialization::access;
    template <class Archive> void serialize(Archive &ar, const unsigned int version) {
      ar &theTLB;
      ar &theSize;
    }
    std::pair<bool, PhysicalMemoryAddress> lookUp(const VirtualMemoryAddress &anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      std::pair<bool, PhysicalMemoryAddress> ret{false, PhysicalMemoryAddress(0)};
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        iter->second.theRate++;
        if (iter->second.theVaddr == anAddressAligned) {
          iter->second.theRate = 0;
          ret.first = true;
          ret.second = iter->second.thePaddr;
        }
      }
      return ret;
    }
    TLBentry &operator[](VirtualMemoryAddress anAddress) {
      VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
      auto iter = theTLB.find(anAddressAligned);
      if (iter == theTLB.end()) {
        size_t s = theTLB.size();
        if (s == theSize) {
          evict();
        }
        std::pair<tlbIterator, bool> result;
        result = theTLB.insert({anAddressAligned, TLBentry(anAddressAligned)});
        (static_cast <bool> (result.second) ? void (0) : __assert_fail ("result.second", "components/MMU/MMUImpl.cpp", 167, __extension__ __PRETTY_FUNCTION__));
        iter = result.first;
      }
      return iter->second;
    }
    void resize(size_t aSize) {
      theTLB.reserve(aSize);
      theSize = aSize;
    }
    size_t capacity() {
      return theSize;
    }
    void clear() {
      theTLB.clear();
    }
    size_t size() {
      return theTLB.size();
    }
  private:
    void evict() {
      auto res = theTLB.begin();
      for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
        if (iter->second.theRate > res->second.theRate) {
          res = iter;
        }
      }
      theTLB.erase(res);
    }
    size_t theSize;
    std::unordered_map<VirtualMemoryAddress, TLBentry> theTLB;
    typedef std::unordered_map<VirtualMemoryAddress, TLBentry>::iterator tlbIterator;
  };
  std::unique_ptr<PageWalk> thePageWalker;
  TLB theInstrTLB;
  TLB theDataTLB;
  std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
  std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
  std::set<VirtualMemoryAddress> alreadyPW;
  std::vector<boost::intrusive_ptr<Translation>> standingEntries;
  std::shared_ptr<Flexus::Qemu::Processor> theCPU;
  std::shared_ptr<mmu_t> theMMU;
  bool theMMUInitialized;
public:
  MMUComponent (cfg_t & aCfg, MMUInterface::jump_table & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth) : base(aCfg, aJumpTable, anIndex, aWidth) {
  }
  bool isQuiesced() const {
    return false;
  }
  void saveState(std::string const &aDirName) {
    std::ofstream iFile(aDirName + "/" + statName() + "-itlb", std::ofstream::out);
    std::ofstream dFile(aDirName + "/" + statName() + "-dtlb", std::ofstream::out);
    boost::archive::text_oarchive ioarch(iFile);
    boost::archive::text_oarchive doarch(dFile);
    ioarch << theInstrTLB;
    doarch << theDataTLB;
    iFile.close();
    dFile.close();
  }
  void loadState(std::string const &aDirName) {
    std::ifstream iFile(aDirName + "/" + statName() + "-itlb", std::ifstream::in);
    std::ifstream dFile(aDirName + "/" + statName() + "-dtlb", std::ifstream::in);
    boost::archive::text_iarchive iiarch(iFile);
    boost::archive::text_iarchive diarch(dFile);
    size_t iSize = theInstrTLB.capacity();
    size_t dSize = theDataTLB.capacity();
    iiarch >> theInstrTLB;
    diarch >> theDataTLB;
    if (iSize != theInstrTLB.capacity()) {
      size_t old_size = theInstrTLB.capacity();
      theInstrTLB.clear();
      theInstrTLB.resize(iSize);
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 257, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Changing iTLB capacity from " << old_size << " to " << iSize).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    }
    if (dSize != theDataTLB.capacity()) {
      size_t old_size = theDataTLB.capacity();
      theDataTLB.clear();
      theDataTLB.resize(dSize);
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 263, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Changing dTLB capacity from " << old_size << " to " << dSize).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    }
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 265, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Size - iTLB:" << theInstrTLB.capacity() << ", dTLB:" << theDataTLB.capacity()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    iFile.close();
    dFile.close();
  }
  // Initialization
  void initialize() {
    theCPU = std::make_shared<Flexus::Qemu::Processor>(
        Flexus::Qemu::Processor::getProcessor((int)flexusIndex()));
    thePageWalker.reset(new PageWalk(flexusIndex()));
    thePageWalker->setMMU(theMMU);
    theMMUInitialized = false;
    theInstrTLB.resize(cfg.iTLBSize);
    theDataTLB.resize(cfg.dTLBSize);
  }
  void finalize() {
  }
  // MMUDrive
  //----------
  void drive(interface::MMUDrive const &) {
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && (*this).debugEnabled()) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 288, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("CompName", (*this).statName()) .set("CompIndex", (*this).flexusIndex()) .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "MMUDrive").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    busCycle();
    thePageWalker->cycle();
    processMemoryRequests();
  }
  void busCycle() {
    while (!theLookUpEntries.empty()) {
      TranslationPtr item = theLookUpEntries.front();
      theLookUpEntries.pop();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 300, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Processing lookup entry for " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->isInstr() != item->isData())) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 301, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->isInstr() != item->isData())"); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 303, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      std::pair<bool, PhysicalMemoryAddress> entry =
          (item->isInstr() ? theInstrTLB : theDataTLB)
              .lookUp((VirtualMemoryAddress)(item->theVaddr));
      if (cfg.PerfectTLB) {
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        entry.first = true;
        entry.second = perfectPaddr;
        if (perfectPaddr == qemuFaultAddress)
          item->setPagefault();
      }
      if (entry.first) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 320, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is a Hit " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        // item exists so mark hit
        item->setHit();
        if(entry.second == (qemuFaultAddress & PAGEMASK))
          item->thePaddr = qemuFaultAddress;
        else
          item->thePaddr = (PhysicalMemoryAddress)(entry.second | (item->theVaddr & ~(PAGEMASK)));
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        // DBG_Assert(item->thePaddr == perfectPaddr, (<< "Translation mismatch. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr));
        if(item->thePaddr != perfectPaddr){
          do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 5 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(5), "components/MMU/MMUImpl.cpp", 335, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. Correcting address forcefully. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
          item->thePaddr = perfectPaddr;
          (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] = (PhysicalMemoryAddress)(item->thePaddr);
        }
        if (item->isInstr())
          get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
        else
          get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
      } else {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 345, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is a miss " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) == alreadyPW.end()) {
          // mark miss
          item->setMiss();
          if (thePageWalker->push_back(item)) {
            alreadyPW.insert(pageAddr);
            thePageWalkEntries.push(item);
          } else {
            PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
                *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
                item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
                item->theVaddr));
            item->setHit();
            item->thePaddr = perfectPaddr;
            if (item->isInstr())
              get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
            else
              get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
          }
        } else {
          standingEntries.push_back(item);
        }
      }
    }
    while (!thePageWalkEntries.empty()) {
      TranslationPtr item = thePageWalkEntries.front();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 375, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Processing PW entry for " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      if (item->isAnnul()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 378, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item was annulled " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        thePageWalkEntries.pop();
      } else if (item->isDone()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 381, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item was Done translationg " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 383, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Translation is Done " << item->theVaddr << " -- " << item->thePaddr << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
        thePageWalkEntries.pop();
        VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
        if (alreadyPW.find(pageAddr) != alreadyPW.end()) {
          alreadyPW.erase(pageAddr);
          for (auto it = standingEntries.begin(); it != standingEntries.end();) {
            if (((*it)->theVaddr & pageAddr) == pageAddr && item->isInstr() == (*it)->isInstr()) {
              theLookUpEntries.push(*it);
              standingEntries.erase(it);
            } else {
              ++it;
            }
          }
        }
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->isInstr() != item->isData())) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 399, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->isInstr() != item->isData())"); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 3 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(3), "components/MMU/MMUImpl.cpp", 400, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        if (!item->isPagefault()) {
          (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] =
              (PhysicalMemoryAddress)(item->thePaddr);
        }
        PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
            *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
            item->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
            item->theVaddr));
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(item->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 410, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(item->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << item->theVaddr << ", PA:" << item->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        if (item->isInstr())
          get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << item;
        else
          get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << item;
      } else {
        break;
      }
    }
  }
  void processMemoryRequests() {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 422, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    while (thePageWalker->hasMemoryRequest()) {
      TranslationPtr tmp = thePageWalker->popMemoryRequest();
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 425, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Sending a Memory Translation request to Core ready(" << tmp->isReady() << ")  " << tmp->theVaddr << " -- " << tmp->thePaddr << "  -- ID " << tmp->theID).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      get_channel(interface::MemoryRequestOut(), jump_table_.wire_available_MemoryRequestOut, jump_table_.wire_manip_MemoryRequestOut, flexusIndex()) << tmp;
    }
  }
  void resyncMMU(int anIndex) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 433, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 434, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Resynchronizing MMU").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
    static bool optimize = false;
    theMMUInitialized = optimize;
    if (!theMMUInitialized) {
      theMMU.reset(new mmu_t());
      theMMUInitialized = true;
    }
    theMMU->initRegsFromQEMUObject(getMMURegsFromQEMU(anIndex));
    theMMU->setupAddressSpaceSizesAndGranules();
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran0->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 445, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran0->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG0 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran1->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 446, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran1->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG1 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
    if (thePageWalker) {
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 449, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Annulling all PW entries").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      thePageWalker->annulAll();
    }
    if (thePageWalkEntries.size() > 0) {
      do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 453, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting PageWalk Entries").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
      while (!thePageWalkEntries.empty()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 455, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting MMU PageWalk Entrie " << thePageWalkEntries.front()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        thePageWalkEntries.pop();
      }
    }
    alreadyPW.clear();
    standingEntries.clear();
    if (theLookUpEntries.size() > 0) {
      while (!theLookUpEntries.empty()) {
        do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 464, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "deleting MMU Lookup Entrie " << theLookUpEntries.front()).str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0);
        theLookUpEntries.pop();
      }
    }
    get_channel(interface::ResyncOut(), jump_table_.wire_available_ResyncOut, jump_table_.wire_manip_ResyncOut, flexusIndex()) << anIndex;
  }
  // Msutherl: Fetch MMU's registers
  std::shared_ptr<mmu_regs_t> getMMURegsFromQEMU(uint8_t anIndex) {
    std::shared_ptr<mmu_regs_t> mmu_obj(new mmu_regs_t());
    mmu_obj->SCTLR[0] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(0);
    mmu_obj->SCTLR[1] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(1);
    mmu_obj->SCTLR[2] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(2);
    mmu_obj->SCTLR[3] = Flexus::Qemu::Processor::getProcessor(anIndex)->readSCTLR(3);
    mmu_obj->TCR[0] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 0);
    mmu_obj->TCR[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 1);
    mmu_obj->TCR[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 2);
    mmu_obj->TCR[3] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TCR, -1, 3);
    // mmu_obj->TTBR1[EL0] = Qemu::API::qemu_callbacks.QEMU_read_register(
    //     *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, EL0);
    mmu_obj->TTBR0[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 1);
    mmu_obj->TTBR1[1] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, -1, 1);
    mmu_obj->TTBR0[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 2);
    mmu_obj->TTBR1[2] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, -1, 2);
    mmu_obj->TTBR0[3] = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR0, -1, 3);
    // mmu_obj->TTBR1[EL3] = Qemu::API::qemu_callbacks.QEMU_read_register(
    //     *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_TTBR1, EL3);
    mmu_obj->ID_AA64MMFR0_EL1 = Qemu::API::qemu_callbacks.QEMU_read_register(
        *Flexus::Qemu::Processor::getProcessor(anIndex), Qemu::API::kMMU_ID_AA64MMFR0, -1, -1);
    return mmu_obj;
  }
  bool IsTranslationEnabledAtEL(uint8_t &anEL) {
    return true; // theCore->IsTranslationEnabledAtEL(anEL);
  }
  bool available(interface::ResyncIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::ResyncIn const &, index_t anIndex, int &aResync) {
    resyncMMU(aResync);
  }
  bool available(interface::iRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::iRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 524, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Instruction RequestIn" << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }
  bool available(interface::dRequestIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::dRequestIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    do { do { if (false) { if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 1 && (1 || !0)) { if ((DBG_Cats::MMU_debug_enabled) && true) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(1), "components/MMU/MMUImpl.cpp", 535, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "\e[0;32m" << __func__ << " " << "MMU: Data RequestIn" << "\e[0m").str()); (entry__).addCategories(MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ; } } while (0); } while (0);
    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
  }
  void sendTLBresponse(TranslationPtr aTranslation) {
    PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
        *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
        aTranslation->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
        aTranslation->theVaddr));
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(aTranslation->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 548, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(aTranslation->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << aTranslation->theVaddr << ", PA:" << aTranslation->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (aTranslation->isInstr()) {
      get_channel(interface::iTranslationReply(), jump_table_.wire_available_iTranslationReply, jump_table_.wire_manip_iTranslationReply, flexusIndex()) << aTranslation;
    } else {
      get_channel(interface::dTranslationReply(), jump_table_.wire_available_dTranslationReply, jump_table_.wire_manip_dTranslationReply, flexusIndex()) << aTranslation;
    }
  }
  bool available(interface::TLBReqIn const &, index_t anIndex) {
    return true;
  }
  void push(interface::TLBReqIn const &, index_t anIndex, TranslationPtr &aTranslate) {
    PhysicalMemoryAddress perfectPaddr(Qemu::API::qemu_callbacks.QEMU_logical_to_physical(
        *Flexus::Qemu::Processor::getProcessor(flexusIndex()),
        aTranslate->isInstr() ? Qemu::API::QEMU_DI_Instruction : Qemu::API::QEMU_DI_Data,
        aTranslate->theVaddr));
    if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(aTranslate->thePaddr == perfectPaddr)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 565, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(aTranslate->thePaddr == perfectPaddr)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "Translation mismatch. VA:" << aTranslate->theVaddr << ", PA:" << aTranslate->thePaddr << ", PerfectPaddr:" << perfectPaddr).str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
    if (!cfg.PerfectTLB &&
        (aTranslate->isInstr() ? theInstrTLB : theDataTLB).lookUp(aTranslate->theVaddr).first ==
            false) {
      if (!theMMUInitialized) {
        theMMU.reset(new mmu_t());
        theMMU->initRegsFromQEMUObject(getMMURegsFromQEMU((int)flexusIndex()));
        theMMU->setupAddressSpaceSizesAndGranules();
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran0->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 573, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran0->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG0 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        if (Flexus::Dbg::Debugger::theDebugger->theMinimumSeverity <= 6 && (1 || !1)) { if ((DBG_Cats::Assert_debug_enabled || DBG_Cats::MMU_debug_enabled) && !(theMMU->Gran1->getlogKBSize() == 12)) { using namespace DBG_Cats; using DBG_Cats::Core; Flexus::Dbg::Entry entry__( Flexus::Dbg::Severity(6), "components/MMU/MMUImpl.cpp", 574, __FUNCTION__, Flexus::Dbg::Debugger::theDebugger->count(), Flexus::Dbg::Debugger::theDebugger->cycleCount()); (void)(entry__).append("Condition", "!(theMMU->Gran1->getlogKBSize() == 12)") .set("Message", static_cast<std::stringstream &>(std::stringstream() << std::dec << "TG1 has non-4KB size - unsupported").str()); (entry__).addCategories(Assert | MMU); Flexus::Dbg::Debugger::theDebugger->process(entry__); } } do { } while (0) ;
        PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
        thePageWalker->setMMU(theMMU);
        theMMUInitialized = true;
      }
      (aTranslate->isInstr() ? theInstrTLB : theDataTLB)[aTranslate->theVaddr] =
          aTranslate->thePaddr;
      thePageWalker->push_back_trace(aTranslate,
                                     Flexus::Qemu::Processor::getProcessor((int)flexusIndex()));
    }
  }
};
} // End Namespace nMMU
MMUInterface * MMUInterface::instantiate( MMUConfiguration::cfg_struct_ &aCfg, MMUInterface::jump_table &aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth) { return new nMMU::MMUComponent(aCfg, aJumpTable, anIndex, aWidth); } struct eat_semicolon_;
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::dRequestIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::iRequestIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::ResyncIn const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::iTranslationReply const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::dTranslationReply const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::MemoryRequestOut const &) {
  return (cfg.Cores);
}
Flexus::Core::index_t MMUInterface::width( MMUInterface::configuration &cfg, MMUInterface::TLBReqIn const &) {
  return (cfg.Cores);
}
