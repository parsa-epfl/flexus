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

#ifndef FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED
#define FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <components/Decoder/OperandMap.hpp>
#include <functional>
#include <memory>
#include <string>

namespace nuArch {

struct uArch;

/* Valid values for ARMCPRegInfo state field, indicating which of
 * the AArch32 and AArch64 execution states this register is visible in.
 * If the reginfo doesn't explicitly specify then it is AArch32 only.
 * If the reginfo is declared to be visible in both states then a second
 * reginfo is synthesised for the AArch32 view of the AArch64 register,
 * such that the AArch32 view is the lower 32 bits of the AArch64 one.
 * Note that we rely on the values of these enums as we iterate through
 * the various states in some places.
 */
enum eRegExecutionState
{
    kARM_STATE_AA32 = 0,
    kARM_STATE_AA64 = 1,
    kARM_STATE_BOTH = 2,
};

class SysRegInfo
{
  public:
    /* Definition of an ARM system register */
    /* Name of register (useful mainly for debugging, need not be unique) */
    std::string name;
    /* Execution state in which this register is visible: ARCH_STATE_* */
    int state;
    /* Register type: ARM_CP_* bits/values */
    int type;

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
    uint8_t opc0; // or cp
    uint8_t opc1;
    uint8_t opc2;
    uint8_t crn;
    uint8_t crm;

    /* Access rights: PL*_[RW] */
    int access;

    /* Security state: ARM_CP_SECSTATE_* bits/values */
    //      int secure;

    /* Value of this register, if it is ARM_CP_CONST. Otherwise, if
     * fieldoffset is non-zero, the reset value of the register.
     */
    uint64_t resetvalue;

    /* Function for making any access checks for this register in addition to
     * those specified by the 'access' permissions bits. If NULL, no extra
     * checks required. The access check is performed at runtime, not at
     * translate time.
     */
    virtual eAccessResult accessfn(uArch* aCore)
    {
        return kACCESS_OK; // Msutherl: Right now it's all OK. QEMU/TCG will check access and trap if
                           // necessary
    }

    /* Function for handling reads of this register. If NULL, then reads
     * will be done by loading from the offset into CPUARMState specified
     * by fieldoffset.
     */
    virtual uint64_t readfn(uArch* aCore) { return aCore->readUnhashedSysReg(opc0, opc1, opc2, crn, crm); }

    /* Function for handling writes of this register. If NULL, then writes
     * will be done by writing to the offset into CPUARMState specified
     * by fieldoffset.
     */
    virtual void writefn(uArch* aCore, uint64_t aVal) { DBG_Assert(false); }
    /* Function for doing a "raw" read; used when we need to copy
     * coprocessor state to the kernel for KVM or out for
     * migration. This only needs to be provided if there is also a
     * readfn and it has side effects (for instance clear-on-read bits).
     */

    virtual uint64_t raw_readfn(uArch* aCore)
    {
        DBG_Assert(false);
        return 0;
    }
    /* Function for doing a "raw" write; used when we need to copy KVM
     * kernel coprocessor state into userspace, or for inbound
     * migration. This only needs to be provided if there is also a
     * writefn and it masks out "unwritable" bits or has write-one-to-clear
     * or similar behaviour.
     */

    virtual void raw_writefn(uArch* aCore) { DBG_Assert(false); }
    /* Function for resetting the register. If NULL, then reset will be done
     * by writing resetvalue to the field specified in fieldoffset. If
     * fieldoffset is 0 then no reset will be done.
     */

    virtual void reset(uArch* aCore) {}

    virtual void setSystemRegisterEncodingValues(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t aCrn, uint8_t aCrm)
    {
        this->opc0 = op0;
        this->opc1 = op1;
        this->opc2 = op2;
        this->crn  = aCrn;
        this->crm  = aCrm;
    }

    /* Default constructor sets undefined code */
    SysRegInfo()
      : name("SysRegInfo")
      , state(kARM_STATE_BOTH)
      , type(kARM_CONST)
      , opc0(0x0)
      , opc1(0x0)
      , opc2(0x0)
      , crn(0x0)
      , crm(0x0)
      , access(kPL1_RW)
    {
    }

    /* Specific constructor for defining a code, state, and access */
    SysRegInfo(std::string aName,
               int aState,
               int aType,
               uint8_t anOpc0,
               uint8_t anOpc1,
               uint8_t anOpc2,
               uint8_t aCrn,
               uint8_t aCrm,
               int anAccess)
      : name(aName)
      , state(aState)
      , type(aType)
      , opc0(anOpc0)
      , opc1(anOpc1)
      , opc2(anOpc2)
      , crn(aCrn)
      , crm(aCrm)
      , access(anAccess)
    {
    }

    /* Specific constructor for just a code */
    SysRegInfo(uint8_t anOpc0, uint8_t anOpc1, uint8_t anOpc2, uint8_t aCrn, uint8_t aCrm)
      : name("SysRegInfo")
      , state(kARM_STATE_BOTH)
      , type(kARM_CONST)
      , opc0(anOpc0)
      , opc1(anOpc1)
      , opc2(anOpc2)
      , crn(aCrn)
      , crm(aCrm)
      , access(kPL1_RW)
    {
    }

    virtual ~SysRegInfo() {}
};

ePrivRegs
getPrivRegType(const uint8_t op0, const uint8_t op1, const uint8_t op2, const uint8_t crn, const uint8_t crm);
std::unique_ptr<SysRegInfo>
getPriv(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm);
std::unique_ptr<SysRegInfo>
getPriv(ePrivRegs aCode);

} // namespace nuArch

#endif // FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED