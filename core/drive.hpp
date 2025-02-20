#ifndef FLEXUS_DRIVE_HPP_INCLUDED
#define FLEXUS_DRIVE_HPP_INCLUDED

#include <boost/mpl/deref.hpp>
#include <core/drive_reference.hpp>
#include <core/performance/profile.hpp>

namespace Flexus {
namespace Core {

namespace aux_ {

template<int32_t N, class DriveHandleIter>
struct do_cycle_core
{
    static void doCycle(index_t idx)
    {
        DBG_(VVerb, (<< "[Core] Drive component ID: " << N << " core idx: " << idx));
        mpl::deref<DriveHandleIter>::type::getReference(idx).drive(
            typename mpl::deref<DriveHandleIter>::type::drive());

        do_cycle_core<N - 1, typename mpl::next<DriveHandleIter>::type>::doCycle(idx);
    }
};

template<class DriveHandleIter>
struct do_cycle_core<0, DriveHandleIter>
{
    static void doCycle(index_t idx) {}
};

template<int32_t N, class DriveHandleIter>
struct do_cycle_uncore
{
    static void doCycle()
    {
        {
            FLEXUS_PROFILE_N(mpl::deref<DriveHandleIter>::type::drive::name());
            for (index_t i = 0; i < mpl::deref<DriveHandleIter>::type::width(); i++) {
                DBG_(VVerb, (<< "[Uncore] Drive Component ID: " << N << " uncore idx: " << i));
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

        index_t* freq = ComponentManager::getComponentManager().getFreq().freq;
        index_t maxFreq = ComponentManager::getComponentManager().getFreq().maxFreq;
        index_t sysWidth = ComponentManager::getComponentManager().systemWidth();

        for(index_t iter = 0; iter < maxFreq; ++iter) {
            for(index_t id = 0; id <= sysWidth; ++id) {
                if(iter < freq[id]) {
                    if(id == sysWidth) {
                        do_cycle_uncore<mpl::size<uncoreDriveHandles>::value, typename mpl::begin<uncoreDriveHandles>::type>::doCycle();
                    } else {
                        do_cycle_core<mpl::size<coreDriveHandles>::value, typename mpl::begin<coreDriveHandles>::type>::doCycle(id);
                    }
                }
            }
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
