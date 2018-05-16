#ifndef FLEXUS_CORE_BOOST_EXTENSIONS_LEXICAL_CAST_HPP_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSIONS_LEXICAL_CAST_HPP_INCLUDED

#include <core/exception.hpp>
#include <boost/lexical_cast.hpp>

namespace boost {

template<>
inline std::string lexical_cast<std::string, std::string > (const std::string & aString) {
  return aString;
}

template<>
inline bool lexical_cast<bool, std::string > (const std::string & aString) {
  if (aString == "true" || aString == "yes" || aString == "1") {
    return true;
  } else if (aString == "false" || aString == "no" || aString == "0") {
    return false;
  }
  throw Flexus::Core::FlexusException(__FILE__, __LINE__, std::string("Unable to parse boolean value '") + aString + "', must be one of: true, yes, 1, false, no, 0");
}

}

#endif //FLEXUS_CORE_BOOST_EXTENSIONS_LEXICAL_CAST_HPP_INCLUDED
