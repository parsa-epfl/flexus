// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
#define FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED

#include <algorithm>
#include <list>
#include <vector>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include "uArchInterfaces.hpp"
namespace ll = boost::lambda;
namespace nuArchARM {

class BypassNetwork {
protected:
  uint32_t theXRegs;
  uint32_t theVRegs;
  uint32_t theCCRegs;
  typedef std::function< bool(register_value) > bypass_fn;
  typedef std::pair< boost::intrusive_ptr<Instruction>, bypass_fn> bypass_handle;
  typedef std::list< bypass_handle > bypass_handle_list;
  typedef std::vector< bypass_handle_list > bypass_map;
  typedef std::vector< int32_t > collect_counter;
  bypass_map theXDeps;
  collect_counter theXCounts;
  bypass_map theVDeps;
  collect_counter theVCounts;
  bypass_map theCCDeps;
  collect_counter theCCCounts;

public:
  BypassNetwork( uint32_t anXRegs, uint32_t anVRegs, uint32_t anCCRegs)
    : theXRegs(anXRegs)
    , theVRegs(anVRegs)
    , theCCRegs(anCCRegs){
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
      case xRegisters:
        return theXDeps[anIndex.theIndex];
      case vRegisters:
        return theVDeps[anIndex.theIndex];
      case ccBits:
        return theCCDeps[anIndex.theIndex];
      default:
        DBG_Assert(false);
        return theXDeps[0]; //Suppress compiler warning
    }
  }

  void collect( mapped_reg anIndex ) {
    switch (anIndex.theType) {
      case xRegisters:
        --theXCounts[anIndex.theIndex];
        if ( theXCounts[anIndex.theIndex] <= 0) {
          theXCounts[anIndex.theIndex] = 10;
          doCollect(  theXDeps[anIndex.theIndex] );
        }
        break;
      case vRegisters:
        --theVCounts[anIndex.theIndex];
        if ( theVCounts[anIndex.theIndex] <= 0) {
          theVCounts[anIndex.theIndex] = 10;
          doCollect(  theVDeps[anIndex.theIndex] );
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
    for (uint32_t i = 0; i < theXRegs; ++i) {
      theXCounts[i] = 10;
      doCollect(  theXDeps[i] );
    }
    for (uint32_t i = 0; i < theVRegs; ++i) {
      theVCounts[i] = 10;
      doCollect(  theVDeps[i] );

    }
    for (uint32_t i = 0; i < theCCRegs; ++i) {
      theCCCounts[i] = 10;
      doCollect(  theCCDeps[i] );
    }
  }

  void reset() {
    FLEXUS_PROFILE();
    theXDeps.clear();
    theVDeps.clear();
    theCCDeps.clear();
    theXDeps.resize(theXRegs);
    theVDeps.resize(theVRegs);
    theCCDeps.resize(theCCRegs);
    theXCounts.clear();
    theVCounts.clear();
    theCCCounts.clear();
    theXCounts.resize(theXRegs, 10);
    theVCounts.resize(theVRegs, 10);
    theCCCounts.resize(theCCRegs, 10);
  }

  void connect( mapped_reg anIndex, boost::intrusive_ptr<Instruction> inst, bypass_fn fn) {
    FLEXUS_PROFILE();
    collect(anIndex);
    lookup(anIndex).push_back( std::make_pair(inst, fn) );
  }
  void unmap( mapped_reg anIndex) {
    FLEXUS_PROFILE();
    lookup(anIndex).clear();
  }

  void write( mapped_reg anIndex, register_value aValue, uArchARM & aCore ) {
    FLEXUS_PROFILE();
    bypass_handle_list & list = lookup(anIndex);
    bypass_handle_list::iterator iter = list.begin();
    bypass_handle_list::iterator end = list.end();
    while (iter != end) {
      if ( iter->second(aValue) ) {
        auto temp = iter;
        ++iter;
        list.erase(temp);
      } else {
        ++iter;
      }
    }
  }
};

} //nuArchARM

#endif //FLEXUS_uARCH_BYPASSNETWORK_HPP_INCLUDED
