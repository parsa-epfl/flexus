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
#ifndef FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCMPCache {

namespace Stat = Flexus::Stat;
/*
For L2 miss ratio, please use this command in your format file:

L2 miss ratio:
<EXPR:100*({sys-L2-Misses:Onchip:Read}+{sys-L2-Misses:Onchip:Write}+{sys-L2-Misses:Onchip:Fetch}+{sys-L2-Misses:Onchip:NAStore}+{sys-L2-Misses:Offchip:Read}+{sys-L2-Misses:Offchip:Write}+{sys-L2-Misses:Offchip:Fetch}+{sys-L2-Misses:Offchip:NAStore})/({sys-L2-Hits:Read}+{sys-L2-Hits:Write}+{sys-L2-Hits:Fetch}+{sys-L2-Hits:NAStore}+{sys-L2-Misses:Onchip:Read}+{sys-L2-Misses:Onchip:Write}+{sys-L2-Misses:Onchip:Fetch}+{sys-L2-Misses:Onchip:NAStore}+{sys-L2-Misses:Offchip:Read}+{sys-L2-Misses:Offchip:Write}+{sys-L2-Misses:Offchip:Fetch}+{sys-L2-Misses:Offchip:NAStore})>

*/
struct CacheStats
{
    Stat::StatCounter ReadReq_kModified_ZeroSharers;
    Stat::StatCounter ReadReq_kModified_OneSharer;
    Stat::StatCounter ReadReq_kModified_ManySharers;
    Stat::StatCounter ReadReq_kExclusive_ZeroSharers;
    Stat::StatCounter ReadReq_kExclusive_OneSharer;
    Stat::StatCounter ReadReq_kExclusive_ManySharers;
    Stat::StatCounter ReadReq_kShared_ZeroSharers;
    Stat::StatCounter ReadReq_kShared_OneSharer;
    Stat::StatCounter ReadReq_kShared_ManySharers;
    Stat::StatCounter ReadReq_kInvalid_ZeroSharers;
    Stat::StatCounter ReadReq_kInvalid_OneSharer;
    Stat::StatCounter ReadReq_kInvalid_ManySharers;

    Stat::StatCounter FetchReq_kModified_ZeroSharers;
    Stat::StatCounter FetchReq_kModified_OneSharer;
    Stat::StatCounter FetchReq_kModified_ManySharers;
    Stat::StatCounter FetchReq_kExclusive_ZeroSharers;
    Stat::StatCounter FetchReq_kExclusive_OneSharer;
    Stat::StatCounter FetchReq_kExclusive_ManySharers;
    Stat::StatCounter FetchReq_kShared_ZeroSharers;
    Stat::StatCounter FetchReq_kShared_OneSharer;
    Stat::StatCounter FetchReq_kShared_ManySharers;
    Stat::StatCounter FetchReq_kInvalid_ZeroSharers;
    Stat::StatCounter FetchReq_kInvalid_OneSharer;
    Stat::StatCounter FetchReq_kInvalid_ManySharers;

    Stat::StatCounter UpgradeReq_kModified_ZeroSharers;
    Stat::StatCounter UpgradeReq_kModified_OneSharer;
    Stat::StatCounter UpgradeReq_kModified_ManySharers;
    Stat::StatCounter UpgradeReq_kExclusive_ZeroSharers;
    Stat::StatCounter UpgradeReq_kExclusive_OneSharer;
    Stat::StatCounter UpgradeReq_kExclusive_ManySharers;
    Stat::StatCounter UpgradeReq_kShared_ZeroSharers;
    Stat::StatCounter UpgradeReq_kShared_OneSharer;
    Stat::StatCounter UpgradeReq_kShared_ManySharers;
    Stat::StatCounter UpgradeReq_kInvalid_ZeroSharers;
    Stat::StatCounter UpgradeReq_kInvalid_OneSharer;
    Stat::StatCounter UpgradeReq_kInvalid_ManySharers;

    Stat::StatCounter WriteReq_kModified_ZeroSharers;
    Stat::StatCounter WriteReq_kModified_OneSharer;
    Stat::StatCounter WriteReq_kModified_ManySharers;
    Stat::StatCounter WriteReq_kExclusive_ZeroSharers;
    Stat::StatCounter WriteReq_kExclusive_OneSharer;
    Stat::StatCounter WriteReq_kExclusive_ManySharers;
    Stat::StatCounter WriteReq_kShared_ZeroSharers;
    Stat::StatCounter WriteReq_kShared_OneSharer;
    Stat::StatCounter WriteReq_kShared_ManySharers;
    Stat::StatCounter WriteReq_kInvalid_ZeroSharers;
    Stat::StatCounter WriteReq_kInvalid_OneSharer;
    Stat::StatCounter WriteReq_kInvalid_ManySharers;

    Stat::StatCounter NonAllocatingStoreReq_kModified_ZeroSharers;
    Stat::StatCounter NonAllocatingStoreReq_kModified_OneSharer;
    Stat::StatCounter NonAllocatingStoreReq_kModified_ManySharers;
    Stat::StatCounter NonAllocatingStoreReq_kExclusive_ZeroSharers;
    Stat::StatCounter NonAllocatingStoreReq_kExclusive_OneSharer;
    Stat::StatCounter NonAllocatingStoreReq_kExclusive_ManySharers;
    Stat::StatCounter NonAllocatingStoreReq_kShared_ZeroSharers;
    Stat::StatCounter NonAllocatingStoreReq_kShared_OneSharer;
    Stat::StatCounter NonAllocatingStoreReq_kShared_ManySharers;
    Stat::StatCounter NonAllocatingStoreReq_kInvalid_ZeroSharers;
    Stat::StatCounter NonAllocatingStoreReq_kInvalid_OneSharer;
    Stat::StatCounter NonAllocatingStoreReq_kInvalid_ManySharers;

    Stat::StatCounter EvictClean_kModified_ZeroSharers;
    Stat::StatCounter EvictClean_kModified_OneSharer;
    Stat::StatCounter EvictClean_kModified_ManySharers;
    Stat::StatCounter EvictClean_kExclusive_ZeroSharers;
    Stat::StatCounter EvictClean_kExclusive_OneSharer;
    Stat::StatCounter EvictClean_kExclusive_ManySharers;
    Stat::StatCounter EvictClean_kShared_ZeroSharers;
    Stat::StatCounter EvictClean_kShared_OneSharer;
    Stat::StatCounter EvictClean_kShared_ManySharers;
    Stat::StatCounter EvictClean_kInvalid_ZeroSharers;
    Stat::StatCounter EvictClean_kInvalid_OneSharer;
    Stat::StatCounter EvictClean_kInvalid_ManySharers;

    Stat::StatCounter EvictWritable_kModified_ZeroSharers;
    Stat::StatCounter EvictWritable_kModified_OneSharer;
    Stat::StatCounter EvictWritable_kModified_ManySharers;
    Stat::StatCounter EvictWritable_kExclusive_ZeroSharers;
    Stat::StatCounter EvictWritable_kExclusive_OneSharer;
    Stat::StatCounter EvictWritable_kExclusive_ManySharers;
    Stat::StatCounter EvictWritable_kShared_ZeroSharers;
    Stat::StatCounter EvictWritable_kShared_OneSharer;
    Stat::StatCounter EvictWritable_kShared_ManySharers;
    Stat::StatCounter EvictWritable_kInvalid_ZeroSharers;
    Stat::StatCounter EvictWritable_kInvalid_OneSharer;
    Stat::StatCounter EvictWritable_kInvalid_ManySharers;

    Stat::StatCounter EvictDirty_kModified_ZeroSharers;
    Stat::StatCounter EvictDirty_kModified_OneSharer;
    Stat::StatCounter EvictDirty_kModified_ManySharers;
    Stat::StatCounter EvictDirty_kExclusive_ZeroSharers;
    Stat::StatCounter EvictDirty_kExclusive_OneSharer;
    Stat::StatCounter EvictDirty_kExclusive_ManySharers;
    Stat::StatCounter EvictDirty_kShared_ZeroSharers;
    Stat::StatCounter EvictDirty_kShared_OneSharer;
    Stat::StatCounter EvictDirty_kShared_ManySharers;
    Stat::StatCounter EvictDirty_kInvalid_ZeroSharers;
    Stat::StatCounter EvictDirty_kInvalid_OneSharer;
    Stat::StatCounter EvictDirty_kInvalid_ManySharers;

    Stat::StatCounter Hits_Read;
    Stat::StatCounter Hits_Write;
    Stat::StatCounter Hits_Fetch;
    Stat::StatCounter Hits_NAStore;

    // Misses in L2 that are satisfied from memory
    Stat::StatCounter Misses_Offchip_Read;
    Stat::StatCounter Misses_Offchip_Write;
    Stat::StatCounter Misses_Offchip_Fetch;
    Stat::StatCounter Misses_Offchip_NAStore;

    // Misses in L2 that are satisfied by peer-L1 caches
    Stat::StatCounter Misses_Onchip_Read;
    Stat::StatCounter Misses_Onchip_Write;
    Stat::StatCounter Misses_Onchip_Fetch;
    Stat::StatCounter Misses_Onchip_NAStore;

    Stat::StatCounter NoStat;

    CacheStats(std::string const& theName)
      : ReadReq_kModified_ZeroSharers(theName + "-ReadReq:kModified:ZeroSharers")
      , ReadReq_kModified_OneSharer(theName + "-ReadReq:kModified:OneSharer")
      , ReadReq_kModified_ManySharers(theName + "-ReadReq:kModified:ManySharers")
      , ReadReq_kExclusive_ZeroSharers(theName + "-ReadReq:kExclusive:ZeroSharers")
      , ReadReq_kExclusive_OneSharer(theName + "-ReadReq:kExclusive:OneSharer")
      , ReadReq_kExclusive_ManySharers(theName + "-ReadReq:kExclusive:ManySharers")
      , ReadReq_kShared_ZeroSharers(theName + "-ReadReq:kShared:ZeroSharers")
      , ReadReq_kShared_OneSharer(theName + "-ReadReq:kShared:OneSharer")
      , ReadReq_kShared_ManySharers(theName + "-ReadReq:kShared:ManySharers")
      , ReadReq_kInvalid_ZeroSharers(theName + "-ReadReq:kInvalid:ZeroSharers")
      , ReadReq_kInvalid_OneSharer(theName + "-ReadReq:kInvalid:OneSharer")
      , ReadReq_kInvalid_ManySharers(theName + "-ReadReq:kInvalid:ManySharers")

      , FetchReq_kModified_ZeroSharers(theName + "-FetchReq:kModified:ZeroSharers")
      , FetchReq_kModified_OneSharer(theName + "-FetchReq:kModified:OneSharer")
      , FetchReq_kModified_ManySharers(theName + "-FetchReq:kModified:ManySharers")
      , FetchReq_kExclusive_ZeroSharers(theName + "-FetchReq:kExclusive:ZeroSharers")
      , FetchReq_kExclusive_OneSharer(theName + "-FetchReq:kExclusive:OneSharer")
      , FetchReq_kExclusive_ManySharers(theName + "-FetchReq:kExclusive:ManySharers")
      , FetchReq_kShared_ZeroSharers(theName + "-FetchReq:kShared:ZeroSharers")
      , FetchReq_kShared_OneSharer(theName + "-FetchReq:kShared:OneSharer")
      , FetchReq_kShared_ManySharers(theName + "-FetchReq:kShared:ManySharers")
      , FetchReq_kInvalid_ZeroSharers(theName + "-FetchReq:kInvalid:ZeroSharers")
      , FetchReq_kInvalid_OneSharer(theName + "-FetchReq:kInvalid:OneSharer")
      , FetchReq_kInvalid_ManySharers(theName + "-FetchReq:kInvalid:ManySharers")

      , UpgradeReq_kModified_ZeroSharers(theName + "-UpgradeReq:kModified:ZeroSharers")
      , UpgradeReq_kModified_OneSharer(theName + "-UpgradeReq:kModified:OneSharer")
      , UpgradeReq_kModified_ManySharers(theName + "-UpgradeReq:kModified:ManySharers")
      , UpgradeReq_kExclusive_ZeroSharers(theName + "-UpgradeReq:kExclusive:ZeroSharers")
      , UpgradeReq_kExclusive_OneSharer(theName + "-UpgradeReq:kExclusive:OneSharer")
      , UpgradeReq_kExclusive_ManySharers(theName + "-UpgradeReq:kExclusive:ManySharers")
      , UpgradeReq_kShared_ZeroSharers(theName + "-UpgradeReq:kShared:ZeroSharers")
      , UpgradeReq_kShared_OneSharer(theName + "-UpgradeReq:kShared:OneSharer")
      , UpgradeReq_kShared_ManySharers(theName + "-UpgradeReq:kShared:ManySharers")
      , UpgradeReq_kInvalid_ZeroSharers(theName + "-UpgradeReq:kInvalid:ZeroSharers")
      , UpgradeReq_kInvalid_OneSharer(theName + "-UpgradeReq:kInvalid:OneSharer")
      , UpgradeReq_kInvalid_ManySharers(theName + "-UpgradeReq:kInvalid:ManySharers")

      , WriteReq_kModified_ZeroSharers(theName + "-WriteReq:kModified:ZeroSharers")
      , WriteReq_kModified_OneSharer(theName + "-WriteReq:kModified:OneSharer")
      , WriteReq_kModified_ManySharers(theName + "-WriteReq:kModified:ManySharers")
      , WriteReq_kExclusive_ZeroSharers(theName + "-WriteReq:kExclusive:ZeroSharers")
      , WriteReq_kExclusive_OneSharer(theName + "-WriteReq:kExclusive:OneSharer")
      , WriteReq_kExclusive_ManySharers(theName + "-WriteReq:kExclusive:ManySharers")
      , WriteReq_kShared_ZeroSharers(theName + "-WriteReq:kShared:ZeroSharers")
      , WriteReq_kShared_OneSharer(theName + "-WriteReq:kShared:OneSharer")
      , WriteReq_kShared_ManySharers(theName + "-WriteReq:kShared:ManySharers")
      , WriteReq_kInvalid_ZeroSharers(theName + "-WriteReq:kInvalid:ZeroSharers")
      , WriteReq_kInvalid_OneSharer(theName + "-WriteReq:kInvalid:OneSharer")
      , WriteReq_kInvalid_ManySharers(theName + "-WriteReq:kInvalid:ManySharers")

      , NonAllocatingStoreReq_kModified_ZeroSharers(theName + "-NonAllocatingStoreReq:kModified:ZeroSharers")
      , NonAllocatingStoreReq_kModified_OneSharer(theName + "-NonAllocatingStoreReq:kModified:OneSharer")
      , NonAllocatingStoreReq_kModified_ManySharers(theName + "-NonAllocatingStoreReq:kModified:ManySharers")
      , NonAllocatingStoreReq_kExclusive_ZeroSharers(theName + "-NonAllocatingStoreReq:kExclusive:ZeroSharers")
      , NonAllocatingStoreReq_kExclusive_OneSharer(theName + "-NonAllocatingStoreReq:kExclusive:OneSharer")
      , NonAllocatingStoreReq_kExclusive_ManySharers(theName + "-NonAllocatingStoreReq:kExclusive:ManySharers")
      , NonAllocatingStoreReq_kShared_ZeroSharers(theName + "-NonAllocatingStoreReq:kShared:ZeroSharers")
      , NonAllocatingStoreReq_kShared_OneSharer(theName + "-NonAllocatingStoreReq:kShared:OneSharer")
      , NonAllocatingStoreReq_kShared_ManySharers(theName + "-NonAllocatingStoreReq:kShared:ManySharers")
      , NonAllocatingStoreReq_kInvalid_ZeroSharers(theName + "-NonAllocatingStoreReq:kInvalid:ZeroSharers")
      , NonAllocatingStoreReq_kInvalid_OneSharer(theName + "-NonAllocatingStoreReq:kInvalid:OneSharer")
      , NonAllocatingStoreReq_kInvalid_ManySharers(theName + "-NonAllocatingStoreReq:kInvalid:ManySharers")

      , EvictClean_kModified_ZeroSharers(theName + "-EvictClean:kModified:ZeroSharers")
      , EvictClean_kModified_OneSharer(theName + "-EvictClean:kModified:OneSharer")
      , EvictClean_kModified_ManySharers(theName + "-EvictClean:kModified:ManySharers")
      , EvictClean_kExclusive_ZeroSharers(theName + "-EvictClean:kExclusive:ZeroSharers")
      , EvictClean_kExclusive_OneSharer(theName + "-EvictClean:kExclusive:OneSharer")
      , EvictClean_kExclusive_ManySharers(theName + "-EvictClean:kExclusive:ManySharers")
      , EvictClean_kShared_ZeroSharers(theName + "-EvictClean:kShared:ZeroSharers")
      , EvictClean_kShared_OneSharer(theName + "-EvictClean:kShared:OneSharer")
      , EvictClean_kShared_ManySharers(theName + "-EvictClean:kShared:ManySharers")
      , EvictClean_kInvalid_ZeroSharers(theName + "-EvictClean:kInvalid:ZeroSharers")
      , EvictClean_kInvalid_OneSharer(theName + "-EvictClean:kInvalid:OneSharer")
      , EvictClean_kInvalid_ManySharers(theName + "-EvictClean:kInvalid:ManySharers")

      , EvictWritable_kModified_ZeroSharers(theName + "-EvictWritable:kModified:ZeroSharers")
      , EvictWritable_kModified_OneSharer(theName + "-EvictWritable:kModified:OneSharer")
      , EvictWritable_kModified_ManySharers(theName + "-EvictWritable:kModified:ManySharers")
      , EvictWritable_kExclusive_ZeroSharers(theName + "-EvictWritable:kExclusive:ZeroSharers")
      , EvictWritable_kExclusive_OneSharer(theName + "-EvictWritable:kExclusive:OneSharer")
      , EvictWritable_kExclusive_ManySharers(theName + "-EvictWritable:kExclusive:ManySharers")
      , EvictWritable_kShared_ZeroSharers(theName + "-EvictWritable:kShared:ZeroSharers")
      , EvictWritable_kShared_OneSharer(theName + "-EvictWritable:kShared:OneSharer")
      , EvictWritable_kShared_ManySharers(theName + "-EvictWritable:kShared:ManySharers")
      , EvictWritable_kInvalid_ZeroSharers(theName + "-EvictWritable:kInvalid:ZeroSharers")
      , EvictWritable_kInvalid_OneSharer(theName + "-EvictWritable:kInvalid:OneSharer")
      , EvictWritable_kInvalid_ManySharers(theName + "-EvictWritable:kInvalid:ManySharers")

      , EvictDirty_kModified_ZeroSharers(theName + "-EvictDirty:kModified:ZeroSharers")
      , EvictDirty_kModified_OneSharer(theName + "-EvictDirty:kModified:OneSharer")
      , EvictDirty_kModified_ManySharers(theName + "-EvictDirty:kModified:ManySharers")
      , EvictDirty_kExclusive_ZeroSharers(theName + "-EvictDirty:kExclusive:ZeroSharers")
      , EvictDirty_kExclusive_OneSharer(theName + "-EvictDirty:kExclusive:OneSharer")
      , EvictDirty_kExclusive_ManySharers(theName + "-EvictDirty:kExclusive:ManySharers")
      , EvictDirty_kShared_ZeroSharers(theName + "-EvictDirty:kShared:ZeroSharers")
      , EvictDirty_kShared_OneSharer(theName + "-EvictDirty:kShared:OneSharer")
      , EvictDirty_kShared_ManySharers(theName + "-EvictDirty:kShared:ManySharers")
      , EvictDirty_kInvalid_ZeroSharers(theName + "-EvictDirty:kInvalid:ZeroSharers")
      , EvictDirty_kInvalid_OneSharer(theName + "-EvictDirty:kInvalid:OneSharer")
      , EvictDirty_kInvalid_ManySharers(theName + "-EvictDirty:kInvalid:ManySharers")

      , Hits_Read(theName + "-Hits:Read")
      , Hits_Write(theName + "-Hits:Write")
      , Hits_Fetch(theName + "-Hits:Fetch")
      , Hits_NAStore(theName + "-Hits:NAStore")

      , Misses_Offchip_Read(theName + "-Misses:Offchip:Read")
      , Misses_Offchip_Write(theName + "-Misses:Offchip:Write")
      , Misses_Offchip_Fetch(theName + "-Misses:Offchip:Fetch")
      , Misses_Offchip_NAStore(theName + "-Misses:Offchip:NAStore")

      , Misses_Onchip_Read(theName + "-Misses:Onchip:Read")
      , Misses_Onchip_Write(theName + "-Misses:Onchip:Write")
      , Misses_Onchip_Fetch(theName + "-Misses:Onchip:Fetch")
      , Misses_Onchip_NAStore(theName + "-Misses:Onchip:NAStore")

      , NoStat(theName + "-NoStat")
    {
    }

    void update() {}
};

} // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED */
