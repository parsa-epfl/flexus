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


#ifndef __CACHE_STATS_HPP__
#define __CACHE_STATS_HPP__

namespace nCMPCache {

struct CacheStats {

  Flexus::Stat::StatCounter WriteHit;
  Flexus::Stat::StatCounter WriteMissInvalidatesOnly;
  Flexus::Stat::StatCounter WriteMissInvalidatesAndData;
  Flexus::Stat::StatCounter WriteMissMemory;
  Flexus::Stat::StatCounter ReadHit;
  Flexus::Stat::StatCounter ReadMissMemory;
  Flexus::Stat::StatCounter ReadMissPeer;
  Flexus::Stat::StatCounter FetchHit;
  Flexus::Stat::StatCounter FetchMissPeer;
  Flexus::Stat::StatCounter FetchMissMemory;
  Flexus::Stat::StatCounter UpgradeHit;
  Flexus::Stat::StatCounter UpgradeMiss;
  Flexus::Stat::StatCounter NASHit;
  Flexus::Stat::StatCounter NASMissInvalidatesAndData;
  Flexus::Stat::StatCounter NASMissMemory;
  Flexus::Stat::StatCounter	EvictClean;
  Flexus::Stat::StatCounter	EvictWritableWrite;
  Flexus::Stat::StatCounter	EvictWritableBypass;
  Flexus::Stat::StatCounter	EvictDirtyWrite;
  Flexus::Stat::StatCounter	EvictDirtyBypass;




  CacheStats(const std::string & aName) : WriteHit(aName + "-WriteHit")
    , WriteMissInvalidatesOnly(aName + "-WriteMissInvalidatesOnly")
    , WriteMissInvalidatesAndData(aName + "-WriteMissInvalidatesAndData")
    , WriteMissMemory(aName + "-WriteMissMemory")
    , ReadHit(aName + "-ReadHit")
    , ReadMissMemory(aName + "-ReadMissMemory")
    , ReadMissPeer(aName + "-ReadMissPeer")
    , FetchHit(aName + "-FetchHit")
    , FetchMissPeer(aName + "-FetchMissPeer")
    , FetchMissMemory(aName + "-FetchMissMemory")
    , UpgradeHit(aName + "-UpgradeHit")
    , UpgradeMiss(aName + "-UpgradeMiss")
    , NASHit(aName + "-NASHit")
    , NASMissInvalidatesAndData(aName + "-NASMissInvalidatesAndData")
    , NASMissMemory(aName + "-NASMissMemory")
    , EvictClean(aName + "-EvictClean")
    , EvictWritableWrite(aName + "-EvictWritableWrite")
    , EvictWritableBypass(aName + "-EvictWritableBypass")
    , EvictDirtyWrite(aName + "-EvictDirtyWrite")
    , EvictDirtyBypass(aName + "-EvictDirtyBypass")

  {}

}; // struct CacheStats

}; // namespace nCMPCache

#endif // ! __CACHE_STATS_HPP__
