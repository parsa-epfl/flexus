#ifndef FLEXUS_SIMICS_CONFIGURATION_API_HPP_INCLUDED
#define FLEXUS_SIMICS_CONFIGURATION_API_HPP_INCLUDED

#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include <core/debug/debug.hpp>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

#include <core/boost_extensions/member_function_traits.hpp>
#include <boost/bind.hpp>

#include <core/simics/trampoline.hpp>
#include <core/exception.hpp>
#include <core/simics/api_wrappers.hpp>
#include <core/simics/attribute_value.hpp>
#include <core/simics/aux_/simics_command_manager.hpp>

extern "C" {
  //Typedef within simics removed in Simics 2.0
  typedef Flexus::Simics::API::conf_object_t * (*new_instance_t)(Flexus::Simics::API::parse_object_t *);
  typedef int (*delete_instance_t)(Flexus::Simics::API::conf_object_t *);
}

namespace Flexus {
namespace Simics {

void runPython(std::vector<std::string> & aCommandVector);

namespace aux_ {
//Stub function declarations which assist in making this code behave properly
//both with and without simics
API::conf_object_t * NewObject_stub(API::conf_class_t * aClass, std::string const & aName, API::conf_object_t * (&constructor)(API::parse_object_t *) );
} //namespace aux_

enum Persistence {
  Persistent = API::Sim_Class_Kind_Vanilla,
  Session = API::Sim_Class_Kind_Session,
  Transient = API::Sim_Class_Kind_Pseudo
};

template <class ObjectClassImpl>
class BuiltInObject : public aux_::Object, public aux_::BuiltIn {
protected:
  ObjectClassImpl theImpl;
public:
  typedef ObjectClassImpl implementation_class;

  BuiltInObject () : theImpl(0) {}
  explicit BuiltInObject (ObjectClassImpl & anImpl) : theImpl(anImpl) {}
  explicit BuiltInObject (API::conf_object_t * anObject) : theImpl(anObject) {}

  //This overloaded address-of operator is used internally by the
  //configuration-api implementation
  ObjectClassImpl * operator&() {
    return &theImpl;
  }

  //These forwarding functions make the wrapper class behave like a
  //pointer to the simics object
  ObjectClassImpl * operator->() {
    return &theImpl;
  }
  ObjectClassImpl & operator *() {
    return theImpl;
  }
  ObjectClassImpl const * operator->() const {
    return &theImpl;
  }
  ObjectClassImpl const  & operator *() const {
    return theImpl;
  }
  //operator bool const () const {
  //  return theImpl.isNull();
  //}

  //defineClass does nothing for built-in classes, since they are defined
  //by Simics
  template <class Class>
  static void defineClass(Class & aClass) {}
};

template <class ObjectClassImpl>
class AddInObject : public aux_::Object {
  ObjectClassImpl * theImpl;
public:
  typedef ObjectClassImpl implementation_class;

  AddInObject() : theImpl(0) {}
  explicit AddInObject(API::conf_object_t * aSimicsObject) : theImpl(new ObjectClassImpl(aSimicsObject)) {}
  explicit AddInObject(ObjectClassImpl * anImpl) : theImpl(anImpl) {}

  //This overloaded address-of operator is used internally by the
  //configuration-api implementation
  ObjectClassImpl * operator&() {
    return theImpl;
  }

  //These forwarding functions make the wrapper class behave like a
  //pointer to the simics object
  ObjectClassImpl * operator->() {
    return theImpl;
  }
  ObjectClassImpl & operator *() {
    return *theImpl;
  }
  ObjectClassImpl const * operator->() const {
    return theImpl;
  }
  ObjectClassImpl const  & operator *() const {
    return *theImpl;
  }
  operator bool const () const {
    return theImpl;
  }

  //Unlike built-in objects, add-in objects must provide a destruct
  //which simics can call.  This deletes theImpl, which will invoke
  //the destructor of theImpl to do any cleanup that the implementation
  //object requires.
  void destruct() {
    delete theImpl;
  }

  //The default defineClass() implementation does nothing.  This is
  //overridden in order to add attributes or commands to a class.
  template <class Class>
  static void defineClass(Class & aClass) {}
};

namespace aux_ {
//This tag is used to choose between the "registering" and "non-registering"
//constructor for Addin classes
struct no_class_registration_tag {};

//trait template for determining of CppObjectClass represents a Simics Builtin class
template <class CppObjectClass>
struct is_builtin {
  static const bool value = boost::is_base_and_derived<BuiltIn, CppObjectClass>::value;
};

//This class collects functionality common to both the Builtin and Addin
//implementations of Class.  Also, it allows us to store pointers to
//Class objects as BaseClassImpl *, so we don't have to know what the
//CppObjectClass template argument is.
class BaseClassImpl : boost::noncopyable {
protected:
  API::conf_class_t * theSimicsClass; //Simics class data structure

public:
  //Default construction and destruction.
  API::conf_class_t /*const*/ * getSimicsClass() const {
    return theSimicsClass;
  }

  bool operator ==(const BaseClassImpl & aClass) {
    return theSimicsClass == aClass.theSimicsClass;
  }

  bool operator !=(const BaseClassImpl & aClass) {
    return !(*this == aClass);
  }
};

//Implementation of the Class type for Simics Builtin classes
template < class CppObjectClass, bool IsBuiltin /*true*/ >
class ClassImpl : public BaseClassImpl {
protected:
  //Class objects for built in classes, never have to register the class, since Simics
  //already knows about the class.  Thus, the "register" and "no-register" versions
  //of the constructor are the same.
  ClassImpl(no_class_registration_tag const & = no_class_registration_tag()) {
    if ((theSimicsClass = API::SIM_get_class(CppObjectClass::className())) == 0) {
      throw SimicsException(std::string("Simics does not have any class named:") + CppObjectClass::className());
    }
  }
public:
  //In order to cast a conf_object_t * to the corresponding C++ object, we simply
  //create a CppObjectClass to wrap the conf_object_t * and return it.  CppObjectClass
  //should behave like a pointer.  Note that CppObjectClass never owns the storage
  //pointed to by the conf_object_t * for builtin types.
  static CppObjectClass cast_object(API::conf_object_t * aSimicsObject) {
    return CppObjectClass(aSimicsObject);
  }
};

//Stuff used for registering attributes.  Hopefully will go away soon.
struct register_functor {
  BaseClassImpl & theBase;
  register_functor (BaseClassImpl & aBase) : theBase(aBase) {}
  template< typename U > void operator()(U x) {
    x.doRegister(theBase);
  }
};

template <class T, bool> struct register_helper {
  static void registerAttributes(BaseClassImpl & aClass) {}
};

template <class T> struct register_helper<T, true> {
  static void registerAttributes(BaseClassImpl & aClass) {
    mpl::for_each < typename T::Attributes >( register_functor(aClass) );
  }
};

typedef char YesType;
typedef struct {
  char a[2];
} NoType;

template<class U>
NoType hasAttributes_helper(...);
template<class U>
YesType hasAttributes_helper(typename U::Attributes const * );

template<class T>
struct hasAttributes {
  static const bool value = sizeof(hasAttributes_helper<T>(0)) == sizeof(YesType) ;
};

template <class MemberFunction, int Arity>
struct member_function_arity {
  static const bool value = (boost::member_function_traits<MemberFunction>::arity == Arity);
};

//Implementation of the Class type for Addin classes
template <class CppObjectClass>
class ClassImpl < CppObjectClass, false /* ! builtin */ > : public BaseClassImpl {

  //Note: the ordering of the elements in this struct is significant. conf_object
  //must come first, and this struct must be a POD, to guarantee that the
  //reinterpret_casts below are legal.
  typedef SimicsObject<CppObjectClass> simics_object;

public:

  template <class Function>
  typename boost::enable_if_c
  < member_function_arity<Function, 0>::value
  >::type
  addCommand( Function aFunction, std::string const & aCommand, std::string const & aDescription ) {
    DBG_(Iface, ( << " addCommand " << CppObjectClass::className() << '.' << aCommand << "()") );

    SimicsCommandManager * mgr = SimicsCommandManager::get();
    mgr->addCommand
    ( CppObjectClass::className()
      , aCommand
      , aDescription
      , boost::bind
      ( & invoke0< Function >
        , aFunction
        , _1
        , _2
        , _3
        , _4
      )
    );
  }

  template <class Function>
  typename boost::enable_if_c
  < member_function_arity<Function, 1>::value
  >::type
  addCommand( Function aFunction, std::string const & aCommand, std::string const & aDescription, std::string const & anArg1Name) {
    DBG_(Iface, ( << " addCommand " << CppObjectClass::className() << '.' << aCommand << '(' << anArg1Name << ')') );

    SimicsCommandManager * mgr = SimicsCommandManager::get();

    mgr->addCommand
    ( CppObjectClass::className()
      , aCommand
      , aDescription
      , boost::bind
      ( & invoke1< Function, typename boost::member_function_traits<Function>::arg1_type >
        , aFunction
        , aCommand
        , _1
        , _2
        , _3
        , _4
      )
      , anArg1Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg1_type>::value
    );
  }

  template <class Function>
  typename boost::enable_if_c
  < member_function_arity<Function, 2>::value
  >::type
  addCommand( Function aFunction, std::string const & aCommand, std::string const & aDescription, std::string const & anArg1Name, std::string const & anArg2Name) {
    DBG_(Iface, ( << " addCommand " << CppObjectClass::className() << '.' << aCommand << '(' << anArg1Name << ", " << anArg2Name << ')') );

    SimicsCommandManager * mgr = SimicsCommandManager::get();
    mgr->addCommand
    ( CppObjectClass::className()
      , aCommand
      , aDescription
      , boost::bind
      ( & invoke2< Function, typename boost::member_function_traits<Function>::arg1_type, typename boost::member_function_traits<Function>::arg2_type >
        , aFunction
        , aCommand
        , _1
        , _2
        , _3
        , _4
      )
      , anArg1Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg1_type>::value
      , anArg2Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg2_type>::value
    );
  }

  template <class Function>
  typename boost::enable_if_c
  < member_function_arity<Function, 3>::value
  >::type
  addCommand( Function aFunction, std::string const & aCommand, std::string const & aDescription, std::string const & anArg1Name, std::string const & anArg2Name, std::string const & anArg3Name) {

    DBG_(Iface, ( << " addCommand " << CppObjectClass::className() << '.' << aCommand << '(' << anArg1Name << ", " << anArg2Name << ", " << anArg3Name << ')') );

    SimicsCommandManager * mgr = SimicsCommandManager::get();
    mgr->addCommand
    ( CppObjectClass::className()
      , aCommand
      , aDescription
      , boost::bind
      ( & invoke3< Function, typename boost::member_function_traits<Function>::arg1_type, typename boost::member_function_traits<Function>::arg2_type, typename boost::member_function_traits<Function>::arg3_type >
        , aFunction
        , aCommand
        , _1
        , _2
        , _3
        , _4
      )
      , anArg1Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg1_type>::value
      , anArg2Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg2_type>::value
      , anArg3Name
      , aux_::SimicsCommandArgClass<typename boost::member_function_traits<Function>::arg3_type>::value
    );
  }

  template <class ImplementationFn>
  static void invoke0( ImplementationFn aFunction, API::conf_object_t * anObject, API::attr_value_t const & /* ignored */, API::attr_value_t const & /* ignored */, API::attr_value_t const & /* ignored */) {
    DBG_(Verb, ( << " invoke" << CppObjectClass::className() ) );
    ((*cast_object(anObject)) .* aFunction) ();
  }

  template <class ImplementationFn, class Arg1>
  static void invoke1( ImplementationFn aFunction, std::string aCommand, API::conf_object_t * anObject, API::attr_value_t const & anArg1, API::attr_value_t const & anArg2, API::attr_value_t const & /* ignored */) {
    DBG_(Verb, ( << " invoke " << CppObjectClass::className() << ":" << aCommand) );
    try {
      ((*cast_object(anObject)) .* aFunction) ( attribute_cast<Arg1>(anArg1)  );
    } catch (bad_attribute_cast c) {
      DBG_(Dev, ( << "Bad attribute cast." << anArg1.kind ));
      DBG_(Verb, ( << "Obj name" << anObject->name ));
      DBG_(Verb, ( << "Arg1.kind: " << anArg1.kind ));
    }
  }

  template <class ImplementationFn, class Arg1, class Arg2>
  static void invoke2( ImplementationFn aFunction, std::string aCommand, API::conf_object_t * anObject, API::attr_value_t const & anArg1, API::attr_value_t const & anArg2, API::attr_value_t const & /* ignored */) {
    DBG_(Verb, ( << " invoke " << CppObjectClass::className() << ":" << aCommand) );
    try {
      ((*cast_object(anObject)) .* aFunction) ( attribute_cast<Arg1>(anArg1), attribute_cast<Arg2>(anArg2));
    } catch (bad_attribute_cast c) {
      DBG_(Dev, ( << "Bad attribute cast." << anArg1.kind ));
      DBG_(Verb, ( << "Obj name" << anObject->name ));
      DBG_(Verb, ( << "Arg1.kind: " << anArg1.kind ));
      DBG_(Verb, ( << "Arg2.kind: " << anArg2.kind ));
    }
  }

  template <class ImplementationFn, class Arg1, class Arg2, class Arg3>
  static void invoke3( ImplementationFn aFunction, std::string aCommand, API::conf_object_t * anObject, API::attr_value_t const & anArg1, API::attr_value_t const & anArg2, API::attr_value_t const & anArg3) {
    DBG_(Verb, ( << " invoke " << CppObjectClass::className() << ":" << aCommand) );
    try {
      ((*cast_object(anObject)) .* aFunction) ( attribute_cast<Arg1>(anArg1), attribute_cast<Arg2>(anArg2), attribute_cast<Arg3>(anArg3));
    } catch (bad_attribute_cast c) {
      DBG_(Dev, ( << "Bad attribute cast." << anArg1.kind ));
      DBG_(Verb, ( << "Obj name" << anObject->name ));
      DBG_(Verb, ( << "Arg1.kind: " << anArg1.kind ));
      DBG_(Verb, ( << "Arg2.kind: " << anArg2.kind ));
      DBG_(Verb, ( << "Arg3.kind: " << anArg3.kind ));
    }
  }

  //Note: the reintepret_casts in these methods are safe since SimicsObject is POD and
  //conf_object comes first in SimicsObject.

  //This function is the constructor Simics will call to create objects of the type this class
  //represents.
  static API::conf_object_t * constructor(API::parse_object_t * aParseObject) {
    simics_object * object = new simics_object();
    //Note: the reintepret cast here is safe since SimicsObject is POD and
    //conf_object comes first in SimicsObject.
    APIFwd::SIM_object_constructor(reinterpret_cast<API::conf_object *>(object), aParseObject);
    object->theObject = CppObjectClass(reinterpret_cast<API::conf_object *>(object));
    return reinterpret_cast<API::conf_object *>(object);
  }

  //The corresponding destructor
  static int destructor(API::conf_object_t * anInstance) {
    simics_object * obj = reinterpret_cast<simics_object *>(anInstance);
    //Destruct the C++ object
    obj->theObject.destruct();
    //Free the memory for the shared object
    delete obj;
    return 0;
  }

  //Cast a bare conf_object_t * to the C++ object type
  static CppObjectClass cast_object(API::conf_object_t * aSimicsObject) {
    return reinterpret_cast<simics_object *>(aSimicsObject)->theObject;
  }

protected:
  //The no-argument constructor registers the Addin class represented by this object with Simics.
  ClassImpl() {
    API::class_data_t class_data;

    class_data.new_instance = &make_signature_from_free_fn<new_instance_t>::with<ClassImpl::constructor>::trampoline;
    class_data.delete_instance = &make_signature_from_free_fn<delete_instance_t>::with<ClassImpl::destructor>::trampoline;
    class_data.default_get_attr = 0;
    class_data.default_set_attr = 0;
    std::string descr = CppObjectClass::classDescription();
    class_data.description = descr.c_str();
    class_data.kind = static_cast<API::class_kind_t>(CppObjectClass::class_persistence );
    class_data.finalize_instance = 0;
#if SIM_VERSION < 1200
    //register_attr field was removed in Simics 1.2.x
    class_data.register_attr = 0;
#endif //SIM_VERSION < 1200
    class_data.parent = 0;

    //This function call registers a class when linked with Simics, and does nothing when linked standalone
    theSimicsClass = aux_::RegisterClass_stub(CppObjectClass::className(), &class_data);

    //Regsiter all the interfaces that this class implements

    //Register all the attributes of this class
    register_helper< CppObjectClass, hasAttributes<CppObjectClass>::value >::registerAttributes(*this);

    CppObjectClass::defineClass(*this);
  }

  //This constructor does not register the class.  Used when the class is already registered, by
  //some of the engine of attribute_cast<>
  ClassImpl(no_class_registration_tag const &) {
    if ((theSimicsClass = API::SIM_get_class(CppObjectClass::className().c_str())) == 0) {
      throw SimicsException(std::string("Simics does not have any class named:") + CppObjectClass::className());
    }
  }

};
}//End aux_

template <class CppObjectClass>
struct Class : public aux_::ClassImpl<CppObjectClass, aux_::is_builtin<CppObjectClass>::value > {
  typedef aux_::ClassImpl<CppObjectClass, aux_::is_builtin<CppObjectClass>::value > base;
  typedef CppObjectClass object_type;

  //Forwarding constructors
  Class() {}
  Class(aux_::no_class_registration_tag const & aTag ) : base( aTag ) {}
};

namespace aux_ {
//Implementation for Simics objects
template <typename DestType>
struct AttributeCastImpl<DestType, object_tag> {
  static DestType cast(const AttributeValue & anAttribute) {
    if (AttributeFriend::getKind(anAttribute) != API::Sim_Val_Object) {
      throw bad_attribute_cast();
    }
    API::conf_object_t * object = AttributeFriend::get<API::conf_object_t *>(anAttribute);
    Class<DestType> target = Class<DestType>(no_class_registration_tag());
    return target.cast_object(object);
  }
};
} //namespace aux_

//make_attribute helper.

#if 0
template <aux_::SimicsSetFnT SetFn, aux_::SimicsGetFnT GetFn>
struct AttributeImpl { /*: public BaseAttribute*/
  static void doRegister(Detail::BaseClassImpl & aClass, std::string anAttributeName, std::string anAttributeDescription) {
    APIFwd::SIM_register_attribute(aClass.getSimicsClass(), anAttributeName.c_str(), GetFn, 0, SetFn, 0, API::Sim_Attr_Session, anAttributeDescription.c_str());
  }

};

template <class T, typename R, R T::* DataMember >
API::attr_value_t SimicsGetAttribute(void * /*ignored*/, API::conf_object_t * anObject, API::attr_value_t * /*ignored*/) {
  return AttributeValue( &( (reinterpret_cast< SimicsObject<T> * > (anObject) )->theObject) ->* DataMember);
}

template <class T, typename R, R T::* DataMember >
API::set_error_t SimicsSetAttribute(void * /*ignored*/, API::conf_object_t * anObject, API::attr_value_t * aValue, API::attr_value_t * /*ignored*/) {
  try {
    (&( (reinterpret_cast< SimicsObject<T> * > (anObject) )->theObject) ->* DataMember) = attribute_cast<R>(*aValue);

    return API::Sim_Set_Ok;
  } catch (bad_attribute_cast & aBadCast) {
    return API::Sim_Set_Illegal_Value;  //Could return a more specific error code, ie which type is desired.
  }
}

template <class T, typename R, R T::* DataMember >
struct Base {};

template <class AttributeClass, class ImplementationClass, typename AttributeType, AttributeType ImplementationClass::* DataMember >
struct AttributeT
    : public AttributeImpl
    < & Simics::SimicsSetAttribute< ImplementationClass, AttributeType, DataMember >
    , & Simics::SimicsGetAttribute< ImplementationClass, AttributeType, DataMember >
    > {
  typedef AttributeImpl
  < & Simics::SimicsSetAttribute< ImplementationClass, AttributeType, DataMember >
  , & Simics::SimicsGetAttribute< ImplementationClass, AttributeType, DataMember >
  > base;
  static void doRegister(Detail::BaseClassImpl & aClass) {
    base::doRegister(aClass, AttributeClass::attributeName(), AttributeClass::attributeDescription());
  }
};
#endif

template <class CppObjectClass >
class Factory {
  typedef Class<CppObjectClass> class_;
  class_ * theClass;
public:
  Factory() {
    static class_ theStaticClass;
    theClass = &theStaticClass;
  }
  typename class_::object_type create(std::string aSimicsName) {
    //Ask simics to create the object for us
    API::conf_object_t * object = aux_::NewObject_stub(
                                    const_cast<API::conf_class_t *>( theClass->getSimicsClass() ), aSimicsName, theClass->constructor);

    if (!object) {
      throw SimicsException(std::string("An exception occured while attempting to create object ") + aSimicsName);
    }
    //Ask the SimicsClass to walk the magic data structure to get to the C++ object
    return theClass->cast_object(object);
  }

  API::conf_class_t * getSimicsClass() const {
    return theClass->getSimicsClass();
  }

  class_ & getClass() {
    return *theClass;
  }
};

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_CONFIGURATION_API_HPP_INCLUDED

