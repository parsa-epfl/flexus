
#include "Validations.hpp"

#include "components/uArch/uArchInterfaces.hpp"
#include <components/uArch/ValueTracker.hpp>
#include "core/qemu/api_wrappers.hpp"

#define DBG_DeclareCategories Decoder
#define DBG_SetDefaultOps     AddCat(Decoder)
#include DBG_Control()

namespace nDecoder {

using namespace nuArch;

bool
validateXRegister::operator()()
{
    if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
        return true; // Don't check
    }
    if (theInstruction->raised() != kException_None) {
        DBG_(VVerb,
             (<< " Not performing register validation for " << theReg << " because of exception. " << *theInstruction));
        return true;
    }

    uint64_t flexus = theInstruction->operand<uint64_t>(theOperandCode);
    uint64_t qemu =
      (Flexus::Qemu::Processor::getProcessor(theInstruction->cpu()).read_register(Flexus::Qemu::API::GENERAL, theReg)) &
      (the_64 ? -1LL : 0xFFFFFFFF);

    DBG_(Dev,
         Condition(flexus != qemu)(<< "flexus value in " << std::setw(10) << theOperandCode << "  = " << std::hex
                                   << flexus << std::dec));
    DBG_(Dev,
         Condition(flexus != qemu)(<< "qemu value in   " << std::setw(10) << theReg << "  = " << std::hex << qemu
                                   << std::dec));

    return (flexus == qemu);
}

bool
validatePC::operator()()
{
    if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
        return true; // Don't check
    }
    if (theInstruction->raised() != kException_None) {
        DBG_(VVerb, (<< " Not performing  validation because of exception. " << *theInstruction));
        return true;
    }

    uint64_t flexus = thePreValidation ? theInstruction->pc() : theInstruction->pcNext();
    uint64_t qemu   = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu()).read_register(Flexus::Qemu::API::PC);

    DBG_(Dev, Condition(flexus != qemu)(<< "flexus PC value " << std::hex << flexus << std::dec));
    DBG_(Dev, Condition(flexus != qemu)(<< "qemu PC value   " << std::hex << qemu << std::dec));

    return flexus == qemu;
}

bool
validateMemory::operator()()
{
    if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
        return true; // Don't check
    }

    bits flexus;
    if (theValueCode == kResult) {
        flexus = theInstruction->operand<bits>(theValueCode);
    } else {
        flexus = theInstruction->operand<uint64_t>(theValueCode);
    }

    Flexus::Qemu::Processor c = Flexus::Qemu::Processor::getProcessor(theInstruction->cpu());
    VirtualMemoryAddress vaddr(theInstruction->operand<uint64_t>(theAddressCode));
    int theSize_orig = theSize, theSize_extra = 0;
    if (theSize < 8) flexus &= 0xFFFFFFFF >> (32 - theSize * 8);
    VirtualMemoryAddress vaddr_final = vaddr + theSize_orig - 1;
    if ((vaddr & 0x1000) != (vaddr_final & 0x1000)) {
        theSize_extra = (vaddr_final & 0xFFF) + 1;
        DBG_Assert(theSize_extra < 16);
        theSize_orig -= theSize_extra;
        vaddr_final = (VirtualMemoryAddress)(vaddr_final & ~0xFFFULL);
    }
    PhysicalMemoryAddress paddr = c.translate_va2pa(vaddr);
    bits qemu                   = c.read_pa(paddr, theSize_orig);
    if (theSize_extra) {
        DBG_Assert((qemu >> (theSize_orig * 8)) == 0);
        PhysicalMemoryAddress paddr_spill = c.translate_va2pa(vaddr_final);
        qemu |= c.read_pa(paddr_spill, theSize_extra) << (theSize_orig * 8);
    }

    if (flexus == qemu) return true;
    // TODO: check
    // mmio
    if (theInstruction->getAccessAddress() < 0x40000000)
        return true;

    ValueTracker::valueTracker(0).invalidate(paddr, theSize);

    DBG_(Dev, Condition(flexus != qemu)(<< "flexus value: " << std::hex << flexus));
    DBG_(Dev, Condition(flexus != qemu)(<< "qemu value:   " << std::hex << qemu));

    return (flexus == qemu);
}

} // namespace nDecoder
