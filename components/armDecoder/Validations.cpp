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


#include <iostream>
#include <iomanip>
#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>
#include <core/qemu/mai_api.hpp>

#include "SemanticInstruction.hpp"
#include "Effects.hpp"
#include "SemanticActions.hpp"
#include "Validations.hpp"
#include <components/uArchARM/systemRegister.hpp>

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;


bool validateXRegister::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  if (theInstruction->raised() != kException_None) {
    DBG_( VVerb, ( << " Not performing register validation for " << theReg << " because of exception. " << *theInstruction ) );
    return true;
  }

  uint64_t flexus = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t qemu = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readXRegister( theReg );

  DBG_(Dev,(<< "flexus value in " << theOperandCode << "  " << flexus ));
  DBG_(Dev,(<< "qemu value in " << theReg << "  " << qemu   ));

  DBG_( Dev, ( << "Validating mapped_reg " << theReg << " flexus=" << std::hex << flexus << " qemu=" << qemu << std::dec << "\n" ) );

  return (flexus == qemu);
}


bool validatePC::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  if (theInstruction->raised() != kException_None) {
    DBG_( VVerb, ( << " Not performing  validation because of exception. " << *theInstruction ) );
    return true;
  }

  bits flexus = theAddr;
  bits qemu = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readPC();

  DBG_( VVerb, ( << "Validating flexus PC=" << std::hex << flexus << " qemu PC=" << qemu << std::dec << "\n" << std::internal << *theInstruction ) );

  return (flexus == qemu);
}

bool validateVRegister::operator () () {

    return true;
//  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
//    return true; //Don't check
//  }

//  bits flexus = theInstruction->operand< bits > (theOperandCode);
//  bits simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readX( theReg & (~1) );
//  if (theReg & 1) {
//    simics &= 0xFFFFFFFFULL;
//  } else {
//    simics >>= 32;
//  }
//  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for mapped_reg " << theReg << " flexus=" << std::hex << flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );
//  return (flexus == simics);
}

bool validateMemory::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());

  VirtualMemoryAddress vaddr(theInstruction->operand< uint64_t > (theAddressCode));
  PhysicalMemoryAddress paddr = c->translateVirtualAddress(vaddr);



  bits flexus_val = theInstruction->operand< bits > (theValueCode);
  bits qemu_val = c->readPhysicalAddress(paddr, theSize);

  DBG_(Dev,(<< "flexus value: " << flexus_val ));
  DBG_(Dev,(<< "qemu value:   " << qemu_val   ));


  bool b = (flexus_val == qemu_val);

  return (flexus_val == qemu_val);
}



} //narmDecoder
