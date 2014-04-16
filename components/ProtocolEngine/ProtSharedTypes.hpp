#ifndef _PROT_SHARED_TYPES_H_
#define _PROT_SHARED_TYPES_H_

#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>

using boost::counted_base;

#include <components/Common/Slices/ProtocolMessage.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace nProtocolEngine {

using namespace Flexus::SharedTypes;

typedef Flexus::SharedTypes::PhysicalMemoryAddress tAddress;
typedef Flexus::SharedTypes::ProtocolMessage tPacket;
typedef Flexus::SharedTypes::Protocol::ProtocolMessageType tMessageType;
typedef Flexus::SharedTypes::DirectoryEntry tDirEntry;

// data source for sending a message
enum tSendMsgDataSrc {
  SEND_MSG_NO_DATA,
  SEND_MSG_DATA_FROM_MEM,
  SEND_MSG_DATA_FROM_CACHE    // data from cache line or writeback buffer
};
std::ostream & operator << (std::ostream & anOstream, tSendMsgDataSrc const aDataSrc);

// type of cpu operation
enum tCpuOpType {
  CPU_INVALIDATE,
  CPU_DOWNGRADE,
  MISS_REPLY,
  MISS_WRITABLE_REPLY,
  UPGRADE_REPLY,
  PREFETCH_READ_REPLY
};

std::ostream & operator << (std::ostream & anOstream, tCpuOpType const anOp);

// Indicate whether the cpu operation carries data
// If so, specify the source
typedef enum tCpuOpDataSrc {
  NO_DATA,
  DATA_FROM_MEM,
  DATA_FROM_MSG
} tCpuOpDataSrc;

std::ostream & operator << (std::ostream & anOstream, tCpuOpDataSrc const aDataSrc);

// lock operation type
enum tLockOpType {
  LOCK,
  UNLOCK,
  LOCK_SQUASH
} ;

std::ostream & operator << (std::ostream & anOstream, tLockOpType const x);

// memory operation type
enum tMemOpType {
  READ,
  WRITE,
  MEM_LOCK,
  MEM_UNLOCK,
  MEM_LOCK_SQUASH
};

std::ostream & operator << (std::ostream & anOstream, tMemOpType const x);

// memory operation destination
// implicitly define the data source (message)
typedef enum tMemOpDest {
  DIRECTORY,
  DATA_FROM_MESSAGE,
  DIRECTORY_AND_DATA_FROM_MESSAGE
} tMemOpDest;

std::ostream & operator << (std::ostream & anOstream, tMemOpDest const x);

} // namespace nProtocolEngine

#endif  // _PROT_SHARED_TYPES_H_
