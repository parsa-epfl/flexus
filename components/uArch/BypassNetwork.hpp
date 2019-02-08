#ifndef FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
#define FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED

#include <algorithm>
#include <list>
#include <vector>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "uArchInterfaces.hpp"

namespace ll = boost::lambda;

namespace nuArch {

class BypassNetwork {
protected:
  uint32_t theRRegs;
  uint32_t theFRegs;
  uint32_t theCCRegs;
  typedef boost::function< bool(register_value) > bypass_fn;
  typedef std::pair< boost::intrusive_ptr<Instruction>, bypass_fn> bypass_handle;
  typedef std::list< bypass_handle > bypass_handle_list;
  typedef std::vector< bypass_handle_list > bypass_map;
  typedef std::vector< int32_t > collect_counter;
  bypass_map theRDeps;
  collect_counter theRCounts;
  bypass_map theFDeps;
  collect_counter theFCounts;
  bypass_map theCCDeps;
  collect_counter theCCCounts;

public:
  BypassNetwork( uint32_t anRRegs, uint32_t anFRegs, uint32_t aCCRegs)
    : theRRegs(anRRegs)
    , theFRegs(anFRegs)
    , theCCRegs(aCCRegs) {
    reset();
  }

  void doCollect( bypass_handle_list & aList ) {
    FLEXUS_PROFILE();
    bypass_handle_list::iterator iter, temp, end;
    iter = aList.begin();
    end = aList.end();
    while ( iter != end ) {
      temp = iter;
      ++iter;
      if (temp->first->isComplete()) {
        aList.erase(temp);
      }
    }
  }

  bypass_handle_list & lookup( mapped_reg anIndex ) {
    switch (anIndex.theType) {
      case rRegisters:
        return theRDeps[anIndex.theIndex];
      case fRegisters:
        return theFDeps[anIndex.theIndex];
      case ccBits:
        return theCCDeps[anIndex.theIndex];
      default:
        DBG_Assert(false);
        return theRDeps[0]; //Suppress compiler warning
    }
  }

  void collect( mapped_reg anIndex ) {
    switch (anIndex.theType) {
      case rRegisters:
        --theRCounts[anIndex.theIndex];
        if ( theRCounts[anIndex.theIndex] <= 0) {
          theRCounts[anIndex.theIndex] = 10;
          doCollect(  theRDeps[anIndex.theIndex] );
        }
        break;
      case fRegisters:
        --theFCounts[anIndex.theIndex];
        if ( theFCounts[anIndex.theIndex] <= 0) {
          theFCounts[anIndex.theIndex] = 10;
          doCollect(  theFDeps[anIndex.theIndex] );
        }
        break;
      case ccBits:
        --theCCCounts[anIndex.theIndex];
        if ( theCCCounts[anIndex.theIndex] <= 0) {
          theCCCounts[anIndex.theIndex] = 10;
          doCollect(  theCCDeps[anIndex.theIndex] );
        }
        break;
      default:
        DBG_Assert(false);
    }
  }

  void collectAll() {
    FLEXUS_PROFILE();
    for (uint32_t i = 0; i < theRRegs; ++i) {
      theRCounts[i] = 10;
      doCollect(  theRDeps[i] );
    }
    for (uint32_t i = 0; i < theFRegs; ++i) {
      theFCounts[i] = 10;
      doCollect(  theFDeps[i] );

    }
    for (uint32_t i = 0; i < theCCRegs; ++i) {
      theCCCounts[i] = 10;
      doCollect(  theCCDeps[i] );
    }
  }

  void reset() {
    FLEXUS_PROFILE();
    theRDeps.clear();
    theFDeps.clear();
    theCCDeps.clear();
    theRDeps.resize(theRRegs);
    theFDeps.resize(theFRegs);
    theCCDeps.resize(theCCRegs);

    theRCounts.clear();
    theFCounts.clear();
    theCCCounts.clear();
    theRCounts.resize(theRRegs, 10);
    theFCounts.resize(theFRegs, 10);
    theCCCounts.resize(theCCRegs, 10);
  }

  void connect( mapped_reg anIndex, boost::intrusive_ptr<Instruction> inst, bypass_fn fn) {
    FLEXUS_PROFILE();
    collect(anIndex);
    lookup(anIndex).push_back( std::make_pair(inst, fn) );
  }

  /*
  void disconnect( mapped_reg anIndex, boost::intrusive_ptr<Instruction> inst) {
    FLEXUS_PROFILE();
    bypass_handle_list & list = lookup(anIndex);
    std::remove_if( list.begin(), list.end(), ll::bind( &bypass_handle::first, ll::_1) == inst);
  }
  */

  void unmap( mapped_reg anIndex) {
    FLEXUS_PROFILE();
    lookup(anIndex).clear();
  }

  void write( mapped_reg anIndex, register_value aValue, uArch & aCore ) {
    FLEXUS_PROFILE();
    bypass_handle_list & list = lookup(anIndex);
    bypass_handle_list::iterator iter = list.begin();
    bypass_handle_list::iterator end = list.end();
    while (iter != end) {
      if ( iter->second(aValue) ) {
        bypass_handle_list::iterator temp = iter;
        ++iter;
        list.erase(temp);
      } else {
        ++iter;
      }
    }
  }
};

} //nuArch

#endif //FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
