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

#include <components/ITLB/ITLB.hpp>

#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>

#include "components/TLBControllers/TLBController.hpp"
#include <components/CommonQEMU/Slices/Translation.hpp>

#include <core/qemu/mai_api.hpp>

#include <unordered_map>
#include <queue>

#define DBG_DefineCategories ITLB
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()
#define DBG_SetDefaultOps AddCat(ITLB)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT ITLB 
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTLB {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace nCache;

using std::unique_ptr;

class FLEXUS_COMPONENT(ITLB) {
  FLEXUS_COMPONENT_IMPL(ITLB);

private:

  std::unique_ptr<CacheController> theController; //Deleted automagically when the cache goes away

  Flexus::Stat::StatCounter theBusDeadlocks;

  typedef std::map<VirtualMemoryAddress, PhysicalMemoryAddress> TLBentry;
  TLBentry theInstrTLB;
  TLBentry theDataTLB;

  TranslatedAddresses theRequests;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(ITLB)
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
  }

  void finalize() {}

  //CacheDrive
  //----------
  void drive(interface::CacheDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "CacheDrive" ) ) ;
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
          }
      }
      else if (item->isData()) {
          if (theDataTLB.find(item->theVaddr) != theDataTLB.end()) {
                // item exists so mark hit
              item->setHit();

          } else {
              // mark miss
              item->setMiss();

          }
      }

      FLEXUS_CHANNEL(ReplyOut) << theRequests;


  }

  // Msutherl
  FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &,
             TranslatedAddresses& translateUs ) {

      theRequests = translateUs;

  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(PopulateTLB);
  void push( interface::PopulateTLB const &,
             TranslatedAddresses& translateUs ) {

      while(translateUs->internalContainer.size() > 0){
          boost::intrusive_ptr<Translation> tr = translateUs->internalContainer.front();

          translateUs->internalContainer.pop();

          DBG_Assert(tr->isUnresuloved());
          if (tr->isInstr()) {
              theInstrTLB[tr->theVaddr] = tr->thePaddr;
          } else if (tr->isData()) {
                  theDataTLB[tr->theVaddr] = tr->thePaddr;
          }
      }
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

} //End Namespace nTLB

FLEXUS_COMPONENT_INSTANTIATOR( ITLB , nTLB );
//FLEXUS_PORT_ARRAY_WIDTH( ITLB , FrontSideOut_I )           {
//  return (cfg.Cores);
//}
//FLEXUS_PORT_ARRAY_WIDTH( ITLB , FrontSideIn_Request )    {
//  return (cfg.Cores);
//}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT ITLB

#define DBG_Reset
#include DBG_Control()
