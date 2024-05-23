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
#include <components/CommonQEMU/Slices/TransactionTracker.hpp>
#include <components/CommonQEMU/Transports/PrefetchTransport.hpp>
#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT SpatialPrefetcher
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "c-level", Flexus::SharedTypes::eL1 )
  PARAMETER( UsageEnable, bool, "Enable usage stats", "usage", false )
  PARAMETER( RepetEnable, bool, "Enable repetition stats", "repet", false )
  PARAMETER( TimeRepetEnable, bool, "Enable time rep tracking", "time", false )
  PARAMETER( BufFetchEnable, bool, "Enable local buffer fetching", "buf-fetch", false )
  PARAMETER( PrefetchEnable, bool, "Enable prefetching", "enable", false )
  PARAMETER( ActiveEnable, bool, "Enable active group tracking", "active", false )
  PARAMETER( OrderEnable, bool, "Enable group ordering", "ordering", false )
  PARAMETER( StreamEnable, bool, "Enable streaming", "streaming", false )
  PARAMETER( BlockSize, long, "Cache block size", "bsize", 64 )
  PARAMETER( SgpBlocks, long, "Blocks per SPG", "sgp-blocks", 64 )
  PARAMETER( RepetType, long, "Repet type", "repet-type", 1 )
  PARAMETER( RepetFills, bool, "Record fills for repet", "repet-fills", false )
  PARAMETER( SparseOpt, bool, "Enable sparse optimization", "sparse", false )
  PARAMETER( PhtSize, long, "Size of PHT (entries, 0 = infinite)", "pht-size", 256 )
  PARAMETER( PhtAssoc, long, "Assoc of PHT (0 = fully-assoc)", "pht-assoc", 0 )
  PARAMETER( PcBits, long, "Number of PC bits to use", "pc-bits", 30 )
  PARAMETER( CptType, long, "Current Pattern Table type", "cpt-type", 1 )
  PARAMETER( CptSize, long, "Size of CPT (entries, 0 = infinite)", "cpt-size", 256 )
  PARAMETER( CptAssoc, long, "Assoc of CPT (0 = fully-assoc)", "cpt-assoc", 0 )
  PARAMETER( CptSparse, bool, "Allow unbounded sparse in CPT", "cpt-sparse", false )
  PARAMETER( FetchDist, bool, "Track buffetch dist-to-use", "fetch-dist", false )
  PARAMETER( StreamWindow, long, "Size of stream window", "window", 0 )
  PARAMETER( StreamDense, bool, "Do not use order for dense groups", "str-dense", false )
  PARAMETER( BufSize, long, "Size of stream value buffer", "buf-size", 0 )
  PARAMETER( StreamDescs, long, "Number of stream descriptors", "str-descs", 0 )
  PARAMETER( DelayedCommits, bool, "Enable delayed commit support", "dc", false )
  PARAMETER( CptFilter, long, "Size of filter table (entries, 0 = infinite)", "cpt-filt", 0 )
);

COMPONENT_INTERFACE(
  PORT(PushOutput, PrefetchTransport, PrefetchOut_1)
  PORT(PushOutput, PrefetchTransport, PrefetchOut_2)
  DRIVE(PrefetchDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SpatialPrefetcher
// clang-format on
