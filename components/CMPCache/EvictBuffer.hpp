#ifndef __EVICT_BUFFER_HPP__
#define __EVICT_BUFFER_HPP__

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <sstream>

using namespace boost::multi_index;

namespace nCMPCache {

// DirEvictBuffer needs to know about State
// but State is dependant on the Policy
// The Directory Controller doesn't know about State
// but it needs to managed reservations, so we have an
// abstract class to provide a nice interface to the Controller

class AbstractDirEvictBuffer {
public:
  virtual ~AbstractDirEvictBuffer() {}
  virtual bool full() const = 0;
  virtual bool freeSlotsPending() const = 0;
  virtual void reserve(int32_t n = 1) = 0;
  virtual void unreserve(int32_t n = 1) = 0;
  virtual bool idleWorkReady() const {
    return false;
  }
  virtual int32_t size() const = 0;
  virtual int32_t used() const = 0;
  virtual int32_t reserved() const = 0;
  virtual int32_t invalidates() const = 0;
  virtual std::string listInvalidates() const {
    return "No Invalidates";
  }
};

template<typename _State>
class AbstractDirEBEntry {
private:
  mutable int  theCacheEBReserved;

public:
  AbstractDirEBEntry() : theCacheEBReserved(0) {}

  virtual ~AbstractDirEBEntry() {}
  virtual bool invalidatesRequired() const = 0;
  virtual int invalidatesPending() const = 0;
  virtual void setInvalidatesPending(int) const = 0;
  virtual void completeInvalidate() const = 0;
  virtual _State & state() const = 0;
  virtual const MemoryAddress & address() const = 0;

  virtual int & cacheEBReserved() const {
    return theCacheEBReserved;  // Mutable :)
  }
};

class InfiniteDirEvictBuffer : public AbstractDirEvictBuffer {
public:
  virtual ~InfiniteDirEvictBuffer() {}
  virtual bool full() const {
    return false;
  }
  virtual bool freeSlotsPending() const {
    return true;
  }
  virtual void reserve(int32_t n = 1) {}
  virtual void unreserve(int32_t n = 1) {}
  virtual int32_t size() const {
    return 0;
  }
  virtual int32_t used() const {
    return 0;
  }
  virtual int32_t reserved() const {
    return 0;
  }
  virtual int32_t invalidates() const {
    return 0;
  }
};

template<typename _State>
class DirEvictBuffer : public AbstractDirEvictBuffer {
private:

  class EvictEntry : public AbstractDirEBEntry<_State> {
  private:
    MemoryAddress theAddress;
    mutable _State theState;
    mutable int theInvalidatesSent;
    mutable DirEvictBuffer<_State> * theBuffer;

  public:
    virtual bool invalidatesRequired() const {
      return (theInvalidatesSent == 0);
    }
    virtual int invalidatesPending() const {
      return theInvalidatesSent;
    }
    virtual void completeInvalidate() const {
      theInvalidatesSent--;	// Mutable
      if (theInvalidatesSent == 0) {
        theBuffer->thePendingInvalidates--; 	// Mutable :)
      }
    }
    virtual void setInvalidatesPending(int count) const {
      theInvalidatesSent = count; 	// Mutable :)
      theBuffer->thePendingInvalidates++; 	// Mutable :)
    }
    virtual _State & state() const {
      return theState;  // Mutable :)
    }
    virtual const MemoryAddress & address() const {
      return theAddress;
    }

    EvictEntry(MemoryAddress anAddress, _State & aState, DirEvictBuffer<_State> * aBuffer)
      : theAddress(anAddress), theState(aState), theInvalidatesSent(0), theBuffer(aBuffer)
    { }

    EvictEntry(const EvictEntry & e)
      : theAddress(e.theAddress), theState(e.theState), theInvalidatesSent(e.theInvalidatesSent), theBuffer(e.theBuffer)
    { }

    virtual ~EvictEntry() {}

  }; // class DirEvictBufferEntry

  friend class DirEvictBufferEntry;

  typedef multi_index_container
  < EvictEntry
  , indexed_by
  < ordered_unique
  < const_mem_fun< EvictEntry, const MemoryAddress &, &EvictEntry::address > >
  , sequenced <>
  >
  > evict_buf_t;

  evict_buf_t theDirEvictBuffer;

  typedef typename evict_buf_t::template nth_index<0>::type::iterator iterator;
  typedef typename evict_buf_t::template nth_index<1>::type::const_iterator seq_iterator;

protected:

  int32_t theSize;
  int32_t theReserve;
  int32_t theCurSize;
  int32_t thePendingInvalidates;

public:
  DirEvictBuffer(int32_t aSize) : theSize(aSize), theReserve(0), theCurSize(0), thePendingInvalidates(0) {}

  virtual const AbstractDirEBEntry<_State> * find(MemoryAddress anAddress) {
    iterator ret = theDirEvictBuffer.find(anAddress);
    return ((ret != theDirEvictBuffer.end()) ? & (*ret) : NULL);
  }

  virtual void remove(MemoryAddress anAddress) {
    iterator entry = theDirEvictBuffer.find(anAddress);
    DBG_Assert(entry != theDirEvictBuffer.end());
    if (entry->invalidatesPending()) {
      thePendingInvalidates--;
    }

    theDirEvictBuffer.erase(entry);
    theCurSize--;
  }

  virtual bool empty() const {
    return (theDirEvictBuffer.size() == 0);
  }
  virtual bool full() const {
    return ((theCurSize + theReserve) >= theSize);
  }
  virtual bool freeSlotsPending() const {
    bool ret = ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
    DBG_(Iface, ( << "In DirEvictBuffer<_State>::freeSlotsPending(), return value = " << std::boolalpha << ret ));
    return ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
  }
  virtual int32_t size() const {
    return theSize;
  }
  virtual int32_t used() const {
    return theCurSize;
  }
  virtual int32_t reserved() const {
    return theReserve;
  }
  virtual int32_t invalidates() const {
    return thePendingInvalidates;
  }

  virtual std::string listInvalidates() const {
    std::stringstream aList;
    aList << std::hex;
    seq_iterator iter = theDirEvictBuffer.get<1>().begin();
    seq_iterator end = theDirEvictBuffer.get<1>().end();
    for (; iter != end; iter++) {
      if (iter->invalidatesPending()) aList << " 0x" << iter->address();
    }
    return aList.str();
  }


  virtual bool idleWorkReady() const {
    if (theDirEvictBuffer.size() != 0) {
      return theDirEvictBuffer.get<1>().back().invalidatesRequired();
    } else {
      return false;
    }
  }

  virtual void reserve(int32_t n = 1) {
    DBG_Assert( (theReserve + theCurSize + n) <= theSize );
    theReserve += n;
  }
  virtual void unreserve(int32_t n = 1) {
    DBG_Assert( theReserve >= n );
    theReserve -= n;
  }

  virtual const AbstractDirEBEntry<_State> * oldestRequiringInvalidates() {
    typename evict_buf_t::template nth_index<1>::type::iterator o_iter;
    o_iter = theDirEvictBuffer.get<1>().begin();
    for (; o_iter != theDirEvictBuffer.get<1>().end() && !o_iter->invalidatesRequired(); o_iter++);
//		return ((o_iter != theDirEvictBuffer.get<1>().end()) ? &(*o_iter) : NULL);
    const AbstractDirEBEntry<_State> * ret = ((o_iter != theDirEvictBuffer.get<1>().end()) ? & (*o_iter) : NULL);
    if (ret == NULL) {
      DBG_(Iface, ( << "oldestRequiringInvalidates() === NULL. CurSize = " << theCurSize << ", theReserve = " << theReserve << ", theSize = " << theSize << ", PendInval = " << thePendingInvalidates ));
    }
    return ret;
  }

  virtual void insert(MemoryAddress addr, _State state) {
    theCurSize++;
    theDirEvictBuffer.get<1>().push_back(EvictEntry(addr, state, this));
  }

}; // DirEvictBuffer<>

};

#endif //! __EVICT_BUFFER_HPP__
