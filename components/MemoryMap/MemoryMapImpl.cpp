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


#include <components/MemoryMap/MemoryMap.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryMap
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories MemMap
#define DBG_SetDefaultOps AddCat(MemMap)
#include DBG_Control()

#include <iomanip>
#include <fstream>

#include <boost/shared_array.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/types.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/stats.hpp>

#include <components/CommonQEMU/MemoryMap.hpp>

namespace nMemoryMap {

using namespace Flexus;
using namespace Flexus::SharedTypes;
typedef uint32_t node_id_t;

template <class MemoryMapComponent>
struct MemoryMapImpl : public MemoryMap {
  MemoryMapComponent & theMemoryMap;
  node_id_t theRequestingComponent;
  bool theRequestingComponent_isValid;

  MemoryMapImpl(MemoryMapComponent & aMemoryMap)
    : theMemoryMap(aMemoryMap)
    , theRequestingComponent_isValid(false)
  {}

  MemoryMapImpl(MemoryMapComponent & aMemoryMap, node_id_t aRequestingComponent)
    : theMemoryMap(aMemoryMap)
    , theRequestingComponent(aRequestingComponent)
    , theRequestingComponent_isValid(true)
  {}

  virtual bool isCacheable(PhysicalMemoryAddress const & anAddress) {
    return theMemoryMap.isCacheable(anAddress);
  }
  virtual bool isMemory(PhysicalMemoryAddress const & anAddress) {
    return theMemoryMap.isMemory(anAddress);
  }
  virtual bool isIO(PhysicalMemoryAddress const & anAddress) {
    return theMemoryMap.isIO(anAddress);
  }
  virtual node_id_t node(PhysicalMemoryAddress const & anAddress) {
    return theMemoryMap.node(anAddress, theRequestingComponent);
  }
  virtual void loadState(std::string const & aDirName) {
    theMemoryMap.loadState(aDirName);
  }

  virtual void recordAccess(PhysicalMemoryAddress const & anAddress, AccessType aType) {
#ifdef TRACK_ACCESSES
    theRequestingComponent_isValid ? theMemoryMap.recordAccess(anAddress, aType, theRequestingComponent) : theMemoryMap.recordAccess(anAddress, aType) ;
#endif
  }

};

//Initialized upon construction of the MemoryMapComponent
class MemoryMapFactory;
MemoryMapFactory * theMemoryMapFactory = 0;

//The MemoryMapFactory knows how to create MemoryMap objects
struct MemoryMapFactory {
  virtual ~MemoryMapFactory() { }
  MemoryMapFactory() {
    DBG_Assert( (theMemoryMapFactory == 0) );
    theMemoryMapFactory = this;
  }
  virtual boost::intrusive_ptr<MemoryMap> createMemoryMap() = 0;
  virtual boost::intrusive_ptr<MemoryMap> createMemoryMap(node_id_t aRequestingNode) = 0;
};

struct MemoryPage {
private:
  typedef uint32_t access_count_array[MemoryMap::NumAccessTypes];
  node_id_t theNumCPUs;
  boost::shared_array< access_count_array > theAccessCounts;
public:
  friend std::ostream & operator<<(std::ostream & anOstream, MemoryPage const & aMemoryPage);

  void initialize(node_id_t aNumCPUs) {
    theNumCPUs = aNumCPUs;
    theAccessCounts.reset(new access_count_array[aNumCPUs + 1]);
    for (uint32_t i = 0; i < MemoryMap::NumAccessTypes; ++i) {
      for (uint32_t j = 0; j < aNumCPUs + 1; ++j) {
        theAccessCounts[j][i] = 0;
      }
    }
  }

  void finalize() {}

  void increment(MemoryMap::AccessType anAccessType, node_id_t aNode) {
    ++theAccessCounts[aNode][anAccessType];
  }
};

std::ostream & operator<<(std::ostream & anOstream, MemoryPage const & aMemoryPage) {
  for (int32_t i = 0; i < MemoryMap::NumAccessTypes; ++i) {
    anOstream << "  " << MemoryMap::AccessType_toString(MemoryMap::AccessType(i)) << ':';
    for (uint32_t j = 0; j < aMemoryPage.theNumCPUs + 1; ++j) {
      anOstream << " " << std::setw(4) << aMemoryPage.theAccessCounts[j][i];
    }
    anOstream << std::endl;
  }
  return anOstream;
}

typedef std::map<PhysicalMemoryAddress, MemoryPage> PageMap;

class PageMap_QemuObject_Impl  {
  PageMap * thePageMap; //Non-owning pointer

public:
  PageMap_QemuObject_Impl(Qemu::API::conf_object_t * /*ignored*/ )
    : thePageMap(0)
  {}

  void setPageMap(PageMap & aPageMap) {
    thePageMap = &aPageMap;
  }

  void printStats() {
    if (thePageMap) {
      PageMap::iterator iter = thePageMap->begin();
      while (iter != thePageMap->end()) {
        PhysicalMemoryAddress base_address = iter->first;
        std::cout << "Page @" << std::hex << base_address << std::endl;
        std::cout << iter->second;
        ++iter;
      }
    }
  }
};

class PageMap_QemuObject : public Qemu::AddInObject <PageMap_QemuObject_Impl> {
  typedef Qemu::AddInObject<PageMap_QemuObject_Impl> base;
public:
  static const Qemu::Persistence  class_persistence = Qemu::Session;
  //These constants are defined in Simics/simics.cpp
  static std::string className() {
    return "MemoryMap";
  }
  static std::string classDescription() {
    return "Flexus MemoryMap object";
  }

  PageMap_QemuObject() : base() { }
  PageMap_QemuObject(Qemu::API::conf_object_t * aQemuObject) : base(aQemuObject) {}
  PageMap_QemuObject(PageMap_QemuObject_Impl * anImpl) : base(anImpl) {}

  template <class Class>
  static void defineClass(Class & aClass) {

//ALEX - This is adding functions to Simics' Command Line Interface.
//Disabled it, as the interface with QEMU is probably completely different
/*    aClass.addCommand
    ( & PageMap_QemuObject_Impl::printStats
      , "print-stats"
      , "Print out memory map statistics"
    );
*/
  }

};

Qemu::Factory<PageMap_QemuObject> thePageMapFactory;

class FLEXUS_COMPONENT(MemoryMap), public MemoryMapFactory {
  FLEXUS_COMPONENT_IMPL(MemoryMap);

  PageMap thePageMap;
  PageMap_QemuObject thePageMapObject;
  uint32_t theNumCPUs;
  uint32_t theNode_shift;
  uint32_t theNode_mask;

  bool theMapLoaded;

  // for first touch allocation
  typedef std::map<PhysicalMemoryAddress, node_id_t> HomeMap;
  HomeMap theHomeMap;
  std::unique_ptr< std::ofstream > theHomeMapFile;
  std::vector<boost::intrusive_ptr<Flexus::Stat::StatCounter> > thePageCounts;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MemoryMap)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  bool isQuiesced() const {
    return true; //MemoryMap is always quiesced
  }

  void saveState(std::string const & aDirName) {
    writePageMap( aDirName + "/page_map.out");
  }

  void loadState(std::string const & aDirName) {
    readPageMap( aDirName + "/page_map.out");  // reads from ckpt directory
  }

  // Initialization
  void initialize() {
    //Ensure that PageSize is a non-zero power of 2
    DBG_Assert((cfg.PageSize != 0), Comp(*this));
    DBG_Assert((((cfg.PageSize - 1) & cfg.PageSize) == 0), Comp(*this));
    DBG_Assert((((cfg.NumNodes - 1) & cfg.NumNodes) == 0), Comp(*this));
    theNumCPUs = cfg.NumNodes;
    thePageMapObject = thePageMapFactory.create(cfg.name());
    thePageMapObject->setPageMap(thePageMap);
    theMapLoaded = false;
    int32_t temp = cfg.PageSize;
    int32_t page_size_log2 = 0;
    while (temp) {
      ++page_size_log2;
      temp >>= 1;
    }
    --page_size_log2;

    theNode_shift = page_size_log2;
    theNode_mask = (cfg.NumNodes - 1);

    for (unsigned i = 0; i < theNumCPUs; i++) {
      thePageCounts.push_back(new Flexus::Stat::StatCounter(std::string(boost::padded_string_cast < 3, '0' > (i) + "-memory-Pages")));
    }

    if (cfg.ReadPageMap) {
      readPageMap("page_map.out");  // reads from current directory
    }

    if (cfg.CreatePageMap) {
      if (cfg.ReadPageMap) {
        theHomeMapFile.reset( new std::ofstream("page_map.out", std::ios::out | std::ios::app ) );
      } else {
        theHomeMapFile.reset( new std::ofstream("page_map.out", std::ios::out ) );
      }
    }
  }

  void finalize() {}

  void writePageMap(std::string const & aFilename) {
    std::ofstream out(aFilename.c_str());
    if (out) {
      HomeMap::iterator iter = theHomeMap.begin();
      HomeMap::iterator end = theHomeMap.end();
      while (iter != end) {
        out << iter->second << " " << static_cast<int64_t>(iter->first) << "\n";
        ++iter;
      }
    } else {
      DBG_(Crit, ( << "Unable to save page map to " << aFilename) );
    }
  }

  void readPageMap(std::string const & aFilename) {
    DBG_Assert(!theMapLoaded, ( << "Attempted to load page map twice (" << aFilename << ")"));
    std::ifstream in(aFilename.c_str());
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
          HomeMap::iterator ignored;
          bool is_new;
          std::tie(ignored, is_new) = theHomeMap.insert( {PhysicalMemoryAddress(addr), node_id_t(node)} );
          DBG_Assert(is_new);
          ++count;
        }
      }
      theMapLoaded = true;

      DBG_(Dev, ( << "Assigned " << count << " pages."));
    } else {
      DBG_(Dev, ( << "Page map file page_map.out was not found.") );
    }
  }

  virtual ~MemoryMapComponent() {} //Eliminate warning about virtual destructor

  virtual boost::intrusive_ptr<MemoryMap> createMemoryMap() {
    return boost::intrusive_ptr<MemoryMap>(new MemoryMapImpl<self>(*this));
  }

  virtual boost::intrusive_ptr<MemoryMap> createMemoryMap(node_id_t aRequestingNode) {
    return boost::intrusive_ptr<MemoryMap> (new MemoryMapImpl<self>(*this, aRequestingNode));
  }

  bool isCacheable(PhysicalMemoryAddress const & anAddress) {
    return true;
  }

  bool isMemory(PhysicalMemoryAddress const & anAddress) {
    return true;
  }

  bool isIO(PhysicalMemoryAddress const & anAddress) {
    return true;
  }

  node_id_t newPage(PhysicalMemoryAddress const & aPageAddr, const node_id_t aRequestingNode) {
    node_id_t node = aRequestingNode;
    if (cfg.RoundRobin) {
      node = static_cast<node_id_t>( (aPageAddr) & theNode_mask );
    }
    DBG_Assert((node < cfg.NumNodes), Comp(*this));

    // first touch - allocate page to the round-robin owner (based on address)

    (*thePageCounts[node])++;
    std::pair<HomeMap::iterator, bool> insert_result = theHomeMap.insert( std::make_pair(aPageAddr, node) );
    DBG_Assert(insert_result.second);

    if (cfg.CreatePageMap) {
      (*theHomeMapFile) << node << " " << static_cast<int64_t>(aPageAddr) << "\n";
      theHomeMapFile->flush();
    }

    return node;
  }

  node_id_t node(PhysicalMemoryAddress const & anAddress, const node_id_t aRequestingNode) {

    PhysicalMemoryAddress pageAddr = PhysicalMemoryAddress(anAddress >> theNode_shift);

    HomeMap::iterator iter = theHomeMap.find(pageAddr);
    if (iter != theHomeMap.end()) {
      // this is not the first touch - we know which node this page belongs to
      DBG_Assert((iter->second < cfg.NumNodes), Comp(*this));

      // STUPID TRUSS HACK: if aRequestingNode is > NumNodes, this is
      // TRUSS.  More specifically, if aRequestingNode % NumNodes ==
      // iter->second, then this is a slave node accessing it's local
      // memory, just like the master node told it to.  So we can't
      // return the master node (# < NumNodes), we need to return
      // aRequestingNode

      return aRequestingNode % cfg.NumNodes != iter->second ? iter->second : aRequestingNode;
    } else {
      return newPage( pageAddr, aRequestingNode);
    }
  }

  void recordAccess(PhysicalMemoryAddress const & anAddress, MemoryMap::AccessType aType) {
    //Accesses that come from theNumCPUs come from an "Unknown node"
    recordAccess(anAddress, aType, theNumCPUs );
  }

  void recordAccess(PhysicalMemoryAddress const & anAddress, MemoryMap::AccessType aType, node_id_t anAccessingComponent) {
#ifdef TRACK_ACCESSES
    DBG_(Iface, ( << "Access by " << anAccessingComponent << " of " << MemoryMap::AccessType_toString(aType) << " @" << & std::hex << anAddress << & std::dec ) Addr(anAddress) Comp(*this));

    //Get page base address
    PhysicalMemoryAddress page_base(anAddress & ~(cfg.PageSize - 1));

    //Get the new or existing page object
    pair<PageMap::iterator, bool> insert_result = thePageMap.insert( std::make_pair(page_base, MemoryPage()) );
    if (insert_result.second) {
      //If the new page was inserted
      insert_result.first->second.initialize(theNumCPUs);
    }

    //Increment the access count
    insert_result.first->second.increment(aType, anAccessingComponent);

#endif //TRACK_ACCESSES
  }

};

} //End Namespace nMemoryMap

namespace Flexus {
namespace SharedTypes {
boost::intrusive_ptr<MemoryMap> MemoryMap::getMemoryMap() {
  DBG_Assert(nMemoryMap::theMemoryMapFactory != 0);
  return nMemoryMap::theMemoryMapFactory->createMemoryMap();
}
boost::intrusive_ptr<MemoryMap> MemoryMap::getMemoryMap(node_id_t aRequestingNode) {
  DBG_Assert(nMemoryMap::theMemoryMapFactory != 0);
  return nMemoryMap::theMemoryMapFactory->createMemoryMap(aRequestingNode);
}
} //end SharedTypes
} //end Flexus

FLEXUS_COMPONENT_INSTANTIATOR( MemoryMap, nMemoryMap );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MemoryMap

#define DBG_Reset
#include DBG_Control()

