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

#include "armUnallocated.hpp"

namespace narmDecoder {
using namespace nuArchARM;


struct BlackBoxInstruction : public armInstruction {

  BlackBoxInstruction(VirtualMemoryAddress aPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, uint32_t  aCPU, int64_t aSequenceNo)
    : armInstruction(aPC, anOpcode, aBPState, aCPU, aSequenceNo) {
      DBG_(Tmp,(<< "\033[2;31m DECODER: BLACKBOXING \033[0m"));
    setClass(clsSynchronizing, codeBlackBox);
    forceResync();
  }

  virtual bool mayRetire() const {
    return true;
  }

  virtual bool postValidate() {
    return false;
  }

  virtual void describe(std::ostream & anOstream) const {
    armInstruction::describe(anOstream);
    anOstream << instCode();
  }
};

arminst blackBox( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo )
{
  return arminst( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
}

arminst grayBox( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, eInstructionCode aCode )
{
  arminst inst( new BlackBoxInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo));
  inst->forceResync();
  DBG_(Tmp, (<<"In grayBox"));
  inst->setClass(clsSynchronizing, aCode);
  return inst;
}

arminst nop( armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo ) {
  SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode, aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
  inst->setClass(clsComputation, codeNOP);
  return arminst( inst );
}

arminst unallocated_encoding(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: unallocated_encoding \033[0m"));
    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
    /* Unallocated and reserved encodings are uncategorized */
}

arminst unsupported_encoding(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: unsupported_encoding \033[0m"));
    /* Unallocated and reserved encodings are uncategorized */
    return blackBox( aFetchedOpcode, aCPU, aSequenceNo);
    //throw Exception;
}


} // narmDecoder
