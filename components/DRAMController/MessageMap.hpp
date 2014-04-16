#ifndef FLEXUS_DRAMCONTROLLER_MESSAGE_MAP_HPP_INCLUDED
#define FLEXUS_DRAMCONTROLLER_MESSAGE_MAP_HPP_INCLUDED

#include <list>
#include <map>

#include <core/stats.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

#define DBG_DeclareCategories CommonQueues
#define DBG_SetDefaultOps AddCat(CommonQueues)
#include DBG_Control()

namespace DRAMSim {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

#define DRAM_TEST

class TimestampedTransport: public MemoryTransport{
 public:
  uint64_t cycle;
  TimestampedTransport(){
  }
  TimestampedTransport(const MemoryTransport &trans, uint64_t aCycle): MemoryTransport(trans), cycle(aCycle)
  {};
  TimestampedTransport(const TimestampedTransport& trans):MemoryTransport(trans) {
    cycle=trans.cycle;
  }
  TimestampedTransport & operator = (const TimestampedTransport& trans) {
    if (this != &trans) {
        this->MemoryTransport::operator=(trans);
        cycle=trans.cycle;
    }
    return *this;  
  }
};

class MessageMap {

  std::map<uint64_t, TimestampedTransport > theQueue;
  uint32_t theSize;
  uint32_t theCurrentUsage;

public:
  MessageMap(uint32_t aSize)
    : theSize(aSize)
    , theCurrentUsage(0)
  {}

#ifdef DRAM_TEST
void print(TimestampedTransport const & aMessage){
            std::string str="/home/jevdjic/dramvalid/oracle";
            str.append(".txt");
            char* cstr = new char [str.size()+1];
            strcpy (cstr, str.c_str());
            ofstream output;
            output.open(cstr, ios::out | ios::app);

            output<< "cycle: "<< theFlexus->cycleCount();
            output<< " | Address: "<<hex<< aMessage[MemoryMessageTag]->address();
            output<< " | type : "<< aMessage[MemoryMessageTag]->type()<<std::endl;
            output.close();
  }
#endif

  void insert(const TimestampedTransport & aMessage, uint64_t address) {
    DBG_Assert(theCurrentUsage<theSize,(<<"Not enough space in the list"));
    DBG_Assert(theQueue.count(address)==0,(<<"A transaction with the same address is already in the list!"));
    theQueue[address]=aMessage;
    ++theCurrentUsage;
  }

  TimestampedTransport remove(uint64_t address) {
    DBG_Assert(theCurrentUsage>0,(<<"Empty list"));
    DBG_Assert(theQueue.count(address)!=0,(<<"Address is NOT in the list!"));
    TimestampedTransport trans(theQueue[address]);
    theQueue.erase(address);    
    --theCurrentUsage;
    return trans;
  }
  
 bool empty() {
  return theCurrentUsage==0;
 }

 bool isPending(uint64_t address, MemoryMessage::MemoryMessageType &type){
   if(theQueue.count(address)>0) {
     type=theQueue[address][MemoryMessageTag]->type();
     return true;
   }
   return false;
 } 

 bool full(){
  return theSize==theCurrentUsage;
 }
};
} //namespace

#define DBG_Reset
#include DBG_Control()

#endif
