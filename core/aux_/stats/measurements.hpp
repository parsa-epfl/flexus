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
#ifndef FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
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

class Measurement : public boost::counted_base
{
    std::string theName;
    std::string theStatExpressionStr;
    boost::regex theStatExpression;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, uint32_t version) const
    {
        ar & theName;
        std::string temp(theStatExpressionStr);
        ar & temp;
    }
    template<class Archive>
    void load(Archive& ar, uint32_t version)
    {
        ar & theName;
        std::string temp;
        ar & temp;
        theStatExpressionStr = temp;
        theStatExpression    = temp;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
  protected:
    Measurement() {}

  public:
    Measurement(std::string const& aName, std::string const& aStatExpression)
      : theName(aName)
      , theStatExpressionStr(aStatExpression)
      , theStatExpression(aStatExpression)
    {
    }
    virtual ~Measurement() {}
    bool includeStat(Stat* aStat);
    std::string const& name() { return theName; }
    void resetName(std::string const& aName) { theName = aName; }
    friend std::ostream& operator<<(std::ostream& anOstream, Measurement const& aMeasurement)
    {
        anOstream << aMeasurement.theName;
        return anOstream;
    }
    virtual bool isSimple() { return false; }
    virtual void print(std::ostream& anOstream, std::string const& options = std::string("")) {}
    virtual void addToMeasurement(Stat* aStat) {}
    virtual void format(std::ostream& anOstream, std::string const& aStat, std::string const& options = std::string(""))
    {
    }
    virtual void doOp(std::ostream& anOstream, std::string const& anOp, std::string const& options)
    {
        if (anOp == "MSMT") {
            doMSMT(anOstream);
        } else {
            anOstream << "{ERR:Op Not Supported}";
        }
    }
    void doMSMT(std::ostream& anOstream) { anOstream << name(); }
    virtual void reduce(eReduction aReduction, Measurement* aMeasurement) {}
    virtual void reduceNodes() {}

    virtual void close() {}
};

class SimpleMeasurement : public Measurement
{
    typedef std::map<std::string, StatValueHandle> stat_handle_map;
    stat_handle_map theStats;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<Measurement>(*this);
        ar & theStats;
    }
    SimpleMeasurement() {}

  public:
    SimpleMeasurement(std::string const& aName, std::string const& aStatExpression)
      : Measurement(aName, aStatExpression)
    {
    }
    virtual ~SimpleMeasurement() {}

    void addToMeasurement(Stat* aStat);
    void close();
    bool isSimple() { return true; }
    void print(std::ostream& anOstream, std::string const& options = std::string(""));
    void format(std::ostream& anOstream, std::string const& aStat, std::string const& options = std::string(""));
    int64_t asLongLong(std::string const& aStat);
    double asDouble(std::string const& aStat);
    int64_t sumAsLongLong(std::string const& aStatFilter);
    int64_t minAsLongLong(std::string const& aStatFilter);
    int64_t maxAsLongLong(std::string const& aStatFilter);
    double avgAsDouble(std::string const& aStatFilter);
    void doOp(std::ostream& anOstream, std::string const& anOp, std::string const& options);
    void doSUM(std::ostream& anOstream, std::string const& options);
    void doHISTSUM(std::ostream& anOstream, std::string const& options);
    void doINSTSUM(std::ostream& anOstream, std::string const& options);
    void doINST2HIST(std::ostream& anOstream, std::string const& options);
    void doCSV(std::ostream& anOstream, std::string const& options);
    void reduce(eReduction aReduction, Measurement* aMeasurement);
    void reduceNodes();
};

class PeriodicMeasurement : public Measurement
{
    typedef std::map<std::string, StatValueArrayHandle> stat_handle_map;
    stat_handle_map theStats;
    int64_t thePeriod;
    int64_t theCurrentPeriod;
    bool theCancelled;
    accumulation_type theAccumulationType;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<Measurement>(*this);
        ar & thePeriod;
        ar & theCurrentPeriod;
        ar & theAccumulationType;
        ar & theStats;
    }
    PeriodicMeasurement() {}

  public:
    PeriodicMeasurement(std::string const& aName,
                        std::string const& aStatExpression,
                        int64_t aPeriod,
                        accumulation_type anAccumulationType);
    virtual ~PeriodicMeasurement() {}

    void addToMeasurement(Stat* aStat);
    void close();
    void print(std::ostream& anOstream, std::string const& options = std::string(""));
    void fire();
    void format(std::ostream& anOstream, std::string const& aStat, std::string const& options = std::string(""));
};

class LoggedPeriodicMeasurement : public Measurement
{
    typedef std::map<std::string, StatValueHandle> stat_handle_map;
    stat_handle_map theStats;
    int64_t thePeriod;
    int64_t theCurrentPeriod;
    bool theCancelled;
    bool theFirst;
    accumulation_type theAccumulationType;
    std::ostream& theOstream;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<Measurement>(*this);
        ar & thePeriod;
        ar & theCurrentPeriod;
        ar & theAccumulationType;
        ar & theStats;
    }
    LoggedPeriodicMeasurement()
      : theOstream(std::cout)
    {
    }

  public:
    LoggedPeriodicMeasurement(std::string const& aName,
                              std::string const& aStatExpression,
                              int64_t aPeriod,
                              accumulation_type anAccumulationType,
                              std::ostream& anOstream);
    virtual ~LoggedPeriodicMeasurement() {}

    void addToMeasurement(Stat* aStat);
    void close();
    void print(std::ostream& anOstream, std::string const& options = std::string(""));
    void format(std::ostream& anOstream, std::string const& aStat, std::string const& options = std::string(""));
    void fire();
};

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_MEASUREMENTS__HPP__INCLUDED
