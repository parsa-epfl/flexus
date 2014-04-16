namespace nMrpTable {

using boost::intrusive_ptr;

template < int32_t N >
struct HistoryEntry : boost::counted_base {
  // NOTE: read accesses go into the current event, whereas
  //       writes go directly into the proper history
  HistoryEntry(const bool write, uint32_t node) {
    DBG_Assert(N > 1);
    if (write) {
      theEvents[0].set(true, node);
    } else {
      theCurrent.set(false, node);
    }
  }

  uint32_t currReaders() {
    return theCurrent.readers();
  }

  int32_t currWriter() {
    if (theCurrent.value != 0) {
      return -1;
    }
    return theEvents[0].writer();
  }

  void pushRead(uint32_t node) {
    if (theCurrent.value != 0) {
      // the current state is reading - add another sharer
      theCurrent.addReader(node);
    } else {
      // currently writing - start using the "current" event
      theCurrent.set(false, node);
      // also mark the old writer as a sharer, because it
      // still has a read-only copy of the cache block
      theCurrent.addReader(theEvents[0].writer());
    }
  }
  void pushWrite(uint32_t node) {
    if (theCurrent.value != 0) {
      // the current state is reading - push the "current" into
      // the second-most-recent history entry, the write into
      // the most recent, and everything else moves back two
      for (int32_t ii = N - 1; ii > 1; ii--) {
        theEvents[ii] = theEvents[ii-2];
      }
      theEvents[1] = theCurrent;
      theEvents[0].set(true, node);
      theCurrent.clear();
    } else {
      // currently writing - push the history events back
      for (int32_t ii = N - 1; ii > 0; ii--) {
        theEvents[ii] = theEvents[ii-1];
      }
      theEvents[0].set(true, node);
    }
  }

  HistoryEvent * current() {
    return &theCurrent;
  }

  HistoryEvent theEvents[N];
  HistoryEvent theCurrent;
};  // end struct HistoryEntry

template < int32_t N, class SignatureMapping >
class MrpHistoryTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

private:
  typedef HistoryEntry<N> HistEntry;
  typedef intrusive_ptr<HistEntry> HistEntry_p;
  typedef std::map<MemoryAddress, HistEntry_p> HistMap;
  typedef typename HistMap::iterator Iterator;
  typedef std::pair<MemoryAddress, HistEntry_p> HistPair;
  typedef std::pair<Iterator, bool> InsertPair;

public:
  MrpHistoryTableType(SignatureMapping & aMapper)
    : theSigMapper(aMapper)
  {}

  void readAccess(MemoryAddress anAddr, uint32_t node) {
    DBG_(VVerb, ( << "node " << node << " read addr 0x" << std::hex << anAddr ) );
    Iterator iter = theHistory.find(anAddr);
    if (iter == theHistory.end()) {
      // not in table - start a new history
      HistEntry_p newEntry( new HistEntry(false, node) );
      add(anAddr, newEntry);
    } else {
      // the table already contains a history trace for this
      // address - munge the new access in
      iter->second->pushRead(node);
    }
  }

  void writeAccess(MemoryAddress anAddr, uint32_t node) {
    DBG_(VVerb, ( << "node " << node << " write addr 0x" << std::hex << anAddr ) );
    Iterator iter = theHistory.find(anAddr);
    if (iter == theHistory.end()) {
      // not in table - start a new history
      HistEntry_p newEntry( new HistEntry(true, node) );
      add(anAddr, newEntry);
    } else {
      // munge the new address into the existing history trace -
      // always push the history back for a write
      iter->second->pushWrite(node);
    }
  }

  Signature makeHistory(MemoryAddress anAddr) {
    DBG_(VVerb, ( << "history for addr 0x" << std::hex << anAddr ) );
    Signature ret(0);
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      // munge the existing history events together
      ret = theSigMapper.encodeHistory(iter->second->theEvents);
    }
    return ret;
  }

  uint32_t currReaders(MemoryAddress anAddr) {
    // if currently writing, returns the list of readers for the previous write
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      return iter->second->currReaders();
    }
    return 0;
  }

  int32_t currWriter(MemoryAddress anAddr) {
    // if currently reading, returns -1; otherwise the writer's node
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      return iter->second->currWriter();
    }
    return -1;
  }

private:
  void add(MemoryAddress anAddr, HistEntry_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    HistPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = theHistory.insert(insert);
    DBG_Assert(result.second);
  }

  // use a Map to allow fast lookup and flexible size
  HistMap theHistory;
  SignatureMapping & theSigMapper;

};  // end class MrpHistoryTableType

}  // end namespace nMrpTable
