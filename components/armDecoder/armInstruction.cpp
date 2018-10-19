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


#include <list>
#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/none.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include <core/qemu/mai_api.hpp>
#include <core/stats.hpp>

#include "armInstruction.hpp"
#include "SemanticInstruction.hpp"
#include "SemanticActions.hpp"
#include "Effects.hpp"
#include "Validations.hpp"
#include "Constraints.hpp"
#include "Interactions.hpp"
#include "armBitManip.hpp"

#include <bitset>         // std::bitset

#include "encodings/armEncodings.hpp"


#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

namespace Stat = Flexus::Stat;

using nuArchARM::xRegisters;
using nuArchARM::vRegisters;
using nuArchARM::ccBits;

using namespace nuArchARM;

void armInstruction::describe(std::ostream & anOstream) const {
  Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(theCPU);
  anOstream <<
            "#" << theSequenceNo << "[" << std::setfill('0') << std::right << std::setw(2) << cpu->id() <<  "] "
            << printInstClass() << " PC: @" << thePC  << " OPC: | " << std::hex << theOpcode << std::dec << " | ";
//            << std::endl << "QEMU disas: " << std::setfill(' ') << cpu->disassemble(thePC);
  if ( theRaisedException) {
    anOstream << " {raised " /*<< cpu->describeException(theRaisedException) << "(" << theRaisedException*/ << ")} ";
  }
  if (theResync) {
    anOstream << " {force-resync}";
  }
  if (haltDispatch()) {
    anOstream << " {halt-dispatch}";
  }
}

std::string armInstruction::disassemble() const {
  return Flexus::Qemu::Processor::getProcessor(theCPU)->disassemble(thePC);
}

void armInstruction::setWillRaise(eExceptionType aSetting) {
  theWillRaise = aSetting;
}

void armInstruction::doDispatchEffects(  ) {
  if (bpState() && ! isBranch()) {
    //Branch predictor identified an instruction that is not a branch as a
    //branch.
    DBG_( Tmp, ( << *this << " predicted as a branch, but is a non-branch.  Fixing" ) );

    boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
    feedback->thePC = pc();
    feedback->theActualType = kNonBranch;
    feedback->theActualDirection = kNotTaken;
    feedback->theActualTarget = VirtualMemoryAddress(0);
    feedback->theBPState = bpState();
    core()->branchFeedback(feedback);
    core()->applyToNext( boost::intrusive_ptr< nuArchARM::Instruction >( this ) , new BranchInteraction(VirtualMemoryAddress(0)) );

  }
}

bool armInstruction::usesIntAlu() const {
  return theUsesIntAlu;
}

bool armInstruction::usesIntMult() const {
  return theUsesIntMult;
}

bool armInstruction::usesIntDiv() const {
  return theUsesIntDiv;
}

bool armInstruction::usesFpAdd() const {
  return theUsesFpAdd;
}

bool armInstruction::usesFpCmp() const {
  return theUsesFpCmp;
}

bool armInstruction::usesFpCvt() const {
  return theUsesFpCvt;
}

bool armInstruction::usesFpMult() const {
  return theUsesFpMult;
}

bool armInstruction::usesFpDiv() const {
  return theUsesFpDiv;
}

bool armInstruction::usesFpSqrt() const {
  return theUsesFpSqrt;
}


std::pair< boost::intrusive_ptr<AbstractInstruction>, bool> decode( Flexus::SharedTypes::FetchedOpcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo, int32_t aUop )
{
    bool last_uop = true;
    boost::intrusive_ptr<AbstractInstruction> ret_val = disas_a64_insn(aFetchedOpcode, aCPU, aSequenceNo, aUop);
    return std::make_pair(ret_val, last_uop);
}




} //narmDecoder
