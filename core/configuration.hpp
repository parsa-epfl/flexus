#ifndef FLEXUS_CONFIGURATION_HPP_INCLUDED
#define FLEXUS_CONFIGURATION_HPP_INCLUDED

#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <core/debug/debug.hpp>
#include <core/exception.hpp>
#include <sstream>
#include <type_traits>

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
struct ParameterBase
{
    // The Parameter definition object describing the command line options
    std::string theName;
    std::string theDescription;
    std::string theSwitch;
    const std::string& theConfig;

    // Non-copyable
    ParameterBase(const ParameterBase&)            = delete;
    ParameterBase& operator=(const ParameterBase&) = delete;

    // Constructor taking a ParameterDefinition object.  Defined after
    // ConfigurationManager as it requires the declaration of
    // theConfigurationManager.
    ParameterBase(const std::string& aName,
                  const std::string& aDescr,
                  const std::string& aSwitch,
                  const std::string& aConfig);

    // This setValue() method is overridden in the derived DynamicParameter
    // classes This is the key to getting dynamic parameters to work.  When this
    // is invoked, the derived class converts the string stored in lexical_value
    // into the appropriate type for the parameter
    virtual void setValue(std::string aValue) = 0;
    virtual bool isOverridden()               = 0;
    virtual std::string lexicalValue()        = 0;

    virtual ~ParameterBase() {}
};

} // namespace aux_

struct ConfigurationManager
{
    // Sets up a configuration based on anArgc and anArgv
    virtual void processCommandLineConfiguration(int32_t anArgc, char** anArgv) = 0;

    // Print out the command line options
    virtual void printParams()       = 0;
    virtual void checkAllOverrides() = 0;

    // Print out the command line options
    virtual void printConfiguration(std::ostream&)                        = 0;
    virtual void parseConfiguration(std::istream& anIstream)              = 0;
    virtual void set(std::string const& aName, std::string const& aValue) = 0;

    // Adds a DynamicParameter to theParameters
    virtual void registerParameter(aux_::ParameterBase& aParam) = 0;

    static ConfigurationManager& getConfigurationManager();
    // added by PLotfi
    static std::string getParameterValue(std::string const& aName, bool exact = true);
    // end PLotfi

    // Non-copyable
    ConfigurationManager(const ConfigurationManager&)            = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

  protected:
    // Constructible by derived classes only
    ConfigurationManager() {}
    // Suppress warning
    virtual ~ConfigurationManager() {}
    // added by PLotfi
    virtual void determine(std::string const& aName, bool exact) = 0;
    // end PLotfi
};

namespace aux_ {

// Dynamic Parameter Implementation
//******************************
// Template for defininig dynamic parameters
template<class ParamStruct, class ParamTag>
struct DynamicParameter : public ParameterBase
{
    ParamStruct& theConfig;
    bool theOverridden;

    typedef typename ParamStruct::template get<ParamTag> param_def;

    // Default Constructor initializes to default value
    DynamicParameter(ParamStruct& aConfig)
      : ParameterBase(param_def::name(), param_def::description(), param_def::cmd_line_switch(), aConfig.theConfigName_)
      , theConfig(aConfig)
      , theOverridden(false)
    {
        theConfig.*(param_def::member()) = param_def::defaultValue();
    }

    void initialize(typename param_def::type const& aVal)
    {
        theConfig.*(param_def::member()) = aVal;
        theOverridden                    = true;
    }

    // When setValue is called, we process the command line parameter
    // and fill in value
    virtual void setValue(std::string aValue)
    {
        try {
            theConfig.*(param_def::member()) = boost::lexical_cast<typename param_def::type>(aValue);
            theOverridden                    = true;
        } catch (bad_lexical_cast& e) {
            DBG_(Crit, (<< "Bad Lexical Cast attempting to set dynamic parameter."));
            std::cout << "WARNING: Unable to set parameter " << param_def::name() << " to " << aValue << std::endl;
        }
    }

    bool isOverridden() { return theOverridden; }

    template<typename T>
    T& toString(T& v)
    {
        return v;
    }
    template<bool>
    const char* toString(bool& v)
    {
        return (v ? "true" : "false");
    }

    std::string lexicalValue()
    {
        std::stringstream lex;
        lex << toString(theConfig.*(param_def::member()));
        return lex.str();
    }
};
} // namespace aux_

} // namespace Core
} // namespace Flexus

#endif // FLEXUS_CONFIGURATION_HPP_INCLUDED
