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
/*
 * Copyright (c) 1998 by Mark Hill, James Larus, and David Wood for the
 * Wisconsin Wind Tunnel Project.
 *
 * This source file is part of the WWT2 simulator, originally written by
 * Steven K. Reinhardt and subsequently maintained and enhanced by Babak
 * Falsafi, Michael J. Litzkow, Shubhendu S. Mukherjee, and
 * Steven K. Reinhardt.
 *
 * This software is furnished under a license and may be used and
 * copied only in accordance with the terms of such license and the
 * inclusion of the above copyright notice.  This software or any other
 * copies thereof or any derivative works may not be provided or
 * otherwise made available to any other persons.  Title to and ownership
 * of the software is retained by Mark Hill, James Larus, and David Wood.
 *
 * This software is provided "as is".  The licensor makes no
 * warrenties about its correctness or performance.
 *
 * Do not distribute.  Direct all inquiries to:
 *
 * Mark Hill
 * Computer Sciences Department
 * University of Wisconsin
 * 1210 West Dayton Street
 * Madison, WI 53706
 * markhill@cs.wisc.edu
 * 608-262-2196
 *
 */
#ifndef _fast_alloc_h
#define _fast_alloc_h

#ifdef __GNUC__
#pragma interface
#endif

#include <core/debug/debug.hpp>
#include <cstddef>

class FastAlloc
{
  public:
    static void* allocate(size_t);
    static void deallocate(void*, size_t);

    void* operator new(size_t);
    // inline void operator delete(void *, size_t);
    inline void operator delete(void*, size_t);

#ifdef FAST_ALLOC_DEBUG
    FastAlloc();
    virtual ~FastAlloc();
#else
    virtual ~FastAlloc() {}
#endif

  private:
    // Max_Alloc_Size is the largest object that can be allocated with
    // this class.  There's no fundamental limit, but this limits the
    // size of the freeLists array.  Let's not make this really huge
    // like in Blizzard.
    static const size_t Max_Alloc_Size = 512;

    // Alloc_Quantum is the difference in size between adjacent
    // buckets in the free list array.
    static const int32_t Log2_Alloc_Quantum = 3;
    static const int32_t Alloc_Quantum      = (1 << Log2_Alloc_Quantum);

    // Num_Buckets = bucketFor(Max_Alloc_Size) + 1
    static const int32_t Num_Buckets = ((Max_Alloc_Size + Alloc_Quantum - 1) >> Log2_Alloc_Quantum) + 1;

    // when we call new() for more structures, how many should we get?
    static const int32_t Num_Structs_Per_New = 20;

    static int32_t bucketFor(size_t);
    static void* moreStructs(int32_t bucket);

    static void* freeLists[Num_Buckets];

#ifdef FAST_ALLOC_STATS
    static unsigned newCount[Num_Buckets];
    static unsigned deleteCount[Num_Buckets];
    static unsigned allocCount[Num_Buckets];
#endif

#ifdef FAST_ALLOC_DEBUG
    static FastAlloc* inUseList;
    FastAlloc* inUsePrev;
    FastAlloc* inUseNext;
#endif
};

inline int32_t
FastAlloc::bucketFor(size_t sz)
{
    return (sz + Alloc_Quantum - 1) >> Log2_Alloc_Quantum;
}

inline void*
FastAlloc::allocate(size_t sz)
{
    int32_t b;
    void* p;

    if (sz > Max_Alloc_Size) return (void*)::new char[sz];

    b = bucketFor(sz);
    p = freeLists[b];

    if (p)
        freeLists[b] = *(void**)p;
    else
        p = moreStructs(b);

#ifdef FAST_ALLOC_STATS
    ++newCount[b];
#endif

    return p;
}

inline void*
FastAlloc::operator new(size_t sz)
{
    return allocate(sz);
}

inline void
FastAlloc::deallocate(void* p, size_t sz)
{
    int32_t b;

    DBG_Assert(p != nullptr);

    if (sz > Max_Alloc_Size) {
        ::delete[] ((char*)p);
        return;
    }

    b            = bucketFor(sz);
    *(void**)p   = freeLists[b];
    freeLists[b] = p;
#ifdef FAST_ALLOC_STATS
    ++deleteCount[b];
#endif
}

inline void
FastAlloc::operator delete(void* p, size_t sz)
{
    deallocate(p, sz);
}

#endif
