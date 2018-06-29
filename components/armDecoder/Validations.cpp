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

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;

static const int32_t kY = 34;
static const int32_t kCCR = 35;
static const int32_t kFPSR = 36;
static const int32_t kFPCR = 37;
static const int32_t kASI = 38;
static const int32_t kGSR = 40;
static const int32_t kPSTATE = 45;
static const int32_t kAG = 0x1;
static const int32_t kMG = 0x400;
static const int32_t kIG = 0x800;
static const int32_t kTL = 46;
static const int32_t kPIL = 47;
static const int32_t kTPC1 = 48;
static const int32_t kTNPC1 = 58;
static const int32_t kTSTATE1 = 68;
static const int32_t kTT1 = 78;
static const int32_t kTBA = 88;

static const int32_t kCWP = 90;
static const int32_t kCANSAVE = 91;
static const int32_t kCANRESTORE = 92;
static const int32_t kOTHERWIN = 93;
static const int32_t kWSTATE = 94;
static const int32_t kCLEANWIN = 95;

void overrideRegister::operator () () {
  uint64_t flexus = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( theReg );
  if (flexus != simics) {
    DBG_( Tmp, ( << *theInstruction << " overriding simics register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus << std::dec ));
    Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->writeRegister( theReg, flexus );
  }
}

void overrideFloatSingle::operator () () {
  uint64_t flexus = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readX( theReg & (~1) );
  uint64_t flexus_align = simics;
  if (theReg & 1) {
    flexus_align = ( (flexus_align &  0xFFFFFFFF00000000ULL) | (flexus & 0xFFFFFFFFULL) );
    if (flexus_align != simics) {
      DBG_( Tmp, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus_align << std::dec ));
      Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
    }
  } else {
    flexus_align = ( (flexus_align &  0xFFFFFFFFULL) | (flexus << 32) );
    if (flexus_align != simics) {
      DBG_( Tmp, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus_align << std::dec ));
      Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
    }
  }
}

void overrideFloatDouble::operator () () {
  uint64_t flexus_hi = theInstruction->operand< uint64_t > (theOperandCodeHi);
  uint64_t flexus_lo = theInstruction->operand< uint64_t > (theOperandCodeLo);
  uint64_t simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readX( theReg );
  uint64_t flexus_align = ( ( flexus_hi << 32 ) | (flexus_lo & 0xFFFFFFFFULL) );
  if (flexus_align != simics) {
    DBG_( Tmp, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCodeHi << ":" << theOperandCodeLo << " = " << flexus_align << std::dec ));
    Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
  }
}

bool validateRegister::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  if (theInstruction->raised()) {
    DBG_( Tmp, ( << " Not performing register validation for " << theReg << " because of exception. " << *theInstruction ) );
    return true;
  }

  uint64_t flexus = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t simics = 0;
//  if (theReg == nuArchARM::kRegY) {
//    simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kY );
//  } else if (theReg == nuArchARM::kRegASI) {
//    simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kASI );
//  } else if (theReg == nuArchARM::kRegGSR) {
//    simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kGSR );
//  } else {
    simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( theReg );
//  }

  DBG_( Tmp, ( << "Validating mapped_reg " << theReg << " flexus=" << std::hex << flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for mapped_reg " << theReg << " flexus=" << std::hex << flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );

  return (flexus == simics);
}

bool validateFRegister::operator () () {

  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  uint64_t flexus = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t simics = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readX( theReg & (~1) );
  if (theReg & 1) {
    simics &= 0xFFFFFFFFULL;
  } else {
    simics >>= 32;
  }
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for mapped_reg " << theReg << " flexus=" << std::hex << flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus == simics);
}

bool validateCC::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  std::bitset<8> flexus( theInstruction->operand< std::bitset<8> > (theOperandCode) );
  std::bitset<8> simics( Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kCCR ) );
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for CC bits flexus=" << flexus << " simics=" << simics << "\n" << std::internal << *theInstruction ) );
  return (flexus == simics);
}

bool validateFCC::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  std::bitset<8> flexus( theInstruction->operand< std::bitset<8> > (theOperandCode) );
  uint32_t flexus_fcc = flexus.to_ulong() & 3;
  uint64_t fpsr = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kFPSR );
  uint32_t simics_fcc = 0;
  switch (theFCC) {
    case 0:
      simics_fcc = ( fpsr >> 10) & 3;
      break;
    case 1:
      simics_fcc = ( fpsr >> 32) & 3;
      break;
    case 2:
      simics_fcc = ( fpsr >> 34) & 3;
      break;
    case 3:
      simics_fcc = ( fpsr >> 36) & 3;
      break;
    default:
      simics_fcc = 0xFFFF; //Force failure;
  }
  DBG_( Dev, Condition( flexus_fcc != simics_fcc) ( << "Validation Mismatch for FCC val flexus=" << fccVals(flexus_fcc) << " simics=" << fccVals(simics_fcc) << "\n" << std::internal << *theInstruction ) );
  return (flexus_fcc == simics_fcc);
}

bool validateMemory::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  VirtualMemoryAddress flexus_addr(theInstruction->operand< uint64_t > (theAddressCode));
  if (theInstruction->hasOperand( kUopAddressOffset ) ) {
    uint64_t offset = theInstruction->operand< uint64_t > (kUopAddressOffset);
    flexus_addr += offset;
  }

  int32_t asi( theInstruction->operand< uint64_t > (theASICode) );
  flexus_addr += theAddrOffset;
  uint64_t flexus_value = theInstruction->operand< uint64_t > (theValueCode);
  switch (theSize) {
    case kByte:
      flexus_value &= 0xFFULL;
      break;
    case kHalfWord:
      flexus_value &= 0xFFFFULL;
      break;
    case kWord:
      flexus_value &= 0xFFFFFFFFULL;
      break;
    case kDoubleWord:
    default:
      break;
  }

  Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
  Flexus::Qemu::Translation xlat;
  xlat.theVaddr = flexus_addr;
  xlat.theType = Flexus::Qemu::Translation::eLoad;
  xlat.theTL = c->readRegister( kTL );
  xlat.thePSTATE = c->readRegister( kPSTATE );
  xlat.theASI = asi;
  c->translate(xlat, false);
  if (xlat.isMMU()) {
    //bool mmu_ok = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->validateMMU();
    //if (! mmu_ok) {
    //DBG_( Dev, Condition( ! mmu_ok) ( << "Validation Mismatch for MMUs\n" << std::internal << *theInstruction ) );
    //Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->dumpMMU();
    //}
    return true;
  } else if (xlat.thePaddr > 0x400000000LL) {
    DBG_( Tmp, ( << "Non-memory store " << std::hex << asi << " flexus=" << flexus_value << " Insn: " << *theInstruction ) );
    return true;
  } else if (xlat.isTranslating() && !xlat.isSideEffect()) {
    //uint64_t simics_value = c->readVAddrXendian_QemuImpl( xlat.theVaddr, xlat.theASI, static_cast<int>(theSize) );
    //DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for address " << flexus_addr << " flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
    return 1; //(1 || flexus_value == simics_value);
  } else {
    DBG_( Tmp, ( << "No validation available for ASI 0x" << std::hex << asi << " flexus=" << flexus_value << " Insn: " << *theInstruction ) );
    return true;
  }
}

uint32_t simicsReg( uint32_t aPR) {
    assert(false);

//  switch (aPR) {
//    case 0: //TPC
//      return  kTPC1;
//    case 1: //NTPC
//      return  kTNPC1;
//    case 2: //TSTATE
//      return  kTSTATE1;
//    case 3: //TT
//      return  kTT1;
//    case 5: //TBA
//      return  kTBA;
//    case 6: //PSTATE
//      return  kPSTATE;
//    case 7: //TL
//      return  kTL;
//    case 8: //PIL
//      return  kPIL;
//    case 9: //CWP
//      return  kCWP;
//    case 10: //CANSAVE
//      return  kCANSAVE;
//    case 11: //CANRESTORE
//      return  kCANRESTORE;
//    case 12: //CLEANWIN
//      return  kCLEANWIN;
//    case 13: //OTHERWIN
//      return  kOTHERWIN;
//    case 14: //WSTATE
//      return  kWSTATE;
//    default:
//      DBG_( Tmp, ( << "Validate invalid PR: " << aPR ) );
//      return 0;
//      break;
//  }
}

bool validateTPR::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  uint64_t flexus_value = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t tl = theInstruction->operand< uint64_t > (theTLOperandCode);
  uint64_t simics_value = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( simicsReg( thePR ) + tl - 1);
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for " << theInstruction->core()->prName( thePR ) << " flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool doValidatePR( SemanticInstruction * theInstruction, eOperandCode theOperandCode, uint32_t thePR) {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  uint64_t flexus_value = theInstruction->operand< uint64_t > (theOperandCode);
  uint64_t simics_value = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( simicsReg( thePR )  );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for " << theInstruction->core()->prName( thePR ) << " flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool validatePR::operator () () {
  return doValidatePR( theInstruction, theOperandCode, thePR);
}

//bool validateSaveRestore::operator () () {
//  bool ok = true;
//  if (theInstruction->raised()) {
//    DBG_( Tmp, ( << "Save or restore instruction raised: \n" << *theInstruction ) );
//    return true;
//  } else {
//    ok = ok && doValidatePR( theInstruction, kocCWP, 9 /*CWP*/ );
//    ok = ok && doValidatePR( theInstruction, kocCANSAVE, 10 /*CANSAVE*/ );
//    ok = ok && doValidatePR( theInstruction, kocCANRESTORE, 11 /*CANRESTORE*/ );
//    return ok;
//  }
//}

bool validateFPCR::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }


  uint64_t flexus_value = theInstruction->operand< uint64_t > (kocFPSR);
  uint64_t simics_value = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kFPSR );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for FPSR flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool validateFPSR::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  uint64_t flexus_value = theInstruction->operand< uint64_t > (kocFPSR);
  uint64_t simics_value = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu())->readRegister( kFPSR );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for FPSR flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool validateLegalReturn::operator ()() {
    if (theInstruction->core()->getCurrentEL() == EL0) {
        return false;
    }
    return true;



};

} //narmDecoder
