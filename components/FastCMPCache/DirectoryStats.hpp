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
#ifndef FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCMPCache {

namespace Stat = Flexus::Stat;

struct DirectoryStats {
  Stat::StatCounter theReadsOnChip;
  Stat::StatCounter theFetchesOnChip;
  Stat::StatCounter theWritesOnChip;
  Stat::StatCounter theUpgradesOnChip;
  Stat::StatCounter theReadsOffChip;
  Stat::StatCounter theFetchesOffChip;
  Stat::StatCounter theWritesOffChip;

  Stat::StatInstanceCounter<int64_t> theReadsOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theReadsOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theWritesOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theWritesOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOffChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theFetchesOnChip_ASharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_PSharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_FSharers;
  Stat::StatInstanceCounter<int64_t> theUpgradesOnChip_ASharers;

  Stat::StatCounter uatAccess;
  Stat::StatCounter uatHitBoth;
  Stat::StatCounter uatHitMain;
  Stat::StatCounter uatHitSelf; 
  Stat::StatCounter uatInv;
  Stat::StatCounter uatEvict;

  Stat::StatCounter theReadsOnChipLatency;
  Stat::StatCounter theWritesOnChipLatency;
  Stat::StatCounter theFetchesOnChipLatency;
  Stat::StatCounter theUpgradesOnChipLatency;
  Stat::StatCounter theOtherOnChipLatency;
  Stat::StatCounter theReadsOffChipLatency;
  Stat::StatCounter theWritesOffChipLatency;
  Stat::StatCounter theFetchesOffChipLatency;
  Stat::StatCounter theOtherOffChipLatency;

  DirectoryStats(std::string const &theName)
      : theReadsOnChip(theName + "-Reads:OnChip"), theFetchesOnChip(theName + "-Fetches:OnChip"),
        theWritesOnChip(theName + "-Writes:OnChip"),
        theUpgradesOnChip(theName + "-Upgrades:OnChip"),
        theReadsOffChip(theName + "-Reads:OffChip"),
        theFetchesOffChip(theName + "-Fetches:OffChip"),
        theWritesOffChip(theName + "-Writes:OffChip"),
        theReadsOffChip_PSharers(theName + "-Reads:Off:PotentialSharers"),
        theReadsOnChip_PSharers(theName + "-Reads:On:PotentialSharers"),
        theReadsOnChip_FSharers(theName + "-Reads:On:WrongSharers"),
        theReadsOnChip_ASharers(theName + "-Reads:On:AdditionalSharers"),
        theWritesOffChip_PSharers(theName + "-Writes:Off:PotentialSharers"),
        theWritesOnChip_PSharers(theName + "-Writes:On:PotentialSharers"),
        theWritesOnChip_FSharers(theName + "-Writes:On:WrongSharers"),
        theWritesOnChip_ASharers(theName + "-Writes:On:AdditionalSharers"),
        theFetchesOffChip_PSharers(theName + "-Fetches:Off:PotentialSharers"),
        theFetchesOnChip_PSharers(theName + "-Fetches:On:PotentialSharers"),
        theFetchesOnChip_FSharers(theName + "-Fetches:On:WrongSharers"),
        theFetchesOnChip_ASharers(theName + "-Fetches:On:AdditionalSharers"),
        theUpgradesOnChip_PSharers(theName + "-Upgrades:On:PotentialSharers"),
        theUpgradesOnChip_FSharers(theName + "-Upgrades:On:WrongSharers"),
        theUpgradesOnChip_ASharers(theName + "-Upgrades:On:AdditionalSharers"),
        uatAccess(theName + "-uatAccess"),
        uatHitBoth(theName + "-uatHitBoth"),
        uatHitMain(theName + "-uatHitMain"),
        uatHitSelf(theName + "-uatHitSelf"),
        uatInv(theName + "-uatInv"),
        uatEvict(theName + "-uatEvict"),
        theReadsOnChipLatency(theName + "Reads:On:Latency"),
        theWritesOnChipLatency(theName + "Writes:On:Latency"),
        theFetchesOnChipLatency(theName + "Fetches:On:Latency"),
        theUpgradesOnChipLatency(theName + "Upgrades:On:Latency"),
        theOtherOnChipLatency(theName + "Other:On:Latency"),
        theReadsOffChipLatency(theName + "Reads:Off:Latency"),
        theWritesOffChipLatency(theName + "Writes:Off:Latency"),
        theFetchesOffChipLatency(theName + "Fetches:Off:Latency"),
        theOtherOffChipLatency(theName + "Other:Off:Latency")
        {
  }

  void update() {
  }
};

} // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_DIRECTORYSTATS_HPP_INCLUDED */
