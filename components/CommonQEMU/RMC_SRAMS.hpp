#ifndef FLEXUS_RMCSRAMS_HPP_INCLUDED
#define FLEXUS_RMCSRAMS_HPP_INCLUDED

//#define TID_RANGE 4000	//number of in-flight remote operations per node
#define TID_RANGE   100    //number of in-flight remote operations per RMC
#define ATT_SIZE	16 //64

//%%%%%%%%%%%% DEFINITIONS FOR SABRES %%%%%%%%%%%%%%%%%%%%%%%%%%
#define HARDWARE_LOCK_CHECK   //if set, the hardware also checks the value of the lock to identify an abort early
#define LOCK_LOCATION       64  //lock location in object's first block (BIT offset) -- Should be 62 for FARM
#define LOCK_LENGTH         8   //lock length in BITS -- Should be 2 for FARM - for now assume this is at most 8, i.e., a byte
#define VERSION_LOCATION    0  //version location in object's first block (BYTE offset) - for now assume it's always 0, i.e., block-aligned
#define VERSION_LENGTH      4   //version length in BYTES - for now assume it can't be more than 4 bytes

#define LOCK_MASK    ((1ULL << LOCK_LENGTH) - 1) //<< (64-LOCK_LENGTH - LOCK_LOCATION%64)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/*Entries of ITT table that is used to keep track of the multiple packets that a single remote operation
 * that is larger than a block is unrolled into
 * ITT table is indexed by tid (carried in each packet's header)
 * The size of the table is determined by the maximum number of in-flight transactions we want to support,
 * and is a function of the BW-delay product for remote ops
 */
typedef struct ITT_table_entry {
	//initialized by RGP, read and modified by RCP
	uint32_t counter;		//keeps track of the in-flight packets that belong to that remote access transaction
	uint32_t size;	//unlike the counter, this field does not get decremented and is only used by the RCP for flow control reasons
	uint16_t QP_id;			//index of QP this transaction belongs to
	uint16_t WQ_index;		//index of WQ that indicates the WQ entry this packet originated from
	uint64_t lbuf_addr;		//virtual address of local buffer - needed to write payload, for RREAD replies
    bool success;		//only for SABRes, which can fail
    bool isSABRe;		//used by the RCP to know when to replenish prefetch tokens
} ITT_table_entry_t;


/*Entries of QP table
 * QP table is indexed by the QP_id, which is retrieved from the ITT_table
 * The QP_id is a unique identifier for a Queue Pair. Each (context, QP) pair
 * should correspond to a unique QP_id.
 * 1. We limit the number of QPs per context per core to one
 * 2. For now, we think that constraining the maximum number of contexts to 10 is not practically limit soNUMA's flexibility
 * 3. We statically provision dedicated SRAM for the RMC for that number of contexts: 10*number of cores*sizeof(QP_table_entry)
 * ==> Matching of WQ to a QP_id: core_id * 10 + context_id
 * **Need to make sure that software maps context ids to a [0..9] range**
 */
 #define MAX_CTX_ID 9

typedef struct QP_table_entry {
	//used by RGP
	uint64_t WQ_base;		//stores the WQ's base physical address
	uint16_t WQ_tail;		//index of WQ tail - need log2(sizeof(WQ)) bits
	uint8_t WQ_SR:1;		//sense reverse bit for this WQ

	//used by RCP
	uint64_t CQ_base;		//same stuff for CQ
	uint16_t CQ_head;
	uint8_t CQ_SR:1;

	//used by RRPP for messaging
	int cur_jobs;

	//Software-only fields
	uint64_t lbuf_base, lbuf_size;	// ctx_size;	-- alex: ctx_size used to be here. Now moved to contextMap
} QP_table_entry_t;

/* Protocol version 2.3 (messaging)
 * This structure keeps the information that associates a context with its send/recv buffers
 * Essentially the same structure as the application's ctx_entry_t
 */

typedef struct messaging_struct {
	uint16_t ctx_id;
	uint16_t num_nodes;			//used for indexing of the send/recv buffers
	uint64_t send_buf_addr;	//base address of send queue
	uint64_t recv_buf_addr;			//base address of recv buffer
	uint64_t msg_entry_size;		//size of max recv buf entry in bytes
	uint64_t msg_buf_entry_count;	//# of messages in send/recv buffer per src/dest pair
	uint8_t SR;				//sense reverse for the send queue
	uint32_t sharedCQ_size;		//when sharedCQ is used
} messaging_struct_t;

/*Entries of Lock/versions table that is used to keep track of the active SABRes at the destination (RRPP).
 * The unique identifier for each entry of the Lock table is src id + src RMC id + tid
 * The maintained data in the Lock table can be used to use either one of the two violation detection mechanisms:
 * a) Snoop on lock, or b) compare object's version at beginning and end
 * Current support only for b.
 */

typedef struct ATT_entry {
	Flexus::SharedTypes::RMCPacket packet, lastPacket;
	uint16_t SABRE_size;	//in number of blocks - set by first packet
	uint16_t req_counter;	//gets incremented for every request received for that SABRe - for source unrolling
	uint16_t offset;		//gets incremented by each read req sent to mem - offset should always be <= req_counter
	uint16_t read_blocks;	//incremented for ever data block read
	uint64_t base_Vaddr;	//the virtual address - set by first packet
	bool lock_read;			//marks whether lock/version has been read, and thus data reading can proceed
	bool valid;				//marks if SABRe aborted -- only really used in the case of lock snooping (not in version comparison)
	bool last_data_read;	//is set when the last data is read
	bool ready;				//ready to move to next step; either lock acquired, or last data_read
	uint16_t prefetches_issued;	//While the SABRe's lock has not been acquired, the RRPP issues prefetches for the next blocks to mask the lock acquiring latency
	//The following two fields only used in the case of version comparison (not in lock snooping)
	uint64_t version;		//stores the initial version for that SABRe
	uint64_t SABREkey;		//essentially a backwards pointer, associating the ATT entry with its tag in the ATTtags
//	uint64_t last_data[8];	//stores last cache block of SABRe that was read, until version is validated
	//LATENCY INSTRUMENTATION
	uint64_t arrivalTS, versionIssue1TS, versionAcquire1TS, dataUnrollStartTS, dataUnrollEndTS, versionIssue2TS, versionAcquire2TS;
} ATT_entry_t;

#endif
