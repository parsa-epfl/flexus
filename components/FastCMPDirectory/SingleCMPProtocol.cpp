#include <core/debug/debug.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>

#include <ext/hash_map>
#include <list>

namespace nFastCMPDirectory {

class SingleCMPProtocol : public AbstractProtocol {
private:
  typedef std::pair<SharingState, MMType> key_t;

  struct hash_func_t {
    std::size_t operator()( const key_t & key ) const {
      return ((int)key.second << 3 | (int)key.first);
    };
  };

  typedef __gnu_cxx::hash_map<key_t, PrimaryAction, hash_func_t> protocol_hash_t;
  protocol_hash_t theProtocolHash;

  PrimaryAction poison_action;

#define ADD_PROTOCOL_ACTION(req,state,snoop,terminal1,terminal2,resp1,resp2,fwd,multi,rfs,poison) \
 theProtocolHash.insert(std::make_pair( std::make_pair(state, Flexus::SharedTypes::MemoryMessage::req),    \
   PrimaryAction(Flexus::SharedTypes::MemoryMessage::snoop, Flexus::SharedTypes::MemoryMessage::terminal1,  \
       Flexus::SharedTypes::MemoryMessage::terminal2,  Flexus::SharedTypes::MemoryMessage::resp1, \
       Flexus::SharedTypes::MemoryMessage::resp2, fwd, multi, rfs, poison) ))

#define NO_RESP NumMemoryMessageTypes

  inline void initializeProtocol(bool propCEs, bool imprecise_sharers) {

    //                  Request, State,   Snoop,  Terminal[0], Terminal[1], Response1, Response2, Fwd, Multi, RFS, POISON

    ADD_PROTOCOL_ACTION(ReadReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReplyWritable, NO_RESP, true, false, false, false);
    ADD_PROTOCOL_ACTION(ReadReq, OneSharer,  ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);
    ADD_PROTOCOL_ACTION(ReadReq, ExclSharer,  ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);
    ADD_PROTOCOL_ACTION(ReadReq, ManySharers, ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);

    ADD_PROTOCOL_ACTION(FetchReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReply, NO_RESP, true, false, false, false);
    ADD_PROTOCOL_ACTION(FetchReq, OneSharer,  ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);
    ADD_PROTOCOL_ACTION(FetchReq, ExclSharer,  ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);
    ADD_PROTOCOL_ACTION(FetchReq, ManySharers, ReturnReq, ReturnReply, ReturnReplyDirty, MissReply, FwdReplyOwned, true, false, true, false);

    ADD_PROTOCOL_ACTION(UpgradeReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  UpgradeReply, NO_RESP, false, false, false, true);
    ADD_PROTOCOL_ACTION(UpgradeReq, OneSharer,  Invalidate, InvalidateAck, InvUpdateAck, UpgradeReply, UpgradeReply, false, false, true, false);
    ADD_PROTOCOL_ACTION(UpgradeReq, ExclSharer,  Invalidate, InvalidateAck, InvUpdateAck, UpgradeReply, UpgradeReply, false, false, true, false);
    ADD_PROTOCOL_ACTION(UpgradeReq, ManySharers, Invalidate, InvalidateAck, InvUpdateAck, UpgradeReply, UpgradeReply, false, true, false, false);

    ADD_PROTOCOL_ACTION(WriteReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReplyWritable, NO_RESP, true, false, false, false);
    ADD_PROTOCOL_ACTION(WriteReq, OneSharer,  Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, MissReplyDirty, true, false, true, false);
    ADD_PROTOCOL_ACTION(WriteReq, ExclSharer,  Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, MissReplyDirty, true, false, true, false);
    ADD_PROTOCOL_ACTION(WriteReq, ManySharers, Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, MissReplyDirty, true, true, false, false);

    if (propCEs) {
      ADD_PROTOCOL_ACTION(EvictClean, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictClean, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);

      ADD_PROTOCOL_ACTION(EvictWritable, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictWritable, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, !imprecise_sharers);
    } else {
      ADD_PROTOCOL_ACTION(EvictClean, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictClean, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, false);

      ADD_PROTOCOL_ACTION(EvictWritable, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictWritable, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, !imprecise_sharers);
    }

    ADD_PROTOCOL_ACTION(EvictDirty, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, false, false, false, true);
    ADD_PROTOCOL_ACTION(EvictDirty, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
    ADD_PROTOCOL_ACTION(EvictDirty, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);
    ADD_PROTOCOL_ACTION(EvictDirty, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, NO_RESP, true, false, false, false);

    // Note: If the cache has an Owner state, then we allow EvictDirty
  };

#undef ADD_PROTOCOL_ACTION

public:

  SingleCMPProtocol(bool procCEs, bool imprecise_sharers) {
    initializeProtocol(procCEs, imprecise_sharers);
  };

  virtual const PrimaryAction & getAction(SharingState state, MMType type, PhysicalMemoryAddress address) {
    protocol_hash_t::const_iterator iter = theProtocolHash.find( std::make_pair(state, type) );
    if (iter == theProtocolHash.end() || iter->second.poison) {
      DBG_Assert(false, ( << "Poison Action! State: " << SharingStatePrinter(state) << ", Request: " << type << ", Address: 0x" << std::hex << (uint64_t)address ));
      return poison_action;
    }
    return iter->second;
  };

  static AbstractProtocol * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    bool propagateCEs = false;
    bool imprecise_sharers = false;
    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (iter->first == "PropagateCE") {
        if (iter->second == "true" || iter->second == "1") {
          propagateCEs = true;
        }
      } else if (strcasecmp(iter->first.c_str(), "imprecise") == 0) {
        imprecise_sharers = boost::lexical_cast<bool>(iter->second);
      }
    }
    return new SingleCMPProtocol(propagateCEs, imprecise_sharers);
  }

  static const std::string name;

};

REGISTER_PROTOCOL_TYPE(SingleCMPProtocol, "SingleCMP");

}; // namespace

