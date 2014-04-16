#ifndef __DIRECTORY_INFO_HPP__
#define __DIRECTORY_INFO_HPP__

namespace nCMPDirectory {

struct DirectoryInfo {

  int   theNodeId;
  std::string theName;
  std::string theDirType;
  std::string theDirParams;
  std::string thePolicyType;

  int theCores;
  int32_t theBlockSize;
  int32_t theNumBanks;
  int32_t theBankInterleaving;
  int32_t theMAFSize;
  int32_t theEBSize;

  DirectoryInfo( int32_t aNodeId,
                 const std::string & aName,
                 std::string & aPolicyType,
                 std::string & aDirType,
                 std::string & aDirParams,
                 int32_t aNumCores,
                 int32_t aBlockSize,
                 int32_t aNumBanks,
                 int32_t anInterleaving,
                 int32_t aMAFSize,
                 int32_t anEBSize
               )
    : theNodeId(aNodeId)
    , theName(aName)
    , theDirType(aDirType)
    , theDirParams(aDirParams)
    , thePolicyType(aPolicyType)
    , theCores(aNumCores)
    , theBlockSize(aBlockSize)
    , theNumBanks(aNumBanks)
    , theBankInterleaving(anInterleaving)
    , theMAFSize(aMAFSize)
    , theEBSize(anEBSize)
  { }

  DirectoryInfo( const DirectoryInfo & info)
    : theNodeId(info.theNodeId)
    , theName(info.theName)
    , theDirType(info.theDirType)
    , theDirParams(info.theDirParams)
    , thePolicyType(info.thePolicyType)
    , theCores(info.theCores)
    , theBlockSize(info.theBlockSize)
    , theNumBanks(info.theNumBanks)
    , theBankInterleaving(info.theBankInterleaving)
    , theMAFSize(info.theMAFSize)
    , theEBSize(info.theEBSize)
  { }

};

};

#endif // ! __DIRECTORY_INFO_HPP__

