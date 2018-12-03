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


struct Translation : public boost::counted_base {

    Translation():theTLBtype(kINST), theTLBstatus(kTLBunresolved) {}
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
  int thePSTATE;
  uint32_t theIndex;
  enum eTranslationType {
    eStore,
    eLoad,
    eFetch
  } theType;

  enum eTLBtype{
      kINST,
      kDATA
  } theTLBtype;

  eTLBtype type (){
      return theTLBtype;
  }

  bool isInstr(){return type() == kINST;}
  bool isData(){return type() == kDATA;}



  enum eTLBstatus {
      kTLBunresolved,
      kTLBmiss,
      kTLBhit
  }theTLBstatus;

  eTLBstatus status (){
      return theTLBstatus;
  }

  bool isMiss(){ return status() == kTLBmiss; }
  bool isUnresuloved(){return status() == kTLBunresolved;}
  void setHit() {theTLBstatus = kTLBhit; }
  void setMiss() {theTLBstatus = kTLBmiss; }


  PhysicalMemoryAddress thePaddr;
  int theException;
  unsigned long long theTTEEntry;
  bool isCacheable();
  bool isSideEffect();
  bool isXEndian();
  bool isInterrupt();
  bool isTranslating();
};




} //SharedTypes
} //Flexus
#endif //FLEXUS_SLICES__TRANSLATION_HPP_INCLUDED
