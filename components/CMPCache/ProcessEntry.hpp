
#ifndef __PROCESS_ENTRY_HPP__
#define __PROCESS_ENTRY_HPP__

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <list>
using namespace boost::multi_index;

#include <components/CMPCache/MissAddressFile.hpp>

namespace nCMPCache {

using namespace Flexus::SharedTypes;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

// Different types of processes
enum ProcessType
{
    eProcWakeMAF,
    eProcCacheEvict,
    eProcDirEvict,
    eProcReply,
    eProcSnoop,
    eProcRequest,
    eProcIdleWork
};

// Each process can carry-out one of these actions
enum ProcessAction
{
    eFwdAndWaitAck,
    eNotifyAndWaitAck,
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

enum PipelineAction
{
    eMAFStage,
    eTagReadStage,
    eTagWriteStage,
    eDataStage,
    eLongLatencyStage,
    eTransmitRequest,
    eTransmitSnoop,
    eTransmitReply,
    eMAFRemoval,
    eEvictMAFWake,
    eWaitingMAFWake
};

inline std::ostream&
operator<<(std::ostream& os, const PipelineAction& p)
{

    switch (p) {
        case eMAFStage: os << "MAFStage"; break;
        case eTagReadStage: os << "TagReadStage"; break;
        case eTagWriteStage: os << "TagWriteStage"; break;
        case eDataStage: os << "DataStage"; break;
        case eLongLatencyStage: os << "LongLatencyStage"; break;
        case eTransmitRequest: os << "TransmitRequest"; break;
        case eTransmitSnoop: os << "TransmitSnoop"; break;
        case eTransmitReply: os << "TransmitReply"; break;
        case eMAFRemoval: os << "MAFRemoval"; break;
        case eEvictMAFWake: os << "EvictMAFWake"; break;
        case eWaitingMAFWake: os << "WaitingMAFWake"; break;
    }
    return os;
}

struct ActionItem
{
    PipelineAction preceeding_stage;
    int32_t preceeding_iteration;
    PipelineAction type;
    int32_t repeat_count;
    MemoryTransport transport;

    ActionItem(PipelineAction pstage, int32_t piteration, PipelineAction type, int32_t count)
      : preceeding_stage(pstage)
      , preceeding_iteration(piteration)
      , type(type)
      , repeat_count(count)
    {
    }

    ActionItem(PipelineAction pstage, int32_t piteration, PipelineAction type, MemoryTransport& t)
      : preceeding_stage(pstage)
      , preceeding_iteration(piteration)
      , type(type)
      , repeat_count(1)
      , transport(t)
    {
    }

    ActionItem(PipelineAction pstage, int32_t piteration, PipelineAction type)
      : preceeding_stage(pstage)
      , preceeding_iteration(piteration)
      , type(type)
      , repeat_count(1)
    {
    }
};

inline std::ostream&
operator<<(std::ostream& os, const ActionItem& a)
{
    os << a.type << "(" << a.repeat_count << ")"
       << "<-" << a.preceeding_stage << "(" << a.preceeding_iteration << ")";
    return os;
}

typedef multi_index_container<
  ActionItem,
  indexed_by<ordered_non_unique<composite_key<ActionItem,
                                              member<ActionItem, PipelineAction, &ActionItem::preceeding_stage>,
                                              member<ActionItem, int32_t, &ActionItem::preceeding_iteration>>>>>
  ActionSet;

class ProcessEntry : public boost::counted_base
{
  private:
    ProcessType theType;
    ProcessAction theAction;

    MissAddressFile::iterator theMAFEntry;

    MemoryTransport theOrigTransport;
    MemoryTransport theReplyTransport;
    std::list<MemoryTransport> theSnoopTransports;
    int32_t theDirLookupsRequired;
    bool theRequiresData;
    bool theTransmitAfterTag;

    ActionSet theScheduledActions;
    int32_t theScheduledTagReads;

    explicit ProcessEntry() { DBG_Assert(false); }

    friend std::ostream& operator<<(std::ostream& os, const ProcessEntry& p);

  public:
    // Reservations, seperate fields for cleaner code + more flexibility
    uint8_t snoop_out_reserved;
    uint8_t req_out_reserved;
    uint8_t reply_out_reserved;
    bool maf_reserved;
    int32_t cache_eb_reserved;
    int32_t dir_eb_reserved;

    uint8_t snoops_scheduled;
    uint8_t reqs_scheduled;
    uint8_t replies_scheduled;

    bool remove_MAF;

    const ProcessType& type() const { return theType; }
    const ProcessAction& action() const { return theAction; }
    bool lookupRequired() const { return (theDirLookupsRequired > 0); }

    void addLookup() { theDirLookupsRequired++; }
    void completeLookup() { theDirLookupsRequired++; }
    int32_t requiredLookups() const { return theDirLookupsRequired; }

    bool requiresData() const { return theRequiresData; }
    void setRequiresData(bool _requires) { theRequiresData = _requires; }

    bool transmitAfterTag() const { return theTransmitAfterTag; }
    void setTransmitAfterTag(bool transmit) { theTransmitAfterTag = transmit; }

    bool noReservations() const
    {
        return (snoop_out_reserved == 0) && (reply_out_reserved == 0) && (req_out_reserved == 0) &&
               (maf_reserved == false) && (dir_eb_reserved == false) && (cache_eb_reserved == false);
    }

    ProcessEntry(const ProcessType& aType, const MemoryTransport& aTransport)
      : theType(aType)
      , theAction(eNoAction)
      , theOrigTransport(aTransport)
      , theDirLookupsRequired(1)
      , theRequiresData(false)
      , theTransmitAfterTag(false)
      , theScheduledTagReads(0)
      , snoop_out_reserved(0)
      , req_out_reserved(0)
      , reply_out_reserved(0)
      , maf_reserved(0)
      , cache_eb_reserved(0)
      , dir_eb_reserved(0)
      , snoops_scheduled(0)
      , reqs_scheduled(0)
      , replies_scheduled(0)
    {
    }

    ProcessEntry(const ProcessType& aType, MissAddressFile::iterator aMaf)
      : theType(aType)
      , theAction(eNoAction)
      , theMAFEntry(aMaf)
      , theOrigTransport(aMaf->transport())
      , theDirLookupsRequired(1)
      , theRequiresData(false)
      , theTransmitAfterTag(false)
      , theScheduledTagReads(0)
      , snoop_out_reserved(0)
      , req_out_reserved(0)
      , reply_out_reserved(0)
      , maf_reserved(0)
      , cache_eb_reserved(0)
      , dir_eb_reserved(0)
      , snoops_scheduled(0)
      , reqs_scheduled(0)
      , replies_scheduled(0)
    {
    }

    ~ProcessEntry()
    {
        DBG_Assert(noReservations(), (<< "Process Desctructor called while reservations held: " << *this));
        DBG_Assert(theScheduledActions.empty(),
                   (<< "Process Desctructor called while scheduled actions remain: " << *this));
    }

    void setAction(ProcessAction anAction) { theAction = anAction; }

    void setLookupsRequired(int32_t aRequired) { theDirLookupsRequired = aRequired; }

    MissAddressFile::iterator& maf() { return theMAFEntry; }

    void setMAF(MissAddressFile::iterator& aMAFEntry) { theMAFEntry = aMAFEntry; }

    MemoryTransport& transport() { return theOrigTransport; }

    const MemoryTransport& transport() const { return theOrigTransport; }

    void addSnoopTransport(MemoryTransport& aTransport)
    {
        DBG_Assert(aTransport[DestinationTag] != nullptr, (<< "MemoryMessage: " << (*aTransport[MemoryMessageTag])));
        theSnoopTransports.push_back(aTransport);
    }

    void setReplyTransport(MemoryTransport& aTransport)
    {
        DBG_Assert(aTransport[DestinationTag] != nullptr);
        theReplyTransport = aTransport;
    }

    std::list<MemoryTransport>& getSnoopTransports() { return theSnoopTransports; }

    MemoryTransport& getReplyTransport() { return theReplyTransport; }

    int32_t getSnoopReservations() const { return snoop_out_reserved; }
    int32_t getReplyReservations() const { return reply_out_reserved; }
    int32_t getRequestReservations() const { return req_out_reserved; }
    bool getMAFReserved() const { return maf_reserved; }
    int32_t getCacheEBReserved() const { return cache_eb_reserved; }
    bool getDirEBReserved() const { return dir_eb_reserved; }

    inline void scheduleRequest(PipelineAction preceeding_stage, int32_t iteration, MemoryTransport transport)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eTransmitRequest, transport));
        reqs_scheduled++;
    }
    inline void scheduleSnoop(PipelineAction preceeding_stage, int32_t iteration, MemoryTransport transport)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eTransmitSnoop, transport));
        snoops_scheduled++;
    }
    inline void scheduleReply(PipelineAction preceeding_stage, int32_t iteration, MemoryTransport transport)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eTransmitReply, transport));
        replies_scheduled++;
    }

    inline void setTagReadCount(int32_t count) { theScheduledTagReads = count; }
    inline int32_t tagReadsScheduled() const { return theScheduledTagReads; }

    inline void scheduleTagWriteStage(int32_t preceeding_reads, int32_t write_accesses)
    {
        DBG_Assert(write_accesses > 0);
        theScheduledActions.insert(ActionItem(eTagReadStage, preceeding_reads, eTagWriteStage, write_accesses));
    }
    inline void scheduleDataStage(int32_t preceeding_tag_reads, int32_t data_accesses)
    {
        DBG_Assert(data_accesses > 0);
        theScheduledActions.insert(ActionItem(eTagReadStage, preceeding_tag_reads, eDataStage, data_accesses));
    }
    inline void scheduleLongLatencyStage(PipelineAction preceeding_stage, int32_t iteration, int32_t stage_count)
    {
        DBG_Assert(stage_count > 0);
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eLongLatencyStage, stage_count));
    }
    inline std::pair<ActionSet::iterator, ActionSet::iterator> getReadyActions(PipelineAction stage, int32_t count)
    {
        return theScheduledActions.equal_range(std::make_tuple(stage, count));
    }
    inline void eraseActions(ActionSet::iterator first, ActionSet::iterator last)
    {
        theScheduledActions.erase(first, last);
    }
    inline void scheduleWakeAfterEvict(PipelineAction preceeding_stage, int32_t iteration)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eEvictMAFWake));
    }
    inline void scheduleMAFRemoval(PipelineAction preceeding_stage, int32_t iteration)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eMAFRemoval));
    }
    inline void scheduleWakeWaitingMAFs(PipelineAction preceeding_stage, int32_t iteration)
    {
        theScheduledActions.insert(ActionItem(preceeding_stage, iteration, eWaitingMAFWake));
    }
};

typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;

inline std::ostream&
operator<<(std::ostream& os, const ProcessEntry& p)
{
    os << "Proc(";
    switch (p.type()) {
        case eProcWakeMAF: os << "WakeMAF"; break;
        case eProcCacheEvict: os << "CacheEvict"; break;
        case eProcDirEvict: os << "DirEvict"; break;
        case eProcReply: os << "Reply"; break;
        case eProcSnoop: os << "Snoop"; break;
        case eProcRequest: os << "Request"; break;
        case eProcIdleWork: os << "IdleWork"; break;
    }
    os << ")";

    os << "{";
    switch (p.action()) {
        case eFwdAndWaitAck: os << "FwdAndWaitAck"; break;
        case eNotifyAndWaitAck: os << "NotifyAndWaitAck"; break;
        case eFwdAndWakeEvictMAF: os << "FwdAndWakeEvictMAF"; break;
        case eWakeEvictMAF: os << "WakeEvictMAF"; break;
        case eForward: os << "Forward"; break;
        case eFwdRequest: os << "FwdRequest"; break;
        case eFwdAndReply: os << "FwdAndReply"; break;
        case eReply: os << "Reply"; break;
        case eReplyAndRemoveMAF: os << "ReplyAndRemoveMAF"; break;
        case eRemoveMAF: os << "RemoveMAF"; break;
        case eRemoveAndWakeMAF: os << "RemoveAndWakeMAF"; break;
        case eStall: os << "Stall"; break;
        case eNoAction: os << "NoAction"; break;
    }
    os << "}";

    os << " | Reservation:";
    if (p.getSnoopReservations() > 0) os << " Snoop=" << p.getSnoopReservations();
    if (p.getRequestReservations() > 0) os << " Request=" << p.getRequestReservations();
    if (p.getReplyReservations() > 0) os << " Reply=" << p.getReplyReservations();
    if (p.getMAFReserved()) os << " MAF";
    if (p.getCacheEBReserved()) os << " CacheEB=" << p.getCacheEBReserved();
    if (p.getDirEBReserved()) os << " DirEB";
    os << " | ";

    if (p.transport()[MemoryMessageTag] != nullptr) { os << *(p.transport()[MemoryMessageTag]); }

    ActionSet::iterator iter = p.theScheduledActions.begin();
    ActionSet::iterator end  = p.theScheduledActions.end();

    if (iter != end) {
        os << "Scheduled(" << p.tagReadsScheduled() << "): ";
        for (; iter != end; iter++) {
            os << "[" << *iter << "]";
        }
        os << " | ";
    }

    return os;
}

}; // namespace nCMPCache

#endif // __PROCESS_ENTRY_HPP__
