#ifndef FLEXUS_CORE_DEBUG_FIELD_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_FIELD_HPP_INCLUDED

#include <boost/optional.hpp>
#include <string>
#include <utility>

namespace Flexus {
namespace Dbg {

using namespace std::rel_ops;

class Field
{
    std::string theName;
    // These members are declared mutable for 2 reasons:
    // 1 - the numeric to string conversion for theTextValue is done
    //    when value() is called, and value needs to be a const member.
    // 2 - Changing these members does not affect the ordering of Fields
    mutable boost::optional<std::string> theTextValue;
    mutable boost::optional<int64_t> theNumericValue;

  public:
    bool operator==(Field const& aField) const;
    bool operator<(Field const& aField) const;
    Field(std::string const& aName);

    // This method is declared const because it does not affecgt the ordering
    // of Fields
    void setValue(std::string const& aString) const;

    // This method is declared const because it does not affecgt the ordering
    // of Fields
    void setValue(int64_t aValue) const;
    bool isNumeric() const;
    std::string const& value() const;
    int64_t numericValue() const;
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_FIELD_HPP_INCLUDED
