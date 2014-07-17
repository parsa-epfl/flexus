#ifndef FLEXUS_QEMU_HAP_API_HPP_INCLUDED
#define FLEXUS_QEMU_HAP_API_HPP_INCLUDED

#include <vector>
#include <algorithm>
#include <string>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <core/simics/trampoline.hpp>
#include <core/exception.hpp>

namespace Flexus {
namespace Qemu {

namespace API {
extern "C" {
#include <core/qemu/api.h>
}
}

using std::vector;
using std::for_each;
using std::string;
using std::to_string;

namespace HAPs {

struct Core_Initial_Configuration {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_noc_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Continuation {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_noc_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Simulation_Stopped {
	// typedef void (*cb_func_nocIs_t)(lang_void *, conf_object_t *,
	//                                 integer_t, char *);
  typedef  API::cb_func_nocIs_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long, char *);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Asynchronous_Trap {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_nocI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Exception_Return {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_nocI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Magic_Instruction {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_nocI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Ethernet_Network_Frame {
	// typedef void (*cb_func_noiiI_t)(lang_void *, int, int, integer_t);
  typedef  API::cb_func_noiiI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, int, int, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(int, int, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Ethernet_Frame {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_nocI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Core_Periodic_Event {
	// typedef void (*cb_func_noc_t)(lang_void *, conf_object_t *);
  typedef  API::cb_func_nocI_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Xterm_Break_String {
	// typedef void (*cb_func_nocs_t)(lang_void *, conf_object_t *, char *);
  typedef  API::cb_func_nocs_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, char *);
  };
  static const API::QEMU_callback_event_t hap_name;
};

struct Gfx_Break_String {
	// typedef void (*cb_func_nocs_t)(lang_void *, conf_object_t *, char *);
  typedef  API::cb_func_nocs_t qemu_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, char *);
  };
  static const API::QEMU_callback_event_t hap_name;
};

}

template <class Hap, typename Hap::free_fn_ptr FreeFnPtr>
class HapToFreeFnBinding {
  API::hap_type_t theHapNumber;
  API::hap_handle_t theHapHandle;
public:
  HapToFreeFnBinding (void * aUserPtr = 0) {
    if (! theHapNumber) {
      throw QemuException(
			  string("While attempting to register Hap handler: No Such HAP: ") 
			  + to_string(Hap::hap_name));
    }
    theHapHandle = API::QEMU_hap_add_callback(
                     Hap::hap_name,
                     reinterpret_cast<API::obj_hap_func_t> 
						(& make_signature_from_free_fn
						 <typename Hap::qemu_fn_type>::
						 template with<FreeFnPtr>::trampoline),
                     aUserPtr);
    if (theHapHandle == -1) {
      throw QemuException(
			  string("While attempting to register Hap handler: Unable to" +
				  "install handler: ") + to_string(Hap::hap_name));
    }
  }

  ~HapToFreeFnBinding () {
    API::QEMU_hap_delete_callback(Hap::hap_name, theHapHandle);
  }
};

template <class Hap, class T, typename 
	Hap::template member<T>::member_fn_ptr MemberFnPtr>
class HapToMemFnBinding {
  API::hap_type_t theHapNumber;
  API::hap_handle_t theHapHandle;
public:
  HapToMemFnBinding (T * anObject) {
    theHapHandle = API::QEMU_hap_add_callback(
                     Hap::hap_name,
                     reinterpret_cast<API::obj_hap_func_t> 
					 (& make_signature_from_member_fn
					  <typename Hap::qemu_fn_type>::template for_class<T>::
					  template with<MemberFnPtr>::trampoline),
                     anObject);
    if (theHapHandle == -1) {
      throw QemuException(
			  string("While attempting to register Hap handler: Unable to " +
				  "install handler: ") + to_string(Hap::hap_name));
    }
  }

  HapToMemFnBinding (T * anObject, long long anIndex) {
    theHapHandle = API::QEMU_hap_add_callback_index(
                     Hap::hap_name,
                     reinterpret_cast<API::obj_hap_func_t> 
					 (& make_signature_from_member_fn
					  <typename Hap::qemu_fn_type>::template for_class<T>::
					  template with<MemberFnPtr>::trampoline),
                     anObject,
                     anIndex);
    if (theHapHandle == -1) {
      throw QemuException(
			  string("While attempting to register Hap handler: Unable to " +
				  "install handler: ") + to_string(Hap::hap_name));
    }
  }

  ~HapToMemFnBinding() {
    API::QEMU_hap_delete_callback_id(Hap::hap_name, theHapHandle);
  }
};

class InitialConfigHapHandler {
  typedef InitialConfigHapHandler self;
  InitialConfigHapHandler() :
    theBinding((self *)this) {}
  ~InitialConfigHapHandler() {}

  static void registerFunctor( boost::function<void ()> aFunctor) {
    if (theStaticInitialConfigHapHandler == 0) {
      theStaticInitialConfigHapHandler = new InitialConfigHapHandler();
    }
    theStaticInitialConfigHapHandler
		->theInitialConfigFunctors.push_back(aFunctor);
  }
public:
  static void callOnConfigReady( boost::function<void ()> aFunctor) {
    //We cache the result of QEMU_initial_configuration_ok to save ourselves the function call
    //after it returns true.
    static bool configuration_ok = false;
    if (configuration_ok || (configuration_ok = API::QEMU_initial_configuration_ok())) {
      //call the functor now since the initial config has already been loaded
      aFunctor();
    } else {
      //Register the functor to be called when the Initial Config HAP occurs
      registerFunctor(aFunctor);
    }
  }
  void OnInitialConfig(API::conf_object_t * ignored) {
    using namespace boost::lambda;
    for_each(
      theInitialConfigFunctors.begin(),
      theInitialConfigFunctors.end(),
      boost::lambda::bind<void>(boost::lambda::_1) //Invoke the functor stored in the vector.  Note that we need to tell bind the return type
    );
    //Self destruct after the Initial Config Hap
    delete this;
    theStaticInitialConfigHapHandler = 0;
  }
private:
  HapToMemFnBinding <
  HAPs::Core_Initial_Configuration,
       self,
       &self::OnInitialConfig > theBinding;

  vector< boost::function<void ()> > theInitialConfigFunctors;

  static self * theStaticInitialConfigHapHandler;

};

inline void CallOnConfigReady(boost::function<void ()> aFunctor) {
  InitialConfigHapHandler::callOnConfigReady(aFunctor);
}

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_SIMICS_HAP_API_HPP_INCLUDED

