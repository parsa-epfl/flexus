#ifndef FLEXUS__RMCPACKET_HPP_INCLUDED
#define FLEXUS__RMCPACKET_HPP_INCLUDED

#ifdef FLEXUS_RMCPacket_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::RMCEntry data type"
#endif
#define FLEXUS_RMCPacket_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>

#include <core/types.hpp>
#include <core/exception.hpp>

#include <components/CommonQEMU/rmc_breakpoints.hpp>

namespace Flexus {
namespace SharedTypes {

typedef enum RemoteOp
{
	RREAD,		//0
	RWRITE,		//1
	RRREPLY,	//2
	RWREPLY,	//3
	RMW,        //4
	RMWREPLY,   //5
   	SABRE,		//6
   	SABREREPLY,	//7
   	PREFETCH,	//8 - this is a prefetch issued by the RRPP. Used for SABRes
    SEND,       //9
    SENDREPLY,  //10
    RECV,       //11
    RECVREPLY,   //12
		SEND_TO_SHARED_CQ, //13 - this is a special "flavor" of SEND. It tells the RCP to write the SEND notification in the shared CQ instead of the private per-core CQ
	NUM_RMC_OP_TYPES
} RemoteOp;

std::ostream & operator << (std::ostream & s, RemoteOp const & aMemMsgType);

typedef struct {
	uint8_t data[64];
	uint8_t length;	//in bytes - always 64
} aBlock;

struct RMCPacket : public boost::counted_base  {
public:
	RMCPacket() {
		tid = 0;
	}

	RMCPacket(uint16_t atid,  uint16_t source, uint16_t dest, uint16_t cntxt, uint32_t oft, RemoteOp theOp, uint64_t buf, uint64_t thePktIndex, uint16_t plSize) {
		context = cntxt;
		offset = oft;
		tid = atid;
		src_nid = source;
		dst_nid = dest;
		op = theOp;
		bufaddr = buf;
		pl_size = plSize;
		pkt_index = thePktIndex;
		//DBG_Assert(plSize <= 64);
	}

	RMCPacket(uint16_t atid, uint16_t source, uint16_t dest, uint16_t cntxt, uint32_t oft, RemoteOp theOp, aBlock pld, uint64_t buf, uint64_t thePktIndex, uint16_t plSize) {
		context = cntxt;
		offset = oft;
		tid = atid;
		src_nid = source;
		dst_nid = dest;
		op = theOp;
		payload = pld;
		bufaddr = buf;
		pl_size = plSize;
		pkt_index = thePktIndex;
		//DBG_Assert(plSize <= 64);
	}

	~RMCPacket() {}

    uint16_t SABRElength;	//In cache blocks (64B) - field only used by SABRes & SEND
    bool succeeded;			//only for SABRes
	uint64_t offset, bufaddr, pkt_index;
	uint16_t context, tid, src_nid, dst_nid, vc;
	uint16_t pl_size;	//When the packet is in the UnrollQueue, the pl_size indicates the number of block requests remaining for that request
						//When the packet exits the RMC pipelines, pl_size is the packet's payload in bytes (either 0 or 64)
    uint8_t RMCid;		//Need this field to identify which RMC of the node initiated the request; reply has to get routed back to the same RMC
    uint64_t routing_hint;	//used by SABRes + source unrolling, so that all packets of same SABRe get routed to the same RRPP
							//this is just set to the base offset of the SABRe's first packet
	RemoteOp op;
	aBlock payload;
	uint64_t uniqueID;		//just used for statistics at the RRPP

	bool carriesPayload() {
		switch(op) {
			case RREAD:
			case RWREPLY:
      case SABRE:
			case SENDREPLY:
			case RECV:
			case RECVREPLY:
			case RMWREPLY:
			case PREFETCH:
				return false;
			case RWRITE:
			case RRREPLY:
			case SEND:
			case RMW:
				return true;
			case SABREREPLY:
				if (succeeded)
					return true;
				else
					return false;
			default:
				DBG_Assert(false, (<< "Unkown op type: " << op));
				return false;
		}
	}

	bool isReply() {
		switch(op) {
			case RRREPLY:
			case RWREPLY:
			case RMWREPLY:
			case SENDREPLY:
      case SABREREPLY:
			case RECVREPLY:
				return true;
			default:
				return false;
		}
	}

	bool isRequest() {
		switch(op) {
			case RREAD:
			case RWRITE:
			case RMW:
      case SABRE:
			case SEND:
			case RECV:
				return true;
			default:
				return false;
		}
	}

	void copy(RMCPacket aPacket) {
		offset = aPacket.offset;
		bufaddr = aPacket.bufaddr;
		context = aPacket.context;
		tid = aPacket.tid;
		src_nid = aPacket.src_nid;
		dst_nid = aPacket.dst_nid;
		vc = aPacket.vc;
		pl_size = aPacket.pl_size;
		op = aPacket.op;
		payload = aPacket.payload;
		pkt_index = aPacket.pkt_index;
        succeeded = aPacket.succeeded;
        SABRElength = aPacket.SABRElength;
        routing_hint = aPacket.routing_hint;
        RMCid = aPacket.RMCid;
	}

	inline void swapSrcDst() {
		uint16_t temp;
		temp = src_nid;
		src_nid = dst_nid;
		dst_nid = temp;
	}

    uint32_t packetSize() {	//in Bytes
		if (carriesPayload()) return 75;
		else return 11;
	}
};
}
}
#endif //FLEXUS__RMCPACKET_HPP_INCLUDED
