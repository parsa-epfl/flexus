#include <components/RMCConnector/RMCConnector.hpp>
#include <core/stats.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/qemu/mai_api.hpp>

#include <components/CommonQEMU/seq_map.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/export.hpp>
#include <fstream>
#include <string>
#include <bitset>

#include <components/CommonQEMU/Util.hpp>
using nCommonUtil::log_base2;

#define DBG_DefineCategories RMCConnector
#define DBG_SetDefaultOps AddCat(RMCConnector) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT RMCConnector
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nRMCConnector {

    using namespace Flexus;
    using namespace Core;
    using namespace SharedTypes;

    class FLEXUS_COMPONENT(RMCConnector) {
        FLEXUS_COMPONENT_IMPL( RMCConnector );

        std::vector<int> theRGPIndexMap, theRRPPIndexMap;

        int32_t BitsPerCycle;		//bandwidth
        int64_t replyLatency, RRPPCount;

        uint32_t RowBits, theMemShift, theMemMask;

        uint64_t RRPPLatSum, RRPPLatCount;
        bool isLoopback;

        std::list<queuePacket> packetInQueues[64][2];		//Up to 64 nodes - Separate requests and responses for each direction
        std::list<queuePacket> packetOutQueues[64][2];
        //std::bitset<64> inArbiter, outArbiter;
        uint8_t inArbiter[64], outArbiter[64];
        std::list<queueReplyPacket> RoundTripQueue;

        queuePacket tempPacket;
        queueReplyPacket tempReplyPacket;

        enum RRPPDistr_t {
            eRandom,
            eAddrLLC,
            eAddrMem
        } theRRPPDistr;

        public:
        FLEXUS_COMPONENT_CONSTRUCTOR(RMCConnector)
            : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
        {}

        public:

        void initialize(void) {
            DBG_(Dev, (<< "RMCConnector: setting network latency to " << cfg.networkLatency << " cycles, bandwidth (one direction) to " << cfg.bandwidth
                        << "Gbps, and initial remote RMC latency to 100 cycles"));

            DBG_Assert(Flexus::Core::ComponentManager::getComponentManager().systemWidth() <= 64, ( << "Queues provisioned for up to 64 cores. Changes required"));

            BitsPerCycle = (cfg.bandwidth/cfg.NodeDegree) / (cfg.cpu_freq/1000);	//rough conversion from Gbps to bits per cycle	-> SerDes serialization delay
            DBG_(Dev, (<< "RMCConnector data rate: " << BitsPerCycle << " bits per cycle."));

            isLoopback = cfg.Loopback;
            if (isLoopback) {
                DBG_(Dev, (<< "\tRequests looping back in and serviced by the same node.\n\tWARNING: There's currently no bandwidth limitation in the network. We run at RMC peak throughput."));
            } else {
                DBG_(Dev, (<< "\tOutgoing requests matched by same hypothetical requests, from some remote node."));
            }

            replyLatency = 2*cfg.networkLatency + 100;		//init value
            RRPPLatSum = 0;
            RRPPLatCount = 0;

            for (int i = 0; i < 64; i++) {
                inArbiter[i] = REPIN;
                outArbiter[i] = REPOUT;
            }

            RRPPCount = (int)sqrt((double)Flexus::Core::ComponentManager::getComponentManager().systemWidth());
            RowBits = log_base2(RRPPCount);
            theMemShift = log_base2(cfg.MemInterleaving) - 6;	//Offset of incoming packets is block address (not byte address)
            theMemMask = cfg.RRPPCount - 1;
            DBG_Assert(cfg.MemInterleaving >= 64);

            // Map RGPs and RRPPs to nodes
            std::string rgp_loc_str = cfg.RGPLocation;
            std::string rrpp_loc_str = cfg.RRPPLocation;
            std::list<int> rgp_loc_list, rrpp_loc_list;
            std::string::size_type loc;
            do {
                loc = rgp_loc_str.find(',', 0);
                if (loc != std::string::npos) {
                    std::string cur_loc = rgp_loc_str.substr(0, loc);
                    rgp_loc_str = rgp_loc_str.substr(loc + 1);
                    rgp_loc_list.push_back(boost::lexical_cast<int>(cur_loc));
                }
            } while (loc != std::string::npos);
            if (rgp_loc_str.length() > 0) {
                rgp_loc_list.push_back(boost::lexical_cast<int>(rgp_loc_str));
            }
            DBG_Assert((int)rgp_loc_list.size() == cfg.RGPCount,
                    ( << "Configuration specifies " << cfg.RGPCount
                      << " RGPs, but locations given for " << rgp_loc_list.size() ));
            do {
                loc = rrpp_loc_str.find(',', 0);
                if (loc != std::string::npos) {
                    std::string cur_loc = rrpp_loc_str.substr(0, loc);
                    rrpp_loc_str = rrpp_loc_str.substr(loc + 1);
                    rrpp_loc_list.push_back(boost::lexical_cast<int>(cur_loc));
                }
            } while (loc != std::string::npos);
            if (rrpp_loc_str.length() > 0) {
                rrpp_loc_list.push_back(boost::lexical_cast<int>(rrpp_loc_str));
            }
            DBG_Assert((int)rrpp_loc_list.size() == cfg.RRPPCount,
                    ( << "Configuration specifies " << cfg.RRPPCount
                      << " RRPPs, but locations given for " << rrpp_loc_list.size() ));

            theRGPIndexMap.resize(cfg.RGPCount, -1);
            theRRPPIndexMap.resize(cfg.RRPPCount, -1);

            for (int32_t i = 0; i < cfg.RGPCount; i++) {
                int32_t loc = rgp_loc_list.front();
                rgp_loc_list.pop_front();
                theRGPIndexMap[i] = loc;
            }
            for (int32_t i = 0; i < cfg.RRPPCount; i++) {
                int32_t loc = rrpp_loc_list.front();
                rrpp_loc_list.pop_front();
                theRRPPIndexMap[i] = loc;
            }

            // Determine how to distribute traffic to the RRPPs
            if (strcasecmp(cfg.RRPPDistribution.c_str(), "random") == 0) {
                theRRPPDistr = eRandom;
            } else if (strcasecmp(cfg.RRPPDistribution.c_str(), "address-LLC") == 0) {
                theRRPPDistr = eAddrLLC;
            } else if (strcasecmp(cfg.RRPPDistribution.c_str(), "address-mem") == 0) {
                theRRPPDistr = eAddrMem;
            } else {
                DBG_Assert(false, ( << "Unknown RRPP traffic distribution '" << cfg.RRPPDistribution << "'" ));
            }

            DBG_Assert(cfg.DirInterleaving == 64, ( << "RMCConnector currently only supports block interleaving for directory" ));
        }

        bool isQuiesced() const {
            for (int j=0; j<64; j++) {
                for (int i=0; i<2; i++) {
                    if (!packetInQueues[j][i].empty()) return false;
                    if (!packetOutQueues[j][i].empty()) return false;
                }
            }
            if (!RoundTripQueue.empty()) return false;
            return true;
        }

        void finalize(void) {}

        //The RRPPLatency is only useful for non-Loopback mode
        FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RRPPLatency);
        void push( interface::RRPPLatency const &, index_t aQueue, uint64_t & theLatency) {
            if (!isLoopback) {
                RRPPLatSum += theLatency;
                RRPPLatCount++;
                replyLatency = 2*cfg.networkLatency + (RRPPLatSum/RRPPLatCount);
                DBG_(CONNECTOR_DEBUG, ( << "RMCConnector received RRPP avg latency value: " << theLatency << ". Setting new reply latency to " << replyLatency));
            }
        }

        // request input port
        bool available( interface::MessageIn const &, index_t aQueue) {
            uint32_t theNode, theVC;
            theNode = aQueue / 2;
            theVC = aQueue % 2;

            //*************For flow control in cases where not all RMCs are on the edge and thus they cannot directly check availability of RMCConnector
            for (int i = 0; i<RRPPCount; i++) {
                if (theRRPPIndexMap[i] == (int)theNode) break;
                if (packetInQueues[theRRPPIndexMap[theNode%RRPPCount]][theVC].size() < cfg.buffers) return true;		//WARNING! This assumes that we have one RMCConnector access point per row, on column 0,
                else return false;																						//			 and all RGP/RCPs will route their traffic through their row's column 0 tile
            }
            //*************

            if (packetInQueues[theNode][theVC].size() < cfg.buffers) return true;
            DBG_Assert(!packetInQueues[theNode][theVC].empty());
            DBG_(CONNECTOR_DEBUG, (<< "Checking availability of input with id " << aQueue << " => packetInQueue["<<theNode<<"]["<<theVC <<"]"));
            DBG_(CONNECTOR_DEBUG, (<< "\t Queue is FULL! Size = " << packetInQueues[theNode][theVC].size() ));
            return false;
        }
        //FLEXUS_PORT_ALWAYS_AVAILABLE(MessageIn);
        void push( interface::MessageIn const &, index_t aQueue, RMCPacket & aPacket) {

            uint32_t theNode, theVC;
            theNode = aQueue / 2;
            theVC = aQueue % 2;

            uint32_t size;
            if (aPacket.carriesPayload()) size = 75*8;
            else size = 11*8;

            tempPacket.thePacket.copy(aPacket);
            tempPacket.cycleCounter = (size+BitsPerCycle-1)/BitsPerCycle;
            packetInQueues[theNode][theVC].push_back(tempPacket);
            DBG_(CONNECTOR_DEBUG, (<< "\tEnqueueing in packetInQueue[" << theNode << "][" << theVC << "]"));

            if (isLoopback) {
                tempReplyPacket.thePacket.copy(aPacket);
                tempReplyPacket.cycleCounter = (size+BitsPerCycle-1)/BitsPerCycle;

                uint16_t targetRMC;
                tempReplyPacket.cycleCounter += cfg.networkLatency;
                if (aPacket.isReply()) { //Replies are from RRPPs to RCPs. Destination determined by RMCid field
                    DBG_Assert(theVC == REPIN);
                    targetRMC = aPacket.RMCid;
                    DBG_(CONNECTOR_DEBUG, (<< "RMCConnector received a REPLY packet from RRPP " << theNode << " on VC " << theVC << " with src_nid="
                                << (int)aPacket.src_nid<<", dst_nid=" << (int)aPacket.dst_nid	<< " and tid=" << (int)aPacket.tid
                                << ". Packet injection requires " << (size/BitsPerCycle) << " cycles. Target RCP is " << targetRMC));
                } else {    //Requests are from RGPs to RRPPs. Destination is determined by routing_hint in packets.
                    DBG_Assert(theVC == REQIN);
                    uint16_t targetRRPP = ((aPacket.routing_hint >> 6) & (Flexus::Core::ComponentManager::getComponentManager().systemWidth() - 1)) >> RowBits;
                    targetRMC = theRRPPIndexMap[targetRRPP];
                    DBG_(CONNECTOR_DEBUG, (<< "RMCConnector received a REQUEST packet from RGP " << theNode << " on VC " << theVC << " with src_nid="
                                << (int)aPacket.src_nid<<", dst_nid=" << (int)aPacket.dst_nid	<< " and tid=" << (int)aPacket.tid
                                << ". Packet injection requires " << (size/BitsPerCycle) << " cycles. Target RRPP is " << targetRMC));
                }
                tempReplyPacket.theRMCId = targetRMC;
                tempReplyPacket.msgType = theVC;
                RoundTripQueue.push_back(tempReplyPacket);
            } else { //!isLoopback
                DBG_(CONNECTOR_DEBUG, (<< "RMCConnector received a packet from RMC " << theNode << " on VC " << theVC << " with src_nid="
                            << (int)aPacket.src_nid<<", dst_nid=" << (int)aPacket.dst_nid	<< " and tid=" << (int)aPacket.tid
                            << ". Packet injection requires " << tempPacket.cycleCounter << " cycles."));
                if (aPacket.isReply()) {
                    DBG_Assert(theVC == REPIN);
                    DBG_(CONNECTOR_DEBUG, (<< "This is a reply. Dropping..."));
                } else {
                    DBG_Assert(theVC == REQIN);
                    DBG_(CONNECTOR_DEBUG, (<< "This is a request from " << std::dec << (int)aPacket.src_nid << " to " << std::dec << (int)aPacket.dst_nid));

                    /*Need to do two things:
                     * A) Generate a synthetic request coming from a remote node
                     * B) Create response to that request, and send it back to the requester after an appropriate delay	 */

                    //A) Synthetic request. For now, this is just the outgoing request, mirrored
                    tempPacket.thePacket.swapSrcDst();
                    tempPacket.cycleCounter = size/BitsPerCycle;

                    uint32_t targetRRPP = 0;
                    switch (theRRPPDistr) {
                        case eRandom:	//Incoming remote requests are distributed to RRPPs at random!
                            targetRRPP = (rand()%RRPPCount);
                            break;
                        case eAddrLLC:	//Block interleaving across RRPPs. aPacket.offset should be a block address (not byte)
                            targetRRPP = ((aPacket.routing_hint >> 6) & (Flexus::Core::ComponentManager::getComponentManager().systemWidth() - 1)) >> RowBits;
                            DBG_Assert(targetRRPP <= RRPPCount);
                            if (aPacket.op != SABRE) DBG_Assert(targetRRPP == (((aPacket.offset >> 6) & (Flexus::Core::ComponentManager::getComponentManager().systemWidth() - 1)) >> RowBits));
                            DBG_(CONNECTOR_DEBUG, (<< "Inc requests are address interleaved across RRPPs. TargetRRPP for packet with offset 0x"
                                        << std::hex << aPacket.offset << " is " << std::dec << theRRPPIndexMap[targetRRPP]));
                            DBG_(CONNECTOR_DEBUG, (<< "\t((aPacket.offset >> 6) & (Flexus::Core::ComponentManager::getComponentManager().systemWidth() - 1)) = "
                                        << ((aPacket.offset >> 6) & (Flexus::Core::ComponentManager::getComponentManager().systemWidth() - 1) )
                                        << ", >> by RowBits : " << RowBits));
                            break;
                        case eAddrMem:	//Interleave to memory controllers
                            targetRRPP = (aPacket.offset >> theMemShift) & theMemMask;
                            break;
                        default:
                            DBG_Assert(false);
                    }
                    DBG_Assert(targetRRPP < RRPPCount);
                    targetRRPP = theRRPPIndexMap[targetRRPP];

                    packetOutQueues[targetRRPP][REQOUT].push_back(tempPacket);
                    DBG_(CONNECTOR_DEBUG, (<< "A) Mirroring packet as incoming request: src = " << std::dec << (int)tempPacket.thePacket.src_nid
                                << ", dst = " << std::dec << (int)tempPacket.thePacket.dst_nid
                                << ". Enqueueing in packetOutQueue[" << targetRRPP << "][" << REQOUT << "]"));

                    //B) Response
                    tempReplyPacket.thePacket = tempPacket.thePacket;
                    switch (tempReplyPacket.thePacket.op) {
                        case RREAD:
                            tempReplyPacket.thePacket.op = RRREPLY;
                            break;
                        case RWRITE:
                            tempReplyPacket.thePacket.op = RWREPLY;
                            break;
                        case RMW:
                            tempReplyPacket.thePacket.op = RMWREPLY;
                            break;
                        case SABRE:
                            tempReplyPacket.thePacket.op = SABREREPLY;
                            break;;
                        default:
                            DBG_Assert(false, (<< "Unknown request message type: " << tempReplyPacket.thePacket.op));
                    }
                    if (tempReplyPacket.thePacket.carriesPayload()) {
                        for (int i=0; i<64; i++) {
                            tempReplyPacket.thePacket.payload.data[i] = 0xAA;	//bogus data
                        }
                        tempReplyPacket.thePacket.payload.length = 64;
                    }
                    tempReplyPacket.cycleCounter = replyLatency;
                    tempReplyPacket.theRMCId = theNode;
                    tempReplyPacket.msgType = REPOUT;
                    tempReplyPacket.thePacket.succeeded = true;   //this is a bogus loopback, so there can be no atomicity violation
                    RoundTripQueue.push_back(tempReplyPacket);

                    DBG_(CONNECTOR_DEBUG, (<< "B) Generating bogus reply for request: src = " << std::dec << (int)tempPacket.thePacket.src_nid
                                << ", dst = " << std::dec << (int)tempPacket.thePacket.dst_nid
                                << ". Reply will be enqueued in packetOutQueue[" << tempReplyPacket.theRMCId << "][" << REPOUT << "] after " << replyLatency << " cycles"));
                }
            }
        }

        void drive( interface::ProcessMessages const & ) {
            uint32_t i;
            std::list<queuePacket>::iterator packetIter, prev;
            std::list<queueReplyPacket>::iterator prevReply, replyIter = RoundTripQueue.begin();

            //advance all messages in RoundTripQueue and move to packetOutQueues when they're ready to be delivered
            DBG_(CONNECTOR_VVERB, (<< "Advancing messages in RoundtripQueue"));
            while (replyIter != RoundTripQueue.end()) {
                replyIter->cycleCounter--;
                DBG_(CONNECTOR_VVERB, (<< "\tPacket cycleCounter: " << replyIter->cycleCounter));
                if (replyIter->cycleCounter == 0) {
                    queuePacket packet;
                    packet.thePacket = replyIter->thePacket;
                    if (packet.thePacket.carriesPayload()) {
                        packet.cycleCounter = (75*8+BitsPerCycle-1)/BitsPerCycle;
                    } else {
                        packet.cycleCounter = (11*8+BitsPerCycle-1)/BitsPerCycle;
                    }
                    packetOutQueues[replyIter->theRMCId][replyIter->msgType].push_back(packet);
                    if (!isLoopback) DBG_Assert(replyIter->msgType == REPOUT);
                    DBG_(CONNECTOR_VVERB, (<< "\t\t cycleCounter is 0, moving packet to packetOutQueues[" <<replyIter->theRMCId<<"]["<<REPOUT<<"]"));
                    prevReply = replyIter;
                    replyIter++;
                    RoundTripQueue.erase(prevReply);
                } else {
                    replyIter++;
                }
            }

            //Move messages in outQueues and InQueues
            //drive outQueues
            DBG_(CONNECTOR_VVERB, (<< "Advancing OutQueues"));
            for (i=0; i<Flexus::Core::ComponentManager::getComponentManager().systemWidth(); i++) {
                if (packetOutQueues[i][outArbiter[i]].empty() && !packetOutQueues[i][(outArbiter[i] == REQOUT) ? REPOUT : REQOUT].empty()) {
                    outArbiter[i] = (outArbiter[i] == REQOUT) ? REPOUT : REQOUT;
                }
                DBG_(CONNECTOR_VVERB, (<< "OutArbiter[" << i << "] = " << (int)outArbiter[i]));
                if (packetOutQueues[i][outArbiter[i]].begin() != packetOutQueues[i][outArbiter[i]].end()) {
                    packetIter = packetOutQueues[i][outArbiter[i]].begin();
                    DBG_(CONNECTOR_VVERB, (<< "OutQueue["<<i<<"]["<<outArbiter[i]<<"]: CycleCounter is " << packetIter->cycleCounter));
                    if (packetIter->cycleCounter == 0) {
                        if (FLEXUS_CHANNEL_ARRAY(MessageOut, i*2+outArbiter[i]).available()) {
                            if (outArbiter[i] == REPOUT) {
                                DBG_(CONNECTOR_DEBUG, (<< "RMCConnector delivering a REPLY packet with src_nid="<<std::hex
                                            << (int)packetIter->thePacket.src_nid<<", dst_nid=" << std::hex
                                            << (int)packetIter->thePacket.dst_nid	<< " and tid=" << std::dec << (int)packetIter->thePacket.tid
                                            <<" to node " << (int) packetIter->thePacket.dst_nid << "'s RCP " << i));
                            } else {
                                DBG_Assert(packetIter->thePacket.op == RREAD || packetIter->thePacket.op == RWRITE || packetIter->thePacket.op == RMW || packetIter->thePacket.op == SABRE);
                                DBG_(CONNECTOR_DEBUG, (<< "RMCConnector delivering a REQUEST packet with src_nid="
                                            << (int)packetIter->thePacket.src_nid<<", dst_nid="	<< (int)packetIter->thePacket.dst_nid
                                            << " and tid=" << (int)packetIter->thePacket.tid <<" to node " << (int) packetIter->thePacket.dst_nid
                                            << "'s RRPP " << i));
                            }
                            FLEXUS_CHANNEL_ARRAY(MessageOut, i*2+outArbiter[i]) << packetIter->thePacket;
                            packetOutQueues[i][outArbiter[i]].erase(packetIter);
                            outArbiter[i] = (outArbiter[i] == REQOUT) ? REPOUT : REQOUT;	//SWITCH
                        } else {
                            DBG_(CONNECTOR_VVERB, (<< "Packet is ready to be delivered, but RMC-" << i << " backpressures!"));
                        }
                    } else {
                        packetIter->cycleCounter--;
                    }
                }
                if (!(Flexus::Core::theFlexus->cycleCount() % 10000)) {
                    DBG_(QUEUES_DEBUG, ( << "packetOutQueue[" << i << "][REQOUT] size: " << packetOutQueues[i][REQOUT].size()
                                << ", packetOutQueue[" << i << "][REPOUT] size: " << packetOutQueues[i][REPOUT].size()));
                }
            }

            //drive inQueues to free up buffers
            DBG_(CONNECTOR_VVERB, (<< "Advancing inQueues"));
            for (i=0; i<Flexus::Core::ComponentManager::getComponentManager().systemWidth(); i++) {
                if (packetInQueues[i][inArbiter[i]].empty()
                        && !packetInQueues[i][(inArbiter[i] == REQIN) ? REPIN : REQIN].empty()) {
                    inArbiter[i] = (inArbiter[i] == REQIN) ? REPIN : REQIN;
                }
                if (!packetInQueues[i][inArbiter[i]].empty()) {
                    DBG_(CONNECTOR_VVERB, (<< "packetInQueue["<<i<<"]["<<inArbiter[i]<<"] is not empty. Advancing..."));
                    packetIter = packetInQueues[i][inArbiter[i]].begin();
                    if (packetIter->cycleCounter == 0) {
                        DBG_(CONNECTOR_VVERB, (<< "Freeing buffer in inQueue[" << i << "][" << (int) inArbiter[i] <<"]"));
                        packetInQueues[i][inArbiter[i]].erase(packetIter);
                        DBG_(CONNECTOR_DEBUG, (<< "\tnew queue Size = " << packetInQueues[i][inArbiter[i]].size()));
                        inArbiter[i] = (inArbiter[i] == REQIN) ? REPIN : REQIN;	//SWITCH
                    } else {
                        packetIter->cycleCounter--;
                    }
                } else {
                    DBG_(CONNECTOR_VVERB, (<< "packetInQueue["<<i<<"]["<<(int)inArbiter[i]<<"] is empty."));
                    DBG_(CONNECTOR_VVERB, (<< "packetInQueue["<<i<<"]["<<(int)((inArbiter[i] == REQIN) ? REPIN : REQIN)<<"] empty too?"
                                << packetInQueues[i][inArbiter[(inArbiter[i] == REQIN) ? REPIN : REQIN]].empty()
                                << "\tsize=" << (packetInQueues[i][inArbiter[(inArbiter[i] == REQIN) ? REPIN : REQIN]].size())));
                }
            }
            //DBG_(Tmp, ( << "Size of packetInQueues[0][1] = " << packetInQueues[0][1].size()
            //			<< ", isEmpty = " << 	packetInQueues[0][1].empty()));
        }
    };
}

FLEXUS_COMPONENT_INSTANTIATOR( RMCConnector, nRMCConnector );

FLEXUS_PORT_ARRAY_WIDTH( RMCConnector, RRPPLatency )   {
    return (Flexus::Core::ComponentManager::getComponentManager().systemWidth());
}

FLEXUS_PORT_ARRAY_WIDTH( RMCConnector, MessageIn )   {
    return (Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2);
}

FLEXUS_PORT_ARRAY_WIDTH( RMCConnector, MessageOut )   {
    return (Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2);
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT RMCConnector
