// DO-NOT-REMOVE begin-copyright-block 
//QFlex consists of several software components that are governed by various
//licensing terms, in addition to software that was developed internally.
//Anyone interested in using QFlex needs to fully understand and abide by the
//licenses governing all the software components.
//
//### Software developed externally (not by the QFlex group)
//
//    * [NS-3](https://www.gnu.org/copyleft/gpl.html)
//    * [QEMU](http://wiki.qemu.org/License) 
//    * [SimFlex] (http://parsa.epfl.ch/simflex/)
//
//Software developed internally (by the QFlex group)
//**QFlex License**
//
//QFlex
//Copyright (c) 2016, Parallel Systems Architecture Lab, EPFL
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//    * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//      nor the names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior written
//      permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
//EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// DO-NOT-REMOVE end-copyright-block   
#ifndef FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED
#define FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/debug/debug.hpp>
namespace Flexus {
namespace SharedTypes {

static uint64_t translationID;


struct Translation : public boost::counted_base {

    enum eTranslationType {
      eStore,
      eLoad,
      eFetch
    };

    enum eTLBstatus {
        kTLBunresolved,
        kTLBmiss,
        kTLBhit
    };

    enum eTLBtype{
        kINST,
        kDATA,
        kNONE
    };

    Translation()
        : theTLBstatus(kTLBunresolved)
        , theTLBtype(kNONE)
        , theReady(false)
        , theWaiting(false)
        , theDone(false)
        , theCurrentTranslationLevel(0)
        , rawTTEValue(0)
        , theID(translationID++)
        , theAnnul(false)

    {}
    ~Translation(){}

    Translation (const Translation& aTr){
        theVaddr       = aTr.theVaddr;
        thePSTATE      = aTr.thePSTATE;
        theType        = aTr.theType;
        thePaddr       = aTr.thePaddr;
        theException   = aTr.theException;
        theTTEEntry    = aTr.theTTEEntry;
        theTLBstatus   = aTr.theTLBstatus;
        theTLBtype     = aTr.theTLBtype;
    }

  Translation& operator= (Translation& rhs) {
      theVaddr = rhs.theVaddr;
      thePSTATE = rhs.thePSTATE;
      theType = rhs.theType;
      thePaddr = rhs.thePaddr;
      theException = rhs.theException;
      theTTEEntry = rhs.theTTEEntry;
      theTLBstatus   = rhs.theTLBstatus;
      theTLBtype     = rhs.theTLBtype;
      return *this;
  }

  VirtualMemoryAddress theVaddr;
  PhysicalMemoryAddress thePaddr;

  int thePSTATE;
  uint32_t theIndex;
  eTranslationType theType;
  eTLBstatus theTLBstatus;
  eTLBtype theTLBtype;
  int theException;
  uint64_t theTTEEntry;

  bool theReady; // ready for translation - step i
  bool theWaiting; // in memory - step ii
  bool theDone; // all done step iii
  uint8_t theCurrentTranslationLevel;
  uint64_t rawTTEValue;
  uint64_t theID;
  bool theAnnul;

  void setData(){theTLBtype = kDATA;}
  void setInstr(){theTLBtype = kINST;}
  eTLBtype type (){return theTLBtype;}
  bool isInstr(){return type() == kINST;}
  bool isData(){return type() == kDATA;}
  eTLBstatus status (){return theTLBstatus;}
  bool isMiss(){ return status() == kTLBmiss; }
  bool isUnresuloved(){return status() == kTLBunresolved;}
  void setHit() {theTLBstatus = kTLBhit;}
  void setMiss() {theTLBstatus = kTLBmiss;}


  bool isWaiting(){return theWaiting;}
  void toggleWaiting() {theWaiting = !theWaiting;}

  bool isReady() {return theReady;}
  void toggleReady() {theReady = !theReady;}

  bool isDone() {return theDone;}
  void setDone() {theDone = true;}

  bool isAnnul() {return theAnnul;}
  void setAnnul() {theAnnul = true;}

};

typedef boost::intrusive_ptr<Translation> TranslationPtr;


} //SharedTypes
} //Flexus
#endif //FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED