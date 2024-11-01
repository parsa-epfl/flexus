#ifndef FLEXUS_MMUIMPL_HPP_INCLUDED
#define FLEXUS_MMUIMPL_HPP_INCLUDED

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

#define FLEXUS_BEGIN_COMPONENT MMU
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <core/checkpoint/json.hpp>
#include <core/stats.hpp>

namespace std {
template<>
struct hash<VirtualMemoryAddress>
{
    std::size_t operator()(const VirtualMemoryAddress& anAddress) const;
};
} // namespace std

namespace nMMU {
using namespace Flexus;
using namespace Qemu;
using namespace Core;
using namespace SharedTypes;
using json = nlohmann::json;
extern uint64_t PAGEMASK;

struct TLBentry
{

    TLBentry();

    TLBentry(VirtualMemoryAddress aVAddress,
             PhysicalMemoryAddress aPaddress,
             uint64_t aRate,
             uint16_t anASID,
             bool aNG);

    TLBentry(VirtualMemoryAddress anAddress, uint16_t anASID);
    uint64_t theRate;
    VirtualMemoryAddress theVaddr;
    PhysicalMemoryAddress thePaddr;
    uint16_t theASID; // Address Space Identifier
    bool thenG;       // non-Global bit
};

struct TLB
{
    void loadState(json checkpoint);
    json saveState();
    std::pair<bool, PhysicalMemoryAddress> lookUp(TranslationPtr& tr);
    void insert(TranslationPtr& tr);
    void resize(size_t set, size_t associativity);
    size_t capacity();
    void clear();
    void clearFaultyEntry();
    size_t size();

  private:
    void evict(size_t which_set);

    size_t theAssociativity;
    size_t theSets;

    boost::optional<TLBentry> faultyEntry;
    std::vector<std::unordered_multimap<VirtualMemoryAddress, TLBentry>> theTLB;
    typedef std::unordered_multimap<VirtualMemoryAddress, TLBentry>::iterator tlbIterator;
};

class FLEXUS_COMPONENT(MMU)
{
  public:
    TLB theInstrTLB;
    TLB theDataTLB;
    TLB theSecondTLB;

  private:
    FLEXUS_COMPONENT_IMPL(MMU);

    std::unique_ptr<PageWalk> thePageWalker;

    std::queue<boost::intrusive_ptr<Translation>> theLookUpEntries;
    std::queue<boost::intrusive_ptr<Translation>> thePageWalkEntries;
    std::set<VirtualMemoryAddress> alreadyPW;
    std::vector<boost::intrusive_ptr<Translation>> standingEntries;

    Flexus::Qemu::Processor theCPU;
    std::shared_ptr<mmu_t> theMMU;

  public:
    Stat::StatCounter itlb_accesses;
    Stat::StatCounter dtlb_accesses;
    Stat::StatCounter stlb_accesses;
    Stat::StatCounter itlb_misses;
    Stat::StatCounter dtlb_misses;
    Stat::StatCounter stlb_misses;

  private:
    // Is true when the MMU has been reseted
    bool mmu_is_init;
    bool cfg_mmu(index_t anIndex);

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(MMU);

    uint16_t getASID();

    bool isQuiesced() const;

    void saveState(std::string const& dirname);

    void loadState(std::string const& dirname);

    // Initialization
    void initialize();

    void finalize();

    // MMUDrive
    //----------
    void drive(interface::MMUDrive const&);

    void busCycle();

    void processMemoryRequests();

    void resyncMMU(int anIndex);

    bool IsTranslationEnabledAtEL(uint8_t& anEL);

    bool available(interface::ResyncIn const&, index_t anIndex);
    void push(interface::ResyncIn const&, index_t anIndex, int& aResync);

    bool available(interface::iRequestIn const&, index_t anIndex);
    void push(interface::iRequestIn const&, index_t anIndex, TranslationPtr& aTranslate);

    bool available(interface::dRequestIn const&, index_t anIndex);
    void push(interface::dRequestIn const&, index_t anIndex, TranslationPtr& aTranslate);

    void sendTLBresponse(TranslationPtr aTranslation);

    bool available(interface::TLBReqIn const&, index_t anIndex);
    void push(interface::TLBReqIn const&, index_t anIndex, TranslationPtr& aTranslate);
};
}
#endif
