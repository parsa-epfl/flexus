#ifndef __PROCESS_ENTRY_HPP__
#define __PROCESS_ENTRY_HPP__

#include <list>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/CMPDirectory/MissAddressFile.hpp>

namespace nCMPDirectory {

using namespace Flexus::SharedTypes;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

// Different types of processes
enum ProcessType {
  eProcWakeMAF,
  eProcEvict,
  eProcReply,
  eProcSnoop,
  eProcRequest,
  eProcIdleWork
};

// Each process can carry-out one of these actions
enum ProcessAction {
  eFwdAndWaitAck,
  eNotifyAndWaitAck,
  eForwardAndRemoveMAF,
  eFwdAndWakeEvictMAF,
  eWakeEvictMAF,
  eForward,
  eFwdRequest,
  eFwdAndReply,
  eReply,
  eReplyAndRemoveMAF,
  eRemoveMAF,
  eRemoveAndWakeMAF,
  eStall,
  eNoAction
};

class ProcessEntry : public boost::counted_base {
private:
  ProcessType     theType;
  ProcessAction    theAction;

  MissAddressFile::iterator theMAFEntry;

  MemoryTransport    theOrigTransport;
  MemoryTransport    theReplyTransport;
  std::list<MemoryTransport> theSnoopTransports;
  int       theDirLookupsRequired;

public:
  // Reservations, seperate fields for cleaner code + more flexibility
  uint8_t snoop_out_reserved;
  uint8_t req_out_reserved;
  uint8_t reply_out_reserved;
  bool maf_reserved;
  int32_t eb_reserved;

  bool remove_MAF;

  const ProcessType & type() const {
    return theType;
  }
  const ProcessAction & action() const {
    return theAction;
  }
  bool lookupRequired() const {
    return (theDirLookupsRequired > 0);
  }

  void addLookup() {
    theDirLookupsRequired++;
  }
  void completeLookup() {
    theDirLookupsRequired++;
  }
  const int32_t requiredLookups() const {
    return theDirLookupsRequired;
  }

  bool noReservations() const {
    return (snoop_out_reserved == 0)
           && (reply_out_reserved == 0)
           && (req_out_reserved == 0)
           && (maf_reserved == false)
           && (eb_reserved == false);
  }

  ProcessEntry(const ProcessType & aType, const MemoryTransport & aTransport)
    : theType(aType), theAction(eNoAction), theOrigTransport(aTransport), theDirLookupsRequired(1),
      snoop_out_reserved(0), req_out_reserved(0), reply_out_reserved(0), maf_reserved(0), eb_reserved(0) {
  }

  ProcessEntry(const ProcessType & aType, MissAddressFile::iterator aMaf)
    : theType(aType), theAction(eNoAction), theMAFEntry(aMaf), theOrigTransport(aMaf->transport()), theDirLookupsRequired(1),
      snoop_out_reserved(0), req_out_reserved(0), reply_out_reserved(0), maf_reserved(0), eb_reserved(0) {
  }

  void setAction(ProcessAction anAction) {
    theAction = anAction;
  }

  void setLookupsRequired(int32_t aRequired) {
    theDirLookupsRequired = aRequired;
  }

  MissAddressFile::iterator & maf() {
    return theMAFEntry;
  }

  void setMAF(MissAddressFile::iterator & aMAFEntry) {
    theMAFEntry = aMAFEntry;
  }

  MemoryTransport & transport() {
    return theOrigTransport;
  }

  const MemoryTransport & transport() const {
    return theOrigTransport;
  }

  void addSnoopTransport(MemoryTransport & aTransport) {
    DBG_Assert(aTransport[DestinationTag] != nullptr, ( << "MemoryMessage: " << (*aTransport[MemoryMessageTag]) ));
    theSnoopTransports.push_back(aTransport);
  }

  void setReplyTransport(MemoryTransport & aTransport) {
    DBG_Assert(aTransport[DestinationTag] != nullptr);
    theReplyTransport = aTransport;
  }

  std::list<MemoryTransport>& getSnoopTransports() {
    return theSnoopTransports;
  }

  MemoryTransport & getReplyTransport() {
    return theReplyTransport;
  }

  int32_t getSnoopReservations() const {
    return snoop_out_reserved;
  }
  int32_t getReplyReservations() const {
    return reply_out_reserved;
  }
  int32_t getRequestReservations() const {
    return req_out_reserved;
  }
  bool getMAFReserved() const {
    return maf_reserved;
  }
  bool getEBReserved() const {
    return eb_reserved;
  }
};

typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;

inline std::ostream & operator<<(std::ostream & os, const ProcessEntry & p) {
  switch (p.type()) {
    case eProcWakeMAF:
      os << "ProcWakeMAF: ";
      break;
    case eProcEvict:
      os << "ProcEvict: ";
      break;
    case eProcReply:
      os << "ProcReply: ";
      break;
    case eProcSnoop:
      os << "ProcSnoop: ";
      break;
    case eProcRequest:
      os << "ProcRequest: ";
      break;
    case eProcIdleWork:
      os << "ProcIdleWork: ";
      break;
  }

  switch (p.action()) {
    case eFwdAndWaitAck:
      os << "FwdAndWaitAck, ";
      break;
    case eNotifyAndWaitAck:
      os << "NotifyAndWaitAck, ";
      break;
    case eForwardAndRemoveMAF:
      os << "ForwardAndRemoveMAF, ";
      break;
    case eFwdAndWakeEvictMAF:
      os << "FwdAndWakeEvictMAF, ";
      break;
    case eWakeEvictMAF:
      os << "WakeEvictMAF, ";
      break;
    case eForward:
      os << "Forward, ";
      break;
    case eFwdRequest:
      os << "FwdRequest, ";
      break;
    case eFwdAndReply:
      os << "FwdAndReply, ";
      break;
    case eReply:
      os << "Reply, ";
      break;
    case eReplyAndRemoveMAF:
      os << "ReplyAndRemoveMAF, ";
      break;
    case eRemoveMAF:
      os << "RemoveMAF, ";
      break;
    case eRemoveAndWakeMAF:
      os << "RemoveAndWakeMAF, ";
      break;
    case eStall:
      os << "Stall, ";
      break;
    case eNoAction:
      os << "NoAction, ";
      break;
  }

  os << "snoop reservations: " << p.getSnoopReservations() << ", ";
  os << "request reservations: " << p.getRequestReservations() << ", ";
  os << "reply reservations: " << p.getReplyReservations() << ", ";
  if (p.getMAFReserved()) {
    os << "MAF reserved, ";
  }
  if (p.getEBReserved()) {
    os << "EB reserved, ";
  }
  if (p.transport()[MemoryMessageTag] != nullptr) {
    os << *(p.transport()[MemoryMessageTag]);
  }
  return os;
}

#if 0
inline std::ostream & operator<<(std::ostream & os, const ProcessAction & action) {
  switch (action) {
    case eFwdAndWaitAck:
      os << "FwdAndWaitAck, ";
      break;
    case eNotifyAndWaitAck:
      os << "NotifyAndWaitAck, ";
      break;
    case eForwardAndRemoveMAF:
      os << "ForwardAndRemoveMAF, ";
      break;
    case eFwdAndWakeEvictMAF:
      os << "FwdAndWakeEvictMAF, ";
      break;
    case eWakeEvictMAF:
      os << "WakeEvictMAF, ";
      break;
    case eForward:
      os << "Forward, ";
      break;
    case eFwdRequest:
      os << "FwdRequest, ";
      break;
    case eFwdAndReply:
      os << "FwdAndReply, ";
      break;
    case eReply:
      os << "Reply, ";
      break;
    case eReplyAndRemoveMAF:
      os << "ReplyAndRemoveMAF, ";
      break;
    case eRemoveMAF:
      os << "RemoveMAF, ";
      break;
    case eRemoveAndWakeMAF:
      os << "RemoveAndWakeMAF, ";
      break;
    case eStall:
      os << "Stall, ";
      break;
    case eNoAction:
      os << "NoAction, ";
      break;
  }
  return os;
}
#endif

}; // namespace nCMPDirectory

#endif // __PROCESS_ENTRY_HPP__
