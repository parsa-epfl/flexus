//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS_CORE_STATS_HPP__INCLUDED
#define FLEXUS_CORE_STATS_HPP__INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <functional>
#include <iostream>
#include <string>

#include <core/debug/debug.hpp>
#include <core/types.hpp>

namespace Flexus {
namespace Stat {

namespace aux_ {
class StatValue_PredictionCounter;
}

class Stat; // forward declare

enum class accumulation_type { Accumulate, Reset };

class Prediction : public boost::counted_base {
  std::vector<boost::intrusive_ptr<aux_::StatValue_PredictionCounter>> theCounters;
  int64_t theUpdate;

public:
  Prediction(int64_t anUpdate);
  ~Prediction();
  void connectCounter(boost::intrusive_ptr<aux_::StatValue_PredictionCounter> aCounter);
  void confirm();
  void confirm(int64_t anOverride);
  void dismiss();
  void guess();
  void guess(int64_t anOverride);
  void goodGuess();
  void reject();
  void reject(int64_t anOverride);
};

enum class eReduction { eSum, eAverage, eStdDev, eCount };

} // namespace Stat
} // namespace Flexus

#include <core/aux_/stats/stats_aux_.hpp>

namespace Flexus {
namespace Stat {

struct StatManager {
  virtual ~StatManager() {
  }
  virtual void registerStat(Stat *) = 0;
  virtual void initialize() = 0;
  virtual void finalize() = 0;
  virtual boost::intrusive_ptr<aux_::Measurement>
  openMeasurement(std::string const &aName, std::string const &aStatSpec = std::string(".*")) = 0;
  virtual void openPeriodicMeasurement(std::string const &aName, int64_t aPeriod,
                                       accumulation_type anAccumulation,
                                       std::string const &aStatSpec = std::string(".*")) = 0;
  virtual void openLoggedPeriodicMeasurement(std::string const &aName, int64_t aPeriod,
                                             accumulation_type anAccumulation,
                                             std::ostream &anOstream,
                                             std::string const &aStatSpec = std::string(".*")) = 0;
  virtual void closeMeasurement(std::string const &aName) = 0;
  virtual void listStats(std::ostream &anOstream) = 0;
  virtual void listMeasurements(std::ostream &anOstream) = 0;
  virtual void printMeasurement(std::string const &aMeasurementSpec, std::ostream &anOstream) = 0;
  virtual void format(std::string const &aMeasurementSpec, std::string const &aFormat,
                      std::ostream &anOstream) = 0;
  virtual void formatFile(std::string const &aMeasurementSpec, std::string const &aFileName,
                          std::ostream &anOstream) = 0;
  virtual void collapse(std::string const &aMeasurementSpec, std::string const &aFormat,
                        std::ostream &anOstream) = 0;
  virtual void collapseFile(std::string const &aMeasurementSpec, std::string const &aFileName,
                            std::ostream &anOstream) = 0;
  virtual void reduce(eReduction aReduction, std::string const &aMeasurementSpec,
                      std::string const &aDestMeasurement, std::ostream &anOstream) = 0;
  virtual void saveMeasurements(std::string const &aMeasurementSpec,
                                std::string const &aFileName) const = 0;
  virtual void tick(int64_t anAdvance = 1) = 0;
  virtual int64_t ticks() = 0;
  virtual void reduceNodes(std::string const &aMeasurementSpec) = 0;
  virtual void addEvent(int64_t aDeadline, std::function<void()> anEvent) = 0;
  virtual void addFinalizer(std::function<void()> aFinalizer) = 0;
  virtual void save(std::ostream &anOstream) const = 0;
  virtual void load(std::istream &anIstream) = 0;
  virtual void loadMore(std::istream &anIstream, std::string const &aPrefix) = 0;
};

StatManager *getStatManager();

class Stat : public boost::counted_base {
  std::string theFullName;

public:
  Stat(std::string const &aName) : theFullName(aName) {
  }
  void registerStat() {
    getStatManager()->registerStat(this);
  }
  virtual ~Stat() {
  }
  std::string const &name() const {
    return theFullName;
  }
  std::string const *namePtr() const {
    return &theFullName;
  }
  virtual std::string const &type() const = 0;
  virtual aux_::StatValueHandle createValue() = 0;
  virtual aux_::StatValueArrayHandle createValueArray() = 0;
  friend std::ostream &operator<<(std::ostream &anOstream, Stat const &aStat) {
    anOstream << aStat.name();
    return anOstream;
  }
};

class StatCounter
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_Counter::update_type>> {

public:
  typedef aux_::StatValue_Counter stat_value_type;
  typedef aux_::StatValueArray_Counter stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;
  int64_t theInitialValue;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(theInitialValue));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, theInitialValue, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(
        new stat_value_array_type(theInitialValue));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type, stat_value_array_type::update_type>(
            new_value, theInitialValue, this, theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatCounter(std::string const &aName, int64_t anInitialValue = 0)
      : Stat(aName), theUpdater(0), theInitialValue(anInitialValue) {
    registerStat();
  }

  template <class Component>
  StatCounter(std::string const &aName, Component *aComponent, int64_t anInitialValue = 0)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0), theInitialValue(anInitialValue) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("Counter");
    return theType;
  }

  // Increment Counter
  StatCounter &operator++() {
    if (theUpdater)
      theUpdater->update(1);
    return *this;
  }
  StatCounter &operator++(int) {
    if (theUpdater)
      theUpdater->update(1);
    return *this;
  }

  // Decrement Counter
  StatCounter &operator--() {
    if (theUpdater)
      theUpdater->update(-1);
    return *this;
  }
  StatCounter &operator--(int) {
    if (theUpdater)
      theUpdater->update(-1);
    return *this;
  }

  // Increase Counter
  StatCounter &operator+=(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  // Decrease Counter
  StatCounter &operator-=(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(-anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

/*
class StatDoubleCounter : public Stat, public aux_::StatUpdaterLink<
aux_::StatUpdater< aux_::StatValue_DoubleCounter::update_type > > {

  public:
    typedef aux_::StatValue_DoubleCounter stat_value_type;
    typedef aux_::StatValueArray_DoubleCounter stat_value_array_type;

  // Updater Link Implementation
    private:
      typedef aux_::StatUpdater< stat_value_type::update_type > updater_type;
      updater_type * theUpdater;
      double theInitialValue;
    public:
      virtual void setNextUpdater(updater_type * aLink) {
        theUpdater = aLink;
      }

  // Interface to Measurements
    aux_::StatValueHandle createValue() {
      boost::intrusive_ptr<stat_value_type> new_value(new
stat_value_type(theInitialValue)); boost::intrusive_ptr<updater_type>
new_updater(new aux_::SimpleStatUpdater<stat_value_type,
stat_value_type::update_type>(new_value, theInitialValue, this, theUpdater ) );
      return aux_::StatValueHandle( this, new_value, new_updater );
    }
    aux_::StatValueArrayHandle createValueArray() {
      boost::intrusive_ptr<stat_value_array_type> new_value(new
stat_value_array_type(theInitialValue)); boost::intrusive_ptr<updater_type>
new_updater(new aux_::SimpleStatUpdater<stat_value_array_type,
stat_value_array_type::update_type>(new_value, theInitialValue, this, theUpdater
) ); return aux_::StatValueArrayHandle( this, new_value, new_updater );
    }

  public:
    //Create a counter with a specific name
    StatDoubleCounter( std::string const & aName, double anInitialValue = 0 )
      : Stat( aName )
      , theUpdater(0)
      , theInitialValue(anInitialValue)
      {
        registerStat();
      }

    template <class Component>
    StatDoubleCounter( std::string const & aName , Component * aComponent,
int64_t anInitialValue = 0) : Stat( aComponent->statName() + "-" + aName) ,
theUpdater(0) , theInitialValue(anInitialValue)
      {
        registerStat();
      }

    std::string const & type() const { static std::string theType("Counter");
return theType; }

    //Increment Counter
    StatDoubleCounter & operator ++() { if (theUpdater) theUpdater->update(1);
return *this; } StatDoubleCounter & operator ++(int) { if (theUpdater)
theUpdater->update(1); return *this; }

    //Decrement Counter
    StatDoubleCounter & operator --() { if (theUpdater) theUpdater->update(-1);
return *this; } StatDoubleCounter & operator --(int) { if (theUpdater)
theUpdater->update(-1); return *this; }

    //Increase Counter
    StatDoubleCounter & operator +=( stat_value_type::update_type anUpdate) { if
(theUpdater) theUpdater->update(anUpdate); return *this; }

    //Decrease Counter
    StatDoubleCounter & operator -=( stat_value_type::update_type anUpdate) { if
(theUpdater) theUpdater->update(-anUpdate); return *this; }

    bool enabled() { return true; }

};
*/

class StatPredictionCounter
    : public Stat,
      public aux_::StatUpdaterLink<
          aux_::StatUpdater<aux_::StatValue_PredictionCounter::update_type>> {

public:
  typedef aux_::StatValue_PredictionCounter stat_value_type;
  typedef aux_::StatValue_PredictionCounter stat_value_array_type; // Not supported

  // Updater Link Implementation
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    DBG_Assert(false);
    return aux_::StatValueArrayHandle(0, 0, 0); // suppress warning
  }

public:
  // Create a counter with a specific name
  StatPredictionCounter(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatPredictionCounter(std::string const &aName, Component *aComponent, int64_t anInitialValue = 0)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("PredictionCounter");
    return theType;
  }

  // Increment Counter
  boost::intrusive_ptr<Prediction> predict(int64_t anIncrement = 1) {
    boost::intrusive_ptr<Prediction> ret_val = new Prediction(anIncrement);
    if (theUpdater) {
      theUpdater->update(*ret_val);
    }
    return ret_val;
  }

  bool enabled() {
    return true;
  }
};

class StatMax : public Stat,
                public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_Max::update_type>> {

public:
  typedef aux_::StatValue_Max stat_value_type;
  typedef aux_::StatValueArray_Max stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type, stat_value_array_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatMax(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatMax(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("Max");
    return theType;
  }

  // Increment Counter
  StatMax &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatAnnotation
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_Annotation::update_type>> {

public:
  typedef aux_::StatValue_Annotation stat_value_type;
  typedef aux_::StatValueArray_Annotation stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(theInitialValue));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, theInitialValue, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type(""));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type, stat_value_array_type::update_type>(
            new_value, theInitialValue, this, theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

  std::string theInitialValue;

public:
  // Create a counter with a specific name
  StatAnnotation(std::string const &aName, std::string const &aValue = "")
      : Stat(aName), theUpdater(0), theInitialValue(aValue) {
    registerStat();
  }

  template <class Component>
  StatAnnotation(std::string const &aName, Component *aComponent, std::string const &aValue = "")
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0), theInitialValue(aValue) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("Annotation");
    return theType;
  }

  // Increment Counter
  StatAnnotation &operator=(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    theInitialValue = anUpdate;
    return *this;
  }
  StatAnnotation &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatAverage
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_Average::update_type>> {

public:
  typedef aux_::StatValue_Average stat_value_type;
  typedef aux_::StatValueArray_Average stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type, stat_value_array_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatAverage(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatAverage(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("Average");
    return theType;
  }

  // Increment Counter
  StatAverage &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
  StatAverage &operator<<(stat_value_type::update_type::first_type anUpdate) {
    if (theUpdater)
      theUpdater->update(std::make_pair(anUpdate, 1));
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatStdDev
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_StdDev::update_type>> {

public:
  typedef aux_::StatValue_StdDev stat_value_type;
  typedef aux_::StatValueArray_StdDev stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type, stat_value_array_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatStdDev(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatStdDev(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("StdDev");
    return theType;
  }

  // Increment Counter
  StatStdDev &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatLog2Histogram
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<aux_::StatValue_Log2Histogram::update_type>> {

public:
  typedef aux_::StatValue_Log2Histogram stat_value_type;
  typedef aux_::StatValue_Log2Histogram stat_value_array_type; // Arrays not supported

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    DBG_Assert(0);
    return aux_::StatValueArrayHandle(0, 0, 0); // Suppress warning
  }

public:
  // Create a counter with a specific name
  StatLog2Histogram(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatLog2Histogram(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("Log2Histogram");
    return theType;
  }

  // Increment Counter
  StatLog2Histogram &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
  // Decrement Counter
  StatLog2Histogram &operator>>(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(-1LL * anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatWeightedLog2Histogram
    : public Stat,
      public aux_::StatUpdaterLink<
          aux_::StatUpdater<aux_::StatValue_WeightedLog2Histogram::update_type>> {

public:
  typedef aux_::StatValue_WeightedLog2Histogram stat_value_type;
  typedef aux_::StatValue_WeightedLog2Histogram stat_value_array_type; // Arrays not supported

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    DBG_Assert(0);
    return aux_::StatValueArrayHandle(0, 0, 0); // Suppress warning
  }

public:
  // Create a counter with a specific name
  StatWeightedLog2Histogram(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatWeightedLog2Histogram(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("WeightedLog2Histogram");
    return theType;
  }

  // Increment Counter
  StatWeightedLog2Histogram &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
  // Decrement Counter
  StatWeightedLog2Histogram &operator>>(stat_value_type::update_type anUpdate) {
    anUpdate.second = -1LL * anUpdate.second;
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

class StatStdDevLog2Histogram
    : public Stat,
      public aux_::StatUpdaterLink<
          aux_::StatUpdater<aux_::StatValue_StdDevLog2Histogram::update_type>> {

public:
  typedef aux_::StatValue_StdDevLog2Histogram stat_value_type;
  typedef aux_::StatValue_StdDevLog2Histogram stat_value_array_type; // Arrays not supported

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type(0));
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    DBG_Assert(0);
    return aux_::StatValueArrayHandle(0, 0, 0); // Suppress warning
  }

public:
  // Create a counter with a specific name
  StatStdDevLog2Histogram(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatStdDevLog2Histogram(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("StdDevLog2Histogram");
    return theType;
  }

  // Increment Counter
  StatStdDevLog2Histogram &operator<<(stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
  bool enabled() {
    return true;
  }
};

template <class CountedValueType>
class StatUniqueCounter : public Stat,
                          public aux_::StatUpdaterLink<aux_::StatUpdater<CountedValueType>> {

public:
  typedef aux_::StatValue_UniqueCounter<CountedValueType> stat_value_type;
  typedef aux_::StatValueArray_UniqueCounter<CountedValueType> stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<typename stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type);
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, typename stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type);
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type,
                                    typename stat_value_array_type::update_type>(new_value, 0, this,
                                                                                 theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatUniqueCounter(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatUniqueCounter(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("UniqueCounter");
    return theType;
  }

  // Count operator
  StatUniqueCounter &operator<<(typename stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }

  bool enabled() {
    return true;
  }
};

template <class CountedValueType>
class StatInstanceCounter
    : public Stat,
      public aux_::StatUpdaterLink<aux_::StatUpdater<
          typename aux_::StatValue_InstanceCounter<CountedValueType>::update_type>> {

public:
  typedef aux_::StatValue_InstanceCounter<CountedValueType> stat_value_type;
  typedef aux_::StatValueArray_InstanceCounter<CountedValueType> stat_value_array_type;

  // Updater Link Implementation
private:
  typedef aux_::StatUpdater<typename stat_value_type::update_type> updater_type;
  updater_type *theUpdater;

public:
  virtual void setNextUpdater(updater_type *aLink) {
    theUpdater = aLink;
  }

  // Interface to Measurements
  aux_::StatValueHandle createValue() {
    boost::intrusive_ptr<stat_value_type> new_value(new stat_value_type);
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_type, typename stat_value_type::update_type>(
            new_value, 0, this, theUpdater));
    return aux_::StatValueHandle(this, new_value, new_updater);
  }
  aux_::StatValueArrayHandle createValueArray() {
    boost::intrusive_ptr<stat_value_array_type> new_value(new stat_value_array_type);
    boost::intrusive_ptr<updater_type> new_updater(
        new aux_::SimpleStatUpdater<stat_value_array_type,
                                    typename stat_value_array_type::update_type>(new_value, 0, this,
                                                                                 theUpdater));
    return aux_::StatValueArrayHandle(this, new_value, new_updater);
  }

public:
  // Create a counter with a specific name
  StatInstanceCounter(std::string const &aName) : Stat(aName), theUpdater(0) {
    registerStat();
  }

  template <class Component>
  StatInstanceCounter(std::string const &aName, Component *aComponent)
      : Stat(aComponent->statName() + "-" + aName), theUpdater(0) {
    registerStat();
  }

  std::string const &type() const {
    static std::string theType("InstanceCounter");
    return theType;
  }

  bool enabled() {
    return true;
  }

  // Count operator
  StatInstanceCounter &operator<<(typename stat_value_type::update_type anUpdate) {
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
  // Remove operator
  StatInstanceCounter &operator>>(typename stat_value_type::update_type anUpdate) {
    anUpdate.second = -1LL * anUpdate.second;
    if (theUpdater)
      theUpdater->update(anUpdate);
    return *this;
  }
};

} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_STATS_HPP__INCLUDED
