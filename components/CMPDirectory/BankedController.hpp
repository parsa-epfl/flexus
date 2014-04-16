#ifndef __BANKED_CONTROLLER_HPP__
#define __BANKED_CONTROLLER_HPP__

#include <list>
#include <components/Common/MessageQueues.hpp>
#include <components/CMPDirectory/DirectoryController.hpp>

#include <vector>

namespace nCMPDirectory {

using namespace Flexus::SharedTypes;
using namespace nMessageQueues;

class BankedController {

private:

  std::string theName;

  std::vector<DirectoryController *> theControllers;

  int32_t theNumBanks, theBankShift, theSkewShift, theBankMask;

  bool theLocalDirectory;

  int32_t mapIncoming(const MemoryTransport & transport, int32_t index);
  int32_t mapOutgoing(const MemoryTransport & transport, int32_t index);

public:
  BankedController(const std::string & aName,
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
                  );

  ~BankedController();

  // Message queues
  std::vector<MessageQueue<MemoryTransport> > RequestIn;
  std::vector<MessageQueue<MemoryTransport> > SnoopIn;
  std::vector<MessageQueue<MemoryTransport> > ReplyIn;

  std::vector<MessageQueue<MemoryTransport> > RequestOut;
  std::vector<MessageQueue<MemoryTransport> > SnoopOut;
  std::vector<MessageQueue<MemoryTransport> > ReplyOut;

  bool isQuiesced() const;

  void saveState(std::string const & aDirName);

  void loadState(std::string const & aDirName);

  void processMessages();

}; // BankedController

};

#endif // !__BANKED_CONTROLLER_HPP__
