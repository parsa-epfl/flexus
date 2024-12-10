#ifndef FLEXUS_DRIVE_HPP_INCLUDED
#define FLEXUS_DRIVE_HPP_INCLUDED

#include <boost/mpl/deref.hpp>
#include <core/drive_reference.hpp>
#include <core/performance/profile.hpp>

namespace Flexus {
namespace Core {

namespace aux_ {

template<int32_t N, class DriveHandleIter>
struct do_cycle_step
{
    static void doCycle(index_t idx)
    {
        DBG_(Dev, (<< "Drive: " << N << " idx: " << idx));
        mpl::deref<DriveHandleIter>::type::getReference(idx).drive(
            typename mpl::deref<DriveHandleIter>::type::drive());
        
        do_cycle_step<N - 1, typename mpl::next<DriveHandleIter>::type>::doCycle(idx);
    }
};

template<class DriveHandleIter>
struct do_cycle_step<0, DriveHandleIter>
{
    static void doCycle(index_t idx) {}
};

template<class DriveHandles>
struct do_cycle_core
{
    static void doCycle()
    {
        const index_t syswidth = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
        // Note: this assumes all components (eg, fetch, decoder, etc) have the same number of instances = syswidth
        for(index_t sysIdx = 0; sysIdx < syswidth; ++sysIdx) {
            do_cycle_step<mpl::size<DriveHandles>::value, typename mpl::begin<DriveHandles>::type>::doCycle(sysIdx);
        }
    }
};

template<int32_t N, class DriveHandleIter>
struct do_cycle_uncore
{
    static void doCycle()
    {
        {
            FLEXUS_PROFILE_N(mpl::deref<DriveHandleIter>::type::drive::name());
            for (index_t i = 0; i < mpl::deref<DriveHandleIter>::type::width(); i++) {
                mpl::deref<DriveHandleIter>::type::getReference(i).drive(
                  typename mpl::deref<DriveHandleIter>::type::drive());
            }
        }
        do_cycle_uncore<N - 1, typename mpl::next<DriveHandleIter>::type>::doCycle();
    }
};

template<class DriveHandleIter>
struct do_cycle_uncore<0, DriveHandleIter>
{
    static void doCycle() {}
};

template<class DriveHandles>
struct do_cycle
{
    static void doCycle()
    {
        typedef typename mpl::deref<typename mpl::begin<DriveHandles>::type>::type coreDriveHandles;
        typedef typename mpl::deref<typename mpl::next<typename mpl::begin<DriveHandles>::type>::type>::type uncoreDriveHandles;

        const Flexus::Core::freq_opts freq = ComponentManager::getComponentManager().getFreq();
        index_t minFreq = freq.core < freq.uncore ? freq.core : freq.uncore;
        index_t diffCore = freq.core - minFreq;
        index_t diffUncore = freq.uncore - minFreq;

        for(index_t iter = 0; iter < minFreq; ++iter) {
            do_cycle_core<coreDriveHandles>::doCycle();
            do_cycle_uncore<mpl::size<uncoreDriveHandles>::value, typename mpl::begin<uncoreDriveHandles>::type>::doCycle();
        }

        for(index_t iter = 0; iter < diffCore; ++iter) {
            do_cycle_core<coreDriveHandles>::doCycle();
        }

        for(index_t iter = 0; iter < diffUncore; ++iter) {
            do_cycle_uncore<mpl::size<uncoreDriveHandles>::value, typename mpl::begin<uncoreDriveHandles>::type>::doCycle();
        }
    }
};
} // namespace aux_

namespace aux_ {
template<int32_t N, class DriveHandleIter>
struct list_drives_step
{
    static void listDrives()
    {
        DBG_(Dev, (<< mpl::deref<DriveHandleIter>::type::drive::name()));
        list_drives_step<N - 1, typename mpl::next<DriveHandleIter>::type>::listDrives();
    }
};

template<class DriveHandleIter>
struct list_drives_step<0, DriveHandleIter>
{
    static void listDrives() {}
};

template<class DriveHandles>
struct list_drives
{
    static void listDrives()
    {
        DBG_(Dev, (<< "Ordered list of DriveHandles follows:"));
        list_drives_step<mpl::size<DriveHandles>::value, typename mpl::begin<DriveHandles>::type>::listDrives();
    }
};
} // namespace aux_

template<class OrderedDriveHandleList>
class Drive : public DriveBase
{
    virtual void doCycle()
    {
        // Through the magic of template expansion and static dispatch, this calls
        // every Drive's do_cycle() method in the order specified in
        // OrderedDriveHandleList.
        FLEXUS_PROFILE_N("Drive::doCycle");
        aux_::do_cycle<OrderedDriveHandleList>::doCycle();
    }
};

} // End Namespace Core
} // namespace Flexus

#endif // FLEXUS_DRIVE_HPP_INCLUDE
