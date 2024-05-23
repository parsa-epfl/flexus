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
#ifndef FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED

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

template<class Archive>
void
save(Archive& ar, ::boost::intrusive_ptr<StatValueBase> const& ptr, uint32_t version)
{
    StatValueBase* svb = ptr.get();
    ar & svb;
}

template<class Archive>
void
load(Archive& ar, ::boost::intrusive_ptr<StatValueBase>& ptr, uint32_t version)
{
    StatValueBase* svb;
    ar & svb;
    ptr = boost::intrusive_ptr<Flexus::Stat::aux_::StatValueBase>(svb);
}

template<class Archive>
void
save(Archive& ar, ::boost::intrusive_ptr<StatValueArrayBase> const& ptr, uint32_t version)
{
    StatValueArrayBase* svab = ptr.get();
    ar & svab;
}

template<class Archive>
void
load(Archive& ar, ::boost::intrusive_ptr<StatValueArrayBase>& ptr, uint32_t version)
{
    StatValueArrayBase* svab;
    ar & svab;
    ptr = boost::intrusive_ptr<Flexus::Stat::aux_::StatValueArrayBase>(svab);
}

template<class StatUpdater>
struct StatUpdaterLink
{
    virtual ~StatUpdaterLink() {}
    virtual void setNextUpdater(StatUpdater* aLink) = 0;
};

template<class StatUpdater>
class StatUpdaterLinkImpl : public StatUpdaterLink<StatUpdater>
{
  protected:
    StatUpdaterLink<StatUpdater>* thePreviousUpdater;
    StatUpdater* theNextUpdater;

    StatUpdaterLinkImpl(StatUpdaterLink<StatUpdater>* aPrevious, StatUpdater* aNext)
      : thePreviousUpdater(aPrevious)
      , theNextUpdater(aNext)
    {
        if (theNextUpdater) { theNextUpdater->setPreviousUpdater(this); }
    }

    virtual ~StatUpdaterLinkImpl()
    {
        if (theNextUpdater) { theNextUpdater->setPreviousUpdater(thePreviousUpdater); }
        thePreviousUpdater = 0;
        theNextUpdater     = 0;
    }

    StatUpdater* getNextUpdater() { return theNextUpdater; }
    virtual void setNextUpdater(StatUpdater* aLink) { theNextUpdater = aLink; }
    virtual void setPreviousUpdater(StatUpdaterLink<StatUpdater>* aLink) { thePreviousUpdater = aLink; }
};

struct StatUpdaterBase : public boost::counted_base
{
    virtual ~StatUpdaterBase() {}
    virtual void reset() = 0;
};

template<class UpdateType>
struct StatUpdater
  : public StatUpdaterBase
  , public StatUpdaterLinkImpl<StatUpdater<UpdateType>>
{
    StatUpdater(StatUpdaterLink<StatUpdater<UpdateType>>* aPrevious, StatUpdater<UpdateType>* aNext)
      : StatUpdaterLinkImpl<StatUpdater<UpdateType>>(aPrevious, aNext)
    {
        if (this->thePreviousUpdater) { this->thePreviousUpdater->setNextUpdater(this); }
    }
    virtual ~StatUpdater()
    {
        if (this->thePreviousUpdater) { this->thePreviousUpdater->setNextUpdater(this->theNextUpdater); }
    }
    virtual void update(UpdateType anUpdate) = 0;
};

template<class StatValueType, class UpdateType>
class SimpleStatUpdater : public StatUpdater<UpdateType>
{
    boost::intrusive_ptr<StatValueType> theValue;
    typename StatValueType::value_type theResetValue;

  public:
    SimpleStatUpdater(boost::intrusive_ptr<StatValueType> aValue,
                      typename StatValueType::value_type aResetValue,
                      StatUpdaterLink<StatUpdater<UpdateType>>* aPrevious,
                      StatUpdater<UpdateType>* aNext)
      : StatUpdater<UpdateType>(aPrevious, aNext)
      , theValue(aValue)
      , theResetValue(aResetValue)
    {
    }
    virtual ~SimpleStatUpdater() {}
    virtual void update(UpdateType anUpdate)
    {
        theValue->update(anUpdate);
        if (this->theNextUpdater) { this->theNextUpdater->update(anUpdate); }
    }
    virtual void reset()
    {
        theValue->reset(theResetValue);
        if (this->theNextUpdater) { this->theNextUpdater->reset(); }
    }
};

} // namespace aux_
} // namespace Stat
} // namespace Flexus

#endif // FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED
