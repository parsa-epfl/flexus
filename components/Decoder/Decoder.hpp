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
#define FLEXUS_BEGIN_COMPONENT Decoder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/uFetch/uFetchTypes.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>

typedef std::pair < int32_t /*# of instruction */ , bool /* core synchronized? */ > dispatch_status;

COMPONENT_PARAMETERS(
  PARAMETER( FIQSize, uint32_t, "Fetch instruction queue size", "fiq", 32 )
  PARAMETER( DispatchWidth, uint32_t, "Maximum dispatch per cycle", "dispatch", 8 )
  PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )
);

COMPONENT_INTERFACE(
  PORT( PushInput, pFetchBundle, FetchBundleIn)
  PORT( PullOutput, int32_t, AvailableFIQOut)
  PORT( PullInput, dispatch_status, AvailableDispatchIn)
  PORT( PushOutput, boost::intrusive_ptr< AbstractInstruction>, DispatchOut)
  PORT( PushOutput, eSquashCause, SquashOut)
  PORT( PushInput, eSquashCause, SquashIn)
  PORT( PullOutput, int32_t, ICount)
  PORT( PullOutput, bool, Stalled)

  PORT( PushOutput, int64_t, DispatchedInstructionOut) // Send instruction word to Power Tracker

  DRIVE( DecoderDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Decoder
// clang-format on
