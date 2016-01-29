#include <components/DRAMController/DRAMController.hpp>
#include <components/DRAMController/MemStats.hpp>
#include <components/DRAMController/MessageMap.hpp>

#include <components/Common/MessageQueues.hpp>

#define FLEXUS_BEGIN_COMPONENT DRAMController
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#define DBG_DefineCategories Memory
#define DBG_SetDefaultOps AddCat(Memory)
#include DBG_Control()

#include <components/Common/MemoryMap.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <core/simics/mai_api.hpp>

//#define DRAM_TEST

namespace DRAMSim{

using namespace Flexus;
using namespace Simics;
using namespace Core;
using namespace SharedTypes;


using Flexus::SharedTypes::MemoryMap;

using boost::intrusive_ptr;


void power_callback(double a, double b, double c, double d){}

void alignTransactionAddress(Transaction &trans)
{
        // zero out the low order bits which correspond to the size of a transaction
        unsigned chunk=BL*JEDEC_DATA_BUS_BITS/8;
        unsigned throwAwayBits = dramsim_log2(chunk);
        uint64_t mask=(1L<<throwAwayBits) - 1;
        uint64_t new_address=trans.address & (~mask); 
        trans.address >>= throwAwayBits;
        trans.address <<= throwAwayBits;
        DBG_Assert(trans.address==new_address);
}

class FLEXUS_COMPONENT(DRAMController) {
  FLEXUS_COMPONENT_IMPL(DRAMController);

  boost::intrusive_ptr<MemoryMap> theMemoryMap;

  MemoryMessage::MemoryMessageType theFetchReplyType;
 
  MultiChannelMemorySystem* mem; 
 
  MemStats* theStats;
   
  unsigned size; //size in MB; 

  unsigned interleaving; //address interleaving (in bytes) among memory controllers
  unsigned InterconnectDelay;
public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DRAMController)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    if (! outQueue) {
      return true; //Quiesced before initialization
    }
    return (outQueue->empty() && pendingList->empty() && evictedList->empty());
  }

uint64_t translateAddress(uint64_t address){
  uint64_t rest = address % interleaving;
  uint64_t base = address - rest;
  return rest+ ((base-flexusIndex()*interleaving)/flexusWidth());
 // return address;
}

 void read_complete(unsigned id, uint64_t address, uint64_t clock_cycle){
   DBG_Assert(!outQueue->full());
   int64_t delay=0;
   MemoryMessage::MemoryMessageType aType;
   if(evictedList->isPending(address, aType)){
       DBG_Assert(aType!=MemoryMessage::EvictAck, (<<aType));
       TimestampedTransport trans(evictedList->remove(address));
       outQueue->enqueue(trans, InterconnectDelay-1);
       delay=theFlexus->cycleCount()-trans.cycle;
  }
  else if(pendingList->isPending(address, aType)){
       TimestampedTransport trans(pendingList->remove(address));
       if((aType!= MemoryMessage::EvictAck) || ((aType == MemoryMessage::EvictAck) && trans[MemoryMessageTag]->ackRequired())) outQueue->enqueue(trans, InterconnectDelay-1);
       delay=theFlexus->cycleCount()-trans.cycle;
   }
   else DBG_Assert(false, (<<"No address "<<address<<" found among the pending transactions"));
   DBG_Assert(delay>0);
   theStats->Latency_Histogram << std::make_pair(delay, 1);
   theStats->updateMinLatency(delay);
   theStats->updateMaxLatency(delay);
 }

 void write_complete(unsigned id, uint64_t address, uint64_t clock_cycle){
   DBG_Assert(!outQueue->full());
   int64_t delay=0;
   MemoryMessage::MemoryMessageType aType;
   DBG_Assert(!evictedList->isPending(address, aType));
   if(pendingList->isPending(address, aType)){
       TimestampedTransport trans(pendingList->remove(address));
       DBG_Assert(aType==MemoryMessage::EvictAck, (<<aType));
       if(trans[MemoryMessageTag]->ackRequired()) outQueue->enqueue(trans, InterconnectDelay-1);
       delay=theFlexus->cycleCount()-trans.cycle;
   }
   else DBG_Assert(false, (<<"No address "<<address<<" found among the pending transactions"));
   DBG_Assert(delay>0);
   theStats->Latency_Histogram << std::make_pair(delay, 1);
   theStats->updateMinLatency(delay);
   theStats->updateMaxLatency(delay);
}

  // Initialization
  void initialize() {
    if (cfg.MaxRequests<cfg.MaxReplies) {
      std::cout << "Error: more requests than replies." << std::endl;
      throw FlexusException();
    }
   std::string str1=name();
   str1.append(std::to_string(flexusIndex()));
   theStats=new MemStats(str1);
 
    theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());

    outQueue.reset(new nMessageQueues::DelayFifo<DRAMSim::TimestampedTransport>(cfg.MaxReplies));
    pendingList.reset(new DRAMSim::MessageMap(cfg.MaxRequests)); 
    evictedList.reset(new DRAMSim::MessageMap(cfg.MaxRequests)); 

    if (cfg.UseFetchReply) {
      theFetchReplyType = MemoryMessage::FetchReply;
    } else {
      theFetchReplyType = MemoryMessage::MissReplyWritable;
    }
 
    interleaving = cfg.Interleaving;

    if(flexusIndex()==0) DBG_(Crit, (<<"System memory" << Processor::getProcessor(0)->getMemorySizeInMB()));

    size=cfg.Size;
    if(cfg.DynamicSize) size=Processor::getProcessor(0)->getMemorySizeInMB();
    DBG_Assert(size>0,(<<"Incorrectly read the memory size from the simics file"));
    size=size/flexusWidth();     
    InterconnectDelay=cfg.InterconnectDelay*cfg.Frequency/1000;

    mem = new MultiChannelMemorySystem(cfg.DeviceFile, cfg.MemorySystemFile, ".", "dram_results", size);

    Callback_t *read_cb = new Callback<  BOOST_PP_CAT(DRAMController,Component), void, unsigned, uint64_t, uint64_t>(this, &BOOST_PP_CAT(DRAMController,Component)::read_complete);
    Callback_t *write_cb = new Callback< BOOST_PP_CAT(DRAMController,Component), void, unsigned, uint64_t, uint64_t>(this, &BOOST_PP_CAT(DRAMController,Component)::write_complete);
    mem->RegisterCallbacks(read_cb, write_cb, power_callback );
    mem->setCPUClockSpeed(cfg.Frequency*1000000L);
  }

  void fillTracker(MemoryTransport & aMessageTransport) {
    if (aMessageTransport[TransactionTrackerTag]) {
      aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
      if (!aMessageTransport[TransactionTrackerTag]->fillType() ) {
        aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
      }
      aMessageTransport[TransactionTrackerTag]->setResponder(flexusIndex());
      aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
    }
  }



  //LoopBackIn PushInput Port
  //=========================
  bool available(interface::LoopbackIn const &) {
   bool isAvailable = mem->willAcceptTransaction();
   if(isAvailable) DBG_Assert(!pendingList->full(), Component(*this) ( << "No more space in the pending list!") ); 
   
   return isAvailable; 
 }

#ifdef DRAM_TEST
 void printMessage(MemoryTransport & aMessage){
            std::string str="/home/jevdjic/dramvalid/oracle_NAS";
            str.append(std::to_string(flexusIndex()));
            str.append(".txt");
            char* cstr = new char [str.size()+1];
            strcpy (cstr, str.c_str());
            ofstream output;
            output.open(cstr, ios::out | ios::app);

            output<< "cycle: "<< theFlexus->cycleCount();
            output<< " | Address: "<<hex<< (aMessage[MemoryMessageTag]->address());
            output<< " | type : "<< aMessage[MemoryMessageTag]->type()<<std::endl;
            output.close();
      }

 void printTransaction(Transaction & t){
            std::string str="/home/jevdjic/dramvalid/oracle_NAS";
             
            str.append(std::to_string(flexusIndex()));
            str.append(".txt");
            char* cstr = new char [str.size()+1];
            strcpy (cstr, str.c_str());
            ofstream output;
            output.open(cstr, ios::out | ios::app);

            output<< "cycle: "<< t.timeAdded;
            output<< " | Address: "<<hex<<t.address;
            output<< " | type : "<< t.transactionType<<std::endl;
            output.close();
          }

#endif

void push(interface::LoopbackIn const &, MemoryTransport & aMessageTransport) {
	DBG_(Trace, Comp(*this) ( << "request received: " << *aMessageTransport[MemoryMessageTag]) Addr(aMessageTransport[MemoryMessageTag]->address()) );
	intrusive_ptr<MemoryMessage> reply;
	TransactionType operation=DATA_READ;
	switch (aMessageTransport[MemoryMessageTag]->type()) {
		case MemoryMessage::LoadReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(MemoryMessage::LoadReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
                        theStats->MemReads++;
			break;
		case MemoryMessage::FetchReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(theFetchReplyType, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
			theStats->MemReads++;
			break;
		case MemoryMessage::StoreReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
			reply = new MemoryMessage(MemoryMessage::StoreReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 0;
			fillTracker(aMessageTransport);
			operation=DATA_WRITE;
			theStats->MemWrites++;
			break;
		case MemoryMessage::StorePrefetchReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
			reply = new MemoryMessage(MemoryMessage::StorePrefetchReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 0;
			fillTracker(aMessageTransport);
			operation=DATA_WRITE;
                        theStats->MemWrites++;
			break;
		case MemoryMessage::CmpxReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
			reply = new MemoryMessage(MemoryMessage::CmpxReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_WRITE;
                        theStats->MemWrites++;
			break;

		case MemoryMessage::ReadReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
                        theStats->MemReads++;
			break;
		case MemoryMessage::WriteReq:
		case MemoryMessage::WriteAllocate:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(MemoryMessage::MissReplyWritable, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
                        theStats->MemReads++;
			break;
		case MemoryMessage::NonAllocatingStoreReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
			// reply = aMessageTransport[MemoryMessageTag];
			// reply->type() = MemoryMessage::NonAllocatingStoreReply;
			// make a new msg just loks ALL the other msg types
			reply = new MemoryMessage(MemoryMessage::NonAllocatingStoreReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = aMessageTransport[MemoryMessageTag]->reqSize();
			fillTracker(aMessageTransport);
			operation=DATA_WRITE;
                        theStats->MemNAS++;
			break;

		case MemoryMessage::UpgradeReq:
		case MemoryMessage::UpgradeAllocate:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
			reply = new MemoryMessage(MemoryMessage::UpgradeReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 0;
			fillTracker(aMessageTransport);
			operation=DATA_WRITE;
                        theStats->MemWrites++;
			break;
		case MemoryMessage::FlushReq:
		case MemoryMessage::Flush:
		case MemoryMessage::EvictDirty:
                        // no reply required
                        if (aMessageTransport[TransactionTrackerTag]) {
                                aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
                                aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
                                aMessageTransport[TransactionTrackerTag]->complete();
                        }
                        operation=DATA_WRITE;
                        theStats->MemEvictions++;
                        reply = new MemoryMessage(MemoryMessage::EvictAck, aMessageTransport[MemoryMessageTag]->address());
                        reply->reqSize() = 0;
                        reply->ackRequired()= aMessageTransport[MemoryMessageTag]->ackRequired();
                        break;
		case MemoryMessage::EvictWritable:
		case MemoryMessage::EvictClean:
			// no reply required
			if (aMessageTransport[TransactionTrackerTag]) {
				aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
				aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
				aMessageTransport[TransactionTrackerTag]->complete();
			}
			return;
		case MemoryMessage::PrefetchReadAllocReq:
		case MemoryMessage::PrefetchReadNoAllocReq:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(MemoryMessage::PrefetchWritableReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
                        theStats->MemReads++;
			break;
		case MemoryMessage::StreamFetch:
			theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
			reply = new MemoryMessage(MemoryMessage::StreamFetchWritableReply, aMessageTransport[MemoryMessageTag]->address());
			reply->reqSize() = 64;
			fillTracker(aMessageTransport);
			operation=DATA_READ;
                        theStats->MemReads++;
			break;
		case MemoryMessage::PrefetchInsert:
			// should never happen
			DBG_Assert(false, Component(*this) ( << "DRAMController received PrefetchInsert request") );
                        break;
                case MemoryMessage::PrefetchInsertWritable:
               // should never happen
                        DBG_Assert(false, Component(*this) ( << "DRAMController received PrefetchInsertWritable request") );
                        break;
                default:
                        DBG_Assert(false, Component(*this) ( << "Don't know how to handle message: " << aMessageTransport[MemoryMessageTag]->type() << "  No reply sent." ) );
                        return;
          }
  

    DBG_(VVerb, Comp(*this) ( << "Queing reply: " << *reply) Addr(aMessageTransport[MemoryMessageTag]->address()) );
    aMessageTransport.set(MemoryMessageTag, reply);

    uint64_t address= aMessageTransport[MemoryMessageTag]->address();
    address=translateAddress(address);
    Transaction t = Transaction(operation, address, nullptr);
    alignTransactionAddress(t);
    mem->addTransaction(t);
    MemoryMessage::MemoryMessageType aType;
    bool alreadyExists = pendingList->isPending(t.address, aType);
    TimestampedTransport trans(aMessageTransport,theFlexus->cycleCount());
    if(alreadyExists) {
      DBG_Assert(aType==MemoryMessage::EvictAck, (<<aType)); //pending list must have evictions in this case
      evictedList->insert(trans, t.address);
      DBG_(Iface, ( << trans[MemoryMessageTag]->type()<<" put in evict : " << hex<<trans[MemoryMessageTag]->address()/64) );
    }
    else {
        pendingList->insert(trans, t.address);
        DBG_(Iface, ( << trans[MemoryMessageTag]->type()<<" put in pending : " <<hex<< trans[MemoryMessageTag]->address()/64) );
     }
    unsigned long size_in_bytes=size*1024L*1024L;
   if (address>=size_in_bytes) DBG_(Crit, (<<"In memory controller "<<flexusIndex()<<" address "<<address<<" is bigger than size "<<size_in_bytes));

  }

  //Drive Interfaces
  void drive(interface::DRAMDrive const &) {
    mem->update();

    if (outQueue->ready() && !FLEXUS_CHANNEL(LoopbackOut).available()) {
      DBG_(Trace, Comp(*this) ( << "Faile to send reply, channel not available." ));
    }
    while (FLEXUS_CHANNEL(LoopbackOut).available() && outQueue->ready()) {
      TimestampedTransport trans(outQueue->dequeue());
      DBG_(Trace, Comp(*this) ( << "Sending reply: " << *(trans[MemoryMessageTag]) ) Addr(trans[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL(LoopbackOut) << trans;
    }
  }

void finalize() {}

private:
 std::unique_ptr< nMessageQueues::DelayFifo< DRAMSim::TimestampedTransport > > outQueue;
 std::unique_ptr< DRAMSim::MessageMap> pendingList;
 std::unique_ptr< DRAMSim::MessageMap> evictedList; 

};

} //End Namespace DRAMSim

FLEXUS_COMPONENT_INSTANTIATOR( DRAMController, DRAMSim );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DRAMController

#define DBG_Reset
#include DBG_Control()

