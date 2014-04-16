#include <boost/dynamic_bitset.hpp>
#include <core/flexus.hpp>

#include <core/simulator_layout.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/MessageQueues.hpp>

#include <components/CMPDirectory/BankedController.hpp>

#include <fstream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <components/Common/Util.hpp>

using nCommonUtil::log_base2;

namespace nCMPDirectory {

BankedController::BankedController( const std::string & aName,
                                    int32_t aNumCores,
                                    int32_t aBlockSize,
                                    int32_t aNumBanks,
                                    int32_t aBankNum,
                                    int32_t anInterleaving,
                                    int32_t aSkewShift,
                                    int32_t aDirLatency,
                                    int32_t aDirIssueLat,
                                    int32_t aQueueSize,
                                    int32_t aMAFSize,
                                    int32_t anEBSize,
                                    std::string & aDirPolicy,
                                    std::string & aDirType,
                                    std::string & aDirConfig,
                                    bool aLocalDir
                                  )
  : theName(aName)
  , theControllers(aNumBanks)
  , theNumBanks(aNumBanks)
  , theLocalDirectory(aLocalDir)
  , RequestIn(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize))
  , SnoopIn(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize))
  , ReplyIn(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize))
  , RequestOut(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize))
  , SnoopOut(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize))
  , ReplyOut(aNumBanks, MessageQueue<MemoryTransport>(aQueueSize)) {
  for (int32_t i = 0; i < aNumBanks; i++) {
    theControllers[i] = new DirectoryController( aName,
        aNumCores,
        aBlockSize,
        aNumBanks,
        i,
        anInterleaving,
        aDirLatency,
        aDirIssueLat,
        aQueueSize,
        aMAFSize,
        anEBSize,
        aDirPolicy,
        aDirType,
        aDirConfig
                                               );
  }

  theBankShift = log_base2(anInterleaving);
  theSkewShift = aSkewShift;
  theBankMask = aNumBanks - 1;

}

BankedController::~BankedController() {
  for (int32_t i = 0; i < theNumBanks; i++) {
    delete theControllers[i];
  }
}

bool BankedController::isQuiesced() const {
  bool is_quiesced = true;
  for (int32_t i = 0; i < theNumBanks && is_quiesced; i++) {
    is_quiesced &= theControllers[i]->isQuiesced();
  }
  return is_quiesced;
}

void BankedController::saveState(std::string const & aDirName) {
  for (int32_t i = 0; i < theNumBanks; i++) {
    theControllers[i]->saveState(aDirName);
  }
}

void BankedController::loadState(std::string const & aDirName) {
  for (int32_t i = 0; i < theNumBanks; i++) {
    theControllers[i]->loadState(aDirName);
  }
}

int32_t BankedController::mapIncoming(const MemoryTransport & transport, int32_t index) {
  if (theLocalDirectory) {
    const PhysicalMemoryAddress & address = transport[MemoryMessageTag]->address();
    if (theSkewShift > 0) {
      return ((address >> theBankShift) ^ (address >> theSkewShift))& theBankMask;
    } else {
      return (address >> theBankShift) & theBankMask;
    }
  } else {
    return index;
  }
}

int32_t BankedController::mapOutgoing(const MemoryTransport & transport, int32_t index) {
  if (theLocalDirectory && transport[DestinationTag]->requester >= 0) {
    return transport[DestinationTag]->requester;
  } else {
    return index;
  }
}

void BankedController::processMessages() {
  static int32_t first_input_bank = 0;
  static int32_t first_output_bank = 0;

  // Move messages from Input Port Queues to Bank Queues
  int32_t index = first_input_bank;
  bool progress = false;
  for (int32_t i = 0; i < theNumBanks; i++, index++) {
    if (index >= theNumBanks) {
      index = 0;
    }

    if (!RequestIn[index].empty()) {
      const MemoryTransport msg = RequestIn[index].peek();
      int32_t dest_index = mapIncoming(msg, index);
      if (theControllers[dest_index]->requestQueueAvailable()) {
        theControllers[dest_index]->enqueueRequest(RequestIn[index].dequeue());
        progress = true;
      }
    }
    if (!SnoopIn[index].empty()) {
      const MemoryTransport msg = SnoopIn[index].peek();
      int32_t dest_index = mapIncoming(msg, index);
      if (!theControllers[dest_index]->SnoopIn.full()) {
        theControllers[dest_index]->SnoopIn.enqueue(SnoopIn[index].dequeue());
        progress = true;
      }
    }
    if (!ReplyIn[index].empty()) {
      const MemoryTransport msg = ReplyIn[index].peek();
      int32_t dest_index = mapIncoming(msg, index);
      if (!theControllers[dest_index]->ReplyIn.full()) {
        theControllers[dest_index]->ReplyIn.enqueue(ReplyIn[index].dequeue());
        progress = true;
      }
    }
  }
  if (progress) {
    first_input_bank++;
    if (first_input_bank >= theNumBanks) {
      first_input_bank = 0;
    }
  }

  // Process messages in each Bank
  for (int32_t i = 0; i < theNumBanks; i++) {
    theControllers[i]->processMessages();
  }

  // Move messages from Bank Queues to Output Port Queues
  index = first_output_bank;
  progress = false;
  for (int32_t i = 0; i < theNumBanks; i++, index++) {
    if (index >= theNumBanks) {
      index = 0;
    }

    if (!theControllers[index]->RequestOut.empty()) {
      const MemoryTransport msg = theControllers[index]->RequestOut.peek();
      int32_t out_index = mapOutgoing(msg, index);
      if (!RequestOut[out_index].full()) {
        RequestOut[out_index].enqueue(theControllers[index]->RequestOut.dequeue());
        progress = true;
      }
    }
    if (!theControllers[index]->SnoopOut.empty()) {
      const MemoryTransport msg = theControllers[index]->SnoopOut.peek();
      int32_t out_index = mapOutgoing(msg, index);
      if (!SnoopOut[out_index].full()) {
        SnoopOut[out_index].enqueue(theControllers[index]->SnoopOut.dequeue());
        progress = true;
      }
    }
    if (!theControllers[index]->ReplyOut.empty()) {
      const MemoryTransport msg = theControllers[index]->ReplyOut.peek();
      int32_t out_index = mapOutgoing(msg, index);
      if (!ReplyOut[out_index].full()) {
        ReplyOut[out_index].enqueue(theControllers[index]->ReplyOut.dequeue());
        progress = true;
      }
    }
  }
  if (progress) {
    first_output_bank++;
    if (first_output_bank >= theNumBanks) {
      first_output_bank = 0;
    }
  }

}

}; // namespace nCMPDirectory

