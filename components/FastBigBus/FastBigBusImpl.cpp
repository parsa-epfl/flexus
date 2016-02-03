#include <components/FastBigBus/FastBigBus.hpp>

#include <fstream>
#include <unordered_map>
#include <boost/tuple/tuple.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/stats.hpp>

#include <core/simics/simics_interface.hpp>
#include <core/simics/mai_api.hpp>

#define FLEXUS_BEGIN_COMPONENT FastBigBus
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FastBigBus
#define DBG_SetDefaultOps AddCat(FastBigBus)
#include DBG_Control()

namespace nFastBigBus {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

typedef uint32_t block_address_t;

struct coherence_state_t {
  coherence_state_t() : sharers(0), state(0), owner(0) {}
  uint64_t sharers;
  uint16_t state;
  uint16_t owner;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & sharers;
    ar & state;
    ar & owner;
  }
};

std::ostream & operator<<(std::ostream & os, coherence_state_t & state) {
  os << std::hex << state.state << ":" << state.sharers << ":" << state.owner;
  return os;
}
std::istream & operator>>(std::istream & os, coherence_state_t & state) {
  char c1, c2;
  os >> std::hex >> state.state >> c1 >> state.sharers >> c2 >> state.owner;
  DBG_Assert( (c1 == ':') && (c2 == ':'), ( << "coherence_state_t did not found expected ':',':' found '" << c1 << "','" << c2 << "' instead"));
  return os;
}

uint16_t kInvalid = 0x0;
uint16_t kValid = 0x4;
uint16_t kExclusive = 0x8;
uint16_t kNAW = 0x2;
uint16_t kDMA = 0x1;
uint64_t kSharers = 0xFFFFFFFFFFFFFFFFULL;

#if 0
typedef uint32_t coherence_state_t;
coherence_state_t kInvalid = 0x00000000UL;
coherence_state_t kExclusive = 0x80000000UL;
coherence_state_t kNAW = 0x20000000UL;
coherence_state_t kDMA = 0x10000000UL;
coherence_state_t kOwner = 0x0000000FUL; //At most 16 nodes
coherence_state_t kSharers = 0x0000FFFFUL; //At most 16 nodes
#endif

const uint8_t DIR_STATE_INVALID   = 0;
const uint8_t DIR_STATE_SHARED    = 1;
const uint8_t DIR_STATE_EXCLUSIVE = 2;

uint32_t population(uint32_t x) {
  x = ((x & 0xAAAAAAAAUL) >> 1) + (x & 0x55555555UL);
  x = ((x & 0xCCCCCCCCUL) >> 2) + (x & 0x33333333UL);
  x = ((x & 0xF0F0F0F0UL) >> 4) + (x & 0x0F0F0F0FUL);
  x = ((x & 0xFF00FF00UL) >> 8) + (x & 0x00FF00FFUL);
  x = ((x & 0xFFFF0000UL) >> 16) + (x & 0x0000FFFFUL);
  return x;
}

enum SnoopSource {
  eSrcDMA,
  eSrcNAW,
  eSrcNormal,
};

class dir_entry_t {
public:
  dir_entry_t(uint16_t owner) : theWasModified(false) {
    state.state = kExclusive;
    state.owner = owner;
    thePastReaders = (1ULL << owner);
  }
  dir_entry_t(coherence_state_t s) : state(s), theWasModified(false), thePastReaders(s.sharers) {}
  dir_entry_t() : state(), theWasModified(false), thePastReaders(0) {}
  coherence_state_t state;
  bool theWasModified;
  uint64_t thePastReaders;
};

bool isSharer( coherence_state_t & state, index_t node) {
  return( state.sharers & (1ULL << node) );
}

void setSharer( coherence_state_t & state, index_t node) {
  state.sharers |= (1ULL << node);
}

bool isExclusive( coherence_state_t & state) {
  return( state.state & kExclusive );
}

struct IntHash {
  std::size_t operator()(uint32_t key) const {
    key = key >> 6;
    return key;
  }
};

uint32_t log_base2(uint32_t num) {
  uint32_t ii = 0;
  while (num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

class MemoryMap : public boost::counted_base {
public:
  MemoryMap(uint32_t page_size, unsigned num_nodes, bool round_robin)
    : thePageSize(page_size)
    , theNumNodes(num_nodes)
    , theRoundRobin(round_robin) {
    thePageSizeL2 = log_base2(thePageSize);
  }

  void loadState(std::string const & aDirName) {
    // read map from "page_map"
    std::string fname = aDirName + "/page_map.out";
    std::ifstream in(fname.c_str());
    if (in) {
      DBG_(Dev, ( << "Page map file page_map.out found.  Reading contents...") );
      int32_t count = 0;
      while (in) {
        int32_t node;
        int64_t addr;
        in >> node;
        in >> addr;
        if (in.good()) {
          DBG_(VVerb, ( << "Page " << addr << " assigned to node " << node ) );
          pagemap_t::iterator ignored;
          bool is_new;
          std::tie(ignored, is_new) = thePageMap.insert( std::make_pair(addr, node) );
          DBG_Assert(is_new, ( << "(" << addr << "," << node << ")"));
          ++count;
        }
      }
      DBG_(Dev, ( << "Assigned " << count << " pages."));
    } else {
      DBG_(Dev, ( << "Page map file page_map.out was not found."));
    }
  }

  void saveState(std::string const & aDirName) {
    // write page map to "page_map"
    std::string fname = aDirName + "/page_map.out";
    std::ofstream out(fname.c_str());
    if (out) {
      pagemap_t::iterator iter = thePageMap.begin();
      while (iter != thePageMap.end()) {
        out << iter->second << " " << static_cast<int64_t>(iter->first) << "\n";
        ++iter;
      }
      out.close();
    } else {
      DBG_(Crit, ( << "Unable to save page map to " << fname));
    }
  }

  int32_t node(unsigned req_node, block_address_t addr) {
    pagemap_t::iterator iter;
    bool is_new;
    block_address_t page_addr = addr >> thePageSizeL2;

    // 'insert', which won't really insert if its already there
    std::tie(iter, is_new) = thePageMap.insert( std::make_pair( page_addr, req_node ) ); //Optimized for first-touch

    // if found, return mapping
    if (is_new && theRoundRobin) {
      // overwrite the default first-touch
      iter->second = (page_addr) & (theNumNodes - 1);
    } // first-touch done by default in 'insert' call above

    return iter->second;
  }

private:
  uint32_t thePageSize;
  unsigned theNumNodes;
  bool theRoundRobin;

  uint32_t thePageSizeL2;

  typedef std::unordered_map<block_address_t, unsigned, IntHash > pagemap_t;
  pagemap_t thePageMap;
};

class FLEXUS_COMPONENT(FastBigBus) {
  FLEXUS_COMPONENT_IMPL( FastBigBus );

  typedef std::unordered_map<block_address_t, dir_entry_t, IntHash > directory_t;
  directory_t theDirectory;
  block_address_t theBlockMask;
  MemoryMessage theSnoopMessage;
  boost::intrusive_ptr<MemoryMap> theMemoryMap;

  Stat::StatCounter ** theUpgrades_S2X;
  Stat::StatCounter ** theUpgrades_X2X;
  Stat::StatCounter ** theConsumptions;
  Stat::StatCounter ** theDMABlockReads;
  Stat::StatCounter ** theDMABlockWrites;
  Stat::StatCounter ** theProductions;
  Stat::StatCounter ** theOffChipData;
  Stat::StatCounter ** theEvictions;
  Stat::StatCounter ** theFlushes;
  Stat::StatCounter ** theInvalidations;

  Stat::StatCounter theDMATransfers;
  Stat::StatCounter theDMABytes;
  Stat::StatUniqueCounter<uint32_t> theDMAAddresses;

  Stat::StatCounter theNonAllocateWrites;
  Stat::StatCounter theNonAllocateWriteBytes;
  Stat::StatUniqueCounter<uint32_t> theNonAllocateWriteAddresses;

  Stat::StatCounter theInvals;
  Stat::StatCounter theInvals_Valid;
  Stat::StatCounter theInvals_Dirty;

  Stat::StatCounter theInvals_Norm;
  Stat::StatCounter theInvals_Norm_Valid;
  Stat::StatCounter theInvals_Norm_Dirty;

  Stat::StatCounter theInvals_DMA;
  Stat::StatCounter theInvals_DMA_Valid;
  Stat::StatCounter theInvals_DMA_Dirty;

  Stat::StatCounter theInvals_NAW;
  Stat::StatCounter theInvals_NAW_Valid;
  Stat::StatCounter theInvals_NAW_Dirty;

  Stat::StatCounter theDowngrades;
  Stat::StatCounter theDowngrades_Dirty;

  Stat::StatCounter theDowngrades_Norm;
  Stat::StatCounter theDowngrades_Norm_Dirty;

  Stat::StatCounter theDowngrades_DMA;
  Stat::StatCounter theDowngrades_DMA_Dirty;

  Stat::StatCounter theOffChipReads_Fetch;
  Stat::StatCounter theOffChipReads_Prefetch;
  Stat::StatCounter theOffChipReads_Cold;
  Stat::StatCounter theOffChipReads_Replacement;
  Stat::StatCounter theOffChipReads_Coherence;
  Stat::StatCounter theOffChipReads_DMA;
  Stat::StatCounter theOffChipReads_NAW;

  Stat::StatInstanceCounter<int64_t> theDMAReadPCs;
  Stat::StatInstanceCounter<int64_t> theNAWReadPCs;

  std::vector<uint64_t> theLastStepCount_OffChipRead;
  std::vector<uint64_t> theLastStepCount_OffChipRead_Coherence;

  Stat::StatAverage theAvgInsPer_OffChipRead;
  Stat::StatStdDev  theStdDevInsPer_OffChipRead;
  Stat::StatAverage theAvgInsPer_OffChipRead_Coherence;
  Stat::StatStdDev  theStdDevInsPer_OffChipRead_Coherence;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastBigBus)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theSnoopMessage(MemoryMessage::Invalidate)
    , theDMATransfers("sys-bus-DMA:Transfers")
    , theDMABytes("sys-bus-DMA:Bytes")
    , theDMAAddresses("sys-bus-DMA:UniqueAddresses")
    , theNonAllocateWrites("sys-bus-NAW:Writes")
    , theNonAllocateWriteBytes("sys-bus-NAW:Bytes")
    , theNonAllocateWriteAddresses("sys-bus-NAW:UniqueAddresses")
    , theInvals("sys-bus-Invalidations:All")
    , theInvals_Valid("sys-bus-Invalidations:All:Valid")
    , theInvals_Dirty("sys-bus-Invalidations:All:Dirty")
    , theInvals_Norm("sys-bus-Invalidations:All:Normal")
    , theInvals_Norm_Valid("sys-bus-Invalidations:Normal:Valid")
    , theInvals_Norm_Dirty("sys-bus-Invalidations:Normal:Dirty")
    , theInvals_DMA("sys-bus-Invalidations:DMA")
    , theInvals_DMA_Valid("sys-bus-Invalidations:DMA:Valid")
    , theInvals_DMA_Dirty("sys-bus-Invalidations:DMA:Dirty")
    , theInvals_NAW("sys-bus-Invalidations:NAW")
    , theInvals_NAW_Valid("sys-bus-Invalidations:NAW:Valid")
    , theInvals_NAW_Dirty("sys-bus-Invalidations:NAW:Dirty")
    , theDowngrades("sys-bus-Downgrades:All")
    , theDowngrades_Dirty("sys-bus-Downgrades:All:Dirty")
    , theDowngrades_Norm("sys-bus-Downgrades:Normal")
    , theDowngrades_Norm_Dirty("sys-bus-Downgrades:Normal:Dirty")
    , theDowngrades_DMA("sys-bus-Downgrades:DMA")
    , theDowngrades_DMA_Dirty("sys-bus-Downgrades:DMA:Dirty")
    , theOffChipReads_Fetch("sys-bus-OffChipRead:Fetch")
    , theOffChipReads_Prefetch("sys-bus-OffChipRead:Prefetch")
    , theOffChipReads_Cold("sys-bus-OffChipRead:Cold")
    , theOffChipReads_Replacement("sys-bus-OffChipRead:Replacement")
    , theOffChipReads_Coherence("sys-bus-OffChipRead:Coherence")
    , theOffChipReads_DMA("sys-bus-OffChipRead:DMA")
    , theOffChipReads_NAW("sys-bus-OffChipRead:NAW")

    , theDMAReadPCs( "sys-bus-DMAReadPCs" )
    , theNAWReadPCs( "sys-bus-NAWReadPCs" )

    , theAvgInsPer_OffChipRead("sys-bus-PerIns:Avg:OffChipRead")
    , theStdDevInsPer_OffChipRead("sys-bus-PerIns:StdDev:OffChipRead")
    , theAvgInsPer_OffChipRead_Coherence("sys-bus-PerIns:Avg:OffChipRead:Coh")
    , theStdDevInsPer_OffChipRead_Coherence("sys-bus-PerIns:StdDev:OffChipRead:Coh")

  {}

  inline block_address_t blockAddress(PhysicalMemoryAddress addr) {
    return addr & theBlockMask;
  }

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
    unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    std::ofstream oa[width];

    for (unsigned i = 0; i < width; i++) {
      std::stringstream fname;
      fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflexV";
      oa[i].open(fname.str().c_str(), std::ofstream::out);
      DBG_Assert(oa[i].good(), ( << "Couldn't open " << fname.str() << " for writing"));
    }

    directory_t::iterator iter = theDirectory.begin();
    while (iter != theDirectory.end()) {
      unsigned node = theMemoryMap->node(0, iter->first);
      DBG_Assert(node >= 0 && node < width, ( << "node outside bounds: " << node));

      oa[node] << std::hex
               << "[" << iter->first << "] "
               << iter->second.state << " "
               << iter->second.theWasModified << " "
               << iter->second.thePastReaders << std::endl;

      ++iter;
    }

    for (unsigned i = 0; i < width; i++) {
      oa[i].close();
    }

    // save memory map
    if (cfg.SavePageMap) {
      theMemoryMap->saveState(aDirName);
    }
  }

  void loadState_TFlex(std::string const & aDirName) {
    unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    std::ifstream ia[width];

    for (unsigned i = 0; i < width; i++) {
      std::stringstream fname;
      fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflexV";
      ia[i].open(fname.str().c_str());
      if (!ia[i].good()) {
        DBG_(Crit, ( << "Couldn't open " << fname.str() << " for reading -- resetting to empty directory"));
        return;
      }
    }

    for (uint32_t i = 0; i < width; i++) {
      do {
        block_address_t addr;
        dir_entry_t entry;
        char paren;
        ia[i] >> std::hex
              >> paren >> addr >> paren
              >> entry.state
              >> entry.theWasModified
              >> entry.thePastReaders;
        if (!ia[i].eof()) {
          theDirectory.insert(std::make_pair(addr, entry));
        }
      } while (!ia[i].eof());

      ia[i].close();
    }

  }

  //Legacy load function for migratory coherence checkpoints
  void loadState_TFlex_Mig(std::string const & aDirName) {
    unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    std::ifstream ia[width];

    for (unsigned i = 0; i < width; i++) {
      std::stringstream fname;
      fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflexV-mig";
      ia[i].open(fname.str().c_str());
      if (!ia[i].good()) {
        DBG_(Crit, ( << "Couldn't open " << fname.str() << " for reading -- resetting to empty directory"));
        return;
      }
    }
    for (uint32_t i = 0; i < width; i++) {
      std::stringstream fname;
      fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflex-mig";
      DBG_( Dev, ( << "Loading from " << fname.str() ) );
      uint64_t last_addr = 1;
      do {
        uint64_t addr = 0;
        dir_entry_t entry;
        char paren;
        int64_t ignored;
        ia[i] >> std::hex;
        ia[i] >> paren >> addr >> paren;
        ia[i] >> entry.state;
        ia[i] >> entry.theWasModified;
        ia[i] >> ignored;
        ia[i] >> std::dec >> ignored;
        ia[i] >> std::hex >> entry.thePastReaders;
        if (!ia[i].eof()) {
          theDirectory.insert(std::make_pair(block_address_t(addr), entry));
        }
        DBG_Assert( addr != last_addr, ( << " Broken page_map line at " << fname << " addr=" << std::hex << addr ) );
        last_addr = addr;
      } while (!ia[i].eof());

      ia[i].close();
    }
    DBG_( Dev, ( << "Done Loading." ) );
  }

  void loadState(std::string const & aDirName) {
    // load memory map
    theMemoryMap->loadState(aDirName);

    std::ifstream check;
    std::stringstream fname;
    fname << aDirName << "/" << std::setw(2) << std::setfill('0') << 0 << "-directory-tflexV-mig";
    check.open(fname.str().c_str());
    if (check.good()) {
      check.close();
      loadState_TFlex_Mig(aDirName);
    } else {
      //Load old directory state
      loadState_TFlex(aDirName);
    }
  }

  // Initialization
  void initialize() {
    theBlockMask = ~(cfg.BlockSize - 1);
    DBG_Assert ( Flexus::Core::ComponentManager::getComponentManager().systemWidth()  <= 64, ( << "This implementation only supports 64 nodes" ) );
    theMemoryMap = new MemoryMap(cfg.PageSize, Flexus::Core::ComponentManager::getComponentManager().systemWidth(), cfg.RoundRobin);

    if (cfg.TrackWrites) {
      theUpgrades_S2X = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      theUpgrades_X2X = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theUpgrades_S2X[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Upgrades:S2X" );
        theUpgrades_X2X[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Upgrades:X2X" );
      }
    }
    if (cfg.TrackReads) {
      theConsumptions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theConsumptions[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Consumptions" );
      }
    }
    if (cfg.TrackDMA) {
      theDMABlockReads = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      theDMABlockWrites = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theDMABlockReads[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-DMABlockReads" );
        theDMABlockWrites[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-DMABlockWrites" );
      }
    }

    if (cfg.TrackProductions) {
      theProductions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theProductions[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Productions" );
      }
    }
    if (cfg.TrackEvictions) {
      theEvictions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theEvictions[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Evictions" );
      }
    }
    if (cfg.TrackFlushes) {
      theFlushes = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theFlushes[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Flushes" );
      }
    }
    if (cfg.TrackInvalidations) {
      theInvalidations = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theInvalidations[i] = new Stat::StatCounter( boost::padded_string_cast < 2, '0' > (i) + "-bus-Invalidations" );
      }
    }

    for (uint32_t i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
      theLastStepCount_OffChipRead.push_back( Simics::Processor::getProcessor(i)->stepCount() );
      theLastStepCount_OffChipRead_Coherence.push_back( Simics::Processor::getProcessor(i)->stepCount() );
    }
  }

  // Ports

  void production(MemoryMessage & aMessage, index_t aNode) { //not precise unless cache is infinite - EvictDirty may mask productions
    if (! cfg.TrackProductions) {
      return;
    }
    ++(*theProductions[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
  }

  void upgrade_S2X(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackWrites) {
      return;
    }
    ++(*theUpgrades_S2X[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Writes) << aMessage;
  }

  void upgrade_X2X(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackWrites) {
      return;
    }
    ++(*theUpgrades_X2X[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Writes) << aMessage;
  }

  void eviction(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackEvictions) {
      return;
    }
    ++(*theEvictions[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Evictions) << aMessage;
  }

  void flush(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackFlushes) {
      return;
    }
    ++(*theFlushes[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode;
    }
    FLEXUS_CHANNEL(Flushes) << aMessage;
  }

  void invalidation(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackInvalidations) {
      return;
    }
    ++(*theInvalidations[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Invalidations) << aMessage;
  }

  void offChipRead(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackReads) {
      return;
    }

    uint64_t step_count = Simics::Processor::getProcessor(aNode)->stepCount();
    uint64_t cycles_since_last = step_count - theLastStepCount_OffChipRead[aNode];
    theLastStepCount_OffChipRead[aNode] = step_count;
    theAvgInsPer_OffChipRead << cycles_since_last;
    theStdDevInsPer_OffChipRead << cycles_since_last;

    aMessage.address() = PhysicalMemoryAddress(aMessage.address() & ~63);
    switch (aMessage.fillType()) {
      case eCold:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Cold" ));
        ++theOffChipReads_Cold;
        break;
      case eReplacement:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Repl" ));
        ++theOffChipReads_Replacement;
        break;
      case eCoherence:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Coh" ));
        cycles_since_last = step_count - theLastStepCount_OffChipRead_Coherence[aNode];
        theLastStepCount_OffChipRead_Coherence[aNode] = step_count;
        theAvgInsPer_OffChipRead_Coherence << cycles_since_last;
        theStdDevInsPer_OffChipRead_Coherence << cycles_since_last;
        ++theOffChipReads_Coherence;
        break;
      case eDMA:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " DMA" ));
        ++theOffChipReads_DMA;
        break;
      case eNAW:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " NAW" ));
        ++theOffChipReads_NAW;
        break;
      case ePrefetch:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Pref" ));
        ++theOffChipReads_Prefetch;
        break;
      case eFetch:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Fetch" ));
        ++theOffChipReads_Fetch;
        break;
      default:
        DBG_Assert(false);
    }
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    if (aMessage.fillType() == eFetch) {
      FLEXUS_CHANNEL(Fetches) << aMessage;
    } else {
      FLEXUS_CHANNEL(Reads) << aMessage;
    }
  }

  void dmaRead(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackDMA) {
      return;
    }
    ++(*theDMABlockReads[aNode]);
  }
  void dmaWrite(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackDMA) {
      return;
    }
    ++(*theDMABlockWrites[aNode]);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromCaches);
  void push(interface::FromCaches const &, index_t anIndex, MemoryMessage & message) {
    DBG_(Iface, Addr(message.address()) ( << "request from node " << anIndex << " received: " << message ) );

    //Get the block address
    uint32_t blockAddr = blockAddress(message.address());

    // not used yet, but we should get the page mapping right
    theMemoryMap->node(anIndex, blockAddr);

    message.fillLevel() = eLocalMem;
    message.fillType() = eCold;

    //Lookup or insert the new block in the directory
    directory_t::iterator iter;
    bool is_new;
    std::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(anIndex) ) ); //Optimized for exclusive case - avoids shift operation on writes.
    if (is_new) {
      //New block
      switch (message.type()) {
        case MemoryMessage::ReadReq:
          message.fillType() = eCold;
          goto label_new_read_cases;
        case MemoryMessage::LoadReq:
          message.fillType() = eCold;
          goto label_new_read_cases;
        case MemoryMessage::FetchReq:
          message.fillType() = eFetch;
          goto label_new_read_cases;
        case MemoryMessage::PrefetchReadNoAllocReq:
          message.fillType() = ePrefetch;
          goto label_new_read_cases;
        case MemoryMessage::PrefetchReadAllocReq:
          message.fillType() = ePrefetch;
          goto label_new_read_cases;
label_new_read_cases:
          offChipRead(message, anIndex);
          if (message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
            message.type() = MemoryMessage::PrefetchReadReply;
          } else {
            message.type() = MemoryMessage::MissReply;
          }
          message.fillLevel() = eLocalMem;
          //iter->second.state = (1UL << anIndex); //Set block state to shared with this node as sharer;
          iter->second.state.sharers = (1ULL << anIndex); //Set block state to shared with this node as sharer;
          iter->second.state.state = kValid;
          iter->second.thePastReaders |= (1ULL << anIndex);
          DBG_(VVerb, Addr(message.address()) ( << "Read new " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          break;

        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          message.type() = MemoryMessage::MissReplyWritable;
          message.fillLevel() = eLocalMem;
          //Block state initialized to exclusive with the correct owner in insert above
          iter->second.theWasModified = true;
          DBG_(VVerb, Addr(message.address()) ( << "Write new " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          upgrade_S2X(message, anIndex);
          break;
        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
        case MemoryMessage::FlushReq:
        default:
          DBG_Assert(false, ( << "request from node " << anIndex << " received: " << message));
      }

    } else {
      //Existing block
      //uint32_t & state = iter->second.state;
      coherence_state_t & state = iter->second.state;
      bool counted = false;
      switch (message.type()) {
        case MemoryMessage::ReadReq:
          goto label_existing_read_cases;
        case MemoryMessage::LoadReq:
          goto label_existing_read_cases;
        case MemoryMessage::FetchReq:
          message.fillType() = eFetch;
          counted = true;
          goto label_existing_read_cases;
        case MemoryMessage::PrefetchReadNoAllocReq:
          message.fillType() = ePrefetch;
          counted = true;
          goto label_existing_read_cases;
        case MemoryMessage::PrefetchReadAllocReq:
          message.fillType() = ePrefetch;
          counted = true;
          goto label_existing_read_cases;
label_existing_read_cases:
          if (state.state & kExclusive) {
            if ( (state.owner) == anIndex) {
              //Read request by current owner -- how did we get here?
              //DBG_Assert(message.type() != MemoryMessage::ReadReq, ( << "Read from owner? " << message << " Addr: " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
              if (message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
                message.type() = MemoryMessage::PrefetchWritableReply;
              } else {
                message.type() = MemoryMessage::MissReplyWritable;
              }
              message.fillLevel() = eLocalMem;
              if (!counted) {
                message.fillType() = eReplacement;
              }
            } else {
              //Read request by new node.
              if (!counted) {
                message.fillType() = eCoherence;
              }
              downgrade(state, blockAddr);
              state.sharers = (1ULL << (state.owner)) | (1ULL << (anIndex));
              state.state = kValid;
              iter->second.thePastReaders |= (1ULL << anIndex);
              if (message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
                message.type() = MemoryMessage::PrefetchReadReply;
              } else {
                message.type() = MemoryMessage::MissReply;
              }
              message.fillLevel() = eRemoteMem;
              DBG_(VVerb, Addr(message.address()) ( << "Read downgrade " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
              production(message, anIndex); //not precise
            }
          } else {
            if (state.state & kDMA) {
              //Don't clear DMA bit on read
              dmaRead(message, anIndex);
              message.fillType() = eDMA;
              counted = true;
            } else if (state.state & kNAW) {
              //Don't clear NAW bit on read
              message.fillType() = eNAW;
              counted = true;
            }
            message.fillLevel() = eLocalMem;

            //Read in shared state
            if (message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
              message.type() = MemoryMessage::PrefetchReadReply;
            } else {
              message.type() = MemoryMessage::MissReply;
            }
            if (! (state.sharers & (1ULL << anIndex)) ) {
              if (!counted) {
                message.fillType() = eCoherence;
              }
            } else if (!counted) {
              message.fillType() = eReplacement;
            }
            state.sharers |= (1ULL << (anIndex));
            iter->second.thePastReaders |= (1ULL << (anIndex));
            DBG_(VVerb, Addr(message.address()) ( << "Read shared " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          }
          offChipRead(message, anIndex);
          // DBG_Assert(message.type() == MemoryMessage::MissReply, (<< "message.type() not set to MissReply: " << message));
          break;
        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          if (state.state & kExclusive) {
            if ( (state.owner) != anIndex) {
              //Write request by new node.
              if (cfg.InvalAll) {
                invalidate(kSharers & (~(1ULL << anIndex)), blockAddr);
              } else {
                invalidateOne(state.owner, blockAddr);
              }
              //Note - implicitly clears DMA/NAW bits
              //state = kExclusive | anIndex;
              state.state = kExclusive;
              state.owner = anIndex;
              state.sharers = 0;
              iter->second.theWasModified = true;
              iter->second.thePastReaders |= (1ULL << anIndex);
              upgrade_X2X(message, anIndex);
              message.fillLevel() = eRemoteMem;
            } else {
              message.fillLevel() = eLocalMem;
            }
            message.type() = MemoryMessage::MissReplyDirty;
          } else {
            if (state.state & kDMA) {
              state.state &= (~kDMA); //clear DMA bit
              message.fillType() = eDMA;
              dmaWrite(message, anIndex);
            } else if (state.state & kNAW) {
              state.state &= (~kNAW); //clear NAW bit
              message.fillType() = eNAW;
              counted = true;
            }
            message.fillLevel() = eRemoteMem;

            //Write in shared state
            bool anyInvs = false;

            if (cfg.InvalAll) {
              anyInvs = invalidate(kSharers & (~(1ULL << anIndex)), blockAddr);
            } else {
              anyInvs = invalidate(state.sharers & kSharers & (~(1ULL << anIndex)), blockAddr); //Do not invalidate the writer
            }
            if (anyInvs) message.setInvs();
            message.type() = MemoryMessage::MissReplyDirty; // why was this set to MissReply?
            //state = kExclusive | anIndex;
            state.state = kExclusive;
            state.owner = anIndex;
            state.sharers = 0;
            iter->second.theWasModified = true;
            iter->second.thePastReaders |= (1ULL << anIndex);
            upgrade_S2X(message, anIndex);
          }
          break;
        case MemoryMessage::EvictDirty:
          //Better be exclusive, better be owned by anIndex
          DBG_Assert (  (state.state & kExclusive) && (state.owner == anIndex), ( << "Block: " << std::hex << blockAddr << " state: " << state << std::dec ));
          state.state = kValid;
          state.sharers = (1ULL << anIndex);
          //state = (1 << anIndex ); //Shared, spurious sharer optimization
          // no break -- continue to next case
        case MemoryMessage::EvictWritable: // invariant: $ hierarchy still has dirty token
        case MemoryMessage::EvictClean:
          eviction(message, anIndex);
          break;
        case MemoryMessage::FlushReq:
          //state = (1 << anIndex );
          state.sharers = (1ULL << anIndex );
          state.state = kValid;
          flush(message, anIndex);
          break;
        default:
          DBG_Assert(false);
      }

    }

    DBG_(Iface, Addr(message.address()) ( << "done request, replying: " << message ) );
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(NonAllocateWrite);
  void push(interface::NonAllocateWrite const &, index_t anIndex,  MemoryMessage & message) {
    DBG_(Verb, Addr(message.address()) ( << "Non-allocating Write from node: " << anIndex << " message: " << message ) );

    //Get the block address
    uint32_t blockAddr = blockAddress(message.address());

    ++theNonAllocateWrites;
    theNonAllocateWriteBytes += message.reqSize();
    theNonAllocateWriteAddresses << blockAddr;

    //Lookup or insert the new block in the directory
    directory_t::iterator iter;
    bool is_new;
    std::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(kNAW) ) );

    if (! is_new) {
      coherence_state_t & state = iter->second.state;

      if (state.state & kExclusive) {
        if ( (state.owner) != anIndex) {
          //Non-allocating write by non-owner
          if (cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcNAW);
          } else {
            invalidateOne(state.owner, blockAddr, eSrcNAW);
          }
          //state = 0;  // don't even set anIndex as a sharer
          state.state = kInvalid;
          state.sharers = 0;
          state.owner = 0;
          iter->second.theWasModified = true;
          iter->second.thePastReaders |= (1ULL << anIndex);
        } else {
          //No state transaction action if the current owner uses a non-a
        }
        state.state |= kNAW;
      } else {
        //Write in shared state
        if (cfg.InvalAll) {
          invalidate(kSharers, blockAddr, eSrcNAW);
        } else {
          invalidate(state.sharers, blockAddr, eSrcNAW);
        }
        state.state = kNAW;
        state.sharers = 0;
        state.owner = 0;
      }
    }

  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(DMA);
  void push(interface::DMA const &, MemoryMessage & message) {

    //Get the block address
    uint32_t blockAddr = blockAddress(message.address());

    ++theDMATransfers;
    theDMABytes += message.reqSize();
    theDMAAddresses << blockAddr;

    if (message.type() == MemoryMessage::WriteReq) {
      //Lookup or insert the new block in the directory
      directory_t::iterator iter;
      bool is_new;
      std::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(kDMA) ) );
      if (! is_new) {
        coherence_state_t & state = iter->second.state;

        if (state.state & kExclusive) {
          if (cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcDMA);
          } else {
            invalidateOne(state.owner, blockAddr, eSrcDMA);
          }
          state.sharers = 0;
          state.owner = 0;
          state.state = kDMA;
        } else {
          //Write in shared state
          if (cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcDMA);
          } else {
            invalidate(state.sharers, blockAddr, eSrcDMA); //Do not invalidate the writer
          }
          state.sharers = 0;
          state.owner = 0;
          state.state = kDMA;
        }

      }

    } else {
      DBG_Assert( message.type() == MemoryMessage::ReadReq );

      //Lookup or insert the new block in the directory
      directory_t::iterator iter = theDirectory.find( blockAddr );
      if (iter != theDirectory.end()) {
        coherence_state_t & state = iter->second.state;

        if (state.state & kExclusive) {
          downgrade(state, blockAddr, eSrcDMA);
          state.sharers = (1ULL << (state.owner) ); //Shared, spurious sharer optimization
          state.state = kValid;
        } else {
          //Take no action
        }

      }

    }
  }

private:

  void downgrade(coherence_state_t & state, block_address_t address, SnoopSource src = eSrcNormal) {
    theSnoopMessage.type() = MemoryMessage::Downgrade;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);

    FLEXUS_CHANNEL_ARRAY( ToSnoops, state.owner) << theSnoopMessage;
    countDowngrade( theSnoopMessage.type(), src);
  }

  bool invalidate(uint64_t sharers, block_address_t address, SnoopSource src = eSrcNormal) {
    bool anyInvs = false;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);
    if (!sharers) {
      return anyInvs;
    }
    int32_t width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    uint64_t bit = 1;
    for (int32_t i = 0; i < width; ++i) {
      if (sharers & bit) {
        theSnoopMessage.type() = MemoryMessage::Invalidate;
        invalidation(theSnoopMessage, i);
        FLEXUS_CHANNEL_ARRAY( ToSnoops, i) << theSnoopMessage;
        countInvalidation(theSnoopMessage.type(), src);
        sharers &= ~bit;
        if (!sharers) {
          return anyInvs;
        }
      }
      bit <<= 1;
    }
    return anyInvs;
  }

  void invalidateOne(index_t aNode, block_address_t address, SnoopSource src = eSrcNormal) {
    theSnoopMessage.type() = MemoryMessage::Invalidate;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);
    invalidation(theSnoopMessage, aNode);
    FLEXUS_CHANNEL_ARRAY( ToSnoops, aNode) << theSnoopMessage;
    countInvalidation(theSnoopMessage.type(), src);
  }

  void countDowngrade( MemoryMessage::MemoryMessageType type, SnoopSource src) {
    ++theDowngrades;
    if (theSnoopMessage.type() == MemoryMessage::DownUpdateAck) {
      ++theDowngrades_Dirty;
      switch (src) {
        case eSrcNormal:
          ++theDowngrades_Norm;
          ++theDowngrades_Norm_Dirty;
          break;
        case eSrcDMA:
          ++theDowngrades_DMA;
          ++theDowngrades_DMA_Dirty;
          break;
        case eSrcNAW:
          DBG_Assert(false);
          break;
      };
    } else {
      switch (src) {
        case eSrcNormal:
          ++theDowngrades_Norm;
          break;
        case eSrcDMA:
          ++theDowngrades_DMA;
          break;
        case eSrcNAW:
          DBG_Assert(false);
          break;
      };
    }
  }

  void countInvalidation( MemoryMessage::MemoryMessageType type, SnoopSource src) {
    ++theInvals;
    if (theSnoopMessage.type() == MemoryMessage::InvalidateAck) {
      ++theInvals_Valid;
      switch (src) {
        case eSrcNormal:
          ++theInvals_Norm;
          ++theInvals_Norm_Valid;
          break;
        case eSrcDMA:
          ++theInvals_DMA;
          ++theInvals_DMA_Valid;
          break;
        case eSrcNAW:
          ++theInvals_NAW;
          ++theInvals_NAW_Valid;
          break;
      };
    } else if (theSnoopMessage.type() == MemoryMessage::InvUpdateAck) {
      ++theInvals_Dirty;
      switch (src) {
        case eSrcNormal:
          ++theInvals_Norm;
          ++theInvals_Norm_Dirty;
          break;
        case eSrcDMA:
          ++theInvals_DMA;
          ++theInvals_DMA_Dirty;
          break;
        case eSrcNAW:
          ++theInvals_NAW;
          ++theInvals_NAW_Dirty;
          break;
      };
    } else {
      switch (src) {
        case eSrcNormal:
          ++theInvals_Norm;
          break;
        case eSrcDMA:
          ++theInvals_DMA;
          break;
        case eSrcNAW:
          ++theInvals_NAW;
          break;
      };
    }
  }

};

}//End namespace nFastBigBus

FLEXUS_COMPONENT_INSTANTIATOR( FastBigBus, nFastBigBus);
FLEXUS_PORT_ARRAY_WIDTH( FastBigBus, ToSnoops ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( FastBigBus, FromCaches ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH( FastBigBus, NonAllocateWrite ) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastBigBus

#define DBG_Reset
#include DBG_Control()
