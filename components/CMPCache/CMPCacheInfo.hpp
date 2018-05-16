// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


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

