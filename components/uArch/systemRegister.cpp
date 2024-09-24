
#include "systemRegister.hpp"
#include "CoreModel.hpp"

#include <string>

namespace nuArch {

/* Minimal set of EL0-visible registers. This will need to be expanded
 * significantly for system emulation of AArch64 CPUs.
 */
class NZCV_ : public SysRegInfo
{
  public:
    std::string name                      = "NZCV";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 3;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 2;
    static const eAccessRight access      = kPL0_RW;
    static const eRegInfo type            = kARM_NZCV;
    static const uint64_t resetvalue      = -1;
    virtual void writefn(uArch* aCore, uint64_t aVal) override
    {
        std::bitset<8> a(aCore->_PSTATE().d());
        std::bitset<8> b(aVal);

        CORE_DBG("PSTATE VALUE BEFORE NZCV UPDATE: " << a << " -- NZCV: " << b);
        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 28, 4, aVal));
        CORE_DBG("PSTATE VALUE AFTER NZCV UPDATE: " << std::bitset<8>(aCore->_PSTATE().d()));

    } // FIXME
    virtual uint64_t readfn(uArch* aCore) override { return aCore->_PSTATE().NZCV(); }
    virtual void sync(uArch* aCore, size_t theNode) override
    {
        auto pstate = Flexus::Qemu::API::qemu_api.read_register(theNode, Flexus::Qemu::API::PSTATE, 0);
        writefn(aCore, extract32(pstate, 28, 4));

        CoreModel* bCore = dynamic_cast<CoreModel*>(aCore);
        if (bCore) {
            bCore->initializeRegister(bCore->map(ccRegArch(0)), register_value(static_cast<uint64_t>(extract32(pstate, 28, 4))));
        }
    }

    NZCV_()
      : SysRegInfo("NZCV_",
                   NZCV_::state,
                   NZCV_::type,
                   NZCV_::opc0,
                   NZCV_::opc1,
                   NZCV_::opc2,
                   NZCV_::crn,
                   NZCV_::crm,
                   NZCV_::access)
    {
    }
};

class DAIF_ : public SysRegInfo
{
  public:
    std::string name                      = "DAIF";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 3;
    static const uint8_t opc2             = 1;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 2;
    static const eAccessRight access      = kPL0_RW;
    static const eRegInfo type            = kARM_NO_RAW;
    uint64_t resetvalue                   = -1;

    virtual eAccessResult accessfn(uArch* aCore) override
    {
        return kACCESS_OK; // access OK since we assume the access right is EL0_RW
    }

    virtual void writefn(uArch* aCore, uint64_t aVal) override { aCore->setDAIF(aVal); } // FIXME
    virtual void reset(uArch* aCore) override { DBG_Assert(false); } // FIXME /*arm_cp_reset_ignore*/
    virtual uint64_t readfn(uArch* aCore) override { return aCore->_PSTATE().DAIF(); }
    virtual void sync(uArch* aCore, size_t theNode) override
    {
        auto pstate = Flexus::Qemu::API::qemu_api.read_register(theNode, Flexus::Qemu::API::PSTATE, 0);
        writefn(aCore, pstate);
    }
    DAIF_()
      : SysRegInfo("DAIF_",
                   DAIF_::state,
                   DAIF_::type,
                   DAIF_::opc0,
                   DAIF_::opc1,
                   DAIF_::opc2,
                   DAIF_::crn,
                   DAIF_::crm,
                   DAIF_::access)
    {
    }
};

class TPIDR_EL0_ : public SysRegInfo
{
  public:
    std::string name                      = "TPIDR_EL0";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 3;
    static const uint8_t opc2             = 2;
    static const uint8_t crn              = 13;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL0_RW;
    static const eRegInfo type            = kARM_NO_RAW;
    uint64_t resetvalue                   = -1;

    virtual eAccessResult accessfn(uArch* aCore) override
    {
        return kACCESS_OK; // access OK since we assume the access right is EL0_RW
    } // FIXME /*aa64_daif_access*/
    virtual void writefn(uArch* aCore, uint64_t aVal) override {  }
    virtual uint64_t readfn(uArch* aCore) override { return aCore->getTPIDR(0); }
    TPIDR_EL0_()
      : SysRegInfo("TPIDR_EL0_",
                   TPIDR_EL0_::state,
                   TPIDR_EL0_::type,
                   TPIDR_EL0_::opc0,
                   TPIDR_EL0_::opc1,
                   TPIDR_EL0_::opc2,
                   TPIDR_EL0_::crn,
                   TPIDR_EL0_::crm,
                   TPIDR_EL0_::access)
    {
    }
};

class TPIDR_EL2_ : public SysRegInfo
{
  public:
    std::string name                      = "TPIDR_EL2";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 2;
    static const uint8_t crn              = 13;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL0_RW;
    static const eRegInfo type            = kARM_NO_RAW;
    uint64_t resetvalue                   = -1;

    virtual eAccessResult accessfn(uArch* aCore) override
    {
        if (aCore->currentEL() <= 2) {
            return kACCESS_TRAP_EL2;
        }
        return kACCESS_OK; // access OK since we assume the access right is EL0_RW
    } // FIXME /*aa64_daif_access*/
    virtual void writefn(uArch* aCore, uint64_t aVal) override {  }
    virtual uint64_t readfn(uArch* aCore) override { return aCore->getTPIDR(2); }
    TPIDR_EL2_()
      : SysRegInfo("TPIDR_EL2_",
                   TPIDR_EL2_::state,
                   TPIDR_EL2_::type,
                   TPIDR_EL2_::opc0,
                   TPIDR_EL2_::opc1,
                   TPIDR_EL2_::opc2,
                   TPIDR_EL2_::crn,
                   TPIDR_EL2_::crm,
                   TPIDR_EL2_::access)
    {
    }
};

class FPCR_ : public SysRegInfo
{
  public:
    std::string name                      = "FPCR";
    static const eRegExecutionState state = kARM_STATE_AA64;
    // MARK: Edited to match ARM v8-A ISA Manual, page C5-374 (Section 5.2.7)
    static const uint8_t opc0        = 3;
    static const uint8_t opc1        = 3;
    static const uint8_t opc2        = 0;
    static const uint8_t crn         = 4;
    static const uint8_t crm         = 4;
    static const eAccessRight access = kPL0_RW;
    static const eRegInfo type       = kARM_SPECIAL;
    uint64_t resetvalue              = -1;

    /* Override for accessfn(). FPSR can be accessed as RW from EL0 and EL1 */
    virtual eAccessResult accessfn(uArch* aCore) override
    {
        return kACCESS_OK; // MARK: This is RW-allowed from every exception level
    }

    // TODO: This models a single, non-renameable FPSR/FPCR. Will resync
    // with QEMU often since many of the fields are written per-instruction.
    virtual uint64_t readfn(uArch* aCore) override { return aCore->getFPCR(); }          // /*aa64_fpcr_read*/
    virtual void writefn(uArch* aCore, uint64_t aVal) override { aCore->setFPCR(aVal); } // /*aa64_fpcr_write*/
    FPCR_()
      : SysRegInfo("FPCR_",
                   FPCR_::state,
                   FPCR_::type,
                   FPCR_::opc0,
                   FPCR_::opc1,
                   FPCR_::opc2,
                   FPCR_::crn,
                   FPCR_::crm,
                   FPCR_::access)
    {
    }
};

class FPSR_ : public SysRegInfo
{
  public:
    std::string name                      = "FPSR";
    static const eRegExecutionState state = kARM_STATE_AA64;
    // MARK: Edited to match ARM v8-A ISA Manual, page C5-378 (Section 5.2.8)
    static const uint8_t opc0        = 3;
    static const uint8_t opc1        = 3;
    static const uint8_t opc2        = 1;
    static const uint8_t crn         = 4;
    static const uint8_t crm         = 4;
    static const eAccessRight access = kPL0_RW;
    static const eRegInfo type       = kARM_SPECIAL;
    uint64_t resetvalue              = -1; // architecturally undefined as per manual

    /* Override for accessfn(). FPSR can be accessed as RW from EL0 and EL1 */
    virtual eAccessResult accessfn(uArch* aCore) override
    {
        return kACCESS_OK; // MARK: This is RW-allowed from every exception level
    }

    // TODO: This models a single, non-renameable FPSR/FPCR. Will resync
    // with QEMU often since many of the fields are written per-instruction.
    virtual uint64_t readfn(uArch* aCore) override { return aCore->getFPSR(); }          /*aa64_fpsr_read*/
    virtual void writefn(uArch* aCore, uint64_t aVal) override { aCore->setFPSR(aVal); } /*aa64_fpsr_write*/
    FPSR_()
      : SysRegInfo("FPSR_",
                   FPSR_::state,
                   FPSR_::type,
                   FPSR_::opc0,
                   FPSR_::opc1,
                   FPSR_::opc2,
                   FPSR_::crn,
                   FPSR_::crm,
                   FPSR_::access)
    {
    }
};

class DCZID_EL0_ : public SysRegInfo
{
  public:
    std::string name                      = "DCZID_EL0";
    static const eRegExecutionState state = kARM_STATE_AA64; // FIXME: confirm this
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 3;
    static const uint8_t opc2             = 7;
    static const uint8_t crn              = 0;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL0_R;
    static const eRegInfo type            = kARM_NO_RAW;
    uint64_t resetvalue                   = -1;

    virtual uint64_t readfn(uArch* aCore) override { return aCore->readDCZID_EL0(); }
    // No need for sync, effects are done by the readfn
    virtual void sync(uArch* aCore, size_t theNode) override {}
    DCZID_EL0_()
      : SysRegInfo("DCZID_EL0_",
                   DCZID_EL0_::state,
                   DCZID_EL0_::type,
                   DCZID_EL0_::opc0,
                   DCZID_EL0_::opc1,
                   DCZID_EL0_::opc2,
                   DCZID_EL0_::crn,
                   DCZID_EL0_::crm,
                   DCZID_EL0_::access)
    {
    }
};

class DC_ZVA_ : public SysRegInfo
{
  public:
    std::string name                      = "DC_ZVA";
    static const eRegExecutionState state = kARM_STATE_AA64; // FIXME: confirm this
    static const uint8_t opc0             = 1;
    static const uint8_t opc1             = 3;
    static const uint8_t opc2             = 1;
    static const uint8_t crn              = 7;
    static const uint8_t crm              = 4;
    static const eAccessRight access      = kPL0_W;
    static const eRegInfo type            = kARM_DC_ZVA;
    uint64_t resetvalue                   = -1;
    virtual eAccessResult accessfn(uArch* aCore) override { return aCore->accessZVA(); }
    // No need for sync, effects are done by the readfn
    virtual void sync(uArch* aCore, size_t theNode) override {}
    DC_ZVA_()
      : SysRegInfo("DC_ZVA_",
                   DC_ZVA_::state,
                   DC_ZVA_::type,
                   DC_ZVA_::opc0,
                   DC_ZVA_::opc1,
                   DC_ZVA_::opc2,
                   DC_ZVA_::crn,
                   DC_ZVA_::crm,
                   DC_ZVA_::access)
    {
    }
};

class CURRENT_EL_ : public SysRegInfo
{
  public:
    std::string name                      = "CURRENT_EL";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 0;
    static const uint8_t opc2             = 2;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 2;
    static const eAccessRight access      = kPL1_R;
    static const eRegInfo type            = kARM_CURRENTEL;
    uint64_t resetvalue                   = -1;

    virtual uint64_t readfn(uArch* aCore) override { return aCore->_PSTATE().EL(); }
    virtual void writefn(uArch* aCore, uint64_t aVal) override
    {
        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 2, 2, aVal));
    }
    virtual void sync(uArch* aCore, size_t theNode) override
    {
        auto pstate = Flexus::Qemu::API::qemu_api.read_register(theNode, Flexus::Qemu::API::PSTATE, 0);
        writefn(aCore, extract32(pstate, 2, 2));
    }

    CURRENT_EL_()
      : SysRegInfo("CURRENT_EL_",
                   CURRENT_EL_::state,
                   CURRENT_EL_::type,
                   CURRENT_EL_::opc0,
                   CURRENT_EL_::opc1,
                   CURRENT_EL_::opc2,
                   CURRENT_EL_::crn,
                   CURRENT_EL_::crm,
                   CURRENT_EL_::access)
    {
    }
};

class ELR_EL2_ : public SysRegInfo
{
  public:
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 1;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_ALIAS;
    ELR_EL2_()
      : SysRegInfo("ELR_EL2",
                   ELR_EL2_::state,
                   ELR_EL2_::type,
                   ELR_EL2_::opc0,
                   ELR_EL2_::opc1,
                   ELR_EL2_::opc2,
                   ELR_EL2_::crn,
                   ELR_EL2_::crm,
                   ELR_EL2_::access)
    {
    }
};
class ELR_EL1_ : public SysRegInfo
{
  public:
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 0;
    static const uint8_t opc2             = 1;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_ALIAS;

    virtual uint64_t readfn(uArch* aCore) override
    {
        // if PSTATE.EL == EL0 then
        //     UNDEFINED;
        // elsif PSTATE.EL == EL1 then
        //     if EL2Enabled() && HCR_EL2.<NV2,NV1,NV> == '011' then
        //         AArch64.SystemAccessTrap(EL2, 0x18);
        //     elsif EL2Enabled() && HCR_EL2.<NV2,NV1,NV> == '111' then
        //         return NVMem[0x230];
        //     else
        //         return ELR_EL1;
        // elsif PSTATE.EL == EL2 then
        //     if HCR_EL2.E2H == '1' then
        //         return ELR_EL2;
        //     else
        //         return ELR_EL1;
        // elsif PSTATE.EL == EL3 then
        //     return ELR_EL1;
        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, false);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);

        if (currentel == 1 || currentel == 3) return aCore->getELR_el(1);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                return aCore->getELR_el(2);
            else
                return aCore->getELR_el(1);
        }

        return aCore->getELR_el(1);
    }
    virtual void writefn(uArch* aCore, uint64_t aVal) override
    {
        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, false);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);

        if (currentel == 1 || currentel == 3) aCore->setELR_el(1, aVal);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                aCore->setELR_el(2, aVal);
            else
                aCore->setELR_el(1, aVal);
        }
    }
    virtual void sync(uArch* aCore, size_t theNode) override
    {
        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, true);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);
        auto valELR_EL1  = Flexus::Qemu::API::qemu_api.read_sys_register(theNode, opc0, opc1, opc2, crn, crm, true);
        auto valELR_EL2  = Flexus::Qemu::API::qemu_api.read_sys_register(theNode,
                                                                        ELR_EL2_::opc0,
                                                                        ELR_EL2_::opc1,
                                                                        ELR_EL2_::opc2,
                                                                        ELR_EL2_::crn,
                                                                        ELR_EL2_::crm, true);

        if (currentel == 1 || currentel == 3) writefn(aCore, valELR_EL1);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                writefn(aCore, valELR_EL2);
            else
                writefn(aCore, valELR_EL1);
        }
    }
    ELR_EL1_()
      : SysRegInfo("ELR_EL1",
                   ELR_EL1_::state,
                   ELR_EL1_::type,
                   ELR_EL1_::opc0,
                   ELR_EL1_::opc1,
                   ELR_EL1_::opc2,
                   ELR_EL1_::crn,
                   ELR_EL1_::crm,
                   ELR_EL1_::access)
    {
    }
};
class SPSR_EL2_ : public SysRegInfo
{
  public:
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    SPSR_EL2_()
      : SysRegInfo("SPSR_EL2",
                   SPSR_EL2_::state,
                   SPSR_EL2_::type,
                   SPSR_EL2_::opc0,
                   SPSR_EL2_::opc1,
                   SPSR_EL2_::opc2,
                   SPSR_EL2_::crn,
                   SPSR_EL2_::crm,
                   SPSR_EL2_::access)
    {
    }
};
class SPSR_EL1_ : public SysRegInfo
{
  public:
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 0;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 0;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    virtual uint64_t readfn(uArch* aCore) override
    {
        // if PSTATE.EL == EL0 then
        //     UNDEFINED;
        // elsif PSTATE.EL == EL1 then
        //     if EffectiveHCR_EL2_NVx() == '011' then
        //         AArch64.SystemAccessTrap(EL2, 0x18);
        //     elsif EffectiveHCR_EL2_NVx() IN {'111'} then
        //         X[t, 64] = NVMem[0x160];
        //     else
        //         X[t, 64] = SPSR_EL1;
        // elsif PSTATE.EL == EL2 then
        //     if ELIsInHost(EL2) then
        //         X[t, 64] = SPSR_EL2;
        //     else
        //         X[t, 64] = SPSR_EL1;
        // elsif PSTATE.EL == EL3 then
        //     X[t, 64] = SPSR_EL1;
        //
        // boolean ELIsInHost(bits(2) el)
        //     ...
        //     when EL2
        //         return EL2Enabled() && HCR_EL2.E2H == '1';
        //     ...
        //
        // boolean EL2Enabled()
        //  return HaveEL(EL2) && (!HaveEL(EL3) || SCR_GEN[].NS == '1' || IsSecureEL2Enabled());

        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, false);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);

        if (currentel == 1 || currentel == 3) return aCore->getSPSR_el(1);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                return aCore->getSPSR_el(2);
            else
                return aCore->getSPSR_el(1);
        }

        return aCore->getSPSR_el(1);
    }
    virtual void writefn(uArch* aCore, uint64_t aVal) override
    {
        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, false);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);

        if (currentel == 1 || currentel == 3) return aCore->setSPSR_el(1, aVal);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                return aCore->setSPSR_el(2, aVal);
            else
                return aCore->setSPSR_el(1, aVal);
        }
    }

    virtual void sync(uArch* aCore, size_t theNode) override
    {
        auto currentel   = aCore->_PSTATE().EL();
        auto HCR_EL2     = Flexus::Qemu::API::qemu_api.read_sys_register(0, 3, 4, 0, 1, 1, true);
        auto HCR_EL2_E2H = extract64(HCR_EL2, 34, 1);
        auto valSPSR_EL1 = Flexus::Qemu::API::qemu_api.read_sys_register(theNode, opc0, opc1, opc2, crn, crm, true);
        auto valSPSR_EL2 = Flexus::Qemu::API::qemu_api.read_sys_register(theNode,
                                                                         SPSR_EL2_::opc0,
                                                                         SPSR_EL2_::opc1,
                                                                         SPSR_EL2_::opc2,
                                                                         SPSR_EL2_::crn,
                                                                         SPSR_EL2_::crm, true);

        if (currentel == 1 || currentel == 3) writefn(aCore, valSPSR_EL1);
        if (currentel == 2) {
            if (HCR_EL2_E2H == 1)
                writefn(aCore, valSPSR_EL2);
            else
                writefn(aCore, valSPSR_EL1);
        }
    }

    SPSR_EL1_()
      : SysRegInfo("SPSR_EL1",
                   SPSR_EL1_::state,
                   SPSR_EL1_::type,
                   SPSR_EL1_::opc0,
                   SPSR_EL1_::opc1,
                   SPSR_EL1_::opc2,
                   SPSR_EL1_::crn,
                   SPSR_EL1_::crm,
                   SPSR_EL1_::access)
    {
    }
};

/* We rely on the access checks not allowing the guest to write to the
 * state field when SPSel indicates that it's being used as the stack
 * pointer.
 */
class SP_EL0_ : public SysRegInfo
{
  public:
    std::string name                      = "SP_EL0";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 0;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 1;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    virtual uint64_t readfn(uArch* aCore) override { return aCore->getSP_el(0); }
    virtual eAccessResult accessfn(uArch* aCore) override
    {
        return kACCESS_OK; // access OK since QEMU will check access and trap if necessary
    }

    virtual void writefn(uArch* aCore, uint64_t aVal) override { aCore->setSP_el(0, aVal); }

    SP_EL0_()
      : SysRegInfo("SP_EL0",
                   SP_EL0_::state,
                   SP_EL0_::type,
                   SP_EL0_::opc0,
                   SP_EL0_::opc1,
                   SP_EL0_::opc2,
                   SP_EL0_::crn,
                   SP_EL0_::crm,
                   SP_EL0_::access)
    {
    }
};

class SP_EL1_ : public SysRegInfo
{
  public:
    std::string name                      = "SP_EL1";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 1;
    static const eAccessRight access      = kPL2_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    virtual uint64_t readfn(uArch* aCore) override { return aCore->getSP_el(1); }

    virtual void writefn(uArch* aCore, uint64_t aVal) override { aCore->setSP_el(1, aVal); }

    SP_EL1_()
      : SysRegInfo("SP_EL1",
                   SP_EL1_::state,
                   SP_EL1_::type,
                   SP_EL1_::opc0,
                   SP_EL1_::opc1,
                   SP_EL1_::opc2,
                   SP_EL1_::crn,
                   SP_EL1_::crm,
                   SP_EL1_::access)
    {
    }
};

class SPSel_ : public SysRegInfo
{
  public:
    std::string name                      = "SPSel";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 0;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 2;
    static const eAccessRight access      = kPL1_RW;
    static const eRegInfo type            = kARM_NO_RAW;
    uint64_t resetvalue                   = -1;

    /*spsel_read*/
    virtual uint64_t readfn(uArch* aCore) override { return aCore->_PSTATE().SP(); }
    virtual void writefn(uArch* aCore, uint64_t aVal) override
    {
        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 0, 1, aVal));
        // unsigned int cur_el = aCore->_PSTATE().EL();
        /* Update PSTATE SPSel bit; this requires us to update the
         * working stack pointer in xregs[31].
         */
        //        if (!((aVal ^ aCore->_PSTATE().d()) & PSTATE_SP)) {
        //            return;
        //        }

        //        if (aCore->_PSTATE().d() & PSTATE_SP) {
        //            aCore->setSP_el(cur_el, aCore->getXRegister(31));
        //        } else {
        //            aCore->setSP_el(0, aCore->getXRegister(31));
        //        }

        //        aCore->setPSTATE(deposit32(aCore->_PSTATE().d(), 0, 1, aVal));

        //        /* We rely on illegal updates to SPsel from EL0 to get trapped
        //         * at translation time.
        //         */
        //        assert(cur_el >= 1 && cur_el <= 3);

        //        if (aCore->_PSTATE().d() & PSTATE_SP) {
        //            aCore->setXRegister(31, aCore->getSP_el(cur_el));
        //        } else {
        //            aCore->setXRegister(31, aCore->getSP_el(0));
        //        }

        // aCore->setSP_el(cur_el, aVal);
    }

    SPSel_()
      : SysRegInfo("SPSel",
                   SPSel_::state,
                   SPSel_::type,
                   SPSel_::opc0,
                   SPSel_::opc1,
                   SPSel_::opc2,
                   SPSel_::crn,
                   SPSel_::crm,
                   SPSel_::access)
    {
    }
};

class SPSR_IRQ_ : public SysRegInfo
{
  public:
    std::string name                      = "SPSR_IRQ";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 0;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 3;
    static const eAccessRight access      = kPL2_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    SPSR_IRQ_()
      : SysRegInfo("SPSR_IRQ",
                   SPSR_IRQ_::state,
                   SPSR_IRQ_::type,
                   SPSR_IRQ_::opc0,
                   SPSR_IRQ_::opc1,
                   SPSR_IRQ_::opc2,
                   SPSR_IRQ_::crn,
                   SPSR_IRQ_::crm,
                   SPSR_IRQ_::access)
    {
    }
};

class SPSR_ABT_ : public SysRegInfo
{
  public:
    std::string name                      = "SPSR_IRQ";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 1;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 3;
    static const eAccessRight access      = kPL2_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    SPSR_ABT_()
      : SysRegInfo("SPSR_ABT",
                   SPSR_ABT_::state,
                   SPSR_ABT_::type,
                   SPSR_ABT_::opc0,
                   SPSR_ABT_::opc1,
                   SPSR_ABT_::opc2,
                   SPSR_ABT_::crn,
                   SPSR_ABT_::crm,
                   SPSR_ABT_::access)
    {
    }
};

class SPSR_UND_ : public SysRegInfo
{
  public:
    std::string name                      = "SPSR_UND";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 2;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 3;
    static const eAccessRight access      = kPL2_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    SPSR_UND_()
      : SysRegInfo("SPSR_UND",
                   SPSR_UND_::state,
                   SPSR_UND_::type,
                   SPSR_UND_::opc0,
                   SPSR_UND_::opc1,
                   SPSR_UND_::opc2,
                   SPSR_UND_::crn,
                   SPSR_UND_::crm,
                   SPSR_UND_::access)
    {
    }
};

class SPSR_FIQ_ : public SysRegInfo
{
  public:
    std::string name                      = "SPSR_FIQ";
    static const eRegExecutionState state = kARM_STATE_AA64;
    static const uint8_t opc0             = 3;
    static const uint8_t opc1             = 4;
    static const uint8_t opc2             = 3;
    static const uint8_t crn              = 4;
    static const uint8_t crm              = 3;
    static const eAccessRight access      = kPL2_RW;
    static const eRegInfo type            = kARM_ALIAS;
    uint64_t resetvalue                   = -1;

    SPSR_FIQ_()
      : SysRegInfo("SPSR_FIQ",
                   SPSR_FIQ_::state,
                   SPSR_FIQ_::type,
                   SPSR_FIQ_::opc0,
                   SPSR_FIQ_::opc1,
                   SPSR_FIQ_::opc2,
                   SPSR_FIQ_::crn,
                   SPSR_FIQ_::crm,
                   SPSR_FIQ_::access)
    {
    }
};

class INVALID_PRIV_ : public SysRegInfo
{
  public:
    std::string name = "INVALID_PRIV";
};

std::vector<std::pair<std::array<uint8_t, 5>, ePrivRegs>> supported_sysRegs = {
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>({ NZCV_::opc0, NZCV_::opc1, NZCV_::opc2, NZCV_::crn, NZCV_::crm },
                                                      kNZCV),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>({ DAIF_::opc0, DAIF_::opc1, DAIF_::opc2, DAIF_::crn, DAIF_::crm },
                                                      kDAIF),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { TPIDR_EL0_::opc0, TPIDR_EL0_::opc1, TPIDR_EL0_::opc2, TPIDR_EL0_::crn, TPIDR_EL0_::crm },
      kTPIDR_EL0),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { TPIDR_EL2_::opc0, TPIDR_EL2_::opc1, TPIDR_EL2_::opc2, TPIDR_EL2_::crn, TPIDR_EL2_::crm },
      kTPIDR_EL2),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>({ FPCR_::opc0, FPCR_::opc1, FPCR_::opc2, FPCR_::crn, FPCR_::crm },
                                                      kFPCR),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>({ FPSR_::opc0, FPSR_::opc1, FPSR_::opc2, FPSR_::crn, FPSR_::crm },
                                                      kFPSR),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { DCZID_EL0_::opc0, DCZID_EL0_::opc1, DCZID_EL0_::opc2, DCZID_EL0_::crn, DCZID_EL0_::crm },
      kDCZID_EL0),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { DC_ZVA_::opc0, DC_ZVA_::opc1, DC_ZVA_::opc2, DC_ZVA_::crn, DC_ZVA_::crm },
      kDC_ZVA),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { CURRENT_EL_::opc0, CURRENT_EL_::opc1, CURRENT_EL_::opc2, CURRENT_EL_::crn, CURRENT_EL_::crm },
      kCURRENT_EL),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { ELR_EL1_::opc0, ELR_EL1_::opc1, ELR_EL1_::opc2, ELR_EL1_::crn, ELR_EL1_::crm },
      kELR_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { SPSR_EL1_::opc0, SPSR_EL1_::opc1, SPSR_EL1_::opc2, SPSR_EL1_::crn, SPSR_EL1_::crm },
      kSPSR_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { SP_EL0_::opc0, SP_EL0_::opc1, SP_EL0_::opc2, SP_EL0_::crn, SP_EL0_::crm },
      kSP_EL0),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { SP_EL1_::opc0, SP_EL1_::opc1, SP_EL1_::opc2, SP_EL1_::crn, SP_EL1_::crm },
      kSP_EL1),
    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
      { SPSel_::opc0, SPSel_::opc1, SPSel_::opc2, SPSel_::crn, SPSel_::crm },
      kSPSel),
    // std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
    //    {MDSCR_EL1_::opc0, MDSCR_EL1_::opc1, MDSCR_EL1_::opc2, MDSCR_EL1_::crn, MDSCR_EL1_::crm},
    //    kMDSCR_EL1) std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_IRQ_::opc0,
    //    SPSR_IRQ_::opc1, SPSR_IRQ_::opc2, SPSR_IRQ_::crn, SPSR_IRQ_::crm},  kSPSR_IRQ),
    //    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>( {SPSR_ABT_::opc0, SPSR_ABT_::opc1,
    //    SPSR_ABT_::opc2, SPSR_ABT_::crn, SPSR_ABT_::crm},  kSPSR_ABT),
    //    std::make_pair<std::array<uint8_t, 5>, ePrivRegs>(
    //    {SPSR_UND_::opc0, SPSR_UND_::opc1, SPSR_UND_::opc2, SPSR_UND_::crn,
    //    SPSR_UND_::crm},  kSPSR_UND), std::make_pair<std::array<uint8_t,
    //    5>, ePrivRegs>( {SPSR_FIQ_::opc0, SPSR_FIQ_::opc1, SPSR_FIQ_::opc2,
    //    SPSR_FIQ_::crn, SPSR_FIQ_::crm},  kSPSR_FIQ)
};

ePrivRegs
getPrivRegType(const uint8_t op0, const uint8_t op1, const uint8_t op2, const uint8_t crn, const uint8_t crm)
{

    std::array<uint8_t, 5> op = { op0, op1, op2, crn, crm };
    for (std::pair<std::array<uint8_t, 5>, ePrivRegs>& e : supported_sysRegs) {
        if (e.first == op) return e.second;
    }
    return kLastPrivReg;
}

std::unique_ptr<SysRegInfo>
getPriv(ePrivRegs aCode)
{
    switch (aCode) {
        case kNZCV: return std::make_unique<NZCV_>();
        case kDAIF: return std::make_unique<DAIF_>();
        case kFPCR: return std::make_unique<FPCR_>();
        case kFPSR: return std::make_unique<FPSR_>();
        case kDCZID_EL0: return std::make_unique<DCZID_EL0_>();
        case kDC_ZVA: return std::make_unique<DC_ZVA_>();
        case kCURRENT_EL: return std::make_unique<CURRENT_EL_>();
        case kELR_EL1: return std::make_unique<ELR_EL1_>();
        case kSPSR_EL1: return std::make_unique<SPSR_EL1_>();
        case kSP_EL0: return std::make_unique<SP_EL0_>();
        case kSP_EL1: return std::make_unique<SP_EL1_>();
        case kSPSel: return std::make_unique<SPSel_>();
        case kSPSR_IRQ: return std::make_unique<SPSR_IRQ_>();
        case kSPSR_ABT: return std::make_unique<SPSR_ABT_>();
        case kSPSR_UND: return std::make_unique<SPSR_UND_>();
        case kSPSR_FIQ: return std::make_unique<SPSR_FIQ_>();
        case kTPIDR_EL0: return std::make_unique<TPIDR_EL0_>();
        case kTPIDR_EL2: return std::make_unique<TPIDR_EL2_>();
        default: // FIXME: Only return default/abstract if implemented by QEMU
            return std::make_unique<SysRegInfo>();
            // DBG_Assert(false, (<< "Unimplemented SysReg Code" << aCode));
            // return INVALID_PRIV_;
    }
}

std::unique_ptr<SysRegInfo>
getPriv(uint8_t op0, uint8_t op1, uint8_t op2, uint8_t crn, uint8_t crm)
{
    ePrivRegs r = getPrivRegType(op0, op1, op2, crn, crm);
    return getPriv(r);
}

} // namespace nuArch
