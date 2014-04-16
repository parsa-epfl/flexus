#ifndef FASTCMPDIRECTORY_TOPOLOGY_HPP
#define FASTCMPDIRECTORY_TOPOLOGY_HPP

#include <core/stats.hpp>
#include <core/types.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>

#include <components/FastCMPDirectory/AbstractFactory.hpp>
#include <components/FastCMPDirectory/SharingVector.hpp>

#include <list>

namespace nFastCMPDirectory {

struct TopologyMessage {
  // Messages on the critical path are included in latency calculations
  // This allows for better latency and BW estimates
  TopologyMessage(int32_t s, int32_t d, bool c = true) : source(s), dest(d), critical(c), multicast(false) {};
  TopologyMessage(int32_t s, std::list<int> d) : source(s), dest_list(d), critical(true), multicast(true) {};

  int32_t source;
  int32_t dest;
  std::list<int> dest_list;
  bool critical;
  bool multicast;
};

class TopologyStats {
public:
  Flexus::Stat::StatInstanceCounter<int64_t>  theRequestLatencyDist;
  Flexus::Stat::StatCounter  theTotalLinkUsage;
  std::vector<Flexus::Stat::StatCounter *> theLinkUsageCounters;

  TopologyStats(const std::string & aName, int32_t aNumLinks)
    : theRequestLatencyDist(aName + "-MsgLatencyDist")
    , theTotalLinkUsage(aName + "-TotalLinkUsage") {
    char buf[4];
    for (int32_t i = 0; i < aNumLinks; i++) {
      sprintf(buf, "%.3d", i);
      theLinkUsageCounters.push_back(new Flexus::Stat::StatCounter(aName + "-LinkUsage:" + std::string(buf)));
    }
  };
};

class AbstractTopology {
protected:
  TopologyStats * theStats;

  int32_t theNumCores;
  int32_t theNumLinks;
  int32_t theNumMemControllers;
  int32_t theMemControllerInterleaving;
  std::string theMemControllerLocation;

  void setMemControllerLocation(std::string & loc) {
    theMemControllerLocation = loc;
  }

  void setNumMemControllers(const std::string & arg) {
    theNumMemControllers = atoi(arg.c_str());
  }

  void setMemControllerInterleaving(const std::string & arg) {
    theMemControllerInterleaving = std::atoi(arg.c_str());
  }

  inline void recordLinkUsage(int32_t link) {
    theStats->theTotalLinkUsage++;
    (*(theStats->theLinkUsageCounters[link]))++;
  }

  inline void recordRequestLatency(int32_t latency) {
    theStats->theRequestLatencyDist << std::make_pair((int64_t)latency, 1);
  }

public:
  // Record latency and BW of the messages in the list
  // All of these messages correspond to a single request
  virtual int32_t processMessageList(std::list<TopologyMessage> &msgs) {
    // Add latency+BW for each message
    int32_t total_latency = 0;
    std::list<TopologyMessage>::iterator iter = msgs.begin();
    for (; iter != msgs.end(); iter++) {
      int32_t latency = 0;
      if (!iter->multicast) {
        latency = addBW(iter->source, iter->dest);
      } else {
        latency = addBW(iter->source, iter->dest_list);
      }
      if (iter->critical) {
        total_latency += latency;
      }
    }
    recordRequestLatency(total_latency);
    return total_latency;
  };

  // Convert a sharing vector into an ordered list of integers
  // Messages will be sent from src
  // Order from shorters trip to longest
  // If optimize3hop is true, then we're going to send a successful snoop sends a reply to final_dest
  // so order the sharers to optimize that path
  // If we're multicasting, order won't matter
  virtual std::list<int> orderSnoops(SharingVector sharers, int32_t src, bool optimize3hop, int32_t final_dest, bool multicast) = 0;

  // Calculate latency, and record BW for message from src to dest
  virtual int32_t addBW(int32_t src, int32_t dest) = 0;
  virtual int32_t addBW(int32_t src, std::list<int> &dest, bool include_replies = true) = 0;

  // Convert an memory address into an index corresponding to a location in the topology
  virtual int memoryToIndex(Flexus::SharedTypes::PhysicalMemoryAddress address) = 0;

  virtual void initialize(const std::string & aName) {
    theStats = new TopologyStats(aName, theNumLinks);
  }

  void setNumCores(int32_t n) {
    theNumCores = n;
  }

  void processGenericOptions(std::list< std::pair<std::string, std::string> > &arg_list) {
    std::list< std::pair<std::string, std::string> >::iterator iter = arg_list.begin();
    std::list< std::pair<std::string, std::string> >::iterator next = iter;
    next++;
    for (; iter != arg_list.end(); iter = next, next++) {
      if (iter->first == "MemLoc") {
//DBG_( Dev, ( << "setMemControllerLocation(" << iter->second << ")" ) );
        setMemControllerLocation(iter->second);
        arg_list.erase(iter);
      } else if (iter->first == "NumMem") {
//DBG_( Dev, ( << "setNumMemControllers(" << iter->second << ")" ) );
        setNumMemControllers(iter->second);
        arg_list.erase(iter);
      } else if (iter->first == "MemInterleaving") {
//DBG_( Dev, ( << "setMemControllerInterleaving(" << iter->second << ")" ) );
        setMemControllerInterleaving(iter->second);
        arg_list.erase(iter);
      }
    }
  }

  virtual ~AbstractTopology() {};

// To work properly, every subclass should have the following two additional public members:
//   1. static std::string name
//  2. static AbstractTopology* createInstance(std::string args)

};

// Don't hate me for this
// But I'm going to use some C++ Trickery to make some things easier.
// Here's how this works. In theory, there are many types of Topologies that all inherit from AbstractTopology
// We want to be able to easily create one of them, based on some flexus parameters
// Rather than having lots of parameters and a big ugly if statement to sort through them
// We use 1 String argument and a system of "Factories"
// Our AbstractFactory extracts the type of topology from the string
// This maps to a static factory object for a particular type of topology
// We then pass the remaining string argument to that static factory
// Yes, this is probably excessive with one or two topologies considering it's only called at startup
// But if you want to compare a bunch of different topologies, you'll be amazed how easy this makes things
// This could be made into a nice little templated thingy somewhere if someone wanted to take the time to do that

#define REGISTER_TOPOLOGY_TYPE(type,n) const std::string type::name = n; static ConcreteFactory<AbstractTopology,type> type ## _Factory
#define CREATE_TOPOLOGY(args)   AbstractFactory<AbstractTopology>::createInstance(args)

/* To create and use an instance of an AbstractTopology, you should ALWAYS do the following:
 *
 *  AbstractTopology *top_ptr = CREATE_TOPOLOGY(args);
 *  top_ptr->setNumCores(num_cores);
 *  top_ptr->initialize();
 *
 * Any other information needed to initialize it should be specificed as part of args
 * the format of args is "<name>[:<topology specific options>]"
 */

}; // namespace

#endif // FASTCMPDIRECTORY_TOPOLOGY_HPP

