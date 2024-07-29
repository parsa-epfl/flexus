//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block

#include "coreModelImpl.hpp"
#include <execinfo.h>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

void CoreImpl::bypass(mapped_reg aReg, register_value aValue) {
  theBypassNetwork.write(aReg, aValue, *this);
}

void CoreImpl::connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst,
                             std::function<bool(register_value)> fn) {
  theBypassNetwork.connect(aReg, inst, fn);
}

void CoreImpl::mapRegister(mapped_reg aRegister) {
  FLEXUS_PROFILE();
  DBG_( VVerb, ( << theName << " Mapping " << aRegister ) );
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
  mapped_reg ret_val;
  ret_val.theType = aReg.theType;
  ret_val.theIndex = mapTable(aReg.theType).map(aReg.theIndex);
  return ret_val;
}

std::pair<mapped_reg, mapped_reg> CoreImpl::create(reg aReg) {
  FLEXUS_PROFILE();
  std::pair<mapped_reg, mapped_reg> mapped;
  mapped.first.theType = mapped.second.theType = aReg.theType;
    // (new register index, old register index)
  std::tie(mapped.first.theIndex, mapped.second.theIndex) = mapTable(aReg.theType).create(aReg.theIndex);
  mapRegister(mapped.first);

  eResourceStatus status = theRegisters.status(mapped.second);
  //DBG_Assert(status != kUnmapped, (<< " aRegister=" << mapped.second << " status=" << status));
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  /*
  DBG_Assert(mapTable(aReg.theType).checkInvariants(),
             (<< "MapTable Invariant check failed after creating new mapping for " << aReg
              << ", new mapping: " << mapped.first << ", old mapping: " << mapped.second
              << "MapTable: " << mapTable(aReg.theType)));
  */
  return mapped;
}

void CoreImpl::free(mapped_reg aReg) {
  FLEXUS_PROFILE();
  mapTable(aReg.theType).free(aReg.theIndex);
  unmapRegister(aReg);
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  /*
  DBG_Assert(mapTable(aReg.theType).checkInvariants(),
             (<< "MapTable Invariant check failed after freeing " << aReg
              << "MapTable: " << mapTable(aReg.theType)));
  */
}

void CoreImpl::restore(reg aName, mapped_reg aReg) {
  FLEXUS_PROFILE();
  DBG_Assert(aName.theType == aReg.theType);
  mapTable(aName.theType).restore(aName.theIndex, aReg.theIndex);
  // This assertion is extremely slow - 15% of total execution time.  Enable
  // at your own risk.
  /*
  DBG_Assert(mapTable(aReg.theType).checkInvariants(),
             (<< "MapTable Invariant check failed after restoring " << aReg
              << "MapTable: " << mapTable(aReg.theType)));
  */
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

void CoreImpl::getState(State &aState) {
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

void CoreImpl::restoreState(State &aState) {
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

void CoreImpl::compareState(State &aLeft, State &aRight) {
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

/* Msutherl: API to read system register value using QEMU encoding */
uint64_t CoreImpl::readUnhashedSysReg(uint8_t opc0, uint8_t opc1, uint8_t opc2, uint8_t crn,
                                      uint8_t crm) {
  return theQEMUCPU.read_sysreg(opc0, opc1, opc2, crn, crm);
}

} // namespace nuArch