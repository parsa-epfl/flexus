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

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MTManagerComponent
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include "components/MTManager/MTManager.hpp"

static const int32_t kFE_StrictRoundRobin = 0;
static const int32_t kFE_ICount = 1;

static const int32_t kBE_StrictRoundRobin = 0;
static const int32_t kBE_SmartRoundRobin = 1;

COMPONENT_PARAMETERS(
  PARAMETER(Cores, uint32_t, "Number of cores", "cores", 1 )
  PARAMETER(Threads, uint32_t, "Number of threads per core", "threads", 1 )
  PARAMETER(FrontEndPolicy, int, "Scheduling policy for front end", "front_policy", kFE_StrictRoundRobin )
  PARAMETER(BackEndPolicy, int, "Scheduling policy for back end", "back_policy", kBE_StrictRoundRobin )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PullInput, bool, EXStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, FAGStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, FStalled )
  DYNAMIC_PORT_ARRAY( PullInput, bool, DStalled )
  DYNAMIC_PORT_ARRAY( PullInput, int, FAQ_ICount)
  DYNAMIC_PORT_ARRAY( PullInput, int, FIQ_ICount)
  DYNAMIC_PORT_ARRAY( PullInput, int, ROB_ICount)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MTManagerComponent
// clang-format on
