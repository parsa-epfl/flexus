#include "coreModelImpl.hpp"
#include <execinfo.h>

#define DBG_DeclareCategories uArchCat
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

void CoreImpl::bypass(mapped_reg aReg, register_value aValue) {
  theBypassNetwork.write(aReg, aValue, *this);
}

void CoreImpl::connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst, boost::function< bool( register_value) > fn) {
  theBypassNetwork.connect(aReg, inst, fn);
}

void CoreImpl::mapRegister( mapped_reg aRegister ) {
  FLEXUS_PROFILE();
  DBG_( Verb, ( << theName << " Mapping " << aRegister ) );
  eResourceStatus status = theRegisters.status( aRegister ) ;
  DBG_Assert( status == kUnmapped, ( << " aRegister=" << aRegister << " status=" << status ));
  theRegisters.map( aRegister );
}

void CoreImpl::unmapRegister( mapped_reg aRegister ) {
  FLEXUS_PROFILE();
  eResourceStatus status = theRegisters.status( aRegister ) ;
  DBG_Assert( status != kUnmapped, ( << " aRegister=" << aRegister << " status=" << status ));
  theRegisters.unmap( aRegister );
  theBypassNetwork.unmap(aRegister);
}

eResourceStatus CoreImpl::requestRegister( mapped_reg aRegister, InstructionDependance const & aDependance ) {
  return theRegisters.request(aRegister, aDependance);
}

eResourceStatus CoreImpl::requestRegister( mapped_reg aRegister ) {
  return theRegisters.status(aRegister);
}

void CoreImpl::squashRegister( mapped_reg aRegister) {
  return theRegisters.squash(aRegister, *this);
}

register_value CoreImpl::readRegister( mapped_reg aRegister ) {
  return theRegisters.read( aRegister );
}

register_value CoreImpl::readArchitecturalRegister( unmapped_reg aRegister, bool aRotate ) {
  unmapped_reg reg = aRegister;
  if (aRotate) {
    reg = theArchitecturalWindowMap.rotate(reg);
  }
  mapped_reg mreg;
  mreg.theType = reg.theType;
  mreg.theIndex = mapTable(reg.theType).mapArchitectural(reg.theIndex);
  return theRegisters.peek( mreg );
}

void CoreImpl::writeRegister( mapped_reg aRegister, register_value aValue ) {
  return theRegisters.write( aRegister, aValue, *this );
}

void CoreImpl::initializeRegister(mapped_reg aRegister, register_value aValue) {
  theRegisters.poke( aRegister, aValue);
  theRegisters.setStatus( aRegister, kReady );
}

void CoreImpl::copyRegValue(mapped_reg aSource, mapped_reg aDest) {
  theRegisters.poke(aDest, theRegisters.peek(aSource) );
}

/*
  void CoreImpl::disconnectRegister( mapped_reg aReg, boost::intrusive_ptr< Instruction > inst) {
    theRegisters.disconnect( aReg, inst);
  }
*/

PhysicalMap & CoreImpl::mapTable( eRegisterType aMapTable ) {
  return * theMapTables[aMapTable];
}

mapped_reg CoreImpl::map( unmapped_reg aReg ) {
  FLEXUS_PROFILE();
  unmapped_reg reg = theWindowMap.rotate(aReg);
  mapped_reg ret_val;
  ret_val.theType = reg.theType;
  ret_val.theIndex = mapTable(reg.theType).map(reg.theIndex);
  return ret_val;
}

std::pair<mapped_reg, mapped_reg> CoreImpl::create( unmapped_reg aReg ) {
  FLEXUS_PROFILE();
  unmapped_reg reg = theWindowMap.rotate(aReg);
  std::pair<mapped_reg, mapped_reg> mapped;
  mapped.first.theType = mapped.second.theType = aReg.theType;
  boost::tie( mapped.first.theIndex, mapped.second.theIndex) = mapTable(reg.theType).create(reg.theIndex);
  mapRegister(mapped.first);

  eResourceStatus status = theRegisters.status( mapped.second ) ;
  DBG_Assert( status != kUnmapped, ( << " aRegister=" << mapped.second << " status=" << status ));
  //This assertion is extremely slow - 15% of total execution time.  Enable
  //at your own risk.
  //DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable Invariant check failed after freeing " << aReg ) );
  return mapped;
}

void CoreImpl::free( mapped_reg aReg ) {
  FLEXUS_PROFILE();
  mapTable(aReg.theType).free(aReg.theIndex);
  unmapRegister( aReg );
  //This assertion is extremely slow - 15% of total execution time.  Enable
  //at your own risk.
  //DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable Invariant check failed after freeing " << aReg ) );
}

void CoreImpl::restore( unmapped_reg aName, mapped_reg aReg ) {
  FLEXUS_PROFILE();
  DBG_Assert( aName.theType == aReg.theType );
  unmapped_reg name(theWindowMap.rotate(aName) );
  mapTable(aName.theType).restore(name.theIndex, aReg.theIndex);
  //This assertion is extremely slow - 15% of total execution time.  Enable
  //at your own risk.
  //DBG_Assert( mapTable(aReg.theType).checkInvariants(), ( << "MapTable Invariant check failed after freeing " << aReg ) );
}

void dec_mod8(uint32_t & anInt) {
  if (anInt == 0) {
    anInt = 7;
  } else {
    --anInt;
  }
}

void inc_mod8(uint32_t & anInt) {
  ++anInt;
  if (anInt >= 8 ) {
    anInt = 0;
  }
}

uint32_t CoreImpl::saveWindow() {
  theWindowMap.incrementCWP();
  return getCWP();
}

void CoreImpl::saveWindowPriv() {
  theArchitecturalWindowMap.incrementCWP();
  if (theCANSAVE == 0) {
    return; //SAVE will raise a spill exception
  } else {
    if (theCLEANWIN - theCANRESTORE == 0) {
      return; //SAVE will raise a cleanwin exception
    }
  }
  inc_mod8(theCANRESTORE);
  dec_mod8(theCANSAVE);
}

void CoreImpl::saved() {
  inc_mod8(theCANSAVE);
  if (theOTHERWIN == 0) {
    dec_mod8(theCANRESTORE);
  } else {
    dec_mod8(theOTHERWIN);
  }
}

uint32_t CoreImpl::restoreWindow() {
  theWindowMap.decrementCWP();
  return getCWP();
}

void CoreImpl::restoreWindowPriv() {
  theArchitecturalWindowMap.decrementCWP();
  if (theCANRESTORE == 0) {
    return; //RESTORE will raise a fill exception
  }
  dec_mod8(theCANRESTORE);
  inc_mod8(theCANSAVE);
}

void CoreImpl::restored() {
  inc_mod8(theCANRESTORE);
  if (theCLEANWIN < 7) {
    inc_mod8(theCLEANWIN);
  }
  if (theOTHERWIN == 0) {
    dec_mod8(theCANSAVE);
  } else {
    dec_mod8(theOTHERWIN);
  }
}

uint32_t CoreImpl::getCWP() {
  return theWindowMap.getCWP();
}

uint64_t CoreImpl::getRRegister(uint32_t aReg) {
  unmapped_reg reg;
  reg.theType = rRegisters;
  reg.theIndex = aReg;
  mapped_reg reg_p = map(reg);
  register_value reg_val = readRegister( reg_p );
  return boost::get<uint64_t>(reg_val);
}

void CoreImpl::setRRegister(uint32_t aReg, uint64_t aVal) {
  unmapped_reg reg;
  reg.theType = rRegisters;
  reg.theIndex = aReg;
  mapped_reg reg_p = map(reg);
  writeRegister( reg_p, aVal );
}

void CoreImpl::setASI(uint64_t aVal) {
  return setRRegister(kRegASI, aVal);
}

uint64_t CoreImpl::getASI() {
  return getRRegister(kRegASI);
}

void CoreImpl::setCCR(uint32_t aValue) {
  unmapped_reg reg;
  reg.theType = ccBits;
  reg.theIndex = 0;
  mapped_reg reg_p = map(reg);
  writeRegister( reg_p, std::bitset<8>(aValue));
}

uint32_t CoreImpl::getCCR() {
  unmapped_reg reg;
  reg.theType = ccBits;
  reg.theIndex = 0;
  mapped_reg reg_p = map(reg);
  if ( theRegisters.status(reg_p) == kReady ) {
    register_value ccr_val = readRegister( reg_p );
    std::bitset<8> ccbits = boost::get<std::bitset<8> >(ccr_val);
    return ccbits.to_ulong();
  } else {
    DBG_( Crit, ( << theName << " CCR is not ready " ) );
    return 0xDEAD;
  }
}

uint32_t CoreImpl::getCCRArchitectural() {
  mapped_reg mreg;
  mreg.theType = ccBits;
  mreg.theIndex = mapTable( ccBits ).mapArchitectural(0);
  std::bitset<8> ccbits = boost::get<std::bitset<8> >(theRegisters.peek( mreg ));
  return ccbits.to_ulong();
}

void CoreImpl::writeFSR( uint64_t anFSR) {
  theFSR = anFSR;
  std::bitset<8> fcc0( ( anFSR >> 10) & 3 );
  std::bitset<8> fcc1( ( anFSR >> 32) & 3 );
  std::bitset<8> fcc2( ( anFSR >> 34) & 3 );
  std::bitset<8> fcc3( ( anFSR >> 36) & 3 );

  unmapped_reg reg;
  reg.theType = ccBits;

  reg.theIndex = 1;
  theRegisters.poke( map(reg), fcc0);
  reg.theIndex = 2;
  theRegisters.poke( map(reg), fcc1);
  reg.theIndex = 3;
  theRegisters.poke( map(reg), fcc2);
  reg.theIndex = 4;
  theRegisters.poke( map(reg), fcc3);
  setRoundingMode( (anFSR >> 30) & 3 );
}

uint64_t CoreImpl::readFSR() {
  uint64_t fsr = theFSR;
  unmapped_reg reg;
  reg.theType = ccBits;

  try {
    reg.theIndex = 1;
    std::bitset<8> fcc0 = boost::get< std::bitset<8> >(theRegisters.peek( map(reg) ));
    reg.theIndex = 2;
    std::bitset<8> fcc1 = boost::get< std::bitset<8> >(theRegisters.peek( map(reg) ));
    reg.theIndex = 3;
    std::bitset<8> fcc2 = boost::get< std::bitset<8> >(theRegisters.peek( map(reg) ));
    reg.theIndex = 4;
    std::bitset<8> fcc3 = boost::get< std::bitset<8> >(theRegisters.peek( map(reg) ));
    uint64_t fcc0_ull = ( fcc0.to_ulong() & 3);
    uint64_t fcc1_ull = ( fcc1.to_ulong() & 3);
    uint64_t fcc2_ull = ( fcc2.to_ulong() & 3);
    uint64_t fcc3_ull = ( fcc3.to_ulong() & 3);
    //Mask out all FCC fields
    fsr &= 0xFFFFFFC0FFFFF3FFULL;
    fsr |= (fcc0_ull << 10) | (fcc1_ull << 32) | (fcc2_ull << 34) | (fcc3_ull << 36);
  } catch (...) {
    DBG_(Dev, ( << "readFSR() threw an exception.  return an incorrect value and let validation clean it up. (reg.theIndex = " << reg.theIndex << ", map(reg) = " << map(reg) << ")" ) );
  }
  return fsr;
}

int32_t CoreImpl::selectedRegisterSet() const {
  if (theArchitecturalWindowMap.theMG) {
    return 2;
  } else if (theArchitecturalWindowMap.theIG) {
    return 3;
  } else if (theArchitecturalWindowMap.theAG) {
    return 1;
  } else {
    return 0;
  }
}

void CoreImpl::getv9State( v9State & aState) {
  unmapped_reg reg;
  reg.theType = rRegisters;

  for (int32_t i = 0; i < 32; ++i) {
    reg.theIndex = i;
    aState.theGlobalRegs[i] = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  }
  for (int32_t i = 0 ; i < 8 * 16; ++i) {
    reg.theIndex = i + 32;
    aState.theWindowRegs[i] = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  }

  reg.theType = fRegisters;
  for (int32_t i = 0 ; i < 64; ++i) {
    reg.theIndex = i;
    aState.theFPRegs[i] = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  }
  reg.theType = rRegisters;

  aState.thePC = pc();
  aState.theNPC = npc();
  aState.theTBA = getTBA();
  aState.theCWP = theArchitecturalWindowMap.getCWP();
  aState.theCCR = getCCRArchitectural();
  aState.theFPRS = getFPRS();
  aState.theFSR = getFSR();
  aState.thePSTATE = getPSTATE();
  reg.theIndex = kRegASI;
  aState.theASI = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  reg.theIndex = kRegGSR;
  aState.theGSR = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  aState.theTL = getTL();
  aState.thePIL = getPIL();
  aState.theCANSAVE = getCANSAVE();
  aState.theCANRESTORE = getCANRESTORE();
  aState.theCLEANWIN = getCLEANWIN();
  aState.theOTHERWIN = getOTHERWIN();
  aState.theSOFTINT = 0; //FIXME
  aState.theWSTATE = getWSTATE();

  reg.theIndex = kRegY;
  aState.theY = boost::get<uint64_t>(readArchitecturalRegister( reg, false ));
  aState.theGLOBALS = 0; //FIXME
  aState.theTICK = getTICK();
  aState.theTICK_CMPR = 0; //FIXME
  aState.theSTICK = getSTICK();
  aState.theSTICK_CMPR = 0; //FIXME
  for (int32_t i = 0; i < 5; ++i) {
    aState.theTTE[i].theTPC = getTPC(i + 1);
    aState.theTTE[i].theTNPC = getTNPC(i + 1);
    aState.theTTE[i].theTSTATE = getTSTATE(i + 1);
    aState.theTTE[i].theTT = getTT(i + 1);
  }
}

void CoreImpl::restorev9State( v9State & aState) {
  //Note - assumes that resetv9 has been called to clear core state

  for (int32_t i = 0; i < 32; ++i) {
    initializeRegister( rReg(i), aState.theGlobalRegs[i]);
  }
  for (int32_t i = 0 ; i < 8 * 16; ++i) {
    initializeRegister( rReg(i + 32), aState.theWindowRegs[i]);
  }
  for (int32_t i = 0 ; i < 64; ++i) {
    initializeRegister( fReg(i), aState.theFPRegs[i]);
  }

  setPC(aState.thePC);
  setNPC(aState.theNPC);
  setTBA(aState.theTBA);
  setCWP(aState.theCWP);

  //CCRs
  std::bitset<8> ccbits(aState.theCCR);
  initializeRegister( ccReg(0), ccbits);
  uint64_t fsr = aState.theFSR;
  std::bitset<8> fcc0( ( fsr >> 10) & 3 );
  std::bitset<8> fcc1( ( fsr >> 32) & 3 );
  std::bitset<8> fcc2( ( fsr >> 34) & 3 );
  std::bitset<8> fcc3( ( fsr >> 36) & 3 );
  initializeRegister( ccReg(1), fcc0);
  initializeRegister( ccReg(2), fcc1);
  initializeRegister( ccReg(3), fcc2);
  initializeRegister( ccReg(4), fcc3);

  setFPRS( aState.theFPRS );
  setRoundingMode( (fsr >> 30) & 3 );
  setFSR( aState.theFSR );
  setPSTATE( aState.thePSTATE);

  initializeRegister( rReg( kRegASI ), aState.theASI);
  initializeRegister( rReg( kRegGSR ), aState.theGSR);

  setTL( aState.theTL );
  setPIL( aState.thePIL );
  setCANSAVE( aState.theCANSAVE );
  setCANRESTORE( aState.theCANRESTORE );
  setCLEANWIN( aState.theCLEANWIN );
  setOTHERWIN( aState.theOTHERWIN );
  setWSTATE( aState.theWSTATE );

  initializeRegister( rReg( kRegY ), aState.theY);

  //Do not restore TICK
  //Do not restore STICK

  for (int32_t i = 0; i < 5; ++i) {
    setTPC( aState.theTTE[i].theTPC, i + 1 );
    setTNPC( aState.theTTE[i].theTNPC, i + 1 );
    setTSTATE( aState.theTTE[i].theTSTATE, i + 1 );
    setTT( aState.theTTE[i].theTT, i + 1 );
  }

}

void CoreImpl::comparev9State( v9State & aLeft, v9State & aRight) {
  for (int32_t i = 0; i < 32; ++i) {
    DBG_Assert( (aLeft.theGlobalRegs[i] == aRight.theGlobalRegs[i]), ( << "aLeft.theGlobalRegs[" << i << "] = "  << std::hex << aLeft.theGlobalRegs[i] << std::dec << ", aRight.theGlobalRegs[" << i  << "] = " << std::hex << aRight.theGlobalRegs[i] << std::dec ));
  }
  for (int32_t i = 0 ; i < 8 * 16; ++i) {
    DBG_Assert( (aLeft.theWindowRegs[i] == aRight.theWindowRegs[i]), ( << "aLeft.theWindowRegs[" << i << "] = "  << std::hex << aLeft.theWindowRegs[i] << std::dec << ", aRight.theWindowRegs[" << i  << "] = " << std::hex << aRight.theWindowRegs[i] << std::dec  ));
  }
  for (int32_t i = 0 ; i < 64; ++i) {
    DBG_Assert( (aLeft.theFPRegs[i] == aRight.theFPRegs[i]), ( << "aLeft.theFPRegs[" << i << "] = "  << std::hex << aLeft.theFPRegs[i] << std::dec << ", aRight.theFPRegs[" << i  << "] = " << std::hex << aRight.theFPRegs[i] << std::dec  ));
  }

  DBG_Assert( (aLeft.thePC == aRight.thePC), ( << std::hex << "aLeft.thePC = "  << aLeft.thePC << ", aRight.thePC = " << aRight.thePC << std::dec ));
  DBG_Assert( (aLeft.theNPC == aRight.theNPC), ( << std::hex << "aLeft.theNPC = "  << aLeft.theNPC << ", aRight.theNPC = " << aRight.theNPC << std::dec ));
  DBG_Assert( (aLeft.theTBA == aRight.theTBA), ( << std::hex << "aLeft.theTBA = "  << aLeft.theTBA << ", aRight.theTBA = " << aRight.theTBA << std::dec ));
  DBG_Assert( (aLeft.theCWP == aRight.theCWP), ( << std::hex << "aLeft.theCWP = "  << aLeft.theCWP << ", aRight.theCWP = " << aRight.theCWP << std::dec ));

  DBG_Assert( (aLeft.theFSR == aRight.theFSR), ( << std::hex << "aLeft.theFSR = "  << aLeft.theFSR << ", aRight.theFSR = " << aRight.theFSR << std::dec ));
  DBG_Assert( (aLeft.theFPRS == aRight.theFPRS), ( << std::hex << "aLeft.theFPRS = "  << aLeft.theFPRS << ", aRight.theFPRS = " << aRight.theFPRS << std::dec ));
  DBG_Assert( (aLeft.thePSTATE == aRight.thePSTATE), ( << std::hex << "aLeft.thePSTATE = "  << aLeft.thePSTATE << ", aRight.thePSTATE = " << aRight.thePSTATE << std::dec ));
  DBG_Assert( (aLeft.theASI == aRight.theASI), ( << std::hex << "aLeft.theASI = "  << aLeft.theASI << ", aRight.theASI = " << aRight.theASI << std::dec ));
  DBG_Assert( (aLeft.theGSR == aRight.theGSR), ( << std::hex << "aLeft.theGSR = "  << aLeft.theGSR << ", aRight.theGSR = " << aRight.theGSR << std::dec ));
  DBG_Assert( (aLeft.theTL == aRight.theTL), ( << std::hex << "aLeft.theTL = "  << aLeft.theTL << ", aRight.theTL = " << aRight.theTL << std::dec ));
  DBG_Assert( (aLeft.thePIL == aRight.thePIL), ( << std::hex << "aLeft.thePIL = "  << aLeft.thePIL << ", aRight.thePIL = " << aRight.thePIL << std::dec ));
  DBG_Assert( (aLeft.theCANSAVE == aRight.theCANSAVE), ( << std::hex << "aLeft.theCANSAVE = "  << aLeft.theCANSAVE << ", aRight.theCANSAVE = " << aRight.theCANSAVE << std::dec ));
  DBG_Assert( (aLeft.theCANRESTORE == aRight.theCANRESTORE), ( << std::hex << "aLeft.theCANRESTORE = "  << aLeft.theCANRESTORE << ", aRight.theCANRESTORE = " << aRight.theCANRESTORE << std::dec ));
  DBG_Assert( (aLeft.theCLEANWIN == aRight.theCLEANWIN), ( << std::hex << "aLeft.theCLEANWIN = "  << aLeft.theCLEANWIN << ", aRight.theCLEANWIN = " << aRight.theCLEANWIN << std::dec ));
  DBG_Assert( (aLeft.theOTHERWIN == aRight.theOTHERWIN), ( << std::hex << "aLeft.theOTHERWIN = "  << aLeft.theOTHERWIN << ", aRight.theOTHERWIN = " << aRight.theOTHERWIN << std::dec ));
  DBG_Assert( (aLeft.theWSTATE == aRight.theWSTATE), ( << std::hex << "aLeft.theWSTATE = "  << aLeft.theWSTATE << ", aRight.theWSTATE = " << aRight.theWSTATE << std::dec ));
  DBG_Assert( (aLeft.theY == aRight.theY), ( << std::hex << "aLeft.theY = "  << aLeft.theY << ", aRight.theY = " << aRight.theY << std::dec ));

  //Do not verify TICK
  //Do not verify STICK

  for (int32_t i = 0; i < 5; ++i) {
    DBG_Assert( (aLeft.theTTE[i].theTPC == aRight.theTTE[i].theTPC), ( << "aLeft.theTTE[" << i << "].theTPC = "  << std::hex << aLeft.theTTE[i].theTPC << std::dec << ", aRight.theTTE[" << i << "].theTPC = " << std::hex << aRight.theTTE[i].theTPC << std::dec ));
    DBG_Assert( (aLeft.theTTE[i].theTNPC == aRight.theTTE[i].theTNPC), ( << "aLeft.theTTE[" << i << "].theTNPC = "  << std::hex << aLeft.theTTE[i].theTNPC << std::dec << ", aRight.theTTE[" << i << "].theTNPC = " << std::hex << aRight.theTTE[i].theTNPC << std::dec ));
    DBG_Assert( (aLeft.theTTE[i].theTSTATE == aRight.theTTE[i].theTSTATE), ( << "aLeft.theTTE[" << i << "theTSTATE = "  << std::hex << aLeft.theTTE[i].theTSTATE << std::dec << ", aRight.theTTE[" << i << "].theTSTATE = " << std::hex << aRight.theTTE[i].theTSTATE << std::dec ));
    DBG_Assert( (aLeft.theTTE[i].theTT == aRight.theTTE[i].theTT), ( << "aLeft.theTTE[" << i << "].theTT = "  << std::hex << aLeft.theTTE[i].theTT << std::dec << ", aRight.theTTE[" << i << "].theTT = " << std::hex << aRight.theTTE[i].theTT << std::dec ));
  }

}

} //nuArch
