
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

    virtual void sync(uArch* aCore, size_t theNode)
    {
        this->writefn(aCore,
                      Flexus::Qemu::API::qemu_api
                        .read_sys_register(theNode, this->opc0, this->opc1, this->opc2, this->crn, this->crm, true));
    }

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

extern std::vector<std::pair<std::array<uint8_t, 5>, ePrivRegs>> supported_sysRegs;

ePrivRegs
getPrivRegType(const uint8_t op0, const uint8_t op1, const uint8_t op2, const uint8_t crn, const uint8_t crm);
std::unique_ptr<SysRegInfo>
getPriv(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm);
std::unique_ptr<SysRegInfo>
getPriv(ePrivRegs aCode);

} // namespace nuArch

#endif // FLEXUS_uARCH_SYSTEM_REGISTER_HPP_INCLUDED