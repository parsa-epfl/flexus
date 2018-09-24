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


#ifndef FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED
#define FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED

#include <functional>
#include <memory>
#include <string>
#include "uArchInterfaces.hpp"
#include <components/armDecoder/OperandMap.hpp>


namespace nuArchARM {

struct uArchARM;


/* Valid values for ARMCPRegInfo state field, indicating which of
 * the AArch32 and AArch64 execution states this register is visible in.
 * If the reginfo doesn't explicitly specify then it is AArch32 only.
 * If the reginfo is declared to be visible in both states then a second
 * reginfo is synthesised for the AArch32 view of the AArch64 register,
 * such that the AArch32 view is the lower 32 bits of the AArch64 one.
 * Note that we rely on the values of these enums as we iterate through
 * the various states in some places.
 */
enum eExecutionState{
    kARM_STATE_AA32 = 0,
    kARM_STATE_AA64 = 1,
    kARM_STATE_BOTH = 2,
};


struct SysRegInfo {

      ~SysRegInfo(){}

      /* Definition of an ARM system register */
      /* Name of register (useful mainly for debugging, need not be unique) */
      std::string name;
      /* Execution state in which this register is visible: ARCH_STATE_* */
      int state;
      /* Location of register: coprocessor number and (crn,crm,opc1,opc2)
       * tuple. Any of crm, opc1 and opc2 may be CP_ANY to indicate a
       * 'wildcard' field -- any value of that field in the MRC/MCR insn
       * will be decoded to this register. The register read and write
       * callbacks will be passed an ARMCPRegInfo with the crn/crm/opc1/opc2
       * used by the program, so it is possible to register a wildcard and
       * then behave differently on read/write if necessary.
       * For 64 bit registers, only crm and opc1 are relevant; crn and opc2
       * must both be zero.
       * For AArch64-visible registers, opc0 is also used.
       * Since there are no "coprocessors" in AArch64, cp is purely used as a
       * way to distinguish (for KVM's benefit) guest-visible system registers
       * from demuxed ones provided to preserve the "no side effects on
       * KVM register read/write from QEMU" semantics. cp==0x13 is guest
       * visible (to match KVM's encoding); cp==0 will be converted to
       * cp==0x13 when the ARMCPRegInfo is registered, for convenience.
       */
      uint8_t cp;
      uint8_t opc0;
      uint8_t opc1;
      uint8_t opc2;
      uint8_t crn;
      uint8_t crm;

      /* Access rights: PL*_[RW] */
      int access;
      /* Register type: ARM_CP_* bits/values */
      int type;

      /* Security state: ARM_CP_SECSTATE_* bits/values */
      int secure;

      /* Value of this register, if it is ARM_CP_CONST. Otherwise, if
       * fieldoffset is non-zero, the reset value of the register.
       */
      uint64_t resetvalue;



      /* Function for making any access checks for this register in addition to
       * those specified by the 'access' permissions bits. If NULL, no extra
       * checks required. The access check is performed at runtime, not at
       * translate time.
       */
      virtual eAccessResult accessfn (uArchARM* aCore){}

      /* Function for handling reads of this register. If NULL, then reads
       * will be done by loading from the offset into CPUARMState specified
       * by fieldoffset.
       */
      virtual narmDecoder::Operand readfn (uArchARM* aCore){}

      /* Function for handling writes of this register. If NULL, then writes
       * will be done by writing to the offset into CPUARMState specified
       * by fieldoffset.
       */
      virtual void writefn (uArchARM* aCore, narmDecoder::Operand aVal){}
      /* Function for doing a "raw" read; used when we need to copy
       * coprocessor state to the kernel for KVM or out for
       * migration. This only needs to be provided if there is also a
       * readfn and it has side effects (for instance clear-on-read bits).
       */

      virtual narmDecoder::Operand raw_readfn (uArchARM* aCore){}
      /* Function for doing a "raw" write; used when we need to copy KVM
       * kernel coprocessor state into userspace, or for inbound
       * migration. This only needs to be provided if there is also a
       * writefn and it masks out "unwritable" bits or has write-one-to-clear
       * or similar behaviour.
       */

      virtual void raw_writefn (uArchARM* aCore){}
      /* Function for resetting the register. If NULL, then reset will be done
       * by writing resetvalue to the field specified in fieldoffset. If
       * fieldoffset is 0 then no reset will be done.
       */

      virtual void reset (uArchARM* aCore){}
};


SysRegInfo* getRegInfo(std::array<uint8_t, 5> aCode);
SysRegInfo* getRegInfo_cp(std::array<uint8_t, 5> aCode);

std::multimap<int, SysRegInfo*> * initSysRegs();

} //nuArchARM

#endif //FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED
