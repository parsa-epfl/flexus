/*! \file TaglessInclMOESIControllerImpl.hpp
 * \brief
 *
 * Defines the interfaces that the CacheController and the TaglessInclMOESIControllerImpl
 * use to talk to one another
 *
 * Revision History:
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 */

#ifndef _INCLUSIVE_MOESI_CONTROLLERIMPL_HPP
#define _INCLUSIVE_MOESI_CONTROLLERIMPL_HPP

#include "BaseCacheControllerImpl.hpp"
#include <components/NewCache/BasicCacheState.hpp>

#include <list>
#include <boost/intrusive_ptr.hpp>

namespace nNewCache {

class TaglessInclMOESIControllerImpl : public BaseCacheControllerImpl {
private:

  typedef BasicCacheState State;

  AbstractArray<State> *theArray;

  EvictBuffer<State> theEvictBuffer;

  bool theNAckAlways;

public:
  static BaseCacheControllerImpl * createInstance( std::list<std::pair<std::string, std::string> > &args, const ControllerParams & params);

  static const std::string name;

  // These are used for simple translations to various address types
  virtual MemoryAddress getBlockAddress (MemoryAddress const & anAddress) const {
    return theArray->blockAddress(anAddress);
  }

  virtual BlockOffset getBlockOffset(MemoryAddress const & anAddress) const {
    return theArray->blockOffset(anAddress);
  }

  virtual std::function<bool (MemoryAddress a, MemoryAddress b)> setCompareFn() const {
    return theArray->setCompareFn();
  }

private:
  TaglessInclMOESIControllerImpl( CacheController * aController,
                                  CacheInitInfo  * aInit,
                                  bool    anAlwaysNAck );

protected:

  virtual void saveArrayState(std::ostream & os) {
    theArray->saveState(os);
  }

  virtual bool loadArrayState(std::istream & is, bool aTextFlexpoint) {
    return theArray->loadState(is, theNodeId, aTextFlexpoint);
  }

  virtual uint32_t arrayEvictResourcesFree() const {
    return theArray->freeEvictionResources();
  }
  virtual bool arrayEvictResourcesAvailable() const {
    return theArray->evictionResourcesAvailable();
  }
  virtual void reserveArrayEvictResource() {
    theArray->reserveEvictionResource();
  }
  virtual void unreserveArrayEvictResource() {
    theArray->unreserveEvictionResource();
  }

  virtual AbstractEvictBuffer & evictBuffer() {
    return theEvictBuffer;
  }
  virtual const AbstractEvictBuffer & const_EvictBuffer() const {
    return theEvictBuffer;
  }

  // Perform lookup, select action and update cache state if necessary
  virtual std::tuple<bool, bool, Action> doRequest ( MemoryTransport        transport,
      bool                   has_maf_entry,
      TransactionTracker_p aWakingTracker =  TransactionTracker_p() );
#if 0
  virtual std::tuple<bool, bool, Action> doRequest ( MemoryMessage_p        msg,
      TransactionTracker_p   tracker,
      bool                   has_maf_entry,
      TransactionTracker_p aWakingTracker =  TransactionTracker_p() );
#endif

  virtual Action handleBackMessage ( MemoryTransport transport );

  virtual Action handleSnoopMessage ( MemoryTransport transport );

  virtual Action handleIprobe ( bool                 aHit,
                                MemoryTransport    transport );

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

  virtual void removeIdleWorkReservations(ProcessEntry_p process, Action & action);

  // take any necessary actions to start the idle work
  virtual Action handleIdleWork( MemoryTransport transport);

  virtual Action handleWakeSnoop ( MemoryTransport transport );

  virtual Action doEviction ();

  virtual bool canStartRequest( MemoryAddress const & anAddress) const {
    return (theRequestTracker.getActiveRequests(theArray->getSet(anAddress)) < theArray->requestsPerSet());
  }
  virtual void addPendingRequest( MemoryAddress const & anAddress) {
    theRequestTracker.startRequest(theArray->getSet(anAddress));
  }
  virtual void removePendingRequest( MemoryAddress const & anAddress) {
    theRequestTracker.endRequest(theArray->getSet(anAddress));
  }

private:

  typedef boost::intrusive_ptr<AbstractLookupResult<State> > LookupResult_p;

  // we use a specialized version of this
  virtual Action finalizeSnoop (  MemoryTransport transport,
                                  LookupResult_p  lookup );

  bool evictBlock(LookupResult_p victim);

  void allocateBlock(LookupResult_p result, MemoryAddress aBlockAddress);

  friend std::ostream & operator<<( std::ostream & os, const TaglessInclMOESIControllerImpl::State & state);

}; // class TaglessInclMOESIControllerImpl

}  // end namespace nNewCache

#endif // _INCLUSIVE_MOESI_CONTROLLERIMPL_HPP
