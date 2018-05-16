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


#ifndef __ABSTRACT_POLICY_HPP
#define __ABSTRACT_POLICY_HPP

#include <components/CommonQEMU/AbstractFactory.hpp>

#include <components/CMPCache/CMPCacheInfo.hpp>
#include <components/CMPCache/ProcessEntry.hpp>
#include <components/CMPCache/EvictBuffer.hpp>
#include <components/CMPCache/CacheBuffers.hpp>

#include <iostream>

namespace nCMPCache {

using namespace Flexus::SharedTypes;

class AbstractPolicy {
public:
  virtual ~AbstractPolicy() {}

  virtual void handleRequest ( ProcessEntry_p process ) = 0;
  virtual void handleSnoop ( ProcessEntry_p process ) = 0;
  virtual void handleReply ( ProcessEntry_p process ) = 0;
  virtual void handleWakeMAF ( ProcessEntry_p process ) = 0;
  virtual void handleCacheEvict ( ProcessEntry_p process ) = 0;
  virtual void handleDirEvict ( ProcessEntry_p process ) = 0;
  virtual void handleIdleWork ( ProcessEntry_p process ) = 0;

  virtual bool isQuiesced() const = 0;

  virtual AbstractDirEvictBuffer & DirEB() = 0;
  virtual AbstractEvictBuffer & CacheEB() = 0;
  virtual const AbstractEvictBuffer & CacheEB() const = 0;
  virtual MissAddressFile & MAF() = 0;

  virtual bool hasIdleWorkAvailable() {
    return false;
  }
  virtual MemoryTransport getIdleWorkTransport() = 0;
  virtual MemoryTransport getCacheEvictTransport() = 0;
  virtual MemoryTransport getDirEvictTransport() = 0;
  virtual void getIdleWorkReservations( ProcessEntry_p process ) {}

  virtual bool freeCacheEBPending() {
    return CacheEB().freeSlotsPending();
  }
  virtual bool freeDirEBPending() {
    return DirEB().freeSlotsPending();
  }

  virtual bool EBFull() {
    return (DirEB().full() || !CacheEBHasSpace());
  }

  virtual int32_t getEBRequirements(const MemoryTransport & transport) {
    return 1;
  }

  virtual bool arrayEvictResourcesAvailable() const = 0;
  virtual bool CacheEBHasSpace() const {
    return (!CacheEB().full() && arrayEvictResourcesAvailable());
  }
  virtual bool EBHasSpace(const MemoryTransport & transport) {
    return (!DirEB().full() && CacheEBHasSpace());
  }

  virtual int32_t maxSnoopsPerRequest() {
    return 2;
  }

  virtual void wakeMAFs(MemoryAddress anAddress) = 0;

  virtual bool loadDirState( std::istream & is) = 0;
  virtual bool loadCacheState( std::istream & is) = 0;

  virtual void reserveArrayEvictResource(int32_t n) = 0;
  virtual void unreserveArrayEvictResource(int32_t n) = 0;

  virtual void reserveEB(int32_t n) {
    DirEB().reserve(n);
    reserveCacheEB(n);
  }

  virtual void reserveCacheEB(int32_t n) {
    CacheEB().reserve(n);
    reserveArrayEvictResource(n);
  }

  virtual void unreserveCacheEB(int32_t n) {
    CacheEB().unreserve(n);
    unreserveArrayEvictResource(n);
  }

  virtual void unreserveDirEB(int32_t n) {
    DirEB().unreserve(n);
  }

}; // AbstractPolicy

#define REGISTER_CMP_CACHE_POLICY(type, n) const std::string type::name = n; static ConcreteFactory<AbstractPolicy,type,CMPCacheInfo> type ## _Factory

}; // namespace nCMPCache

#endif // ! __ABSTRACT_POLICY_HPP

