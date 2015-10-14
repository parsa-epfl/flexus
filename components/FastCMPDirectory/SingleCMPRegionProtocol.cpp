#include <core/debug/debug.hpp>
#include <components/FastCMPDirectory/AbstractProtocol.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <unordered_map>
#include <list>

namespace nFastCMPDirectory {

// Same as above, but more allow EvictWritable when there are many sharers
class SingleCMPRegionProtocol : public AbstractProtocol {
private:
  typedef std::pair<SharingState, MMType> key_t;

  struct hash_func_t {
    std::size_t operator()( const key_t & key ) const {
      return ((int)key.second << 3 | (int)key.first);
    };
  };

  typedef std::unordered_map<key_t, PrimaryAction, hash_func_t> protocol_hash_t;
  protocol_hash_t theProtocolHash;

  PrimaryAction poison_action;

// BROKEN!!!
//

// Needs to be updated to support proper 2-response model

#define ADD_PROTOCOL_ACTION(req,state,snoop,terminal1,terminal2,resp,fwd,multi,rfs,poison) \
 theProtocolHash.insert(std::make_pair( std::make_pair(state, Flexus::SharedTypes::MemoryMessage::req),    \
   PrimaryAction(Flexus::SharedTypes::MemoryMessage::snoop, Flexus::SharedTypes::MemoryMessage::terminal1,  \
       Flexus::SharedTypes::MemoryMessage::terminal2,  Flexus::SharedTypes::MemoryMessage::resp, \
       Flexus::SharedTypes::MemoryMessage::NoRequest, fwd, multi, rfs, poison) ))

  inline void initializeProtocol(bool propagateCleanEvicts) {

    //                  Request, State,   Snoop,  Terminal[0], Terminal[1], Response, Fwd, Multi, RFS, POISON

    ADD_PROTOCOL_ACTION(ReadReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReplyWritable, true, false, false, false);
    ADD_PROTOCOL_ACTION(ReadReq, OneSharer,  ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);
    ADD_PROTOCOL_ACTION(ReadReq, ExclSharer,  ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);
    ADD_PROTOCOL_ACTION(ReadReq, ManySharers, ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);

    ADD_PROTOCOL_ACTION(FetchReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReply, true, false, false, false);
    ADD_PROTOCOL_ACTION(FetchReq, OneSharer,  ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);
    ADD_PROTOCOL_ACTION(FetchReq, ExclSharer,  ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);
    ADD_PROTOCOL_ACTION(FetchReq, ManySharers, ReturnReq, ReturnReply, ReturnReply, MissReply, true, false, true, false);

    // If we get an upgrade and there's only 1 sharer, no need to send invalidates
    ADD_PROTOCOL_ACTION(UpgradeReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  UpgradeReply, false, false, false, true);
    ADD_PROTOCOL_ACTION(UpgradeReq, OneSharer,  NoRequest, NoRequest,  NoRequest,  UpgradeReply, false, false, false, false);
    ADD_PROTOCOL_ACTION(UpgradeReq, ExclSharer,  NoRequest, NoRequest,  NoRequest,  UpgradeReply, false, false, false, false);
    ADD_PROTOCOL_ACTION(UpgradeReq, ManySharers, Invalidate, InvalidateAck, InvUpdateAck, UpgradeReply, false, true, false, false);

    ADD_PROTOCOL_ACTION(WriteReq, ZeroSharers, NoRequest, NoRequest,  NoRequest,  MissReplyWritable, true, false, false, false);
    ADD_PROTOCOL_ACTION(WriteReq, OneSharer,  Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, true, false, true, false);
    ADD_PROTOCOL_ACTION(WriteReq, ExclSharer,  Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, true, false, true, false);
    ADD_PROTOCOL_ACTION(WriteReq, ManySharers, Invalidate, InvalidateAck, InvUpdateAck, MissReplyWritable, true, true, false, false);

    if (propagateCleanEvicts) {
      ADD_PROTOCOL_ACTION(EvictClean, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictClean, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);

      ADD_PROTOCOL_ACTION(EvictWritable, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictWritable, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
    } else {
      ADD_PROTOCOL_ACTION(EvictClean, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictClean, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictClean, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);

      ADD_PROTOCOL_ACTION(EvictWritable, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, true);
      ADD_PROTOCOL_ACTION(EvictWritable, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);
      ADD_PROTOCOL_ACTION(EvictWritable, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, false);
    }

    ADD_PROTOCOL_ACTION(EvictDirty, ZeroSharers, NoRequest, NoRequest,  NoRequest,  NoRequest, false, false, false, true);
    ADD_PROTOCOL_ACTION(EvictDirty, OneSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
    ADD_PROTOCOL_ACTION(EvictDirty, ExclSharer,  NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);
    ADD_PROTOCOL_ACTION(EvictDirty, ManySharers, NoRequest, NoRequest,  NoRequest,  NoRequest, true, false, false, false);

    // Note: If the cache has an Owner state, then we allow EvictDirty
  };

#undef ADD_PROTOCOL_ACTION

public:

  SingleCMPRegionProtocol(bool propagateCleanEvicts) {
    initializeProtocol(propagateCleanEvicts);
  };

  virtual const PrimaryAction & getAction(SharingState state, MMType type, PhysicalMemoryAddress address) {
    protocol_hash_t::const_iterator iter = theProtocolHash.find( std::make_pair(state, type) );
    if (iter == theProtocolHash.end() || iter->second.poison) {
      //DBG_Assert(false, ( << "Poison Action! State: " << SharingStatePrinter(state) << ", Request: " << type ));
      return poison_action;
    }
    return iter->second;
  };

  static AbstractProtocol * createInstance(std::list<std::pair<std::string, std::string> > &args) {
    bool propagateCEs = false;
    std::list<std::pair<std::string, std::string> >::iterator iter = args.begin();
    for (; iter != args.end(); iter++) {
      if (iter->first == "PropagateCE") {
        if (iter->second == "true" || iter->second == "1") {
          propagateCEs = true;
        }
      }
    }
    return new SingleCMPRegionProtocol(propagateCEs);
  }

  static const std::string name;

};

REGISTER_PROTOCOL_TYPE(SingleCMPRegionProtocol, "SingleCMPRegion");

}; // namespace

