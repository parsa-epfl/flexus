#ifndef __DIRECTORY_CONTROLLER_HPP__
#define __DIRECTORY_CONTROLLER_HPP__

#include <list>
#include <components/Common/MessageQueues.hpp>
#include <components/CMPDirectory/ProcessEntry.hpp>
#include <components/CMPDirectory/AbstractPolicy.hpp>

namespace nCMPDirectory {

using namespace Flexus::SharedTypes;
using namespace nMessageQueues;

class DirectoryController {

private:

  typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;
  typedef PipelineFifo<ProcessEntry_p> Pipeline;

  std::string theName;

  DirectoryInfo theDirectoryInfo;

  Pipeline theMAFPipeline;
  Pipeline theDirectoryPipeline;

  Stat::StatInstanceCounter<int64_t> theMafEntriesUsed;
  Stat::StatInstanceCounter<int64_t> theMafEntriesActive;
  Stat::StatInstanceCounter<int64_t> theMafUtilization;
  Stat::StatInstanceCounter<int64_t> theDirUtilization;

  Stat::StatCounter theRequestMAFStalls;
  Stat::StatCounter theRequestEBStalls;
  Stat::StatCounter theRequestQueueStalls;

  AbstractPolicy * thePolicy;

  int32_t theMaxSnoopsPerRequest;

  // Make this queue private so we can track stats properly
  MessageQueue<MemoryTransport> RequestIn;

public:
  DirectoryController(const std::string & aName,
                      int32_t aNumCores,
                      int32_t aBlockSize,
                      int32_t aNumBanks,
                      int32_t aBankNum,
                      int32_t anInterleaving,
                      int32_t aDirLatency,
                      int32_t aDirIssueLat,
                      int32_t aQueueSize,
                      int32_t aMAFSize,
                      int32_t anEBSize,
                      std::string & aDirPolicy,
                      std::string & aDirType,
                      std::string & aDirConfig
                     );

  ~DirectoryController();

  // Message queues
  MessageQueue<MemoryTransport> SnoopIn;
  MessageQueue<MemoryTransport> ReplyIn;

  MessageQueue<MemoryTransport> RequestOut;
  MessageQueue<MemoryTransport> SnoopOut;
  MessageQueue<MemoryTransport> ReplyOut;

  void enqueueRequest(const MemoryTransport & transport);
  inline bool requestQueueAvailable() const {
    return !RequestIn.full();
  }

  bool isQuiesced() const;

  void saveState(std::string const & aDirName);

  void loadState(std::string const & aDirName);

  void processMessages();

  inline void reserveSnoopOut( ProcessEntry_p process, uint8_t n) {
    SnoopOut.reserve(n);
    process->snoop_out_reserved += n;
  }

  inline void reserveReplyOut( ProcessEntry_p process, uint8_t n) {
    ReplyOut.reserve(n);
    process->reply_out_reserved += n;
  }

  inline void reserveRequestOut( ProcessEntry_p process, uint8_t n) {
    RequestOut.reserve(n);
    process->req_out_reserved += n;
  }

  inline void reserveEB( ProcessEntry_p process, int32_t n) {
    thePolicy->EB().reserve(n);
    process->eb_reserved += n;
  }

  inline void reserveMAF( ProcessEntry_p process) {
    thePolicy->MAF().reserve();
    process->maf_reserved = true;
  }

  inline void unreserveSnoopOut( ProcessEntry_p process, uint8_t n) {
    SnoopOut.unreserve(n);
    process->snoop_out_reserved -= n;
  }

  inline void unreserveReplyOut( ProcessEntry_p process, uint8_t n) {
    ReplyOut.unreserve(n);
    process->reply_out_reserved -= n;
  }

  inline void unreserveRequestOut( ProcessEntry_p process, uint8_t n) {
    RequestOut.unreserve(n);
    process->req_out_reserved -= n;
  }

  inline void unreserveEB( ProcessEntry_p process) {
    thePolicy->EB().unreserve(process->eb_reserved);
    process->eb_reserved = 0;
  }

  inline void unreserveMAF( ProcessEntry_p process) {
    thePolicy->MAF().unreserve();
    process->maf_reserved = false;
  }

  void dumpMAFEntries(MemoryAddress addr);

protected:

  void scheduleNewProcesses();
  void advanceMAFPipeline();
  void advanceDirPipeline();
  void finalizeProcess(ProcessEntry_p process);

  void runRequestProcess(ProcessEntry_p process);
  void runSnoopProcess(ProcessEntry_p process);
  void runReplyProcess(ProcessEntry_p process);
  void runEvictProcess(ProcessEntry_p process);
  void runIdleWorkProcess(ProcessEntry_p process);
  void runWakeMAFProcess(ProcessEntry_p process);

  void sendRequest(ProcessEntry_p process);
  void sendSnoops(ProcessEntry_p process);
  void sendReply(ProcessEntry_p process);

}; // DirectoryController

};

#endif // !__DIRECTORY_CONTROLLER_HPP__
