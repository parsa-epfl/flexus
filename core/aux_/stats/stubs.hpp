#ifndef FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED
#define FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numeric>
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

template<class Archive>
void save(Archive & ar, ::boost::intrusive_ptr<StatValueBase> const & ptr, uint32_t version) {
  StatValueBase * svb = ptr.get();
  ar & svb;
}

template<class Archive>
void load(Archive & ar, ::boost::intrusive_ptr<StatValueBase> & ptr, uint32_t version) {
  StatValueBase * svb;
  ar & svb;
  ptr = boost::intrusive_ptr<Flexus::Stat::aux_::StatValueBase> (svb);
}

template<class Archive>
void save(Archive & ar, ::boost::intrusive_ptr<StatValueArrayBase> const & ptr, uint32_t version) {
  StatValueArrayBase * svab = ptr.get();
  ar & svab;
}

template<class Archive>
void load(Archive & ar, ::boost::intrusive_ptr<StatValueArrayBase> & ptr, uint32_t version) {
  StatValueArrayBase * svab;
  ar & svab;
  ptr = boost::intrusive_ptr<Flexus::Stat::aux_::StatValueArrayBase> (svab);
}

template <class StatUpdater>
struct StatUpdaterLink {
  virtual ~StatUpdaterLink() {}
  virtual void setNextUpdater(StatUpdater * aLink) = 0;
};

template <class StatUpdater>
class StatUpdaterLinkImpl : public StatUpdaterLink<StatUpdater> {
protected:
  StatUpdaterLink<StatUpdater> * thePreviousUpdater;
  StatUpdater * theNextUpdater;

  StatUpdaterLinkImpl( StatUpdaterLink<StatUpdater> * aPrevious, StatUpdater * aNext)
    : thePreviousUpdater(aPrevious)
    , theNextUpdater(aNext) {
    if (theNextUpdater) {
      theNextUpdater->setPreviousUpdater(this);
    }
  }

  virtual ~StatUpdaterLinkImpl() {
    if (theNextUpdater) {
      theNextUpdater->setPreviousUpdater(thePreviousUpdater);
    }
    thePreviousUpdater = 0;
    theNextUpdater = 0;
  }

  void getNextUpdater() {
    return theNextUpdater;
  }
  virtual void setNextUpdater(StatUpdater * aLink) {
    theNextUpdater = aLink;
  }
  virtual void setPreviousUpdater(StatUpdaterLink<StatUpdater> * aLink) {
    thePreviousUpdater = aLink;
  }
};

struct StatUpdaterBase : public boost::counted_base {
  virtual ~StatUpdaterBase() {
  }
  virtual void reset() = 0;
};

template < class UpdateType >
struct StatUpdater : public StatUpdaterBase, public StatUpdaterLinkImpl< StatUpdater<UpdateType> > {
  StatUpdater(StatUpdaterLink< StatUpdater<UpdateType> > * aPrevious, StatUpdater<UpdateType> * aNext)
    : StatUpdaterLinkImpl< StatUpdater<UpdateType> >(aPrevious, aNext) {
    if (this->thePreviousUpdater) {
      this->thePreviousUpdater->setNextUpdater(this);
    }
  }
  virtual ~StatUpdater() {
    if (this->thePreviousUpdater) {
      this->thePreviousUpdater->setNextUpdater(this->theNextUpdater);
    }
  }
  virtual void update( UpdateType anUpdate ) = 0;
};

template < class StatValueType, class UpdateType >
class SimpleStatUpdater : public StatUpdater<UpdateType> {
  boost::intrusive_ptr<StatValueType> theValue;
  typename StatValueType::value_type theResetValue;
public:
  SimpleStatUpdater( boost::intrusive_ptr<StatValueType> aValue, typename StatValueType::value_type aResetValue, StatUpdaterLink<StatUpdater<UpdateType> > * aPrevious, StatUpdater<UpdateType> * aNext)
    : StatUpdater<UpdateType> (aPrevious, aNext)
    , theValue(aValue)
    , theResetValue(aResetValue)
  {}
  virtual ~SimpleStatUpdater() {}
  virtual void update( UpdateType anUpdate ) {
    theValue->update(anUpdate);
    if (this->theNextUpdater) {
      this->theNextUpdater->update(anUpdate);
    }
  }
  virtual void reset() {
    theValue->reset(theResetValue);
    if (this->theNextUpdater) {
      this->theNextUpdater->reset();
    }
  }
};

} // end aux_
} // end Stat
} // end Flexus

#endif //FLEXUS_CORE_AUX__STATS_STUBS__HPP__INCLUDED
