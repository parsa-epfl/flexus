#ifndef FLEXUS_RMCPIPELINES_HPP_INCLUDED
#define FLEXUS_RMCPIPELINES_HPP_INCLUDED

#define RG1_LQ_SIZE 16
#define RG5_LQ_SIZE 16
#define RRP_LSQ_SIZE 64     //RRPP accessing memory. For an RRPP to match a DDR4 memory channel
                            //(25.6GBps) and an average latency of X cycles for accessing memory, need
                            //about 0.215 * X entries in the LSQ. E.g., w/ 150 cycles -> 32 entries
#define UNROLLQ_SIZE 10
#define CQ_BUFFERS 16
#define CQ_DELAY_LIMIT 1	//Max time (in cycles) a CQ can be buffered before the RCP tries to flush it out to memory   //FIXME: Does not currently work for values > 1.

#include <components/CommonQEMU/Slices/RMCPacket.hpp>
#include <stdlib.h>
#ifdef __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

typedef struct myWQEntry{
        uint8_t op;
        uint8_t nid;
        uint8_t cid;
        uint64_t offset;
        uint64_t length;
        uint64_t buf_addr;
        uint8_t SR : 1;
        uint8_t valid;
} WQEntry_t;

//stuff for pipelines
typedef struct pipelineStage {
	bool inputFree;
} pStage;

//RGP
typedef struct RGData_1 {
	uint16_t QP_id, WQ_tail;
	uint64_t WQ_base;
	uint8_t WQ_SR:1;
} RGData1_t;

typedef struct RG2LQ {
	uint64_t WQaddr;
	uint16_t QP_id, WQ_tail;
	uint8_t WQ_SR:1;
	bool completed;
	uint64_t WQEntry1, WQEntry2;	//local data
} RG2LQ_t;

typedef struct RGData_2 {
	RG2LQ_t incData;	//data that flows in
	std::list<RG2LQ_t> LoadQ;	//local data
	bool full() {
		if (LoadQ.size() >= RG1_LQ_SIZE) return true;
		return false;
	}
} RGData2_t;

typedef struct RGData_3 {
	WQEntry_t WQEntry;
	uint16_t QP_id, WQ_tail;
	uint8_t WQ_SR:1;
} RGData3_t;

typedef struct RG4LQ {
	uint16_t QP_id;			//need to carry it for stage 5
   	bool pleasePrefetch;		//should only be set for SABRes; for RGP to send a prefetch for 1st access
	Flexus::SharedTypes::RMCPacket packet;
  bool ready;   //added this for SEND requests; by default, ready == true. Ready == false while waiting for entry in send_buf
  bool sendBufAllocated;  //set to true only after a send buffer entry has been allocated
} RG4LQ_t;

typedef struct RGData_4 {
	RG4LQ_t incData;		//data that flows in
	std::list<RG4LQ_t> UnrollQ;	//local data
	bool full() {
		if (UnrollQ.size() >= UNROLLQ_SIZE) return true;
		return false;
	}
} RGData4_t;

typedef struct RG5LQ {
	uint64_t VAddr, PAddr;
	uint16_t QP_id;
	Flexus::SharedTypes::RMCPacket packet;
	bool completed;
} RG5LQ_t;

typedef struct RGData_5 {
	RG5LQ_t incData;			//data that flows in
	std::list<RG5LQ_t> LoadQ;	//local data
	bool full() {
		if (LoadQ.size() >= RG5_LQ_SIZE) return true;
		return false;
	}
} RGData5_t;

typedef struct RGData_6 {
	Flexus::SharedTypes::RMCPacket packet;
} RGData6_t;

//RCP

typedef struct RCData_0 {
	Flexus::SharedTypes::RMCPacket packet;
} RCData0_t;

typedef struct RCData_1 {
	Flexus::SharedTypes::RMCPacket packet;		//data that flows in
	uint64_t lbuf_addr;		//local data
} RCData1_t;

typedef struct RCData_2 {
	uint16_t QP_id, WQ_index, tid;
  bool success;
  uint8_t op;
  uint64_t buf_addr; //for incoming SEND requests
} RCData2_t;

typedef struct CQ_buf {
    //uint16_t theEntries[16];    //protocol v2.2
    uint64_t theEntries[8];       //protocol v2.3
    uint16_t entry_counter, cycle_counter, first;
	uint16_t QP_id;
} CQ_buffer_t;

typedef struct RCData_3 {
	uint64_t CQ_base;				//data that flows in
	uint16_t CQ_head, WQ_index, QP_id;
	uint8_t CQ_SR : 1;
	uint64_t PAddr;					//local data
  bool success;
  uint8_t op;
  uint64_t buf_addr;  //for incoming SEND requests: VAddr of recv buffer entry
	std::tr1::unordered_map<uint64_t, CQ_buffer_t> theCQBuffers;
	bool isFull() {
		if (theCQBuffers.size() >= CQ_BUFFERS)
			return true;
		return false;
	}
} RCData3_t;

//RRPP

typedef struct RRPData_0 {
	Flexus::SharedTypes::RMCPacket packet;
} RRPData0_t;

typedef struct RRPData_2 {
    Flexus::SharedTypes::RMCPacket packet;
    bool isSabre, cacheMe;
    uint16_t ATTidx;	//index of corresponding entry in ATT; only for SABRes
    uint64_t VAddr;
} RRPData2_t;

typedef struct RRP_LSQ_entry {
	Flexus::SharedTypes::RMCPacket packet;
	bool completed;
	uint64_t TS_scheduled;	//timestamp - pushed to MAQ
  uint64_t TS_issued; //timestamp - pushed from MAQ to mem
  uint16_t ATTidx;	//used to locate SABRe's entry in ATT
  uint16_t counter;  //for SENDs: multiple RecvBufUpdate requests for the same SEND request might co-exist in the LSQ
} RRP_LSQ_entry_t;

typedef struct RRPData_3 {
	Flexus::SharedTypes::RMCPacket packet;		//data that flows in
	uint64_t PAddr, VAddr;
    uint16_t ATTidx;
	bool cacheMe;
	std::tr1::unordered_map<uint64_t, RRP_LSQ_entry_t> LSQ;	//local data
	bool isFull() {
		if (LSQ.size() >= RRP_LSQ_SIZE)
			return true;
		return false;
	}
} RRPData3_t;

#endif
