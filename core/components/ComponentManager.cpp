#include <iostream>
#include <vector>
#include <algorithm>

#include <functional>
#include <boost/scoped_ptr.hpp>

#include <core/component.hpp>
#include <core/debug/debug.hpp>

namespace Flexus {
namespace Wiring {
bool connectWiring();
}
}

namespace Flexus {
namespace Core {
namespace aux_ {

class ComponentManagerImpl : public ComponentManager {

  typedef std::vector< std::function< void (Flexus::Core::index_t ) > > instatiation_vector;
  std::vector< std::function< void (Flexus::Core::index_t aSystemWidth ) > > theInstantiationFunctions;
  std::vector< ComponentInterface * > theComponents;
  Flexus::Core::index_t theSystemWidth;

public:
  virtual ~ComponentManagerImpl() {}

  Flexus::Core::index_t systemWidth() const {
    return theSystemWidth;
  }

  void registerHandle( std::function< void (Flexus::Core::index_t) > anInstantiator) {
    theInstantiationFunctions.push_back(anInstantiator);
  }

  void instantiateComponents(Flexus::Core::index_t aSystemWidth  ) {
    theSystemWidth = aSystemWidth;
    DBG_( Dev, ( << "Instantiating system with a width factor of: " << aSystemWidth ) );
    Flexus::Wiring::connectWiring();
    instatiation_vector::iterator iter = theInstantiationFunctions.begin();
    instatiation_vector::iterator end = theInstantiationFunctions.end();
    while (iter != end) {
      (*iter)( aSystemWidth );
      ++iter;
    }
  }

  void registerComponent( ComponentInterface * aComponent) {
    theComponents.push_back( aComponent );
  }

  void initComponents() {
    DBG_( Dev, ( << "Initializing " << theComponents.size() <<" components..." ) );
    std::vector< ComponentInterface * >::iterator iter = theComponents.begin();
    std::vector< ComponentInterface * >::iterator end = theComponents.end();
    int counter = 1;
    while (iter != end) {
      DBG_( Dev, ( << "Component " << counter << ": Initializing " << (*iter)->name() ) );
      (*iter)->initialize();
      ++iter;
      ++counter;
    }
  }

// added by PLotfi
  void finalizeComponents() {
    DBG_( Dev, ( << "Finalizing components..." ) );
    std::vector< ComponentInterface * >::iterator iter = theComponents.begin();
    std::vector< ComponentInterface * >::iterator end = theComponents.end();
    while (iter != end) {
      DBG_( Dev, ( << "Finalizing " << (*iter)->name() ) );
      (*iter)->finalize();
      ++iter;
    }
  }
// end PLotfi

  bool isQuiesced() const {
    bool quiesced = true;
    for(auto* aComponent: theComponents){
      quiesced = quiesced && aComponent->isQuiesced();
    }
    // std::for_each
    // ( theComponents.begin()
    //   , theComponents.end()
    //   , ll::var(quiesced) = ll::var(quiesced) && ll::bind( &ComponentInterface::isQuiesced, ll::_1 )
    // );
    return quiesced;
  }

  void doSave(std::string const & aDirectory) const {
    for(auto* aComponent: theComponents){
      aComponent->saveState(aDirectory);
    }
    // std::for_each
    // ( theComponents.begin()
    //   , theComponents.end()
    //   , ll::bind( &ComponentInterface::saveState, ll::_1, aDirectory )
    // );
  }

  void doLoad(std::string const & aDirectory) {
    std::vector< ComponentInterface * >::iterator iter, end;
    iter = theComponents.begin();
    end = theComponents.end();
    while (iter != end) {
      DBG_( Dev, ( << "Loading state: " << (*iter)->name() ) );
      (*iter)->loadState(aDirectory);
      ++iter;
    }
    DBG_( Crit, ( << " Done loading.") );
    /*
          std::for_each
            ( theComponents.begin()
            , theComponents.end()
            , ll::bind( &ComponentInterface::loadState, ll::_1, aDirectory )
            );
    */
  }

};

} //namespace aux_

boost::scoped_ptr<aux_::ComponentManagerImpl> theComponentManager(0);

ComponentManager & ComponentManager::getComponentManager() {
  if (theComponentManager == 0) {
    std::cerr << "Initializing Flexus::ComponentManager...";
    theComponentManager.reset(new aux_::ComponentManagerImpl());
    std::cerr << "done" << std::endl;
  }
  return *theComponentManager;
}

} //namespace Core
} //namespace Flexus

