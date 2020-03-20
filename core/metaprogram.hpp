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
#ifndef FLEXUS_METAPROGRAM_HPP_INCLUDED
#define FLEXUS_METAPROGRAM_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>

#include <boost/mpl/size.hpp>
namespace mpl = boost::mpl;

#ifdef BOOST_MPL_LIMIT_VECTOR_SIZE
#if BOOST_MPL_LIMIT_VECTOR_SIZE < 50
#error "MPL List size must be set to 50 for Wiring to work"
#endif
#else
#define BOOST_MPL_LIMIT_VECTOR_SIZE 50
#endif

//#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#include <boost/mpl/vector.hpp>

#undef BOOST_MPL_NO_PREPROCESSED_HEADERS

/*

  #define FLEXUS_DECLARE_TAG_TYPE( Name, Value ) \
    typedef char (& Name) [ Value ]

  #define FLEXUS_TAG_TYPE( Value ) \
    char (&) [ Value ]

  #define FLEXUS_LOOKUP_TAG( LookupFn, ...) \
    sizeof( LookupFn( static_cast< __VA_ARGS__ *>(0) ) )

  #define FLEXUS_DECLARE_MEMBER_TYPE_TEST( MemberType ) \
    template <typename T> boost::type_traits::yes_type
BOOST_PP_CAT(member_type_test_,MemberType) ( typename T::MemberType const *); \
    template <typename T> boost::type_traits::no_type
BOOST_PP_CAT(member_type_test_,MemberType) (...); \
    template <class T> struct
BOOST_PP_CAT(check_class_has_member_type_,MemberType) { \
          static const bool value = (sizeof(
BOOST_PP_CAT(member_type_test_,MemberType)<T>(0)) ==
sizeof(boost::type_traits::yes_type)); \
    }

  #define FLEXUS_CLASS_HAS_MEMBER_TYPE(MemberType,Type) \
    BOOST_PP_CAT(check_class_has_member_type_,MemberType)<Type>::value

namespace Flexus {
namespace Core {

    template <int32_t N>
    struct int_{};

    static const int32_t null_tag_id = 1;
    FLEXUS_DECLARE_TAG_TYPE( null_tag, null_tag_id ) ;

    static const int32_t false_tag_id = 1;
    FLEXUS_DECLARE_TAG_TYPE( false_tag, false_tag_id ) ;

    static const int32_t true_tag_id = 2;
    FLEXUS_DECLARE_TAG_TYPE( true_tag, true_tag_id ) ;

} //Flexus
} //Core
*/

#endif // FLEXUS_METAPROGRAM_HPP_INCLUDED
