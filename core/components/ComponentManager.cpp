//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
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

  public:
    virtual ~ComponentManagerImpl() {}

    Flexus::Core::index_t systemWidth() const { return theSystemWidth; }

    void registerHandle(std::function<void(Flexus::Core::index_t)> anInstantiator)
    {
        theInstantiationFunctions.push_back(anInstantiator);
    }

    void instantiateComponents(Flexus::Core::index_t aSystemWidth)
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
        // std::for_each
        // ( theComponents.begin()
        //   , theComponents.end()
        //   , ll::bind( &ComponentInterface::saveState, ll::_1, aDirectory )
        // );
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
