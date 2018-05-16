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


/*! \file TaglessInclusiveMOESIControllerImpl.hpp
 * \brief
 *
 * Defines the interfaces that the CacheController and the TaglessInclusiveMOESIControllerImpl
 * use to talk to one another
 *
 * Revision History:
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 */

#ifndef _TAGLESS_INCLUSIVE_MOESI_HPP__
#define _TAGLESS_INCLUSIVE_MOESI_HPP__

#include "BaseCacheControllerImpl.hpp"

#include <list>
#include <boost/intrusive_ptr.hpp>

#include <components/Cache/BasicCacheState.hpp>

namespace nCache {

class TaglessInclusiveMOESIControllerImpl : public BaseCacheControllerImpl {
private:

  typedef BasicCacheState State;

  AbstractArray<State> *theArray;

  EvictBuffer<State> theEvictBuffer;

  bool theNAckAlways;

  int32_t thePendingEvicts;
  int32_t theEvictThreshold;

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
  TaglessInclusiveMOESIControllerImpl( CacheController * aController,
                                       CacheInitInfo  * aInit,
                                       bool    anAlwaysNAck );

protected:

  virtual void saveArrayState(std::ostream & os) {
    theArray->saveState(os);
  }

  virtual bool loadArrayState(std::istream & is, bool aTextFlexpoint) {
    return theArray->loadState(is, theNodeId, aTextFlexpoint);
  }

  virtual void setProtectedBlock(MemoryAddress addr, bool flag){
    LookupResult_p lookup = (*theArray)[addr];
    if (lookup->state() != State::Invalid )
      lookup->setProtected(flag);
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
  virtual uint32_t freeEvictBuffer() const;
  virtual bool evictableBlockExists(int32_t anIndex) const;

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

  friend std::ostream & operator<<( std::ostream & os, const TaglessInclusiveMOESIControllerImpl::State & state);

}; // class TaglessInclusiveMOESIControllerImpl

}  // end namespace nCache

#endif // _TAGLESS_INCLUSIVE_MOESI_HPP__
