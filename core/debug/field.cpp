#include <utility>
#include <string>
#include <boost/optional.hpp>
#include <core/boost_extensions/lexical_cast.hpp>

#include <core/debug/field.hpp>

namespace Flexus {
namespace Dbg {

bool Field::operator ==(Field const & aField) const {
  return theName == aField.theName;
}

bool Field::operator < (Field const & aField) const {
  return theName < aField.theName;
}

Field::Field(std::string const & aName)
  : theName(aName)
{ }

//This method is declared const because it does not affecgt the ordering
//of Fields
void Field::setValue(std::string const & aString) const {
  theNumericValue.reset();
  theTextValue.reset(aString);
}

//This method is declared const because it does not affecgt the ordering
//of Fields
void Field::setValue(int64_t aValue) const {
  theTextValue.reset();
  theNumericValue.reset(aValue);
}

bool Field::isNumeric() const {
  return theNumericValue != boost::none;
}

std::string const & Field::value() const {
  if (! theTextValue) {
    if (isNumeric()) {
      theTextValue.reset(boost::lexical_cast<std::string>(*theNumericValue));
    } else {
      theTextValue.reset(std::string());
    }
  }
  return *theTextValue;
}

int64_t Field::numericValue() const {
  if (isNumeric()) {
    return *theNumericValue;
  } else {
    return 0;
  }
}

} //Dbg
} //Flexus

