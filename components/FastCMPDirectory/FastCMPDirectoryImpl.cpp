#include <components/FastCMPDirectory/FastCMPDirectory.hpp>

#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/FastCMPDirectory/AbstractTopology.hpp>
#include <components/FastCMPDirectory/AbstractDirectory.hpp>

#include <components/FastCMPDirectory/DirectoryStats.hpp>

#include <core/performance/profile.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>
#include <ext/hash_map>

#include <list>
#include <fstream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <stdlib.h> // for random()

/*
  #define DBG_DefineCategories CMPCache
  #define DBG_SetDefaultOps AddCat(CMPCache) Comp(*this)
  #include DBG_Control()
*/

#define FLEXUS_BEGIN_COMPONENT FastCMPDirectory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nFastCMPDirectory {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

class FLEXUS_COMPONENT(FastCMPDirectory) {
  FLEXUS_COMPONENT_IMPL( FastCMPDirectory );

  // some bookkeeping vars
  uint32_t    theCMPWidth;       // # cores per CMP chip
  uint32_t    theCMPGridWidth;   // # cores per row in a tiled CMP chip

  PhysicalMemoryAddress theCoherenceUnitMask;

  // the cache and the associated statistics
  DirectoryStats    *    theStats;

  AbstractProtocol * theProtocol;
  AbstractTopology * theTopology;
  AbstractDirectory * theDirectory;

  bool theAlwaysMulticast;

  std::list<boost::function<void(void)> > theScheduledActions;

  inline MemoryMessage::MemoryMessageType combineSnoopResponse( MemoryMessage::MemoryMessageType & a, MemoryMessage::MemoryMessageType & b) const {

    if (a == MemoryMessage::NoRequest) {
      return b;
    } else if (b == MemoryMessage::NoRequest) {
      return a;
    } else if (a == MemoryMessage::InvUpdateAck || b == MemoryMessage::InvUpdateAck) {
      return MemoryMessage::InvUpdateAck;
    } else if (a == MemoryMessage::InvalidateAck || b == MemoryMessage::InvalidateAck) {
      return MemoryMessage::InvalidateAck;
    } else if (a == MemoryMessage::Invalidate || b == MemoryMessage::Invalidate) {
      return MemoryMessage::Invalidate;
    } else if (a == MemoryMessage::ReturnReplyDirty || b == MemoryMessage::ReturnReplyDirty) {
      return MemoryMessage::ReturnReplyDirty;
    } else if (a == MemoryMessage::ReturnReply || b == MemoryMessage::ReturnReply) {
      return MemoryMessage::ReturnReply;
    } else if (a == MemoryMessage::ReturnNAck && b == MemoryMessage::ReturnNAck) {
      return MemoryMessage::ReturnNAck;
    } else {
      DBG_Assert( false, ( << "Unknown message types to combine '" << a << "' and '" << b << "'" ));
      return a;
    }
  }

  inline MemoryMessage::MemoryMessageType calculateResponse( const MemoryMessage::MemoryMessageType & a, const MemoryMessage::MemoryMessageType & b) const {
    return b;
  }

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastCMPDirectory)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {

    return true;
  }

  void finalize( void ) {
    // Flush any pending actions
    performDelayedActions();

    theStats->update();
    theDirectory->finalize();
  }

  void sendRegionProbe(RegionScoutMessage & msg, int32_t index) {
    FLEXUS_CHANNEL_ARRAY(RegionProbe, index) << msg;
  }

  void scheduleDelayedAction(boost::function<void(void)> fn) {
    theScheduledActions.push_back(fn);
  }

  void performDelayedActions() {
    while (!theScheduledActions.empty()) {
      theScheduledActions.front()();
      theScheduledActions.pop_front();
    }
  }

  void invalidateBlock(PhysicalMemoryAddress address, SharingVector sharers) {
    std::list<int> slist(sharers.toList());

    MemoryMessage msg(MemoryMessage::Invalidate, address);
    while (!slist.empty()) {
      DBG_(Trace, ( << "Sending Invalidate to core " << slist.front() << " for block " << std::hex << address ));
      msg.type() = MemoryMessage::Invalidate;
      FLEXUS_CHANNEL_ARRAY(SnoopOut, slist.front()) << msg;
      slist.pop_front();
    }

  }

  void initialize(void) {
    theCMPWidth = (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());

    theCMPGridWidth = (uint32_t)ceil(sqrt(theCMPWidth));

    static volatile bool widthPrintout = true;
    if (widthPrintout) {
      DBG_( Crit, ( << "Running with CMP width " << theCMPWidth ) );
      widthPrintout = false;
    }
    if (((theCMPWidth - 1) & theCMPWidth) != 0) {
      DBG_( Crit, ( << "CMP width is NOT a power of 2, some stats will be invalid!"));
    }

    DBG_Assert ( theCMPWidth <= MAX_NUM_SHARERS, ( << "This implementation only supports up to " << MAX_NUM_SHARERS << " nodes" ) );

    theStats = new DirectoryStats(statName());

    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nFastCMPDirectory::FastCMPDirectoryComponent::finalize, this ) );

    theTopology = CREATE_TOPOLOGY(cfg.TopologyType);
    theTopology->setNumCores(theCMPWidth);
    theTopology->initialize(statName());

    theDirectory = CREATE_DIRECTORY(cfg.DirectoryType);
    theDirectory->setNumCores(theCMPWidth);
    theDirectory->setTopology(theTopology);
    theDirectory->setBlockSize(cfg.CoherenceUnit);
    theDirectory->setPortOperations( boost::bind( &FastCMPDirectoryComponent::sendRegionProbe, this, _1, _2),
                                     boost::bind( &FastCMPDirectoryComponent::scheduleDelayedAction, this, _1) );
    theDirectory->setInvalidateAction( boost::bind( &FastCMPDirectoryComponent::invalidateBlock, this, _1, _2) );
    theDirectory->initialize(statName());

    theProtocol = CREATE_PROTOCOL(cfg.Protocol);

    theCoherenceUnitMask = PhysicalMemoryAddress(~(cfg.CoherenceUnit - 1));

    DBG_( Dev, ( << "Coherence Unit is: " << cfg.CoherenceUnit ));

    DBG_( Dev, ( << "sizeof(PhysicalMemoryAddress) = " << (int)sizeof(PhysicalMemoryAddress) ));

    theAlwaysMulticast = cfg.AlwaysMulticast;
  }

  ////////////////////// from ICache
  bool available( interface::FetchRequestIn const &,
                  index_t anIndex ) {
    return true;
  }

  void push( interface::FetchRequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port FetchRequestIn[" << anIndex << "] Request: " << aMessage ));
    aMessage.dstream() = false;
    processRequest( anIndex, aMessage );
  }

  ////////////////////// from DCache
  bool available( interface::RequestIn const &,
                  index_t anIndex) {
    return true;
  }

  void push( interface::RequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage ) {
    DBG_( Iface, Addr(aMessage.address()) ( << "Received on Port RequestIn[" << anIndex << "] Request: " << aMessage ));
    aMessage.dstream() = true;
    processRequest( anIndex, aMessage );
  }

  ////////////////////// snoop port
  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
  void push( interface::SnoopIn const &,
             MemoryMessage & aMessage) {
  }

  ////////////////////// from DCache
  bool available( interface::RegionNotify const &,
                  index_t anIndex) {
    return true;
  }

  void push( interface::RegionNotify const &,
             index_t         anIndex,
             RegionScoutMessage & aMessage ) {
    DBG_( Iface, Addr(aMessage.region()) ( << "Received on Port RegionNotify[" << anIndex << "] Request: " << aMessage ));
    theDirectory->regionNotify(anIndex, aMessage);
  }

  void drive( interface::UpdateStatsDrive const &) {
    theStats->update();
  }

private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Helper functions

  /////////////////////////////////////////////////////////////////////////////////////
  ////// process incoming requests
  void processRequest( const index_t   anIndex,
                       MemoryMessage & aMessage ) {
    FLEXUS_PROFILE();
    int32_t potential_sharers = 0;
    int32_t unnecessary_snoops_sent = 0;
    int32_t extra_snoops_sent = 0;
    bool accessed_memory = false;
    PhysicalMemoryAddress addr(aMessage.address() & theCoherenceUnitMask);

    performDelayedActions();

    // First we do a directory lookup
    // This gives us the following things:
    //   1. vector of sharers
    //   2. sharing state
    //   3. list of messages that were sent during the lookup
    //   4. location at which the remaining logic is performed
    //   5. list of "secondary actions", mostly these are for fixed size directories where we might need to evict things to allocate a new directory entry.
    //  6. a pointer to a directory entry so we can easily update it without doing more directory lookups

    SharingVector sharers;
    SharingState state;
    int cur_location;
    AbstractEntry_p dir_entry;
    std::list<TopologyMessage> msg_list;
    std::list<boost::function<void(void)> > extra_actions;

    std::tie(sharers, state, cur_location, dir_entry) = theDirectory->lookup(anIndex, addr, aMessage.type(), msg_list, extra_actions);

    // Next, we use the sharing state and the request type to determine the actions based on some coherence protocol
    // The actions consist of the following information:
    //   - Type of snoops to send: Invalidate, ReturnReq, None
    //   - Bit vector of potential snoop destinations
    //   - Whether snoop should be multicast to all destinations, or sent serially
    //   - If snoops are sent serially, what response(s) terminates the snoop process
    //   - What kind of Fill to send, (normal, writable, dirty)
    //   - is this a valid state & request combination
    DBG_(Trace, Addr(aMessage.address()) ( << "Received " << aMessage.type() << " for 0x" << std::hex << addr << std::dec << " from " << anIndex << ", state = " << SharingStatePrinter(state) << ", sharers = " << sharers ));

    const PrimaryAction & action = theProtocol->getAction(state, aMessage.type(), aMessage.address());

    DBG_Assert( !action.poison , ( << "POISON: state = " << (int)state << ", request = " << aMessage ) );

    DBG_(Iface, Addr(aMessage.address()) ( << "Snoop is " << action.snoop_type << ", Multicast " << action.multicast ));

    // If snoops are required, this is where we do them
    MemoryMessage::MemoryMessageType response = MemoryMessage::NoRequest;
    MemoryMessage::MemoryMessageType our_response = action.response[0];
    bool snoop_success = false;
    if (action.snoop_type != MemoryMessage::NoRequest) {
      MemoryMessage snoop_msg(aMessage);

      // We want to turn the vector of sharers into an ordered list that determines the order of the snoops
      // In simple cases, the order is determined solely by the topology (ie. snoop closest nodes first)
      // In more complex cases, the Directory may want to control the order as well
      // (ie. Local node knows of some sharers, if they miss the main directory provides others
      // Since the Directory has a pointer to the Topology,
      // We provide flexibility by always letting the directory order the snoops

      std::list<int> snoop_list = theDirectory->orderSnoops(dir_entry, sharers, cur_location, action.reply_from_snoop, anIndex, action.multicast);

      bool multiple_snoops = snoop_list.size() > 1;

      potential_sharers = snoop_list.size();
      // Send snoops to all the nodes in the list we just made
      // We also track the messages we send, and use responses to update the directory information
      // If we receive the terminating condition, then stop snooping
      std::list<int>::iterator index_iter = snoop_list.begin();
      int32_t new_location = -1;
      for (; index_iter != snoop_list.end(); index_iter++) {
        DBG_(Iface, Addr(aMessage.address()) ( << "Snoop list contains " << (*index_iter) ));
        if (snoop_list.size() == 2 && ((uint32_t)*index_iter == anIndex)) {
          multiple_snoops = false;
        }
      }
      index_iter = snoop_list.begin();

      bool found_terminal = false;

      for (; index_iter != snoop_list.end(); index_iter++) {
        // Skip the requesting node
        if ((uint32_t)*index_iter == anIndex) {
          DBG_(Iface, Addr(aMessage.address()) ( << "Skipping snoop to " << (*index_iter) ));
          //potential_sharers--;
          continue;
        }

        snoop_msg.type() = action.snoop_type;
        FLEXUS_CHANNEL_ARRAY(SnoopOut, *index_iter) << snoop_msg;

        DBG_(Iface, Addr(aMessage.address()) ( << "Sending " << action.snoop_type << " to " << (*index_iter) << " after recieving " << aMessage.type() << " from " << anIndex << " reponse is " << snoop_msg.type() ) );

        response = combineSnoopResponse(response, snoop_msg.type());

        new_location = theDirectory->processSnoopResponse(*index_iter, snoop_msg.type(), dir_entry, addr);

        if (response != action.terminal_response[0] && response == action.terminal_response[1]) {
          our_response = action.response[1];
        }

        if (!action.multicast && !(theAlwaysMulticast && multiple_snoops)) {
          // Record the snoop message that we sent
          msg_list.push_back(TopologyMessage(cur_location, *index_iter));
          if (response == action.terminal_response[0] || response == action.terminal_response[1]) {

            // If we're sending a reply from the Snooped node, add in the latency here
            if (action.reply_from_snoop) {
              msg_list.push_back(TopologyMessage(*index_iter, anIndex));
              msg_list.push_back(TopologyMessage(*index_iter, cur_location, false));
            } else {
              // Record the snoop reply that we recieved
              msg_list.push_back(TopologyMessage(*index_iter, cur_location));
            }
            DBG_(Iface, Addr(aMessage.address()) ( << "Got terminal response, ending snoop process" ));
            break;
          } else {
            unnecessary_snoops_sent++;
          }
          // Record the snoop reply that we recieved
          msg_list.push_back(TopologyMessage(*index_iter, cur_location));
        } else if (snoop_msg.type() != action.terminal_response[0] && snoop_msg.type() != action.terminal_response[1]) {
          unnecessary_snoops_sent++;
        } else if (!action.multicast && theAlwaysMulticast && multiple_snoops &&
                   (snoop_msg.type() == action.terminal_response[0] || snoop_msg.type() == action.terminal_response[1])) {
          if (!found_terminal) {
            found_terminal = true;
          } else {
            extra_snoops_sent++;
          }
        }

        if (new_location >= 0) {
          msg_list.push_back(TopologyMessage(cur_location, new_location));
          cur_location = new_location;
        }
      }
      if (response == action.terminal_response[0] || response == action.terminal_response[1]) {
        snoop_success = true;
      }

      if (action.multicast || (theAlwaysMulticast && multiple_snoops)) {
        msg_list.push_back(TopologyMessage(cur_location, snoop_list));
      }
    }

    // Remember the original message type
    MemoryMessage::MemoryMessageType orig_msg_type = aMessage.type();

    // Next we forward the response to memory
    // If we reached the termination condition when doing snoops, then there's no need to forward to memory
    if (!snoop_success && action.forward_request) {
      accessed_memory = true;
      FLEXUS_CHANNEL(RequestOut) << aMessage;
      response = aMessage.type();

      // Add Request/Response to list of messages
      msg_list.push_back(TopologyMessage(cur_location, theTopology->memoryToIndex(aMessage.address()) ));
      msg_list.push_back(TopologyMessage(theTopology->memoryToIndex(aMessage.address()), cur_location ));
    }

    // Now, we perform any necessary secondary actions
    // This basically assumes we wait until we get a response from the memory before allocating a new directory entry
    while (!extra_actions.empty()) {
      extra_actions.front()();
      extra_actions.pop_front();
    }

    // Now we can caluclate what response we will return
    // This depends on the Snoop response or the response from memory, as well as the fill type specified by the protocol
    aMessage.type() = calculateResponse(response, our_response);

    if (!action.reply_from_snoop || !snoop_success) {
      msg_list.push_back(TopologyMessage(cur_location, anIndex));
    }

    // Update the directory, passing it the original message type, the source, and the response we're sending
    theDirectory->processRequestResponse(anIndex, orig_msg_type, aMessage.type(), dir_entry, addr, accessed_memory);

    // Finally, use the topology to calculate all the messages we've sent and the total latency
    int32_t latency = 1;
    if (!cfg.NoLatency) {
      latency = theTopology->processMessageList(msg_list);
    }

    if (accessed_memory) {
      switch (orig_msg_type) {
        case MemoryMessage::ReadReq:
          theStats->theReadsOffChip++;
          theStats->theReadsOffChip_PSharers << std::make_pair(potential_sharers, 1);
          //theStats->theReadsOffChipLatency << std::make_pair(latency, 1);
          theStats->theReadsOffChipLatency += latency;
          break;
        case MemoryMessage::WriteReq:
          theStats->theWritesOffChip++;
          theStats->theWritesOffChip_PSharers << std::make_pair(potential_sharers, 1);
          //theStats->theWritesOffChipLatency << std::make_pair(latency, 1);
          theStats->theWritesOffChipLatency += latency;
          break;
        case MemoryMessage::FetchReq:
          theStats->theFetchesOffChip++;
          theStats->theFetchesOffChip_PSharers << std::make_pair(potential_sharers, 1);
          //theStats->theFetchesOffChipLatency << std::make_pair(latency, 1);
          theStats->theFetchesOffChipLatency += latency;
          break;
        default:
          //theStats->theOtherOffChipLatency << std::make_pair(latency, 1);
          theStats->theOtherOffChipLatency += latency;
          break;
      }
    } else {
      switch (orig_msg_type) {
        case MemoryMessage::ReadReq:
          theStats->theReadsOnChip++;
          theStats->theReadsOnChip_PSharers << std::make_pair(potential_sharers, 1);
          theStats->theReadsOnChip_FSharers << std::make_pair(unnecessary_snoops_sent, 1);
          theStats->theReadsOnChip_ASharers << std::make_pair(extra_snoops_sent, 1);
          //theStats->theReadsOnChipLatency << std::make_pair(latency, 1);
          theStats->theReadsOnChipLatency += latency;
          break;
        case MemoryMessage::WriteReq:
          theStats->theWritesOnChip++;
          theStats->theWritesOnChip_PSharers << std::make_pair(potential_sharers, 1);
          theStats->theWritesOnChip_FSharers << std::make_pair(unnecessary_snoops_sent, 1);
          theStats->theWritesOnChip_ASharers << std::make_pair(extra_snoops_sent, 1);
          //theStats->theWritesOnChipLatency << std::make_pair(latency, 1);
          theStats->theWritesOnChipLatency += latency;
          break;
        case MemoryMessage::FetchReq:
          theStats->theFetchesOnChip++;
          theStats->theFetchesOnChip_PSharers << std::make_pair(potential_sharers, 1);
          theStats->theFetchesOnChip_FSharers << std::make_pair(unnecessary_snoops_sent, 1);
          theStats->theFetchesOnChip_ASharers << std::make_pair(extra_snoops_sent, 1);
          //theStats->theFetchesOnChipLatency << std::make_pair(latency, 1);
          theStats->theFetchesOnChipLatency += latency;
          break;
        case MemoryMessage::UpgradeReq:
          theStats->theUpgradesOnChip++;
          theStats->theUpgradesOnChip_PSharers << std::make_pair(potential_sharers, 1);
          theStats->theUpgradesOnChip_FSharers << std::make_pair(unnecessary_snoops_sent, 1);
          theStats->theUpgradesOnChip_ASharers << std::make_pair(extra_snoops_sent, 1);
          //theStats->theUpgradesOnChipLatency << std::make_pair(latency, 1);
          theStats->theUpgradesOnChipLatency += latency;
          break;
        default:
          //theStats->theOtherOnChipLatency << std::make_pair(latency, 1);
          theStats->theOtherOnChipLatency += latency;
          break;
      }
    }

  }

  // Topology has no state
  void saveState(std::string const & aDirName) {
    std::string fname( aDirName );
    fname += "/" + statName() + ".gz";
    std::ofstream ofs(fname.c_str(), std::ios::binary);

    boost::iostreams::filtering_stream<boost::iostreams::output> out;
    out.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(9)));
    out.push(ofs);

    theDirectory->saveState ( out , aDirName );
  }

  void loadState( std::string const & aDirName ) {
    std::string fname( aDirName);
    fname += "/" + statName() + ".gz";
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      //ifs >> std::skipws;

      boost::iostreams::filtering_stream<boost::iostreams::input> in;
      in.push(boost::iostreams::gzip_decompressor());
      in.push(ifs);

      if ( ! theDirectory->loadState( in, aDirName ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                      ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
    }
  }

};  // end class FastCMPDirectory

}  // end Namespace nFastCMPDirectory

FLEXUS_COMPONENT_INSTANTIATOR( FastCMPDirectory, nFastCMPDirectory);

FLEXUS_PORT_ARRAY_WIDTH( FastCMPDirectory, RequestIn)      {
  return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPDirectory, FetchRequestIn) {
  return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPDirectory, SnoopOut)       {
  return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPDirectory, RegionProbe)    {
  return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}
FLEXUS_PORT_ARRAY_WIDTH( FastCMPDirectory, RegionNotify)   {
  return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCMPDirectory

#define DBG_Reset
#include DBG_Control()
