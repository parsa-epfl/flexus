#include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories FetchAddressGenerate
#define DBG_SetDefaultOps AddCat(FetchAddressGenerate)
#include DBG_Control()

#include <core/flexus.hpp>
#include <core/qemu/mai_api.hpp>

#include <components/Common/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>

namespace nFetchAddressGenerate {

using namespace Flexus;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(FetchAddressGenerate)  {
  FLEXUS_COMPONENT_IMPL(FetchAddressGenerate);

  std::vector<MemoryAddress> thePC;
  std::vector<MemoryAddress> theNextPC;
  std::vector<MemoryAddress> theRedirectPC;
  std::vector<MemoryAddress> theRedirectNextPC;
  std::vector<bool> theRedirect;
  std::unique_ptr<BranchPredictor> theBranchPredictor;
  uint32_t theCurrentThread;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  void initialize() {
    thePC.resize(cfg.Threads);
    theNextPC.resize(cfg.Threads);
    theRedirectPC.resize(cfg.Threads);
    theRedirectNextPC.resize(cfg.Threads);
    theRedirect.resize(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      Qemu::Processor cpu = Qemu::Processor::getProcessor(flexusIndex() * cfg.Threads + i);
      thePC[i] = cpu->getPC();
      theNextPC[i] = cpu->getNPC();
      theRedirectPC[i] = MemoryAddress(0);
      theRedirectNextPC[i] = MemoryAddress(0);
      theRedirect[i] = false;
      DBG_( Dev, Comp(*this) ( << "Thread[" << flexusIndex() << "." << i << "] connected to " << ((static_cast<Flexus::Qemu::API::conf_object_t *>(cpu))->name ) << " Initial PC: " << thePC[i] ) );
    }
    theCurrentThread = cfg.Threads;
    theBranchPredictor.reset( BranchPredictor::combining(statName(), flexusIndex()) );
  }

  void finalize() {}

  bool isQuiesced() const {
    //the FAG is always quiesced.
    return true;
  }

  void saveState(std::string const & aDirName) {
    theBranchPredictor->saveState(aDirName);
  }

  void loadState(std::string const & aDirName) {
    theBranchPredictor->loadState(aDirName);
  }

public:
  //RedirectIn
  //----------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex, std::pair<MemoryAddress, MemoryAddress> & aRedirect) {
    theRedirectPC[anIndex] = aRedirect.first;
    theRedirectNextPC[anIndex] = aRedirect.second;
    theRedirect[anIndex] = true;
  }

  //BranchFeedbackIn
  //----------------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BranchFeedbackIn);
  void push(interface::BranchFeedbackIn const &, index_t anIndex, boost::intrusive_ptr<BranchFeedback> & aFeedback) {
    theBranchPredictor->feedback(*aFeedback);
  }

  //Drive Interfaces
  //----------------
  //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
  //passing each instruction to its FetchOut port.
  void drive( interface::FAGDrive const &) {
    int32_t td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFAGThread(flexusIndex());
    }
    doAddressGen(td);
  }

private:
  //Implementation of the FetchDrive drive interface
  void doAddressGen(index_t anIndex) {

    if (theFlexus->quiescing()) {
      //We halt address generation when we are trying to quiesce Flexus
      return;
    }

    if (theRedirect[anIndex]) {
      thePC[anIndex] = theRedirectPC[anIndex];
      theNextPC[anIndex] = theRedirectNextPC[anIndex];
      theRedirect[anIndex] = false;
      DBG_(Iface, Comp(*this) ( << "Redirect Thread[" << anIndex << "] " << thePC[anIndex]) );
    }
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( FetchAddrOut, anIndex).available() );
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex).available() );
    int32_t available_faq = 0;
    FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex) >> available_faq;

    int32_t max_addrs = cfg.MaxFetchAddress;
    if (max_addrs > available_faq) {
      max_addrs = available_faq;
    }
    int32_t max_predicts = cfg.MaxBPred;

    boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
    while ( max_addrs > 0 ) {
      FetchAddr faddr(thePC[anIndex]);

      //Advance the PC
      if ( theBranchPredictor->isBranch( faddr.theAddress ) ) {
        if (max_predicts == 0) {
          break;
        }
        thePC[anIndex] = theNextPC[anIndex];
        theNextPC[anIndex] = theBranchPredictor->predict( faddr );
        if (theNextPC[anIndex] == 0) {
          theNextPC[anIndex] = thePC[anIndex] + 4;
        }
        DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
        fetch->theFetches.push_back( faddr);
        -- max_predicts;
      } else {
        thePC[anIndex] = theNextPC[anIndex];
        DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
        fetch->theFetches.push_back( faddr );
        theNextPC[anIndex] = thePC[anIndex] + 4;
      }

      --max_addrs;
    }

    if (fetch->theFetches.size() > 0) {
      //Send it to FetchOut
      FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
    }
  }

public:
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) {
    int32_t available_faq = 0;
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex ).available() ) ;
    FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex ) >> available_faq;
    return available_faq == 0;
  }

};
}//End namespace nFetchAddressGenerate

FLEXUS_COMPONENT_INSTANTIATOR( FetchAddressGenerate, nFetchAddressGenerate);

FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, RedirectIn )           {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, BranchFeedbackIn )      {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, FetchAddrOut )    {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, AvailableFAQ )   {
  return (cfg.Threads);
}
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, Stalled )   {
  return (cfg.Threads);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

#define DBG_Reset
#include DBG_Control()
