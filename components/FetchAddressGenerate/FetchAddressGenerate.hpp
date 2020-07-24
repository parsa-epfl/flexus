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

#include <core/simulator_layout.hpp>

#include <components/uFetch/uFetchTypes.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#if FLEXUS_TARGET_IS(ARM)
typedef Flexus::SharedTypes::VirtualMemoryAddress vaddr_pair;
#elif FLEXUS_TARGET_IS(v9)
typedef std::pair<Flexus::SharedTypes::VirtualMemoryAddress, Flexus::SharedTypes::VirtualMemoryAddress> vaddr_pair;
#endif
COMPONENT_PARAMETERS(
  PARAMETER( MaxFetchAddress, uint32_t, "Max fetch addresses generated per cycle", "faddrs", 10 )
  PARAMETER( MaxBPred, uint32_t, "Max branches predicted per cycle", "bpreds", 2 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this FAG", "threads", 1 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, vaddr_pair, RedirectIn )
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackIn )

  DYNAMIC_PORT_ARRAY( PushOutput, boost::intrusive_ptr<FetchCommand>, FetchAddrOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFAQ )
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  PORT( PullInput, bool, uArchHalted)

  DRIVE( FAGDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate
// clang-format on
