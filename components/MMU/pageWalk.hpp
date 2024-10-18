
#ifndef FLEXUS_PAGEWALK_HPP_INCLUDED
#define FLEXUS_PAGEWALK_HPP_INCLUDED

#include "MMUUtil.hpp"

#include <components/CommonQEMU/Translation.hpp>
#include <components/CommonQEMU/Transports/TranslationTransport.hpp>
#include <core/types.hpp>

using namespace Flexus::SharedTypes;

namespace nMMU {

class MMUComponent;

class PageWalk
{

    std::shared_ptr<mmu_t> theMMU;

    std::list<TranslationTransport> theTranslationTransports;

    std::queue<boost::intrusive_ptr<Translation>> theDoneTranslations;
    std::queue<boost::intrusive_ptr<Translation>> theMemoryTranslations;

    PhysicalMemoryAddress trace_address;

    bool TheInitialized;

  public:
    PageWalk(uint32_t aNode, MMUComponent *aMMU)
      : TheInitialized(false)
      , mmu(aMMU)
      , theNode(aNode)
    {
    }
    ~PageWalk() {}
    void setMMU(std::shared_ptr<mmu_t> aMMU) { theMMU = aMMU; }
    void translate(TranslationTransport& aTransport);
    void preTranslate(TranslationTransport& aTransport);
    void cycle();
    bool push_back(boost::intrusive_ptr<Translation> aTranslation);
    bool push_back_trace(boost::intrusive_ptr<Translation> aTranslation, Flexus::Qemu::Processor theCPU);
    TranslationPtr popMemoryRequest();
    bool hasMemoryRequest();
    void annulAll();

  private:
    void preWalk(TranslationTransport& aTranslation);
    bool walk(TranslationTransport& aTranslation);
    void setupTTResolver(TranslationTransport& aTr, uint64_t TTDescriptor);
    bool InitialTranslationSetup(TranslationTransport& aTranslation);
    void pushMemoryRequest(TranslationPtr aTranslation);

    TTEDescriptor getNextTTDescriptor(TranslationTransport& aTranslation);

    std::list <TranslationTransport> delay;
    MMUComponent *mmu;

    uint8_t currentEL();
    uint32_t currentPSTATE();

    uint32_t theNode;
};

} // end namespace nMMU

#endif // FLEXUS_PAGEWALK_HPP_INCLUDED
