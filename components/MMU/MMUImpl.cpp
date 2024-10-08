
// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info

#include "MMUUtil.hpp"
#include "pageWalk.hpp"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <components/CommonQEMU/Translation.hpp>
#include <components/MMU/MMU.hpp>
#include <core/performance/profile.hpp>
#include <core/qemu/configuration_api.hpp>
#include <fstream>
#include <iostream>

#define DBG_DefineCategories MMU
#define DBG_SetDefaultOps    AddCat(MMU)
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <core/checkpoint/json.hpp>

using json = nlohmann::json;

namespace std {
template<>
struct hash<VirtualMemoryAddress>
{
    std::size_t operator()(const VirtualMemoryAddress& anAddress) const
    {
        return ((hash<uint64_t>()(uint64_t(anAddress))));
    }
};
} // namespace std

namespace nMMU {

using namespace Flexus;
using namespace Qemu;
using namespace Core;
using namespace SharedTypes;
uint64_t PAGEMASK;

class FLEXUS_COMPONENT(MMU)
{
    FLEXUS_COMPONENT_IMPL(MMU);

  private:
    struct TLBentry
    {

        TLBentry() {}

        TLBentry(VirtualMemoryAddress aVAddress, PhysicalMemoryAddress aPaddress, uint64_t aRate)
          : theRate(aRate)
          , theVaddr(aVAddress)
          , thePaddr(aPaddress)
        {
        }

        TLBentry(VirtualMemoryAddress anAddress)
          : theRate(0)
          , theVaddr(anAddress)
        {
        }
        uint64_t theRate;
        VirtualMemoryAddress theVaddr;
        PhysicalMemoryAddress thePaddr;

        bool operator==(const TLBentry& other) const { return (theVaddr == other.theVaddr); }

        TLBentry& operator=(const PhysicalMemoryAddress& other)
        {
            PhysicalMemoryAddress otherAligned(other & PAGEMASK);
            thePaddr = otherAligned;
            return *this;
        }
    };

    struct TLB
    {

        void loadState(json checkpoint)
        {
            size_t size = checkpoint["capacity"];

            theTLB.clear();

            if (size != theSize) { resize(size); }

            size_t TLBSize = checkpoint["entries"].size();
            for (size_t i = 0; i < TLBSize; i++) {
                VirtualMemoryAddress aVaddr  = VirtualMemoryAddress((uint64_t)checkpoint["entries"].at(i)["vpn"]);
                PhysicalMemoryAddress aPaddr = PhysicalMemoryAddress((uint64_t)checkpoint["entries"].at(i)["ppn"]);
                uint64_t index               = (uint64_t)(TLBSize - i - 1);
                theTLB.insert({ aVaddr, TLBentry(aVaddr, aPaddr, index) });
                DBG_(Dev, (<< "Inserting TLB line with" << aVaddr << " " << aPaddr << "at index: [" << index << "]"));
            }
        }

        json saveState()
        {

            json checkpoint;

            checkpoint["capacity"] = theSize;

            // sorting TLB entries for LRU
            std::vector<TLBentry> entries;
            for (const auto& pair : theTLB) {
                entries.push_back(pair.second);
            }
            std::sort(entries.begin(), entries.end(), [](const TLBentry& a, const TLBentry& b) {
                return b.theRate < a.theRate;
            });

            // saving the ordered entries
            checkpoint["entries"] = json::array();
            size_t i              = 0;
            for (const auto& entry : entries) {
                checkpoint["entries"][i++] = { { "vpn", (uint64_t)entry.theVaddr },
                                               { "ppn", (uint64_t)entry.thePaddr } };
            }

            return checkpoint;
        }

        std::pair<bool, PhysicalMemoryAddress> lookUp(const VirtualMemoryAddress& anAddress)
        {
            VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
            std::pair<bool, PhysicalMemoryAddress> ret{ false, PhysicalMemoryAddress(0) };
            for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
                iter->second.theRate++;
                if (iter->second.theVaddr == anAddressAligned) {
                    iter->second.theRate = 0;
                    ret.first            = true;
                    ret.second           = iter->second.thePaddr;
                }
            }
            return ret;
        }

        TLBentry& operator[](VirtualMemoryAddress anAddress)
        {
            VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
            auto iter = theTLB.find(anAddressAligned);
            if (iter == theTLB.end()) {
                size_t s = theTLB.size();
                if (s == theSize) { evict(); }
                std::pair<tlbIterator, bool> result;
                result = theTLB.insert({ anAddressAligned, TLBentry(anAddressAligned) });
                assert(result.second);
                iter = result.first;
            }
            return iter->second;
        }

        void resize(size_t aSize)
        {
            theTLB.reserve(aSize);
            theSize = aSize;
        }

        size_t capacity() { return theSize; }

        void clear() { theTLB.clear(); }

        size_t size() { return theTLB.size(); }

      private:
        void evict()
        {
            auto res = theTLB.begin();
            for (auto iter = theTLB.begin(); iter != theTLB.end(); ++iter) {
                if (iter->second.theRate > res->second.theRate) { res = iter; }
            }
            theTLB.erase(res);
        }

        size_t theSize;
        std::unordered_map<VirtualMemoryAddress, TLBentry> theTLB;
        typedef std::unordered_map<VirtualMemoryAddress, TLBentry>::iterator tlbIterator;
    };

    std::unique_ptr<PageWalk> thePageWalker;
    TLB theInstrTLB;
    TLB theDataTLB;

    std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
    std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
    std::set<VirtualMemoryAddress> alreadyPW;
    std::vector<boost::intrusive_ptr<Translation>> standingEntries;

    Flexus::Qemu::Processor theCPU;
    std::shared_ptr<mmu_t> theMMU;

    // Is true when the MMU has been reseted
    bool mmu_is_init;

  private:
    bool cfg_mmu(index_t anIndex)
    {
        bool ret = false;
        theMMU.reset(new mmu_t());

        if (theMMU->init_mmu_regs(anIndex)) {
            theMMU->setupAddressSpaceSizesAndGranules();
            DBG_Assert(theMMU->Gran0->getlogKBSize() == 12, (<< "TG0 has non-4KB size - unsupported"));
            DBG_Assert(theMMU->Gran1->getlogKBSize() == 12, (<< "TG1 has non-4KB size - unsupported"));
            PAGEMASK = ~((1 << theMMU->Gran0->getlogKBSize()) - 1);
            ret      = true;
        }

        return ret;
    }

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MMU)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const { return false; }

    void saveState(std::string const& dirname)
    {

        std::ofstream iFile, dFile;

        iFile.open(dirname + "/" + statName() + "-itlb.json", std::ofstream::out);
        json iCheckpoint = theInstrTLB.saveState();
        iFile << std::setw(4) << iCheckpoint << std::endl;
        iFile.close();

        dFile.open(dirname + "/" + statName() + "-dtlb.json", std::ofstream::out);
        json dCheckpoint = theDataTLB.saveState();
        dFile << std::setw(4) << dCheckpoint << std::endl;
        dFile.close();
    }

    void loadState(std::string const& dirname)
    {
        json iCheckpoint, dCheckpoint;

        std::string itlb_ckpt_file = dirname + "/" + statName() + "-itlb.json";

        DBG_(Dev, (<< "Loading iTLB checkpoint form:" << itlb_ckpt_file));
        std::ifstream itlb_s(itlb_ckpt_file, std::ifstream::in);

        if (!itlb_s.good()) {
            DBG_(Dev, (<< "checkpoint file: " << itlb_ckpt_file << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        itlb_s >> iCheckpoint;
        theInstrTLB.loadState(iCheckpoint);
        itlb_s.close();

        std::string dtlb_ckpt_file = dirname + "/" + statName() + "-dtlb.json";
        DBG_(Dev, (<< "Loading dTLB checkpoint form:" << dtlb_ckpt_file));
        std::ifstream dtlb_s(dtlb_ckpt_file, std::ifstream::in);

        if (!dtlb_s.good()) {
            DBG_(Dev, (<< "checkpoint file: " << itlb_ckpt_file << " not found."));
            DBG_Assert(false, (<< "FILE NOT FOUND"));
        }

        dtlb_s >> dCheckpoint;
        theDataTLB.loadState(dCheckpoint);
        dtlb_s.close();
    }

    // Initialization
    void initialize()
    {
        theCPU = Flexus::Qemu::Processor::getProcessor(flexusIndex());
        thePageWalker.reset(new PageWalk(flexusIndex()));
        thePageWalker->setMMU(theMMU);
        mmu_is_init = false;
        theInstrTLB.resize(cfg.iTLBSize);
        theDataTLB.resize(cfg.dTLBSize);
    }

    void finalize() {}

    // MMUDrive
    //----------
    void drive(interface::MMUDrive const&)
    {
        DBG_(VVerb, Comp(*this)(<< "MMUDrive"));
        busCycle();
        thePageWalker->cycle();
        processMemoryRequests();
    }

    void busCycle()
    {

        while (!theLookUpEntries.empty()) {

            TranslationPtr item = theLookUpEntries.front();
            theLookUpEntries.pop();
            DBG_(VVerb, (<< "Processing lookup entry for " << item->theVaddr));
            DBG_Assert(item->isInstr() != item->isData());

            DBG_(VVerb, (<< "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr));

            std::pair<bool, PhysicalMemoryAddress> entry =
              (item->isInstr() ? theInstrTLB : theDataTLB).lookUp((VirtualMemoryAddress)(item->theVaddr));
            if (cfg.PerfectTLB || !mmu_is_init) {
                PhysicalMemoryAddress perfectPaddr(API::qemu_api.translate_va2pa(flexusIndex(), item->theVaddr));
                entry.first  = true;
                entry.second = perfectPaddr;
                if (perfectPaddr == 0xFFFFFFFFFFFFFFFF) item->setPagefault();
            }
            if (entry.first) {
                DBG_(VVerb, (<< "Item is a Hit " << item->theVaddr));

                // item exists so mark hit
                item->setHit();
                item->thePaddr = (PhysicalMemoryAddress)(entry.second | (item->theVaddr & ~(PAGEMASK)));

                if (item->isInstr())
                    FLEXUS_CHANNEL(iTranslationReply) << item;
                else
                    FLEXUS_CHANNEL(dTranslationReply) << item;
            } else {
                DBG_(VVerb, (<< "Item is a miss " << item->theVaddr));

                VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
                if (alreadyPW.find(pageAddr) == alreadyPW.end()) {
                    // mark miss
                    item->setMiss();
                    if (thePageWalker->push_back(item)) {
                        alreadyPW.insert(pageAddr);
                        thePageWalkEntries.push(item);
                    } else {
                        PhysicalMemoryAddress perfectPaddr(
                          API::qemu_api.translate_va2pa(flexusIndex(), item->theVaddr));
                        item->setHit();
                        item->thePaddr = perfectPaddr;
                        if (item->isInstr())
                            FLEXUS_CHANNEL(iTranslationReply) << item;
                        else
                            FLEXUS_CHANNEL(dTranslationReply) << item;
                    }
                } else {
                    standingEntries.push_back(item);
                }
            }
        }

        while (!thePageWalkEntries.empty()) {

            TranslationPtr item = thePageWalkEntries.front();
            DBG_(VVerb, (<< "Processing PW entry for " << item->theVaddr));

            if (item->isAnnul()) {
                DBG_(VVerb, (<< "Item was annulled " << item->theVaddr));
                thePageWalkEntries.pop();
            } else if (item->isDone()) {
                DBG_(VVerb, (<< "Item was Done translationg " << item->theVaddr));

                CORE_DBG("MMU: Translation is Done " << item->theVaddr << " -- " << item->thePaddr);

                thePageWalkEntries.pop();

                VirtualMemoryAddress pageAddr(item->theVaddr & PAGEMASK);
                if (alreadyPW.find(pageAddr) != alreadyPW.end()) {
                    alreadyPW.erase(pageAddr);
                    for (auto it = standingEntries.begin(); it != standingEntries.end();) {
                        if (((*it)->theVaddr & pageAddr) == pageAddr && item->isInstr() == (*it)->isInstr()) {
                            theLookUpEntries.push(*it);
                            standingEntries.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                DBG_Assert(item->isInstr() != item->isData());
                DBG_(Iface,
                     (<< "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr));
                // update TLB unless it is a pagefault
                if (!item->isPagefault())
                    (item->isInstr() ? theInstrTLB : theDataTLB)[(VirtualMemoryAddress)(item->theVaddr)] =
                      (PhysicalMemoryAddress)(item->thePaddr);
                if (item->isInstr())
                    FLEXUS_CHANNEL(iTranslationReply) << item;
                else
                    FLEXUS_CHANNEL(dTranslationReply) << item;
            } else {
                break;
            }
        }
    }

    void processMemoryRequests()
    {
        CORE_TRACE;
        while (thePageWalker->hasMemoryRequest()) {
            TranslationPtr tmp = thePageWalker->popMemoryRequest();
            DBG_(VVerb,
                 (<< "Sending a Memory Translation request to Core ready(" << tmp->isReady() << ")  " << tmp->theVaddr
                  << " -- " << tmp->thePaddr << "  -- ID " << tmp->theID));
            FLEXUS_CHANNEL(MemoryRequestOut) << tmp;
        }
    }

    void resyncMMU(int anIndex)
    {
        CORE_TRACE;
        DBG_(VVerb, (<< "Resynchronizing MMU"));

        mmu_is_init = cfg_mmu(anIndex);

        if (thePageWalker) {
            DBG_(VVerb, (<< "Annulling all PW entries"));
            thePageWalker->annulAll();
        }

        if (thePageWalkEntries.size() > 0) {
            DBG_(VVerb, (<< "deleting PageWalk Entries"));
            while (!thePageWalkEntries.empty()) {
                DBG_(VVerb, (<< "deleting MMU PageWalk Entrie " << thePageWalkEntries.front()));
                thePageWalkEntries.pop();
            }
        }

        alreadyPW.clear();
        standingEntries.clear();

        if (theLookUpEntries.size() > 0) {
            while (!theLookUpEntries.empty()) {
                DBG_(VVerb, (<< "deleting MMU Lookup Entrie " << theLookUpEntries.front()));
                theLookUpEntries.pop();
            }
        }
        FLEXUS_CHANNEL(ResyncOut) << anIndex;
    }

    bool IsTranslationEnabledAtEL(uint8_t& anEL)
    {
        return true; // theCore->IsTranslationEnabledAtEL(anEL);
    }

    bool available(interface::ResyncIn const&, index_t anIndex) { return true; }
    void push(interface::ResyncIn const&, index_t anIndex, int& aResync)
    {

        if (cfg.PerfectTLB) return;

        resyncMMU(aResync);
    }

    bool available(interface::iRequestIn const&, index_t anIndex) { return true; }
    void push(interface::iRequestIn const&, index_t anIndex, TranslationPtr& aTranslate)
    {
        CORE_DBG("MMU: Instruction RequestIn");

        aTranslate->theIndex = anIndex;
        aTranslate->toggleReady();
        theLookUpEntries.push(aTranslate);
    }

    bool available(interface::dRequestIn const&, index_t anIndex) { return true; }
    void push(interface::dRequestIn const&, index_t anIndex, TranslationPtr& aTranslate)
    {
        CORE_DBG("MMU: Data RequestIn");

        aTranslate->theIndex = anIndex;

        aTranslate->toggleReady();
        theLookUpEntries.push(aTranslate);
    }

    void sendTLBresponse(TranslationPtr aTranslation)
    {
        if (aTranslation->isInstr()) {
            FLEXUS_CHANNEL(iTranslationReply) << aTranslation;
        } else {
            FLEXUS_CHANNEL(dTranslationReply) << aTranslation;
        }
    }

    bool available(interface::TLBReqIn const&, index_t anIndex) { return true; }
    void push(interface::TLBReqIn const&, index_t anIndex, TranslationPtr& aTranslate)
    {
        if (cfg.PerfectTLB) return;
        if ((aTranslate->isInstr() ? theInstrTLB : theDataTLB).lookUp(aTranslate->theVaddr).first) return;

        if (!mmu_is_init) mmu_is_init = cfg_mmu(anIndex);
        if (!mmu_is_init) return;

        thePageWalker->setMMU(theMMU);

        thePageWalker->push_back_trace(aTranslate, Flexus::Qemu::Processor::getProcessor(flexusIndex()));
        (aTranslate->isInstr() ? theInstrTLB : theDataTLB)[aTranslate->theVaddr] = aTranslate->thePaddr;
    }
};

} // End Namespace nMMU

FLEXUS_COMPONENT_INSTANTIATOR(MMU, nMMU);
FLEXUS_PORT_ARRAY_WIDTH(MMU, dRequestIn)
{
    return 1;
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iRequestIn)
{
    return 1;
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, ResyncIn)
{
    return 1;
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, iTranslationReply)
{
    return 1;
}
FLEXUS_PORT_ARRAY_WIDTH(MMU, dTranslationReply)
{
    return 1;
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, MemoryRequestOut)
{
    return 1;
}

FLEXUS_PORT_ARRAY_WIDTH(MMU, TLBReqIn)
{
    return 1;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MMU

#define DBG_Reset
#include DBG_Control()
