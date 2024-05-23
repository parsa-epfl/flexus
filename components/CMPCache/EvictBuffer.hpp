//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#ifndef __EVICT_BUFFER_HPP__
#define __EVICT_BUFFER_HPP__

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <sstream>

using namespace boost::multi_index;

namespace nCMPCache {

// DirEvictBuffer needs to know about State
// but State is dependant on the Policy
// The Directory Controller doesn't know about State
// but it needs to managed reservations, so we have an
// abstract class to provide a nice interface to the Controller

class AbstractDirEvictBuffer
{
  public:
    virtual ~AbstractDirEvictBuffer() {}
    virtual bool full() const             = 0;
    virtual bool freeSlotsPending() const = 0;
    virtual void reserve(int32_t n = 1)   = 0;
    virtual void unreserve(int32_t n = 1) = 0;
    virtual bool idleWorkReady() const { return false; }
    virtual int32_t size() const        = 0;
    virtual int32_t used() const        = 0;
    virtual int32_t reserved() const    = 0;
    virtual int32_t invalidates() const = 0;
    virtual std::string listInvalidates() const { return "No Invalidates"; }
};

template<typename _State>
class AbstractDirEBEntry
{
  private:
    mutable int theCacheEBReserved;

  public:
    AbstractDirEBEntry()
      : theCacheEBReserved(0)
    {
    }

    virtual ~AbstractDirEBEntry() {}
    virtual bool invalidatesRequired() const      = 0;
    virtual int invalidatesPending() const        = 0;
    virtual void setInvalidatesPending(int) const = 0;
    virtual void completeInvalidate() const       = 0;
    virtual _State& state() const                 = 0;
    virtual const MemoryAddress& address() const  = 0;

    virtual int& cacheEBReserved() const
    {
        return theCacheEBReserved; // Mutable :)
    }
};

class InfiniteDirEvictBuffer : public AbstractDirEvictBuffer
{
  public:
    virtual ~InfiniteDirEvictBuffer() {}
    virtual bool full() const { return false; }
    virtual bool freeSlotsPending() const { return true; }
    virtual void reserve(int32_t n = 1) {}
    virtual void unreserve(int32_t n = 1) {}
    virtual int32_t size() const { return 0; }
    virtual int32_t used() const { return 0; }
    virtual int32_t reserved() const { return 0; }
    virtual int32_t invalidates() const { return 0; }
};

template<typename _State>
class DirEvictBuffer : public AbstractDirEvictBuffer
{
  private:
    class EvictEntry : public AbstractDirEBEntry<_State>
    {
      private:
        MemoryAddress theAddress;
        mutable _State theState;
        mutable int theInvalidatesSent;
        mutable DirEvictBuffer<_State>* theBuffer;

      public:
        virtual bool invalidatesRequired() const { return (theInvalidatesSent == 0); }
        virtual int invalidatesPending() const { return theInvalidatesSent; }
        virtual void completeInvalidate() const
        {
            theInvalidatesSent--; // Mutable
            if (theInvalidatesSent == 0) {
                theBuffer->thePendingInvalidates--; // Mutable :)
            }
        }
        virtual void setInvalidatesPending(int count) const
        {
            theInvalidatesSent = count;         // Mutable :)
            theBuffer->thePendingInvalidates++; // Mutable :)
        }
        virtual _State& state() const
        {
            return theState; // Mutable :)
        }
        virtual const MemoryAddress& address() const { return theAddress; }

        EvictEntry(MemoryAddress anAddress, _State& aState, DirEvictBuffer<_State>* aBuffer)
          : theAddress(anAddress)
          , theState(aState)
          , theInvalidatesSent(0)
          , theBuffer(aBuffer)
        {
        }

        EvictEntry(const EvictEntry& e)
          : theAddress(e.theAddress)
          , theState(e.theState)
          , theInvalidatesSent(e.theInvalidatesSent)
          , theBuffer(e.theBuffer)
        {
        }

        virtual ~EvictEntry() {}

    }; // class DirEvictBufferEntry

    friend class DirEvictBufferEntry;

    typedef multi_index_container<
      EvictEntry,
      indexed_by<ordered_unique<const_mem_fun<EvictEntry, const MemoryAddress&, &EvictEntry::address>>, sequenced<>>>
      evict_buf_t;

    evict_buf_t theDirEvictBuffer;

    typedef typename evict_buf_t::template nth_index<0>::type::iterator iterator;
    typedef typename evict_buf_t::template nth_index<1>::type::const_iterator seq_iterator;

  protected:
    int32_t theSize;
    int32_t theReserve;
    int32_t theCurSize;
    int32_t thePendingInvalidates;

  public:
    DirEvictBuffer(int32_t aSize)
      : theSize(aSize)
      , theReserve(0)
      , theCurSize(0)
      , thePendingInvalidates(0)
    {
    }

    virtual const AbstractDirEBEntry<_State>* find(MemoryAddress anAddress)
    {
        iterator ret = theDirEvictBuffer.find(anAddress);
        return ((ret != theDirEvictBuffer.end()) ? &(*ret) : nullptr);
    }

    virtual void remove(MemoryAddress anAddress)
    {
        iterator entry = theDirEvictBuffer.find(anAddress);
        DBG_Assert(entry != theDirEvictBuffer.end());
        if (entry->invalidatesPending()) { thePendingInvalidates--; }

        theDirEvictBuffer.erase(entry);
        theCurSize--;
    }

    virtual bool empty() const { return (theDirEvictBuffer.size() == 0); }
    virtual bool full() const { return ((theCurSize + theReserve) >= theSize); }
    virtual bool freeSlotsPending() const
    {
        bool ret = ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
        DBG_(Iface, (<< "In DirEvictBuffer<_State>::freeSlotsPending(), return value = " << std::boolalpha << ret));
        return ((thePendingInvalidates > 0) || ((theCurSize + theReserve) < theSize));
    }
    virtual int32_t size() const { return theSize; }
    virtual int32_t used() const { return theCurSize; }
    virtual int32_t reserved() const { return theReserve; }
    virtual int32_t invalidates() const { return thePendingInvalidates; }

    virtual std::string listInvalidates() const
    {
        std::stringstream aList;
        aList << std::hex;
        seq_iterator iter = (theDirEvictBuffer.template get<1>()).begin();
        seq_iterator end  = (theDirEvictBuffer.template get<1>()).end();
        for (; iter != end; iter++) {
            if (iter->invalidatesPending()) aList << " 0x" << iter->address();
        }
        return aList.str();
    }

    virtual bool idleWorkReady() const
    {
        if (theDirEvictBuffer.size() != 0) {
            return (theDirEvictBuffer.template get<1>()).back().invalidatesRequired();
        } else {
            return false;
        }
    }

    virtual void reserve(int32_t n = 1)
    {
        DBG_Assert((theReserve + theCurSize + n) <= theSize);
        theReserve += n;
    }
    virtual void unreserve(int32_t n = 1)
    {
        DBG_Assert(theReserve >= n);
        theReserve -= n;
    }

    virtual const AbstractDirEBEntry<_State>* oldestRequiringInvalidates()
    {
        typename evict_buf_t::template nth_index<1>::type::iterator o_iter;
        o_iter = (theDirEvictBuffer.template get<1>()).begin();
        for (; o_iter != (theDirEvictBuffer.template get<1>()).end() && !o_iter->invalidatesRequired(); o_iter++)
            ;
        //		return ((o_iter != (theDirEvictBuffer.template get<1>()).end())?
        //&(*o_iter) : nullptr);
        const AbstractDirEBEntry<_State>* ret =
          ((o_iter != (theDirEvictBuffer.template get<1>()).end()) ? &(*o_iter) : nullptr);
        if (ret == nullptr) {
            DBG_(Iface,
                 (<< "oldestRequiringInvalidates() === nullptr. CurSize = " << theCurSize << ", theReserve = "
                  << theReserve << ", theSize = " << theSize << ", PendInval = " << thePendingInvalidates));
        }
        return ret;
    }

    virtual void insert(MemoryAddress addr, _State state)
    {
        theCurSize++;
        (theDirEvictBuffer.template get<1>()).push_back(EvictEntry(addr, state, this));
    }

}; // DirEvictBuffer<>

}; // namespace nCMPCache

#endif //! __EVICT_BUFFER_HPP__
