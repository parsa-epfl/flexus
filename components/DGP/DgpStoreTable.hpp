namespace nDgpTable {

using boost::intrusive_ptr;

class DgpStoreTable {
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  typedef std::map<MemoryAddress, MemoryAddress> StoreMap;  // <addr,pc>
  typedef StoreMap::iterator Iterator;
  typedef std::pair<MemoryAddress, MemoryAddress> StorePair;
  typedef std::pair<Iterator, bool> InsertPair;

public:

  MemoryAddress retrievePC(MemoryAddress anAddr) {
    Iterator iter = theStores.find(anAddr);
    DBG_Assert(iter != theStores.end(), ( << "no PC entry for addr " << std::hex << anAddr ) );
    theStores.erase(iter);
    return iter->second;
  }

  void insertStore(MemoryAddress anAddr, MemoryAddress aPC) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    StorePair insert = std::make_pair(anAddr, aPC);
    InsertPair result = theStores.insert(insert);
    DBG_Assert(result.second);
  }

private:
  // use a Map to allow fast lookup (this is intended to be a small
  // fully-associative structure)
  StoreMap theStores;

};  // end class DgpStoreTable

}  // end namespace nDgpTable
