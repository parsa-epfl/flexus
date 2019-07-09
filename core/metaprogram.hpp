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
