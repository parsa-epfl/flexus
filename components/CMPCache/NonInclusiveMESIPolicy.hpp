#ifndef __NON_INCLUSIVE_MESI_POLICY_HPP__
#define __NON_INCLUSIVE_MESI_POLICY_HPP__

#include <components/CMPCache/SimpleDirectoryState.hpp>
#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/AbstractDirectory.hpp>
#include <components/CMPCache/AbstractArray.hpp>
#include <components/CMPCache/MissAddressFile.hpp>
#include <components/CMPCache/EvictBuffer.hpp>
#include <components/CMPCache/CacheBuffers.hpp>
#include <components/CMPCache/CacheState.hpp> // STATE
#include <components/CMPCache/CacheStats.hpp> // STATISTICS

namespace nCMPCache {

class NonInclusiveMESIPolicy : public AbstractPolicy {

public:

  typedef SimpleDirectoryState State;

  NonInclusiveMESIPolicy(const CMPCacheInfo & params);

  virtual ~NonInclusiveMESIPolicy();

  virtual void handleRequest( ProcessEntry_p process );
  virtual void handleSnoop( ProcessEntry_p process );
  virtual void handleReply( ProcessEntry_p process );
  virtual void handleWakeMAF( ProcessEntry_p process );
  virtual void handleCacheEvict( ProcessEntry_p process );
  virtual void handleDirEvict( ProcessEntry_p process );
  virtual void handleIdleWork( ProcessEntry_p process );

  virtual bool arrayEvictResourcesAvailable() const {
    return theCache->evictionResourcesAvailable();
  }

  virtual void reserveArrayEvictResource(int32_t n) {
    theCache->reserveEvictionResource(n);
  }
  virtual void unreserveArrayEvictResource(int32_t n) {
    theCache->unreserveEvictionResource(n);
  }

  virtual bool isQuiesced() const;

  virtual MissAddressFile & MAF() {
    return theMAF;
  }

  virtual bool hasIdleWorkAvailable();
  virtual MemoryTransport getIdleWorkTransport();
  virtual MemoryTransport getCacheEvictTransport();
  virtual MemoryTransport getDirEvictTransport();

  virtual void wakeMAFs(MemoryAddress anAddress);

  // static memembers to work with AbstractFactory
  static AbstractPolicy * createInstance(std::list<std::pair<std::string, std::string> > & args, const CMPCacheInfo & params);
  static const std::string name;

  virtual bool loadDirState(std::istream & is);
  virtual bool loadCacheState(std::istream & is);

  virtual AbstractDirEvictBuffer & DirEB() {
    return *theDirEvictBuffer;
  }
  virtual AbstractEvictBuffer & CacheEB() {
    return theCacheEvictBuffer;
  }
  virtual const AbstractEvictBuffer & CacheEB() const {
    return theCacheEvictBuffer;
  }

private:

  typedef boost::intrusive_ptr<MemoryMessage> MemoryMessage_p;
  typedef boost::intrusive_ptr<TransactionTracker> TransactionTracker_p;

  typedef MissAddressFile::iterator maf_iter_t;

  typedef AbstractLookupResult<State> DirLookupResult;
  typedef boost::intrusive_ptr<DirLookupResult> DirLookupResult_p;

  typedef AbstractArrayLookupResult<CacheState> CacheLookupResult;
  typedef boost::intrusive_ptr<CacheLookupResult> CacheLookupResult_p;

  void doEvict( ProcessEntry_p process, bool has_maf = false ); // handle incoming evicts
  void doRequest( ProcessEntry_p process, bool has_maf );
  bool allocateDirectoryEntry( DirLookupResult_p lookup, MemoryAddress anAddress, const State & aState);

  CMPCacheInfo       theCMPCacheInfo;
  AbstractDirectory<SimpleDirectoryState> * theDirectory;
  AbstractArray<CacheState> 		*		theCache;
  DirEvictBuffer<SimpleDirectoryState>	* theDirEvictBuffer;
  CacheEvictBuffer<CacheState>   theCacheEvictBuffer;
  MissAddressFile       theMAF;
  CacheStats        theStats;

  bool theDirIdleTurn;

  SimpleDirectoryState theDefaultState;

  int32_t pickSharer(const SimpleDirectoryState & state, int32_t requester, int32_t dir);
  void evictCacheBlock(CacheLookupResult_p victim);

};

}; // namespace nCMPCache

#endif // ! __NON_INCLUSIVE_MESI_POLICY_HPP__
