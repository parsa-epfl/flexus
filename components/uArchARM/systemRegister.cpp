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

#include <functional>
#include <string>
#include <iostream>
#include "systemRegister.hpp"
#include "CoreModel/coreModelImpl.hpp"


namespace nuArchARM {

    /* Minimal set of EL0-visible registers. This will need to be expanded
     * significantly for system emulation of AArch64 CPUs.
     */
struct NZCV : public SysRegInfo {
    std::string name = "NZCV";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 3;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 2;
    eAccessRight access = kPL0_RW;
    eRegInfo type = kARM_NZCV;
    uint64_t resetvalue = -1;
    virtual void writefn (uArchARM * aCore, uint64_t aVal) override {
        std::bitset<8> a(aCore->_PSTATE().d());
        std::bitset<8> b(aVal);

        CORE_DBG("PSTATE VALUE BEFORE NZCV UPDATE: "  << a <<  " -- NZCV: " << b);
        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 28, 4, aVal));
        CORE_DBG( "PSTATE VALUE AFTER NZCV UPDATE: " << std::bitset<8>(aCore->_PSTATE().d()) );

    }// FIXME
    
}NZCV_;


struct DAIF : public SysRegInfo {
    std::string name = "DAIF";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 3;
    uint8_t opc2 = 1;
    uint8_t crn = 4;
    uint8_t crm = 2;
    eAccessRight access = kPL0_RW;
    eRegInfo type = kARM_NO_RAW;
    uint64_t resetvalue = -1;

    virtual eAccessResult accessfn (uArchARM* aCore) {DBG_Assert(false); return kACCESS_TRAP; }// FIXME /*aa64_daif_access*/
    virtual void writefn (uArchARM * aCore, uint64_t aVal) override {DBG_Assert(false); }// FIXME
    virtual void reset (uArchARM * aCore) override {DBG_Assert(false);  }// FIXME /*arm_cp_reset_ignore*/
}DAIF_;


struct FPCR : public SysRegInfo {
    std::string name = "FPCR";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 3;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 4;
    eAccessRight access = kPL0_RW;
    eRegInfo type;
    uint64_t resetvalue = -1;

     virtual uint64_t readfn (uArchARM * aCore) override {DBG_Assert(false); return kACCESS_TRAP; }// FIXME /*aa64_fpcr_read*/
     virtual void writefn (uArchARM * aCore, uint64_t aVal) override {DBG_Assert(false);}// FIXME /*aa64_fpcr_write*/
}FPCR_;

struct FPSR : public SysRegInfo {
    std::string name = "FPSR";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 3;
    uint8_t opc2 = 1;
    uint8_t crn = 4;
    uint8_t crm = 4;
    eAccessRight access = kPL0_RW;
    eRegInfo type;
    uint64_t resetvalue = -1;

     virtual uint64_t readfn (uArchARM * aCore) override  {DBG_Assert(false); return 0; }/*aa64_fpsr_read*/
     virtual void writefn (uArchARM * aCore, uint64_t aVal) override {DBG_Assert(false); } /*aa64_fpsr_write*/
}FPSR_;

struct DCZID_EL0 : public SysRegInfo {
    std::string name = "DCZID_EL0";
    eRegExecutionState state;
    uint8_t opc0 = 3;
    uint8_t opc1 = 3;
    uint8_t opc2 = 7;
    uint8_t crn = 0;
    uint8_t crm = 0;
    eAccessRight access = kPL0_R;
    eRegInfo type = kARM_NO_RAW;
    uint64_t resetvalue = -1;

    virtual uint64_t readfn(uArchARM * aCore) override {
         return aCore->readDCZID_EL0();
     }
}DCZID_EL0_;

struct DC_ZVA : public SysRegInfo {
    std::string name = "DC_ZVA";
    eRegExecutionState state;
    uint8_t opc0 = 1;
    uint8_t opc1 = 3;
    uint8_t opc2 = 7;
    uint8_t crn = 4;
    uint8_t crm = 1;
    eAccessRight access = kPL0_W;
    eRegInfo type = kARM_DC_ZVA;
    uint64_t resetvalue = -1;
    virtual eAccessResult accessfn(uArchARM * aCore) override {
         return aCore->accessZVA();
     }
}DC_ZVA_;

struct CURRENT_EL : public SysRegInfo {
    std::string name = "CURRENT_EL";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 0;
    uint8_t opc2 = 2;
    uint8_t crn = 4;
    uint8_t crm = 2;
    eAccessRight access = kPL1_R;
    eRegInfo type = kARM_CURRENTEL;
    uint64_t resetvalue = -1;

}CURRENT_EL_;

struct ELR_EL1 : public SysRegInfo {
    std::string name = "ELR_EL1";
    eRegExecutionState state  = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 0;
    uint8_t opc2 = 1;
    uint8_t crn = 4;
    uint8_t crm = 0;
    eAccessRight access = kPL1_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;
}ELR_EL1_;

struct SPSR_EL1 : public SysRegInfo {
    std::string name = "SPSR_EL1";
    eRegExecutionState state  = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 0;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 0;
    eAccessRight access = kPL1_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;
}SPSR_EL1_;


/* We rely on the access checks not allowing the guest to write to the
* state field when SPSel indicates that it's being used as the stack
* pointer.
*/
struct SP_EL0 : public SysRegInfo {
    std::string name = "SP_EL0";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 0;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 1;
    eAccessRight access = kPL1_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;


     virtual eAccessResult accessfn (uArchARM* aCore) {DBG_Assert(false); return kACCESS_TRAP; }// FIXME /*sp_el0_access*/


}SP_EL0_;

struct SP_EL1 : public SysRegInfo {
    std::string name = "SP_EL1";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 4;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 1;
    eAccessRight access = kPL2_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;

}SP_EL1_;

struct SPSel : public SysRegInfo {
    std::string name = "SPSel";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 0;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 2;
    eAccessRight access = kPL1_RW;
    eRegInfo type = kARM_NO_RAW;
    uint64_t resetvalue = -1;

     virtual uint64_t readfn (uArchARM * aCore) override {DBG_Assert(false); return 0;} /*spsel_read*/
     virtual void writefn (uArchARM * aCore, uint64_t aVal) override {
        unsigned int cur_el = aCore->_PSTATE().EL();
        /* Update PSTATE SPSel bit; this requires us to update the
         * working stack pointer in xregs[31].
         */
        if (!((aVal ^ aCore->_PSTATE().d()) & PSTATE_SP)) {
            return;
        }

        if (aCore->_PSTATE().d() & PSTATE_SP) {
            aCore->setSP_el(cur_el, aCore->getXRegister(31));
        } else {
            aCore->setSP_el(0, aCore->getXRegister(31));
        }

        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 0, 1, aVal));

        /* We rely on illegal updates to SPsel from EL0 to get trapped
         * at translation time.
         */
        assert(cur_el >= 1 && cur_el <= 3);

        if (aCore->_PSTATE().d() & PSTATE_SP) {
            aCore->setXRegister(31, aCore->getSP_el(cur_el));
        } else {
            aCore->setXRegister(31, aCore->getSP_el(0));
        }

    }
}SPSel_;

struct SPSR_IRQ : public SysRegInfo {
    std::string name = "SPSR_IRQ";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 4;
    uint8_t opc2 = 0;
    uint8_t crn = 4;
    uint8_t crm = 3;
    eAccessRight access = kPL2_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;

}SPSR_IRQ_;

struct SPSR_ABT : public SysRegInfo {
    std::string name = "SPSR_IRQ";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 4;
    uint8_t opc2 = 1;
    uint8_t crn = 4;
    uint8_t crm = 3;
    eAccessRight access = kPL2_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;

}SPSR_ABT_;

struct SPSR_UND : public SysRegInfo {
    std::string name = "SPSR_UND";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 4;
    uint8_t opc2 = 2;
    uint8_t crn = 4;
    uint8_t crm = 3;
    eAccessRight access = kPL2_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;

}SPSR_UND_;

struct SPSR_FIQ : public SysRegInfo {
    std::string name = "SPSR_FIQ";
    eRegExecutionState state = kARM_STATE_AA64;
    uint8_t opc0 = 3;
    uint8_t opc1 = 4;
    uint8_t opc2 = 3;
    uint8_t crn = 4;
    uint8_t crm = 3;
    eAccessRight access = kPL2_RW;
    eRegInfo type = kARM_ALIAS;
    uint64_t resetvalue = -1;

}SPSR_FIQ_;


struct INVALID_PRIV : public SysRegInfo {
    std::string name = "INVALID_PRIV";

}INVALID_PRIV_;

static std::vector<std::pair<std::array<uint8_t, 5>, ePrivRegs>>  supported_sysRegs = {
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {NZCV_.opc0, NZCV_.opc1, NZCV_.opc2, NZCV_.crn, NZCV_.crm},  kNZCV) ,
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {DAIF_.opc0, DAIF_.opc1, DAIF_.opc2, DAIF_.crn, DAIF_.crm},  kDAIF) ,
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {FPCR_.opc0, FPCR_.opc1, FPCR_.opc2, FPCR_.crn, FPCR_.crm},  kFPCR) ,
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {FPSR_.opc0, FPSR_.opc1, FPSR_.opc2, FPSR_.crn, FPSR_.crm},  kFPSR) ,
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {DCZID_EL0_.opc0, DCZID_EL0_.opc1, DCZID_EL0_.opc2, DCZID_EL0_.crn, DCZID_EL0_.crm},  kDCZID_EL0) ,
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {DC_ZVA_.opc0, DC_ZVA_.opc1, DC_ZVA_.opc2, DC_ZVA_.crn, DC_ZVA_.crm},  kDC_ZVA),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {CURRENT_EL_.opc0, CURRENT_EL_.opc1, CURRENT_EL_.opc2, CURRENT_EL_.crn, CURRENT_EL_.crm},  kCURRENT_EL),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {ELR_EL1_.opc1, ELR_EL1_.opc1, ELR_EL1_.opc2, ELR_EL1_.crn, ELR_EL1_.crm},  kELR_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_EL1_.opc0, SPSR_EL1_.opc1, SPSR_EL1_.opc2, SPSR_EL1_.crn, SPSR_EL1_.crm},  kSPSR_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SP_EL0_.opc0, SP_EL0_.opc1, SP_EL0_.opc2, SP_EL0_.crn, SP_EL0_.crm},  kSP_EL0),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SP_EL1_.opc0, SP_EL1_.opc1, SP_EL1_.opc2, SP_EL1_.crn, SP_EL1_.crm},  kSP_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSel_.opc0, SPSel_.opc1, SPSel_.opc2, SPSel_.crn, SPSel_.crm},  kSPSel),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_IRQ_.opc0, SPSR_IRQ_.opc1, SPSR_IRQ_.opc2, SPSR_IRQ_.crn, SPSR_IRQ_.crm},  kSPSR_IRQ),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_ABT_.opc0, SPSR_ABT_.opc1, SPSR_ABT_.opc2, SPSR_ABT_.crn, SPSR_ABT_.crm},  kSPSR_ABT),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_UND_.opc0, SPSR_UND_.opc1, SPSR_UND_.opc2, SPSR_UND_.crn, SPSR_UND_.crm},  kSPSR_UND),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_FIQ_.opc0, SPSR_FIQ_.opc1, SPSR_FIQ_.opc2, SPSR_FIQ_.crn, SPSR_FIQ_.crm},  kSPSR_FIQ)
};


ePrivRegs getPrivRegType(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm){

    std::array<uint8_t, 5> op = {op0,op1,op2,crn,crm};
    for (std::pair<std::array<uint8_t, 5>, ePrivRegs>& e : supported_sysRegs){
        if (e.first == op)
            return e.second;
    }
    return kLastPrivReg;
}

SysRegInfo& getPriv(ePrivRegs aCode){
    switch (aCode) {
    case kNZCV:
        return NZCV_;
    case kDAIF:
        return DAIF_;
    case kFPCR:
        return FPCR_;
    case kFPSR:
        return FPSR_;
    case kDCZID_EL0:
        return DCZID_EL0_;
    case kDC_ZVA:
        return DC_ZVA_;
    case kCURRENT_EL:
        return CURRENT_EL_;
    case kELR_EL1:
        return ELR_EL1_;
    case kSPSR_EL1:
        return SPSR_EL1_;
    case kSP_EL0:
        return SP_EL0_;
    case kSP_EL1:
        return SP_EL1_;
    case kSPSel:
        return SPSel_;
    case kSPSR_IRQ:
        return SPSR_IRQ_;
    case kSPSR_ABT:
        return SPSR_ABT_;
    case kSPSR_UND:
        return SPSR_UND_;
    case kSPSR_FIQ:
        return SPSR_FIQ_;
    default:
        DBG_Assert( false, ( << "Unimplemented SysReg Code" << aCode ) );
        return INVALID_PRIV_;
    }
}

SysRegInfo& getPriv(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm){
    ePrivRegs r = getPrivRegType(op0, op1, op2, crn, crm);
    return getPriv(r);
}

}//nuArchARM


