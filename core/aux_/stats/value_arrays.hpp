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
#ifndef FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED

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

class StatValueArray_Counter : public StatValueArrayBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueArrayBase>(*this);
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
      : theInitialValue(aValue)
    {
        theValues.push_back(simple_type(theInitialValue));
    }

    // Used by serialization of other stat types
    StatValueArray_Counter(std::vector<simple_type> const& aValueVector, value_type aCurrentValue)
      : theValues(aValueVector)
    {
        theValues.push_back(simple_type(aCurrentValue));
    }

    void reduceSum(const StatValueBase& aBase)
    {
        std::cerr << "reductions not supported (StatValueArray_Counter)" << std::endl;
    }
    void update(update_type anUpdate) { theValues.back().update(anUpdate); }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
            anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
    }
    void newValue(accumulation_type aValueType)
    {
        if (aValueType == accumulation_type::Accumulate) {
            theValues.push_back(theValues.back());
        } else {
            theValues.push_back(simple_type(theInitialValue));
        }
    }
    void reset(value_type aValue)
    {
        theValues.clear();
        theValues.push_back(aValue);
    }
    StatValueBase& operator[](int32_t anIndex) { return theValues[anIndex]; }
    std::size_t size() { return theValues.size(); }
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
      StatValueArray_DoubleCounter( std::vector<simple_type> const &
  aValueVector, value_type aCurrentValue) : theValues(aValueVector)
      {
        theValues.push_back(simple_type(aCurrentValue));
      }

      void reduceSum( const StatValueBase & aBase) {
        std::cerr << "reductions not supported (StatValueArray_Counter)" <<
  std::endl;
      }
      void update( update_type anUpdate ) {
        theValues.back().update(anUpdate);
      }
      void print(std::ostream & anOstream, std::string const & options =
  std::string("")) const { for (int32_t i = 0; i <
  static_cast<int>(theValues.size()) - 1; ++i) { anOstream << theValues[i] << ",
  ";
        }
        anOstream << theValues.back();
      }
      void newValue(accumulation_type aValueType) {
        if (aValueType == accumulation_type::Accumulate) {
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

class StatValueArray_Annotation : public StatValueArrayBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueArrayBase>(*this);
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
      : theInitialValue(aValue)
    {
        theValues.push_back(simple_type(theInitialValue));
    }

    // Used by serialization of other stat types
    StatValueArray_Annotation(std::vector<simple_type> const& aValueVector, value_type aCurrentValue)
      : theValues(aValueVector)
    {
        theValues.push_back(simple_type(aCurrentValue));
    }

    void reduceSum(const StatValueBase& aBase)
    {
        std::cerr << "reductions not supported (StatValueArray_Annotation)" << std::endl;
    }
    void update(update_type anUpdate) { theValues.back().update(anUpdate); }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
            anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
    }
    void newValue(accumulation_type aValueType)
    {
        if (aValueType == accumulation_type::Accumulate) {
            theValues.push_back(theValues.back());
        } else {
            theValues.push_back(simple_type(theInitialValue));
        }
    }
    void reset(value_type aValue)
    {
        theValues.clear();
        theValues.push_back(aValue);
    }
    StatValueBase& operator[](int32_t anIndex) { return theValues[anIndex]; }
    std::size_t size() { return theValues.size(); }
};

class StatValueArray_Max : public StatValueArrayBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueArrayBase>(*this);
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
    StatValueArray_Max(value_type /*ignored*/) { theValues.push_back(simple_type(0)); }
    void reduceSum(const StatValueBase& aBase)
    {
        std::cerr << "reductions not supported (StatValueArray_Max)" << std::endl;
    }
    void update(update_type anUpdate) { theValues.back().update(anUpdate); }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
            anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
    }
    void newValue(accumulation_type aValueType)
    {
        if (aValueType == accumulation_type::Accumulate) {
            theValues.push_back(theValues.back());
        } else {
            theValues.push_back(simple_type(0));
        }
    }
    void reset(value_type /*ignored*/)
    {
        theValues.clear();
        theValues.push_back(simple_type(0));
    }
    StatValueBase& operator[](int32_t anIndex) { return theValues[anIndex]; }
    std::size_t size() { return theValues.size(); }
};

class StatValueArray_Average : public StatValueArrayBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueArrayBase>(*this);
        ar & theValues;
    }
    StatValueArray_Average() {}

  public:
    typedef std::pair<int64_t, int64_t> update_type;
    typedef int64_t value_type;
    typedef StatValue_Average simple_type;

  private:
    std::vector<simple_type> theValues;

  public:
    StatValueArray_Average(value_type /*ignored*/) { theValues.push_back(simple_type(0)); }
    void reduceSum(const StatValueBase& aBase)
    {
        std::cerr << "reductions not supported (StatValueArray_Average)" << std::endl;
    }
    void update(update_type anUpdate) { theValues.back().update(anUpdate); }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
            anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
    }
    void newValue(accumulation_type aValueType)
    {
        if (aValueType == accumulation_type::Accumulate) {
            theValues.push_back(theValues.back());
        } else {
            theValues.push_back(simple_type(0));
        }
    }
    void reset(value_type /*ignored*/)
    {
        theValues.clear();
        theValues.push_back(simple_type(0));
    }
    StatValueBase& operator[](int32_t anIndex) { return theValues[anIndex]; }
    std::size_t size() { return theValues.size(); }
};

class StatValueArray_StdDev : public StatValueArrayBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueArrayBase>(*this);
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
    StatValueArray_StdDev(value_type /*ignored*/) { theValues.push_back(simple_type(0)); }
    void reduceSum(const StatValueBase& aBase)
    {
        std::cerr << "reductions not supported (StatValueArray_StdDev)" << std::endl;
    }
    void update(update_type anUpdate) { theValues.back().update(anUpdate); }
    void print(std::ostream& anOstream, std::string const& options = std::string("")) const
    {
        for (int32_t i = 0; i < static_cast<int>(theValues.size()) - 1; ++i) {
            anOstream << theValues[i] << ", ";
        }
        anOstream << theValues.back();
    }
    void newValue(accumulation_type aValueType)
    {
        if (aValueType == accumulation_type::Accumulate) {
            theValues.push_back(theValues.back());
        } else {
            theValues.push_back(simple_type(0));
        }
    }
    void reset(value_type /*ignored*/)
    {
        theValues.clear();
        theValues.push_back(simple_type(0));
    }
    StatValueBase& operator[](int32_t anIndex) { return theValues[anIndex]; }
    std::size_t size() { return theValues.size(); }
};

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_VALUE_ARRAYS__HPP__INCLUDED
