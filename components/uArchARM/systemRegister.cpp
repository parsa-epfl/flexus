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
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    NZCV ()
        :   name ( "NZCV" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  3 )
        ,   opc2 (  0 )
        ,   crn (  4 )
        ,   crm (  2 )
        ,   access ( kPL0_RW )
        ,   type ( kARM_NZCV )
    {}
};


struct DAIF : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DAIF ()
        :   name ( "DAIF" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  3 )
        ,   opc2 (  1 )
        ,   crn (  4 )
        ,   crm (  2 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL0_RW )
    {}

    virtual eAccessResult accessfn (uArchARM* aCore) {} /*aa64_daif_access*/
    virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {}
    virtual void reset (uArchARM * aCore) override {} /*arm_cp_reset_ignore*/
};


struct FPCR : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    FPCR()
        :   name ( "FPCR" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  3 )
        ,   opc2 (  0 )
        ,   crn (  4 )
        ,   crm (  4 )
        ,   access ( kPL0_RW )
    {}
     virtual narmDecoder::Operand readfn (uArchARM * aCore) override {} /*aa64_fpcr_read*/
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*aa64_fpcr_write*/
};

struct FPSR : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    FPSR ()
        :   name ( "FPSR" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  3 )
        ,   opc2 (  1 )
        ,   crn (  4 )
        ,   crm (  4 )
        ,   access ( kPL0_RW )
    {}
     virtual narmDecoder::Operand readfn (uArchARM * aCore) override {} /*aa64_fpsr_read*/
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*aa64_fpsr_write*/
};

struct DCZID_EL0 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCZID_EL0()
        :   name ( "DCZID_EL0" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  3 )
        ,   opc2 (  7 )
        ,   crn (  0 )
        ,   crm (  0 )
        ,   access ( kPL0_R )
        ,   type ( kARM_NO_RAW )

    {}
     virtual narmDecoder::Operand readfn(uArchARM * aCore) override {
         return aCore->readDCZID_EL0();
     }
};

struct DC_ZVA : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;
     DC_ZVA()
        :   name("DC_ZVA")
        ,   state(kARM_STATE_AA64)
        ,   opc0 ( 1 )
        ,   opc1 ( 3 )
        ,   crn  ( 7 )
        ,   crm  ( 4 )
        ,   opc2 ( 1 )
        ,   access ( kPL0_W )
        ,   type ( kARM_DC_ZVA )
     {}


     virtual eAccessResult accessfn(uArchARM * aCore) override {
         return aCore->accessZVA();
     }
};

struct CURRENTEL : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    CURRENTEL()
        :   name ( "CURRENTEL" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   opc2 (  2 )
        ,   crn (  4 )
        ,   crm (  2 )
        ,   access ( kPL1_R )
        ,   type ( kARM_CURRENTEL )
    {}
};

    /* Cache ops: all NOPs since we don't emulate caches */
struct IC_IALLUIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    IC_IALLUIS()
        :   name ( "IC_IALLUIS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  1 )
        ,   opc2 (  0 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

struct IC_IALLU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    IC_IALLU()
        :   name ( "IC_IALLU" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  0 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

struct IC_IVAU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    IC_IVAU()
        :   name ( "IC_IVAU" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  3 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  1 )
        ,   access ( kPL0_W )
        ,   type ( kARM_NOP )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*aa64_cacheop_access*/
};

struct DC_IVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_IVAC()
        :   name ( "DC_IVAC" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  6 )
        ,   opc2 (  1 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

struct DC_ISW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_ISW ()
        :   name ( "DC_ISW" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  6 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

struct DC_CVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_CVAC()
        :   name ( "DC_CVAC" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  3 )
        ,   crn (  7 )
        ,   crm (  10 )
        ,   opc2 (  1 )
        ,   access ( kPL0_W )
        ,   type ( kARM_NOP )
    {}

     virtual eAccessResult accessfn (uArchARM* aCore) {} /*aa64_cacheop_access*/
};

struct DC_CSW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_CSW()
        :   name ( "DC_CSW" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  10 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

struct DC_CVAU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_CVAU()
        :   name ( "DC_CVAU" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  3 )
        ,   crn (  7 )
        ,   crm (  11 )
        ,   opc2 (  1 )
        ,   access ( kPL0_W )
        ,   type ( kARM_NOP )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*aa64_cacheop_access*/
};

struct DC_CIVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_CIVAC()
        :   name ( "DC_CIVAC" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  3 )
        ,   crn (  7 )
        ,   crm (  14 )
        ,   opc2 (  1 )
        ,   access ( kPL0_W )
        ,   type ( kARM_NOP )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*aa64_cacheop_access*/
};

struct DC_CISW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DC_CISW()
        :   name ( "DC_CISW" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  14 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NOP  )
    {}
};

    /* TLBI operations */
struct TLBI_VMALLE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VMALLE1IS()
        :   name ( "TLBI_VMALLE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  0 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vmalle1is_write*/
};

struct TLBI_VAE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAE1IS()
        :   name ( "TLBI_VAE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  1 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1is_write*/
};

struct TLBI_ASIDE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_ASIDE1IS()
        :   name ( "TLBI_ASIDE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vmalle1is_write*/
};

struct TLBI_VAAE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAAE1IS()
        :   name ( "TLBI_VAAE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  3 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1is_write*/
};

struct TLBI_VALE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VALE1IS()
        :   name ( "TLBI_VALE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  5 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1is_write*/
};

struct TLBI_VAALE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAALE1IS()
        :   name ( "TLBI_VAALE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  7 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1is_write*/
};

struct TLBI_VMALLE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VMALLE1()
        :   name ( "TLBI_VMALLE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  0 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vmalle1_write*/
};

struct TLBI_VAE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAE1()
        :   name ( "TLBI_VAE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  1 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1_write*/
};

struct TLBI_ASIDE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_ASIDE1()
        :   name ( "TLBI_ASIDE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vmalle1_write*/
};

struct TLBI_VAAE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAAE1()
        :   name ( "TLBI_VAAE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  3 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1_write*/
};

struct TLBI_VALE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VALE1()
        :   name ( "TLBI_VALE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  5 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1_write*/
};

struct TLBI_VAALE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VAALE1()
        :   name ( "TLBI_VAALE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  7 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_vae1_write*/
};

struct TLBI_IPAS2E1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_IPAS2E1IS()
        :   name ( "TLBI_IPAS2E1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  0 )
        ,   opc2 (  1 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_ipas2e1is_write*/
};

struct TLBI_IPAS2LE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_IPAS2LE1IS()
        :   name ( "TLBI_IPAS2LE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  0 )
        ,   opc2 (  5 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_ipas2e1is_write*/
};

struct TLBI_ALLE1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_ALLE1IS()
        :   name ( "TLBI_ALLE1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  4 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_alle1is_write*/
};

struct TLBI_VMALLS12E1IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VMALLS12E1IS()
        :   name ( "TLBI_VMALLS12E1IS" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  6 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_alle1is_write*/
};

struct TLBI_IPAS2E1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_IPAS2E1()
        :   name ( "TLBI_IPAS2E1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  4 )
        ,   opc2 (  1 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_ipas2e1_write*/
};

struct TLBI_IPAS2LE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_IPAS2LE1()
        :   name ( "TLBI_IPAS2LE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  4 )
        ,   opc2 (  5 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_ipas2e1_write*/
};

struct TLBI_ALLE1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_ALLE1()
        :   name ( "TLBI_ALLE1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  4 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_alle1_write*/
};

struct TLBI_VMALLS12E1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBI_VMALLS12E1()
        :   name ( "TLBI_VMALLS12E1" )
            ,   state ( kARM_STATE_AA64 )
            ,   opc0 (  1 )
            ,   opc1 (  4 )
            ,   crn (  8 )
            ,   crm (  7 )
            ,   opc2 (  6 )
            ,   access ( kPL2_W )
            ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbi_aa64_alle1is_write*/
};

    /* 64 bit address translation operations */
struct AT_S1E1R : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E1R()
        :   name ( "AT_S1E1R" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  0 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};


struct AT_S1E1W : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E1W()
        :   name ( "AT_S1E1W" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  1 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};


struct AT_S1E0R : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E0R()
        :   name ( "AT_S1E0R" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  2 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};


struct AT_S1E0W : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E0W()
        :   name ( "AT_S1E0W" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  3 )
        ,   access ( kPL1_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};


struct AT_S12E1R : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S12E1R()
        :   name ( "AT_S12E1R" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  4 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

struct AT_S12E1W : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S12E1W()
        :   name ( "AT_S12E1W" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  5 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

struct AT_S12E0R : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S12E0R()
        :   name ( "AT_S12E0R" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  6 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

struct AT_S12E0W : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S12E0W()
        :   name ( "AT_S12E0W" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  4 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  7 )
        ,   access ( kPL2_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

    /* AT S1E2* are elsewhere as they UNDEF from EL3 if EL2 is not present */
struct AT_S1E3R : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E3R()
        :   name ( "AT_S1E3R" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  6 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  0 )
        ,   access ( kPL3_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

struct AT_S1E3W : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    AT_S1E3W()
        :   name ( "AT_S1E3W" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  1 )
        ,   opc1 (  6 )
        ,   crn (  7 )
        ,   crm (  8 )
        ,   opc2 (  1 )
        ,   access ( kPL3_W )
        ,   type ( kARM_NO_RAW )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*ats_write64*/
};

struct PAR_EL1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    PAR_EL1()
        :   name ( "PAR_EL1" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  4 )
        ,   opc2 (  0 )
        ,   access ( kPL1_RW )
        ,   resetvalue ( 0 )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*par_write*/
};

    /* TLB invalidate last level of translation table walk */
struct TLBIMVALIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVALIS()
        :   name ( "TLBIMVALIS" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL1_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimva_is_write*/
};

struct TLBIMVAALIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVAALIS()
        :   name ( "TLBIMVAALIS" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  7 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL1_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimvaa_is_write*/
};

struct TLBIMVAL : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVAL()
        :   name ( "TLBIMVAL" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL1_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimva_write*/
};

struct TLBIMVAAL : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVAAL()
        :   name ( "TLBIMVAAL" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  7 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL1_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimvaa_write*/
};

struct TLBIMVALH : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVALH()
        :   name ( "TLBIMVALH" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  7 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimva_hyp_write*/
};

struct TLBIMVALHIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIMVALHIS()
        :   name ( "TLBIMVALHIS" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  3 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbimva_hyp_is_write*/
};

struct TLBIIPAS2 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIIPAS2()
        :   name ( "TLBIIPAS2" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  4 )
        ,   opc2 (  1 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}


     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbiipas2_write*/
};

struct TLBIIPAS2IS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIIPAS2IS()
        :   name ( "TLBIIPAS2IS" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  0 )
        ,   opc2 (  1 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbiipas2_is_write*/
};

struct TLBIIPAS2L : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIIPAS2L()
        :   name ( "TLBIIPAS2L" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  4 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbiipas2_write*/
};

struct TLBIIPAS2LIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    TLBIIPAS2LIS()
        :   name ( "TLBIIPAS2LIS" )
        ,   cp ( 15 )
        ,   opc1 (  4 )
        ,   crn (  8 )
        ,   crm (  0 )
        ,   opc2 (  5 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL2_W )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*tlbiipas2_is_write*/
};

    /* 32 bit cache operations */
struct ICIALLUIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    ICIALLUIS()
        :   name ( "ICIALLUIS" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  1 )
        ,   opc2 (  0 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct BPIALLUIS : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    BPIALLUIS()
        :   name ( "BPIALLUIS" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  1 )
        ,   opc2 (  6 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct ICIALLU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    ICIALLU()
        :   name ( "ICIALLU" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  0 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct ICIMVAU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    ICIMVAU()
        :   name ( "ICIMVAU" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  1 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct BPIALL : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    BPIALL()
        :   name ( "BPIALL" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  6 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct BPIMVA : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    BPIMVA()
        :   name ( "BPIMVA" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  5 )
        ,   opc2 (  7 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCIMVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCIMVAC()
        :   name ( "DCIMVAC" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  6 )
        ,   opc2 (  1 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCISW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCISW()
        :   name ( "DCISW" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  6 )
        ,   opc2 (  2 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCCMVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCCMVAC()
        :   name ( "DCCMVAC" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  10 )
        ,   opc2 (  1 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCCSW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCCSW()
        :   name ( "DCCSW" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  10 )
        ,   opc2 (  2 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCCMVAU : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCCMVAU()
        :   name ( "DCCMVAU" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  11 )
        ,   opc2 (  1 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCCIMVAC : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCCIMVAC()
        :   name ( "DCCIMVAC" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  14 )
        ,   opc2 (  1 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};

struct DCCISW : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DCCISW()
        :   name ( "DCCISW" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  7 )
        ,   crm (  14 )
        ,   opc2 (  2 )
        ,   type ( kARM_NOP )
        ,   access ( kPL1_W )
    {}
};


    /* MMU Domain access control / MPU write buffer control */
struct DACR : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DACR()
        :   name ( "DACR" )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  3 )
        ,   crm (  0 )
        ,   opc2 (  0 )
        ,   access ( kPL1_RW )
        ,   resetvalue ( 0 )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*dacr_write*/
     virtual void raw_writefn (uArchARM * aCore) override {}
};

struct ELR_EL1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    ELR_EL1()
        :   name ( "ELR_EL1" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   crn (  4 )
        ,   crm (  0 )
        ,   opc2 (  1 )
        ,   access ( kPL1_RW )
    {}
};

struct SPSR_EL1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSR_EL1()
        :   name ( "SPSR_EL1" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   crn (  4 )
        ,   crm (  0 )
        ,   opc2 (  0 )
        ,   access ( kPL1_RW )
    {}
};


/* We rely on the access checks not allowing the guest to write to the
* state field when SPSel indicates that it's being used as the stack
* pointer.
*/
struct SP_EL0 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SP_EL0()
        :   name ( "SP_EL0" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   crn (  4 )
        ,   crm (  1 )
        ,   opc2 (  0 )
        ,   access ( kPL1_RW )
        ,   type ( kARM_ALIAS )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*sp_el0_access*/


};

struct SP_EL1 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SP_EL1()
        :   name ( "SP_EL1" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  4 )
        ,   crm (  1 )
        ,   opc2 (  0 )
        ,   access ( kPL2_RW )
        ,   type ( kARM_ALIAS )
    {}
};

struct SPSel : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSel()
        :   name ( "SPSel" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  0 )
        ,   crn (  4 )
        ,   crm (  2 )
        ,   opc2 (  0 )
        ,   type ( kARM_NO_RAW )
        ,   access ( kPL1_RW )
    {}
     virtual narmDecoder::Operand readfn (uArchARM * aCore) override {} /*spsel_read*/
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*spsel_write*/
};

struct FPEXC32_EL2 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    FPEXC32_EL2()
        :   name ( "FPEXC32_EL2" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  5 )
        ,   crm (  3 )
        ,   opc2 (  0 )
        ,   type ( kARM_ALIAS )
        ,   access ( kPL2_RW )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*fpexc32_access*/
};

struct DACR32_EL2 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    DACR32_EL2()
        :   name ( "DACR32_EL2" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  3 )
        ,   crm (  0 )
        ,   opc2 (  0 )
        ,   access ( kPL2_RW )
        ,   resetvalue ( 0 )
    {}
     virtual void writefn (uArchARM * aCore, narmDecoder::Operand aVal) override {} /*dacr_write*/
    virtual void raw_writefn (uArchARM * aCore) override {} /*dacr_write*/
};

struct IFSR32_EL2 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    IFSR32_EL2()
        :   name ( "IFSR32_EL2" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  5 )
        ,   crm (  0 )
        ,   opc2 (  1 )
        ,   access ( kPL2_RW )
        ,   resetvalue ( 0 )
    {}
};

struct SPSR_IRQ : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSR_IRQ()
        :   name ( "SPSR_IRQ" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  4 )
        ,   crm (  3 )
        ,   opc2 (  0 )
        ,   access ( kPL2_RW )
    {}
};

struct SPSR_ABT : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSR_ABT()
        :   name ( "SPSR_ABT" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  4 )
        ,   crm (  3 )
        ,   opc2 (  1 )
        ,   access ( kPL2_RW )
    {}
};

struct SPSR_UND : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSR_UND()
        :   name ( "SPSR_UND" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  4 )
        ,   crm (  3 )
        ,   opc2 (  2 )
        ,   access ( kPL2_RW )
    {}
};

struct SPSR_FIQ : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SPSR_FIQ()
        :   name ( "SPSR_FIQ" )
        ,   state ( kARM_STATE_AA64 )
        ,   type ( kARM_ALIAS )
        ,   opc0 (  3 )
        ,   opc1 (  4 )
        ,   crn (  4 )
        ,   crm (  3 )
        ,   opc2 (  3 )
        ,   access ( kPL2_RW )
    {}
};

struct MDCR_EL3 : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    MDCR_EL3()
        :   name ( "MDCR_EL3" )
        ,   state ( kARM_STATE_AA64 )
        ,   opc0 (  3 )
        ,   opc1 (  6 )
        ,   crn (  1 )
        ,   crm (  3 )
        ,   opc2 (  1 )
        ,   resetvalue ( 0 )
        ,   access ( kPL3_RW )
    {}
};

struct SDCR : public SysRegInfo {
    std::string name;
    int state;
    uint8_t cp;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;
    int access;
    int type;
    int secure;
    uint64_t resetvalue;

    SDCR()
        :   name ( "SDCR" )
        ,   type ( kARM_ALIAS )
        ,   cp ( 15 )
        ,   opc1 (  0 )
        ,   crn (  1 )
        ,   crm (  3 )
        ,   opc2 (  1 )
        ,   access ( kPL1_RW )
    {}
     virtual eAccessResult accessfn (uArchARM* aCore) {} /*access_trap_aa32s_el1*/
};



std::multimap<int, SysRegInfo*> * initSysRegs(){

    std::multimap<int, SysRegInfo*> * 			tmp = new std::multimap<int, SysRegInfo*>();
    SysRegInfo* r;
    int sum;

    r = new NZCV();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DAIF();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new FPCR();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new FPSR();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCZID_EL0();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_ZVA();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new CURRENTEL();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new IC_IALLUIS();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new IC_IALLU();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new IC_IVAU();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_IVAC();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_ISW();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_CVAC();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_CSW();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_CVAU();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_CIVAC();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DC_CISW();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VMALLE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAE1IS();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_ASIDE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAAE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VALE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAALE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VMALLE1(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAE1();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_ASIDE1();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAAE1();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VALE1();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VAALE1();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_IPAS2E1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_IPAS2LE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_ALLE1IS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VMALLS12E1IS(); if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_IPAS2E1(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_IPAS2LE1(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_ALLE1();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBI_VMALLS12E1(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E1R();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E1W();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E0R();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E0W();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S12E1R();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S12E1W();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S12E0R();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S12E0W();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E3R();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new AT_S1E3W();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new PAR_EL1();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVALIS();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVAALIS();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVAL();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVAAL();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVALH();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIMVALHIS();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIIPAS2();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIIPAS2IS();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIIPAS2L();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new TLBIIPAS2LIS(); 	 if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new ICIALLUIS();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new BPIALLUIS();         if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new ICIALLU();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new ICIMVAU();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new BPIALL();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new BPIMVA();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCIMVAC();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCISW();             if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCCMVAC();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCCSW();             if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCCMVAU();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCCIMVAC();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DCCISW();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DACR();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new ELR_EL1();           if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSR_EL1();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SP_EL0();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SP_EL1();            if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSel();             if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new FPEXC32_EL2();       if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new DACR32_EL2();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new IFSR32_EL2();        if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSR_IRQ();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSR_ABT();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSR_UND();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SPSR_FIQ();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new MDCR_EL3();          if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));
    r = new SDCR();              if (r->cp == 15) {sum = r->cp+r->opc1+r->opc2+r->crn+r->crm;} else {sum = r->opc0+r->opc1+r->opc2+r->crn+r->crm;} tmp->insert(std::pair<int, SysRegInfo*>(sum,r));


    return tmp;
}


SysRegInfo* getRegInfo(std::array<uint8_t, 5> aCode, std::multimap<int, SysRegInfo*>& aSysRegMap){
    uint32_t sum = std::accumulate(aCode.begin(), aCode.end(), 0);
    if (aSysRegMap.count(sum) <= 0) return nullptr;
    if (aSysRegMap.count(sum) > 0) {
        std::multimap<int, SysRegInfo*>::iterator it;
        for (it=aSysRegMap.equal_range(sum).first; it!=aSysRegMap.equal_range(sum).second; ++it){
            if( (*it).second->opc0 == aCode[0] && (*it).second->opc1 == aCode[1] &&
                    (*it).second->opc2 == aCode[2] && (*it).second->crm == aCode[3] && (*it).second->crn == aCode[4]){
                return (*it).second;
            }
            DBG_Assert(false);
        }
    } else {
        std::multimap<int, SysRegInfo*>::iterator it = aSysRegMap.find(sum);
        return it->second;
    }

}

SysRegInfo* getRegInfo_cp(std::array<uint8_t, 5> aCode, std::multimap<int, SysRegInfo*>& aSysRegMap){
    uint32_t sum = std::accumulate(aCode.begin(), aCode.end(), 0);
    if (aSysRegMap.count(sum) <= 0) return nullptr;
    if (aSysRegMap.count(sum) > 0) {
        std::multimap<int, SysRegInfo*>::iterator it;
        for (it=aSysRegMap.equal_range(sum).first; it!=aSysRegMap.equal_range(sum).second; ++it){
            if( (*it).second->cp == aCode[0] && (*it).second->opc1 == aCode[1] &&
                    (*it).second->opc2 == aCode[2] && (*it).second->crm == aCode[3] && (*it).second->crn == aCode[4])
            {
                    return (*it).second;
            }
            DBG_Assert(false);
        }
    } else {
        std::multimap<int, SysRegInfo*>::iterator it = aSysRegMap.find(sum);
        return it->second;
    }

}




}//nuArchARM

