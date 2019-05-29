#include <components/RMCTrace/RMCTrace.hpp>
#include <core/stats.hpp>

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
#include <math.h>

#define DBG_DefineCategories RMCTrace
#define DBG_SetDefaultOps AddCat(RMCTrace) Comp(*this)
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT RMCTrace
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

/*
   namespace Flexus {
   namespace Simics {
   namespace API {
   extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"
} //namespace API
} //namespace Simics
} //namespace Flexus
*/

namespace nRMCTrace {

    using namespace Flexus;
    using namespace Qemu;
    using namespace Core;
    using namespace SharedTypes;
    using namespace RMCTypes;

    static uint32_t RMCcount = 0, READY_COUNT_TARGET = 0;
    static uint32_t ReadyCount = 0;
    static RMCStatus status;

    uint32_t CORES = 0, CORES_PER_RMC = 0, MACHINES = 0, CORES_PER_MACHINE = 0;

    class FLEXUS_COMPONENT(RMCTrace) {
        FLEXUS_COMPONENT_IMPL( RMCTrace );

        std::string loadFname;

        bool NORMC, isRGP, isRRPP;

        uint32_t RMCid;													//This RMC's id
        //uint64_t wqHIndex, wqTIndex, cqHIndex, cqTIndex, wqStart, cqStart, tidStart;		//Physical addresses
        //uint64_t wqHIndexV, wqTIndexV, cqHIndexV, cqTIndexV, wqStartV, cqStartV, tidStartV;
        uint64_t wqSize, cqSize;		//# of entries
        uint32_t TLBsize;
        uint8_t uniqueTID;
        std::vector < std::pair < uint64_t, uint64_t > > RMC_TLB;		//for LRU order
        API::conf_object_t * theCPUs[64], *theCPU;									//the CPUs this RMC is coupled with
        std::map<uint64_t, std::list<RMCPacket> > outQueue;		//outstanding messages for all other RMCs
        std::map<uint64_t, std::list<RMCPacket> > inQueue;		//incoming messages from all other RMCs
        uint64_t iMask, jMask, kMask, PObits, PTbaseV[64];					//for Page Tables - PTbaseV is the Vaddress of the base of the PT
        std::tr1::unordered_map<uint64_t, uint64_t> PTv2p[64];				//translations for PT pages
        std::tr1::unordered_map<uint64_t, uint64_t> bufferv2p[64];
        std::tr1::unordered_map<uint64_t, uint64_t> contextv2p[64];
        std::tr1::unordered_map<uint64_t, uint64_t> sendbufv2p[64];	//translations for send buf
        std::tr1::unordered_map<uint64_t, uint64_t> recvbufv2p[64];	//translations for recv buf
        std::tr1::unordered_map<uint64_t, std::pair <uint64_t, uint64_t> > contextMap[64];			//context IDs to context start V address - one map per machine
        //pair: first is base address, second is context size
        //RMC dedicated SRAMs
        std::tr1::unordered_map<uint16_t, ITT_table_entry_t> ITT_table;
        std::tr1::unordered_map<uint16_t, ITT_table_entry_t>::iterator ITT_table_iter;
        std::tr1::unordered_map<uint16_t, QP_table_entry_t> QP_table;
        std::tr1::unordered_map<uint16_t, QP_table_entry_t>::iterator QP_table_iter;
        std::tr1::unordered_map<uint16_t, messaging_struct_t> messaging_table;
        std::tr1::unordered_map<uint16_t, messaging_struct_t>::iterator messaging_table_iter;

        public:
        FLEXUS_COMPONENT_CONSTRUCTOR(RMCTrace)
            : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
              , theLoadMessage(MemoryMessage::LoadReq)
              , theStoreMessage(MemoryMessage::StoreReq)
        {}

        public:
        MemoryMessage theLoadMessage;
        MemoryMessage theStoreMessage;
        RMCEntry entry;
        RMCPacket packet;

        uint64_t theCycleCount, transactionCounter;
        std::map<uint64_t, uint64_t> tempTranslation;
        std::map<uint64_t, uint64_t>::iterator iter;
        std::list<RMCPacket>::iterator msgIter;
        std::map<uint64_t, std::list<RMCPacket> >::iterator queueIter;

        uint64_t prevPage;

        void saveState(std::string const &aDirName) {
            if (NORMC) return;
            std::string fname( aDirName );

            if (cfg.CopyEmuRMC) {
                fname += "/rmc_state_" + boost::lexical_cast<std::string>(RMCid);
                const std::string from = loadFname + "/rmc_state_" + boost::lexical_cast<std::string>(RMCid);
                const std::string to = fname;

                DBG_(Crit, ( << "CopyEmuRMC enabled. Copying RMC state from file " << from << " to " << to) );

                std::ios_base::openmode omode = std::ios::in;
                std::ifstream ifs(from.c_str(), omode);
                if (!ifs.good()) {
                    DBG_(Crit, ( << "No RMC state file " << from << " found, although CopyEmuRMC=true.") );
                    return;
                }
                boost::iostreams::filtering_stream<boost::iostreams::input> in;
                in.push(ifs);

                omode = std::ios::out;
                std::ofstream ofs(to.c_str(), omode);
                boost::iostreams::filtering_stream<boost::iostreams::output> out;
                out.push(ofs);

                std::string line;
                uint64_t RMCid_simics, address, paddr, bp_type, value;
                while( in >> RMCid_simics >> std::hex >> address >> paddr >> std::dec >> bp_type >> value ) {
                    out << RMCid_simics << " " << std::hex << address << " " << paddr << " " << std::dec << bp_type << " " << value << '\n';
                    if (bp_type == ALL_SET) {
                        ReadyCount++;
                        DBG_(Tmp, ( <<" RMC " << RMCid <<" is ready to start servicing on behalf of cpu <undef>"
                                    << ". ReadyCount: " << ReadyCount << ", ReadyCount Target: " << READY_COUNT_TARGET << ", ALL_SET id=" << value));
                        DBG_Assert(ReadyCount == READY_COUNT_TARGET, ( << "Not all RMC have been initialized." ) ); //ustiugov: FIXME important assertion!!!
                    }
                    DBG_(Tmp, ( << "Written \"" << RMCid_simics << " " << std::hex << address << " " << paddr << " " << std::dec << bp_type << " " << value << "\"") );
                }
                return;
            }

            fname += "/" + statName();
            if (cfg.GZipFlexpoints) {
                fname += ".gz";
            }

            std::ios_base::openmode omode = std::ios::out;
            if (!cfg.TextFlexpoints) {
                omode |= std::ios::binary;
            }

            std::ofstream ofs(fname.c_str(), omode);

            boost::iostreams::filtering_stream<boost::iostreams::output> out;
            if (cfg.GZipFlexpoints) {
                out.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(9)));
            }
            out.push(ofs);

            std::tr1::unordered_map<uint64_t, uint64_t>::iterator it1;

            if (cfg.TextFlexpoints) {
                out << "{ " << PAGE_BITS << " " << PT_I << " " << PT_J << " " << PT_K << " }";	//important values to ensure consistency between flexpoints

                //Save RMC_TLB
                out << " { " << TLBsize << " " << RMC_TLB.size() << " [";
                uint32_t i;
                for (i=0; i<RMC_TLB.size(); i++) {
                    out << " ( " <<  RMC_TLB[i].first << " " << RMC_TLB[i].second << " )";
                }
                out << " ] }";

                //PTv2p translations
                for (i=0; i<64; i++) {
                    out << " { " << PTbaseV[i] << " " << PTv2p[i].size() << " [";
                    for (std::tr1::unordered_map<uint64_t, uint64_t>::iterator it = PTv2p[i].begin(); it != PTv2p[i].end(); it++)
                        out << " ( " << it->first << " " << it->second << " )";
                    out << " ] }";
                }

                //buffers
                for (i=0; i<64; i++) {
                    out << " { " << bufferv2p[i].size() << " [";
                    for (it1 = bufferv2p[i].begin(); it1 != bufferv2p[i].end(); it1++) {
                        out << " ( " << it1->first << " " << it1->second << " )";
                    }
                    out << " ] }";
                }

                //context
                for (i=0; i<64; i++) {
                    out << " { " << contextv2p[i].size() << " [";
                    for (it1 = contextv2p[i].begin(); it1 != contextv2p[i].end(); it1++) {
                        out << " ( " << it1->first << " " << it1->second << " )";
                    }
                    out << " ] }";
                }

                //contextMap
                std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator it2;
                for (i=0; i<64; i++) {
                    out << " { " << contextMap[i].size() << " [";
                    for (it2 = contextMap[i].begin(); it2 != contextMap[i].end(); it2++) {
                        out << " ( " << it2->first << " " << it2->second.first << " " << it2->second.second << " )";
                    }
                    out << " ] } ";
                }

                out << "{ " << outQueue.size();
                for (queueIter = outQueue.begin(); queueIter != outQueue.end(); queueIter++) {
                    out << " { " << queueIter->first << " " << queueIter->second.size() << " [";
                    for (msgIter = queueIter->second.begin(); msgIter != queueIter->second.end(); msgIter++) {
                        out << " ( " << msgIter->tid << " " << msgIter->src_nid << " " << msgIter->dst_nid << " "
                            << msgIter->context << " " << msgIter->offset << " " << msgIter->bufaddr << " "
                            << msgIter->pkt_index << " ";
                        switch (msgIter->op) {
                            case RREAD:
                                out << "0 ";
                                break;
                            case RWRITE:
                                out << "1 ";
                                break;
                            case RRREPLY:
                                out << "2 ";
                                break;
                            case RWREPLY:
                                out << "3 ";
                                break;
                            default:
                                DBG_Assert(false, ( << "Wrong message type"));
                        }
                        if (msgIter->op == RWRITE || msgIter->op == RRREPLY) {	//payload has useful info
                            for (uint32_t i = 0; i<64; i++) {
                                out << std::dec << (int) msgIter->payload.data[i] << " ";
                            }
                        }
                        out << ")";
                    }
                    out << " ] }";
                }
                out << " } { " << inQueue.size();
                for (queueIter = inQueue.begin(); queueIter != inQueue.end(); queueIter++) {
                    out << " { " << queueIter->first << " " << queueIter->second.size() << " [";
                    for (msgIter = queueIter->second.begin(); msgIter != queueIter->second.end(); msgIter++) {
                        out << " ( " << msgIter->tid << " " << msgIter->src_nid << " " << msgIter->dst_nid << " "
                            << msgIter->context << " " << msgIter->offset << " " << msgIter->bufaddr << " "
                            << msgIter->pkt_index << " ";
                        switch (msgIter->op) {
                            case RREAD:
                                out << "0 ";
                                break;
                            case RWRITE:
                                out << "1 ";
                                break;
                            case RRREPLY:
                                out << "2 ";
                                break;
                            case RWREPLY:
                                out << "3 ";
                                break;
                            default:
                                DBG_Assert(false, ( << "Wrong message type"));
                        }
                        if (msgIter->op == RWRITE || msgIter->op == RRREPLY) {	//payload has useful info
                            for (uint32_t i = 0; i<8; i++) {
                                out << std::dec << (int) msgIter->payload.data[i] << " ";
                            }
                        }
                        out << ")";
                    }
                    out << " ] }";
                }
                out << " } " ;
                out << RMCid << " ";
                if (status == INIT)
                    out << 0 << " ";
                else
                    out << 1 << " ";

                out << (uint64_t)uniqueTID << " "  << wqSize << " " << cqSize ;

                //Now save the SRAM structures
                //QP table
                out << " { " << QP_table.size();
                QP_table_iter = QP_table.begin();
                while (QP_table_iter != QP_table.end()) {
                    out << " [ " << QP_table_iter->first << " " << QP_table_iter->second.WQ_base << " "
                        << QP_table_iter->second.WQ_tail << " "
                        << (int)QP_table_iter->second.WQ_SR << " "
                        << QP_table_iter->second.CQ_base << " "
                        << QP_table_iter->second.CQ_head << " "
                        << (int)QP_table_iter->second.CQ_SR << " "
                        << QP_table_iter->second.lbuf_base << " "
                        << QP_table_iter->second.lbuf_size << " ] ";
                    QP_table_iter++;
                }
                out << " } ";

                //ITT table
                out << " { " << ITT_table.size();
                ITT_table_iter = ITT_table.begin();
                while (ITT_table_iter != ITT_table.end()) {
                    out << " [ " << ITT_table_iter->first << " " << ITT_table_iter->second.counter << " "
                        << ITT_table_iter->second.QP_id << " "
                        << ITT_table_iter->second.WQ_index << " "
                        << ITT_table_iter->second.lbuf_addr << " ] ";
                    ITT_table_iter++;
                }
                out << " } ";

                //save the send/recv state structures
                messaging_table_iter = messaging_table.begin();
                out << " { " << messaging_table.size();
                while (messaging_table_iter != messaging_table.end()) {
                    out << " [ " << messaging_table_iter->second.ctx_id << " "
                        << messaging_table_iter->second.num_nodes << " "
                        << messaging_table_iter->second.send_buf_addr << " "
                        << messaging_table_iter->second.recv_buf_addr << " "
                        << messaging_table_iter->second.msg_entry_size << " "
                        << messaging_table_iter->second.msg_buf_entry_count << " "
                        << messaging_table_iter->second.SR << " " << " ] ";
                    messaging_table_iter++;
                }
                out << " } ";

                //save the translations for the send buf
                for (i=0; i<64; i++) {
                    out << " { " << sendbufv2p[i].size() << " [";
                    for (it1 = sendbufv2p[i].begin(); it1 != sendbufv2p[i].end(); it1++) {
                        out << " ( " << it1->first << " " << it1->second << " )";
                    }
                    out << " ] }";
                }

                //save the translations for the recv buf
                for (i=0; i<64; i++) {
                    out << " { " << recvbufv2p[i].size() << " [";
                    for (it1 = recvbufv2p[i].begin(); it1 != recvbufv2p[i].end(); it1++) {
                        out << " ( " << it1->first << " " << it1->second << " )";
                    }
                    out << " ] }";
                }
            } else {
                DBG_Assert(false, ( << "GZipFlexpoints not yet supported for RMC" ));
            }
        }

        void loadState(std::string const &aDirName) {
            //if (cfg.SingleRMCFlexpoint) DBG_Assert(false, ( << "Continuing trace simulation in a Single RMC Flexpoint not yet supported"));
            //This would require having a special case for loading the RMCs, as in Timing. Trace simulation cannot continue with the same abstract idea of a single RMC;
            //a specific layout must be picked, and RMC state should be loaded accordingly
            if (NORMC) return;
            std::string fname( aDirName);
            loadFname = fname;
            /*
               fname += "/" + statName();
               if (cfg.GZipFlexpoints) {
               fname += ".gz";
               }

               std::ios_base::openmode omode = std::ios::in;
               if (!cfg.TextFlexpoints) {
               omode |= std::ios::binary;
               }

               std::ifstream ifs(fname.c_str(), omode);
               if (! ifs.good()) {
               DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to cold RMC state " )  );
               } else {
               boost::iostreams::filtering_stream<boost::iostreams::input> in;
               if (cfg.GZipFlexpoints) {
               in.push(boost::iostreams::gzip_decompressor());
               }
               in.push(ifs);

               if (cfg.TextFlexpoints) {
               DBG_( Tmp, ( << "Loading RMC state" ));
               char paren;
               uint64_t key, value, size, size2;
               uint32_t value32;
               uint8_t value8;
               uint64_t i, j;

               in >> paren >> value;
               DBG_Assert(paren == '{', ( << "Error in loading state" ));
               DBG_Assert(value == PAGE_BITS, ( << "Error in loading state - different page size" ));
               in >> value;
               DBG_Assert(value == PT_I, ( << "Error in loading state" ));
               in >> value;
               DBG_Assert(value == PT_J, ( << "Error in loading state" ));
               in >> value >> paren;
               DBG_Assert(value == PT_K, ( << "Error in loading state" ));

            //Load RMC_TLB
            DBG_( Tmp, ( << "Now loading RMC TLB..." ));
            in >> paren >> value;
            DBG_Assert(value == TLBsize, ( << "The size of the TLB size in previous flexpoint is different than requested (" << value << " versus " << TLBsize << ")" ));
            DBG_Assert(paren == '{', ( << "Error in loading state" ));

            in >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (i = 0; i < size; i++) {
            in >> paren >> key >> value >> paren;
            DBG_Assert(paren == ')', ( << "Error in loading state" ));
            RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(key, value));
            }

            in >> paren >> paren;

            //PTv2p translations
            for (i=0; i<64; i++) {
            in >> paren >> value >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            PTbaseV[i] = value;

            for (j=0; i<size; j++) {
            in >> paren >> key >> value >> paren;
            DBG_Assert(paren == ')', ( << "Error in loading state" ));
            PTv2p[i][key] = value;
            }
            in >> paren >> paren;
            }

            //buffers
            for (i=0; i<64; i++) {
            in >> paren >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (j=0; j<size; j++) {
                in >> paren >> key >> value >> paren;
                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                bufferv2p[i][key] = value;
            }
            in >> paren >> paren;
        }

        //context
        for (i=0; i<64; i++) {
            in >> paren >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (j=0; j<size; j++) {
                in >> paren >> key >> value >> paren;
                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                contextv2p[i][key] = value;
            }
            in >> paren >> paren;
        }

        //contextMap
        for (i=0; i<64; i++) {
            in >> paren >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (j=0; j<size; j++) {
                in >> paren >> key >> value >> paren;
                DBG_Assert(paren == ')', ( << "Error in loading state" ));
                contextMap[i][key] = value;
            }
            in >> paren >> paren;
        }
        DBG_Assert(paren == '}', ( << "Error in loading state" ));

        RMCPacket aPacket;
        uint64_t offset, bufaddr, qid, pkt_index;
        uint8_t context, tid, src_nid, dst_nid;
        RemoteOp op = RREAD;	//bogus initialization
        aBlock payload;

        in >> paren >> size2; 		//outQueue.size()
        DBG_Assert(paren == '{', ( << "Error in loading state" ));
        for (i=0; i<size2; i++) {
            in >> paren >> qid >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (j=0; j<size; j++) {
                in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                DBG_Assert(paren == '(', ( << "Error in loading state" ));
                switch (value) {
                    case 0:
                        op = RREAD;
                        break;
                    case 1:
                        op = RWRITE;
                        break;
                    case 2:
                        op = RRREPLY;
                        break;
                    case 3:
                        op = RWREPLY;
                        break;
                    default:
                        DBG_Assert(false);
                }
                if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                    for (uint32_t k = 0; k<64; k++) {
                        in >> payload.data[k];
                    }
                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                } else {
                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                }
                outQueue[qid].push_back(aPacket);
                in >> paren;
                DBG_Assert(paren == ')', ( << "Error in loading state" ));

            }
            in >> paren >> paren;
            DBG_Assert(paren == '}', ( << "Error in loading state" ));
        }

        in >> paren >> paren >> size2; 		//inQueue.size()
        DBG_Assert(paren == '{', ( << "Error in loading state" ));
        for (i=0; i<size2; i++) {
            in >> paren >> qid >> size >> paren;
            DBG_Assert(paren == '[', ( << "Error in loading state" ));
            for (j=0; j<size; j++) {
                in >> paren >> tid >> src_nid >> dst_nid >> context >> offset >> bufaddr >> pkt_index >> value;
                DBG_Assert(paren == '(', ( << "Error in loading state" ));
                switch (value) {
                    case 0:
                        op = RREAD;
                        break;
                    case 1:
                        op = RWRITE;
                        break;
                    case 2:
                        op = RRREPLY;
                        break;
                    case 3:
                        op = RWREPLY;
                        break;
                    default:
                        DBG_Assert(false);
                }
                if (op == RWRITE || op == RRREPLY) {	//payload has useful info
                    for (uint32_t k = 0; k<64; k++) {
                        in >> payload.data[k];
                    }
                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, payload, bufaddr, pkt_index, 64);
                } else {
                    aPacket = RMCPacket(tid, src_nid, dst_nid, context, offset, op, bufaddr, pkt_index, 64);
                }
                inQueue[qid].push_back(aPacket);
                in >> paren;
                DBG_Assert(paren == ')', ( << "Error in loading state" ));

            }
            in >> paren >> paren;
            DBG_Assert(paren == '}', ( << "Error in loading state" ));
        }
        in >> paren;
        DBG_Assert(paren == '}', ( << "Error in loading state" ));

        in >> value32;
        DBG_Assert(value32 == RMCid, ( << "Error in loading state" ));

        in >> value;
        if (value == 0)
            status = INIT;
        else
            status = READY;

        in >> value;
        uniqueTID = (uint8_t)value;
        in >> value;
        wqSize = value;
        in >> value;
        cqSize = value;

        //Now load the SRAM structures
        //QP table
        in >> paren >> value;
        QP_table_entry_t aQPEntry;
        int idx;
        uint8_t WQ_SR, CQ_SR;
        uint64_t lbuf_size, ctx_size, lbuf_base;

        for (i=0; i< value; i++) {
            in >> paren >> idx >> aQPEntry.WQ_base >> aQPEntry.WQ_tail >> WQ_SR >> aQPEntry.CQ_base
                >> aQPEntry.CQ_head >> CQ_SR >> lbuf_base >> lbuf_size >> ctx_size >> paren;
            aQPEntry.WQ_SR = WQ_SR;
            aQPEntry.CQ_SR = CQ_SR;
            aQPEntry.lbuf_base = lbuf_base;
            aQPEntry.lbuf_size = lbuf_size;
            aQPEntry.ctx_size = ctx_size;
            QP_table[idx] = aQPEntry;
        }
        in >> paren;

        //ITT table
        in >> paren >> value;
        ITT_table_entry_t anITTEntry;
        for (i=0; i< value; i++) {
            in >> paren >> idx >> anITTEntry.counter >> anITTEntry.QP_id >> anITTEntry.WQ_index >> anITTEntry.lbuf_addr >> paren;
            ITT_table[idx] = anITTEntry;
        }
        in >> paren;

        DBG_( Tmp, ( << "Loading RMC state finished!" ));
        } else {
            DBG_Assert(false, ( << "GZipFlexpoints not yet supported for RMC" ));
        }
        }
        */
        }

        void initialize(void) {
            NORMC = true; isRGP = false; isRRPP = false;
            CORES = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
            MACHINES = cfg.MachineCount;
            CORES_PER_RMC = (uint32_t)floor(sqrt(CORES/MACHINES));
            CORES_PER_MACHINE = CORES / MACHINES;
            DBG_Assert(CORES % MACHINES == 0);
            DBG_Assert(CORES_PER_RMC*CORES_PER_RMC*MACHINES == CORES && CORES <= 64);
            uint64_t i;
            RMCid = RMCcount++;
            if ((cfg.SingleRMCFlexpoint && RMCid >= MACHINES) || (!cfg.SingleRMCFlexpoint && RMCid < CORES && !cfg.SharedL1)) return;	//The RMCs with ids > CORES are the ones with their own L1 cache

            /* We initialize one RMC (RGP/RCP + RRPP per tile). Below, we determine which RMC pipelines are
             * really activated, based on the configuration file.
             * We assume a mesh organization and the default configuration is one RMC per row, on the first column.
             * For instance, in a 4x4 mesh:
             *  0  1  2  3
             *  4  5  6  7
             *  8  9 10 11
             * 12 13 14 15
             * only tiles 0,4,8,12 will have an RMC, thus the RMCs with IDs 16, 20, 24, 28 (RMCid > width && RMCid % width == 0)
             */
            if (cfg.RRPPPerTile) DBG_Assert(false, ( << "One RRPP per tile not supported yet"));
            if (cfg.SingleRMCFlexpoint) DBG_(Crit, ( << "Running in Single RMC Flexpoint mode. All RMC state is going to be saved in a single file per machine. "
                        << "WARNING! This practically means that there is a single RMC per machine (RMC-i for machine i) in this trace simulation."));
            //Decide if tile features an RGP
            if (!cfg.RGPPerTile && !cfg.SingleRMCFlexpoint) {
                if (RMCid % CORES_PER_RMC == 0) isRGP = true;
            } else {
                isRGP = true;
            }
            //Decide if tile features an RRPP
            if (!cfg.RRPPWithMCs && RMCid % CORES_PER_RMC == 0) isRRPP = true;
            else if (cfg.RRPPWithMCs && RMCid % CORES_PER_RMC == CORES_PER_RMC - 1) isRRPP = true;

            if (!isRGP && !isRRPP) return;	//This tile has no RMC pipeline

            NORMC = false;
            DBG_( Tmp, ( << "RMC "<< RMCid<< " Initialization... "));
            if (cfg.SingleRMCFlexpoint) {
                DBG_( Tmp,( << "\tRunning in single RMC flexpoint mode. This is the only RMC for machine " << RMCid));
            } else if (isRGP && isRRPP) {
                DBG_( Tmp,( << "\tThis RMC features both an RGP/RCP and an RRPP "));
            } else if (isRGP) {
                DBG_( Tmp,( << "\tThis RMC features only an RGP/RCP"));
            } else {
                DBG_( Tmp,( << "\tThis RMC features only an RRPP"));
            }

            READY_COUNT_TARGET = cfg.ReadyCountTarget;
            DBG_( Tmp, Addr(theLoadMessage.address()) ( << "\t READY_COUNT_TARGET is "<< READY_COUNT_TARGET));
            if (NO_PAGE_WALKS) DBG_( Crit, ( << "WARNING: RMC page walks are disabled!"));

            for (i=0; i<CORES ; i++) {
                theCPUs[i] = API::QEMU_get_cpu_by_index(i);
            }
            status = INIT;
            uniqueTID = 0;

            theLoadMessage.type() = MemoryMessage::LoadReq;
            theLoadMessage.address() = PhysicalMemoryAddress(0);

            theStoreMessage.type() = MemoryMessage::StoreReq;
            theStoreMessage.address() = PhysicalMemoryAddress(0);

            theCycleCount = 0;
            transactionCounter = 0;
            prevPage = 0;

            //bufsRemaining = 0;

            TLBsize = cfg.TLBEntries;
            DBG_(Tmp, ( << "Setting TLBSize to " << TLBsize));

            //for Page Tables
            for (i=0 ; i<64; i++)
                PTbaseV[i] = 0;
            uint64_t theBase = 0;
            PObits = 64 - PT_I - PT_J - PT_K;

            for (i=0; i<PT_I; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            iMask = theBase << (PObits+PT_J+PT_K);

            theBase = 0;
            for (i=0; i<PT_J; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            jMask = theBase << (PObits+PT_K);

            theBase = 0;
            for (i=0; i<PT_K; i++) {
                theBase = theBase << 1;
                theBase = theBase | 1;
            }
            kMask = theBase << PObits;
            DBG_( Tmp, Addr(theLoadMessage.address()) ( << "...done!"));
        }

        bool isQuiesced() const {
            return true;
        }

        void finalize(void) {}

        // request input port
        FLEXUS_PORT_ALWAYS_AVAILABLE(RequestIn);
        void push( interface::RequestIn const &, RMCEntry & anEntry) {
            DBG_( Iface, ( << "Received a breakpoint!"));
            if (NORMC) {
                DBG_( Iface, ( << "...but this is not really an RMC. Ignoring breakpoint..."));
                return;
            }
            uint64_t address, size, type, base_address = 0;
            uint32_t cpu_id, machine_id;

            type = anEntry.getType();
            address = anEntry.getAddress();
            size = anEntry.getSize();
            cpu_id = anEntry.getCPUid();
            machine_id = cpu_id / CORES_PER_MACHINE;
            theCPU = API::QEMU_get_cpu_by_index(cpu_id);

            /* Need to identify which RMC pipeline this control message is destined to
             * RRPPs only register CONTEXT and CONTEXTMAP message types
             * The rest of the messages are intended for RGPs

             * WARNING: For now, all RRPPs of the same machine will use same context, registered as contextMap[machine_id]
             * May want to change that!
             */
            if (!cfg.SingleRMCFlexpoint) {	//if SingleRMCFlexpoint, only one RMC will handle all messages to register all structures
                /*if ((type == CONTEXT || type == CONTEXTMAP) &&
                  (!isRRPP
                  || cpu_id != CONTEXT_CPU)) {
                  if (type == CONTEXTMAP) DBG_( Tmp, ( << "Context registration of cpu-" << cpu_id << " is ignored. All RMCs will use the context registered by cpu-" << CONTEXT_CPU));
                  return;
                  }*/
                /* All RMC pipelines need to register the page tables, to use in case a page walk is needed.
                 * To save memory, all pipelines use the same PT, which is registered by a single core, the CONTEXT_CPU
                 */
                /*if (type == PTENTRY && cpu_id != CONTEXT_CPU) {
                  return;
                  }*/
                /* Now determine which RMC pipeline this message is destined to
                 * for the cases other than CONTEXT and CONTEXTMAP, which were handled above
                 * Assume that every RMC is statically assigned to the cores in the same ROW
                 */
                if ((type == CONTEXT || type == CONTEXTMAP || (type >= SEND_BUF && type <= MSG_BUF_ENTRY_COUNT))
                        && !isRRPP)
                    return;
                else if ( type != CONTEXT && type != CONTEXTMAP && (type < SEND_BUF || type > MSG_BUF_ENTRY_COUNT)) {
                    if (!isRGP) return;		//Easy case first
                    if (cfg.RGPPerTile && cpu_id != RMCid) return;						//if one RGP per tile
                    if (!cfg.RGPPerTile && (cpu_id / CORES_PER_RMC != (RMCid - CORES) / CORES_PER_RMC)) return;		//if one RGP per row, verify if RGP and cpu are on same row
                }
            } else {	//Pick RMC-i to do all the registrations for machine i
                if (RMCid != machine_id ) return;
            }

            //For now, assume one QP per core
            QP_table_entry_t aQPTableEntry;
            bool QP_update = false;
            if (type == WQUEUE || type == CQUEUE || type == BUFFER_SIZE || type == CONTEXT_SIZE || type == BUFFER) {
                QP_table_iter = QP_table.find(cpu_id);
                if (QP_table_iter != QP_table.end()) {
                    aQPTableEntry = QP_table_iter->second;
                } else {
                    aQPTableEntry.lbuf_base = 0;
                }
            }

            messaging_struct_t amessagingEntry;
            amessagingEntry.SR = 1; amessagingEntry.ctx_id = size; amessagingEntry.send_buf_addr = 0; amessagingEntry.recv_buf_addr = 0;
            amessagingEntry.num_nodes = 0; amessagingEntry.msg_buf_entry_count = 0; amessagingEntry.msg_entry_size = 0; amessagingEntry.sharedCQ_size = 0;
            bool messaging_update = false;
            if (type >= SEND_BUF && type <= MSG_BUF_ENTRY_COUNT) {
                messaging_table_iter = messaging_table.find(size);
                if (messaging_table_iter != messaging_table.end()) {
                    amessagingEntry = messaging_table_iter->second;
                } else {
                    messaging_update = true;
                    DBG_(Tmp, (<<"Creating messaging entry for ctx " << size));
                }
            }

            //Do the V->P translation here
            if (type!= WQENTRYDONE &&
                    type!= NEWWQENTRY &&
                    type != NEWWQENTRY_START &&
                    type != ALL_SET &&
                    type != SIM_BREAK &&
                    type != RMC_DEBUG_BP &&
                    type != BENCHMARK_END &&
                    type != NETPIPE_START &&
                    type != 42 &&
                    type != 29 &&
                    type != 30 &&
                    type != BUFFER_SIZE &&
                    type != CONTEXT_SIZE &&
                    type != MSG_BUF_ENTRY_SIZE &&
                    type != NUM_NODES &&
                    type != MSG_BUF_ENTRY_COUNT &&
                    type != SENDENQUEUED &&
                    type != LOCK_SPINNING &&
                    type != NETPIPE_START &&
                    type != NETPIPE_END 
               ) {
                base_address = address & PAGE_BITS;
                tempTranslation.clear();
                tempTranslation[base_address] = API::QEMU_logical_to_physical(theCPU, API::QEMU_DI_Data, base_address);
                API::QEMU_clear_exception();

                if (type == BUFFER) {
                    if (tempTranslation[base_address] == 0) {
                        DBG_( Tmp, ( << "RMC got an Entry of type "<< type <<" for V address "<< address <<" with size "<<size << " but translation FAILED"));
                        return;
                    }
                    if (bufferv2p[cpu_id].find(base_address) != bufferv2p[cpu_id].end())
                        return;
                }

                if (type != PTENTRY && type != CONTEXT) DBG_( Tmp, ( << "RMC-" << RMCid << " got an Entry of type "<< type <<" for V address 0x"<< std::hex << address
                            <<" with size " << std::dec << size));
                if (type != PTENTRY && type != CONTEXT) DBG_( Tmp, ( << "Translation v->p of "<< base_address <<" (0x" << std::hex<<address << ") is 0x"<< std::hex<<tempTranslation[base_address]));

                if (type == WQHEAD ||
                        type == WQTAIL ||
                        type == WQUEUE ||
                        type == CQUEUE ||
                        type == CQHEAD ||
                        type == CQTAIL ||
                        type == CONTEXTMAP) {
                    DBG_Assert(tempTranslation[base_address] != 0, ( << "V to P translation FAILED!"));
                }
                DBG_Assert(base_address != 0);
            }	//translation done

            switch (type) {
                case WQUEUE:
                    wqSize = size;

                    if (wqSize * sizeof(wq_entry_t) > PAGE_SIZE)
                        DBG_Assert( false, ( << "WARNING: WQ spans more than a single page" ));

                    DBG_( Tmp, ( << "Entry type: WQUEUE\n\tWQ has " << wqSize << " entries" ));
                    DBG_( Tmp, ( << "WQ starts at pAddr 0x" << std::hex << tempTranslation[base_address] + address - base_address
                                << ", vAddr 0x"<< address));

                    QP_update = true;
                    aQPTableEntry.WQ_base = tempTranslation[base_address] + address - base_address;
                    aQPTableEntry.WQ_tail = 0;
                    aQPTableEntry.WQ_SR = 1;		//expect 1 for valid entry
                    break;
                case CQUEUE:
                    cqSize = size;

                    if (cqSize * sizeof(cq_entry_t) > PAGE_SIZE)
                        DBG_Assert( false, ( << "WARNING: CQ spans more than a single page" ));

                    DBG_( Tmp, ( << "Entry type: CQUEUE" ));
                    DBG_( Tmp, ( << "CQ starts at pAddr 0x"<< std::hex << tempTranslation[base_address] + address - base_address
                                << ", vAddr 0x"<<  address));

                    QP_update = true;
                    aQPTableEntry.CQ_base = tempTranslation[base_address] + address - base_address;
                    aQPTableEntry.CQ_head = 0;
                    aQPTableEntry.CQ_SR = 1;		//expect 1 for valid entry
                    break;
                case CONTEXT_SIZE: {
                                       DBG_( Tmp, ( << "Entry type: CONTEXT_SIZE by core " << cpu_id << " (machine " << machine_id << ") for context " << address ));
                                       std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter = contextMap[machine_id].find(address);	//WARNING:Only register context by machine_id
                                       if (cntxtIter != contextMap[machine_id].end()) {
                                           DBG_(Tmp, ( << "WARNING: Trying to register size for unregistered context " << address << ". That's ok, unless the program intended to use the flexi debug mode."));
                                       } else {
                                           contextMap[machine_id][address].second = size;
                                           DBG_( Tmp, ( << "Registering size " << size << " for context " << address));
                                       }
                                       break;
                                   }
                case BUFFER_SIZE:
                                   DBG_( Tmp, ( << "Entry type: BUFFER_SIZE = " << size));
                                   QP_update = true;
                                   aQPTableEntry.lbuf_size = size;
                                   break;
                case WQHEAD:
                                   DBG_( Tmp, ( << "Entry type: WQHEAD" ));
                                   DBG_( Tmp, ( << " Not used anymore (deprecated)"));
                                   break;
                case WQTAIL:
                                   DBG_( Tmp, ( << "Entry type: WQTAIL" ));
                                   DBG_( Tmp, ( << " Not used anymore (deprecated)"));
                                   break;
                case CQHEAD:
                                   DBG_( Tmp, ( << "Entry type: CQHEAD" ));
                                   DBG_( Tmp, ( << " Not used anymore (deprecated)"));
                                   break;
                case CQTAIL:
                                   DBG_( Tmp, ( << "Entry type: CQTAIL" ));
                                   DBG_( Tmp, ( << " Not used anymore (deprecated)"));
                                   break;
                case BUFFER:
                                   DBG_( Tmp, ( << "Entry type: BUFFER" ));
                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                       DBG_( Tmp, ( << "Registering translation for Buffer. V: "<< std::hex<< iter->first
                                                   << ", P: 0x"<< std::hex<<iter->second));
                                       bufferv2p[cpu_id][iter->first] = iter->second;
                                   }
                                   if (aQPTableEntry.lbuf_base == 0) {
                                       QP_update = true;
                                       aQPTableEntry.lbuf_base = address;
                                       DBG_( Tmp, ( << "Registering lbuf base address V: "<< std::hex<< address << " for CPU-" << std::dec << cpu_id));
                                   }
                                   break;
                case PTENTRY:
                                   //DBG_( Tmp, ( << "Entry type: PTENTRY - Storing v2p translation: 0x"
                                   //				<< std::hex << tempTranslation.begin()->first
                                   //				<< "->0x" << std::hex << tempTranslation.begin()->second));
                                   PTv2p[cpu_id][tempTranslation.begin()->first] = tempTranslation.begin()->second;
                                   if (PTbaseV[cpu_id] == 0) PTbaseV[cpu_id] = tempTranslation.begin()->first;	//Base of PT must be the first PTENTRY that comes
                                   break;
                case NEWWQENTRY:
                                   DBG_(Tmp, ( << "Application enqueued a new WQ entry - opcount_issued: " << size));
                                   break;
                case SENDENQUEUED:
                                   DBG_(Tmp, ( <<" New SEND request enqueued at index " << address << " by CPU " << cpu_id));
                                   break;
                case NEWWQENTRY_START:
                                   DBG_(Tmp, ( << "Application started enqueuing a new WQ entry with opcount: " << size));
                                   break;
                case WQENTRYDONE:
                                   DBG_(Tmp, ( << "WQ entry done - RMC successfully completed an operation - opcount_completed: " << size));
                                   break;
                case NETPIPE_START:
                                   DBG_(Tmp, ( << "Netpipe action started - count: " << size));
                                   break;
                case NETPIPE_END:
                                   DBG_(Tmp, ( << "Netpipe action completed - count: " << size));
                                   break;
                case LOCK_SPINNING:

                                   break;
                case CONTEXT:	{//for context, the "size" field means context id
                                    DBG_( VVerb, ( << "Entry type: CONTEXT REGISTRATION by core " << cpu_id << " (machine " << machine_id << ")" ));
                                    for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                        DBG_( VVerb, ( << "Registering translation for context. V: 0x"<< std::hex << iter->first
                                                    << ", P: 0x"<< std::hex<<iter->second));
                                        contextv2p[machine_id][iter->first] = iter->second;	//WARNING:Only register context by machine_id
                                    }
                                    break; }
                case CONTEXTMAP: {
                                     DBG_( Tmp, ( << "Entry type: CONTEXT MAPPING by core " << cpu_id << " (machine " << machine_id << ")" ));
                                     std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter = contextMap[machine_id].find(size);	//WARNING:Only register context by machine_id
                                     if (cntxtIter == contextMap[machine_id].end()) {
                                         contextMap[machine_id][size].first = tempTranslation.begin()->first;
                                         DBG_( Tmp, ( << "Establishing context mapping: " << size << " to vAddr " << tempTranslation.begin()->first));
                                     }
                                     break; }
                case SEND_BUF: {
                                   DBG_( Tmp, ( << "Entry type: SEND_BUF" ));
                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                       DBG_( Tmp, ( << "Registering translation for send buf of ctx " << size << ". V: 0x"<< std::hex << iter->first
                                                   << ", P: 0x"<< std::hex<<iter->second));
                                       sendbufv2p[machine_id][iter->first] = iter->second;	//WARNING:Only register context by machine_id
                                   }
                                   if (amessagingEntry.send_buf_addr == 0) {
                                       messaging_update = true;
                                       amessagingEntry.send_buf_addr = address;
                                       DBG_( Tmp, ( << "Registering send buffer base address V: "<< std::hex<< address << " for context " << std::dec << size));
                                   }
                                   break;
                               }
                case RECV_BUF: {
                                   DBG_( Tmp, ( << "Entry type: RECV_BUF" ));
                                   for (iter = tempTranslation.begin(); iter != tempTranslation.end(); iter++) {
                                       DBG_( Tmp, ( << "Registering translation for recv buf of ctx " << size << ". V: 0x"<< std::hex << iter->first
                                                   << ", P: 0x"<< std::hex<<iter->second));
                                       recvbufv2p[machine_id][iter->first] = iter->second;	//WARNING:Only register context by machine_id
                                   }
                                   if (amessagingEntry.recv_buf_addr == 0) {
                                       messaging_update = true;
                                       amessagingEntry.recv_buf_addr = address;
                                       DBG_( Tmp, ( << "Registering recv buffer base address V: "<< std::hex<< address << " for context " << std::dec << size));
                                   }
                                   break;
                               }
                case MSG_BUF_ENTRY_SIZE: {
                                             messaging_update = true;
                                             amessagingEntry.msg_entry_size = address;
                                             DBG_( Tmp, ( << "Entry type: MSG_BUF_ENTRY_SIZE for ctx " << size << " (for messaging). Value: " << address ));
                                             break;
                                         }
                case NUM_NODES: {
                                    messaging_update = true;
                                    amessagingEntry.num_nodes = address;
                                    DBG_( Tmp, ( << "Entry type: NUM_NODES for ctx " << size << " (for messaging). Value: " << address ));
                                    break;
                                }
                case MSG_BUF_ENTRY_COUNT: {
                                              messaging_update = true;
                                              amessagingEntry.msg_buf_entry_count = address;
                                              DBG_( Tmp, ( << "Entry type: MSG_BUF_ENTRY_SIZE for ctx " << size << " (for messaging). Value: " << address ));
                                              break;
                                          }
                case ALL_SET:
                                          ReadyCount++;
                                          DBG_(Tmp, ( <<" RMC " << RMCid <<" is ready to start servicing on behalf of cpu " << cpu_id
                                                      << ". ReadyCount: " << ReadyCount << ", ReadyCount Target: " << READY_COUNT_TARGET));
                                          //if (cfg.CopyEmuRMC && ReadyCount == READY_COUNT_TARGET - 1) {
                                          if (cfg.CopyEmuRMC && ReadyCount == READY_COUNT_TARGET) {

                                              status = READY;
                                              //Simics::API::SIM_break_simulation("Ready for checkpointing! Type: \nflexus.save-state ckpts/trace2");
                                              DBG_Assert(false,(<<"TEMPORARY FAILURE, REPLACING SIMICS API CALLS."));
                                          } else if (!cfg.CopyEmuRMC && ReadyCount == READY_COUNT_TARGET) {
                                              status = READY;
                                              DBG_Assert(false,(<<"TEMPORARY FAILURE, REPLACING SIMICS API CALLS."));
                                              //Simics::API::SIM_break_simulation("Ready for checkpointing!");
                                          }
                                          break;
                case SIM_BREAK:
                                          DBG_Assert(false,(<<"TEMPORARY FAILURE, REPLACING SIMICS API CALLS."));
                                          //Simics::API::SIM_break_simulation("Reach SIM_BREAK breakpoint.");
                                          break;
                case RMC_DEBUG_BP:
                                          break;
                case BENCHMARK_END:
                                          DBG_(Tmp, ( <<" Got a PAGERANK_END breakpoint "));
                                          break;
                case 29:
                                          DBG_(Iface, ( << " Magic break 29: CPU " << cpu_id << " - measurement breakpoint with args: " << address << ", " << size << "(Cycle count: "
                                                      << Flexus::Core::theFlexus->cycleCount() << ").\n\t\t\tWriting " << (address + size) << "to register l0.") );
                                          //Simics::API::SIM_write_register( theCPU , Simics::API::SIM_get_register_number ( theCPU , "l0") , address+size );
                                          break;
                case 30:
                                          DBG_(Iface, ( << " Magic break 30: CPU " << cpu_id << " - measurement breakpoint with args: " << address << ", " << size << "(Cycle count: "
                                                      << Flexus::Core::theFlexus->cycleCount() << ")\n" ) );
                                          /*switch(size) // ustiugov: here size stands for transfer's success (0, 2) or failure (1, 3) registered by the SW
                                            {
                                            case 0:
                                            case 1:
                                            DBG_(Tmp, ( << " Magic break 30: CPU " << cpu_id << " - reader measurement breakpoint with args: " << address << ", " << size ) );
                                            break;
                                            case 2:
                                            case 3:
                                            DBG_(Tmp, ( << " Magic break 30: CPU " << cpu_id << " - writer measurement breakpoint with args: " << address << ", " << size ) );
                                            break;
                                            default:
                                            DBG_Assert( false, ( << "reader or writer? what a heck?" ));
                                            break;
                                            }*/
                                          break;
                case 42:
                                          //BOGUS
                                          break;
                case MEASUREMENT: // ustiugov: just debug breakpoint
                                          break;
                case OBJECT_WRITE:
                                          break;
                case CS_START:
                                          break;
                default:
                                          DBG_Assert( false, ( << "Unknown call type received by RMC: " << type) );
                                          //DBG_(Tmp, ( <<"Unknown call type received by RMC: " << type));
                                          break;

            }
            if (QP_update) {
                DBG_(Tmp, ( <<" RMC " << RMCid <<" inserting QP entry: QP_table[" << cpu_id << "]"));
                QP_table[cpu_id] = aQPTableEntry;
            }
            if (messaging_update) {
                DBG_(Tmp, ( <<" RMC " << RMCid <<" inserting messaging entry for context " << size << ": messaging_table[" << size << "]"));
                messaging_table[size] = amessagingEntry;
            }
            }

            FLEXUS_PORT_ALWAYS_AVAILABLE(MsgIn);
            void push( interface::MsgIn const &, RMCPacket & aPacket) {
                inQueue[aPacket.src_nid].push_back(aPacket);
            }

            void drive( interface::PollL1Cache const & ) {
                if (NORMC) return;
                if (isRRPP) driveRRPP();

                if (!isRGP || status != READY) return;

                return;			//Functionality of latest RMC protocol not implemented in Trace yet. Code hasn't been tested
                driveRCP();

                QP_table_iter = QP_table.begin();
                while (QP_table_iter != QP_table.end()) {	//check all QPs for new remote accesses
                    theCPU = API::QEMU_get_cpu_by_index(QP_table_iter->first);
                    driveRGP(QP_table_iter);
                    QP_table_iter++;
                }
                sendMsgs();
            }

            void driveRGP(std::tr1::unordered_map<uint16_t, QP_table_entry_t>::iterator theIter) {
                QP_table_entry_t theQPEntry = theIter->second;
                int QP_id = theIter->first;
                uint64_t WQEntry1, WQEntry2;
                wq_entry_t theWQEntry;
                std::tr1::unordered_map<uint64_t, uint64_t>::iterator tempAddr;
                uint64_t WQVAddr, WQPAddr, bufferPAddr, bufferVAddr, CQPAddr, CQVAddr;

                //Read WQ entry
#ifndef WQ_COMPACTION
                DBG_Assert(false, ( << "RMC coded to support WQ with compaction. Need to modify code for non-compacted WQ entries"));
#endif
                theLoadMessage.address() = PhysicalMemoryAddress(theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t));
                DBG_( RMC_DEBUG, Addr(theLoadMessage.address()) ( << "Sending read request for CPU " << QP_id << " address 0x" << std::hex <<
                            theLoadMessage.address() << " to L1d cache " << (RMCid)));
                theLoadMessage.type() = MemoryMessage::LoadReq;
                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                sendMsgToMem(&theLoadMessage, false, false);

                // Msutherl: QFlex port to new API.
                // - reads directly into uint64_t (guaranteed to hold 8B)
                uint8_t tmp_arr[sizeof(uint64_t)]; 
                API::QEMU_read_phys_memory(tmp_arr,theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t), 8);
                std::memcpy(&WQEntry1,tmp_arr, sizeof WQEntry1);
                //WQEntry1 = (uint64_t)API::QEMU_read_phys_memory(theCPU, theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t), 8);

                theLoadMessage.address() = PhysicalMemoryAddress(theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t)+8);
                DBG_( RMC_DEBUG, Addr(theLoadMessage.address()) ( << "Sending read request for CPU " << QP_id << " address " <<
                            theLoadMessage.address() << " to L1d cache " << (RMCid)));
                theLoadMessage.type() = MemoryMessage::LoadReq;
                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                sendMsgToMem(&theLoadMessage, false, false);

                // Msutherl: QFlex port to new API.
                // - reads directly into uint64_t (guaranteed to hold 8B)
                API::QEMU_read_phys_memory(tmp_arr,theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t)+8, 8);
                std::memcpy(&WQEntry2,tmp_arr, sizeof WQEntry2);
                //WQEntry2 = (uint64_t)API::QEMU_read_phys_memory(theCPU, theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t)+8, 8);
                theWQEntry.SR = (WQEntry1 >> 61) & 1;

                DBG_(Verb, ( << "RMC- " << RMCid << ": Trying QP_table[" << QP_id << "]. SR in QP_table entry is " << std::dec << (int)theQPEntry.WQ_SR
                            << ", while SR in WQ entry is "	<< std::dec << (int)theWQEntry.SR));

                //if valid WQ entry
                while (theWQEntry.SR == theQPEntry.WQ_SR) {
                    //WARNING! This part may need to change
                    theWQEntry.op = (WQEntry1 >> 62) & 0x03;
                    theWQEntry.cid = (WQEntry1 >> 57) & 0x0F;
                    theWQEntry.nid = (WQEntry1 >> 48) & 0x1FF;
                    theWQEntry.buf_addr = WQEntry1 & 0xFFFFFFFFFFFF;
                    theWQEntry.offset = (WQEntry2 >> 24) & 	0xFFFFFFFFFF;
                    theWQEntry.length = WQEntry2 & 0xFFFFFF;

                    DBG_( Tmp, Addr(theLoadMessage.address()) ( << "New valid WQ entry found!\n\top field value is: " << (int)theWQEntry.op
                                << "\n\tnid field value is: " << (int)theWQEntry.nid
                                << "\n\tcid field value is: " << (int)theWQEntry.cid
                                << "\n\tbuf_address field value is: " << std::hex << (uint64_t)theWQEntry.buf_addr
                                << "\n\toffset field value is: " << std::dec << (uint64_t)theWQEntry.offset
                                << "\n\tlength field value is: " << (uint64_t)theWQEntry.length));

                    //create packets and put in appropriate outQueue - unroll if needed
                    uint32_t dest = theWQEntry.nid+100;					//100 is a bogus value (we model single node)

                    RemoteOp theOp;
                    if (theWQEntry.op == RMC_WRITE) {
                        theOp = RWRITE;
                        DBG_(RMC_DEBUG, (<< "This is an RWRITE"));
                    }
                    else theOp = RREAD;
                    //TODO: Atomic ops

                    //Create ITT entry
                    if (ITT_table.find(uniqueTID) != ITT_table.end()) {
                        DBG_(Tmp, (<< "No free tid left (ITT_table is full). Stalling "));
                        break;
                    }
                    //DBG_Assert(ITT_table.find(uniqueTID) == ITT_table.end(), ( << "No free tid! (Found tid "
                    //							<< std::dec << (int)uniqueTID << " in the ITT_table. this shouldn't have happened))"));
                    ITT_table_entry_t theITTEntry;
                    theITTEntry.counter = theWQEntry.length;
                    theITTEntry.QP_id = QP_id;
                    theITTEntry.WQ_index = theQPEntry.WQ_tail;
                    theITTEntry.lbuf_addr = (uint64_t)theWQEntry.buf_addr;
                    ITT_table[uniqueTID] = theITTEntry;

                    DBG_(Verb, (<< "Initiating remote operation with tid = " << std::dec << (int)uniqueTID));

                    packet = RMCPacket(uniqueTID, RMCid, dest, theWQEntry.cid, (uint64_t)theWQEntry.offset,
                            theOp, (uint64_t)theWQEntry.buf_addr, (uint64_t)0, 64);

                    uint32_t packet_counter = 0;
                    while (packet_counter < theWQEntry.length) {		//packets only contain a payload of 64B
                        packet.offset = theWQEntry.offset + packet_counter*64;
                        if (theOp == RWRITE) {		//need to attach payload
                            DBG_(RMC_DEBUG, (<< "Attach payload to the packet. Packet_counter: " << packet_counter));
                            bufferVAddr = theWQEntry.buf_addr + packet_counter*64;
                            tempAddr = bufferv2p[QP_id].find(bufferVAddr & PAGE_BITS);
                            DBG_Assert( tempAddr != bufferv2p[QP_id].end(), ( << "Should have found the translation") );
                            bufferPAddr = tempAddr->second + bufferVAddr - (bufferVAddr & PAGE_BITS);

                            DBG_( RMC_DEBUG, Addr(theLoadMessage.address()) ( << "Will now read " << theWQEntry.length
                                        << " bytes of data from the buffer with Vaddress 0x" << std::hex << bufferVAddr
                                        << " and Paddress 0x" << std::hex << bufferPAddr));
                            for (uint64_t i=0; i<64; i++) {	//read buffer byte by byte
                                DBG_Assert( bufferVAddr <= theWQEntry.buf_addr + theWQEntry.length*64, ( << "Exceeded buffer length") );
                                tempAddr = bufferv2p[QP_id].find(bufferVAddr & PAGE_BITS);
                                DBG_Assert( tempAddr != bufferv2p[QP_id].end(), ( << "Should have found the translation") );
                                bufferPAddr = tempAddr->second + bufferVAddr - (bufferVAddr & PAGE_BITS);
                                handleTLB(bufferVAddr, bufferPAddr);
                                theLoadMessage.type() = MemoryMessage::LoadReq;
                                theLoadMessage.address() = PhysicalMemoryAddress(bufferPAddr);
                                DBG_( Verb, ( << "\tReading from address " << std::hex << bufferPAddr));
                                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                                sendMsgToMem(&theLoadMessage, true, true);

                                // Msutherl: QFlex port
                                API::QEMU_read_phys_memory(static_cast<uint8_t*>(&(packet.payload.data[i])),theLoadMessage.address(), 1);

                                bufferVAddr++;
                            }
                        }
                        outQueue[dest].push_back(packet);

                        packet_counter++;
                        packet.pkt_index++;
                    }
                    uniqueTID++;
                    if (uniqueTID == 255) uniqueTID=0;

                    //Check next WQ entry
                    theQPEntry.WQ_tail++;
                    if (theQPEntry.WQ_tail >= MAX_NUM_WQ) {		//Check for wrap-around
                        theQPEntry.WQ_tail = 0;
                        theQPEntry.WQ_SR = theQPEntry.WQ_SR ^ 1;		//Flip sense reverse bit!
                    }

                    theLoadMessage.address() = PhysicalMemoryAddress(theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t));
                    DBG_( RMC_DEBUG, Addr(theLoadMessage.address()) ( << "Sending read request for CPU " << QP_id << " address 0x" << std::hex <<
                                theLoadMessage.address() << " to L1d cache " << (RMCid)));
                    theLoadMessage.type() = MemoryMessage::LoadReq;
                    //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                    sendMsgToMem(&theLoadMessage, false, false);

                    // Msutherl: Qflex port
                    uint8_t tmp_arr[sizeof(uint64_t)];
                    API::QEMU_read_phys_memory(tmp_arr, theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t), 8);
                    std::memcpy(&WQEntry1,tmp_arr, sizeof(WQEntry1));

                    theLoadMessage.address() = PhysicalMemoryAddress(theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t)+8);
                    DBG_( RMC_DEBUG, Addr(theLoadMessage.address()) ( << "Sending read request for CPU " << QP_id << " address 0x" << std::hex <<
                                theLoadMessage.address() << " to L1d cache " << (RMCid)));
                    theLoadMessage.type() = MemoryMessage::LoadReq;
                    //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                    sendMsgToMem(&theLoadMessage, false, false);

                    // Msutherl: Qflex port
                    API::QEMU_read_phys_memory(tmp_arr, theQPEntry.WQ_base + theQPEntry.WQ_tail * sizeof(wq_entry_t)+8, 8);
                    std::memcpy(&WQEntry2,tmp_arr, sizeof(WQEntry2));

                    theWQEntry.SR = (WQEntry1 >> 61) & 1;
                    DBG_(Verb, ( << "RMC- " << RMCid << ": Trying QP_table[" << QP_id << "]. SR in QP_table entry is " << std::dec << (int)theQPEntry.WQ_SR
                                << ", while SR in WQ entry is "	<< std::dec << (int)theWQEntry.SR));
                }
                theIter->second = theQPEntry;
            }

            void sendMsgs() {
                for (queueIter = outQueue.begin(); queueIter != outQueue.end(); queueIter++) {
                    while (!queueIter->second.empty()) {
                        FLEXUS_CHANNEL(MsgOut) << *(queueIter->second.begin());
                        queueIter->second.pop_front();
                    }
                }
            }

            void driveRCP() {
                std::tr1::unordered_map<uint64_t, uint64_t>::iterator tempAddr;
                uint64_t CQHead, CQVAddr, CQPAddr, bufferVAddr, bufferPAddr;
                uint32_t i;
                //inQueue[RMCid] contains replies to own requests. The rest contain requests from remote RMCs and will be handled by the RRPP
                //Only treat replies - RCP
                while (!inQueue[RMCid].empty()) {
                    //first, find matching QP via ITT_table, using the packet's tid
                    packet = *(inQueue[RMCid].begin());
                    DBG_Assert(packet.op == RRREPLY || packet.op == RWREPLY, (<< "Reply for unknown operation"));	//TODO: Extend for atomic ops
                    inQueue[RMCid].pop_front();

                    ITT_table_iter = ITT_table.find(packet.tid);
                    DBG_Assert(ITT_table_iter != ITT_table.end(), ( << "Received packet with tid " << packet.tid << ", but no such tid pending in ITT table"));

                    ITT_table_iter->second.counter--;
                    QP_table_iter = QP_table.find(ITT_table_iter->second.QP_id);
                    DBG_Assert(QP_table_iter != QP_table.end(), ( << "Could not find QP with QP_id " << ITT_table_iter->second.QP_id));
                    int QP_id = QP_table_iter->first;
                    theCPU = API::QEMU_get_cpu_by_index(QP_id);

                    if (packet.op == RRREPLY) {
                        bufferVAddr = ITT_table_iter->second.lbuf_addr + packet.pkt_index * 64;
                        tempAddr = bufferv2p[QP_id].find(bufferVAddr & PAGE_BITS);
                        DBG_Assert( tempAddr != bufferv2p[QP_id].end(), ( << "Should have found the translation") );
                        bufferPAddr = tempAddr->second + bufferVAddr - (bufferVAddr & PAGE_BITS);
                        handleTLB(bufferVAddr, bufferPAddr);
                        for (i=0; i<64; i++) {
                            theStoreMessage.type() = MemoryMessage::StoreReq;
                            theStoreMessage.address() = PhysicalMemoryAddress(bufferPAddr+i);
                            API::QEMU_write_phys_memory(theCPU, theStoreMessage.address(), packet.payload.data[i], 1);
                            DBG_( Verb, Addr(theStoreMessage.address()) ( << "Writing data that was remotely read to local buffer: "
                                        << std::dec << (int) packet.payload.data[i] << "(byte #" << i << ")"));
                            //FLEXUS_CHANNEL(RequestOut) << theStoreMessage;
                            sendMsgToMem(&theStoreMessage, true, true);
                        }
                    }

                    //Now write CQ entry if counter == 0 and free tid
                    if (ITT_table_iter->second.counter == 0) {

                        QP_table_entry_t theQPEntry =  QP_table_iter->second;

                        CQPAddr = theQPEntry.CQ_base + theQPEntry.CQ_head * sizeof(cq_entry_t);

                        uint8_t theCQEntry = (theQPEntry.CQ_SR << 7) | (ITT_table_iter->second.WQ_index);
                        DBG_( Verb, ( << "Preparing CQ entry: CQ_SR is " << std::dec << (int)theQPEntry.CQ_SR
                                    << " and WQ index is " << std::dec << ITT_table_iter->second.WQ_index
                                    << "\n\t\t\t\t=> CQ entry to write: 0b" << (std::bitset<8>)theCQEntry));

                        theStoreMessage.type() = MemoryMessage::StoreReq;
                        theStoreMessage.address() = PhysicalMemoryAddress(CQPAddr);
                        API::QEMU_write_phys_memory(theCPU, theStoreMessage.address(), theCQEntry, sizeof(cq_entry_t));
                        DBG_( Verb, Addr(theStoreMessage.address()) ( << "Writing value to CQ entry: 0b" << (std::bitset<8>)theCQEntry
                                    << " (Paddress 0x" << std::hex << CQPAddr << ")"));
                        //FLEXUS_CHANNEL(RequestOut) << theStoreMessage;
                        sendMsgToMem(&theStoreMessage, false, false);

                        //Move CQ head
                        theQPEntry.CQ_head++;
                        //Check for wrap-around
                        if (theQPEntry.CQ_head >= MAX_NUM_WQ) {
                            theQPEntry.CQ_head = 0;
                            theQPEntry.CQ_SR = theQPEntry.CQ_SR ^ 1;		//Flip sense reverse bit!
                        }
                        DBG_( Verb, Addr(theStoreMessage.address()) ( << "Updating CQ head: " << theQPEntry.CQ_head));
                        QP_table_iter->second = theQPEntry;

                        //free tid
                        ITT_table.erase(packet.tid);
                        DBG_(Verb, (<< "Removing entry with tid = " << std::dec << (int)packet.tid << " from ITT_table"));
                    }
                }
            }

            void driveRRPP() {
                std::tr1::unordered_map<uint64_t, uint64_t>::iterator tempAddr;
                uint32_t i, context_cpu, machine_id;

                if (cfg.SingleRMCFlexpoint) {
                    machine_id = RMCid;
                } else {
                    machine_id = (RMCid - CORES) / CORES_PER_MACHINE;
                }
                context_cpu = machine_id * CORES_PER_MACHINE;

                theCPU = API::QEMU_get_cpu_by_index(context_cpu);		//FIRST CPU OF EACH MACHINE HANDLES THE CONTEXT!
                uint64_t contextVAddr, contextPAddr;
                std::tr1::unordered_map<uint64_t, std::pair<uint64_t, uint64_t> >::iterator cntxtIter;
                for (queueIter = inQueue.begin(); queueIter != inQueue.end(); queueIter++) {
                    if (queueIter->first == RMCid) continue;
                    while (!inQueue[queueIter->first].empty()) {
                        packet = *(inQueue[queueIter->first].begin());
                        DBG_Assert(packet.op == RREAD || packet.op == RWRITE, (<< "Unknown request"));
                        inQueue[queueIter->first].pop_front();
                        cntxtIter = contextMap[machine_id].find(packet.context);
                        DBG_Assert(cntxtIter != contextMap[machine_id].end(), ( << "Request for access to unregistered context"));
                        contextVAddr = cntxtIter->second.first + packet.offset;
                        tempAddr = contextv2p[machine_id].find(contextVAddr & PAGE_BITS);
                        DBG_Assert( tempAddr != contextv2p[machine_id].end(), ( << "Should have found the translation") );
                        contextPAddr = tempAddr->second + contextVAddr - (contextVAddr & PAGE_BITS);
                        handleTLB(contextVAddr, contextPAddr);
                        if (packet.op == RREAD) {
                            for (i=0; i<64; i++) {
                                theLoadMessage.type() = MemoryMessage::LoadReq;
                                theLoadMessage.address() = PhysicalMemoryAddress(contextPAddr+i*64);

                                // Msutherl: QFlex port
                                API::QEMU_read_phys_memory(static_cast<uint8_t*>(&packet.payload.data[i]), theLoadMessage.address(), 1);
                                DBG_( Verb, Addr(theLoadMessage.address()) ( << "Reading data that was requested by remote read: "
                                            << std::dec << (int)packet.payload.data[i] << "(byte #" << i << ")"));
                                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                                sendMsgToMem(&theLoadMessage, true, false);
                            }
                            packet.op = RRREPLY;
                        } else if (packet.op == RWRITE) {
                            for (i=0; i<64; i++) {
                                theStoreMessage.type() = MemoryMessage::StoreReq;
                                theStoreMessage.address() = PhysicalMemoryAddress(contextPAddr+i*64);
                                API::QEMU_write_phys_memory(theCPU, theLoadMessage.address(), packet.payload.data[i], 1);
                                DBG_( Verb, Addr(theStoreMessage.address()) ( << "Writing data that was requested by remote write: "
                                            << std::dec << (int)packet.payload.data[i]));
                                //FLEXUS_CHANNEL(RequestOut) << theStoreMessage;
                                sendMsgToMem(&theStoreMessage, true, false);
                            }
                            packet.op = RWREPLY;
                        }
                        //send reply
                        outQueue[packet.src_nid].push_back(packet);
                    }
                }
            }

            //FIXME: TLB also needs address space identifiers (not just VAddr)
            void handleTLB(uint64_t Vaddr, uint64_t Paddr) {
                std::vector<std::pair<uint64_t, uint64_t> >::iterator TLBIter = RMC_TLB.begin();

                DBG_( VVerb, Addr(theLoadMessage.address()) ( << "Checking RMC TLB for V->P translation of 0x" << std::hex << Vaddr
                            << "(page address 0x" << std::hex << (Vaddr & PAGE_BITS) << ")"));
                Vaddr = Vaddr & PAGE_BITS;
                Paddr = Paddr & PAGE_BITS;

                while (TLBIter != RMC_TLB.end()) {
                    if (TLBIter->first == Vaddr) break;
                    TLBIter++;
                }
                if (TLBIter == RMC_TLB.end()) {	//not found; insert to TLB
                    triggerPageWalk(Vaddr);
                    RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(Vaddr, Paddr));
                    if (RMC_TLB.size() > TLBsize) RMC_TLB.erase(RMC_TLB.begin());
                } else {		//found; update LRU
                    RMC_TLB.erase(TLBIter);
                    RMC_TLB.push_back(std::pair<uint64_t, uint64_t>(Vaddr, Paddr));
                }
            }

            void triggerPageWalk(uint64_t Vaddr) {
                if (NO_PAGE_WALKS) return;
                DBG_( Tmp, Addr(theLoadMessage.address()) ( << "Need to page walk for Vaddress 0x" << std::hex << Vaddr));

                std::tr1::unordered_map<uint64_t, uint64_t>::iterator tempAddr;

                uint64_t lvl1index, lvl2index, lvl3index, lvl1offset, lvl2offset, lvl3offset;
                lvl1offset = (Vaddr & iMask) >> (PObits+PT_J+PT_K);
                lvl2offset = (Vaddr & jMask) >> (PObits+PT_K);
                lvl3offset = (Vaddr & kMask) >> PObits;

                //FIXME: Always using the PTs of cpu0
                //access lvl1
                tempAddr = PTv2p[0].find((PTbaseV[0] + lvl1offset) & PAGE_BITS);
                DBG_( Verb, Addr(theLoadMessage.address()) ( << "Find translation for " << ((PTbaseV[0] + lvl1offset) & PAGE_BITS) << " - PTbaseV = " << PTbaseV << ", lvl1offset = " << lvl1offset ));
                DBG_Assert( tempAddr != PTv2p[0].end(), ( << "Should have found the translation for 0x" << std::hex << ((PTbaseV[0] + lvl1offset) & PAGE_BITS) ) );
                lvl1index = tempAddr->second + lvl1offset - (lvl1offset & PAGE_BITS);

                theLoadMessage.type() = MemoryMessage::LoadReq;
                theLoadMessage.address() = PhysicalMemoryAddress(lvl1index);

                // Msutherl: QFlex port
                uint64_t lvl2baseAddress, lvl3baseAddress;
                uint8_t tmp_arr[sizeof(uint64_t)];
                API::QEMU_read_phys_memory(tmp_arr, theLoadMessage.address(), 8);
                std::memcpy(&lvl2baseAddress,tmp_arr,sizeof(uint64_t));
                //FLEXUS_CHANNEL_ARRAY(RequestOut, RMCid) << theLoadMessage;
                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                sendMsgToMem(&theLoadMessage, true, true);

                //access lvl2
                tempAddr = PTv2p[0].find((lvl2baseAddress + lvl2offset) & PAGE_BITS);
                DBG_Assert( tempAddr != PTv2p[0].end(), ( << "Should have found the translation") );
                lvl2index = tempAddr->second + lvl2offset - (lvl2offset & PAGE_BITS);

                theLoadMessage.type() = MemoryMessage::LoadReq;
                theLoadMessage.address() = PhysicalMemoryAddress(lvl2index);
                // Msutherl: QFlex port
                API::QEMU_read_phys_memory(tmp_arr, theLoadMessage.address(), 8);
                std::memcpy(&lvl3baseAddress,tmp_arr,sizeof(uint64_t));
                //FLEXUS_CHANNEL_ARRAY(RequestOut, RMCid) << theLoadMessage;
                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                sendMsgToMem(&theLoadMessage, true, true);

                //access lvl3
                tempAddr = PTv2p[0].find((lvl3baseAddress + lvl3offset) & PAGE_BITS);
                DBG_Assert( tempAddr != PTv2p[0].end(), ( << "Should have found the translation") );
                lvl3index = tempAddr->second + lvl3offset - (lvl3offset & PAGE_BITS);

                theLoadMessage.type() = MemoryMessage::LoadReq;
                theLoadMessage.address() = PhysicalMemoryAddress(lvl3index);
                //FLEXUS_CHANNEL_ARRAY(RequestOut, RMCid) << theLoadMessage;
                //FLEXUS_CHANNEL(RequestOut) << theLoadMessage;
                sendMsgToMem(&theLoadMessage, true, true);
                DBG_( Verb, Addr(theLoadMessage.address()) ( << "Page walk done!"));

                // Msutherl: QFlex port
                uint64_t lvl3index_value;
                API::QEMU_read_phys_memory(tmp_arr,lvl3index,8);
                std::memcpy(&lvl3index_value,tmp_arr,sizeof(uint64_t));
                DBG_Assert( lvl3index_value == 42, ( << "Should have read 42 instead of " << lvl3index_value) );
            }

            void sendMsgToMem(MemoryMessage *theMsg, bool isData, bool inLLC) {
                theMsg->setFromRMC();
                theMsg->RMCext.isData = isData;
                theMsg->RMCext.inLLC = inLLC;	//TODO: Change request accordingly (in- vs out-of-LLC transfers)
                if (isData) {
                    switch (theMsg->type()) {
                        case MemoryMessage::LoadReq: {
                                                         theMsg->type() = MemoryMessage::DATA_LOAD_OP;
                                                         break;
                                                     }
                        case MemoryMessage::StoreReq: {
                                                          theMsg->type() = MemoryMessage::DATA_STORE_OP;
                                                          break;
                                                      }
                        default:
                                                      DBG_Assert(false);
                    }
                    DBG_(RMC_DEBUG, ( << "Sending " << *theMsg << " to LLC"));
                }
                FLEXUS_CHANNEL(RequestOut) << *theMsg;
            }
        };
    }

    FLEXUS_COMPONENT_INSTANTIATOR( RMCTrace, nRMCTrace );

    /*FLEXUS_PORT_ARRAY_WIDTH( RMCTrace, RequestOut ) {
      return (Flexus::Core::ComponentManager::getComponentManager().systemWidth() * 2);
      }*/

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT RMCTrace
