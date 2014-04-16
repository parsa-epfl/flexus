#ifndef __EVICT_BUFFER_HPP__
#define __EVICT_BUFFER_HPP__

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

namespace nCMPDirectory {

// EvictBuffer needs to know about State
// but State is dependant on the Policy
// The Directory Controller doesn't know about State
// but it needs to managed reservations, so we have an
// abstract class to provide a nice interface to the Controller

class AbstractEvictBuffer {
public:
  virtual ~AbstractEvictBuffer() {}
  virtual bool full() const = 0;
  virtual bool freeSlotsPending() const = 0;
  virtual void reserve(int32_t n = 1) = 0;
  virtual void unreserve(int32_t n = 1) = 0;
};

template<typename _State>
class AbstractEBEntry {
public:
  virtual ~AbstractEBEntry() {}
  virtual bool invalidatesRequired() const = 0;
  virtual bool invalidatesPending() const = 0;
  virtual void setInvalidatesPending() const = 0;
  virtual _State & state() const = 0;
  virtual const MemoryAddress & address() const = 0;
};

class InfiniteEvictBuffer : public AbstractEvictBuffer {
public:
  virtual ~InfiniteEvictBuffer() {}
  virtual bool full() const {
    return false;
  }
  virtual bool freeSlotsPending() const {
    return true;
  }
  virtual void reserve(int32_t n = 1) {}
  virtual void unreserve(int32_t n = 1) {}
};

template<typename _State>
class EvictBuffer : public AbstractEvictBuffer {
private:

  class EvictEntry : public AbstractEBEntry<_State> {
  private:
    MemoryAddress theAddress;
    mutable _State theState;
    mutable bool theInvalidatesSent;
    mutable EvictBuffer<_State> *theBuffer;

  public:
    virtual bool invalidatesRequired() const {
      return !theInvalidatesSent;
    }
    virtual bool invalidatesPending() const {
      return theInvalidatesSent;
    }
    virtual void setInvalidatesPending() const {
      theInvalidatesSent = true;  // Mutable :)
      theBuffer->thePendingInvalidates++;  // Mutable :)
    }
    virtual _State & state() const {
      return theState;  // Mutable :)
    }
    virtual const MemoryAddress & address() const {
      return theAddress;
    }

    EvictEntry(MemoryAddress anAddress, _State & aState, EvictBuffer<_State> *aBuffer)
      : theAddress(anAddress), theState(aState), theInvalidatesSent(false), theBuffer(aBuffer)
    { }

    EvictEntry(const EvictEntry & e)
      : theAddress(e.theAddress), theState(e.theState), theInvalidatesSent(e.theInvalidatesSent), theBuffer(e.theBuffer)
    { }

    virtual ~EvictEntry() {}

  }; // class EvictBufferEntry

  friend class EvictBufferEntry;

  typedef multi_index_container
  < EvictEntry
  , indexed_by
  < ordered_unique
  < const_mem_fun< EvictEntry, const MemoryAddress &, &EvictEntry::address > >
  , sequenced <>
  >
  > evict_buf_t;

  evict_buf_t theEvictBuffer;

  typedef typename evict_buf_t::template nth_index<0>::type::iterator iterator;

#if 0
  virtual const AbstractEBEntry<_State>& back() {
    return theEvictBuffer.get<1>().back();
  }
#endif

protected:

  int32_t theSize;
  int32_t theReserve;
  int32_t theCurSize;
  int32_t thePendingInvalidates;

public:
  EvictBuffer(int32_t aSize) : theSize(aSize), theReserve(0), theCurSize(0), thePendingInvalidates(0) {}

  virtual const AbstractEBEntry<_State>* find(MemoryAddress anAddress) {
    iterator ret = theEvictBuffer.find(anAddress);
    return ((ret != theEvictBuffer.end()) ? &(*ret) : NULL);
  }

  virtual void remove(MemoryAddress anAddress) {
    iterator entry = theEvictBuffer.find(anAddress);
    DBG_Assert(entry != theEvictBuffer.end());
    if (entry->invalidatesPending()) {
      thePendingInvalidates--;
    }

    theEvictBuffer.erase(entry);
    theCurSize--;
  }

  virtual bool empty() const {
    return (theEvictBuffer.size() == 0);
  }
  virtual bool full() const {
    return ((theCurSize + theReserve) >= theSize);
  }
  virtual bool freeSlotsPending() const {
//bool ret = ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
//DBG_(Dev, ( << "In EvictBuffer<_State>::freeSlotsPending(), return value = " << std::boolalpha << ret ));
    return ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
  }

  virtual bool idleWorkReady() const {
    return theEvictBuffer.get<1>().back().invalidatesRequired();
  }

  virtual void reserve(int32_t n = 1) {
    DBG_Assert( (theReserve + theCurSize + n) <= theSize );
    theReserve += 1;
  }
  virtual void unreserve(int32_t n = 1) {
    DBG_Assert( theReserve >= n );
    theReserve -= 1;
  }

  virtual const AbstractEBEntry<_State>* oldestRequiringInvalidates() {
    typename evict_buf_t::template nth_index<1>::type::iterator o_iter;
    o_iter = theEvictBuffer.get<1>().begin();
    for (; o_iter != theEvictBuffer.get<1>().end() && !o_iter->invalidatesRequired(); o_iter++);
    return ((o_iter != theEvictBuffer.get<1>().end()) ? &(*o_iter) : NULL);
//const AbstractEBEntry<_State>* ret = ((o_iter != theEvictBuffer.get<1>().end()) ? &(*o_iter) : NULL);
//if (ret == NULL) {
// DBG_(Dev, ( << "oldestRequiringInvalidates() === NULL. CurSize = " << theCurSize << ", theReserve = " << theReserve << ", theSize = " << theSize << ", PendInval = " << thePendingInvalidates ));
//}
//return ret;
  }

  virtual void insert(MemoryAddress addr, _State state) {
    theCurSize++;
    theEvictBuffer.get<1>().push_back(EvictEntry(addr, state, this));
  }

}; // EvictBuffer<>

};

#endif //! __EVICT_BUFFER_HPP__
