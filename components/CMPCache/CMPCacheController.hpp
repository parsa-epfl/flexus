#ifndef __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__
#define __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__

#include <list>
#include <components/Common/MessageQueues.hpp>
#include <components/CMPCache/ProcessEntry.hpp>
#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/AbstractCacheController.hpp>

namespace nCMPCache {

using namespace Flexus::SharedTypes;
using namespace nMessageQueues;

class CMPCacheController : public AbstractCacheController {

private:

  typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;
  typedef PipelineFifo<ProcessEntry_p> Pipeline;

  Pipeline theMAFPipeline;
  Pipeline theDirTagPipeline;
  Pipeline theDataPipeline;
  
  Stat::StatInstanceCounter<int64_t> theRequestInUtilization;
  Stat::StatInstanceCounter<int64_t> theMafUtilization;
  Stat::StatInstanceCounter<int64_t> theDirTagUtilization;
  Stat::StatInstanceCounter<int64_t> theDataUtilization;

  int32_t theMaxSnoopsPerRequest;

public:
  CMPCacheController(const CMPCacheInfo & anInfo);

  ~CMPCacheController();

  bool theIdleWorkScheduled;

  virtual bool isQuiesced() const;

  virtual void saveState(std::string const & aDirName);

  virtual void loadState(std::string const & aDirName);

  virtual void processMessages();

  // static memembers to work with AbstractFactory
  static AbstractCacheController * createInstance(std::list<std::pair<std::string, std::string> > & args, const CMPCacheInfo & params);
  static const std::string name;

protected:

  void scheduleNewProcesses();
  void advanceMAFPipeline();
  void advancePipeline();
  void finalizeProcess(ProcessEntry_p process);

  void runRequestProcess(ProcessEntry_p process);
  void runSnoopProcess(ProcessEntry_p process);
  void runReplyProcess(ProcessEntry_p process);
  void runCacheEvictProcess(ProcessEntry_p process);
  void runDirEvictProcess(ProcessEntry_p process);
  void runIdleWorkProcess(ProcessEntry_p process);
  void runWakeMAFProcess(ProcessEntry_p process);

  void sendRequest(ProcessEntry_p process);
  void sendSnoops(ProcessEntry_p process);
  void sendReply(ProcessEntry_p process);

}; // CMPCacheController

};

#endif // !__CMPCACHE_CMP_CACHE_CONTROLLER_HPP__
