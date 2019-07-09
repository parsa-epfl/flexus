#ifndef FLEXUS_CORE_DEBUG_PARSER_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_PARSER_HPP_INCLUDED

#include <string>

namespace Flexus {
namespace Dbg {

class ParserImpl;

struct Parser {
  static Parser &parser();

  virtual ~Parser(){};
  virtual void parse(std::string const &aConfiguration) = 0;
  virtual void parseFile(std::string const &aConfigFile) = 0;
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_PARSER_HPP_INCLUDED
