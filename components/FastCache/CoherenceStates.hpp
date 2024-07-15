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
#ifndef __COHERENCE_STATES_HPP__
#define __COHERENCE_STATES_HPP__

#include <cstdint>
#include <string>

namespace nFastCache {

// Define states
typedef uint16_t CoherenceState_t;
static const CoherenceState_t kMigratory = 0x0F;
static const CoherenceState_t kModified  = 0x07;
static const CoherenceState_t kOwned     = 0x05;
static const CoherenceState_t kExclusive = 0x03;
static const CoherenceState_t kShared    = 0x01;
static const CoherenceState_t kInvalid   = 0x00;

static const CoherenceState_t kValid_Bit     = 0x1;
static const CoherenceState_t kExclusive_Bit = 0x2;
static const CoherenceState_t kDirty_Bit     = 0x4;
static const CoherenceState_t kMigrate_Bit   = 0x8;

inline std::string
Short2State(CoherenceState_t s)
{
    switch (s) {
        case kMigratory: return "Migratory"; break;
        case kModified: return "Modified"; break;
        case kOwned: return "Owned"; break;
        case kExclusive: return "Exclusive"; break;
        case kShared: return "Shared"; break;
        case kInvalid: return "Invalid"; break;
        default: return "Unknown"; break;
    }
}

inline std::string
state2String(CoherenceState_t s)
{
    return Short2State(s);
}

inline bool
isValid(CoherenceState_t s)
{
    return ((s & kValid_Bit) != 0);
}
} // namespace nFastCache

#endif