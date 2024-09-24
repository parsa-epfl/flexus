#ifndef FLEXUS_FASTCACHE_INCLUSIVEMOESI_HPP_INCLUDED
#define FLEXUS_FASTCACHE_INCLUSIVEMOESI_HPP_INCLUDED

#include "CacheStats.hpp"

namespace nFastCache {

class InclusiveMOESI : public CoherenceProtocol
{
  public:
    InclusiveMOESI(bool using_traces,
                   message_function_t fwd,
                   message_function_t cnt,
                   invalidate_function_t inval,
                   bool DowngradeLRU = false,
                   bool SnoopLRU     = false)
      : CoherenceProtocol(fwd, cnt, inval)
    {

        DECLARE_REQUEST_ACTION(kModified,
                               kReadAccess,
                               SendNone,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillDirty,
                               HitReadModified);
        DECLARE_REQUEST_ACTION(kModified,
                               kWriteAccess,
                               SendNone,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillDirty,
                               HitWriteModified);
        DECLARE_REQUEST_ACTION(kModified,
                               kNAWAccess,
                               SendNone,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillDirty,
                               HitNAWModified);
        DECLARE_REQUEST_ACTION(kModified,
                               kFetchAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitFetchModified);
        DECLARE_REQUEST_ACTION(kModified,
                               kUpgrade,
                               SendNone,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillDirty,
                               HitUpgradeModified);
        DECLARE_REQUEST_ACTION(kModified,
                               kEvictClean,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               NoResponse,
                               HitEvictModified);
        DECLARE_REQUEST_ACTION(kModified, kEvictWritable, POISON, POISON, POISON, POISON, POISON, HitEvictWModified);
        DECLARE_REQUEST_ACTION(kModified, kEvictDirty, POISON, POISON, POISON, POISON, POISON, HitEvictDModified);

        DECLARE_REQUEST_ACTION(kOwned,
                               kReadAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitReadOwned);
        DECLARE_REQUEST_ACTION(kOwned,
                               kWriteAccess,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissWriteOwned);
        DECLARE_REQUEST_ACTION(kOwned,
                               kNAWAccess,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissNAWOwned);
        DECLARE_REQUEST_ACTION(kOwned,
                               kFetchAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitFetchOwned);
        DECLARE_REQUEST_ACTION(kOwned,
                               kUpgrade,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissUpgradeOwned);
        DECLARE_REQUEST_ACTION(kOwned,
                               kEvictClean,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               NoResponse,
                               HitEvictOwned);
        if (using_traces) {
            DECLARE_REQUEST_ACTION(kOwned,
                                   kEvictWritable,
                                   SendNone,
                                   NoAllocate,
                                   SameState,
                                   UpdateLRU,
                                   NoResponse,
                                   MissEvictWOwned);
            DECLARE_REQUEST_ACTION(kOwned,
                                   kEvictDirty,
                                   SendUpgrade,
                                   NoAllocate,
                                   M_State,
                                   UpdateLRU,
                                   NoResponse,
                                   MissEvictDOwned);
        } else {
            DECLARE_REQUEST_ACTION(kOwned, kEvictWritable, POISON, POISON, POISON, POISON, POISON, MissEvictWOwned);
            DECLARE_REQUEST_ACTION(kOwned, kEvictDirty, POISON, POISON, POISON, POISON, POISON, MissEvictDOwned);
        }

        DECLARE_REQUEST_ACTION(kExclusive,
                               kReadAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitReadExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kWriteAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillWritable,
                               HitWriteExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kNAWAccess,
                               SendNone,
                               NoAllocate,
                               M_State,
                               UpdateLRU,
                               FillWritable,
                               HitNAWExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kFetchAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitFetchExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kUpgrade,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillWritable,
                               HitUpgradeExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kEvictClean,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               NoResponse,
                               HitEvictExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kEvictWritable,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               NoResponse,
                               HitEvictWExclusive);
        DECLARE_REQUEST_ACTION(kExclusive,
                               kEvictDirty,
                               SendNone,
                               NoAllocate,
                               M_State,
                               UpdateLRU,
                               NoResponse,
                               HitEvictDExclusive);

        DECLARE_REQUEST_ACTION(kShared,
                               kReadAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitReadShared);
        DECLARE_REQUEST_ACTION(kShared,
                               kWriteAccess,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissWriteShared);
        DECLARE_REQUEST_ACTION(kShared,
                               kNAWAccess,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissNAWShared);
        DECLARE_REQUEST_ACTION(kShared,
                               kFetchAccess,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               FillValid,
                               HitFetchShared);
        DECLARE_REQUEST_ACTION(kShared,
                               kUpgrade,
                               SendUpgrade,
                               NoAllocate,
                               E_State,
                               UpdateLRU,
                               FillWritable,
                               MissUpgradeShared);
        DECLARE_REQUEST_ACTION(kShared,
                               kEvictClean,
                               SendNone,
                               NoAllocate,
                               SameState,
                               UpdateLRU,
                               NoResponse,
                               HitEvictShared);
        if (using_traces) {
            DECLARE_REQUEST_ACTION(kShared,
                                   kEvictWritable,
                                   SendNone,
                                   NoAllocate,
                                   SameState,
                                   UpdateLRU,
                                   NoResponse,
                                   MissEvictWShared);
            DECLARE_REQUEST_ACTION(kShared,
                                   kEvictDirty,
                                   SendUpgrade,
                                   NoAllocate,
                                   M_State,
                                   UpdateLRU,
                                   NoResponse,
                                   MissEvictDShared);
        } else {
            DECLARE_REQUEST_ACTION(kShared, kEvictWritable, POISON, POISON, POISON, POISON, POISON, MissEvictWShared);
            DECLARE_REQUEST_ACTION(kShared, kEvictDirty, POISON, POISON, POISON, POISON, POISON, MissEvictDShared);
        }

        DECLARE_REQUEST_ACTION(kInvalid,
                               kReadAccess,
                               SendRead,
                               Allocate,
                               DependState,
                               UpdateLRU,
                               FillDepend,
                               MissReadInvalid);
        DECLARE_REQUEST_ACTION(kInvalid,
                               kWriteAccess,
                               SendWrite,
                               Allocate,
                               E_State,
                               UpdateLRU,
                               FillDepend,
                               MissWriteInvalid);
        DECLARE_REQUEST_ACTION(kInvalid,
                               kNAWAccess,
                               SendNAW,
                               NoAllocate,
                               SameState,
                               NoUpdateLRU,
                               FillDepend,
                               MissNAWInvalid);
        DECLARE_REQUEST_ACTION(kInvalid,
                               kFetchAccess,
                               SendFetch,
                               Allocate,
                               DependState,
                               UpdateLRU,
                               FillValid,
                               MissFetchInvalid);

        if (using_traces) {
            DECLARE_REQUEST_ACTION(kInvalid,
                                   kUpgrade,
                                   SendWrite,
                                   Allocate,
                                   M_State,
                                   UpdateLRU,
                                   FillDepend,
                                   MissUpgradeInvalid);
            DECLARE_REQUEST_ACTION(kInvalid,
                                   kEvictClean,
                                   SendNone,
                                   NoAllocate,
                                   SameState,
                                   NoUpdateLRU,
                                   NoResponse,
                                   MissEvictInvalid);
            DECLARE_REQUEST_ACTION(kInvalid,
                                   kEvictWritable,
                                   SendNone,
                                   NoAllocate,
                                   SameState,
                                   NoUpdateLRU,
                                   NoResponse,
                                   MissEvictWInvalid);
            DECLARE_REQUEST_ACTION(kInvalid,
                                   kEvictDirty,
                                   SendNone,
                                   NoAllocate,
                                   SameState,
                                   NoUpdateLRU,
                                   NoResponse,
                                   MissEvictDInvalid);
        } else {
            DECLARE_REQUEST_ACTION(kInvalid, kUpgrade, POISON, POISON, POISON, POISON, POISON, MissUpgradeInvalid);
            DECLARE_REQUEST_ACTION(kInvalid, kEvictClean, POISON, POISON, POISON, POISON, POISON, MissEvictInvalid);
            DECLARE_REQUEST_ACTION(kInvalid, kEvictWritable, POISON, POISON, POISON, POISON, POISON, MissEvictWInvalid);
            DECLARE_REQUEST_ACTION(kInvalid, kEvictDirty, POISON, POISON, POISON, POISON, POISON, MissEvictDInvalid);
        }

#define MM MemoryMessage

        if (SnoopLRU) {
            // The arguments here are really template parameters
            // This means they have to be constants, so we have to replace SnoopLRU
            // with true or false as the last argument
            DECLARE_SNOOP_ACTION(kModified,
                                 MM::ReturnReq,
                                 MM::ReturnReplyDirty,
                                 NoSnoop,
                                 S_State,
                                 HitReturnReqModified,
                                 false,
                                 true);
            DECLARE_SNOOP_ACTION(kExclusive,
                                 MM::ReturnReq,
                                 MM::ReturnReply,
                                 DoSnoop,
                                 DependState,
                                 HitReturnReqExclusive,
                                 false,
                                 true);
        } else {
            DECLARE_SNOOP_ACTION(kModified,
                                 MM::ReturnReq,
                                 MM::ReturnReplyDirty,
                                 NoSnoop,
                                 S_State,
                                 HitReturnReqModified,
                                 false,
                                 false);
            DECLARE_SNOOP_ACTION(kExclusive,
                                 MM::ReturnReq,
                                 MM::ReturnReply,
                                 DoSnoop,
                                 DependState,
                                 HitReturnReqExclusive,
                                 false,
                                 false);
        }
        DECLARE_SNOOP_ACTION(kOwned,
                             MM::ReturnReq,
                             MM::ReturnReplyDirty,
                             NoSnoop,
                             S_State,
                             HitReturnReqOwned,
                             false,
                             false);
        DECLARE_SNOOP_ACTION(kShared,
                             MM::ReturnReq,
                             MM::ReturnReply,
                             NoSnoop,
                             SameState,
                             HitReturnReqShared,
                             false,
                             false);
        DECLARE_SNOOP_ACTION(kInvalid,
                             MM::ReturnReq,
                             MM::ReturnNAck,
                             NoSnoop,
                             SameState,
                             MissReturnReqInvalid,
                             false,
                             false);

        DECLARE_SNOOP_ACTION(kModified,
                             MM::Invalidate,
                             MM::InvUpdateAck,
                             DoSnoop,
                             I_State,
                             HitInvalidateModified,
                             false,
                             true);
        DECLARE_SNOOP_ACTION(kOwned,
                             MM::Invalidate,
                             MM::InvalidateAck,
                             DoSnoop,
                             I_State,
                             HitInvalidateOwned,
                             false,
                             true);
        DECLARE_SNOOP_ACTION(kExclusive,
                             MM::Invalidate,
                             SnoopDepend,
                             DoSnoop,
                             I_State,
                             HitInvalidateExclusive,
                             false,
                             true);
        DECLARE_SNOOP_ACTION(kShared,
                             MM::Invalidate,
                             MM::InvalidateAck,
                             DoSnoop,
                             I_State,
                             HitInvalidateShared,
                             false,
                             true);
        DECLARE_SNOOP_ACTION(kInvalid,
                             MM::Invalidate,
                             MM::Invalidate,
                             NoSnoop,
                             SameState,
                             MissInvalidateInvalid,
                             false,
                             false);

        if (DowngradeLRU) {
            DECLARE_SNOOP_ACTION(kModified,
                                 MM::Downgrade,
                                 MM::DownUpdateAck,
                                 DoSnoop,
                                 O_State,
                                 HitDowngradeModified,
                                 false,
                                 true);
            DECLARE_SNOOP_ACTION(kExclusive,
                                 MM::Downgrade,
                                 SnoopDepend,
                                 DoSnoop,
                                 DependState,
                                 HitDowngradeExclusive,
                                 false,
                                 true);
        } else {
            DECLARE_SNOOP_ACTION(kModified,
                                 MM::Downgrade,
                                 MM::DownUpdateAck,
                                 DoSnoop,
                                 O_State,
                                 HitDowngradeModified,
                                 false,
                                 false);
            DECLARE_SNOOP_ACTION(kExclusive,
                                 MM::Downgrade,
                                 SnoopDepend,
                                 DoSnoop,
                                 DependState,
                                 HitDowngradeExclusive,
                                 false,
                                 false);
        }
        DECLARE_SNOOP_ACTION(kOwned, MM::Downgrade, POISON, POISON, POISON, HitDowngradeOwned, false, false);
        DECLARE_SNOOP_ACTION(kShared, MM::Downgrade, POISON, POISON, POISON, HitDowngradeShared, false, false);
        DECLARE_SNOOP_ACTION(kInvalid,
                             MM::Downgrade,
                             MM::DowngradeAck,
                             NoSnoop,
                             SameState,
                             MissDowngradeInvalid,
                             false,
                             false);
#undef MM
    }

    // Need to maintain inclusion on evictions
    virtual MemoryMessage::MemoryMessageType evict(uint64_t tagset, CoherenceState_t state)
    {
        bool was_dirty = sendInvalidate(tagset, true, true);
        if (was_dirty || state == kModified || state == kOwned) {
            return MemoryMessage::EvictDirty;
        } else if (state == kExclusive) {
            return MemoryMessage::EvictWritable;
        }
        return MemoryMessage::EvictClean;
    }
};

} // namespace nFastCache

#endif
