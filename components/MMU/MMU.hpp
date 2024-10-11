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

// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include <core/simulator_layout.hpp>

#include <components/CommonQEMU/Translation.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER(cores,    int,    "Number of cores",            "cores",    1)
    PARAMETER(itlbsets, size_t, "Number of sets of the iTlb", "itlbsets", 1)
    PARAMETER(itlbways, size_t, "Number of ways of the iTlb", "itlbways", 64)
    PARAMETER(dtlbsets, size_t, "Number of sets of the dTlb", "dtlbsets", 1)
    PARAMETER(dtlbways, size_t, "Number of ways of the dTlb", "dtlbways", 64)

    PARAMETER(stlblat,  int,    "sTLB lookup latency",        "stlblat",  2)
    PARAMETER(stlbsets, size_t, "Number of sets of the sTLB", "stlbsets", 1024)
    PARAMETER(stlbways, size_t, "Number of ways of the sTLB", "stlbways", 4)
    PARAMETER(svlbsets, size_t, "Number of sets of the sVLB", "svlbsets", 64)
    PARAMETER(svlbways, size_t, "Number of ways of the sVLB", "svlbways", 4)

    PARAMETER(perfect,  bool,   "TLB never misses",           "perfect",  false )
);

COMPONENT_INTERFACE(
    DYNAMIC_PORT_ARRAY(PushInput,  TranslationPtr, iRequestIn)
    DYNAMIC_PORT_ARRAY(PushInput,  TranslationPtr, dRequestIn)
    DYNAMIC_PORT_ARRAY(PushInput,  int,            ResyncIn)

    DYNAMIC_PORT_ARRAY(PushOutput, TranslationPtr, iTranslationReply)
    DYNAMIC_PORT_ARRAY(PushOutput, TranslationPtr, dTranslationReply)
    DYNAMIC_PORT_ARRAY(PushOutput, TranslationPtr, MemoryRequestOut)

    DYNAMIC_PORT_ARRAY(PushInput,  TranslationPtr, TLBReqIn) // this is for trace

    DYNAMIC_PORT_ARRAY(PushOutput, int,            ResyncOut)

    DRIVE(MMUDrive)
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MMU
// clang-format on
