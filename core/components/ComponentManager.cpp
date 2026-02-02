#include "core/simulator_name.hpp"
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
    Flexus::Core::freq_opts theDriveFreq;

  private:
    std::vector<std::string> splitString(const char * str, char delimiter) {
        std::vector<std::string> tokens;
        if(str == nullptr) return tokens;
        const char* start = str;
        const char* curr = str;
        while(*curr != '\0') {
            if(*curr == delimiter) {
                tokens.emplace_back(start, curr);
                start = curr + 1;
            }
            ++curr;
        }
        if(start != curr) tokens.emplace_back(start, curr);
        return tokens;
    }

  public:
    virtual ~ComponentManagerImpl() {}

    Flexus::Core::index_t systemWidth() const { return theSystemWidth; }
    Flexus::Core::freq_opts getFreq() const { return theDriveFreq; }

    void registerHandle(std::function<void(Flexus::Core::index_t)> anInstantiator)
    {
        theInstantiationFunctions.push_back(anInstantiator);
    }

    void instantiateComponents(Flexus::Core::index_t aSystemWidth, const char * freq)
    {
        // A dirty way to hack the system width.
        if (Flexus::theSimulatorName.find("SemiKraken") != std::string::npos) {
            DBG_(Dev, (<< "Detecting Semikraken. Now the system width is shrunk to half of" << aSystemWidth));
            DBG_Assert(aSystemWidth % 2 == 0, (<< "The system width should be even."));
            theSystemWidth = aSystemWidth / 2;
        } else {
            theSystemWidth = aSystemWidth;
        }

        DBG_(Dev, (<< "Instantiating system with a width factor of: " << theSystemWidth));
        Flexus::Wiring::connectWiring();
        instatiation_vector::iterator iter = theInstantiationFunctions.begin();
        instatiation_vector::iterator end  = theInstantiationFunctions.end();
        while (iter != end) {
            (*iter)(theSystemWidth);
            ++iter;
        }

        // Drive frequency calculations
        std::vector<std::string> freq_split = splitString(freq, ':');
        if (freq_split.size() == 1) {
            for(index_t i = 0; i < theSystemWidth; ++i) {
                freq_split.push_back(freq_split[0]);
            }
        } else {
            DBG_Assert(freq_split.size() == theSystemWidth + 1, (<< "Frequency string does not match the system width."));
        }

        index_t driveFreq, cyclesPerIter, remCycles;
        theDriveFreq.mapCyclesIter = new index_t*[theSystemWidth];
        for(index_t i = 0; i <= theSystemWidth; ++i) {
            driveFreq = (index_t)(std::stof(freq_split[i]) * 10);
            cyclesPerIter = driveFreq / 10;
            remCycles = driveFreq - cyclesPerIter * 10;
            theDriveFreq.mapCyclesIter[i] = new index_t[10];
            for(index_t j = 0; j < 10; ++j) {
                theDriveFreq.mapCyclesIter[i][j] = cyclesPerIter;
                if(j >= (10-remCycles)) theDriveFreq.mapCyclesIter[i][j]++;
            }
        }
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
            DBG_(Dev, (<< "Saving state: " << aComponent->name()));
            aComponent->saveState(aDirectory);
        }
        DBG_(Crit, (<< " Done saving."));
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
