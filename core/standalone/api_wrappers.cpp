// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block   
#ifndef CONFIG_QEMU
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <core/target.hpp>
#include <core/qemu/api_wrappers.hpp>

namespace Flexus {
namespace Simics {
namespace APIFwd {

Simics::API::conf_class_t * SIM_register_class(const char * name, Simics::API::class_data_t * class_data) {
  std::cout << "Error, called unemulated Simics::API method: " << __FUNCTION__ << std::endl;
  return 0;
}

int32_t SIM_register_attribute(
  Simics::API::conf_class_t * class_struct
  , const char * attr_name
  , Simics::API::get_attr_t get_attr
  , Simics::API::lang_void * get_attr_data
  , Simics::API::set_attr_t set_attr
  , Simics::API::lang_void * set_attr_data
  , Simics::API::attr_attr_t attr
  , const char * doc
) {
  //Does nothing for standalone targets
  return 0;
}

void SIM_object_constructor(Simics::API::conf_object_t * conf_obj, Simics::API::parse_object_t * parse_obj) {
  //Does nothing for standalone targets
}

Simics::API::conf_object_t * SIM_new_object(Simics::API::conf_class_t * conf_class, const char * instance_name) {
  Simics::API::conf_object_t * ret_val = new Simics::API::conf_object_t;
  ret_val->name = strdup(instance_name);
  ret_val->class_data = 0;
  ret_val->queue = 0;
  ret_val->object_id = 0;
  return ret_val;
}

void SIM_break_simulation(const char * aMessage) {
  std::cout << "Break requested by debugger.";
  std::abort();
}

void SIM_write_configuration_to_file(const char * aFilename) {
  //Does nothing for standalone targets
}

} //namespace APIFwd
} //End Namespace Simics
} //namespace Flexus
#endif //CONFIG_QEMU
