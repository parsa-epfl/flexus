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
#ifndef FLEXUS_CONFIGURATION_HPP_INCLUDED
#define FLEXUS_CONFIGURATION_HPP_INCLUDED

#include <sstream>
#include <type_traits>

#include <boost/utility.hpp>
#include <core/boost_extensions/lexical_cast.hpp>

#include <core/debug/debug.hpp>

#include <core/exception.hpp>
#include <core/metaprogram.hpp>

namespace Flexus {
namespace Core {

using std::is_integral;
using std::is_same;
using std::string;

using boost::bad_lexical_cast;
using boost::lexical_cast;

namespace aux_ {
// These classes must be complete before ConfigurationManager can be defined,
// hence, they appear before ConfigurationManager

// Base class for Dynamic Parameters
struct ParameterBase {
  // The Parameter definition object describing the command line options
  std::string theName;
  std::string theDescription;
  std::string theSwitch;
  const std::string &theConfig;

  // Non-copyable
  ParameterBase(const ParameterBase &) = delete;
  ParameterBase &operator=(const ParameterBase &) = delete;

  // Constructor taking a ParameterDefinition object.  Defined after
  // ConfigurationManager as it requires the declaration of
  // theConfigurationManager.
  ParameterBase(const std::string &aName, const std::string &aDescr, const std::string &aSwitch,
                const std::string &aConfig);

  // This setValue() method is overridden in the derived DynamicParameter
  // classes This is the key to getting dynamic parameters to work.  When this
  // is invoked, the derived class converts the string stored in lexical_value
  // into the appropriate type for the parameter
  virtual void setValue(std::string aValue) = 0;
  virtual bool isOverridden() = 0;
  virtual std::string lexicalValue() = 0;

  virtual ~ParameterBase() {
  }
};

} // namespace aux_

struct ConfigurationManager {
  // Sets up a configuration based on anArgc and anArgv
  virtual void processCommandLineConfiguration(int32_t anArgc, char **anArgv) = 0;

  // Print out the command line options
  virtual void printParams() = 0;
  virtual void checkAllOverrides() = 0;

  // Print out the command line options
  virtual void printConfiguration(std::ostream &) = 0;
  virtual void parseConfiguration(std::istream &anIstream) = 0;
  virtual void set(std::string const &aName, std::string const &aValue) = 0;

  // Adds a DynamicParameter to theParameters
  virtual void registerParameter(aux_::ParameterBase &aParam) = 0;

  static ConfigurationManager &getConfigurationManager();
  // added by PLotfi
  static std::string getParameterValue(std::string const &aName, bool exact = true);
  // end PLotfi

  // Non-copyable
  ConfigurationManager(const ConfigurationManager &) = delete;
  ConfigurationManager &operator=(const ConfigurationManager &) = delete;

protected:
  // Constructible by derived classes only
  ConfigurationManager() {
  }
  // Suppress warning
  virtual ~ConfigurationManager() {
  }
  // added by PLotfi
  virtual void determine(std::string const &aName, bool exact) = 0;
  // end PLotfi
};

namespace aux_ {

// Dynamic Parameter Implementation
//******************************
// Template for defininig dynamic parameters
template <class ParamStruct, class ParamTag> struct DynamicParameter : public ParameterBase {
  ParamStruct &theConfig;
  bool theOverridden;

  typedef typename ParamStruct::template get<ParamTag> param_def;

  // Default Constructor initializes to default value
  DynamicParameter(ParamStruct &aConfig)
      : ParameterBase(param_def::name(), param_def::description(), param_def::cmd_line_switch(),
                      aConfig.theConfigName_),
        theConfig(aConfig), theOverridden(false) {
    theConfig.*(param_def::member()) = param_def::defaultValue();
  }

  void initialize(typename param_def::type const &aVal) {
    theConfig.*(param_def::member()) = aVal;
    theOverridden = true;
  }

  // When setValue is called, we process the command line parameter
  // and fill in value
  virtual void setValue(std::string aValue) {
    try {
      theConfig.*(param_def::member()) = boost::lexical_cast<typename param_def::type>(aValue);
      theOverridden = true;
    } catch (bad_lexical_cast &e) {
      DBG_(Crit, (<< "Bad Lexical Cast attempting to set dynamic parameter."));
      std::cout << "WARNING: Unable to set parameter " << param_def::name() << " to " << aValue
                << std::endl;
    }
  }

  bool isOverridden() {
    return theOverridden;
  }

  template <typename T> T &toString(T &v) {
    return v;
  }
  template <bool> const char *toString(bool &v) {
    return (v ? "true" : "false");
  }

  std::string lexicalValue() {
    std::stringstream lex;
    lex << toString(theConfig.*(param_def::member()));
    return lex.str();
  }
};
} // namespace aux_

} // namespace Core
} // namespace Flexus

#endif // FLEXUS_CONFIGURATION_HPP_INCLUDED
