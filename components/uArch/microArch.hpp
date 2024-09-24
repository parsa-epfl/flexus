
#ifndef FLEXUS_uARCH_microARCH_HPP_INCLUDED
#define FLEXUS_uARCH_microARCH_HPP_INCLUDED

#include "components/CommonQEMU/Slices/MemOp.hpp"
#include "components/CommonQEMU/Slices/PredictorMessage.hpp" /* CMU-ONLY */
#include "components/CommonQEMU/Translation.hpp"
#include "components/uArch/uArchInterfaces.hpp"

#include <functional>
#include <memory>

using namespace Flexus::SharedTypes;

namespace nuArch {

struct microArch
{
    static std::shared_ptr<microArch> construct(uArchOptions_t options,
                                                std::function<void(eSquashCause)> squash,
                                                std::function<void(VirtualMemoryAddress)> redirect,
                                                std::function<void(boost::intrusive_ptr<BranchFeedback>)> feedback,
                                                std::function<void(bool)> aStoreForwardingHitFunction,
                                                std::function<void(int32_t)> mmuResyncFunction);

    virtual int32_t availableROB()                                                                 = 0;
    virtual const uint32_t core() const                                                            = 0;
    virtual bool isSynchronized()                                                                  = 0;
    virtual bool isQuiesced()                                                                      = 0;
    virtual bool isStalled()                                                                       = 0;
    virtual bool isHalted()                                                                        = 0;
    virtual int32_t iCount()                                                                       = 0;
    virtual void dispatch(boost::intrusive_ptr<AbstractInstruction>)                               = 0;
    virtual void skipCycle()                                                                       = 0;
    virtual void cycle()                                                                           = 0;
    virtual void issueMMU(TranslationPtr aTranslation)                                             = 0;
    virtual void pushMemOp(boost::intrusive_ptr<MemOp>)                                            = 0;
    virtual bool canPushMemOp()                                                                    = 0;
    virtual boost::intrusive_ptr<MemOp> popMemOp()                                                 = 0;
    virtual TranslationPtr popTranslation()                                                        = 0;
    virtual void pushTranslation(TranslationPtr aTranslation)                                      = 0;
    virtual boost::intrusive_ptr<MemOp> popSnoopOp()                                               = 0;
    virtual void markExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize, uint64_t marker) = 0;
    virtual int isExclusiveLocal(PhysicalMemoryAddress anAddress, eSize aSize)                     = 0;
    virtual bool isROBHead(boost::intrusive_ptr<Instruction> anInstruction)                        = 0;
    virtual void clearExclusiveLocal()                                                             = 0;
    virtual ~microArch() {}
    virtual void testCkptRestore()                                    = 0;
    virtual void printROB()                                           = 0;
    virtual void printSRB()                                           = 0;
    virtual void printMemQueue()                                      = 0;
    virtual void printMSHR()                                          = 0;
    virtual void pregs()                                              = 0;
    virtual void pregsAll()                                           = 0;
    virtual void resynchronize(bool was_expected)                     = 0;
    virtual void printRegMappings(std::string)                        = 0;
    virtual void printRegFreeList(std::string)                        = 0;
    virtual void printRegReverseMappings(std::string)                 = 0;
    virtual void printAssignments(std::string)                        = 0;
    virtual void writePermissionLost(PhysicalMemoryAddress anAddress) = 0;
};

} // namespace nuArchARM

#endif // FLEXUS_uARCH_microARCH_HPP_INCLUDED