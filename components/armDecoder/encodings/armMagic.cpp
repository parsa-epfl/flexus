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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

#include "../armInstruction.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {
using namespace nuArchARM;

struct MAGIC : public armInstruction {
  MAGIC(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState,
        uint32_t aCPU, int64_t aSequenceNo)
      : armInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo) {
    DECODER_TRACE;
    setClass(clsSynchronizing, codeMAGIC);
  }

  virtual bool mayRetire() const {
    return true;
  }
  virtual bool preValidate() {
    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
    if (cpu->getPC() != thePC) {
      DBG_(VVerb, (<< *this << " PreValidation failed: PC mismatch flexus=" << thePC
                   << " qemu=" << cpu->getPC()));
    }
    return cpu->getPC() == thePC;
  }

  virtual bool postValidate() {
    //    Flexus::Qemu::Processor cpu =
    //    Flexus::Qemu::Processor::getProcessor(theCPU); if ( cpu->getPC()  !=
    //    theNPC ) {
    //      DBG_( VVerb, ( << *this << " PostValidation failed: PC mismatch
    //      flexus=" << theNPC << " simics=" << cpu->getPC() ) );
    //    }
    //    return ( cpu->getPC()  == theNPC ) ;
    return true;
  }

  virtual void describe(std::ostream &anOstream) const {
    armInstruction::describe(anOstream);
    anOstream << "MAGIC";
  }
};

arminst magic(armcode const &aFetchedOpcode, uint32_t aCPU, int64_t aSequenceNo) {
  return arminst(new MAGIC(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                           aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

} // namespace narmDecoder
