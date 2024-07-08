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

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <components/CMPCache/CMPCache.hpp>

#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>

#include <components/CMPCache/AbstractCacheController.hpp>

#define DBG_DefineCategories CMPCache
#define DBG_SetDefaultOps AddCat(CMPCache)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT CMPCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nCMPCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using std::unique_ptr;

class FLEXUS_COMPONENT(CMPCache) {
  FLEXUS_COMPONENT_IMPL(CMPCache);

private:
  std::unique_ptr<AbstractCacheController> theController;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CMPCache) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
  }

  bool isQuiesced() const {
    // TODO: fix this
    return theController->isQuiesced();
  }

  void saveState(std::string const &aDirName) {
    theController->saveState(aDirName);
  }

  void loadState(std::string const &aDirName) {
    theController->loadState(aDirName);
  }

  // Initialization
  void initialize() {
    DBG_(Dev, (<< "GroupInterleaving = " << cfg.GroupInterleaving));

    if (cfg.Cores == 0)
      cfg.Cores = Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2;

    CMPCacheInfo theInfo((int)flexusIndex(), statName(), cfg.Policy, cfg.DirectoryType,
                         cfg.DirectoryConfig, cfg.ArrayConfiguration, cfg.Cores, cfg.BlockSize,
                         cfg.Banks, cfg.BankInterleaving, cfg.Groups, cfg.GroupInterleaving,
                         cfg.MAFSize, cfg.DirEvictBufferSize, cfg.CacheEvictBufferSize,
                         cfg.EvictClean, cfg.CacheLevel, cfg.DirLatency, cfg.DirIssueLatency,
                         cfg.TagLatency, cfg.TagIssueLatency, cfg.DataLatency, cfg.DataIssueLatency,
                         cfg.QueueSize);

    //	theController.reset(new CMPCacheController(theInfo));
    theController.reset(AbstractFactory<AbstractCacheController, CMPCacheInfo>::createInstance(
        cfg.ControllerType, theInfo));
  }

  void finalize() {
  }

  // Ports
  //======

  // Request_In
  //-----------------
  bool available(interface::Request_In const &) {
    if (theController->RequestIn.full()) {
    }
    return !theController->RequestIn.full();
  }
  void push(interface::Request_In const &, MemoryTransport &aMessage) {
    DBG_Assert(!theController->RequestIn.full());
    DBG_(Trace, Comp(*this)(<< "Received on Port Request_In : " << *(aMessage[MemoryMessageTag])
                            << " from node " << aMessage[DestinationTag]->requester)
                    Addr(aMessage[MemoryMessageTag]->address()));
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Req");
    }

    DBG_Assert(aMessage[DestinationTag],
               (<< "Received Message with NO Dest Tag: " << *(aMessage[MemoryMessageTag])));

    theController->RequestIn.enqueue(aMessage);
  }

  // Snoop_In
  //-----------------
  bool available(interface::Snoop_In const &) {
    if (theController->SnoopIn.full()) {
    }
    return !theController->SnoopIn.full();
  }
  void push(interface::Snoop_In const &, MemoryTransport &aMessage) {
    DBG_Assert(!theController->SnoopIn.full());
    DBG_(Trace, Comp(*this)(<< "Received on Port Snoop_In : " << *(aMessage[MemoryMessageTag]))
                    Addr(aMessage[MemoryMessageTag]->address()));
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Snoop");
    }
    DBG_Assert(aMessage[DestinationTag],
               (<< "Received Message with NO Dest Tag: " << *(aMessage[MemoryMessageTag])));

    theController->SnoopIn.enqueue(aMessage);
  }

  // Reply_In
  //-----------------
  bool available(interface::Reply_In const &) {
    if (theController->ReplyIn.full()) {
    }
    return !theController->ReplyIn.full();
  }
  void push(interface::Reply_In const &, MemoryTransport &aMessage) {
    DBG_Assert(!theController->ReplyIn.full());
    DBG_(Trace, Comp(*this)(<< "Received on Port Reply_In : " << *(aMessage[MemoryMessageTag]))
                    Addr(aMessage[MemoryMessageTag]->address()));
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Directory Rx Reply");
    }
    DBG_Assert(aMessage[DestinationTag],
               (<< "Received Message with NO Dest Tag: " << *(aMessage[MemoryMessageTag])));

    theController->ReplyIn.enqueue(aMessage);
  }

  // DirectoryDrive
  //----------
  void drive(interface::CMPCacheDrive const &) {
    DBG_(VVerb, Comp(*this)(<< "DirectoryDrive"));
    theController->processMessages();
    busCycle();
  }

  void busCycle() {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this)(<< "bus cycle"));

    while (!theController->ReplyOut.empty() && FLEXUS_CHANNEL(Reply_Out).available()) {
      DBG_(Trace, (<< statName() << " Removing item from Reply queue."));
      MemoryTransport transport = theController->ReplyOut.dequeue();
      DBG_(Trace, Comp(*this)(<< "Sent on Port ReplyOut: " << *(transport[MemoryMessageTag]))
                      Addr(transport[MemoryMessageTag]->address()));
      FLEXUS_CHANNEL(Reply_Out) << transport;
    }
    while (!theController->SnoopOut.empty() && FLEXUS_CHANNEL(Snoop_Out).available()) {
      if (theController->SnoopOut.peek()[DestinationTag]->isMultipleMsgs()) {
        MemoryTransport transport = theController->SnoopOut.peek();
        DBG_(Trace, (<< statName() << " Removing Multicast from Snoop queue."));
        transport.set(DestinationTag, transport[DestinationTag]->removeFirstMulticastDest());
        transport.set(MemoryMessageTag, new MemoryMessage(*(transport[MemoryMessageTag])));
        DBG_(Trace, Comp(*this)(<< "Sent on Port SnoopOut: " << *(transport[MemoryMessageTag]))
                        Addr(transport[MemoryMessageTag]->address()));
        FLEXUS_CHANNEL(Snoop_Out) << transport;
      } else {
        DBG_(Trace, (<< statName() << " Removing item from Snoop queue."));
        MemoryTransport transport = theController->SnoopOut.dequeue();
        DBG_Assert(transport[MemoryMessageTag]);
        DBG_Assert(transport[DestinationTag],
                   (<< statName() << " no dest for msg: " << *transport[MemoryMessageTag]));
        if (transport[DestinationTag]->type == DestinationMessage::Multicast) {
          DBG_(Trace, (<< statName()
                       << " Converting multicast message: " << *transport[MemoryMessageTag]));
          transport[DestinationTag]->convertMulticast();
        }
        DBG_(Trace, Comp(*this)(<< "Sent on Port SnoopOut: " << *(transport[MemoryMessageTag]))
                        Addr(transport[MemoryMessageTag]->address()));
        FLEXUS_CHANNEL(Snoop_Out) << transport;
      }
    }
    while (!theController->RequestOut.empty() && FLEXUS_CHANNEL(Request_Out).available()) {
      DBG_(Trace, (<< "Removing item from request queue."));
      MemoryTransport transport = theController->RequestOut.dequeue();
      DBG_(Trace, Comp(*this)(<< "Sent on Port RequestOut: " << *(transport[MemoryMessageTag]))
                      Addr(transport[MemoryMessageTag]->address()));
      FLEXUS_CHANNEL(Request_Out) << transport;
    }
  }
};

} // End Namespace nCMPCache

FLEXUS_COMPONENT_INSTANTIATOR(CMPCache, nCMPCache);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CMPCache

#define DBG_Reset
#include DBG_Control()
