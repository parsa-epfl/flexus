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
#ifndef FLEXUS_CORE_AUX__STATS_BASE__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_BASE__HPP__INCLUDED

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

struct CalcException
{
    std::string theReason;

    CalcException(std::string const& aReason)
      : theReason(aReason)
    {
    }
};

struct StatValueBase : public boost::counted_base
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
    }

  public:
    friend std::ostream& operator<<(std::ostream& anOstream, StatValueBase const& aValue)
    {
        aValue.print(anOstream);
        return anOstream;
    }
    virtual void print(std::ostream& anOstream, std::string const& options = std::string("")) const {};
    virtual int64_t asLongLong() const { return 0; };
    virtual int64_t asLongLong(std::string const& options) const
    {
        throw CalcException(std::string("{Expression options not supported for this stat type}"));
    }
    virtual double asDouble() const { return asLongLong(); };
    virtual double asDouble(std::string const& options) const
    {
        throw CalcException(std::string("{Expression options not supported for this stat type}"));
    }
    virtual ~StatValueBase() {}
    virtual boost::intrusive_ptr<const StatValueBase> serialForm() const { return this; }
    virtual boost::intrusive_ptr<StatValueBase> sumAccumulator()
    {
        throw 1; /* by default, stat's don't support sum accumulation */
    }
    virtual void reduceSum(StatValueBase const& anRHS) {};
    virtual void reduceSum(StatValueBase const* anRHS) {};
    virtual boost::intrusive_ptr<StatValueBase> avgAccumulator()
    {
        throw 1; /* by default, stat's don't support average accumulation */
    }
    virtual void reduceAvg(StatValueBase const& anRHS) {};
    virtual boost::intrusive_ptr<StatValueBase> stdevAccumulator()
    {
        throw 1; /* by default, stat's don't support standard deviation accumulation
                  */
    }
    virtual void reduceStdDev(StatValueBase const& anRHS) {};
    virtual boost::intrusive_ptr<StatValueBase> countAccumulator();
    virtual void reduceCount(StatValueBase const& anRHS) {};
};

struct StatValueArrayBase : public StatValueBase
{
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueBase>(*this);
    }

  public:
    virtual StatValueBase& operator[](int32_t anIndex)  = 0;
    virtual void newValue(accumulation_type aValueType) = 0;
    virtual std::size_t size()                          = 0;
};

template<class Archive>
void
save(Archive& ar, ::boost::intrusive_ptr<StatValueBase> const& ptr, uint32_t version);

template<class Archive>
void
load(Archive& ar, ::boost::intrusive_ptr<StatValueBase>& ptr, uint32_t version);

template<class Archive>
void
save(Archive& ar, ::boost::intrusive_ptr<StatValueArrayBase> const& ptr, uint32_t version);

template<class Archive>
void
load(Archive& ar, ::boost::intrusive_ptr<StatValueArrayBase>& ptr, uint32_t version);

} // namespace aux_
} // namespace Stat
} // namespace Flexus

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Flexus::Stat::aux_::StatValueArrayBase)

namespace boost {
namespace serialization {
template<class Archive>
inline void
serialize(Archive& ar, ::boost::intrusive_ptr<Flexus::Stat::aux_::StatValueBase> const& t, const uint32_t file_version)
{
    split_free(ar, t, file_version);
}
template<class Archive>
inline void
serialize(Archive& ar,
          ::boost::intrusive_ptr<Flexus::Stat::aux_::StatValueArrayBase> const& t,
          const uint32_t file_version)
{
    split_free(ar, t, file_version);
}
} // namespace serialization
} // namespace boost

namespace Flexus {
namespace Stat {
namespace aux_ {

class StatUpdaterBase; // fwd decl.

class StatValueHandle_Base
{
  protected:
    std::string const* theStatName;
    boost::intrusive_ptr<StatUpdaterBase> theUpdater;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, uint32_t version) const
    {
        ar&* theStatName;
    }
    template<class Archive>
    void load(Archive& ar, uint32_t version)
    {
        std::string temp;
        ar & temp;
        theStatName = new std::string(temp); // Leaks, but doesn't matter
        theUpdater  = 0;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

  public:
    StatValueHandle_Base();
    StatValueHandle_Base(Stat* aStat, boost::intrusive_ptr<StatUpdaterBase> anUpdater);
    StatValueHandle_Base(std::string const& aName);
    StatValueHandle_Base(const StatValueHandle_Base& aHandle);
    StatValueHandle_Base& operator=(const StatValueHandle_Base& aHandle);
    StatValueHandle_Base& operator+=(const StatValueHandle_Base& aHandle);
    void reset();
    void releaseUpdater();
    std::string name();
    void rename(std::string aName);
};

class StatValueHandle : public StatValueHandle_Base
{
    boost::intrusive_ptr<StatValueBase> theValue;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, uint32_t version) const
    {
        ar& boost::serialization::base_object<StatValueHandle_Base>(*this);
        boost::intrusive_ptr<const StatValueBase> temp(theValue->serialForm());
        ar & temp;
    }
    template<class Archive>
    void load(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueHandle_Base>(*this);
        ar & theValue;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

  public:
    StatValueHandle();
    StatValueHandle(Stat* aStat,
                    boost::intrusive_ptr<StatValueBase> aValue,
                    boost::intrusive_ptr<StatUpdaterBase> anUpdater);
    StatValueHandle(std::string const& aName, boost::intrusive_ptr<StatValueBase> aValue);
    StatValueHandle(const StatValueHandle& aHandle);
    StatValueHandle& operator=(const StatValueHandle& aHandle);
    StatValueHandle& operator+=(const StatValueHandle& aHandle);
    boost::intrusive_ptr<StatValueBase> sumAccumulator();
    boost::intrusive_ptr<StatValueBase> avgAccumulator();
    boost::intrusive_ptr<StatValueBase> stdevAccumulator();
    boost::intrusive_ptr<StatValueBase> countAccumulator();
    friend std::ostream& operator<<(std::ostream& anOstream, StatValueHandle const& aValueHandle);
    void print(std::ostream& anOstream, std::string const& options = std::string(""));
    int64_t asLongLong();
    int64_t asLongLong(std::string const& options);
    double asDouble();
    double asDouble(std::string const& options);
    boost::intrusive_ptr<StatValueBase> getValue() { return theValue; }
    void setValue(boost::intrusive_ptr<StatValueBase> aValue) { theValue = aValue; }
};

class StatValueArrayHandle : public StatValueHandle_Base
{
    boost::intrusive_ptr<StatValueArrayBase> theValue;

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, uint32_t version) const
    {
        ar& boost::serialization::base_object<StatValueHandle_Base>(*this);
        boost::intrusive_ptr<const StatValueBase> temp(theValue->serialForm());
        ar & temp;
    }
    template<class Archive>
    void load(Archive& ar, uint32_t version)
    {
        ar& boost::serialization::base_object<StatValueHandle_Base>(*this);
        ar & theValue;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

  public:
    StatValueArrayHandle();
    StatValueArrayHandle(Stat* aStat,
                         boost::intrusive_ptr<StatValueArrayBase> aValue,
                         boost::intrusive_ptr<StatUpdaterBase> anUpdater);
    StatValueArrayHandle(const StatValueArrayHandle& aHandle);
    StatValueArrayHandle& operator=(const StatValueArrayHandle& aHandle);
    friend std::ostream& operator<<(std::ostream& anOstream, StatValueArrayHandle const& aValueHandle);
    void print(std::ostream& anOstream, std::string const& options = std::string(""));
    void newValue(accumulation_type anAccumulationType);
    int64_t asLongLong();
    int64_t asLongLong(std::string const& options);
};

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_BASE__HPP__INCLUDED
