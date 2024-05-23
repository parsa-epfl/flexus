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

#ifndef __NON_INCLUSIVE_MESI_POLICY_HPP__
#define __NON_INCLUSIVE_MESI_POLICY_HPP__

#include <components/CMPCache/AbstractArray.hpp>
#include <components/CMPCache/AbstractDirectory.hpp>
#include <components/CMPCache/AbstractPolicy.hpp>
#include <components/CMPCache/CacheBuffers.hpp>
#include <components/CMPCache/CacheState.hpp> // STATE
#include <components/CMPCache/CacheStats.hpp> // STATISTICS
#include <components/CMPCache/EvictBuffer.hpp>
#include <components/CMPCache/MissAddressFile.hpp>
#include <components/CMPCache/SimpleDirectoryState.hpp>

namespace nCMPCache {

class NonInclusiveMESIPolicy : public AbstractPolicy
{

  public:
    typedef SimpleDirectoryState State;

    NonInclusiveMESIPolicy(const CMPCacheInfo& params);

    virtual ~NonInclusiveMESIPolicy();

    virtual void handleRequest(ProcessEntry_p process);
    virtual void handleSnoop(ProcessEntry_p process);
    virtual void handleReply(ProcessEntry_p process);
    virtual void handleWakeMAF(ProcessEntry_p process);
    virtual void handleCacheEvict(ProcessEntry_p process);
    virtual void handleDirEvict(ProcessEntry_p process);
    virtual void handleIdleWork(ProcessEntry_p process);

    virtual bool arrayEvictResourcesAvailable() const { return theCache->evictionResourcesAvailable(); }

    virtual void reserveArrayEvictResource(int32_t n) { theCache->reserveEvictionResource(n); }
    virtual void unreserveArrayEvictResource(int32_t n) { theCache->unreserveEvictionResource(n); }

    virtual bool isQuiesced() const;

    virtual MissAddressFile& MAF() { return theMAF; }

    virtual bool hasIdleWorkAvailable();
    virtual MemoryTransport getIdleWorkTransport();
    virtual MemoryTransport getCacheEvictTransport();
    virtual MemoryTransport getDirEvictTransport();

    virtual void wakeMAFs(MemoryAddress anAddress);

    // static memembers to work with AbstractFactory
    static AbstractPolicy* createInstance(std::list<std::pair<std::string, std::string>>& args,
                                          const CMPCacheInfo& params);
    static const std::string name;

    virtual bool loadDirState(std::istream& is);
    virtual bool loadCacheState(std::istream& is);

    virtual AbstractDirEvictBuffer& DirEB() { return *theDirEvictBuffer; }
    virtual AbstractEvictBuffer& CacheEB() { return theCacheEvictBuffer; }
    virtual const AbstractEvictBuffer& CacheEB() const { return theCacheEvictBuffer; }

  private:
    typedef boost::intrusive_ptr<MemoryMessage> MemoryMessage_p;
    typedef boost::intrusive_ptr<TransactionTracker> TransactionTracker_p;

    typedef MissAddressFile::iterator maf_iter_t;

    typedef AbstractLookupResult<State> DirLookupResult;
    typedef boost::intrusive_ptr<DirLookupResult> DirLookupResult_p;

    typedef AbstractArrayLookupResult<CacheState> CacheLookupResult;
    typedef boost::intrusive_ptr<CacheLookupResult> CacheLookupResult_p;

    void doEvict(ProcessEntry_p process,
                 bool has_maf = false); // handle incoming evicts
    void doRequest(ProcessEntry_p process, bool has_maf);
    bool allocateDirectoryEntry(DirLookupResult_p lookup, MemoryAddress anAddress, const State& aState);

    CMPCacheInfo theCMPCacheInfo;
    AbstractDirectory<SimpleDirectoryState>* theDirectory;
    AbstractArray<CacheState>* theCache;
    DirEvictBuffer<SimpleDirectoryState>* theDirEvictBuffer;
    CacheEvictBuffer<CacheState> theCacheEvictBuffer;
    MissAddressFile theMAF;
    CacheStats theStats;

    bool theDirIdleTurn;

    SimpleDirectoryState theDefaultState;

    int32_t pickSharer(const SimpleDirectoryState& state, int32_t requester, int32_t dir);
    void evictCacheBlock(CacheLookupResult_p victim);
};

}; // namespace nCMPCache

#endif // ! __NON_INCLUSIVE_MESI_POLICY_HPP__
