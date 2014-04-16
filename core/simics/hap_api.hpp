#ifndef FLEXUS_SIMICS_HAP_API_HPP_INCLUDED
#define FLEXUS_SIMICS_HAP_API_HPP_INCLUDED

#include <vector>
#include <algorithm>
#include <string>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <core/simics/trampoline.hpp>
#include <core/exception.hpp>

namespace Flexus {
namespace Simics {

namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(configuration)
}
}

using std::vector;
using std::for_each;
using std::string;

namespace HAPs {

struct Core_Initial_Configuration {
  typedef  API::cb_func_noc_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *);
  };
  static const char * hap_name;
};

struct Core_Continuation {
  typedef  API::cb_func_noc_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *);
  };
  static const char * hap_name;
};

struct Core_Simulation_Stopped {
  typedef  API::cb_func_nocIs_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long, char *);
  };
  static const char * hap_name;
};

struct Core_Asynchronous_Trap {
  typedef  API::cb_func_nocI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const char * hap_name;
};

struct Core_Exception_Return {
  typedef  API::cb_func_nocI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const char * hap_name;
};

struct Core_Magic_Instruction {
  typedef  API::cb_func_nocI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const char * hap_name;
};

struct Ethernet_Network_Frame {
  typedef  API::cb_func_noiiI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, int, int, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(int, int, long long);
  };
  static const char * hap_name;
};

struct Ethernet_Frame {
  typedef  API::cb_func_nocI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const char * hap_name;
};

struct Core_Periodic_Event {
  typedef  API::cb_func_nocI_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, long long);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, long long);
  };
  static const char * hap_name;
};

struct Xterm_Break_String {
  typedef  API::cb_func_nocs_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, char *);
  };
  static const char * hap_name;
};

struct Gfx_Break_String {
  typedef  API::cb_func_nocs_t simics_fn_type;
  typedef  void (*free_fn_ptr)(void *, API::conf_object_t *, char *);
  template <class T>
  struct member {
    typedef  void (T::* member_fn_ptr)(API::conf_object_t *, char *);
  };
  static const char * hap_name;
};

}

template <class Hap, typename Hap::free_fn_ptr FreeFnPtr>
class HapToFreeFnBinding {
  API::hap_type_t theHapNumber;
  API::hap_handle_t theHapHandle;
public:
  HapToFreeFnBinding (void * aUserPtr = 0) {
    theHapNumber = API::SIM_hap_get_number(Hap::hap_name);
    if (! theHapNumber) {
      throw SimicsException(string("While attempting to register Hap handler: No Such HAP: ") + Hap::hap_name);
    }
    theHapHandle = API::SIM_hap_add_callback(
                     theHapNumber,
                     reinterpret_cast<API::obj_hap_func_t> (& make_signature_from_free_fn<typename Hap::simics_fn_type>::template with<FreeFnPtr>::trampoline),
                     aUserPtr);
    if (theHapHandle == -1) {
      throw SimicsException(string("While attempting to register Hap handler: Unable to install handler: ") + Hap::hap_name);
    }
  }

  ~HapToFreeFnBinding () {
    API::SIM_hap_delete_callback(Hap::hap_name, theHapHandle);
  }
};

template <class Hap, class T, typename Hap::template member<T>::member_fn_ptr MemberFnPtr>
class HapToMemFnBinding {
  API::hap_type_t theHapNumber;
  API::hap_handle_t theHapHandle;
public:
  HapToMemFnBinding (T * anObject) {
    theHapHandle = API::SIM_hap_add_callback(
                     Hap::hap_name,
                     reinterpret_cast<API::obj_hap_func_t> (& make_signature_from_member_fn<typename Hap::simics_fn_type>::template for_class<T>::template with<MemberFnPtr>::trampoline),
                     anObject);
    if (theHapHandle == -1) {
      throw SimicsException(string("While attempting to register Hap handler: Unable to install handler: ") + Hap::hap_name);
    }
  }

  HapToMemFnBinding (T * anObject, long long anIndex) {
    theHapHandle = API::SIM_hap_add_callback_index(
                     Hap::hap_name,
                     reinterpret_cast<API::obj_hap_func_t> (& make_signature_from_member_fn<typename Hap::simics_fn_type>::template for_class<T>::template with<MemberFnPtr>::trampoline),
                     anObject,
                     anIndex);
    if (theHapHandle == -1) {
      throw SimicsException(string("While attempting to register Hap handler: Unable to install handler: ") + Hap::hap_name);
    }
  }

  ~HapToMemFnBinding() {
    API::SIM_hap_delete_callback_id(Hap::hap_name, theHapHandle);
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
    theStaticInitialConfigHapHandler->theInitialConfigFunctors.push_back(aFunctor);
  }
public:
  static void callOnConfigReady( boost::function<void ()> aFunctor) {
    //We cache the result of SIM_initial_configuration_ok to save ourselves the function call
    //after it returns true.
    static bool configuration_ok = false;
    if (configuration_ok || (configuration_ok = API::SIM_initial_configuration_ok())) {
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

