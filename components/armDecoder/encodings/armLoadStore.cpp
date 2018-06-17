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

#include "armLoadStore.hpp"

namespace narmDecoder {
using namespace nuArchARM;


void ldrex( SemanticInstruction * inst, uint32_t rs, uint32_t rs2,  uint32_t dest) {

  inst->setClass(clsAtomic, codeLDREX);

  std::vector< std::list<InternalDependance> > rs_deps(2);
  addAddressCompute( inst, rs_deps ) ;

  addReadXRegister(inst, 1, rs, rs_deps[0]);
  addReadXRegister(inst, 1, rs2, rs_deps[1]);

  predicated_dependant_action update_value = updateRMWValueAction( inst );
  inst->addDispatchAction( update_value );

  inst->setOperand( kOperand4, static_cast<uint64_t>(0xFF) );
  inst->addDispatchEffect( satisfy( inst, update_value.dependance ) );

  //obtain the loaded value and write it back to the reg file.
  predicated_dependant_action rmw;
  rmw = rmwAction( inst, kByte, kPD );
  inst->addDispatchEffect( allocateRMW( inst, kByte, rmw.dependance) );
  inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
  inst->addRetirementEffect( retireMem(inst) );
  inst->addCommitEffect( accessMem(inst) );
  inst->setMayCommit( false ) ; //Can't commit till memory-order speculation is resolved by the core
  inst->addSquashEffect( eraseLSQ(inst) );
  addDestination( inst, dest, rmw);
}
void strex(SemanticInstruction * inst, int srcReg, int addrReg, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
    inst->setClass(clsAtomic, codeSTREX);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    // get memory address
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, kAccType_ORDERED) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadRD( inst, srcReg );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
    inst->addCommitEffect( commitStore(inst) );
}

void STR(SemanticInstruction * inst, int srcReg, int addrReg, int size, eAccType type)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store General Reg \033[0m"));
    inst->setClass(clsStore, codeStore);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    // get memory address
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), false, type ));
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadRD( inst, srcReg );
    inst->addPostvalidation( validateMemory( kAddress, kOperand3, kResult, eSize(1<<size), inst ) );
    inst->addCommitEffect( commitStore(inst) );
}

void ldgpr(SemanticInstruction * inst, int destReg, int addrReg, int size, bool sign_extend)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Load General Reg \033[0m"));

    inst->setClass(clsLoad, codeLoad);

    std::vector< std::list<InternalDependance> > rs_deps(2);

    // compute address from register
    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);
//    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );

    predicated_dependant_action load;
    load = loadAction( inst, eSize(1<<size), sign_extend, kPD );

    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_NORMAL ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addDestination( inst, destReg, load);
}

void ldfpr(SemanticInstruction * inst, int addrReg, int destReg, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Load Floating Reg \033[0m"));

    inst->setClass(clsLoad, codeLoadFP);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );

    predicated_dependant_action load;
    load = loadFloatingAction( inst, eSize(1<<size), kPFD0, kPFD1);

    inst->addDispatchEffect( allocateLoad( inst, eSize(1<<size), load.dependance, kAccType_ORDERED ) );
    inst->addCommitEffect( accessMem(inst) );
    inst->addRetirementConstraint( loadMemoryConstraint(inst) );

    addFloatingDestination( inst, destReg, eSize(1<<size), load);
}



void stfpr(SemanticInstruction * inst, int addrReg, int dest, int size)
{
    DBG_(Tmp,(<< "\033[1;31m DECODER: Store Floating Reg \033[0m"));


    inst->setClass(clsStore, codeStoreFP);
    std::vector< std::list<InternalDependance> > rs_deps(2);

    addAddressCompute( inst, rs_deps ) ;
    addReadXRegister(inst, 1, addrReg, rs_deps[0]);

    inst->addAnnulmentEffect(  forceResync(inst) );
    inst->addSquashEffect( eraseLSQ(inst) );
    inst->addCheckTrapEffect( dmmuTranslationCheck(inst) );
    inst->addRetirementEffect( retireMem(inst) );

    inst->addDispatchEffect( allocateStore( inst, eSize(1<<size), true, kAccType_ORDERED ) );
    inst->addCommitEffect( commitStore(inst) );
    inst->addRetirementConstraint( storeQueueAvailableConstraint(inst) );
    inst->addRetirementConstraint( sideEffectStoreConstraint(inst) );

    addReadFValue( inst, dest, eSize(1<<size) );
}

} // narmDecoder
