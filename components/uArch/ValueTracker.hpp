
#ifndef FLEXUS_UARCH_VALUETRACKER_HPP_INCLUDED
#define FLEXUS_UARCH_VALUETRACKER_HPP_INCLUDED

#include "components/uArch/uArchInterfaces.hpp"

#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/debug/debug.hpp>
#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <iostream>
#include <unordered_map>

namespace API = Flexus::Qemu::API;
using namespace Flexus::SharedTypes;

#define DBG_DeclareCategories Special
#include DBG_Control()

namespace nuArch {

inline bits
mask(eSize aSize)
{
    switch (aSize) {
        case kByte: return (bits)0xFFULL;
        case kHalfWord: return (bits)0xFFFFULL;
        case kWord: return (bits)0xFFFFFFFFULL;
        case kDoubleWord: return (bits)0xFFFFFFFFFFFFFFFFULL;
        case kQuadWord: return (bits)-1;
        default: DBG_Assert(false); return 0;
    }
}

struct LocalValue
{
    bits theValue;
    uint32_t theOutstandingStores;
    LocalValue(bits aValue)
      : theValue(aValue)
      , theOutstandingStores(1)
    {
    }
};

typedef enum
{
    kSimicsReflectsG = -1,
    kPoisonedByDMA   = -2
} eValueTrackSpecialValues;

struct ValueTrack
{
    bits theSimicsReflectsCPU;
    bits theGloballyVisibleValue;
    typedef std::map<uint32_t, LocalValue> local_values;
    local_values theLocallyVisibleValues;

    ValueTrack(int32_t aCPU, bits aLocalValue, bits aGlobalValue)
    {
        theSimicsReflectsCPU    = aCPU;
        theGloballyVisibleValue = aGlobalValue;
        theLocallyVisibleValues.insert(std::make_pair(aCPU, LocalValue(aLocalValue)));
    }

    friend std::ostream& operator<<(std::ostream& anOstream, ValueTrack const& t)
    {
        anOstream << " G:" << std::hex << t.theGloballyVisibleValue << std::dec;
        local_values::const_iterator iter = t.theLocallyVisibleValues.begin();
        while (iter != t.theLocallyVisibleValues.end()) {
            anOstream << " " << boost::padded_string_cast<2, '0'>(iter->first) << ":" << std::hex
                      << iter->second.theValue << std::dec << "<" << iter->second.theOutstandingStores << ">";
            ++iter;
        }

        if (t.theSimicsReflectsCPU == (bits)(kSimicsReflectsG)) {
            anOstream << ". Simics Reflects: G";
        } else if (t.theSimicsReflectsCPU == (bits)(kPoisonedByDMA)) {
            anOstream << ". Simics poisoned by DMA";
        } else {
            anOstream << ". Simics Reflects: " << t.theSimicsReflectsCPU;
        }
        return anOstream;
    }
};

class DMATracerImpl
{
    API::conf_object_t* theMapObject;
    int theVM;

  public:
    DMATracerImpl(API::conf_object_t* anUnderlyingObjec) {}

    // Initialize the tracer to the desired CPU
    void init(API::conf_object_t* aMapObject, int aVM)
    {
        theMapObject = aMapObject;
        theVM        = aVM;
        // ALEX - Not setting up tracer for now.
        //
        // API::attr_value_t attr;
        // attr.kind = API::Sim_Val_Object;
        // attr.u.object = theUnderlyingObject;

        /* Tell memory we have a mem hier */
        // API::SIM_set_attribute(theMapObject, "timing_model", &attr);
    }

    // ALEX - WARNING: Disabled dma for now. Also in ValueTracker.cpp
    // API::cycles_t dma_mem_hier_operate(API::conf_object_t * space,
    // API::map_list_t * map, API::generic_transaction_t * aMemTrans);

}; // class DMATracerImpl

class DMATracer : public Flexus::Qemu::AddInObject<DMATracerImpl>
{
    typedef Flexus::Qemu::AddInObject<DMATracerImpl> base;

  public:
    static const Flexus::Qemu::Persistence class_persistence = Flexus::Qemu::Session;
    static std::string className() { return "DMATracer"; }
    static std::string classDescription() { return "Flexus's DMA tracer."; }

    DMATracer()
      : base()
    {
    }
    DMATracer(API::conf_object_t* aSimicsObject)
      : base(aSimicsObject)
    {
    }
    DMATracer(DMATracerImpl* anImpl)
      : base(anImpl)
    {
    }
};

struct ValueTracker
{
    static ValueTracker** theGlobalTracker;
    static int theNumTrackers;
    static ValueTracker& valueTracker(int node)
    {
        if (!theGlobalTracker) {
            theNumTrackers = 1 /*Flexus::Qemu::ProcessorMapper::numVMs()*/;
            if (theNumTrackers == 0) { theNumTrackers = 1; }
            theGlobalTracker = new ValueTracker*[theNumTrackers];
            for (int i = 0; i < theNumTrackers; i++) {
                theGlobalTracker[i] = new ValueTracker;
                // theGlobalTracker[i]->register_mem_iface(i);
            }
        }
        return *theGlobalTracker[0];
    }

    typedef std::unordered_map<uint64_t, ValueTrack> tracker;
    tracker theTracker;
    DMATracer theDMATracer;

    void access(uint32_t aCPU, PhysicalMemoryAddress anAddress)
    {
        FLEXUS_PROFILE();
        DBG_(VVerb, (<< "CPU[" << aCPU << "] Access " << anAddress));
        DBG_Assert(anAddress < 0x40000000000LL);

        // Align the address
        PhysicalMemoryAddress aligned = dwAddr(anAddress);

        // See if we already have a ValueTrack
        tracker::iterator iter = theTracker.find(aligned);
        if (iter == theTracker.end()) {
            DBG_(VVerb, (<< "CPU[" << aCPU << "] Access.NothingOutstanding " << anAddress));
            return;
        } else {
            if (iter->second.theSimicsReflectsCPU == (bits)(kPoisonedByDMA)) {
                // the globally visible value was updated by DMA, so update it just in
                // case we use it later
                Flexus::Qemu::Processor cpu          = Flexus::Qemu::Processor::getProcessor(aCPU);
                iter->second.theGloballyVisibleValue = cpu.read_pa(aligned, 8);
                iter->second.theSimicsReflectsCPU    = kSimicsReflectsG;
                DBG_(VVerb,
                     AddCategory(Special)(<< "CPU[" << aCPU << "] Access.UpdatedGlobalWithDMA " << anAddress
                                          << " Now: " << iter->second));
            }
            ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find(aCPU);
            if (local != iter->second.theLocallyVisibleValues.end()) {
                if (iter->second.theSimicsReflectsCPU == static_cast<bits>(aCPU)) {
                    // Simics currently has the value for this CPU.  We update the tracker
                    // with the new value
                    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
                    bits simics_value           = cpu.read_pa(aligned, 8);

                    if (simics_value != local->second.theValue) {
                        DBG_(VVerb,
                             (<< "CPU[" << aCPU << "] Access.SimicsMoreRecent " << anAddress << " " << std::hex
                              << simics_value << " Expected: " << local->second.theValue << std::dec));
                        // DBG_Assert( false );
                        local->second.theValue = simics_value;
                    }
                    // When the store is completed, Simics will reflect the value visible
                    // to this processor

                    DBG_(VVerb,
                         (<< "CPU[" << aCPU << "] Access.SimicsReflectsCorrectCPU " << anAddress
                          << " Now: " << iter->second));

                } else {
                    // Simics currently has the value for some other CPU.  We overwrite
                    // Simics memory with the correct value for this CPU.

                    // Change simics' value to reflect what this CPU believes memory
                    // looks like
                    //          //cpu.writePAddr( aligned, 8, current_value );
                    iter->second.theSimicsReflectsCPU = aCPU;

                    DBG_(VVerb,
                         AddCategory(Special)(<< "CPU[" << aCPU << "] Access.SimicsSwitchedToCorrectCPU " << anAddress
                                              << " Now: " << iter->second));
                }

            } else {
                // No previous value for this CPU.  Start with the globally visible
                // value

                if (iter->second.theSimicsReflectsCPU == (bits)(kSimicsReflectsG)) {
                    DBG_(VVerb,
                         (<< "CPU[" << aCPU << "] Access.SimicsReflectsCorrectGlobal " << anAddress
                          << " Now: " << iter->second));
                } else {

                    iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

                    DBG_(VVerb,
                         AddCategory(Special)(<< "CPU[" << aCPU << "] Access.SwitchedToGlobalValue " << anAddress
                                              << " Now: " << iter->second));
                }
            }
        }
    }

    void store(uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize, bits aStoreValue)
    {
        FLEXUS_PROFILE();
        DBG_Assert(anAddress != kUnresolved);
        // DBG_Assert( anAddress < 0x40000000000LL );
        DBG_(VVerb,
             (<< "CPU[" << aCPU << "] Store " << anAddress << "[" << aSize << "] = " << std::hex << aStoreValue
              << std::dec));

        // mmio
        // higher region of MMIO starts at 0x40_0000_0000 for RAM less than 256GB
        if (anAddress < 0x40000000 || anAddress >= 0x4000000000) return;

        // Align the address
        PhysicalMemoryAddress aligned = dwAddr(anAddress);

        // See if we already have a ValueTrack
        tracker::iterator iter = theTracker.find(aligned);
        if (iter == theTracker.end()) {
            Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
            bits simics_value           = cpu.read_pa(aligned, aSize);
            // New tracker
            bits updated_value = overlay(simics_value, anAddress, aSize, aStoreValue);

            tracker::iterator iter;
            bool ignored;
            std::tie(iter, ignored) =
              theTracker.insert(std::make_pair(aligned, ValueTrack(aCPU, updated_value, simics_value)));
            // When the store is completed, Simics will reflect the value visible to
            // this processor
            // cpu.writePAddr( aligned, 8, updated_value );

            DBG_(VVerb,
                 (<< "CPU[" << aCPU << "] Store.New " << anAddress
                  << " no prior outstanding values. Now: " << iter->second));

        } else {
            if (iter->second.theSimicsReflectsCPU == (bits)(kPoisonedByDMA)) {
                // the globally visible value was updated by DMA, so update it just in
                // case we use it later
                Flexus::Qemu::Processor cpu          = Flexus::Qemu::Processor::getProcessor(aCPU);
                iter->second.theGloballyVisibleValue = cpu.read_pa(aligned, 8);
                iter->second.theSimicsReflectsCPU    = kSimicsReflectsG;
                DBG_(VVerb,
                     AddCategory(Special)(<< "CPU[" << aCPU << "] Store.UpdatedGlobalWithDMA " << anAddress
                                          << " Now: " << iter->second));
            }

            ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find(aCPU);
            if (local != iter->second.theLocallyVisibleValues.end()) {
                if (iter->second.theSimicsReflectsCPU == static_cast<bits>(aCPU)) {
                    // Simics currently has the value for this CPU.  We update the tracker
                    // with the new value
                    Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);
                    bits simics_value           = cpu.read_pa(aligned, 8);

                    if (simics_value != local->second.theValue) {
                        DBG_(VVerb,
                             (<< "Simics value more recent than ValueTracker: " << std::hex << simics_value
                              << " Expected: " << local->second.theValue << std::dec));
                        // DBG_Assert(false);
                    }
                    bits updated_value     = overlay(simics_value, anAddress, aSize, aStoreValue);
                    local->second.theValue = updated_value;
                    local->second.theOutstandingStores++;
                    // When the store is completed, Simics will reflect the value visible
                    // to this processor cpu.writePAddr ( aligned, 8, updated_value );

                    DBG_(VVerb,
                         AddCategory(Special)(<< "CPU[" << aCPU << "] Store.UpdateByCurrent " << anAddress
                                              << " Now: " << iter->second));

                } else {
                    // Simics currently has the value for some other CPU.  We overwrite
                    // Simics memory with the correct value for this CPU.

                    bits current_value = local->second.theValue;
                    bits updated_value = overlay(current_value, anAddress, aSize, aStoreValue);

                    // Change simics' value to reflect what this CPU believes memory
                    // looks like
                    // cpu.writePAddr( aligned, 8, updated_value );
                    iter->second.theSimicsReflectsCPU = aCPU;

                    local->second.theValue = updated_value;
                    local->second.theOutstandingStores++;

                    DBG_(VVerb,
                         AddCategory(Special)(<< "CPU[" << aCPU << "] Store.UpdateByExisting " << anAddress
                                              << " Now: " << iter->second));
                }
            } else {
                // No previous value for this CPU.  Start with the globally visible
                // value

                bits current_value = iter->second.theGloballyVisibleValue;
                bits updated_value = overlay(current_value, anAddress, aSize, aStoreValue);

                // Change simics' value to reflect what this CPU believes memory
                // looks like
                // cpu.writePAddr( aligned, 8, updated_value );
                iter->second.theSimicsReflectsCPU = aCPU;

                iter->second.theLocallyVisibleValues.insert(std::make_pair(aCPU, LocalValue(updated_value)));

                DBG_(VVerb,
                     AddCategory(Special)(<< "CPU[" << aCPU << "] Store.UpdateByNew " << anAddress
                                          << " Now: " << iter->second));
            }
        }
    }

    void commitStore(uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize, bits aStoreValue)
    {
        FLEXUS_PROFILE();
        DBG_Assert(anAddress != kUnresolved);
        DBG_Assert(anAddress < 0x40000000000LL);
        DBG_(VVerb,
             (<< "CPU[" << aCPU << "] CommitStore " << anAddress << "[" << aSize << "] = " << std::hex << aStoreValue
              << std::dec));
        //
        // mmio
        // higher region of MMIO starts at 0x40_0000_0000 for RAM less than 256GB
        if (anAddress < 0x40000000 || anAddress >= 0x4000000000) return;

        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

        // Align the address
        PhysicalMemoryAddress aligned = dwAddr(anAddress);

        // We must have a ValueTrack
        tracker::iterator iter = theTracker.find(aligned);
        DBG_Assert(iter != theTracker.end());
        ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find(aCPU);
        DBG_Assert(local != iter->second.theLocallyVisibleValues.end());

        if (iter->second.theSimicsReflectsCPU == (bits)(kPoisonedByDMA)) {
            // the globally visible value was updated by DMA, so update it just in
            // case we use it later
            iter->second.theGloballyVisibleValue = cpu.read_pa(aligned, 8);
            iter->second.theSimicsReflectsCPU    = kSimicsReflectsG;
            DBG_(VVerb,
                 AddCategory(Special)(<< "CPU[" << aCPU << "] CommitStore.UpdatedGlobalWithDMA " << anAddress
                                      << " Now: " << iter->second));
        }

        // Compute the new globally visible value
        bits current_value                   = iter->second.theGloballyVisibleValue;
        bits updated_value                   = overlay(current_value, anAddress, aSize, aStoreValue);
        iter->second.theGloballyVisibleValue = updated_value;

        /*
        // To debug store problems (e.g., where the store value matches with Simics
        but the actual
        // bytes written do not), set Flexus to sequential consistency, disable
        writePAddr() in
        // store() above, and enable the check below. Be careful about data races,
        which can
        // appear here as mismatches, even under SC.
        bits simics_value = cpu.read_pa( aligned, 8);
        if(simics_value != updated_value) {
          DBG_(VVerb, ( << "CPU[" << aCPU << "] memory mismatch: Simics: " << std::hex
        << simics_value << "  Flexus: " << updated_value ) );
        }
        */

        // Change simics to reflect the globally visible value
        // cpu.writePAddr( aligned, 8, updated_value );
        iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

        // Decrement outstanding store count
        local->second.theOutstandingStores--;
        if (local->second.theOutstandingStores == 0) {
            DBG_(VVerb,
                 AddCategory(Special)(<< "CPU[" << aCPU << "] CommitStore.LastOutstandingStoreForCPU " << anAddress));
            iter->second.theLocallyVisibleValues.erase(local);
        }
        if (iter->second.theLocallyVisibleValues.empty()) {
            DBG_(VVerb, AddCategory(Special)(<< "CPU[" << aCPU << "] CommitStore.LastOutstandingStore " << anAddress));
            theTracker.erase(iter);
        } else {
            DBG_(VVerb,
                 AddCategory(Special)(<< "CPU[" << aCPU << "] CommitStore.ResultingState " << anAddress
                                      << " Now: " << iter->second));
        }
    }

    bits load(uint32_t aCPU, PhysicalMemoryAddress anAddress, eSize aSize)
    {
        FLEXUS_PROFILE();
        DBG_Assert(anAddress != kUnresolved);
        DBG_Assert(aSize <= 16 && aSize >= 1);
        DBG_Assert(anAddress < 0x40000000000LL);
        DBG_(VVerb, (<< "CPU[" << aCPU << "] Load " << anAddress << "[" << aSize << "]"));

        // mmio
        // higher region of MMIO starts at 0x40_0000_0000 for RAM less than 256GB
        if (anAddress < 0x40000000 || anAddress >= 0x4000000000) return -1;

        Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(aCPU);

        // Align the address
        PhysicalMemoryAddress aligned = dwAddr(anAddress);

        // See if we already have a ValueTrack
        tracker::iterator iter = theTracker.find(aligned);
        if (iter == theTracker.end()) {
            // for mmio loads, we can only assume that they are idempotent
            bits val = cpu.read_pa(anAddress, aSize);
            DBG_(VVerb,
                 (<< "CPU[" << aCPU << "] Load.NoOutstandingValues " << anAddress << "[" << aSize << "] = " << std::hex
                  << val << std::dec));
            return val;
        }

        if (iter->second.theSimicsReflectsCPU == (bits)(kPoisonedByDMA)) {
            // the globally visible value was updated by DMA, so update it just in
            // case we use it later
            iter->second.theGloballyVisibleValue = cpu.read_pa(aligned, 8);
            iter->second.theSimicsReflectsCPU    = kSimicsReflectsG;
            DBG_(VVerb,
                 AddCategory(Special)(<< "CPU[" << aCPU << "] Load.UpdatedGlobalWithDMA " << anAddress
                                      << " Now: " << iter->second));
        }

        ValueTrack::local_values::iterator local = iter->second.theLocallyVisibleValues.find(aCPU);
        if (local == iter->second.theLocallyVisibleValues.end()) {
            if (iter->second.theSimicsReflectsCPU != (bits)(kSimicsReflectsG)) {
                // Change simics to reflect the globally visible value
                // cpu.writePAddr( aligned, 8, iter->second.theGloballyVisibleValue );
                iter->second.theSimicsReflectsCPU = kSimicsReflectsG;

                bits val = cpu.read_pa(anAddress, aSize);
                DBG_(VVerb,
                     AddCategory(Special)(<< "CPU[" << aCPU << "] Load.SwitchToGlobalValue " << anAddress << "["
                                          << aSize << "] = " << std::hex << val << std::dec));
                return bits(val);
            } else {
                bits val = cpu.read_pa(anAddress, aSize);
                DBG_(VVerb,
                     (<< "CPU[" << aCPU << "] Load.SimicsAlreadySetToGlobal " << anAddress << "[" << aSize
                      << "] = " << std::hex << val << std::dec));
                return bits(val);
            }
        }

        if (iter->second.theSimicsReflectsCPU != static_cast<bits>(aCPU)) {

            // Change simics to reflect the value for this CPU
            // cpu.writePAddr( aligned, 8, local->second.theValue );
            iter->second.theSimicsReflectsCPU = aCPU;

            bits val = cpu.read_pa(anAddress, aSize);
            DBG_(VVerb,
                 AddCategory(Special)(<< "CPU[" << aCPU << "] Load.SwitchToLocalValue " << anAddress << "[" << aSize
                                      << "] = " << std::hex << val << std::dec));
            return bits(val);
        } else {

            bits val = cpu.read_pa(anAddress, aSize);
            DBG_(VVerb,
                 (<< "CPU[" << aCPU << "] Load.SimicsAlreadySetToLocal " << anAddress << "[" << aSize
                  << "] = " << std::hex << val << std::dec));
            return bits(val);
        }
    }

    void invalidate(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        DBG_(VVerb, (<< "VT invalidating addr:" << anAddress << " size:" << aSize));
        PhysicalMemoryAddress addr = anAddress;

        for (int32_t i = 0; i < aSize; i++) {
            PhysicalMemoryAddress aligned = dwAddr(addr);
            tracker::iterator iter        = theTracker.find(aligned);

            if (iter != theTracker.end()) {
                Flexus::Qemu::Processor cpu = Flexus::Qemu::Processor::getProcessor(0);
                DBG_(VVerb,
                     (<< "VT invalidating entry due to DMA on " << aligned << " entry: " << iter->second
                      << " simics_value=" << cpu.read_pa(aligned, 8)));
                iter->second.theGloballyVisibleValue = 0xbaadf00d;
                iter->second.theSimicsReflectsCPU    = kPoisonedByDMA;
            }
            addr += 1;
        }
    }

  private:
    // Value Manipulation
    //==========================================================================
    PhysicalMemoryAddress dwAddr(PhysicalMemoryAddress anAddress) { return PhysicalMemoryAddress(anAddress & ~7); }

    uint32_t offset(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        return (16 - aSize) - (static_cast<uint64_t>(anAddress) - dwAddr(anAddress));
    }

    bits makeMask(PhysicalMemoryAddress anAddress, eSize aSize)
    {
        bits mask = nuArch::mask(aSize);
        mask <<= (offset(anAddress, aSize) * 8);
        return mask;
    }

    bits align(bits aValue, PhysicalMemoryAddress anAddress, eSize aSize)
    {
        return (aValue << (offset(anAddress, aSize) * 8));
    }

    bits overlay(bits anOriginalValue, PhysicalMemoryAddress anAddress, eSize aSize, bits aValue)
    {
        bits available      = makeMask(anAddress,
                                  aSize);                 // Determine what part of the int64_t the update covers
        bits update_aligned = align(aValue, anAddress, aSize); // align the update value for computation
        anOriginalValue &= (~available);                       // zero out the overlap region in the original value
        anOriginalValue |= (update_aligned & available);       // paste the update value into the overlap
        return anOriginalValue;
    }
};

} // namespace nuArch

#endif // FLEXUS_UARCH_VALUETRACKER_HPP_INCLUDED
