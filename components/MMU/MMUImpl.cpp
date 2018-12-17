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

// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/MMU/MMU.hpp>

#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>

#include "components/TLBControllers/TLBController.hpp"
#include <components/CommonQEMU/Slices/Translation.hpp>

#include <core/qemu/mai_api.hpp>

#include <map>
#include <queue>
#include <vector>

#define DBG_DefineCategories MMU
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()
#define DBG_SetDefaultOps AddCat(MMU)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT MMU 
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nMMU {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace nCache;

using std::unique_ptr;

class FLEXUS_COMPONENT(MMU) {
  FLEXUS_COMPONENT_IMPL(MMU);

private:

  std::unique_ptr<CacheController> theController; //Deleted automagically when the cache goes away

  Flexus::Stat::StatCounter theBusDeadlocks;


  typedef std::map<VirtualMemoryAddress, PhysicalMemoryAddress> TLBentry;
  TLBentry theInstrTLB;
  TLBentry theDataTLB;

  std::vector<boost::intrusive_ptr<Translation>> thePageWalkEntries;
  std::vector<boost::intrusive_ptr<Translation>> theiLookUpEntries, thedLookUpEntries;

  Flexus::Qemu::Processor theCPU;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MMU)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBusDeadlocks( statName() + "-BusDeadlocks" )
    , theBusDirection(kIdle)
  {}

  enum eDirection {
    kIdle
    , kToBackSideIn_Request
    , kToBackSideIn_Reply
    , kToBackSideOut_Request
    , kToBackSideOut_Snoop
    , kToBackSideOut_Reply
  };

  uint32_t theBusTxCountdown;
  eDirection theBusDirection;
  MemoryTransport theBusContents;
  boost::optional< MemoryTransport> theBackSideIn_RequestBuffer;
  boost::optional< MemoryTransport> theBackSideIn_ReplyBuffer;
  std::list<MemoryTransport> theBackSideIn_ReplyInfiniteQueue;
  std::list<MemoryTransport> theBackSideIn_RequestInfiniteQueue;

  bool isQuiesced() const {
    return theBusDirection == kIdle
           && !theBackSideIn_ReplyBuffer
           && !theBackSideIn_RequestBuffer
           && (theController.get() ? theController->isQuiesced() : true )
           ;
  }

  void saveState(std::string const & aDirName) {
    theController->saveState( aDirName );
  }

  void loadState(std::string const & aDirName) {
    theController->loadState( aDirName, cfg.TextFlexpoints, cfg.GZipFlexpoints );
  }

  // Initialization
  void initialize() {
    theBusTxCountdown = 0;
    theBusDirection = kIdle;

    //theController.reset();/*new CacheController(statName(),
    new CacheController(statName(),
            cfg.Cores,
            cfg.ArrayConfiguration,
            64,//cfg.BlockSize,
            1,//cfg.Banks,
            1,//cfg.Ports,
            1,//cfg.TagLatency,
            1,//cfg.TagIssueLatency,
            1,//cfg.DataLatency,
            1,//cfg.DataIssueLatency,
            (int)flexusIndex(),
            cfg.CacheLevel,
            0,//cfg.QueueSizes,
            0,//cfg.PreQueueSizes,
            0,//cfg.MAFSize,
            0,//cfg.MAFTargetsPerRequest,
            1,//cfg.EvictBufferSize,
            1,//cfg.SnoopBufferSize,
            false,//cfg.ProbeFetchMiss,
            false,//cfg.EvictClean,
            false,//cfg.EvictWritableHasData,
            "InclusiveMESI",//cfg.CacheType,
            0,//cfg.TraceAddress,
            false, // cfg.AllowOffChipStreamFetch
            false,//cfg.EvictOnSnoop,
            false//cfg.UseReplyChannel
            );

    theCPU = Flexus::Qemu::Processor::getProcessor(theNodeId);

  }

  void finalize() {}

  //MMUDrive
  //----------
  void drive(interface::MMUDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "MMUDrive" ) ) ;
    theController->processMessages();
    busCycle();
  }

  uint32_t transferTime(const MemoryTransport & trans) {
    if (theFlexus->isFastMode()) {
      return 0;
    }
    DBG_Assert(trans[MemoryMessageTag] != nullptr);
    return 1; // FIXME: instantaneous transfer
    //return ( (trans[MemoryMessageTag]->reqSize() > 0) ? cfg.BusTime_Data : cfg.BusTime_NoData) - 1;
  }

  void busCycle() {


      DBG_Assert(theRequests->internalContainer.size() != 0);
      boost::intrusive_ptr<Translation> item = theRequests->internalContainer.front();


      if (item->isInstr()) {
          if (theInstrTLB.find(item->theVaddr) != theInstrTLB.end()) {
                // item exists so mark hit
            item->setHit();
          } else {
              // mark miss
              item->setMiss();
              thePageWalkEntries.push_back(item);
          }
      } else if (item->isData()) {
          if (theDataTLB.find(item->theVaddr) != theDataTLB.end()) {
                // item exists so mark hit
              item->setHit();

          } else {
              // mark miss
              item->setMiss();
              thePageWalkEntries.push_back(item);
          }
      }

      if (thePageWalkEntries.size() > 0){
          for (auto& i : thePageWalkEntries) {
              FLEXUS_CHANNEL(TranslateOut) << i;

          }
      }

      FLEXUS_CHANNEL(ReplyOut) << theRequests;


  }

  void initMMU( std::shared_ptr<mmu_regs_t> regsFromQemu ) {
    this->theMMU = std::make_shared<mmu_t>();
    mmu_regs_t* rawRegs = reinterpret_cast<mmu_regs_t*>( regsFromQemu.get() );

    theMMU->initRegsFromQEMUObject( rawRegs );
    theMMU->setupAddressSpaceSizesAndGranules();
    this->mmuInitialized = true;
    DBG_(Tmp,( << "MMU object init'd, " << std::hex << theMMU << std::dec ));
  }

  bool IsTranslationEnabledAtEL(uint8_t & anEL) {
      return theCore->IsTranslationEnabledAtEL(anEL);
      theNodeId

  }

  // Msutherl
  FLEXUS_PORT_ALWAYS_AVAILABLE(iRequestIn);
  void push( interface::RequestIn const &,
             TranslationPtr& aTranslate ) {
      theiLookUpEntries.push_back( aTranslate );
  }
  FLEXUS_PORT_ALWAYS_AVAILABLE(dRequestIn);
  void push( interface::RequestIn const &,
             TranslationPtr& aTranslate ) {
      thedLookUpEntries.push_back( aTranslate );
  }


  void sendTLBresponse(TranslationPtr aTranslation) {
    aTranslation->isInstr()
    ?   FLEXUS_CHANNEL(iTranslationReply) << aTranslation
    :   FLEXUS_CHANNEL(dTranslationReply) << aTranslation;
  }

/*
  //FrontSideIn_Request
    //-------------------
    bool available( interface::FrontSideIn_Request const &,
                    index_t anIndex) {
      return ! theController->FrontSideIn_Request[0].full();
    }
    void push( interface::FrontSideIn_Request const &,
               index_t           anIndex,
               MemoryTransport & aMessage) {
      DBG_Assert(! theController->FrontSideIn_Request[0].full(), ( << statName() ) );
      aMessage[MemoryMessageTag]->coreIdx() = anIndex;
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Request) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Request[0].enqueue(aMessage);
    }

    //BackeSideIn_Reply
    //-----------
    bool available( interface::BackSideIn_Reply const &) {
      return ! theBackSideIn_ReplyBuffer;
    }
    void push( interface::BackSideIn_Reply const &, MemoryTransport & aMessage) {
      if (cfg.NoBus) {
        aMessage[MemoryMessageTag]->coreIdx() = 0;
        theBackSideIn_ReplyInfiniteQueue.push_back(aMessage);
        return;
      }
      DBG_Assert(! theBackSideIn_ReplyBuffer);
      // In non-piranha caches, the core index should always be zero, since
      // things above (such as the processor) do not want to know about core numbers
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideIn_ReplyBuffer = aMessage;
    }

    //BackeSideIn_Request
    //-----------
    bool available( interface::BackSideIn_Request const &) {
      return ! theBackSideIn_RequestBuffer;
    }
    void push( interface::BackSideIn_Request const &, MemoryTransport & aMessage) {
      if (cfg.NoBus) {
        aMessage[MemoryMessageTag]->coreIdx() = 0;
        theBackSideIn_RequestInfiniteQueue.push_back(aMessage);
        return;
      }
      DBG_Assert(! theBackSideIn_RequestBuffer);
      // In non-piranha caches, the core index should always be zero, since
      // things above (such as the processor) do not want to know about core numbers
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideIn_RequestBuffer = aMessage;
    }
*/







};

} //End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR( MMU , nMMU );
//FLEXUS_PORT_ARRAY_WIDTH( MMU , FrontSideOut_I )           {
//  return (cfg.Cores);
//}
//FLEXUS_PORT_ARRAY_WIDTH( MMU , FrontSideIn_Request )    {
//  return (cfg.Cores);
//}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()
