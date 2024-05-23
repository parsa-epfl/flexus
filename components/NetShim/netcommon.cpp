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

#include "netcommon.hpp"

#include <string.h>

namespace nNetShim {

/***************************************************
 * GLOBAL VARIABLE
 ***************************************************/

int64_t currTime;

MessageStateList* mslFreeList = nullptr;
MessageStateList* msFreeList  = nullptr;

#define ALLOCATION_BLOCK_SIZE (512)

int32_t msSerial = 0;

MessageStateList*
allocMessageStateList(void)
{
    MessageStateList* newNode;

    if (mslFreeList == nullptr) {
        int i;

        // Allocate more nodes
        mslFreeList = new MessageStateList[ALLOCATION_BLOCK_SIZE];

        assert(mslFreeList != nullptr);

        // Set up next pointers
        for (i = 0; i < ALLOCATION_BLOCK_SIZE - 1; i++)
            mslFreeList[i].next = &mslFreeList[i + 1];
    }

    newNode     = mslFreeList;
    mslFreeList = newNode->next;

    newNode->prev  = nullptr;
    newNode->next  = nullptr;
    newNode->delay = 0;

#ifdef NS_DEBUG
    assert(newNode->usageCount == 0);
    newNode->usageCount++;
#endif

    return newNode;
}

MessageStateList*
allocMessageStateList(MessageState* msg)
{
    MessageStateList* newNode = allocMessageStateList();

    newNode->msg = msg;
    return newNode;
}

bool
freeMessageStateList(MessageStateList* msgl)
{
#ifdef NS_DEBUG
    msgl->usageCount--;
    assert(msgl->usageCount == 0);
#endif

    //  msgl->msg = (MessageState *)msgl->msg->serial;
    //  msgl->msg   = nullptr;
    msgl->prev  = nullptr;
    msgl->next  = mslFreeList;
    mslFreeList = msgl;

    return false;
}

MessageState*
allocMessageState(void)
{
    MessageState* newState;

    MessageStateList* msl;

    if (msFreeList == nullptr) {
        int32_t i;

        newState = new MessageState[ALLOCATION_BLOCK_SIZE];
        assert(newState != nullptr);

        // TODO: Value initialization is done in the constructor
        // memset(newState, 0, sizeof(MessageState[ALLOCATION_BLOCK_SIZE]));

        for (i = 0; i < ALLOCATION_BLOCK_SIZE; i++)
            freeMessageState(&newState[i]);
    }

    assert(msFreeList != nullptr);

    msl        = msFreeList;
    msFreeList = msl->next;

    newState = msl->msg;

    freeMessageStateList(msl);

    newState->reinit();
    newState->serial = msSerial++;

    TRACE(newState, " allocating message with new serial " << newState->serial);

    return newState;
}

bool
freeMessageState(MessageState* msg)
{
    MessageStateList* newNode;

    TRACE(msg, " deallocating message");

    msg->myList = nullptr;
    msg->serial = -msg->serial;

    newNode       = allocMessageStateList(msg);
    newNode->next = msFreeList;
    msFreeList    = newNode;

    return false;
}

bool
resetMessageStateSerial(void)
{
    msSerial = 0;
    return false;
}

} // namespace nNetShim
