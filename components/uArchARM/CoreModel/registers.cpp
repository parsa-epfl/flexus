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

#include "coreModelImpl.hpp"
#include <execinfo.h>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArchARM {

void CoreImpl::bypass(mapped_reg aReg, register_value aValue) {
  theBypassNetwork.write(aReg, aValue, *this);
}

void CoreImpl::connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst,
                             std::function<bool(register_value)> fn) {
  theBypassNetwork.connect(aReg, inst, fn);
}

void CoreImpl::mapRegister(mapped_reg aRegister) {
  FLEXUS_PROFILE();
  // DBG_( VVerb, ( << theName << " Mapping " << aRegister ) );
  eResourceStatus status = theRegisters.status(aRegister);
  DBG_Assert(status == kUnmapped, (<< " aRegister=" << aRegister << " status=" << status));
  theRegisters.map(aRegister);
}

void CoreImpl::unmapRegister(mapped_reg aRegister) {
  FLEXUS_PROFILE();
  eResourceStatus status = theRegisters.status(aRegister);
  DBG_Assert(status != kUnmapped, (<< " aRegister=" << aRegister << " status=" << status));
  theRegisters.unmap(aRegister);
  theBypassNetwork.unmap(aRegister);
}

eResourceStatus CoreImpl::requestRegister(mapped_reg aRegister,
                                          InstructionDependance const &aDependance) {
  return theRegisters.request(aRegister, aDependance);
}

eResourceStatus CoreImpl::requestRegister(mapped_reg aRegister) {
  return theRegisters.status(aRegister);
}

void CoreImpl::squashRegister(mapped_reg aRegister) {
  return theRegisters.squash(aRegister, *this);
}

register_value CoreImpl::readRegister(mapped_reg aRegister) {
  return theRegisters.read(aRegister);
}

register_value CoreImpl::readArchitecturalRegister(reg aRegister, bool aRotate) {
  reg reg = aRegister;
  if (aRotate) {
    reg = theArchitecturalWindowMap.rotate(reg);
  }
  mapped_reg mreg;
  mreg.theType = reg.theType;
  mreg.theIndex = mapTable(reg.theType).mapArchitectural(reg.theIndex);
  return theRegisters.peek(mreg);
}

void CoreImpl::writeRegister(mapped_reg aRegister, register_value aValue, bool isW = false) {
  return theRegisters.write(aRegister, aValue, *this, isW);
}

void CoreImpl::initializeRegister(mapped_reg aRegister, register_value aValue) {
  theRegisters.poke(aRegister, aValue);
  theRegisters.setStatus(aRegister, kReady);
}

void CoreImpl::copyRegValue(mapped_reg aSource, mapped_reg aDest) {
  theRegisters.poke(aDest, theRegisters.peek(aSource));
}

PhysicalMap &CoreImpl::mapTable(eRegisterType aMapTable) {
  return *theMapTables[aMapTable];
}

/*
  void CoreImpl::disconnectRegister( mapped_reg aReg, boost::intrusive_ptr<
  Instruction > inst) { theRegisters.disconnect( aReg, inst);
  }
*/

mapped_reg CoreImpl::map(reg aReg) {
  FLEXUS_PROFILE();
  reg reg = theWindowMap.rotate(aReg);
  mapped_reg ret_val;
  ret_val.theType = reg.theType;
  ret_val.theIndex = mapTable(reg.theType).map(reg.theIndex);
  return ret_val;
}

std::pair<mapped_reg, mapped_reg> CoreImpl::create(reg aReg) {
  FLEXUS_PROFILE();
  reg reg = theWindowMap.rotate(aReg);
  std::pair<mapped_reg, mapped_reg> mapped;
  mapped.first.theType = mapped.second.theType = aReg.theType;
  std::tie(mapped.first.theIndex, mapped.second.theIndex) =
      mapTable(reg.theType).create(reg.theIndex);
  mapRegister(mapped.first);

  eResourceStatus status = theRegisters.status(mapped.second);
  DBG_Assert(status != kUnmapped, (<< " aRegister=" << mapped.second << " status=" << status));
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  //  DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable
  //  Invariant check failed after freeing " << aReg ) );
  return mapped;
}

void CoreImpl::free(mapped_reg aReg) {
  FLEXUS_PROFILE();
  mapTable(aReg.theType).free(aReg.theIndex);
  unmapRegister(aReg);
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  // DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable
  // Invariant check failed after freeing " << aReg ) );
}

void CoreImpl::restore(reg aName, mapped_reg aReg) {
  FLEXUS_PROFILE();
  DBG_Assert(aName.theType == aReg.theType);
  reg name(theWindowMap.rotate(aName));
  mapTable(aName.theType).restore(name.theIndex, aReg.theIndex);
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  // DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable
  // Invariant check failed after freeing " << aReg ) );
}

void dec_mod8(uint32_t &anInt) {
  if (anInt == 0) {
    anInt = 7;
  } else {
    --anInt;
  }
}

void inc_mod8(uint32_t &anInt) {
  ++anInt;
  if (anInt >= 8) {
    anInt = 0;
  }
}

uint64_t CoreImpl::getXRegister(uint32_t aReg) {
  reg r;
  r.theType = xRegisters;
  r.theIndex = aReg;
  mapped_reg reg_p = map(r);
  register_value reg_val = readRegister(reg_p);
  return boost::get<uint64_t>(reg_val);
}

void CoreImpl::setXRegister(uint32_t aReg, uint64_t aVal) {
  reg r;
  r.theType = xRegisters;
  r.theIndex = aReg;
  mapped_reg reg_p = map(r);
  writeRegister(reg_p, aVal);
}

// void CoreImpl::setASI(uint64_t aVal) {
//    assert(false);

////  return setRRegister(kRegASI, aVal);
//}

// uint64_t CoreImpl::getASI() {
//    assert(false);
////  return getRRegister(kRegASI);
//    return 0;
//}

// void CoreImpl::writeFPSR( uint64_t anFPSR) {
//  theFPSR = anFPSR;
//  bits fcc0( ( anFPSR >> 10) & 3 );
//  bits fcc1( ( anFPSR >> 32) & 3 );
//  bits fcc2( ( anFPSR >> 34) & 3 );
//  bits fcc3( ( anFPSR >> 36) & 3 );

//  reg r;
//  r.theType = ccBits;

//  r.theIndex = 1;
//  theRegisters.poke( map(r), fcc0);
//  r.theIndex = 2;
//  theRegisters.poke( map(r), fcc1);
//  r.theIndex = 3;
//  theRegisters.poke( map(r), fcc2);
//  r.theIndex = 4;
//  theRegisters.poke( map(r), fcc3);
//  setRoundingMode( (anFPSR >> 30) & 3 );
//}

// uint64_t CoreImpl::readFPSR() {
//  uint64_t fpsr = theFPSR;
//  reg r;
//  r.theType = ccBits;

//  try {
//    r.theIndex = 1;
//    std::bitset<8> fcc0 = boost::get< bits >(theRegisters.peek( map(r) ));
//    r.theIndex = 2;
//    std::bitset<8> fcc1 = boost::get< bits >(theRegisters.peek( map(r) ));
//    r.theIndex = 3;
//    std::bitset<8> fcc2 = boost::get< std::bitset<8> >(theRegisters.peek(
//    map(r) )); r.theIndex = 4; std::bitset<8> fcc3 = boost::get<
//    std::bitset<8> >(theRegisters.peek( map(r) )); uint64_t fcc0_ull = (
//    fcc0.to_ulong() & 3); uint64_t fcc1_ull = ( fcc1.to_ulong() & 3); uint64_t
//    fcc2_ull = ( fcc2.to_ulong() & 3); uint64_t fcc3_ull = ( fcc3.to_ulong() &
//    3);
//    //Mask out all FCC fields
//    fpsr &= 0xFFFFFFC0FFFFF3FFULL;
//    fpsr |= (fcc0_ull << 10) | (fcc1_ull << 32) | (fcc2_ull << 34) | (fcc3_ull
//    << 36);
//  } catch (...) {
//    DBG_(Dev, ( << "readFPSR() threw an exception.  return an incorrect value
//    and let validation clean it up. (mapped_reg.theIndex = " << r.theIndex <<
//    ", map(mapped_reg) = " << map(r) << ")" ) );
//  }
//  return fpsr;
//}

void CoreImpl::getARMState(armState &aState) {
  reg r;
  r.theType = xRegisters;

  for (int32_t i = 0; i < 32; ++i) {
    r.theIndex = i;
    aState.theGlobalRegs[i] = boost::get<uint64_t>(readArchitecturalRegister(r, false));
  }
  r.theType = ccBits;
  r.theIndex = 0;
  aState.theCCRegs = boost::get<uint64_t>(readArchitecturalRegister(r, false));

  aState.thePC = pc();
  aState.theFPSR = getFPSR();
  aState.theFPCR = getFPCR();
  aState.thePSTATE = getPSTATE();
}

void CoreImpl::restoreARMState(armState &aState) {
  // Note - assumes that resetv9 has been called to clear core state

  for (int32_t i = 0; i < 32; ++i) {
    initializeRegister(xReg(i), aState.theGlobalRegs[i]);
  }

  initializeRegister(ccReg(0), aState.theCCRegs);
  //  for (int32_t i = 0 ; i < 64; ++i) {
  //    initializeRegister( wReg(i), aState.theFPRegs[i]);
  //  }
  uint64_t fpsr = aState.theFPSR;

  setPC(aState.thePC);
  setFPSR(aState.theFPSR);
  setRoundingMode((fpsr >> 30) & 3);
  setFPCR(aState.theFPCR);
  setPSTATE(aState.thePSTATE);
}

void CoreImpl::compareARMState(armState &aLeft, armState &aRight) {
  for (int32_t i = 0; i < 32; ++i) {
    DBG_Assert((aLeft.theGlobalRegs[i] == aRight.theGlobalRegs[i]),
               (<< "aLeft.theGlobalRegs[" << i << "] = " << std::hex << aLeft.theGlobalRegs[i]
                << std::dec << ", aRight.theGlobalRegs[" << i << "] = " << std::hex
                << aRight.theGlobalRegs[i] << std::dec));
  }
  for (int32_t i = 0; i < 64; ++i) {
    DBG_Assert((aLeft.theFPRegs[i] == aRight.theFPRegs[i]),
               (<< "aLeft.theFPRegs[" << i << "] = " << std::hex << aLeft.theFPRegs[i] << std::dec
                << ", aRight.theFPRegs[" << i << "] = " << std::hex << aRight.theFPRegs[i]
                << std::dec));
  }

  DBG_Assert((aLeft.thePC == aRight.thePC), (<< std::hex << "aLeft.thePC = " << aLeft.thePC
                                             << ", aRight.thePC = " << aRight.thePC << std::dec));

  DBG_Assert((aLeft.theFPCR == aRight.theFPCR),
             (<< std::hex << "aLeft.theFPCR = " << aLeft.theFPCR
              << ", aRight.theFPCR = " << aRight.theFPCR << std::dec));
  DBG_Assert((aLeft.theFPSR == aRight.theFPSR),
             (<< std::hex << "aLeft.theFPSR = " << aLeft.theFPSR
              << ", aRight.theFPSR = " << aRight.theFPSR << std::dec));
  DBG_Assert((aLeft.thePSTATE == aRight.thePSTATE),
             (<< std::hex << "aLeft.thePSTATE = " << aLeft.thePSTATE
              << ", aRight.thePSTATE = " << aRight.thePSTATE << std::dec));
  //  DBG_Assert( (aLeft.theASI == aRight.theASI), ( << std::hex <<
  //  "aLeft.theASI = "  << aLeft.theASI << ", aRight.theASI = " <<
  //  aRight.theASI << std::dec ));
}

} // namespace nuArchARM
