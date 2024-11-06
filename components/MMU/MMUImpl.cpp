
// Changelog:
//  - June'18: msutherl - basic TLB definition, no real timing info
#include "MMUImpl.hpp"
#include <fstream>
#include <iostream>

#define DBG_DefineCategories MMU
#define DBG_SetDefaultOps    AddCat(MMU)
#include DBG_Control()
#define DBG_DefineCategories TLBMissTracking
#include DBG_Control()

using json = nlohmann::json;

namespace std {
std::size_t hash<VirtualMemoryAddress>::operator()(const VirtualMemoryAddress& anAddress) const
{
    return ((hash<uint64_t>()(uint64_t(anAddress))));
}
} // namespace std

namespace nMMU {

using namespace Flexus;
using namespace Qemu;
using namespace Core;
using namespace SharedTypes;
uint64_t PAGEMASK;


TLBentry::TLBentry() {}

TLBentry::TLBentry(VirtualMemoryAddress aVAddress,
            PhysicalMemoryAddress aPaddress,
            uint64_t aRate,
            uint16_t anASID,
            bool aNG)
    : theRate(aRate)
    , theVaddr(aVAddress)
    , thePaddr(aPaddress)
    , theASID(anASID)
    , thenG(aNG)
{
}

TLBentry::TLBentry(VirtualMemoryAddress anAddress, uint16_t anASID)
    : theRate(0)
    , theVaddr(anAddress)
    , theASID(anASID)
{
}

void TLB::loadState(json checkpoint)
{
    size_t associativity = checkpoint["associativity"];
    size_t set = checkpoint["entries"].size();

    clear();

    if (associativity != theAssociativity || set != theSets) {
        DBG_Assert(false, (<< "TLB size mismatch: Expected " << theSets << " sets and " << theAssociativity << " associativity, got " << set << " sets and " << associativity << " associativity"));
    }

    for (size_t i = 0; i < set; ++i) {
        theTLB.push_back(std::unordered_multimap<VirtualMemoryAddress, TLBentry>());

        // get each entry in the set.
        size_t TLBSize = checkpoint["entries"][i].size();

        for (size_t j = 0; j < TLBSize; j++) {
            VirtualMemoryAddress aVaddr  = VirtualMemoryAddress(static_cast<uint64_t>(checkpoint["entries"][i].at(j)["vpn"]) << 12);
            PhysicalMemoryAddress aPaddr = PhysicalMemoryAddress(static_cast<uint64_t>(checkpoint["entries"][i].at(j)["ppn"]) << 12);
            uint16_t anASID              = static_cast<uint16_t>(checkpoint["entries"][i].at(j)["asid"]);
            bool aNG                     = static_cast<bool>(checkpoint["entries"][i].at(j)["ng"]);
            uint64_t index               = static_cast<uint64_t>(TLBSize - j - 1);
            theTLB[i].insert({ aVaddr, TLBentry(aVaddr, aPaddr, index, anASID, aNG) });
            DBG_(Dev, (<< "Inserting TLB line with" << aVaddr << " " << aPaddr << "at index: [" << index << "]"));
        }
    }
}

json TLB::saveState()
{

    json checkpoint;

    checkpoint["associativity"] = theAssociativity;
    checkpoint["entries"] = json::array();
    for (size_t set_idx = 0; set_idx < theSets; ++set_idx) {
        std::vector<TLBentry> entries;
        for (const auto& pair : theTLB[set_idx]) {
            entries.push_back(pair.second);
        }
        std::sort(entries.begin(), entries.end(), [](const TLBentry& a, const TLBentry& b) {
            return b.theRate < a.theRate;
        });

        checkpoint["entries"][set_idx] = json::array();
        size_t i = 0;
        for (const auto& entry : entries) {
            checkpoint["entries"][set_idx][i++] = { { "vpn", static_cast<uint64_t>(entry.theVaddr) >> 12 },
                                                    { "ppn", static_cast<uint64_t>(entry.thePaddr) >> 12 },
                                                    { "asid", static_cast<uint16_t>(entry.theASID) },
                                                    { "ng", static_cast<bool>(entry.thenG) } };
        }
    }

    return checkpoint;
}

std::pair<bool, PhysicalMemoryAddress> TLB::lookUp(TranslationPtr &tr)
{
    VirtualMemoryAddress anAddress = tr->theVaddr;
    uint16_t anASID = tr->theASID;
    VirtualMemoryAddress anAddressAligned(anAddress & PAGEMASK);
    // Find the set.
    size_t set_idx = anAddressAligned & (theSets - 1);
    std::pair<bool, PhysicalMemoryAddress> ret{ false, PhysicalMemoryAddress(0) };
    for (auto iter = theTLB[set_idx].begin(); iter != theTLB[set_idx].end(); ++iter) {
        iter->second.theRate++;
        if (iter->second.theVaddr == anAddressAligned &&
            (anASID == iter->second.theASID || !iter->second.thenG)) {
            iter->second.theRate = 0;
            ret.first            = true;
            ret.second           = iter->second.thePaddr;
        }
    }
    // check if the entry is faulty
    if (faultyEntry && faultyEntry->theVaddr == anAddressAligned &&
        (anASID == faultyEntry->theASID || !faultyEntry->thenG)) {
        ret.first  = true;
        ret.second = faultyEntry->thePaddr;
    }
    return ret;
}

void TLB::insert(TranslationPtr &tr)
{
    bool aNG = tr->theNG;
    uint16_t anASID = tr->theASID;
    VirtualMemoryAddress alignedVirtualAddr(tr->theVaddr & PAGEMASK);
    PhysicalMemoryAddress alignedPhysicalAddr(tr->thePaddr & PAGEMASK);
    if (tr->isPagefault()) {
        if (tr->inTraceMode)
            return;
        faultyEntry = TLBentry(alignedVirtualAddr, alignedPhysicalAddr, 0, anASID, aNG);
        return;
    }
    size_t set_idx = alignedVirtualAddr & (theSets - 1);
    // Check if the virtual address is in TLB (with the same ASID or as a global entry)
    auto iter  = theTLB[set_idx].end();
    auto range = theTLB[set_idx].equal_range(alignedVirtualAddr);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.theASID == anASID || !it->second.thenG) {
            iter = it;
            break;
        }
    }
    // If the virtual address is not in TLB, insert it
    if (iter == theTLB[set_idx].end()) {
        size_t s = theTLB[set_idx].size();
        if (s == theAssociativity) { evict(set_idx); }
        iter = theTLB[set_idx].insert({ alignedVirtualAddr, TLBentry(alignedVirtualAddr, anASID) });
    }
    // update TLB entry
    iter->second.thePaddr = alignedPhysicalAddr;
    iter->second.thenG    = aNG;
    return;
}

void TLB::resize(size_t associativity, size_t set)
{
    theAssociativity = associativity;
    theSets = set;
    theTLB.clear();
    theTLB.resize(set);
}


void TLB::clear() {
    theTLB.clear();
    clearFaultyEntry();
}

void TLB::clearFaultyEntry() { faultyEntry = boost::none; }


void TLB::evict(size_t which_set)
{
    auto res = theTLB[which_set].begin();
    for (auto iter = theTLB[which_set].begin(); iter != theTLB[which_set].end(); ++iter) {
        if (iter->second.theRate > res->second.theRate) { res = iter; }
    }
    theTLB[which_set].erase(res);
}

bool MMUComponent::cfg_mmu(index_t anIndex)
{
    bool ret = false;
    theMMU.reset(new mmu_t());

    if (theMMU->init_mmu_regs(anIndex)) {
        theMMU->setupAddressSpaceSizesAndGranules();
        DBG_Assert(theMMU->Gran0->getlogKBSize() == 12, (<< "TG0 has non-4KB size - unsupported"));
        DBG_Assert(theMMU->Gran1->getlogKBSize() == 12, (<< "TG1 has non-4KB size - unsupported"));
        PAGEMASK = ~((1ULL << theMMU->Gran0->getlogKBSize()) - 1);
        ret      = true;
    }

    return ret;
}

MMUComponent::FLEXUS_COMPONENT_CONSTRUCTOR(MMU)
    : base(FLEXUS_PASS_CONSTRUCTOR_ARGS), itlb_accesses(statName() + "-itlb_accesses"),
    dtlb_accesses(statName() + "-dtlb_accesses"), stlb_accesses(statName() + "-stlb_accesses"),
    itlb_misses(statName() + "-itlb_misses"), dtlb_misses(statName() + "-dtlb_misses"),
    stlb_misses(statName() + "-stlb_misses")
{
}

uint16_t MMUComponent::getASID()
{
    uint16_t ASID;
    auto TCR_EL1 = theMMU->mmu_regs.TCR[EL1];
    auto A1bit   = extract64(TCR_EL1, 22, 1);
    if (A1bit) {
        // TTBR1_EL1.ASID defines the ASID.
        auto TTBR1_EL1 = theMMU->mmu_regs.TTBR1[EL1];
        ASID           = extract64(TTBR1_EL1, 48, 16);
    } else {
        // TTBR0_EL1.ASID defines the ASID.
        auto TTBR0_EL1 = theMMU->mmu_regs.TTBR0[EL1];
        ASID           = extract64(TTBR0_EL1, 48, 16);
    }
    return ASID;
}

bool MMUComponent::isQuiesced() const { return false; }

void MMUComponent::saveState(std::string const& dirname)
{

    std::ofstream iFile, dFile, sFile;

    iFile.open(dirname + "/" + statName() + "-itlb.json", std::ofstream::out);
    json iCheckpoint = theInstrTLB.saveState();
    iFile << std::setw(4) << iCheckpoint << std::endl;
    iFile.close();

    dFile.open(dirname + "/" + statName() + "-dtlb.json", std::ofstream::out);
    json dCheckpoint = theDataTLB.saveState();
    dFile << std::setw(4) << dCheckpoint << std::endl;
    dFile.close();

    sFile.open(dirname + "/" + statName() + "-stlb.json", std::ofstream::out);
    json sCheckpoint = theSecondTLB.saveState();
    sFile << std::setw(4) << sCheckpoint << std::endl;
    sFile.close();
}

void MMUComponent::loadState(std::string const& dirname)
{
    json iCheckpoint, dCheckpoint, sCheckpoint;

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

    std::string stlb_ckpt_file = dirname + "/" + statName() + "-stlb.json";
    DBG_(Dev, (<< "Loading sTLB checkpoint form:" << stlb_ckpt_file));
    std::ifstream stlb_s(stlb_ckpt_file, std::ifstream::in);

    if (!stlb_s.good()) {
        DBG_(Dev, (<< "checkpoint file: " << itlb_ckpt_file << " not found"));
        DBG_Assert(false, (<< "FILE NOT FOUND"));
    }

    stlb_s >> sCheckpoint;
    theSecondTLB.loadState(sCheckpoint);
    stlb_s.close();
}

// Initialization
void MMUComponent::initialize()
{
    theCPU = Flexus::Qemu::Processor::getProcessor(flexusIndex());
    thePageWalker.reset(new PageWalk(flexusIndex(), this));
    mmu_is_init = false;

    theInstrTLB.resize(cfg.iTLBAssoc, cfg.iTLBSet);
    theDataTLB.resize(cfg.dTLBAssoc, cfg.dTLBSet);
    theSecondTLB.resize(cfg.sTLBAssoc, cfg.sTLBSet);

    if (cfg.PerfectTLB) {
        PAGEMASK = ~((1ULL << 12) - 1);
    }
}

void MMUComponent::finalize() {}

// MMUDrive
//----------
void MMUComponent::drive(interface::MMUDrive const&)
{
    DBG_(VVerb, Comp(*this)(<< "MMUDrive"));
    busCycle();
    thePageWalker->cycle();
    processMemoryRequests();
}

void MMUComponent::busCycle()
{

    while (!theLookUpEntries.empty()) {

        TranslationPtr item = theLookUpEntries.front();
        theLookUpEntries.pop();
        DBG_(VVerb, (<< "Processing lookup entry for " << item->theVaddr));
        DBG_Assert(item->isInstr() != item->isData());

        DBG_(VVerb, (<< "Item is " << (item->isInstr() ? "Instruction" : "Data") << " entry " << item->theVaddr));

        std::pair<bool, PhysicalMemoryAddress> entry =
            (item->isInstr() ? theInstrTLB : theDataTLB).lookUp(item);
        if (cfg.PerfectTLB || !mmu_is_init) {
            PhysicalMemoryAddress perfectPaddr(API::qemu_api.translate_va2pa(flexusIndex(), item->theVaddr));
            entry.first  = true;
            entry.second = perfectPaddr;
            if (perfectPaddr == 0xFFFFFFFFFFFFFFFF) item->setPagefault();
        }

        if (item->isInstr() && entry.first) {
            itlb_accesses++;
        } else if (entry.first) {
            dtlb_accesses++;
        }


        if (entry.first) {
            DBG_(VVerb, (<< "Item is a Hit " << item->theVaddr));

            // item exists so mark hit
            item->setHit();
            PhysicalMemoryAddress perfectPaddr(API::qemu_api.translate_va2pa(flexusIndex(), item->theVaddr));
            // item->thePaddr = (PhysicalMemoryAddress)(entry.second | (item->theVaddr & ~(PAGEMASK)));
            item->thePaddr = perfectPaddr;

            if (item->isInstr())
                FLEXUS_CHANNEL(iTranslationReply) << item;
            else
                FLEXUS_CHANNEL(dTranslationReply) << item;
        } else {
            DBG_(VVerb, (<< "Item is a miss " << item->theVaddr));


            if (item->isInstr()) {
                itlb_misses++;
            } else {
                dtlb_misses++;
            }


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
                    if (((*it)->theVaddr & PAGEMASK) == pageAddr) {
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
            // update TLB
            (item->isInstr() ? theInstrTLB : theDataTLB).insert(item);
            if (item->isInstr())
                FLEXUS_CHANNEL(iTranslationReply) << item;
            else
                FLEXUS_CHANNEL(dTranslationReply) << item;
        } else {
            break;
        }
    }
}

void MMUComponent::processMemoryRequests()
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

void MMUComponent::resyncMMU(int anIndex)
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

    theInstrTLB.clearFaultyEntry();
    theDataTLB.clearFaultyEntry();
    theSecondTLB.clearFaultyEntry();
    FLEXUS_CHANNEL(ResyncOut) << anIndex;
}

bool MMUComponent::IsTranslationEnabledAtEL(uint8_t& anEL)
{
    return true; // theCore->IsTranslationEnabledAtEL(anEL);
}

bool MMUComponent::available(interface::ResyncIn const&, index_t anIndex) { return true; }
void MMUComponent::push(interface::ResyncIn const&, index_t anIndex, int& aResync)
{

    if (cfg.PerfectTLB) return;

    resyncMMU(aResync);
}

bool MMUComponent::available(interface::iRequestIn const&, index_t anIndex) { return true; }
void MMUComponent::push(interface::iRequestIn const&, index_t anIndex, TranslationPtr& aTranslate)
{
    CORE_DBG("MMU: Instruction RequestIn");

    aTranslate->setASID(getASID());
    aTranslate->theIndex = anIndex;
    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
}

bool MMUComponent::available(interface::dRequestIn const&, index_t anIndex) { return true; }
void MMUComponent::push(interface::dRequestIn const&, index_t anIndex, TranslationPtr& aTranslate)
{
    CORE_DBG("MMU: Data RequestIn");

    aTranslate->setASID(getASID());
    aTranslate->theIndex = anIndex;

    aTranslate->toggleReady();
    theLookUpEntries.push(aTranslate);
}

void MMUComponent::sendTLBresponse(TranslationPtr aTranslation)
{
    if (aTranslation->isInstr()) {
        FLEXUS_CHANNEL(iTranslationReply) << aTranslation;
    } else {
        FLEXUS_CHANNEL(dTranslationReply) << aTranslation;
    }
}

bool MMUComponent::available(interface::TLBReqIn const&, index_t anIndex) { return true; }
void MMUComponent::push(interface::TLBReqIn const&, index_t anIndex, TranslationPtr& aTranslate)
{
    aTranslate->setASID(getASID());
    if (cfg.PerfectTLB) return;
    if ((aTranslate->isInstr() ? theInstrTLB : theDataTLB).lookUp(aTranslate).first) return;

    if (!mmu_is_init) mmu_is_init = cfg_mmu(anIndex);
    if (!mmu_is_init) return;

    thePageWalker->push_back_trace(aTranslate, Flexus::Qemu::Processor::getProcessor(flexusIndex()));
    (aTranslate->isInstr() ? theInstrTLB : theDataTLB).insert(aTranslate);
}

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
