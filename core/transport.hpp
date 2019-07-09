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
