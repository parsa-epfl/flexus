/*
 * Revision History:
 *     twenisch    05 Oct 04 - Initial Revision
 */

#ifndef _PREFETCHTRACKER_HPP
#define _PREFETCHTRACKER_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
using namespace boost::multi_index;

#define DBG_DeclareCategories PrefetchTracking
#include DBG_Control()

namespace nPrefetchTracker {
namespace Stat = Flexus::Stat;
using boost::counted_base;
using boost::intrusive_ptr;
using Flexus::SharedTypes::TransactionTracker;

enum eMissType {
  kStart_Unknown
  , kStart_FillL2
  , kStart_FillOffChip
  , kEnd_FillL2
  , kEnd_FillOffChip
};

struct MissEvent {
  mutable eMissType theType;
  uint64_t theCycle;
  boost::intrusive_ptr<TransactionTracker> theTracker;
  MissEvent( boost::intrusive_ptr<TransactionTracker> aTracker, eMissType aType)
    : theType( aType )
    , theCycle( Flexus::Core::theFlexus->cycleCount() )
    , theTracker(aTracker)
  {}
};

struct by_cycle;
struct by_miss;
typedef multi_index_container
< MissEvent
, indexed_by
< ordered_non_unique
< tag<by_cycle>
, member< MissEvent, uint64_t, &MissEvent::theCycle>
>
, ordered_non_unique
< tag<by_miss>
, member< MissEvent, boost::intrusive_ptr<TransactionTracker>, &MissEvent::theTracker>
>
>
>
miss_table_t;

struct PrefetchTracker {
  std::string theName;

  //Miss Tracking
  miss_table_t theMissTable;
  uint64_t theLastAccounting_Prefetch;
  uint64_t theLastAccounting_FillL2;
  uint64_t theLastAccounting_FillOffChip;
  uint64_t theLastStart_Prefetch;
  uint64_t theLastStart_FillL2;
  uint64_t theLastStart_FillOffChip;
  uint32_t theCount_Prefetch;
  uint32_t theCount_FillL2;
  uint32_t theCount_FillOffChip;

  //Stats
  Stat::StatCounter theTotal_Prefetch;
  Stat::StatCounter theTotal_FillL2;
  Stat::StatCounter theTotal_FillOffChip;

  Stat::StatLog2Histogram theLatencies_Prefetch;
  Stat::StatCounter theLatenciesSum_Prefetch;
  Stat::StatLog2Histogram theLatencies_FillL2;
  Stat::StatCounter theLatenciesSum_FillL2;
  Stat::StatLog2Histogram theLatencies_FillOffChip;
  Stat::StatCounter theLatenciesSum_FillOffChip;

  Stat::StatLog2Histogram theInterarrivals_Prefetch;
  Stat::StatCounter theInterarrivalsSum_Prefetch;
  Stat::StatLog2Histogram theInterarrivals_FillL2;
  Stat::StatCounter theInterarrivalsSum_FillL2;
  Stat::StatLog2Histogram theInterarrivals_FillOffChip;
  Stat::StatCounter theInterarrivalsSum_FillOffChip;

  Stat::StatInstanceCounter<int64_t> theCyclesWithN_Prefetch;
  Stat::StatInstanceCounter<int64_t> theCyclesWithN_FillL2;
  Stat::StatInstanceCounter<int64_t> theCyclesWithN_FillOffChip;

  PrefetchTracker( std::string const & aName)
    : theName(aName)
    //Counters
    , theLastAccounting_Prefetch( 0 )
    , theLastAccounting_FillL2( 0 )
    , theLastAccounting_FillOffChip( 0 )
    , theLastStart_Prefetch( 0 )
    , theLastStart_FillL2( 0 )
    , theLastStart_FillOffChip( 0 )
    , theCount_Prefetch( 0 )
    , theCount_FillL2( 0 )
    , theCount_FillOffChip( 0 )
    //Stats
    , theTotal_Prefetch( theName + "-Track:Prefetches" )
    , theTotal_FillL2( theName + "-Track:Prefetches:FillL2" )
    , theTotal_FillOffChip( theName + "-Track:Prefetches:FillOffChip" )

    , theLatencies_Prefetch( theName + "-Track:Latency:Prefetch" )
    , theLatenciesSum_Prefetch( theName + "-Track:LatencySum:Prefetch" )
    , theLatencies_FillL2( theName + "-Track:Latency:FillL2" )
    , theLatenciesSum_FillL2( theName + "-Track:LatencySum:FillL2" )
    , theLatencies_FillOffChip( theName + "-Track:Latency:FillOffChip" )
    , theLatenciesSum_FillOffChip( theName + "-Track:LatencySum:FillOffChip" )

    , theInterarrivals_Prefetch( theName + "-Track:Interarrivals:Prefetch" )
    , theInterarrivalsSum_Prefetch( theName + "-Track:InterarrivalsSum:Prefetch" )
    , theInterarrivals_FillL2( theName + "-Track:Interarrivals:FillL2" )
    , theInterarrivalsSum_FillL2( theName + "-Track:InterarrivalsSum:FillL2" )
    , theInterarrivals_FillOffChip( theName + "-Track:Interarrivals:FillOffChip" )
    , theInterarrivalsSum_FillOffChip( theName + "-Track:InterarrivalsSum:FillOffChip" )

    , theCyclesWithN_Prefetch( theName + "-Track:CyclesWithN:Prefetch" )
    , theCyclesWithN_FillL2( theName + "-Track:CyclesWithN:FillL2" )
    , theCyclesWithN_FillOffChip( theName + "-Track:CyclesWithN:FillOffChip" )
  {}

  void startPrefetch( boost::intrusive_ptr<TransactionTracker> tracker ) {
    theMissTable.insert( MissEvent(tracker, kStart_Unknown ) );
  }

  void processTable() {
    while (! theMissTable.empty()) {
      bool erased = false;
      miss_table_t::iterator front = theMissTable.begin();
      switch ( front->theType ) {
        case kStart_Unknown:
          return; //Can't process this entry yet

        case kStart_FillL2:
          theCyclesWithN_Prefetch << std::make_pair( theCount_Prefetch, front->theCycle - theLastAccounting_Prefetch);
          theCyclesWithN_FillL2 << std::make_pair( theCount_FillL2, front->theCycle - theLastAccounting_FillL2);
          theLastAccounting_Prefetch = front->theCycle;
          theLastAccounting_FillL2 = front->theCycle;

          ++theTotal_Prefetch;
          ++theTotal_FillL2;
          ++theCount_Prefetch;
          ++theCount_FillL2;

          theInterarrivals_Prefetch << ( front->theCycle - theLastStart_Prefetch );
          theInterarrivalsSum_Prefetch += ( front->theCycle - theLastStart_Prefetch );
          theInterarrivals_FillL2 << ( front->theCycle - theLastStart_FillL2 );
          theInterarrivalsSum_FillL2 += ( front->theCycle - theLastStart_FillL2 );

          theLastStart_Prefetch = front->theCycle;
          theLastStart_FillL2 = front->theCycle;
          break;

        case kStart_FillOffChip:
          theCyclesWithN_Prefetch << std::make_pair( theCount_Prefetch, front->theCycle - theLastAccounting_Prefetch);
          theCyclesWithN_FillOffChip << std::make_pair( theCount_FillOffChip, front->theCycle - theLastAccounting_FillOffChip);
          theLastAccounting_Prefetch = front->theCycle;
          theLastAccounting_FillOffChip = front->theCycle;

          ++theTotal_Prefetch;
          ++theTotal_FillOffChip;
          ++theCount_Prefetch;
          ++theCount_FillOffChip;

          theInterarrivals_Prefetch << ( front->theCycle - theLastStart_Prefetch );
          theInterarrivalsSum_Prefetch += ( front->theCycle - theLastStart_Prefetch );
          theInterarrivals_FillOffChip << ( front->theCycle - theLastStart_FillOffChip );
          theInterarrivalsSum_FillOffChip += ( front->theCycle - theLastStart_FillOffChip );

          theLastStart_Prefetch = front->theCycle;
          theLastStart_FillOffChip = front->theCycle;
          break;

        case kEnd_FillL2:
          theCyclesWithN_Prefetch << std::make_pair( theCount_Prefetch, front->theCycle - theLastAccounting_Prefetch);
          theCyclesWithN_FillL2 << std::make_pair( theCount_FillL2, front->theCycle - theLastAccounting_FillL2);
          theLastAccounting_Prefetch = front->theCycle;
          theLastAccounting_FillL2 = front->theCycle;

          --theCount_Prefetch;
          --theCount_FillL2;

          theLatencies_Prefetch << ( front->theCycle - front->theTracker->startCycle() );
          theLatenciesSum_Prefetch += ( front->theCycle - front->theTracker->startCycle() );
          theLatencies_FillL2 << ( front->theCycle - front->theTracker->startCycle() );
          theLatenciesSum_FillL2 += ( front->theCycle - front->theTracker->startCycle() );
          break;

        case kEnd_FillOffChip:
          theCyclesWithN_Prefetch << std::make_pair( theCount_Prefetch, front->theCycle - theLastAccounting_Prefetch);
          theCyclesWithN_FillOffChip << std::make_pair( theCount_FillOffChip, front->theCycle - theLastAccounting_FillOffChip);
          theLastAccounting_Prefetch = front->theCycle;
          theLastAccounting_FillOffChip = front->theCycle;

          --theCount_Prefetch;
          --theCount_FillOffChip;

          theLatencies_Prefetch << ( front->theCycle - front->theTracker->startCycle() );
          theLatenciesSum_Prefetch += ( front->theCycle - front->theTracker->startCycle() );
          theLatencies_FillOffChip << ( front->theCycle - front->theTracker->startCycle() );
          theLatenciesSum_FillOffChip += ( front->theCycle - front->theTracker->startCycle() );
          break;
      }

      if (! erased) {
        theMissTable.erase(front);
      }
    }
  }

  void finishPrefetch( boost::intrusive_ptr<TransactionTracker> tracker ) {

    //See if this is a miss
    if ( tracker ) {
      miss_table_t::index<by_miss>::type::iterator iter = theMissTable.get<by_miss>().find( tracker );
      if (iter != theMissTable.get<by_miss>().end() ) {
        //We found the start event for this coherence miss
        DBG_Assert( iter->theType == kStart_Unknown);
        if (tracker->fillLevel() && *tracker->fillLevel() == Flexus::SharedTypes::eL2 ) {
          iter->theType = kStart_FillL2;
          theMissTable.insert( MissEvent( tracker, kEnd_FillL2) );
        } else {
          iter->theType = kStart_FillOffChip;
          theMissTable.insert( MissEvent( tracker, kEnd_FillOffChip) );
        }
      }
    }
    processTable();
  }

};

}  // end namespace nPrefetchTracker

#endif // _PREFETCHTRACKER_HPP
