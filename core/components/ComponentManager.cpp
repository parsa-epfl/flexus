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
        assert(freq_split.size() == aSystemWidth + 1);
        index_t numerator[aSystemWidth+1], denominator[aSystemWidth+1], driveFreq[aSystemWidth+1];

        // Reduce fractions to their simplest form
        for(index_t i = 0; i <= aSystemWidth; ++i) {
            float f_freq = std::stof(freq_split[i]);
            std::tie(numerator[i], denominator[i]) = helper(f_freq);
        }

        // Find the LCM of all denominators
        index_t lcmDen = denominator[0];
        for(index_t i = 1; i <= aSystemWidth; ++i) {
            lcmDen = (lcmDen * denominator[i]) / gcd(lcmDen, denominator[i]);
        }

        // Calculate the drive frequencies
        for(index_t i = 0; i <= aSystemWidth; ++i) {
            driveFreq[i] = (lcmDen / denominator[i]) * numerator[i];
        }

        // Calculate the GCD of all drive frequencies
        index_t gcdFreq = driveFreq[0];
        for(index_t i = 1; i <= aSystemWidth; ++i) {
            gcdFreq = gcd(gcdFreq, driveFreq[i]);
        }

        // Normalize the drive frequencies
        for(index_t i = 0; i <= aSystemWidth; ++i) {
            driveFreq[i] /= gcdFreq;
        }

        // Store the drive frequencies
        theDriveFreq.freq = new index_t[aSystemWidth + 1];
        std::copy(driveFreq, driveFreq + aSystemWidth + 1, theDriveFreq.freq);
        std::sort(driveFreq, driveFreq + aSystemWidth + 1);
        theDriveFreq.maxFreq = driveFreq[aSystemWidth];
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