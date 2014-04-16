#ifndef __CMP_CACHE_INFO_HPP__
#define __CMP_CACHE_INFO_HPP__

namespace nCMPCache {

struct CMPCacheInfo {

  int   theNodeId;
  std::string theName;
  std::string theDirType;
  std::string theDirParams;
  std::string theCacheParams;
  std::string thePolicyType;
  tFillLevel theCacheLevel;

  int theCores;
  int32_t theBlockSize;
  int32_t theNumBanks;
  int32_t theBankInterleaving;
  int32_t theNumGroups;
  int32_t theGroupInterleaving;
  int32_t theMAFSize;
  int32_t theDirEBSize;
  int32_t theCacheEBSize;
  bool theEvictClean;
  int32_t theDirLatency;
  int32_t theDirIssueLatency;
  int32_t theTagLatency;
  int32_t theTagIssueLatency;
  int32_t theDataLatency;
  int32_t theDataIssueLatency;
  int32_t theQueueSize;

  CMPCacheInfo( int32_t aNodeId,
                const std::string & aName,
                std::string & aPolicyType,
                std::string & aDirType,
                std::string & aDirParams,
                std::string & aCacheParams,
                int32_t aNumCores,
                int32_t aBlockSize,
                int32_t aNumBanks,
                int32_t aBankInterleaving,
                int32_t aNumGroups,
                int32_t aGroupInterleaving,
                int32_t aMAFSize,
                int32_t aDirEBSize,
                int32_t aCacheEBSize,
                bool anEvictClean,
                tFillLevel	aCacheLevel,
                int32_t	aDirLatency,
                int32_t	aDirIssueLatency,
                int32_t	aTagLatency,
                int32_t	aTagIssueLatency,
                int32_t	aDataLatency,
                int32_t	aDataIssueLatency,
                int32_t	aQueueSize
              )
    : theNodeId(aNodeId)
    , theName(aName)
    , theDirType(aDirType)
    , theDirParams(aDirParams)
    , theCacheParams(aCacheParams)
    , thePolicyType(aPolicyType)
    , theCacheLevel(aCacheLevel)
    , theCores(aNumCores)
    , theBlockSize(aBlockSize)
    , theNumBanks(aNumBanks)
    , theBankInterleaving(aBankInterleaving)
    , theNumGroups(aNumGroups)
    , theGroupInterleaving(aGroupInterleaving)
    , theMAFSize(aMAFSize)
    , theDirEBSize(aDirEBSize)
    , theCacheEBSize(aCacheEBSize)
    , theEvictClean(anEvictClean)
    , theDirLatency(aDirLatency)
    , theDirIssueLatency(aDirIssueLatency)
    , theTagLatency(aTagLatency)
    , theTagIssueLatency(aTagIssueLatency)
    , theDataLatency(aDataLatency)
    , theDataIssueLatency(aDataIssueLatency)
    , theQueueSize(aQueueSize)
  { }

  CMPCacheInfo( const CMPCacheInfo & info)
    : theNodeId(info.theNodeId)
    , theName(info.theName)
    , theDirType(info.theDirType)
    , theDirParams(info.theDirParams)
    , theCacheParams(info.theCacheParams)
    , thePolicyType(info.thePolicyType)
    , theCacheLevel(info.theCacheLevel)
    , theCores(info.theCores)
    , theBlockSize(info.theBlockSize)
    , theNumBanks(info.theNumBanks)
    , theBankInterleaving(info.theBankInterleaving)
    , theNumGroups(info.theNumGroups)
    , theGroupInterleaving(info.theGroupInterleaving)
    , theMAFSize(info.theMAFSize)
    , theDirEBSize(info.theDirEBSize)
    , theCacheEBSize(info.theCacheEBSize)
    , theEvictClean(info.theEvictClean)
    , theDirLatency(info.theDirLatency)
    , theDirIssueLatency(info.theDirIssueLatency)
    , theTagLatency(info.theTagLatency)
    , theTagIssueLatency(info.theTagIssueLatency)
    , theDataLatency(info.theDataLatency)
    , theDataIssueLatency(info.theDataIssueLatency)
    , theQueueSize(info.theQueueSize)
  { }

};

};

#endif // ! __CMP_CACHE_INFO_HPP__

