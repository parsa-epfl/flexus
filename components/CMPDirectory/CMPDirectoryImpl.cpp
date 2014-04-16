#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/CMPDirectory/CMPDirectory.hpp>

#include <boost/scoped_ptr.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include <components/CMPDirectory/DirectoryController.hpp>

#define DBG_DefineCategories CMPDirectory
#define DBG_SetDefaultOps AddCat(CMPDirectory)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT CMPDirectory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nCMPDirectory {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::scoped_ptr;

class FLEXUS_COMPONENT(CMPDirectory) {
  FLEXUS_COMPONENT_IMPL(CMPDirectory);

private:

  std::auto_ptr<DirectoryController> theController;

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(CMPDirectory)
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
    theController.reset(new DirectoryController(statName(),
                        cfg.Cores,
                        cfg.BlockSize,
                        cfg.Banks,
                        (int)flexusIndex(),
                        cfg.Interleaving,
                        cfg.DirLatency,
                        cfg.DirIssueLatency,
                        cfg.QueueSize,
                        cfg.MAFSize,
                        cfg.EvictBufferSize,
                        cfg.DirectoryPolicy,
                        cfg.DirectoryType,
                        cfg.DirectoryConfig
                                               ));
  }

  // Ports
  //======

  //Request_In
  //-----------------
  bool available( interface::Request_In const &) {
    return theController->requestQueueAvailable();
  }
  void push( interface::Request_In const &,
             MemoryTransport & aMessage ) {
    DBG_Assert( theController->requestQueueAvailable() );
    DBG_(Trace, Comp(*this) ( << "Received on Port Request_In : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Req");
    }

    theController->enqueueRequest(aMessage);
    //theController->RequestIn.enqueue(aMessage);
  }

  //Snoop_In
  //-----------------
  bool available( interface::Snoop_In const &) {
    if (theController->SnoopIn.full()) {
    }
    return ! theController->SnoopIn.full();
  }
  void push( interface::Snoop_In const &,
             MemoryTransport & aMessage ) {
    DBG_Assert(! theController->SnoopIn.full());
    DBG_(Trace, Comp(*this) ( << "Received on Port Snoop_In : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Snoop");
    }

    theController->SnoopIn.enqueue(aMessage);
  }

  //Reply_In
  //-----------------
  bool available( interface::Reply_In const &) {
    if (theController->ReplyIn.full()) {
    }
    return ! theController->ReplyIn.full();
  }
  void push( interface::Reply_In const &,
             MemoryTransport & aMessage ) {
    DBG_Assert(! theController->ReplyIn.full());
    DBG_(Trace, Comp(*this) ( << "Received on Port Reply_In : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Reply");
    }

    theController->ReplyIn.enqueue(aMessage);
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

    while ( !theController->ReplyOut.empty() && FLEXUS_CHANNEL(Reply_Out).available()) {
      MemoryTransport transport = theController->ReplyOut.dequeue();
      DBG_(Trace, Comp(*this) ( << "Sent on Port ReplyOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(Reply_Out) << transport;
    }
    while ( !theController->SnoopOut.empty() && FLEXUS_CHANNEL(Snoop_Out).available()) {
      if (theController->SnoopOut.peek()[DestinationTag]->isMultipleMsgs()) {
        MemoryTransport transport = theController->SnoopOut.peek();
        transport.set( DestinationTag, transport[DestinationTag]->removeFirstMulticastDest());
        transport.set( MemoryMessageTag, new MemoryMessage(*(transport[MemoryMessageTag])));
        DBG_(Trace, Comp(*this) ( << "Sent on Port SnoopOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL(Snoop_Out) << transport;
      } else {
        MemoryTransport transport = theController->SnoopOut.dequeue();
        if (transport[DestinationTag]->type == DestinationMessage::Multicast) {
          transport[DestinationTag]->convertMulticast();
        }
        DBG_(Trace, Comp(*this) ( << "Sent on Port SnoopOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL(Snoop_Out) << transport;
      }
    }
    while ( !theController->RequestOut.empty() && FLEXUS_CHANNEL(Request_Out).available()) {
      MemoryTransport transport = theController->RequestOut.dequeue();
      DBG_(Trace, Comp(*this) ( << "Sent on Port RequestOut: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(Request_Out) << transport;
    }
  }

};

} //End Namespace nCMPDirectory

FLEXUS_COMPONENT_INSTANTIATOR( CMPDirectory, nCMPDirectory );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPDirectory

#define DBG_Reset
#include DBG_Control()
