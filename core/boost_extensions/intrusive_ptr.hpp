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
#ifndef FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/lambda/lambda.hpp>

namespace boost {

struct counted_base {
  mutable int32_t theRefCount;

  counted_base() : theRefCount(0) {
  }
  virtual ~counted_base() {
  }
};

template <class T> void intrusive_ptr_add_ref(T *p) {
  static_cast<boost::counted_base const *>(p)->theRefCount++;
}

template <class T> void intrusive_ptr_release(T *p) {
  if (--static_cast<boost::counted_base const *>(p)->theRefCount == 0) {
    delete p;
  }
}

} // namespace boost

namespace boost {
namespace serialization {
template <class Archive, class T>
void save(Archive &ar, ::boost::intrusive_ptr<T> const &ptr, uint32_t version) {
  T *t = ptr.get();
  ar &t;
}

template <class Archive, class T>
void load(Archive &ar, ::boost::intrusive_ptr<T> &ptr, uint32_t version) {
  T *t;
  ar &t;
  ptr = boost::intrusive_ptr<T>(t);
}

template <class Archive, class T>
inline void serialize(Archive &ar, ::boost::intrusive_ptr<T> &t, const uint32_t file_version) {
  split_free(ar, t, file_version);
}
} // namespace serialization
} // namespace boost

namespace boost {
namespace lambda {
namespace detail {

template <class A> struct contentsof_type<boost::intrusive_ptr<A>> { typedef A &type; };

} // namespace detail
} // namespace lambda
} // namespace boost

#endif // FLEXUS_CORE_BOOST_EXTENSIONS_INTRUSIVE_PTR_HPP_INCLUDED
