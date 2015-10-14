#ifndef FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <boost/regex.hpp>
#include <functional>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/boost_extensions/lexical_cast.hpp>

namespace Flexus {
namespace Stat {
namespace aux_ {

class Measurement : public boost::counted_base {
  std::string theName;
  boost::regex theStatExpression;

private:
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, uint32_t version) const {
    ar & theName;
    std::string temp(theStatExpression.str());
    ar & temp;
  }
  template<class Archive>
  void load(Archive & ar, uint32_t version) {
    ar & theName;
    std::string temp;
    ar & temp;
    theStatExpression = temp;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
protected:
  Measurement( ) { }

public:

  Measurement( std::string const & aName, std::string const & aStatExpression)
    : theName(aName)
    , theStatExpression(aStatExpression)
  {}
  virtual ~Measurement() {}
  bool includeStat( Stat * aStat );
  std::string const & name() {
    return theName;
  }
  void resetName(std::string const & aName) {
    theName = aName;
  }
  friend std::ostream & operator << (std::ostream & anOstream, Measurement const & aMeasurement) {
    anOstream << aMeasurement.theName;
    return anOstream;
  }
  virtual bool isSimple() {
    return false;
  }
  virtual void print( std::ostream & anOstream, std::string const & options = std::string("")) {}
  virtual void addToMeasurement( Stat * aStat ) { }
  virtual void format( std::ostream & anOstream, std::string const & aStat, std::string const & options = std::string("")) {}
  virtual void doOp( std::ostream & anOstream, std::string const & anOp, std::string const & options)  {
    if (anOp == "MSMT") {
      doMSMT(anOstream);
    } else {
      anOstream << "{ERR:Op Not Supported}";
    }
  }
  void doMSMT( std::ostream & anOstream ) {
    anOstream << name();
  }
  virtual void reduce( eReduction aReduction, Measurement * aMeasurement) {}
  virtual void reduceNodes() {}

  virtual void close() {}
};

class SimpleMeasurement : public Measurement {
  typedef std::map<std::string, StatValueHandle> stat_handle_map;
  stat_handle_map theStats;

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<Measurement>(*this);
    ar & theStats;
  }
  SimpleMeasurement( ) {}

public:
  SimpleMeasurement( std::string const & aName, std::string const & aStatExpression)
    : Measurement(aName, aStatExpression)
  { }
  virtual ~SimpleMeasurement() {}

  void addToMeasurement( Stat * aStat );
  void close();
  bool isSimple() {
    return true;
  }
  void print( std::ostream & anOstream, std::string const & options = std::string(""));
  void format( std::ostream & anOstream, std::string const & aStat, std::string const & options = std::string(""));
  int64_t asLongLong( std::string const & aStat);
  double asDouble( std::string const & aStat);
  int64_t sumAsLongLong( std::string const & aStatFilter);
  int64_t minAsLongLong( std::string const & aStatFilter);
  int64_t maxAsLongLong( std::string const & aStatFilter);
  double avgAsDouble( std::string const & aStatFilter);
  void doOp( std::ostream & anOstream, std::string const & anOp, std::string const & options );
  void doSUM( std::ostream & anOstream, std::string const & options);
  void doHISTSUM( std::ostream & anOstream, std::string const & options);
  void doINSTSUM( std::ostream & anOstream, std::string const & options);
  void doINST2HIST( std::ostream & anOstream, std::string const & options);
  void doCSV( std::ostream & anOstream, std::string const & options);
  void reduce( eReduction aReduction, Measurement * aMeasurement);
  void reduceNodes();
};

class PeriodicMeasurement : public Measurement {
  typedef std::map<std::string, StatValueArrayHandle> stat_handle_map;
  stat_handle_map theStats;
  int64_t thePeriod;
  int64_t theCurrentPeriod;
  bool theCancelled;
  accumulation_type theAccumulationType;

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<Measurement>(*this);
    ar & thePeriod;
    ar & theCurrentPeriod;
    ar & theAccumulationType;
    ar & theStats;
  }
  PeriodicMeasurement( ) {}

public:
  PeriodicMeasurement( std::string const & aName, std::string const & aStatExpression, int64_t aPeriod, accumulation_type anAccumulationType);
  virtual ~PeriodicMeasurement() {}

  void addToMeasurement( Stat * aStat );
  void close();
  void print( std::ostream & anOstream, std::string const & options = std::string(""));
  void fire();
  void format( std::ostream & anOstream, std::string const & aStat, std::string const & options = std::string(""));
};

class LoggedPeriodicMeasurement : public Measurement {
  typedef std::map<std::string, StatValueHandle> stat_handle_map;
  stat_handle_map theStats;
  int64_t thePeriod;
  int64_t theCurrentPeriod;
  bool theCancelled;
  bool theFirst;
  accumulation_type theAccumulationType;
  std::ostream & theOstream;

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, uint32_t version) {
    ar & boost::serialization::base_object<Measurement>(*this);
    ar & thePeriod;
    ar & theCurrentPeriod;
    ar & theAccumulationType;
    ar & theStats;
  }
  LoggedPeriodicMeasurement( )
    : theOstream(std::cout)
  {}

public:
  LoggedPeriodicMeasurement( std::string const & aName, std::string const & aStatExpression, int64_t aPeriod, accumulation_type anAccumulationType, std::ostream & anOstream);
  virtual ~LoggedPeriodicMeasurement() {}

  void addToMeasurement( Stat * aStat );
  void close();
  void print( std::ostream & anOstream, std::string const & options = std::string(""));
  void format( std::ostream & anOstream, std::string const & aStat, std::string const & options = std::string(""));
  void fire();
};

} // end aux_
} // end Stat
} // end Flexus

#endif //FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED
