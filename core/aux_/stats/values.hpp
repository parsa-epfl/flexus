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
#ifndef FLEXUS_CORE_AUX__STATS_VALUES__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_VALUES__HPP__INCLUDED

#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/boost_extensions/lexical_cast.hpp>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

namespace Flexus {
namespace Stat {
namespace aux_ {

class StatValue_AvgAccumulator;
class StatValue_StdDevAccumulator;

class StatValue_Counter : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &theValue;
  }

public:
  StatValue_Counter() {
  }

  friend class StatValue_AvgAccumulator;
  friend class StatValue_StdDevAccumulator;

public:
  typedef int64_t update_type;
  typedef int64_t value_type;

private:
  value_type theValue;

public:
  StatValue_Counter(value_type aValue) : theValue(aValue) {
  }

  void reduceSum(StatValueBase const &aBase) {
    StatValue_Counter const &ctr = dynamic_cast<StatValue_Counter const &>(aBase);
    reduceSum(ctr);
  }
  void reduceSum(StatValue_Counter const &aCounter) {
    theValue += aCounter.theValue;
  }

  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_Counter(*this);
  }
  boost::intrusive_ptr<StatValueBase> avgAccumulator();
  boost::intrusive_ptr<StatValueBase> stdevAccumulator();

  void update(update_type anUpdate) {
    theValue += anUpdate;
  }
  void reset(value_type aValue) {
    theValue = aValue;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    anOstream << theValue;
  }
  int64_t asLongLong() const {
    return theValue;
  };
};

/*
  class StatValue_DoubleCounter : public StatValueBase {
    private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, uint32_t version) {
          ar & boost::serialization::base_object<StatValueBase>(*this);
          ar & theValue;
      }
      StatValue_DoubleCounter() {}

      friend class StatValue_AvgAccumulator;
      friend class StatValue_StdDevAccumulator;
    public:
      typedef double update_type;
      typedef double value_type;
    private:
      value_type theValue;
    public:
      StatValue_DoubleCounter(value_type aValue)
        : theValue(aValue)
        {}

      void reduceSum( StatValueBase const & aBase) {
        StatValue_DoubleCounter const & ctr =
  dynamic_cast<StatValue_DoubleCounter const &>(aBase); reduceSum(ctr);
      }
      void reduceSum( StatValue_DoubleCounter const & aCounter) {
        theValue += aCounter.theValue;
      }

      boost::intrusive_ptr<StatValueBase> sumAccumulator() { return new
  StatValue_DoubleCounter(*this); } boost::intrusive_ptr<StatValueBase>
  avgAccumulator(); boost::intrusive_ptr<StatValueBase> stdevAccumulator();

      void update( update_type anUpdate ) {
        theValue += anUpdate;
      }
      void reset( value_type aValue) {
        theValue = aValue;
      }
      void print(std::ostream & anOstream, std::string const & options =
  std::string("")) const { anOstream << theValue;
      }
      int64_t asLongLong() const { return static_cast<int64_t>( theValue ); };
  };
*/

class StatValue_PredictionCounter : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &theValue;
    ar &thePending;
  }
  StatValue_PredictionCounter() {
  }

public:
  typedef Prediction &update_type;
  typedef int64_t value_type;

private:
  value_type theValue;
  value_type thePending;

public:
  StatValue_PredictionCounter(value_type aValue) : theValue(aValue) {
  }
  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_PredictionCounter(*this);
  }
  void reduceSum(StatValueBase const &aBase) {
    const StatValue_PredictionCounter &ptr =
        dynamic_cast<const StatValue_PredictionCounter &>(aBase);
    reduceSum(ptr);
  }
  void reduceSum(const StatValue_PredictionCounter &aCounter) {
    theValue += aCounter.theValue;
    thePending += aCounter.thePending;
  }
  void update(update_type anUpdate) {
    anUpdate.connectCounter(this);
    thePending++;
  }
  void confirm(value_type anUpdate) {
    theValue += anUpdate;
    thePending--;
  }
  void dismiss() {
    thePending--;
  }
  void reset(value_type aValue) {
    theValue = aValue;
  }
  void guess(value_type anUpdate) {
    theValue += anUpdate;
  }
  void goodGuess() {
    thePending--;
  }
  void reject(value_type anUpdate) {
    theValue -= anUpdate;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    if (options == "pending") {
      anOstream << thePending;
    }
    anOstream << theValue;
  }
  int64_t asLongLong() const {
    return theValue;
  };
};

class StatValue_Annotation : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &theValue;
  }
  StatValue_Annotation() {
  }

public:
  typedef std::string update_type;
  typedef std::string value_type;

private:
  value_type theValue;

public:
  StatValue_Annotation(value_type aValue) : theValue(aValue) {
  }

  void reduceSum(StatValueBase const &aBase) {
    const StatValue_Annotation &ptr = dynamic_cast<const StatValue_Annotation &>(aBase);
    reduceSum(ptr);
  }
  void reduceAvg(StatValueBase const &aBase) {
    const StatValue_Annotation &ptr = dynamic_cast<const StatValue_Annotation &>(aBase);
    reduceSum(ptr); // Same as sum for annotations
  }
  void reduceSum(const StatValue_Annotation &other) {
    if (other.theValue.length() > theValue.length()) {
      theValue = other.theValue;
    }
  }

  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_Annotation(*this);
  }
  boost::intrusive_ptr<StatValueBase> avgAccumulator() {
    return new StatValue_Annotation(*this);
  }

  void update(update_type anUpdate) {
    theValue = anUpdate;
  }
  void reset(value_type aValue) {
    theValue = aValue;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    anOstream << theValue;
  }
  int64_t asLongLong() const {
    return 0;
  };
};

class StatValue_Max : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &theValue;
    ar &theIsValid;
  }

public:
  StatValue_Max() {
  }

public:
  typedef int64_t update_type;
  typedef int64_t value_type;

private:
  value_type theValue;
  bool theIsValid;

public:
  StatValue_Max(value_type /*ignored*/) : theValue(0), theIsValid(false) {
  }
  void reduceSum(const StatValueBase &aBase) {
    const StatValue_Max &ptr = dynamic_cast<const StatValue_Max &>(aBase);
    reduceSum(ptr);
  }
  void reduceSum(const StatValue_Max &aMax) {
    if (!aMax.theIsValid) {
      return;
    }
    if (!theIsValid) {
      theIsValid = true;
      theValue = aMax.theValue;
      return;
    }
    if (aMax.theValue > theValue) {
      theValue = aMax.theValue;
    }
  }

  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_Max(*this);
  }

  void update(update_type anUpdate) {
    if (!theIsValid) {
      theIsValid = true;
      theValue = anUpdate;
      return;
    }
    if (anUpdate > theValue) {
      theValue = anUpdate;
    }
  }
  void reset(value_type /*ignored*/) {
    theIsValid = false;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    if (theIsValid) {
      anOstream << theValue;
    } else {
      anOstream << "(none)";
    }
  }
  int64_t asLongLong() const {
    if (theIsValid) {
      return theValue;
    } else {
      return 0;
    }
  };
};

class StatValue_Average : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &theTotal;
    ar &theCount;
  }

public:
  StatValue_Average() {
  }

  friend class StatValue_AvgAccumulator;
  friend class StatValue_StdDevAccumulator;

public:
  typedef std::pair<int64_t, int64_t> update_type;
  typedef int64_t value_type;

private:
  value_type theTotal;
  value_type theCount;

public:
  StatValue_Average(value_type /*ignored*/) : theTotal(0), theCount(0) {
  }

  void reduceSum(const StatValueBase &aBase) {
    const StatValue_Average &ptr = dynamic_cast<const StatValue_Average &>(aBase);
    reduceSum(ptr);
  }
  void reduceSum(const StatValue_Average &anAverage) {
    theTotal += anAverage.theTotal;
    theCount += anAverage.theCount;
  }
  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_Average(*this);
  }
  boost::intrusive_ptr<StatValueBase> avgAccumulator();
  boost::intrusive_ptr<StatValueBase> stdevAccumulator();

  void update(update_type anUpdate) {
    theTotal += anUpdate.first * anUpdate.second;
    theCount += anUpdate.second;
  }
  void reset(value_type /*ignored*/) {
    theTotal = 0;
    theCount = 0;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    if (theCount > 0) {
      anOstream << static_cast<double>(theTotal) / theCount;
    } else {
      anOstream << "{nan}";
    }
  }
  int64_t asLongLong() const {
    if (theCount > 0) {
      return theTotal / theCount;
    } else {
      return 0;
    }
  };
  double asDouble() const {
    if (theCount > 0) {
      return static_cast<double>(theTotal) / theCount;
    } else {
      return 0;
    }
  };
};

// Despite the fact that this implementation is wacked and wastes storage,
// we will keep it for compatability with previous stat files.
// A better implementation is available at:
// http://www.answers.com/topic/algorithms-for-calculating-variance
class StatValue_StdDev : public StatValueBase {
private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, uint32_t version) {
    ar &boost::serialization::base_object<StatValueBase>(*this);
    ar &k;
    ar &Sum;
    ar &SumSq;
    ar &SigmaSqSum;
  }

public:
  StatValue_StdDev() {
  }

public:
  typedef int64_t update_type;
  typedef double value_type;

private:
  uint64_t k;
  double Sum;
  double SumSq;
  double SigmaSqSum;

public:
  StatValue_StdDev(value_type /*ignored*/) : k(0), Sum(0.0), SumSq(0.0), SigmaSqSum(0.0) {
  }
  void reduceSum(const StatValueBase &aBase) {
    const StatValue_StdDev &ptr = dynamic_cast<const StatValue_StdDev &>(aBase);
    reduceSum(ptr);
  }
  void reduceSum(const StatValue_StdDev &aStdDev) {
    // First, compute the variance that results from summing the two
    Sum = Sum + aStdDev.Sum;
    SumSq = SumSq + aStdDev.SumSq;
    k = k + aStdDev.k;

    double variance = SumSq / k - (Sum / k) * (Sum / k);
    SigmaSqSum = variance * k * k;
  }

  boost::intrusive_ptr<StatValueBase> sumAccumulator() {
    return new StatValue_StdDev(*this);
  }

  void update(update_type anUpdate) {
    const double Xsq = anUpdate * anUpdate;

    SigmaSqSum += SumSq + k * Xsq - 2 * anUpdate * Sum;

    k++;
    Sum += anUpdate;
    SumSq += Xsq;
  }
  void reset(value_type /*ignored*/) {
    k = 0;
    Sum = 0;
    SumSq = 0;
    SigmaSqSum = 0;
  }
  void print(std::ostream &anOstream, std::string const &options = std::string("")) const {
    if (k == 0) {
      anOstream << "{nan}";
    } else {
      anOstream << std::sqrt(SigmaSqSum) / static_cast<double>(k);
    }
  }
  int64_t asLongLong() const {
    if (k == 0) {
      return 0;
    } else {
      return static_cast<int64_t>(sqrt(SigmaSqSum) / static_cast<double>(k));
    }
  }
  double asDouble() const {
    if (k == 0) {
      return 0;
    } else {
      return sqrt(SigmaSqSum) / static_cast<double>(k);
    }
  }
};

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_VALUES__HPP__INCLUDED
