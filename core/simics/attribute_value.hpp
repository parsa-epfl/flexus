#ifndef FLEXUS_SIMICS_ATTRIBUTE_VALUE_HPP_INCLUDED
#define FLEXUS_SIMICS_ATTRIBUTE_VALUE_HPP_INCLUDED

#include <string>

#include <core/debug/debug.hpp>

#include <core/metaprogram.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/for_each.hpp>
namespace mpl = boost::mpl;

#include <core/exception.hpp>
#include <core/simics/api_wrappers.hpp>

namespace Flexus {
namespace Simics {

namespace aux_ {
//Forward declare AttributeFriend for the friend declaration in AttributeValue
class AttributeFriend;
struct Object {};
struct BuiltIn {};
struct construct_tag {};
} //namespace aux_

struct Nil {};

class AttributeValue {
  API::attr_value_t theSimicsValue;

  friend class aux_::AttributeFriend;
public:
  AttributeValue() {
    theSimicsValue.kind = API::Sim_Val_Invalid;
  }

  AttributeValue(API::attr_value_t const & anAttrValue) {
    theSimicsValue = anAttrValue;
  }

  AttributeValue(int anInteger) {
    theSimicsValue.u.integer = anInteger;
    theSimicsValue.kind = API::Sim_Val_Integer;
  }

  AttributeValue(long long anInteger) {
    theSimicsValue.u.integer = anInteger;
    theSimicsValue.kind = API::Sim_Val_Integer;
  }

  AttributeValue(double aFloating) {
    theSimicsValue.u.floating = aFloating;
    theSimicsValue.kind = API::Sim_Val_Floating;
  }

  AttributeValue(const char * aString) {
    theSimicsValue.u.string = aString;
    theSimicsValue.kind = API::Sim_Val_String;
  }

  AttributeValue(API::conf_object_t * aConfObject) {
    theSimicsValue.u.object = aConfObject;
    theSimicsValue.kind = API::Sim_Val_Object;
  }

  AttributeValue(API::attr_data_t const & aData) {
    theSimicsValue.u.data = aData;
    theSimicsValue.kind = API::Sim_Val_Data;
  }

  AttributeValue(Nil const &) {
    theSimicsValue.kind = API::Sim_Val_Nil;
  }

  //Default copy constructor and copy assignment operators

  bool isInvalid() const {
    return theSimicsValue.kind == API::Sim_Val_Invalid;
  }
  bool isString() const {
    return theSimicsValue.kind == API::Sim_Val_String;
  }
  bool isInteger() const {
    return theSimicsValue.kind == API::Sim_Val_Integer;
  }
  bool isFloating() const {
    return theSimicsValue.kind == API::Sim_Val_Floating;
  }
  bool isList() const {
    return theSimicsValue.kind == API::Sim_Val_List;
  }
  bool isData() const {
    return theSimicsValue.kind == API::Sim_Val_Data;
  }
  bool isNil() const {
    return theSimicsValue.kind == API::Sim_Val_Nil;
  }
  bool isObject() const {
    return theSimicsValue.kind == API::Sim_Val_Object;
  }

  AttributeValue & operator = (API::attr_value_t const & anAttrValue) {
    theSimicsValue = anAttrValue;
    return *this;
  }

  AttributeValue & operator = ( long long anInteger) {
    theSimicsValue.u.integer = anInteger;
    theSimicsValue.kind = API::Sim_Val_Integer;
    return *this;
  }

  AttributeValue & operator = ( double aFloating) {
    theSimicsValue.u.floating = aFloating;
    theSimicsValue.kind = API::Sim_Val_Floating;
    return *this;
  }

  AttributeValue & operator = ( const char * aString) {
    theSimicsValue.u.string = aString;
    theSimicsValue.kind = API::Sim_Val_String;
    return *this;
  }

  AttributeValue & operator = ( API::conf_object_t * anObject) {
    theSimicsValue.u.object = anObject;
    theSimicsValue.kind = API::Sim_Val_Object;
    return *this;
  }

  AttributeValue & operator = ( API::attr_data_t const & aData) {
    theSimicsValue.u.data = aData;
    theSimicsValue.kind = API::Sim_Val_Data;
    return *this;
  }

  AttributeValue & operator = ( Nil const & ) {
    theSimicsValue.kind = API::Sim_Val_Nil;
    return *this;
  }

  operator API::attr_value_t() {
    return theSimicsValue;
  }

};

class AttributeList {
  API::attr_list_t theList;

public:
  AttributeList() {
    theList.size = 0;
    theList.vector = 0;
  }

  //Give attribute list random access iterators
};

class AttributeData {
  API::attr_data_t theData;

public:
  AttributeData() {
    theData.size = 0;
    theData.data = 0;
  }

  long long size() const {
    return theData.size;
  }
  operator unsigned char * () {
    return theData.data;
  }
  operator unsigned char const * () const {
    return theData.data;
  }
};

class bad_attribute_cast : public SimicsException {};

namespace aux_ {
//This helper class lets us leave the contents of AttributeValue private,
//but let the casting code get at it.
struct AttributeFriend {
  static API::attr_kind_t getKind(AttributeValue const & anAttribute) {
    return anAttribute.theSimicsValue.kind;
  }
  template<class T>
  static T get(AttributeValue const & anAttribute);
};
template <> inline long long  AttributeFriend::get<long long>(AttributeValue const & anAttribute) {
  return anAttribute.theSimicsValue.u.integer;
}
template <> inline double AttributeFriend::get<double>(AttributeValue const & anAttribute) {
  return anAttribute.theSimicsValue.u.floating;
}
template <> inline char const * AttributeFriend::get<char const *>(AttributeValue const & anAttribute) {
  return anAttribute.theSimicsValue.u.string;
}
template <> inline API::conf_object_t * AttributeFriend::get<API::conf_object_t *>(AttributeValue const & anAttribute) {
  return anAttribute.theSimicsValue.u.object;
}

//These tag types are used to indicate what kind of type was asked for
//in an attribute_cast
struct integral_tag {};
struct floating_tag {};
struct string_tag {};
struct char_const_ptr_tag {};
struct object_tag {};
struct invalid_tag {};

//Here are the implemenations for each of the different casts that
//attribute_cast allows.
template <typename DestType, typename DestTypeTag>
struct AttributeCastImpl;

//Implementation for Integral types
template <typename DestType>
struct AttributeCastImpl<DestType, integral_tag> {
  static DestType cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_Integer) {
      throw bad_attribute_cast();
    }
    return AttributeFriend::get<long long>(anAttribute);
  }
};

//Implementation for Floating types
template <typename DestType>
struct AttributeCastImpl<DestType, floating_tag> {
  static DestType cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_Floating) {
      throw bad_attribute_cast();
    }
    return AttributeFriend::get<double>(anAttribute);
  }
};

//Implementation for String
template <typename DestType>
struct AttributeCastImpl<DestType, string_tag> {
  static std::string cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_String) {
      throw bad_attribute_cast();
    }
    return std::string(AttributeFriend::get<char const *>(anAttribute));
  }
};

//Implementation for C-style String
template <typename DestType>
struct AttributeCastImpl<DestType, char_const_ptr_tag> {
  static char const * cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_String) {
      throw bad_attribute_cast();
    }
    return AttributeFriend::get<char const *>(anAttribute);
  }
};

//Implementation to get back the conf_object_t * of a Simics object
template <>
struct AttributeCastImpl<API::conf_object_t *, object_tag> {
  static API::conf_object_t * cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_Object) {
      throw bad_attribute_cast();
    }
    return AttributeFriend::get<API::conf_object_t *>(anAttribute);
  }
};

//Figure out what kind of type T is
template <class T>
struct get_type_tag {
  typedef typename
  mpl::eval_if
  < boost::is_integral<T>
  , mpl::identity<integral_tag>
  , mpl::eval_if
  < boost::is_float<T>
  , mpl::identity<floating_tag>
  , mpl::eval_if
  < boost::is_same< typename boost::remove_cv<T>::type, std::string >
  , mpl::identity<string_tag>
  , mpl::eval_if
  < boost::is_same<T, char const *>
  , mpl::identity<char_const_ptr_tag>
  , mpl::eval_if
  < boost::is_base_and_derived<Object, T>
  , mpl::identity<object_tag>
  , mpl::identity<invalid_tag>
  >
  >
  >
  >
  >::type tag;
};
}  //End aux_

//The actual attribute_cast template
template <typename DestType>
typename boost::remove_reference<DestType>::type attribute_cast(AttributeValue const & anAttribute) {
  return aux_::AttributeCastImpl <
         typename boost::remove_reference<DestType>::type,
         typename aux_::get_type_tag< typename boost::remove_reference<DestType>::type >::tag
         >::cast(anAttribute);
}

} //namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_ATTRIBUTE_VALUE_HPP_INCLUDED
