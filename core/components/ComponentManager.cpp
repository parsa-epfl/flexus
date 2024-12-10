#include <algorithm>
#include <core/component.hpp>
#include <core/debug/debug.hpp>
#include <functional>
#include <iostream>
#include <vector>

namespace Flexus {
namespace Wiring {
bool
connectWiring();
}
} // namespace Flexus

namespace Flexus {
namespace Core {
namespace aux_ {

class ComponentManagerImpl : public ComponentManager
{

    typedef std::vector<std::function<void(Flexus::Core::index_t)>> instatiation_vector;
    std::vector<std::function<void(Flexus::Core::index_t aSystemWidth)>> theInstantiationFunctions;
    std::vector<ComponentInterface*> theComponents;
    Flexus::Core::index_t theSystemWidth;
    Flexus::Core::freq_opts theFreq;

  private:
    // Helper functions to calculate drive frequencies
    index_t gcd(index_t a, index_t b) {
        while (b != 0) {
            int t = b;
            b     = a % b;
            a     = t;
        }
        return a;
    }

    std::tuple<index_t, index_t> helper(float number) {
        index_t num = (index_t)(number * 10);   // Assumption: number only has single digit after decimal point
        index_t den = 10;
        index_t g   = gcd(num, den);
        return std::make_tuple(num / g, den / g);
    }

  public:
    virtual ~ComponentManagerImpl() {}

    Flexus::Core::index_t systemWidth() const { return theSystemWidth; }
    Flexus::Core::freq_opts getFreq() const { return theFreq; }

    void registerHandle(std::function<void(Flexus::Core::index_t)> anInstantiator)
    {
        theInstantiationFunctions.push_back(anInstantiator);
    }

    void instantiateComponents(Flexus::Core::index_t aSystemWidth, float aFreqCore, float aFreqUncore)
    {
        theSystemWidth = aSystemWidth;
        DBG_(Dev, (<< "Instantiating system with a width factor of: " << aSystemWidth));
        Flexus::Wiring::connectWiring();
        instatiation_vector::iterator iter = theInstantiationFunctions.begin();
        instatiation_vector::iterator end  = theInstantiationFunctions.end();
        while (iter != end) {
            (*iter)(aSystemWidth);
            ++iter;
        }
        index_t coreNum, coreDen, uncoreNum, uncoreDen;
        std::tie(coreNum, coreDen) = helper(aFreqCore);
        std::tie(uncoreNum, uncoreDen) = helper(aFreqUncore);
        index_t lcmDen = (coreDen * uncoreDen) / gcd(coreDen, uncoreDen);
        theFreq.core   = (lcmDen / coreDen) * coreNum;
        theFreq.uncore = (lcmDen / uncoreDen) * uncoreNum;
        DBG_(Dev, (<< "[Ayan] Core frequency" << aFreqCore << " Uncore frequency: " << aFreqUncore));
        DBG_(Dev, (<< "[Ayan] Core Drive frequency: " << theFreq.core << " Uncore Drive frequency: " << theFreq.uncore));
    }

    void registerComponent(ComponentInterface* aComponent) { theComponents.push_back(aComponent); }

    void initComponents()
    {
        DBG_(Dev, (<< "Initializing " << theComponents.size() << " components..."));
        std::vector<ComponentInterface*>::iterator iter = theComponents.begin();
        std::vector<ComponentInterface*>::iterator end  = theComponents.end();
        int counter                                     = 1;
        while (iter != end) {
            DBG_(Dev, (<< "Component " << counter << ": Initializing " << (*iter)->name()));
            (*iter)->initialize();
            ++iter;
            ++counter;
        }
    }

    // added by PLotfi
    void finalizeComponents()
    {
        DBG_(Dev, (<< "Finalizing components..."));
        std::vector<ComponentInterface*>::iterator iter = theComponents.begin();
        std::vector<ComponentInterface*>::iterator end  = theComponents.end();
        while (iter != end) {
            DBG_(Dev, (<< "Finalizing " << (*iter)->name()));
            (*iter)->finalize();
            ++iter;
        }
    }
    // end PLotfi

    bool isQuiesced() const
    {
        bool quiesced = true;
        for (auto* aComponent : theComponents) {
            quiesced = quiesced && aComponent->isQuiesced();
        }
        // std::for_each
        // ( theComponents.begin()
        //   , theComponents.end()
        //   , ll::var(quiesced) = ll::var(quiesced) && ll::bind(
        //   &ComponentInterface::isQuiesced, ll::_1 )
        // );
        return quiesced;
    }

    void doSave(std::string const& aDirectory) const
    {
        for (auto* aComponent : theComponents) {
            aComponent->saveState(aDirectory);
        }
    }

    void doLoad(std::string const& aDirectory)
    {
        std::vector<ComponentInterface*>::iterator iter, end;
        iter = theComponents.begin();
        end  = theComponents.end();
        while (iter != end) {
            DBG_(Dev, (<< "Loading state: " << (*iter)->name()));
            (*iter)->loadState(aDirectory);
            ++iter;
        }
        DBG_(Crit, (<< " Done loading."));
        /*
              std::for_each
                ( theComponents.begin()
                , theComponents.end()
                , ll::bind( &ComponentInterface::loadState, ll::_1, aDirectory )
                );
        */
    }
};

} // namespace aux_

std::unique_ptr<aux_::ComponentManagerImpl> theComponentManager{};

ComponentManager&
ComponentManager::getComponentManager()
{
    if (theComponentManager == 0) {
        DBG_(Dev, (<< "Initializing Flexus::ComponentManager..."));
        theComponentManager.reset(new aux_::ComponentManagerImpl());
        DBG_(Dev, (<< "ComponentManager initialized"));
    }
    return *theComponentManager;
}

} // namespace Core
} // namespace Flexus