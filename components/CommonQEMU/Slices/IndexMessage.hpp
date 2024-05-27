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
#ifndef FLEXUS_SLICES__INDEX_MESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__INDEX_MESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <iostream>
#include <list>

#ifdef FLEXUS_IndexMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::IndexMessage data type"
#endif
#define FLEXUS_IndexMessage_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

namespace IndexCommand {
enum IndexCommand
{
    eLookup,
    eInsert,
    eUpdate,
    eMatchReply,
    eNoMatchReply,
    eNoUpdateReply,
};

namespace {
char* IndexCommandStr[] = { "eLookup", "eInsert", "eUpdate", "eMatchReply", "eNoMatchReply", "eNoUpdateReply" };
}

inline std::ostream&
operator<<(std::ostream& aStream, IndexCommand cmd)
{
    if (cmd <= eNoMatchReply) { aStream << IndexCommandStr[cmd]; }
    return aStream;
}
} // namespace IndexCommand

struct IndexMessage : public boost::counted_base
{
    IndexCommand::IndexCommand theCommand;
    MemoryAddress theAddress;
    int32_t theTMSc;
    int32_t theCMOB;
    int64_t theCMOBOffset;
    uint64_t theStartTime;
    std::list<MemoryAddress> thePrefix;
};

inline std::ostream&
operator<<(std::ostream& aStream, const IndexMessage& msg)
{
    aStream << msg.theCommand << " TMSc[" << msg.theTMSc << "] " << msg.theAddress << " -> " << msg.theCMOB << " @"
            << msg.theCMOBOffset;
    return aStream;
}

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_COMMON_INDEXMESSAGE_HPP_INCLUDED
