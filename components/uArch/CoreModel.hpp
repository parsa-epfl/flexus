
#ifndef FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
#define FLEXUS_uARCH_COREMODEL_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <components/CommonQEMU/Slices/PredictorMessage.hpp> /* CMU-ONLY */
#include <components/uFetch/uFetchTypes.hpp>

namespace nuArch {

enum eLoseWritePermission
{
    eLosePerm_Invalidate,
    eLosePerm_Downgrade,
    eLosePerm_Replacement
};

struct State
{
    uint64_t theCCRegs;
    uint64_t theGlobalRegs[32];
    uint64_t thePC;
    uint64_t theFPRegs[64];
    uint32_t theFPSR;
    uint32_t theFPCR;
    uint32_t thePSTATE;
};
struct CoreModel : public uArch
{
    static CoreModel* construct(uArchOptions_t options
                                // Msutherl, removed
                                //, std::function< void (Flexus::Qemu::Translation &) > translate
                                ,
                                std::function<int(bool)> advance,
                                std::function<void(eSquashCause)> squash,
                                std::function<void(VirtualMemoryAddress)> redirect,
                                std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                                std::function<void(bool)> signalStoreForwardingHit,
                                std::function<void(int32_t)> mmuResync);

    // Interface to mircoArch
    virtual void initializeRegister(mapped_reg aRegister, register_value aValue)  = 0;
    virtual register_value readArchitecturalRegister(reg aRegister, bool aRotate) = 0;
    //  virtual int32_t selectedRegisterSet() const = 0;
    virtual void setRoundingMode(uint32_t aRoundingMode) = 0;

    virtual void getState(State& aState)     = 0;
    virtual void restoreState(State& aState) = 0;

    virtual void setPC(uint64_t aPC) = 0;
    virtual uint64_t pc() const      = 0;
    virtual void dumpActions()       = 0;
    virtual void reset()             = 0;

    virtual int32_t availableROB() const                     = 0;
    virtual bool isSynchronized() const                      = 0;
    virtual bool isStalled() const                           = 0;
    virtual bool isHalted() const                            = 0;
    virtual int32_t iCount() const                           = 0;
    virtual bool isQuiesced() const                          = 0;
    virtual void dispatch(boost::intrusive_ptr<Instruction>) = 0;

    virtual void skipCycle()                             = 0;
    virtual void cycle(eExceptionType aPendingInterrupt) = 0;
    virtual void issueMMU(TranslationPtr aTranslation)   = 0;

    virtual bool checkValidatation()                          = 0;
    virtual void pushMemOp(boost::intrusive_ptr<MemOp>)       = 0;
    virtual bool canPushMemOp()                               = 0;
    virtual boost::intrusive_ptr<MemOp> popMemOp()            = 0;
    virtual boost::intrusive_ptr<MemOp> popSnoopOp()          = 0;
    virtual TranslationPtr popTranslation()                   = 0;
    virtual void pushTranslation(TranslationPtr aTranslation) = 0;
    virtual void printROB()                                   = 0;
    virtual void printSRB()                                   = 0;
    virtual void printMemQueue()                              = 0;
    virtual void printMSHR()                                  = 0;

    virtual void printRegMappings(std::string)        = 0;
    virtual void printRegFreeList(std::string)        = 0;
    virtual void printRegReverseMappings(std::string) = 0;
    virtual void printAssignments(std::string)        = 0;

    virtual void loseWritePermission(eLoseWritePermission aReason, PhysicalMemoryAddress anAddress) = 0;

    // MMU and Multi-stage translation, now in CoreModel, not QEMU MAI
    // - Msutherl: Oct'18
    //  virtual void translate(boost::intrusive_ptr<Translation>& aTr) = 0;
};

struct ResynchronizeWithQemuException
{
    bool expected;
    ResynchronizeWithQemuException(bool was_expected = false)
      : expected(was_expected)
    {
    }
};

} // namespace nuArch

#endif // FLEXUS_uARCH_COREMODEL_HPP_INCLUDED