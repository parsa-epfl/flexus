#include <components/CommonQEMU/Slices/RMCPacket.hpp>

namespace Flexus {
namespace SharedTypes {

  std::ostream & operator << (std::ostream & s, RemoteOp const & aMemMsgType) {
    static char const * message_types[] = {
      "RREAD",		//0
    	"RWRITE",		//1
    	"RRREPLY",	//2
    	"RWREPLY",	//3
    	"RMW",        //4
    	"RMWREPLY",   //5
      "SABRE",		//6
      "SABREREPLY",	//7
      "PREFETCH",	//8 - this is a prefetch issued by the RRPP. Used for SABRes
      "SEND",       //9
      "SENDREPLY",  //10
      "RECV",  //11
      "RECVREPLY",   //12
      "SEND_TO_SHARED_CQ", //13
    	"NUM_RMC_OP_TYPES"
    };
    assert(aMemMsgType <= NUM_RMC_OP_TYPES );
    return s << message_types[aMemMsgType];
  }

} //namespace SharedTypes
} //namespace Flexus
