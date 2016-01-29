#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/CMPDirectory/BankedDirectory.hpp>

#include <boost/scoped_ptr.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include <components/CMPDirectory/BankedController.hpp>

#define DBG_DefineCategories BankedDirectory
#define DBG_SetDefaultOps AddCat(BankedDirectory)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT BankedDirectory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nBankedDirectory {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace nCMPDirectory;

using std::unique_ptr;

class FLEXUS_COMPONENT(BankedDirectory) {
  FLEXUS_COMPONENT_IMPL(BankedDirectory);

private:

  std::unique_ptr<BankedController> theController;

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(BankedDirectory)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    // TODO: fix this
    return theController->isQuiesced();
  }

  void saveState(std::string const & aDirName) {
    theController->saveState( aDirName );
  }

  void loadState(std::string const & aDirName) {
    theController->loadState( aDirName );
  }

  // Initialization
  void initialize() {
    theController.reset(new BankedController(statName(),
                        cfg.Cores,
                        cfg.BlockSize,
                        cfg.Banks,
                        (int)flexusIndex(),
                        cfg.Interleaving,
                        cfg.SkewShift,
                        cfg.DirLatency,
                        cfg.DirIssueLatency,
                        cfg.QueueSize,
                        cfg.MAFSize,
                        cfg.EvictBufferSize,
                        cfg.DirectoryPolicy,
                        cfg.DirectoryType,
                        cfg.DirectoryConfig,
                        cfg.LocalDirectory
                                            ));
  }

  // Ports
  //======

  //Request_In
  //-----------------
  bool available( interface::Request_In const &, index_t anIndex) {
    if (theController->RequestIn[anIndex].full()) {
    }
    return ! theController->RequestIn[anIndex].full();
  }
  void push( interface::Request_In const &, index_t anIndex, MemoryTransport & aMessage ) {
    DBG_Assert(! theController->RequestIn[anIndex].full());
    DBG_(Trace, Comp(*this) ( << "Received on Port Request_In[" << anIndex << "] : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Req");
    }

    theController->RequestIn[anIndex].enqueue(aMessage);
  }

  //Snoop_In
  //-----------------
  bool available( interface::Snoop_In const &, index_t anIndex) {
    if (theController->SnoopIn[anIndex].full()) {
    }
    return ! theController->SnoopIn[anIndex].full();
  }
  void push( interface::Snoop_In const &, index_t anIndex, MemoryTransport & aMessage ) {
    DBG_Assert(! theController->SnoopIn[anIndex].full());
    DBG_(Trace, Comp(*this) ( << "Received on Port Snoop_In[" << anIndex << "] : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Snoop");
    }

    theController->SnoopIn[anIndex].enqueue(aMessage);
  }

  //Reply_In
  //-----------------
  bool available( interface::Reply_In const &, index_t anIndex) {
    if (theController->ReplyIn[anIndex].full()) {
    }
    return ! theController->ReplyIn[anIndex].full();
  }
  void push( interface::Reply_In const &, index_t anIndex, MemoryTransport & aMessage ) {
    DBG_Assert(! theController->ReplyIn[anIndex].full());
    DBG_(Trace, Comp(*this) ( << "Received on Port Reply_In[" << anIndex << "] : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Reply");
    }

    theController->ReplyIn[anIndex].enqueue(aMessage);
  }

  //DirectoryDrive
  //----------
  void drive(interface::DirectoryDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "DirectoryDrive" ) ) ;
    theController->processMessages();
    busCycle();
  }

  void busCycle() {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "bus cycle" ) );

    for (int32_t i = 0; i < cfg.Banks; i++) {

      while ( !theController->ReplyOut[i].empty() && FLEXUS_CHANNEL_ARRAY(Reply_Out, i).available()) {
        MemoryTransport transport = theController->ReplyOut[i].dequeue();
        DBG_(Trace, Comp(*this) ( << "Sent on Port ReplyOut[" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL_ARRAY(Reply_Out, i) << transport;
      }
      while ( !theController->SnoopOut[i].empty() && FLEXUS_CHANNEL_ARRAY(Snoop_Out, i).available()) {
        if (theController->SnoopOut[i].peek()[DestinationTag]->isMultipleMsgs()) {
          MemoryTransport transport = theController->SnoopOut[i].peek();
          transport.set( DestinationTag, transport[DestinationTag]->removeFirstMulticastDest());
          transport.set( MemoryMessageTag, new MemoryMessage(*(transport[MemoryMessageTag])));
          DBG_(Trace, Comp(*this) ( << "Sent on Port SnoopOut[" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
          FLEXUS_CHANNEL_ARRAY(Snoop_Out, i) << transport;
        } else {
          MemoryTransport transport = theController->SnoopOut[i].dequeue();
          if (transport[DestinationTag]->type == DestinationMessage::Multicast) {
            transport[DestinationTag]->convertMulticast();
          }
          DBG_(Trace, Comp(*this) ( << "Sent on Port SnoopOut[" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
          FLEXUS_CHANNEL_ARRAY(Snoop_Out, i) << transport;
        }
      }
      while ( !theController->RequestOut[i].empty() && FLEXUS_CHANNEL_ARRAY(Request_Out, i).available()) {
        MemoryTransport transport = theController->RequestOut[i].dequeue();
        DBG_(Trace, Comp(*this) ( << "Sent on Port RequestOut[" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL_ARRAY(Request_Out, i) << transport;
      }
    }
  }

};

} //End Namespace nBankedDirectory

FLEXUS_COMPONENT_INSTANTIATOR( BankedDirectory, nBankedDirectory );
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Request_In) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Snoop_In) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Reply_In) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Request_Out) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Snoop_Out) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( BankedDirectory, Reply_Out) {
  return ( cfg.Banks ? cfg.Banks : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT BankedDirectory

#define DBG_Reset
#include DBG_Control()
