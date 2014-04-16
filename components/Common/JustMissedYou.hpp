#ifndef FLEXUS_JMY_INCLUDED
#define FLEXUS_JMY_INCLUDED

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <core/boost_extensions/filled_lexical_cast.hpp>

namespace nJMY {
using Flexus::SharedTypes::PhysicalMemoryAddress;
using namespace Flexus::Core;
using namespace boost::multi_index;
namespace Stat = Flexus::Stat;

struct Entry {
  PhysicalMemoryAddress theAddress;
  uint64_t theCycle;
  bool theSystem;
  Entry( PhysicalMemoryAddress anAddress, bool isSystem = false)
    : theAddress( anAddress )
    , theCycle( Flexus::Core::theFlexus->cycleCount() )
    , theSystem( isSystem )
  {}
};

struct by_addr {};
struct by_cycle {};
typedef multi_index_container
< Entry
, indexed_by
< sequenced<>
, ordered_non_unique
< tag<by_addr>
, member< Entry, PhysicalMemoryAddress, &Entry::theAddress>
>
, ordered_non_unique
< tag<by_cycle>
, member< Entry, uint64_t , &Entry::theCycle>
>
>
>
jmy_t;

struct JustMissedYou {

  std::string theName;
  int32_t theWidth;

  std::vector<Stat::StatCounter *> theJustMisses_S;
  std::vector<Stat::StatCounter *> theJustMisses_S_Delay;
  std::vector<Stat::StatCounter *> theJustMisses_U;
  std::vector<Stat::StatCounter *> theJustMisses_U_Delay;

  std::vector<jmy_t> theRecentMisses;
  std::vector<jmy_t> theRecentPrefetches;

  JustMissedYou(std::string aName, int32_t aWidth = 1)
    : theName(aName)
    , theWidth(aWidth) {
    theRecentMisses.resize(theWidth);
    theRecentPrefetches.resize(theWidth);
    if (theWidth == 1) {
      theJustMisses_S.push_back( new Stat::StatCounter( theName + "JustMisses:S" ) );
      theJustMisses_U.push_back( new Stat::StatCounter( theName + "JustMisses:U" ) );
      theJustMisses_S_Delay.push_back( new Stat::StatCounter( theName + "JustMisses:S:Delay" ) );
      theJustMisses_U_Delay.push_back( new Stat::StatCounter( theName + "JustMisses:U:Delay " ) );
    } else {
      for (int32_t i = 0; i < theWidth; ++i ) {
        theJustMisses_S.push_back( new Stat::StatCounter( filled_lexical_cast<3>(i) + "-" + theName + "-JustMisses:U" ) );
        theJustMisses_U.push_back( new Stat::StatCounter( filled_lexical_cast<3>(i) + "-" + theName + "-JustMisses:S" ) );
        theJustMisses_S_Delay.push_back( new Stat::StatCounter( filled_lexical_cast<3>(i) + "-" + theName + "-JustMisses:S:Delay " ) );
        theJustMisses_U_Delay.push_back( new Stat::StatCounter( filled_lexical_cast<3>(i) + "-" + theName + "-JustMisses:U:Delay " ) );
      }
    }
  }

  void clean(int32_t aNode) {
    jmy_t::index<by_cycle>::type::iterator clean = theRecentMisses[aNode].get<by_cycle>().lower_bound( theFlexus->cycleCount() - 10000);
    if ( clean != theRecentMisses[aNode].get<by_cycle>().begin()) {
      -- clean;
      theRecentMisses[aNode].get<by_cycle>().erase( theRecentMisses[aNode].get<by_cycle>().begin(), clean);
    }

    clean = theRecentPrefetches[aNode].get<by_cycle>().lower_bound( theFlexus->cycleCount() - 10000);
    if ( clean != theRecentPrefetches[aNode].get<by_cycle>().begin()) {
      -- clean;
      theRecentPrefetches[aNode].get<by_cycle>().erase( theRecentPrefetches[aNode].get<by_cycle>().begin(), clean);
    }
  }

  void coherenceMiss( PhysicalMemoryAddress anAddress, bool isSystem, int32_t aNode = 0 ) {
    DBG_Assert(aNode < theWidth);
    clean(aNode);
    jmy_t::index<by_addr>::type::iterator iter = theRecentPrefetches[aNode].get<by_addr>().find(anAddress);
    if (iter != theRecentPrefetches[aNode].get<by_addr>().end()) {
      if (isSystem) {
        ++(*theJustMisses_S[aNode] );
        (*theJustMisses_S_Delay[aNode] ) += theFlexus->cycleCount() - iter->theCycle ;
      } else {
        ++(*theJustMisses_U[aNode] );
        (*theJustMisses_U_Delay[aNode] ) += theFlexus->cycleCount() - iter->theCycle ;
      }
      DBG_(Trace, ( << filled_lexical_cast<3>(aNode) << "-JMY JustMissed " << anAddress << (isSystem ? "[S]" : "[U]") << " prefetch precedes miss by " << theFlexus->cycleCount() - iter->theCycle ) );
    }
    if (theRecentMisses[aNode].size() >= 64) {
      theRecentMisses[aNode].pop_front();
    }
    theRecentMisses[aNode].push_back( Entry(anAddress, isSystem ) );
  }

  void prefetch( PhysicalMemoryAddress anAddress, int32_t aNode = 0 ) {
    DBG_Assert(aNode < theWidth);
    clean(aNode);
    jmy_t::index<by_addr>::type::iterator iter = theRecentMisses[aNode].get<by_addr>().find(anAddress);
    if (iter != theRecentMisses[aNode].get<by_addr>().end()) {
      if (iter->theSystem) {
        ++(*theJustMisses_S[aNode] );
        (*theJustMisses_S_Delay[aNode] ) += theFlexus->cycleCount() - iter->theCycle ;
      } else {
        ++(*theJustMisses_U[aNode] );
        (*theJustMisses_U_Delay[aNode] ) += theFlexus->cycleCount() - iter->theCycle ;
      }
      DBG_(Trace, ( << filled_lexical_cast<3>(aNode) << "-JMY JustMissed " << anAddress << (iter->theSystem ? "[S]" : "[U]") << " miss precedes prefetch by " << theFlexus->cycleCount() - iter->theCycle ) );
    }
    if (theRecentPrefetches[aNode].size() >= 64) {
      theRecentPrefetches[aNode].pop_front();
    }
    theRecentPrefetches[aNode].push_back( Entry(anAddress) );
  }
};

} //End nJMY

#endif //FLEXUS_JMY_INCLUDED
