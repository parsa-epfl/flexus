#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <core/configuration.hpp>
#include <core/debug/debug.hpp>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

namespace Flexus {
namespace Wiring {
bool
initializeParameters();
}
} // namespace Flexus

namespace Flexus {
namespace Core {

// added by PLotfi
std::string theParameterValue;
// end PLotfi

namespace aux_ {

class ConfigurationManagerDetails : public ConfigurationManager
{

    // map of command line switches to Parameters
    typedef std::map<const string, ParameterBase*> parameter_map;
    std::map<const string, ParameterBase*> theParameters;
    bool theAbortOnUnitialized;

  public:
    ConfigurationManagerDetails()
      : theAbortOnUnitialized(false)
    {
    }

    // Sets up a configuration based on anArgc and anArgv
    void processCommandLineConfiguration(int32_t anArgc, char** anArgv)
    {
        theAbortOnUnitialized = Flexus::Wiring::initializeParameters();

        for (int32_t i = 1; i < anArgc; ++i) {
            std::string arg(anArgv[i]);
            if ((arg.compare("-help") == 0) || (arg.compare("-h") == 0)) {
                printParams();
                std::exit(0);
            } else {
                // Try to match a parameter
                parameter_map::iterator iter = theParameters.find(arg);
                if (iter == theParameters.end()) {
                    std::cout << "WARNING: There is no parameter named \"" << arg << "\"" << std::endl;
                    continue;
                }
                std::string lexical_value(anArgv[++i]);
                iter->second->setValue(lexical_value);
            }
        }
    }

    void checkAllOverrides()
    {
        bool missing                 = false;
        parameter_map::iterator iter = theParameters.begin();
        parameter_map::iterator end  = theParameters.end();
        while (iter != end) {
            if (!iter->second->isOverridden()) {
                DBG_(Crit,
                     (<< "WARNING: " << iter->first << " (" << iter->second->theName
                      << ")  was not set in initializeParameters(), from the command "
                         "line, or from Simics."));
                missing = true;
            }
            ++iter;
        }
        if (theAbortOnUnitialized && missing) {
            DBG_Assert(false,
                       (<< "ERROR: Not all parameters were initialized, and "
                           "initalizeParameters() indicates that they should be."));
        }
    }

    void printConfiguration(std::ostream& anOstream)
    {
        parameter_map::iterator iter = theParameters.begin();
        parameter_map::iterator end  = theParameters.end();
        while (iter != end) {
            anOstream << "flexus.set " << std::setw(30) << std::setiosflags(std::ios::left)
                      << ("\"" + iter->first + "\"") << " " << std::setw(20) << std::setiosflags(std::ios::left)
                      << ("\"" + iter->second->lexicalValue() + "\"");
            anOstream << " # (" << iter->second->theName << ") ";
            anOstream << iter->second->theDescription;
            anOstream << std::endl;
            ++iter;
        }
    }

    void parseConfiguration(std::istream& anIstream)
    {
        std::string line;
        std::vector<std::string> strs;
        while (std::getline(anIstream, line)) {
            boost::split(strs, line, boost::is_any_of(" \t\""), boost::token_compress_on);
            if (strs[0] == "" || line.c_str()[0] == '#') continue;

            std::string param_name, value, comment;
            param_name = strs[1];
            value      = strs[2];

            // get the comment
            std::stringstream ss;
            for (size_t i = 3; i < strs.size(); ++i) {
                if (i != 3) ss << " ";
                ss << strs[i];
            }
            comment = ss.str();

            DBG_(VVerb, (<< "Dynamic param:" << param_name << ", value:" << value << ", comment:" << comment));
            set(param_name, value);
        }
    }

    void set(std::string const& aName, std::string const& aValue)
    {
        parameter_map::iterator iter = theParameters.find(aName);
        if (iter == theParameters.end()) {
            std::cout << "WARNING: There is no parameter named \"" << aName << "\"" << std::endl;
        } else {
            iter->second->setValue(aValue);
        }
    }

    // added by PLotfi to support virtualized chips and/or multi-server chips
    void determine(std::string const& aName, bool exact)
    {
        theParameterValue = "not_found";
        if (exact == true) {
            parameter_map::iterator iter = theParameters.find(aName);
            if (iter != theParameters.end()) { theParameterValue = iter->second->lexicalValue(); }
        } else {
            parameter_map::iterator iter = theParameters.begin();
            parameter_map::iterator end  = theParameters.end();
            while (iter != end) {
                std::string one   = iter->first;
                std::string two   = iter->second->theName;
                std::string three = iter->second->lexicalValue();
                if (iter->first.find(aName) != string::npos) { // aName is found
                    theParameterValue = iter->second->lexicalValue();
                    break;
                }
                ++iter;
            }
        }
    }
    // end PLotfi

    // Helper structs used by printParams()
  private:
    struct param_helper
    {
        void operator()(const std::pair<const std::string, const ParameterBase*>& aParam)
        {
            std::cout << "  -" << aParam.second->theConfig << ":" << aParam.second->theSwitch << "\n"
                      << "       (" << aParam.second->theName << ") " << aParam.second->theDescription << std::endl;
        }
    };

  public:
    // Print out the command line options
    void printParams()
    {
        std::for_each(theParameters.begin(), theParameters.end(), param_helper());
        // for_each(thePolicies.begin(),  thePolicies.end(), policy_helper());
    }

    // Adds a DynamicParameter to theParameters
    void registerParameter(ParameterBase& aParam)
    {
        DBG_(VVerb, Core()(<< "Registered Parameter: -" << aParam.theConfig << ':' << aParam.theSwitch));
        theParameters.insert(std::make_pair(std::string("-") + aParam.theConfig + ":" + aParam.theSwitch, &aParam));
    }
};

} // namespace aux_

std::unique_ptr<aux_::ConfigurationManagerDetails> theConfigurationManager{};

ConfigurationManager&
ConfigurationManager::getConfigurationManager()
{
    if (theConfigurationManager == 0) {
        DBG_(Dev, (<< "Initializing Flexus::ConfigurationManager..."));
        theConfigurationManager.reset(new aux_::ConfigurationManagerDetails());
        DBG_(Dev, (<< "ConfigurationManager initializing"));
    }
    return *theConfigurationManager;
}

std::string
ConfigurationManager::getParameterValue(std::string const& aName, bool exact)
{
    ConfigurationManager::getConfigurationManager().determine(aName, exact);
    return theParameterValue;
}

namespace aux_ {
ParameterBase::ParameterBase(const std::string& aName,
                             const std::string& aDescr,
                             const std::string& aSwitch,
                             const std::string& aConfig)
  : theName(aName)
  , theDescription(aDescr)
  , theSwitch(aSwitch)
  , theConfig(aConfig)
{
    ConfigurationManager::getConfigurationManager().registerParameter(*this);
}
} // namespace aux_

} // End Namespace Core
} // namespace Flexus
