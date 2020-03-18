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
#ifdef __GNUC__
#pragma implementation
#endif

#include <core/fast_alloc.hpp>

void *FastAlloc::freeLists[Num_Buckets];

#ifdef FAST_ALLOC_STATS
unsigned FastAlloc::newCount[Num_Buckets];
unsigned FastAlloc::deleteCount[Num_Buckets];
unsigned FastAlloc::allocCount[Num_Buckets];
#endif

void *FastAlloc::moreStructs(int32_t bucket) {
  assert(bucket > 0 && bucket < Num_Buckets);

  int32_t sz = bucket * Alloc_Quantum;
  const int32_t nstructs = Num_Structs_Per_New; // how many to allocate?
  char *p = (char *)::new char[nstructs * sz];

#ifdef FAST_ALLOC_STATS
  ++allocCount[bucket];
#endif

  freeLists[bucket] = p;
  for (int32_t i = 0; i < (nstructs - 2); ++i, p += sz)
    *(void **)p = p + sz;
  *(void **)p = 0;

  return (p + sz);
}

#ifdef FAST_ALLOC_DEBUG

FastAlloc *FastAlloc::inUseList;

FastAlloc::FastAlloc() {
  inUsePrev = nullptr;
  inUseNext = inUseList;
  if (inUseList) {
    inUseList->inUsePrev = this;
  }
  inUseList = this;
}

FastAlloc::~FastAlloc() {
  if (inUsePrev) {
    inUsePrev->inUseNext = inUseNext;
  } else {
    assert(inUseList == this);
    inUseList = inUseNext;
  }

  inUseNext->inUsePrev = inUsePrev;
}

#endif
