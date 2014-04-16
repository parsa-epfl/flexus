#include <iostream>

#include "ProtSharedTypes.hpp"

namespace nProtocolEngine {

std::ostream & operator << (std::ostream & anOstream, tSendMsgDataSrc const aDataSrc) {
  const char * const name[3] = { "SEND_MSG_NO_DATA",
                                 "SEND_MSG_DATA_FROM_MEM",
                                 "SEND_MSG_DATA_FROM_CACHE"
                               };
  DBG_Assert(aDataSrc < static_cast<int>(sizeof(name)));
  anOstream << name[aDataSrc];
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tCpuOpType const anOp) {
  const char * const name[6] = { "CPU_INVALIDATE",
                                 "CPU_DOWNGRADE",
                                 "MISS_REPLY",
                                 "MISS_WRITABLE_REPLY",
                                 "UPGRADE_REPLY",
                                 "PREFETCH_READ_REPLY"
                               };
  DBG_Assert(anOp < static_cast<int>(sizeof(name)));
  anOstream << name[anOp];
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tCpuOpDataSrc const aDataSrc) {
  const char * const name[3] = { "NO_DATA",
                                 "DATA_FROM_MEM",
                                 "DATA_FROM_MSG"
                               };
  DBG_Assert(aDataSrc < static_cast<int>(sizeof(name)));
  anOstream << name[aDataSrc];
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tLockOpType const x) {
  const char * const name[3] = { "LOCK",
                                 "UNLOCK",
                                 "LOCK_SQUASH"
                               };
  DBG_Assert(x < static_cast<int>(sizeof(name)));
  anOstream << name[x];
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tMemOpType const x) {
  const char * const name[5] = { "READ",
                                 "WRITE",
                                 "MEM_LOCK",
                                 "MEM_UNLOCK",
                                 "MEM_LOCK_SQUASH"
                               };
  DBG_Assert(x < static_cast<int>(sizeof(name)));
  anOstream << name[x];
  return anOstream;
}

std::ostream & operator << (std::ostream & anOstream, tMemOpDest const x) {
  const char * const name[3] = { "DIRECTORY",
                                 "DATA_FROM_MESSAGE",
                                 "DIRECTORY_AND_DATA_FROM_MESSAGE"
                               };
  DBG_Assert(x < static_cast<int>(sizeof(name)));
  anOstream << name[x];
  return anOstream;
}

} // namespace nProtocolEngine
