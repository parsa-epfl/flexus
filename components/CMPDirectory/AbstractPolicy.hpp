#ifndef __ABSTRACT_POLICY_HPP
#define __ABSTRACT_POLICY_HPP

#include <components/Common/AbstractFactory.hpp>

#include <components/CMPDirectory/DirectoryInfo.hpp>
#include <components/CMPDirectory/ProcessEntry.hpp>
#include <components/CMPDirectory/EvictBuffer.hpp>

#include <iostream>

namespace nCMPDirectory {

using namespace Flexus::SharedTypes;

class AbstractPolicy {
public:
  virtual ~AbstractPolicy() {}

  virtual void handleRequest ( ProcessEntry_p process ) = 0;
  virtual void handleSnoop ( ProcessEntry_p process ) = 0;
  virtual void handleReply ( ProcessEntry_p process ) = 0;
  virtual void handleWakeMAF ( ProcessEntry_p process ) = 0;
  virtual void handleEvict ( ProcessEntry_p process ) = 0;
  virtual void handleIdleWork ( ProcessEntry_p process ) = 0;

  virtual bool isQuiesced() const = 0;

  virtual AbstractEvictBuffer & EB() = 0;
  virtual MissAddressFile & MAF() = 0;

  virtual bool hasIdleWorkAvailable() {
    return false;
  }
  virtual MemoryTransport getIdleWorkTransport() = 0;
  virtual MemoryTransport getEvictBlockTransport() = 0;
  virtual void getIdleWorkReservations( ProcessEntry_p process ) {}

  virtual bool EBHasSpace(const MemoryTransport & transport) {
    return !EB().full();
  }
  virtual int32_t getEBRequirements(const MemoryTransport & transport) {
    return 1;
  }

  virtual int32_t maxSnoopsPerRequest() {
    return 2;
  }

  virtual void wakeMAFs(MemoryAddress anAddress) = 0;

  virtual bool loadState( std::istream & is) = 0;

}; // AbstractPolicy

#define REGISTER_DIRECTORY_POLICY(type, n) const std::string type::name = n; static ConcreteFactory<AbstractPolicy,type,DirectoryInfo> type ## _Factory

}; // namespace nCMPDirectory

#endif // ! __ABSTRACT_POLICY_HPP

