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
#ifndef FLEXUS_DRIVE_HPP_INCLUDED
#define FLEXUS_DRIVE_HPP_INCLUDED

#include <boost/mpl/deref.hpp>
#include <core/drive_reference.hpp>
#include <core/metaprogram.hpp>
#include <core/performance/profile.hpp>

namespace Flexus {
namespace Core {

namespace aux_ {

template<int32_t N, class DriveHandleIter>
struct do_cycle_step
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
        do_cycle_step<N - 1, typename mpl::next<DriveHandleIter>::type>::doCycle();
    }
};

template<class DriveHandleIter>
struct do_cycle_step<0, DriveHandleIter>
{
    static void doCycle() {}
};

template<class DriveHandles>
struct do_cycle
{
    static void doCycle()
    {
        do_cycle_step<mpl::size<DriveHandles>::value, typename mpl::begin<DriveHandles>::type>::doCycle();
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
