#include <components/CommonQEMU/DoubleWord.hpp>

namespace Flexus {
namespace SharedTypes {

bool operator== (uint64_t compare, DoubleWord const & aDoubleWord) {
  bool ret_val(true);
  for ( int32_t i = 7; i >= 0 && ret_val; --i) {
    if (aDoubleWord.theValidBits & (1 << i)) {
      ret_val = ((aDoubleWord.theDoubleWord >> (8 * i )) & 0xFF) == ((compare >> (8 * i)) & 0xFF);
    }
  }
  return ret_val;
};

bool operator== (DoubleWord const & entry, uint64_t compare) {
  return compare == entry;
}

std::ostream & operator <<(std::ostream & anOstream, DoubleWord const & aDoubleWord) {

  anOstream << "DW<<" << &std::hex << (uint32_t) aDoubleWord.theValidBits << ',' << aDoubleWord.theDoubleWord << ">> ";
  anOstream << "0x" << &std::hex;
  uint64_t value(aDoubleWord.theDoubleWord);
  for ( int32_t i = 7; i >= 0; --i) {
    if (aDoubleWord.theValidBits & (1 << i)) {
      anOstream << (uint32_t) ((value>>(8 * i + 4)) & 0xF);
      anOstream << (uint32_t) ((value>>(8 * i)) & 0xF);
    } else {
      anOstream << "--";
    }
  }
  anOstream << & std::dec;
  return anOstream;
}

} //SharedTypes
} //Flexus
