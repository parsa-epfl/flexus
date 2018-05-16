// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__
#define __CMPCACHE_CMP_CACHE_CONTROLLER_HPP__

#include <list>
#include <components/CommonQEMU/MessageQueues.hpp>
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
