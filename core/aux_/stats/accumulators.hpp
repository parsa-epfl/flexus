#ifndef FLEXUS_CORE_AUX__STATS_ACCUMULATORS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_ACCUMULATORS__HPP__INCLUDED

#include "base.hpp"
#include "values.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <cmath>
#include <core/boost_extensions/intrusive_ptr.hpp>
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

class StatValue_CountAccumulator : public StatValueBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueBase>(*this);
        ar & theCount;
    }

  public:
    typedef std::pair<int64_t, int64_t> update_type;
    typedef int64_t value_type;

  private:
    value_type theCount;

  public:
    StatValue_CountAccumulator()
      : theCount(1)
    {
    }
    boost::intrusive_ptr<StatValueBase> countAccumulator() { return this; }
    boost::intrusive_ptr<StatValueBase> sumAccumulator() { return this; }

    void reduceSum(const StatValueBase& aBase)
    {
        const StatValue_CountAccumulator& ptr = dynamic_cast<const StatValue_CountAccumulator&>(aBase);
        reduceSum(ptr);
    }
    void reduceSum(const StatValue_CountAccumulator& anAverage)
    {
        if (anAverage.theCount > theCount) { theCount = anAverage.theCount; }
    }

    void reduceCount(const StatValueBase& aBase) { ++theCount; }

    void update(update_type anUpdate) { ++theCount; }
    void reset(value_type /*ignored*/) { theCount = 0; }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const { anOstream << theCount; }
    int64_t asLongLong() const { return theCount; };
};

inline boost::intrusive_ptr<StatValueBase>
StatValueBase::countAccumulator()
{
    return new StatValue_CountAccumulator();
}

class StatValue_AvgAccumulator : public StatValueBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueBase>(*this);
        ar & theTotal;
        ar & theCount;
    }
    friend class StatValue_StdDevAccumulator;
    StatValue_AvgAccumulator() {}

  public:
    typedef std::pair<int64_t, int64_t> update_type;
    typedef int64_t value_type;

  private:
    value_type theTotal;
    value_type theCount;

  public:
    StatValue_AvgAccumulator(value_type total)
      : theTotal(total)
      , theCount(1)
    {
    }
    boost::intrusive_ptr<StatValueBase> avgAccumulator() { return this; }

    void reduceSum(const StatValueBase& aBase)
    {
        const StatValue_AvgAccumulator& ptr = dynamic_cast<const StatValue_AvgAccumulator&>(aBase);
        reduceSum(ptr);
    }
    void reduceSum(const StatValue_AvgAccumulator& anAverage)
    {
        theTotal += anAverage.theTotal;
        if (anAverage.theCount > theCount) { theCount = anAverage.theCount; }
    }
    void reduceAvg(const StatValueBase& aBase)
    {
        try {
            const StatValue_Counter& ptr = dynamic_cast<const StatValue_Counter&>(aBase);
            reduceAvg(ptr);
            return;
        } catch (...) {
        }
        try {
            const StatValue_Average& ptr = dynamic_cast<const StatValue_Average&>(aBase);
            reduceAvg(ptr);
            return;
        } catch (...) {
        }
        try {
            const StatValue_AvgAccumulator& ptr = dynamic_cast<const StatValue_AvgAccumulator&>(aBase);
            reduceAvg(ptr);
            return;
        } catch (...) {
        }
        throw 1;
    }
    void reduceAvg(const StatValue_Counter& anAverage)
    {
        theTotal += anAverage.theValue;
        ++theCount;
    }
    void reduceAvg(const StatValue_Average& anAverage)
    {
        theTotal += anAverage.theTotal;
        theCount += anAverage.theCount;
    }
    void reduceAvg(const StatValue_AvgAccumulator& anAverage)
    {
        theTotal += anAverage.theTotal;
        theCount += anAverage.theCount;
    }

    void update(update_type anUpdate)
    {
        theTotal += anUpdate.first * anUpdate.second;
        theCount += anUpdate.second;
    }
    void reset(value_type /*ignored*/)
    {
        theTotal = 0;
        theCount = 0;
    }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        if (theCount > 0) {
            anOstream << static_cast<double>(theTotal) / theCount;
        } else {
            anOstream << "{nan}";
        }
    }
    int64_t asLongLong() const
    {
        if (theCount > 0) {
            return theTotal / theCount;
        } else {
            return 0;
        }
    }
    double asDouble() const
    {
        if (theCount > 0) {
            return static_cast<double>(theTotal) / theCount;
        } else {
            return 0;
        }
    }
};

inline boost::intrusive_ptr<StatValueBase>
StatValue_Counter::avgAccumulator()
{
    return new StatValue_AvgAccumulator(theValue);
}
inline boost::intrusive_ptr<StatValueBase>
StatValue_Average::avgAccumulator()
{
    return new StatValue_AvgAccumulator(theTotal);
}

class StatValue_StdDevAccumulator : public StatValueBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueBase>(*this);
        ar & k;
        ar & Sum;
        ar & SumSq;
        ar & SigmaSqSum;
    }

  public:
    typedef double update_type;
    typedef double value_type;

  private:
    uint64_t k;
    double Sum;
    double SumSq;
    double SigmaSqSum;

  public:
    StatValue_StdDevAccumulator()
      : k(0)
      , Sum(0.0)
      , SumSq(0.0)
      , SigmaSqSum(0.0)
    {
    }
    void reduceSum(const StatValueBase& aBase)
    {
        k          = 0;
        Sum        = 0;
        SumSq      = 0;
        SigmaSqSum = 0;
    }

    void reduceStdDev(const StatValueBase& aBase)
    {
        try {
            const StatValue_Counter& ptr = dynamic_cast<const StatValue_Counter&>(aBase);
            reduceStdDev(ptr);
            return;
        } catch (...) {
        }
        try {
            const StatValue_Average& ptr = dynamic_cast<const StatValue_Average&>(aBase);
            reduceStdDev(ptr);
            return;
        } catch (...) {
        }
        try {
            const StatValue_AvgAccumulator& ptr = dynamic_cast<const StatValue_AvgAccumulator&>(aBase);
            reduceStdDev(ptr);
            return;
        } catch (...) {
        }
        throw 1;
    }
    void reduceStdDev(const StatValue_Counter& aCount) { update(aCount.theValue); }
    void reduceStdDev(const StatValue_Average& anAverage)
    {
        update(static_cast<double>(anAverage.theTotal) / anAverage.theCount);
    }
    void reduceStdDev(const StatValue_AvgAccumulator& anAverage)
    {
        update(static_cast<double>(anAverage.theTotal) / anAverage.theCount);
    }

    void update(update_type anUpdate)
    {
        const double Xsq = anUpdate * anUpdate;

        SigmaSqSum += SumSq + k * Xsq - 2 * anUpdate * Sum;

        k++;
        Sum += anUpdate;
        SumSq += Xsq;
    }
    void reset(value_type /*ignored*/)
    {
        k          = 0;
        Sum        = 0;
        SumSq      = 0;
        SigmaSqSum = 0;
    }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        if (k == 0) {
            anOstream << "{nan}";
        } else {
            anOstream << std::sqrt(SigmaSqSum / k) / sqrt(k - 1);
        }
    }
    int64_t asLongLong() const
    {
        if (k <= 1) {
            return 0;
        } else {
            return static_cast<int64_t>(sqrt(SigmaSqSum / k) / sqrt(k - 1));
        }
    }
    double asDouble() const
    {
        if (k <= 1) {
            return 0;
        } else {
            return sqrt(SigmaSqSum / k) / sqrt(k - 1);
        }
    }
};

inline boost::intrusive_ptr<StatValueBase>
StatValue_Counter::stdevAccumulator()
{
    StatValue_StdDevAccumulator* stdev = new StatValue_StdDevAccumulator();
    stdev->update(theValue);
    return stdev;
}
inline boost::intrusive_ptr<StatValueBase>
StatValue_Average::stdevAccumulator()
{
    StatValue_StdDevAccumulator* stdev = new StatValue_StdDevAccumulator();
    stdev->update(static_cast<double>(theTotal) / theCount);
    return stdev;
}

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_ACCUMULATORS__HPP__INCLUDED