#include <components/CMOB/CMOB.hpp>

#define FLEXUS_BEGIN_COMPONENT CMOB
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <fstream>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

#define DBG_DefineCategories CMOB
#define DBG_SetDefaultOps AddCat(CMOB)
#include DBG_Control()

namespace nCMOB {

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
using namespace Flexus::SharedTypes;

using boost::intrusive_ptr;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

class FLEXUS_COMPONENT(CMOB) {
  FLEXUS_COMPONENT_IMPL(CMOB);

  std::list< cmob_message_t > theCMOBMessagesIn;
  std::list< cmob_message_t > theCMOBMessagesOut;

  std::list< MemoryTransport > theMemoryMessagesIn;
  std::list< MemoryTransport > theMemoryReadsOut;
  std::list< MemoryTransport > theMemoryWritesOut;

  int64_t theNextCMOBIndex;

  std::vector< MemoryAddress > theCMOB;
  std::vector< bool > theWasHit;

  std::multimap< MemoryAddress, cmob_message_t > thePendingReads;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMOB)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theNextCMOBIndex(1)
  { }

  bool isQuiesced() const {
    return  theCMOBMessagesIn.empty()
            &&    theCMOBMessagesOut.empty()
            &&    theMemoryReadsOut.empty()
            &&    theMemoryWritesOut.empty()
            &&    theMemoryMessagesIn.empty()
            ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) {
    std::ifstream cmob_in( (aDirName + "/" + cfg.CMOBName  + boost::padded_string_cast < 2, '0' > (flexusIndex())).c_str());

    if (cmob_in) {
      uint32_t capacity;
      cmob_in >> capacity;
      DBG_Assert(capacity == cfg.CMOBSize);

      uint32_t anAddress;
      bool was_hit;
      while (! (cmob_in.eof() || cmob_in.fail())) {
        cmob_in >> anAddress >> was_hit;
        append(MemoryAddress(anAddress), was_hit);
      }
    } else {
      DBG_( Dev, ( << "WARNING: No saved CMOB state!" ));
    }
  }

  // Initialization
  void initialize() {
    theCMOB.resize( cfg.CMOBSize, MemoryAddress(0));
    theWasHit.resize( cfg.CMOBSize, false);
  }

//NEED TO CHECK THIS!!!
  void finalize(){

  }

  // Ports
  FLEXUS_PORT_ALWAYS_AVAILABLE(TMSif_Request);
  void push(interface::TMSif_Request const &,  boost::intrusive_ptr<CMOBMessage> & aMessage) {
    DBG_(Iface, Comp(*this) ( << *aMessage) );
    theCMOBMessagesIn.push_back(aMessage);
  }

  int64_t nextAppendIndex() {
    return (theNextCMOBIndex / cfg.CMOBLineSize) * cfg.CMOBLineSize;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(TMSif_NextAppendIndex);
  int64_t pull(interface::TMSif_NextAppendIndex const &) {
    return nextAppendIndex();
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(TMSif_Initialize);
  CMOBMessage pull(interface::TMSif_Initialize const &) {
    CMOBMessage ret_val;
    ret_val.theCMOBId = flexusIndex();
    int64_t line_index = (theNextCMOBIndex / cfg.CMOBLineSize) * cfg.CMOBLineSize;
    ret_val.theCommand = CMOBCommand::eInit;
    ret_val.theCMOBOffset = line_index;
    for (int64_t i = line_index; i < theNextCMOBIndex; ++i) {
      DBG_Assert( theNextCMOBIndex - line_index < 12);
      ret_val.theLine.theAddresses[theNextCMOBIndex - line_index] = theCMOB[i];
      ret_val.theLine.theWasHit[theNextCMOBIndex - line_index] = theWasHit[i];
    }
    ret_val.theRequestTag = theNextCMOBIndex - line_index;
    return ret_val;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromMemory);
  void push(interface::FromMemory const &,  MemoryTransport & trans) {
    DBG_(Iface, Comp(*this) ( << *(trans[MemoryMessageTag]) ) );
    theMemoryMessagesIn.push_back(trans);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(PrefixRead);
  void push(interface::PrefixRead const &, prefix_read_t & aPrefix ) {
    std::list< MemoryAddress> addresses;

    for (int32_t i = 0; i < aPrefix.first; ++i) {
      if (aPrefix.second + i < cfg.CMOBSize) {
        addresses.push_back( theCMOB[ aPrefix.second + i ] );
      }
    }
    FLEXUS_CHANNEL( PrefixReadOut ) << addresses;
  }

  // Drive Interfaces
  void drive(interface::CMOBDrive const &) {
    DBG_(VVerb, Comp(*this) ( << "CMOBDrive" ) ) ;

    processCMOBMessages();
    processMemoryMessages();

    sendMessages();
  }

  void processCMOBMessages() {
    while (! theCMOBMessagesIn.empty() ) {
      cmob_message_t msg = theCMOBMessagesIn.front();
      theCMOBMessagesIn.pop_front();
      switch (msg->theCommand) {
        case CMOBCommand::eWrite:
          processWrite(msg);
          break;
        case CMOBCommand::eRead:
          processRead(msg);
          break;
        default:
          DBG_Assert(false);
      }
    }
  }

  void processMemoryMessages() {
    while (! theMemoryMessagesIn.empty() ) {
      boost::intrusive_ptr<MemoryMessage> msg = theMemoryMessagesIn.front()[MemoryMessageTag];
      theMemoryMessagesIn.pop_front();
      DBG_Assert(msg->type() == MemoryMessage::LoadReply );
      processMemoryReply(*msg);
    }
  }

  void sendMessages() {
    while ( FLEXUS_CHANNEL(TMSif_Reply).available() && !theCMOBMessagesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending TMSif_Reply: "  << *(theCMOBMessagesOut.front()) ));
      FLEXUS_CHANNEL(TMSif_Reply) << theCMOBMessagesOut.front();
      theCMOBMessagesOut.pop_front();
    }

    while ( FLEXUS_CHANNEL(ToMemory).available() && !theMemoryReadsOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToMemory: "  << *(theMemoryReadsOut.front()[MemoryMessageTag]) ));
      FLEXUS_CHANNEL(ToMemory) << theMemoryReadsOut.front();
      theMemoryReadsOut.pop_front();
    }

    while ( FLEXUS_CHANNEL(ToMemory).available() && !theMemoryWritesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToMemory: "  << *(theMemoryWritesOut.front()[MemoryMessageTag]) ));
      FLEXUS_CHANNEL(ToMemory) << theMemoryWritesOut.front();
      theMemoryWritesOut.pop_front();
    }
  }

  void processWrite( cmob_message_t aMessage) {
    DBG_(Verb, Comp(*this) ( << " CMOB Write: "  << *aMessage ));
    DBG_Assert( aMessage->theCMOBId == flexusIndex() );
    DBG_Assert( aMessage->theCMOBOffset == nextAppendIndex() );
    for (int32_t i = 0; i < cfg.CMOBLineSize; ++i) {
      if (nextAppendIndex() + i == theNextCMOBIndex) {
        append( aMessage->theLine.theAddresses[i], aMessage->theLine.theWasHit[i]);
      }
    }

    MemoryAddress cmob_address(  aMessage->theCMOBOffset );

    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( new MemoryMessage(MemoryMessage::EvictDirty, cmob_address));
    operation->reqSize() = 64;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( cmob_address );
    tracker->setInitiator(flexusIndex());
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    theMemoryWritesOut.push_back(transport);

    //TODO: Schedule a CMOB writeback message to main memory.
  }

  MemoryAddress address( uint32_t aCMOBOffset) {
    return MemoryAddress( 0x80000000UL | (flexusIndex() << 28) | aCMOBOffset );
  }

  void processRead( cmob_message_t aMessage) {
    if (! cfg.UseMemory) {
      finishRead( aMessage );
      return;
    }

    int64_t read_offset = aMessage->theCMOBOffset ;
    DBG_Assert( read_offset % cfg.CMOBLineSize == 0);
    if (read_offset > cfg.CMOBSize) {
      read_offset = read_offset % cfg.CMOBSize;
    }

    MemoryAddress cmob_address( address(read_offset) );

    thePendingReads.insert( std::make_pair(cmob_address, aMessage) );

    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( new MemoryMessage(MemoryMessage::LoadReq, cmob_address));
    operation->reqSize() = 0;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( cmob_address );
    tracker->setInitiator(flexusIndex());
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);

    DBG_(Verb, Comp(*this) ( << "Process read " << *aMessage << " Memory request " << *operation) );

    theMemoryReadsOut.push_back(transport);
  }

  void processMemoryReply(MemoryMessage & msg) {
    std::multimap< MemoryAddress, cmob_message_t >::iterator begin, iter, end;
    boost::tie(begin, end) = thePendingReads.equal_range( msg.address() );
    iter = begin;
    while (iter != end) {
      finishRead(iter->second);
      ++iter;
    }
    thePendingReads.erase( begin, end );

  }

  void finishRead( cmob_message_t aMessage) {
    int64_t read_offset = aMessage->theCMOBOffset ;
    DBG_Assert( read_offset % cfg.CMOBLineSize == 0);
    if (read_offset >= cfg.CMOBSize) {
      read_offset = read_offset % cfg.CMOBSize;
    }
    DBG_Assert( read_offset + cfg.CMOBLineSize - 1 <= cfg.CMOBSize, ( << *aMessage << " CMOB: " << cfg.CMOBSize) );
    for (int32_t i = 0; i < cfg.CMOBLineSize; ++i) {
      aMessage->theLine.theAddresses[i] = ( theCMOB[read_offset + i] );
      aMessage->theLine.theWasHit[i] = theWasHit[read_offset + i];
    }
    DBG_(Verb, Comp(*this) ( << " CMOB Read done: "  << *aMessage ));

    theCMOBMessagesOut.push_back( aMessage );
  }

  void append( MemoryAddress anAddress, bool wasHit) {
    theCMOB[theNextCMOBIndex] = MemoryAddress(anAddress & ~63);
    theWasHit[theNextCMOBIndex] = wasHit;

    ++theNextCMOBIndex;
    if (theNextCMOBIndex >= cfg.CMOBSize) {
      theNextCMOBIndex = 0;
    }
  }

};

} //End Namespace nCMOB

FLEXUS_COMPONENT_INSTANTIATOR( CMOB, nCMOB);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMOB

#define DBG_Reset
#include DBG_Control()

