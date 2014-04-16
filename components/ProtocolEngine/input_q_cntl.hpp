#ifndef _INPUT_Q_CNTL_H_
#define _INPUT_Q_CNTL_H_

#include <list>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include "ProtSharedTypes.hpp"
#include "util.hpp"
#include "thread.hpp"
#include <core/stats.hpp>

namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

struct tSrvcProvider;
struct tThreadScheduler;

class tInputQueueController {

private:
  std::string theEngineName;
  tSrvcProvider & theSrvcProvider;
  tThreadScheduler & theThreadScheduler;

  struct tInputQueue {
    typedef std::list< boost::intrusive_ptr<tPacket> >::iterator iterator;
    std::list< boost::intrusive_ptr<tPacket> > theQueue;
  };

  tInputQueue theQueues[kMaxVC + 1];

  Stat::StatAverage statAvgInputQWaitTime;
  Stat::StatLog2Histogram statInputQWaitTimeDistribution;

public:
  bool isQuiesced() const {
    bool quiesced = true;
    for (int32_t i = 0; i < kMaxVC + 1 && quiesced; ++i) {
      quiesced = theQueues[i].theQueue.empty();
    }
    return quiesced;
  }

  tInputQueueController(std::string const & anEngineName, tSrvcProvider & aSrvcProvider, tThreadScheduler & aThreadScheduler);

  void dequeue(const tVC VC, tThread aThread = tThread()); //Remove a delivered packet from the head of the queue - it has been processed
  bool available(tVC aVC) const ;    //Returns true if there is an undelivered packet at the head of the queue
  void refuse(tVC aVC, boost::intrusive_ptr<tPacket> packet);   // Reject a packet from a thread and return it to the head of the input queue.
  void refuseAndRotate(tVC aVC, boost::intrusive_ptr<tPacket> packet);   // Reject a packet from a thread and return it to the tail of the input queue.
  void enqueue(boost::intrusive_ptr<tPacket> packet);   // enqueue (receive a message from the network)

  boost::intrusive_ptr<tPacket> getQueueHead(const tVC VC) {
    DBG_Assert( VC >= 0 && VC <= kMaxVC );
    DBG_Assert( ! theQueues[VC].theQueue.empty());
    DBG_Assert( theQueues[VC].theQueue.front());
    return theQueues[VC].theQueue.front();
  }

  uint32_t size(const tVC VC) const {
    DBG_Assert( VC >= 0 && VC <= kMaxVC );
    //The +1 is used to reserve one queue entry for packet rotation when there is a refuse.
    return theQueues[VC].theQueue.size() + 1;
  }

private:
  void handleInvalidate(boost::intrusive_ptr<tPacket>);
  void handleDowngrade(boost::intrusive_ptr<tPacket>);

};

}  // namespace nProtocolEngine

#endif // _INPUT_Q_CNTL_H_
