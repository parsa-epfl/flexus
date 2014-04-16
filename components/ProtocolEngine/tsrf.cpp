#include <vector>
#include <list>

#include <core/debug/debug.hpp>

#include "tsrf.hpp"

namespace nProtocolEngine {

tTsrf::tTsrf(std::string const & anEngineName, uint32_t aSize)
  : theEngineName(anEngineName)
  , theReseveredForWBEntry_isAvail(true)
  , theReseveredForLocalEntry_isAvail(true) {
  DBG_(VVerb, ( << theEngineName << " initializing Tsrf"));
  DBG_Assert(aSize > 0);
  theTSRF.resize(aSize + 2);

  for (size_t i = 0; i < aSize; ++i) {
    DBG_(VVerb, ( << theEngineName << " pushing entry @" << & std::hex << & (theTSRF[i]) << & std::dec << " onto free list"));
    theFreeList.push_back( & (theTSRF[i]) );
  }

  theReseveredForLocalEntry = & (theTSRF[aSize + 1]);
  theReseveredForWBEntry = & (theTSRF[aSize]);
}

bool tTsrf::isEntryAvail(eEntryRequestType aType) {
  if (! theFreeList.empty()) {
    return true;
  } else {
    switch (aType) {
      case eNormalRequest:
        return false;
      case eRequestWBEntry:
        return theReseveredForWBEntry_isAvail;
      case eRequestLocalEntry:
        return theReseveredForLocalEntry_isAvail;
    }
  }
  return false;
}

tTsrfEntry * tTsrf::allocate(eEntryRequestType aType) {
  tTsrfEntry * ret_val = 0;
  if (! theFreeList.empty()) {
    ret_val = theFreeList.front();
    theFreeList.pop_front();
  } else {
    switch (aType) {
      case eNormalRequest:
        DBG_Assert( false, ( << "A tsrf entry was requested (Normal request) but no tsrf entry is free.") );
        break;
      case eRequestWBEntry:
        DBG_Assert( theReseveredForWBEntry_isAvail, ( << "A tsrf entry was requested (WB request) but no tsrf entry is free.") );
        DBG_(VVerb, ( << theEngineName << " allocating WB reserved TSRF entry.") );
        theReseveredForWBEntry_isAvail = false;
        ret_val = theReseveredForWBEntry;
        break;
      case eRequestLocalEntry:
        DBG_Assert( theReseveredForLocalEntry_isAvail, ( << "A tsrf entry was requested (Local request) but no tsrf entry is free.") );
        DBG_(VVerb, ( << theEngineName << " allocating Local reserved TSRF entry.") );
        theReseveredForLocalEntry_isAvail = false;
        ret_val = theReseveredForLocalEntry;
        break;
    }
  }
  DBG_(VVerb, ( << theEngineName << " allocating tsrf entry " << &std::hex << ret_val << & std::dec) );
  DBG_Assert(ret_val->state() == eNoThread);
  return ret_val;
}

void tTsrf::free(tTsrfEntry * anEntry) {
  DBG_Assert(anEntry->state() == eNoThread);
  DBG_(VVerb, ( << theEngineName << " freeing tsrf entry " << &std::hex << anEntry << & std::dec) );
  if (anEntry == theReseveredForWBEntry) {
    theReseveredForWBEntry_isAvail = true;
    DBG_(VVerb, ( << theEngineName << " freeing WB reserved TSRF entry.") );
  } else if  (anEntry == theReseveredForWBEntry) {
    theReseveredForLocalEntry_isAvail = true;
    DBG_(VVerb, ( << theEngineName << " freeing Local reserved TSRF entry.") );
  } else {
    theFreeList.push_back(anEntry);
  }
}

}  // namespace nProtocolEngine
