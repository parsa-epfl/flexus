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
#ifndef FLEXUS_CORE_AUX__REGISTRATION_DRIVE_LIST_HPP_INCLUDED
#define FLEXUS_CORE_AUX__REGISTRATION_DRIVE_LIST_HPP_INCLUDED

namespace Flexus {
namespace Core {
namespace Detail {

#define DRIVE_STEP0                                                                                                    \
    typedef                                                                                                            \
      typename mpl::push_front<base,                                                                                   \
                               DriveHandle<typename DriveListIter::type, ComponentHandle, ComponentHandle::tag>>::type \
        step1 /**/

#define DRIVE_STEP(N)                                                                                                  \
    typedef typename mpl::push_front<                                                                                  \
      BOOST_PP_CAT(step, N),                                                                                           \
      DriveHandle<typename mpl::advance_c<DriveListIter, N>::type::type, ComponentHandle, ComponentHandle::tag>>::type \
    BOOST_PP_CAT(step, BOOST_PP_INC(N)) /**/

template<int32_t N, class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step
{
    typedef typename make_drive_list__drive_step<N - 5,
                                                 typename mpl::advance_c<DriveListIter, 5>::type,
                                                 ComponentHandle,
                                                 Base>::type base;
    DRIVE_STEP0;
    DRIVE_STEP(1);
    DRIVE_STEP(2);
    DRIVE_STEP(3);
    DRIVE_STEP(4);
    typedef step5 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<5, DriveListIter, ComponentHandle, Base>
{
    typedef Base base;
    DRIVE_STEP0;
    DRIVE_STEP(1);
    DRIVE_STEP(2);
    DRIVE_STEP(3);
    DRIVE_STEP(4);
    typedef step5 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<4, DriveListIter, ComponentHandle, Base>
{
    typedef Base base;
    DRIVE_STEP0;
    DRIVE_STEP(1);
    DRIVE_STEP(2);
    DRIVE_STEP(3);
    typedef step4 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<3, DriveListIter, ComponentHandle, Base>
{
    typedef Base base;
    DRIVE_STEP0;
    DRIVE_STEP(1);
    DRIVE_STEP(2);
    typedef step3 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<2, DriveListIter, ComponentHandle, Base>
{
    typedef Base base;
    DRIVE_STEP0;
    DRIVE_STEP(1);
    typedef step2 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<1, DriveListIter, ComponentHandle, Base>
{
    typedef Base base;
    DRIVE_STEP0;
    typedef step1 type;
};

template<class DriveListIter, class ComponentHandle, class Base>
struct make_drive_list__drive_step<0, DriveListIter, ComponentHandle, Base>
{
    typedef Base type;
};

#undef DRIVE_STEP0
#undef DRIVE_STEP

#define COMPONENT_STEP0                                                                                                \
    typedef typename make_drive_list__drive_step<                                                                      \
      mpl::size<typename mpl::deref<ComponentListIter>::type::component::DriveInterfaces>::value,                      \
      typename mpl::begin<typename mpl::deref<ComponentListIter>::type::component::DriveInterfaces>::type,             \
      typename mpl::deref<ComponentListIter>::type,                                                                    \
      base>::type step1 /**/

#define COMPONENT_STEP(N)                                                                                              \
    typedef typename make_drive_list__drive_step<                                                                      \
      mpl::size<typename mpl::deref<                                                                                   \
        typename mpl::advance_c<ComponentListIter, N>::type>::type::component::DriveInterfaces>::value,                \
      typename mpl::begin<typename mpl::deref<                                                                         \
        typename mpl::advance_c<ComponentListIter, N>::type>::type::component::DriveInterfaces>::type,                 \
      typename mpl::deref<typename mpl::advance_c<ComponentListIter, N>::type>::type,                                  \
      BOOST_PP_CAT(step, N)>::type                                                                                     \
    BOOST_PP_CAT(step, BOOST_PP_INC(N)) /**/

template<int32_t N, class ComponentListIter>
struct make_drive_list__component_step
{
    typedef
      typename make_drive_list__component_step<N - 5, typename mpl::advance_c<ComponentListIter, 5>::type>::type base;

    COMPONENT_STEP0;
    COMPONENT_STEP(1);
    COMPONENT_STEP(2);
    COMPONENT_STEP(3);
    COMPONENT_STEP(4);
    typedef step5 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<5, ComponentListIter>
{
    typedef mpl::vector<> base;
    COMPONENT_STEP0;
    COMPONENT_STEP(1);
    COMPONENT_STEP(2);
    COMPONENT_STEP(3);
    COMPONENT_STEP(4);
    typedef step5 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<4, ComponentListIter>
{
    typedef mpl::vector<> base;
    COMPONENT_STEP0;
    COMPONENT_STEP(1);
    COMPONENT_STEP(2);
    COMPONENT_STEP(3);
    typedef step4 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<3, ComponentListIter>
{
    typedef mpl::vector<> base;
    COMPONENT_STEP0;
    COMPONENT_STEP(1);
    COMPONENT_STEP(2);
    typedef step3 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<2, ComponentListIter>
{
    typedef mpl::vector<> base;
    COMPONENT_STEP0;
    COMPONENT_STEP(1);
    typedef step2 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<1, ComponentListIter>
{
    typedef mpl::vector<> base;
    COMPONENT_STEP0;
    typedef step1 type;
};

template<class ComponentListIter>
struct make_drive_list__component_step<0, ComponentListIter>
{
    typedef mpl::vector<> type;
};

#undef COMPONENT_STEP0
#undef COMPONENT_STEP

template<class ComponentList>
struct make_drive_list
  : public make_drive_list__component_step<mpl::size<ComponentList>::value, typename mpl::begin<ComponentList>::type>
{};

} // namespace Detail
} // namespace Core
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__REGISTRATION_DRIVE_LIST_HPP_INCLUDED
