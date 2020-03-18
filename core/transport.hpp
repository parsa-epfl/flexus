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
#ifndef FLEXUS_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORT_HPP_INCLUDED

#include <boost/mpl/contains.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <core/metaprogram.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

namespace Flexus {
namespace Core {

template <class Tag, class Slice> struct transport_entry {
  typedef Tag tag;
  typedef Slice slice;
};

namespace aux_ {
struct extract_tag {
  template <class Slice> struct apply;

  template <class Tag, class Data> struct apply<transport_entry<Tag, Data>> { typedef Tag type; };
};

template <class Transport, class OtherTransport> struct transport_copier {
  Transport &theDest;
  OtherTransport &theSrc;

  transport_copier(Transport &aDest, OtherTransport &aSrc) : theDest(aDest), theSrc(aSrc) {
  }

  template <class Slice, bool IsMatch> struct copy {
    void operator()(Transport &theDest, OtherTransport &theSrc) {
    }
  };

  template <class Slice> struct copy<Slice, true> {
    void operator()(Transport &theDest, OtherTransport &theSrc) {
      theDest.set(Slice::tag(), theSrc[Slice::tag()]);
    }
  };

  template <class Slice> void operator()(Slice const &) {
    copy<Slice, mpl::contains<Transport, typename Slice::tag>::type::value>()(theDest, theSrc);
  }
};

// This template function does the copy.  As an added bonus, it can deduce its
// template argument types, so its easy to call.
template <class Transport, class OtherTransport>
void transport_copy(Transport &aDest, OtherTransport &aSrc) {
  transport_copier<Transport, OtherTransport> copier(aDest, aSrc);
  mpl::for_each<typename mpl::transform<typename OtherTransport::slice_vector, extract_tag>>(
      copier);
}
} // namespace aux_

namespace aux_ {
template <int32_t N, class SliceIter>
class TransportSlice : public TransportSlice<N - 1, typename mpl::next<SliceIter>::type> {
  typedef TransportSlice<N - 1, typename mpl::next<SliceIter>::type> base;
  typedef typename mpl::deref<SliceIter>::type::tag tag;
  typedef typename mpl::deref<SliceIter>::type::slice data;
  boost::intrusive_ptr<data> theObject;

public:
  using base::operator[];
  using base::set;
  boost::intrusive_ptr<data> operator[](const tag &) {
    return theObject;
  }
  boost::intrusive_ptr<const data> operator[](const tag &) const {
    return theObject;
  }
  void set(const tag &, boost::intrusive_ptr<data> anObject) {
    theObject = anObject;
  }
};

template <class SliceIter> class TransportSlice<0, SliceIter> {
public:
  struct Unique {};
  // Ensure that using declarations compile
  int32_t operator[](const Unique &);
  void set(const Unique &);
};
} // namespace aux_

template <class SliceVector>
struct Transport : public aux_::TransportSlice<mpl::size<SliceVector>::value,
                                               typename mpl::begin<SliceVector>::type> {
  typedef aux_::TransportSlice<mpl::size<SliceVector>::value,
                               typename mpl::begin<SliceVector>::type>
      base;

public:
  typedef SliceVector slice_vector;

  // Default constructor - default construct all contained pointers
  Transport() {
  }

  ~Transport() {
  }

  using base::operator[];
  using base::set;

  // Copy constructor for a non-matching typelist
  template <class OtherSlices> Transport(Transport<OtherSlices> const &anOtherTransport) {
    // Note: On entry to the constructor, all the members of this transport
    // have been default constructed, and are therefore null
    aux_::transport_copy(*this, anOtherTransport);
  }
};

} // namespace Core
} // namespace Flexus

namespace Flexus {
namespace SharedTypes {

using Flexus::Core::Transport;
using Flexus::Core::transport_entry;

} // namespace SharedTypes
} // namespace Flexus

#endif // FLEXUS_TRANSPORT_HPP_INCLUDED
