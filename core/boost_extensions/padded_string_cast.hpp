#ifndef FLEXUS_CORE_BOOST_EXTENSIONS_PADDED_STRING_CAST_INCLUDED
#define FLEXUS_CORE_BOOST_EXTENSIONS_PADDED_STRING_CAST_INCLUDED

#include <sstream>
#include <string>
#include <iomanip>

namespace boost {

template <int32_t w, char c, typename Source>
std::string padded_string_cast(Source s) {
  std::ostringstream ss;
  ss << std::setfill(c) << std::setw(w) << s;
  return ss.str();
}

}

#endif //FLEXUS_CORE_BOOST_EXTENSIONS_PADDED_STRING_CAST_INCLUDED
