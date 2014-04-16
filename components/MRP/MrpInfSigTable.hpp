namespace nMrpTable {

template < class Signature >
class PerAddressType {
  typedef std::map<Signature, Readers> HistReadersMap;
  typedef typename HistReadersMap::iterator Iterator;
  typedef std::pair<Signature, Readers> HistReadersPair;
  typedef std::pair<Iterator, bool> InsertPair;

  HistReadersMap theHistoryTraces;

public:
  PerAddressType() {
  }
  PerAddressType(Signature history, Readers readers) {
    add(history, readers);
  }

  Readers * lookup(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    if (iter != theHistoryTraces.end()) {
      return &(iter->second);
    }
    return 0;
  }

  void add(Signature history, Readers readers) {
    HistReadersPair insert = std::make_pair(history, readers);
    InsertPair result = theHistoryTraces.insert(insert);
    DBG_Assert(result.second);
  }

};  // end class PerAddressType

#ifdef MRP_GLOBAL

template < class SignatureMapping >
class MrpSigTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

public:
  MrpSigTableType(uint32_t numSets,
                  uint32_t assoc) {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  Readers * lookup(MemoryAddress addr, Signature history) {
    return theTable.lookup(history);
  }

  void add(MemoryAddress addr, Signature history, Readers readers) {
    theTable.add(history, readers);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const MrpSigTableType & aTable) {
    return anOstream << "Infinite entries, global organization";
  }

private:

  PerAddressType<Signature> theTable;

};  // end class MrpSigTableType

#else

template < class SignatureMapping >
class MrpSigTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

public:
  MrpSigTableType(uint32_t numSets,
                  uint32_t assoc) {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  Readers * lookup(MemoryAddress addr, Signature history) {
    Iterator iter = thePerAddressTables.find(addr);
    if (iter != thePerAddressTables.end()) {
      return iter->second->lookup(history);
    }
    return 0;
  }

  void add(MemoryAddress addr, Signature history, Readers readers) {
    // check if an entry exists for this address
    Iterator iter = thePerAddressTables.find(addr);
    if (iter != thePerAddressTables.end()) {
      // already exists - ask it to add the new history trace
      iter->second->add(history, readers);
    } else {
      // doesn't exist - create a per address entry
      PerAddress_p entry = new PerAddress(history, readers);
      PerAddressPair insert = std::make_pair(addr, entry);
      InsertPair result = thePerAddressTables.insert(insert);
      DBG_Assert(result.second);
    }
  }

  friend std::ostream & operator << (std::ostream & anOstream, const MrpSigTableType & aTable) {
    return anOstream << "Infinite entries, per address organization";
  }

private:
  typedef PerAddressType<Signature> PerAddress;
  typedef PerAddress * PerAddress_p;
  typedef std::map<MemoryAddress, PerAddress_p> PerAddressMap;
  typedef typename PerAddressMap::iterator Iterator;
  typedef std::pair<MemoryAddress, PerAddress_p> PerAddressPair;
  typedef std::pair<Iterator, bool> InsertPair;

  PerAddressMap thePerAddressTables;

};  // end class MrpSigTableType
#endif //MRP_GLOBAL

}  // end namespace nMrpTable
