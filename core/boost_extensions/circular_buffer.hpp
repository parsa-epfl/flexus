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
#if !defined(BOOST_CIRCULAR_BUFFER_HPP)
#define BOOST_CIRCULAR_BUFFER_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
#pragma once
#endif

#include <boost/detail/workaround.hpp>
#include <boost/type_traits.hpp>

#if defined(NDEBUG) || defined(BOOST_DISABLE_CB_DEBUG)
#define BOOST_CB_ASSERT(Expr) ((void)0)
#define BOOST_CB_ENABLE_DEBUG 0
#else
#include <boost/assert.hpp>
#define BOOST_CB_ASSERT(Expr) BOOST_ASSERT(Expr)
#define BOOST_CB_ENABLE_DEBUG 1
#endif

#if BOOST_WORKAROUND(__BORLANDC__, <= 0x0550) || BOOST_WORKAROUND(__MWERKS__, <= 0x2407) ||        \
    BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
#define BOOST_CB_IS_CONVERTIBLE(Iterator, Type) ((void)0)
#else
#include <boost/detail/iterator.hpp>
#include <boost/static_assert.hpp>
#define BOOST_CB_IS_CONVERTIBLE(Iterator, Type)                                                    \
  BOOST_STATIC_ASSERT(                                                                             \
      (is_convertible<typename detail::iterator_traits<Iterator>::value_type, Type>::value));
#endif

#if !defined(BOOST_NO_EXCEPTIONS)
#define BOOST_CB_TRY try {
#define BOOST_CB_UNWIND(action)                                                                    \
  }                                                                                                \
  catch (...) {                                                                                    \
    action;                                                                                        \
    throw;                                                                                         \
  }
#else
#define BOOST_CB_TRY {
#define BOOST_CB_UNWIND(action) }
#endif

#include "circular_buffer/debug.hpp"
#include "circular_buffer/details.hpp"
#include "circular_buffer_fwd.hpp"

#include "circular_buffer/base.hpp"

#include "circular_buffer/adaptor.hpp"

#undef BOOST_CB_UNWIND
#undef BOOST_CB_TRY
#undef BOOST_CB_IS_CONVERTIBLE
#undef BOOST_CB_ENABLE_DEBUG
#undef BOOST_CB_ASSERT

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_HPP)
