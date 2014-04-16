namespace nDgpTable {

class PerAddress {
  typedef SignatureMapping::Signature Signature;

  typedef std::map<Signature, Confidence> HistConfMap;
  typedef HistConfMap::iterator Iterator;
  typedef std::pair<Signature, Confidence> HistConfPair;
  typedef std::pair<Iterator, bool> InsertPair;

  HistConfMap theHistoryTraces;

public:
  PerAddress() {}

  PerAddress(Signature history) {
    add(history);
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & theHistoryTraces;
  }

  Confidence * lookup(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    if (iter != theHistoryTraces.end()) {
      return &(iter->second);
    }
    return 0;
  }

  void add(Signature history) {
    HistConfPair insert = std::make_pair(history, Confidence());
    InsertPair result = theHistoryTraces.insert(insert);
    DBG_Assert(result.second);
  }

  void remove(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    DBG_Assert(iter != theHistoryTraces.end());
    theHistoryTraces.erase(iter);
  }

  void finalize(DgpStats * stats) {
    int32_t max = 0;
    Iterator iter;

    for (iter = theHistoryTraces.begin(); iter != theHistoryTraces.end(); ++iter) {
      if (iter->second.isFlipping()) {
        stats->SigFlipflop++;
      }
#ifdef DGP_SUBTRACE
      if (!iter->second.isSubtrace()) {
        stats->NumSubtraces << std::make_pair(iter->second.numSubtraces(), 1);
        if (iter->second.numSubtraces() > max) {
          max = iter->second.numSubtraces();
        }
      }
    }

    max = (int)( 0.95 * (double)max );
    for (iter = theHistoryTraces.begin(); iter != theHistoryTraces.end(); ++iter) {
      if (iter->second.numSubtraces() >= max) {
        uint32_t ii;
        for (ii = 0; ii < iter->second.numPCseq.size(); ii++) {
          std::cout << "_" << iter->second.numPCseq[ii];
        }
        std::cout << std::endl;
      }
#endif
    }
  }

};  // end class PerAddress

#ifdef DGP_PER_ADDRESS
/************************ Per-address Signature table ************************/

class DgpSigTable {
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

public:
  DgpSigTable(uint32_t numSets,
              uint32_t assoc) {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & thePerAddressTables;
  }

  Confidence * lookup(MemoryAddress addr, Signature sig) {
    Iterator iter = thePerAddressTables.find(addr);
    if (iter != thePerAddressTables.end()) {
      return iter->second->lookup(sig);
    }
    return 0;
  }

  void add(MemoryAddress addr, Signature sig) {
    // check if an entry exists for this address
    Iterator iter = thePerAddressTables.find(addr);
    if (iter != thePerAddressTables.end()) {
      // already exists - ask it to add the new history trace
      iter->second->add(sig);
    } else {
      // doesn't exist - create a per address entry
      PerAddress_p entry = new PerAddress(sig);
      PerAddressPair insert = std::make_pair(addr, entry);
      InsertPair result = thePerAddressTables.insert(insert);
      DBG_Assert(result.second);
    }
  }

  void remove(MemoryAddress addr, Signature sig) {
    Iterator iter = thePerAddressTables.find(addr);
    DBG_Assert(iter != thePerAddressTables.end());
    iter->second->remove(sig);
  }

  void finalize(DgpStats * stats) {
    Iterator iter = thePerAddressTables.begin();
    while (iter != thePerAddressTables.end()) {
      iter->second->finalize(stats);
      ++iter;
    }
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    return anOstream << "Infinite entries, per address organization";
  }

private:
  //typedef intrusive_ptr<PerAddress> PerAddress_p;
  typedef PerAddress * PerAddress_p;
  typedef std::map<MemoryAddress, PerAddress_p> PerAddressMap;
  typedef PerAddressMap::iterator Iterator;
  typedef std::pair<MemoryAddress, PerAddress_p> PerAddressPair;
  typedef std::pair<Iterator, bool> InsertPair;

  PerAddressMap thePerAddressTables;

};  // end class DgpSigTable

#else
/************************ Global Signature table ************************/

class DgpSigTable {
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

public:
  DgpSigTable(uint32_t numSets,
              uint32_t assoc) {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const uint32_t version) {
    ar & theTable;
  }

  Confidence * lookup(MemoryAddress addr, Signature sig) {
    return theTable.lookup(sig);
  }

  void add(MemoryAddress addr, Signature sig) {
    // check if an entry exists for this address
    theTable.add(sig);
  }

  void remove(MemoryAddress addr, Signature sig) {
    theTable.remove(sig);
  }

  void finalize(DgpStats * stats) {
    theTable.finalize(stats);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    return anOstream << "Infinite entries, global organization";
  }

private:

  PerAddress theTable;
};  // end class DgpSigTable

#endif

}  // end namespace nDgpTable
