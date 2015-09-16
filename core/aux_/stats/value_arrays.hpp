#ifndef FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/boost_extensions/lexical_cast.hpp>

namespace Flexus {
namespace Stat {
namespace aux_ {

class StatValueArray_Counter : public StatValueArrayBase {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<StatValueArrayBase>(*this);
    ar & theInitialValue;
    ar & theValues;
  }
  StatValueArray_Counter() {}

public:
  typedef int64_t update_type;
  typedef int64_t value_type;
  typedef StatValue_Counter simple_type;

private:
  value_type theInitialValue;
  std::vector<simple_type> theValues;
public:
  StatValueArray_Counter(value_type aValue)
    : theInitialValue(aValue) {
    theValues.push_back(simple_type(theInitialValue));
  }

  //Used by serialization of other stat types
  StatValueArray_Counter( std::vector<simple_type> const & aValueVector, value_type aCurrentValue)
    : theValues(aValueVector) {
    theValues.push_back(simple_type(aCurrentValue));
  }

  void reduceSum( const StatValueBase & aBase) {
    std::cerr << "reductions not supported (StatValueArray_Counter)" << std::endl;
  }
  void update( update_type anUpdate ) {
    theValues.back().update(anUpdate);
  }
  void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
    for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
      anOstream << theValues[i] << ", ";
    }
    anOstream << theValues.back();
  }
  void newValue(accumulation_type aValueType) {
    if (aValueType == Accumulate) {
      theValues.push_back(theValues.back());
    } else {
      theValues.push_back( simple_type(theInitialValue) );
    }
  }
  void reset( value_type aValue) {
    theValues.clear();
    theValues.push_back(aValue);
  }
  StatValueBase & operator[](int32_t anIndex) {
    return theValues[anIndex];
  }
  std::size_t size() {
    return theValues.size();
  }
};

/*
  class StatValueArray_DoubleCounter : public StatValueArrayBase {
    private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, uint32_t version) {
          ar & boost::serialization::base_object<StatValueArrayBase>(*this);
          ar & theInitialValue;
          ar & theValues;
      }
      StatValueArray_DoubleCounter() {}

    public:
      typedef double update_type;
      typedef double value_type;
      typedef StatValue_DoubleCounter simple_type;

    private:
      value_type theInitialValue;
      std::vector<simple_type> theValues;
    public:
      StatValueArray_DoubleCounter(value_type aValue)
        : theInitialValue(aValue)
      {
        theValues.push_back(simple_type(theInitialValue));
      }

      //Used by serialization of other stat types
      StatValueArray_DoubleCounter( std::vector<simple_type> const & aValueVector, value_type aCurrentValue)
        : theValues(aValueVector)
      {
        theValues.push_back(simple_type(aCurrentValue));
      }

      void reduceSum( const StatValueBase & aBase) {
        std::cerr << "reductions not supported (StatValueArray_Counter)" << std::endl;
      }
      void update( update_type anUpdate ) {
        theValues.back().update(anUpdate);
      }
      void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
          anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
      }
      void newValue(accumulation_type aValueType) {
        if (aValueType == Accumulate) {
          theValues.push_back(theValues.back());
        } else {
          theValues.push_back( simple_type(theInitialValue) );
        }
      }
      void reset( value_type aValue) {
        theValues.clear();
        theValues.push_back(aValue);
      }
      StatValueBase & operator[](int32_t anIndex) { return theValues[anIndex]; }
      std::size_t size() { return theValues.size(); }
  };
*/

class StatValueArray_Annotation : public StatValueArrayBase {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<StatValueArrayBase>(*this);
    ar & theInitialValue;
    ar & theValues;
  }
  StatValueArray_Annotation() {}

public:
  typedef std::string update_type;
  typedef std::string value_type;
  typedef StatValue_Annotation simple_type;

private:
  value_type theInitialValue;
  std::vector<simple_type> theValues;
public:
  StatValueArray_Annotation(value_type aValue)
    : theInitialValue(aValue) {
    theValues.push_back(simple_type(theInitialValue));
  }

  //Used by serialization of other stat types
  StatValueArray_Annotation( std::vector<simple_type> const & aValueVector, value_type aCurrentValue)
    : theValues(aValueVector) {
    theValues.push_back(simple_type(aCurrentValue));
  }

  void reduceSum( const StatValueBase & aBase) {
    std::cerr << "reductions not supported (StatValueArray_Annotation)" << std::endl;
  }
  void update( update_type anUpdate ) {
    theValues.back().update(anUpdate);
  }
  void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
    for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
      anOstream << theValues[i] << ", ";
    }
    anOstream << theValues.back();
  }
  void newValue(accumulation_type aValueType) {
    if (aValueType == Accumulate) {
      theValues.push_back(theValues.back());
    } else {
      theValues.push_back( simple_type(theInitialValue) );
    }
  }
  void reset( value_type aValue) {
    theValues.clear();
    theValues.push_back(aValue);
  }
  StatValueBase & operator[](int32_t anIndex) {
    return theValues[anIndex];
  }
  std::size_t size() {
    return theValues.size();
  }
};

class StatValueArray_Max : public StatValueArrayBase {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<StatValueArrayBase>(*this);
    ar & theValues;
  }
  StatValueArray_Max() {}

public:
  typedef int64_t update_type;
  typedef int64_t value_type;
  typedef StatValue_Max simple_type;

private:
  std::vector<simple_type> theValues;
public:
  StatValueArray_Max(value_type /*ignored*/) {
    theValues.push_back(simple_type(0));
  }
  void reduceSum( const StatValueBase & aBase) {
    std::cerr << "reductions not supported (StatValueArray_Max)" << std::endl;
  }
  void update( update_type anUpdate ) {
    theValues.back().update(anUpdate);
  }
  void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
    for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
      anOstream << theValues[i] << ", ";
    }
    anOstream << theValues.back();
  }
  void newValue(accumulation_type aValueType) {
    if (aValueType == Accumulate) {
      theValues.push_back(theValues.back());
    } else {
      theValues.push_back( simple_type(0) );
    }
  }
  void reset( value_type /*ignored*/) {
    theValues.clear();
    theValues.push_back( simple_type(0) );
  }
  StatValueBase & operator[](int32_t anIndex) {
    return theValues[anIndex];
  }
  std::size_t size() {
    return theValues.size();
  }
};

class StatValueArray_Average : public StatValueArrayBase {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<StatValueArrayBase>(*this);
    ar & theValues;
  }
  StatValueArray_Average() {}

public:
  typedef std::pair< int64_t, int64_t> update_type;
  typedef int64_t value_type;
  typedef StatValue_Average simple_type;

private:
  std::vector<simple_type> theValues;
public:
  StatValueArray_Average(value_type /*ignored*/) {
    theValues.push_back(simple_type(0));
  }
  void reduceSum( const StatValueBase & aBase) {
    std::cerr << "reductions not supported (StatValueArray_Average)" << std::endl;
  }
  void update( update_type anUpdate ) {
    theValues.back().update(anUpdate);
  }
  void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
    for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
      anOstream << theValues[i] << ", ";
    }
    anOstream << theValues.back();
  }
  void newValue(accumulation_type aValueType) {
    if (aValueType == Accumulate) {
      theValues.push_back(theValues.back());
    } else {
      theValues.push_back( simple_type(0) );
    }
  }
  void reset( value_type /*ignored*/) {
    theValues.clear();
    theValues.push_back( simple_type(0) );
  }
  StatValueBase & operator[](int32_t anIndex) {
    return theValues[anIndex];
  }
  std::size_t size() {
    return theValues.size();
  }
};

class StatValueArray_StdDev : public StatValueArrayBase {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<StatValueArrayBase>(*this);
    ar & theValues;
  }
  StatValueArray_StdDev() {}

public:
  typedef int64_t update_type;
  typedef double value_type;
  typedef StatValue_StdDev simple_type;

private:
  std::vector<simple_type> theValues;
public:
  StatValueArray_StdDev(value_type /*ignored*/) {
    theValues.push_back(simple_type(0));
  }
  void reduceSum( const StatValueBase & aBase) {
    std::cerr << "reductions not supported (StatValueArray_StdDev)" << std::endl;
  }
  void update( update_type anUpdate ) {
    theValues.back().update(anUpdate);
  }
  void print(std::ostream & anOstream, std::string const & options = std::string("")) const {
    for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
      anOstream << theValues[i] << ", ";
    }
    anOstream << theValues.back();
  }
  void newValue(accumulation_type aValueType) {
    if (aValueType == Accumulate) {
      theValues.push_back(theValues.back());
    } else {
      theValues.push_back( simple_type(0) );
    }
  }
  void reset( value_type /*ignored*/) {
    theValues.clear();
    theValues.push_back( simple_type(0) );
  }
  StatValueBase & operator[](int32_t anIndex) {
    return theValues[anIndex];
  }
  std::size_t size() {
    return theValues.size();
  }
};

} // end aux_
} // end Stat
} // end Flexus

#endif //FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED
