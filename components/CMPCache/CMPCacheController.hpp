//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#ifndef __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__
#define __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__

#include <components/CMPCache/AbstractCacheController.hpp>
#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/ProcessEntry.hpp>
#include <components/CommonQEMU/MessageQueues.hpp>
#include <list>

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
  CMPCacheController(const CMPCacheInfo &anInfo);

  ~CMPCacheController();

  bool theIdleWorkScheduled;

  virtual bool isQuiesced() const;

  virtual void saveState(std::string const &aDirName);

  virtual void loadState(std::string const &aDirName);

  virtual void processMessages();

  // static memembers to work with AbstractFactory
  static AbstractCacheController *
  createInstance(std::list<std::pair<std::string, std::string>> &args, const CMPCacheInfo &params);
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

}; // namespace nCMPCache

#endif // !__CMPCACHE_CMP_CACHE_CONTROLLER_HPP__
