#ifndef FLEXUS_DRIVE_HPP_INCLUDED
#define FLEXUS_DRIVE_HPP_INCLUDED


#include <core/metaprogram.hpp>
#include <boost/mpl/deref.hpp>
#include <core/drive_reference.hpp>
#include <core/performance/profile.hpp>


namespace Flexus {
namespace Core {

namespace aux_ {

template <int32_t N, class DriveHandleIter>
struct do_cycle_step {
  static void doCycle() {
    {
      FLEXUS_PROFILE_N( mpl::deref<DriveHandleIter>::type::drive::name() );
      for (index_t i = 0; i < mpl::deref<DriveHandleIter>::type::width(); i++) {
        mpl::deref<DriveHandleIter>::type::getReference(i).drive( typename mpl::deref<DriveHandleIter>::type::drive () );
      }
    }
    do_cycle_step < N - 1, typename mpl::next<DriveHandleIter>::type >::doCycle();
  }
};

template <class DriveHandleIter>
struct do_cycle_step<0, DriveHandleIter> {
  static void doCycle() { }
};

template <class DriveHandles>
struct do_cycle {
  static void doCycle() {
    do_cycle_step< mpl::size<DriveHandles>::value, typename mpl::begin<DriveHandles>::type>::doCycle();
  }
};
}

namespace aux_ {
template <int32_t N, class DriveHandleIter>
struct list_drives_step {
  static void listDrives() {
    DBG_( Dev, ( << mpl::deref<DriveHandleIter>::type::drive::name() ) );
    list_drives_step < N - 1, typename mpl::next<DriveHandleIter>::type >::listDrives();
  }
};

template <class DriveHandleIter>
struct list_drives_step<0, DriveHandleIter> {
  static void listDrives() { }
};

template <class DriveHandles>
struct list_drives {
  static void listDrives() {
    DBG_( Dev, ( << "Ordered list of DriveHandles follows:" )) ;
    list_drives_step< mpl::size<DriveHandles>::value, typename mpl::begin<DriveHandles>::type>::listDrives();
  }
};
}

template < class OrderedDriveHandleList >
class Drive : public DriveBase {
  virtual void doCycle() {
    //Through the magic of template expansion and static dispatch, this calls every Drive's
    //do_cycle() method in the order specified in OrderedDriveHandleList.
    FLEXUS_PROFILE_N("Drive::doCycle");
    aux_::do_cycle<OrderedDriveHandleList>::doCycle();
  }

};

} //End Namespace Core
} //namespace Flexus

#endif //FLEXUS_DRIVE_HPP_INCLUDE
