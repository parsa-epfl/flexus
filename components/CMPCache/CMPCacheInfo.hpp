//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#ifndef __CMP_CACHE_INFO_HPP__
#define __CMP_CACHE_INFO_HPP__

#include <iostream>

#include <components/CommonQEMU/Slices/FillLevel.hpp>

namespace nCMPCache {

using namespace Flexus::SharedTypes;

struct CMPCacheInfo {

  int theNodeId;
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

  CMPCacheInfo(int32_t aNodeId, const std::string &aName, std::string &aPolicyType,
               std::string &aDirType, std::string &aDirParams, std::string &aCacheParams,
               int32_t aNumCores, int32_t aBlockSize, int32_t aNumBanks, int32_t aBankInterleaving,
               int32_t aNumGroups, int32_t aGroupInterleaving, int32_t aMAFSize, int32_t aDirEBSize,
               int32_t aCacheEBSize, bool anEvictClean, tFillLevel aCacheLevel, int32_t aDirLatency,
               int32_t aDirIssueLatency, int32_t aTagLatency, int32_t aTagIssueLatency,
               int32_t aDataLatency, int32_t aDataIssueLatency, int32_t aQueueSize)
      : theNodeId(aNodeId), theName(aName), theDirType(aDirType), theDirParams(aDirParams),
        theCacheParams(aCacheParams), thePolicyType(aPolicyType), theCacheLevel(aCacheLevel),
        theCores(aNumCores), theBlockSize(aBlockSize), theNumBanks(aNumBanks),
        theBankInterleaving(aBankInterleaving), theNumGroups(aNumGroups),
        theGroupInterleaving(aGroupInterleaving), theMAFSize(aMAFSize), theDirEBSize(aDirEBSize),
        theCacheEBSize(aCacheEBSize), theEvictClean(anEvictClean), theDirLatency(aDirLatency),
        theDirIssueLatency(aDirIssueLatency), theTagLatency(aTagLatency),
        theTagIssueLatency(aTagIssueLatency), theDataLatency(aDataLatency),
        theDataIssueLatency(aDataIssueLatency), theQueueSize(aQueueSize) {
  }

  CMPCacheInfo(const CMPCacheInfo &info)
      : theNodeId(info.theNodeId), theName(info.theName), theDirType(info.theDirType),
        theDirParams(info.theDirParams), theCacheParams(info.theCacheParams),
        thePolicyType(info.thePolicyType), theCacheLevel(info.theCacheLevel),
        theCores(info.theCores), theBlockSize(info.theBlockSize), theNumBanks(info.theNumBanks),
        theBankInterleaving(info.theBankInterleaving), theNumGroups(info.theNumGroups),
        theGroupInterleaving(info.theGroupInterleaving), theMAFSize(info.theMAFSize),
        theDirEBSize(info.theDirEBSize), theCacheEBSize(info.theCacheEBSize),
        theEvictClean(info.theEvictClean), theDirLatency(info.theDirLatency),
        theDirIssueLatency(info.theDirIssueLatency), theTagLatency(info.theTagLatency),
        theTagIssueLatency(info.theTagIssueLatency), theDataLatency(info.theDataLatency),
        theDataIssueLatency(info.theDataIssueLatency), theQueueSize(info.theQueueSize) {
  }
};

}; // namespace nCMPCache

#endif // ! __CMP_CACHE_INFO_HPP__
