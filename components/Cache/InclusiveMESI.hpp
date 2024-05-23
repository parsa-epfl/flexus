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

/*! \file InclusiveMESI.hpp
 * \brief
 *
 * Defines the interfaces that the CacheController and the InclusiveMESI
 * use to talk to one another
 *
 * Revision History:
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 */

#ifndef _INCLUSIVEMESI_HPP__
#define _INCLUSIVEMESI_HPP__

#include "BaseCacheControllerImpl.hpp"
#include "BasicCacheState.hpp"

#include <boost/intrusive_ptr.hpp>
#include <list>

namespace nCache {

class InclusiveMESI : public BaseCacheControllerImpl
{
  private:
    typedef BasicCacheState State;

    AbstractArray<State>* theArray;

    EvictBuffer<State> theEvictBuffer;

    bool theNAckAlways;
    bool theSnoopLRU;
    bool theEvictAcksRequired;
    bool the2LevelPrivate;
    int32_t thePendingEvicts;
    int32_t theEvictThreshold;

  public:
    static BaseCacheControllerImpl* createInstance(std::list<std::pair<std::string, std::string>>& args,
                                                   const ControllerParams& params);

    static const std::string name;

    // These are used for simple translations to various address types
    virtual MemoryAddress getBlockAddress(MemoryAddress const& anAddress) const
    {
        return theArray->blockAddress(anAddress);
    }

    virtual BlockOffset getBlockOffset(MemoryAddress const& anAddress) const
    {
        return theArray->blockOffset(anAddress);
    }

    virtual std::function<bool(MemoryAddress a, MemoryAddress b)> setCompareFn() const
    {
        return theArray->setCompareFn();
    }

  private:
    InclusiveMESI(CacheController* aController,
                  CacheInitInfo* aInit,
                  bool anAlwaysNAck,
                  bool aSnoopLRU,
                  bool anEvictAcksRequired,
                  bool a2LevelPrivate);

  protected:
    virtual void saveArrayState(std::ostream& os) { theArray->saveState(os); }

    virtual bool loadArrayState(std::istream& is, bool aTextFlexpoint)
    {
        return theArray->loadState(is, theNodeId, aTextFlexpoint);
    }

    virtual void setProtectedBlock(MemoryAddress addr, bool flag)
    {
        LookupResult_p lookup = nullptr;
        lookup                = (*theArray)[addr];
        if (lookup != nullptr)
            lookup->setProtected(flag);
        else
            DBG_(Dev,
                 (<< " nullptr lookup in setProtectedBlock. This would cause "
                     "segmentation fault !!!!  "));
    }

    virtual uint32_t arrayEvictResourcesFree() const { return theArray->freeEvictionResources(); }
    virtual bool arrayEvictResourcesAvailable() const { return theArray->evictionResourcesAvailable(); }
    virtual void reserveArrayEvictResource() { theArray->reserveEvictionResource(); }
    virtual void unreserveArrayEvictResource() { theArray->unreserveEvictionResource(); }

    virtual AbstractEvictBuffer& evictBuffer() { return theEvictBuffer; }
    virtual const AbstractEvictBuffer& const_EvictBuffer() const { return theEvictBuffer; }

    // Perform lookup, select action and update cache state if necessary
    virtual std::tuple<bool, bool, Action> doRequest(MemoryTransport transport,
                                                     bool has_maf_entry,
                                                     TransactionTracker_p aWakingTracker = TransactionTracker_p());

    virtual Action handleBackMessage(MemoryTransport transport);

    virtual Action handleSnoopMessage(MemoryTransport transport);

    virtual Action handleIprobe(bool aHit, MemoryTransport transport);

  public:
    virtual void dumpEvictBuffer() const;

    /////////////////////////////////////////
    // Idle Work Processing -> additional controller specific work
    //

    // Do we have work available? Are the required resources available?
    virtual bool idleWorkAvailable();

    // get a message decsribing the work to be done
    // also reserve any necessary controller resources
    virtual MemoryMessage_p getIdleWorkMessage(ProcessEntry_p process);

    virtual void removeIdleWorkReservations(ProcessEntry_p process, Action& action);

    // take any necessary actions to start the idle work
    virtual Action handleIdleWork(MemoryTransport transport);

    virtual Action handleWakeSnoop(MemoryTransport transport);

    virtual Action doEviction();
    virtual uint32_t freeEvictBuffer() const;
    virtual bool evictableBlockExists(int32_t anIndex) const;

    virtual bool canStartRequest(MemoryAddress const& anAddress) const
    {
        return (theRequestTracker.getActiveRequests(theArray->getSet(anAddress)) < theArray->requestsPerSet());
    }
    virtual void addPendingRequest(MemoryAddress const& anAddress)
    {
        theRequestTracker.startRequest(theArray->getSet(anAddress));
    }
    virtual void removePendingRequest(MemoryAddress const& anAddress)
    {
        theRequestTracker.endRequest(theArray->getSet(anAddress));
    }

  private:
    typedef boost::intrusive_ptr<AbstractLookupResult<State>> LookupResult_p;

    // we use a specialized version of this
    virtual Action finalizeSnoop(MemoryTransport transport, LookupResult_p lookup);

    bool evictBlock(LookupResult_p victim);

    bool allocateBlock(LookupResult_p result, MemoryAddress aBlockAddress);

    friend std::ostream& operator<<(std::ostream& os, const InclusiveMESI::State& state);

}; // class InclusiveMESI

} // end namespace nCache

#endif // _INCLUSIVEMESI_HPP__
