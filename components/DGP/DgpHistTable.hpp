namespace nDgpTable {

using boost::intrusive_ptr;

class DgpHistoryTable {
public:
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  class HistoryEntry : public boost::counted_base {
  public:
    HistoryEntry() {} //For Serialization
    HistoryEntry(Signature aHist, MemoryAddress aPC)
      : theHist(aHist)
      , theFirstPC(aPC)
      , theLastPC(aPC)
      , theNumPCs(1)
    {}
    Signature theHist;
    MemoryAddress theFirstPC;
    MemoryAddress theLastPC;
    int32_t theNumPCs;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const uint32_t version) {
      ar & theHist;
    }

    friend std::ostream & operator << (std::ostream & os, const HistoryEntry & aHist) {
      os << aHist.theHist;
      return os;
    }

#ifdef DGP_SUBTRACE

    std::set<Confidence *> theSubtraces;

    void addSubtraces(Confidence * conf) {
      theSubtraces.insert(conf);
      std::set<Confidence *>::iterator iter = conf->tracePtrs.begin();
      while (iter != conf->tracePtrs.end()) {
        theSubtraces.insert(*iter);
        ++iter;
      }
    }
#endif
  };

private:
  typedef intrusive_ptr<HistoryEntry> HistEntry_p;
  typedef std::map<MemoryAddress, HistEntry_p> HistMap;
  typedef HistMap::iterator Iterator;
  typedef HistMap::const_iterator ConstIterator;
  typedef std::pair<MemoryAddress, HistEntry_p> HistPair;
  typedef std::pair<Iterator, bool> InsertPair;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & theHistory;
  }

public:
  DgpHistoryTable() {}

  void reset() {
    theHistory.clear();
    theSigMapper = nullptr;
  }

  void setSigMapper(SignatureMapping * aMapper) {
    theSigMapper = aMapper;
  }

  friend std::ostream & operator << (std::ostream & os, const DgpHistoryTable & aTable) {
    for ( ConstIterator it = aTable.theHistory.begin(); it != aTable.theHistory.end(); ++it) {
      os << it->first << ": " << *(it->second) << std::endl;
    }
    return os;
  }

  HistEntry_p find(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      return iter->second;
    }
    return HistEntry_p(0);
  }

  void remove(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      theHistory.erase(iter);
    }
  }

  HistEntry_p findAndRemove(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if (iter != theHistory.end()) {
      HistEntry_p temp = iter->second;
      theHistory.erase(iter);
      return temp;
    }
    return HistEntry_p(0);
  }

  void add(MemoryAddress anAddr, HistEntry_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    HistPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = theHistory.insert(insert);
    DBG_Assert(result.second);
  }

  Signature access(MemoryAddress anAddr, MemoryAddress aPC) {
    Signature ret;
    Iterator iter = theHistory.find(anAddr);
    if (iter == theHistory.end()) {
      // not in table - start a new trace
      ret = theSigMapper->makeHist(aPC);
      HistoryEntry * entry = new HistoryEntry(ret, aPC);
      DBG_Assert( entry != 0);
      add(anAddr, entry);
    } else {
      // the table already contains a history trace for this
      // address - munge the new PC in
      ret = theSigMapper->updateHist(iter->second->theHist, aPC);
      iter->second->theHist = ret;
      iter->second->theLastPC = aPC;
      iter->second->theNumPCs++;
    }
    return ret;
  }

private:
  // use a Map to allow fast lookup and flexible size
  HistMap theHistory;
  SignatureMapping * theSigMapper;

};  // end class DgpHistoryTable

}  // end namespace nDgpTable
