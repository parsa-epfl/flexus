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
#ifndef FLEXUS_COMPONENT_HPP_INCLUDED
#define FLEXUS_COMPONENT_HPP_INCLUDED

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cstdint>
#include <functional>
namespace ll = boost::lambda;

#include <core/boost_extensions/padded_string_cast.hpp>
#include <core/component_interface.hpp>
#include <core/configuration_macros.hpp>
#include <core/debug/debugger.hpp>
#include <core/interface_macros.hpp>

namespace Flexus {
namespace Core {

struct ComponentManager
{
    virtual ~ComponentManager() {}
    virtual void initComponents() = 0;
    // added by PLotfi
    virtual void finalizeComponents() = 0;
    // end PLotfi
    virtual bool isQuiesced() const                                                        = 0;
    virtual void doSave(std::string const& aDirectory) const                               = 0;
    virtual void doLoad(std::string const& aDirectory)                                     = 0;
    virtual void registerComponent(ComponentInterface* aComponent)                         = 0;
    virtual void registerHandle(std::function<void(Flexus::Core::index_t)> anInstantiator) = 0;
    virtual void instantiateComponents(Flexus::Core::index_t aSystemWidth)                 = 0;
    virtual Flexus::Core::index_t systemWidth() const                                      = 0;
    static ComponentManager& getComponentManager();
};

#define FLEXUS_COMPONENT(comp)                                                                                         \
    BOOST_PP_CAT(comp, Component)                                                                                      \
      : public Flexus::Core::FlexusComponentBase<BOOST_PP_CAT(comp, Component),                                        \
                                                 BOOST_PP_CAT(comp, Configuration),                                    \
                                                 BOOST_PP_CAT(comp, Interface)> /**/

#define FLEXUS_COMPONENT_INSTANTIATOR(Comp, Namespace)                                                                 \
    BOOST_PP_CAT(Comp, Interface) *                                                                                    \
      BOOST_PP_CAT(Comp, Interface)::instantiate(BOOST_PP_CAT(Comp, Configuration)::cfg_struct_& aCfg,                 \
                                                 BOOST_PP_CAT(Comp, Interface)::jump_table& aJumpTable,                \
                                                 Flexus::Core::index_t anIndex,                                        \
                                                 Flexus::Core::index_t aWidth)                                         \
    {                                                                                                                  \
        return new Namespace::BOOST_PP_CAT(Comp, Component)(aCfg, aJumpTable, anIndex, aWidth);                        \
    }

#define FLEXUS_PORT_ARRAY_WIDTH(Comp, PortArray)                                                                       \
    Flexus::Core::index_t BOOST_PP_CAT(Comp, Interface)::width(BOOST_PP_CAT(Comp, Interface)::configuration& cfg,      \
                                                               BOOST_PP_CAT(Comp, Interface)::PortArray const&) /**/

#define FLEXUS_COMPONENT_CONSTRUCTOR(comp)                                                                             \
    BOOST_PP_CAT(comp, Component)                                                                                      \
    (cfg_t & aCfg,                                                                                                     \
     BOOST_PP_CAT(comp, Interface)::jump_table & aJumpTable,                                                           \
     Flexus::Core::index_t anIndex,                                                                                    \
     Flexus::Core::index_t aWidth) /**/

#define FLEXUS_PASS_CONSTRUCTOR_ARGS aCfg, aJumpTable, anIndex, aWidth

#define FLEXUS_COMPONENT_IMPL(comp)                                                                                    \
    typedef Flexus::Core::FlexusComponentBase<BOOST_PP_CAT(comp, Component),                                           \
                                              BOOST_PP_CAT(comp, Configuration),                                       \
                                              BOOST_PP_CAT(comp, Interface)>                                           \
      base;                                                                                                            \
    typedef base::cfg_t cfg_t;                                                                                         \
    static std::string componentType()                                                                                 \
    {                                                                                                                  \
        return #comp;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
  public:                                                                                                              \
    using base::flexusIndex;                                                                                           \
    using base::flexusWidth;                                                                                           \
    using base::name;                                                                                                  \
    using base::statName;                                                                                              \
    using base::cfg;                                                                                                   \
    using base::interface;                                                                                             \
    using interface::get_channel;                                                                                      \
    using interface::get_channel_array;                                                                                \
                                                                                                                       \
  private:                                                                                                             \
    typedef base::self self /**/

#define FLEXUS_PORT_ALWAYS_AVAILABLE(PortName)                                                                         \
    bool available(interface::PortName const&)                                                                         \
    {                                                                                                                  \
        return true;                                                                                                   \
    }

#define FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(PortName)                                                                   \
    bool available(interface::PortName const& pn, Flexus::Core::index_t aWidth)                                        \
    {                                                                                                                  \
        DBG_Assert(aWidth < width(cfg, pn));                                                                           \
        return true;                                                                                                   \
    }

#define FLEXUS_CHANNEL(PORT)                                                                                           \
    get_channel(interface::PORT(),                                                                                     \
                jump_table_.BOOST_PP_CAT(wire_available_, PORT),                                                       \
                jump_table_.BOOST_PP_CAT(wire_manip_, PORT),                                                           \
                flexusIndex())
#define FLEXUS_CHANNEL_ARRAY(PORT, INDEX)                                                                              \
    get_channel_array(interface::PORT(),                                                                               \
                      jump_table_.BOOST_PP_CAT(wire_available_, PORT),                                                 \
                      jump_table_.BOOST_PP_CAT(wire_manip_, PORT),                                                     \
                      flexusIndex(),                                                                                   \
                      INDEX,                                                                                           \
                      interface::width(cfg, interface::PORT()))

class FlexusComponent
{
  private:
    index_t theIndex_;
    index_t theWidth_;

  public:
    index_t flexusIndex() const { return theIndex_; }
    index_t flexusWidth() const { return theWidth_; }

    bool theDebugEnabled_;
    bool debugEnabled() { return theDebugEnabled_; }

    virtual ~FlexusComponent() {}

  protected:
    FlexusComponent(index_t anIndex, index_t aWidth)
      : theIndex_(anIndex)
      , theWidth_(aWidth)
      , theDebugEnabled_(true)
    {
    }
};

template<class Component, class Configuration, class Interface>
class FlexusComponentBase
  : public FlexusComponent
  , public Interface
{
  public:
    typedef Component self;
    typedef Interface interface;
    typedef typename Configuration::cfg_struct_ cfg_t;

    cfg_t& cfg;
    typename interface::jump_table& jump_table_;

    std::string name() const
    {
        if (flexusWidth() > 1) {
            return boost::padded_string_cast<3, '0'>(flexusIndex()) + "-" + cfg.name();
        } else {
            return std::string("000-") + cfg.name();
        }
    }

    std::string statName() const { return name(); }

    virtual bool isQuiesced() const
    {
        DBG_(Crit, (<< "Warning: isQuiesced() is not implemented in component " << name()));
        return true;
    }

    virtual void loadState(std::string const& aDirName)
    {
        // Nothing to save
    }

    virtual void saveState(std::string const& aDirName)
    {
        // Nothing to save
    }

    FlexusComponentBase(cfg_t& aCfg, typename interface::jump_table& aJumpTable, index_t anIndex, index_t aWidth)
      : FlexusComponent(anIndex, aWidth)
      , cfg(aCfg)
      , jump_table_(aJumpTable)
    {
        jump_table_.check(aCfg.name());
        Flexus::Dbg::Debugger::theDebugger->registerComponent(cfg.name(), flexusIndex(), &(this->theDebugEnabled_));
    }
};

#define FLEXUS_INSTANTIATE_COMPONENT(Component, Configuration, InstanceName)                                           \
    ComponentInstance<BOOST_PP_CAT(Component, Interface)> BOOST_PP_CAT(InstanceName, _instance)(Configuration.cfg());  \
    typedef ComponentHandle<ComponentInstance<BOOST_PP_CAT(Component, Interface)>,                                     \
                            &BOOST_PP_CAT(InstanceName, _instance)>                                                    \
      InstanceName; /**/

#define FLEXUS_INSTANTIATE_COMPONENT_ARRAY(Component, Configuration, InstanceName, Scale, Multiply, Width)             \
    ComponentInstance<BOOST_PP_CAT(Component, Interface)> BOOST_PP_CAT(                                                \
      InstanceName,                                                                                                    \
      _instance)(Configuration.cfg(), Width, Scale, Multiply);                                                         \
    typedef ComponentHandle<ComponentInstance<BOOST_PP_CAT(Component, Interface)>,                                     \
                            &BOOST_PP_CAT(InstanceName, _instance)>                                                    \
      InstanceName; /**/

template<class ComponentInstance, ComponentInstance* anInstance>
struct ComponentHandle
{
    typedef typename ComponentInstance::iface iface;

    static Flexus::Core::index_t width() { return anInstance->theWidth; }
    static ComponentInstance& getInstance() { return *anInstance; }
    static iface& getReference(index_t anIndex) { return *(*anInstance)[anIndex]; }
};

template<class ComponentHandle, class Drive>
struct DriveHandle : public ComponentHandle
{
    typedef ComponentHandle component_handle;
    typedef Drive drive;
};

template<class ComponentInterface>
struct ComponentInstance
{
    typedef ComponentInterface iface;
    iface** theComponent;
    index_t theWidth;
    typename iface::jump_table theJumpTable;
    typename iface::configuration& theConfiguration;
    bool theScaleWithSystem;
    bool theMultiply;
    ComponentInstance(typename iface::configuration& aConfiguration,
                      Flexus::Core::index_t anArrayWidth = 1,
                      bool aScaleWithSystem              = false,
                      bool aMultiply                     = false)
      : theComponent(0)
      , theWidth(anArrayWidth)
      , theConfiguration(aConfiguration)
      , theScaleWithSystem(aScaleWithSystem)
      , theMultiply(aMultiply)
    {
        ComponentManager::getComponentManager().registerHandle(
          [this](Flexus::Core::index_t x) { return this->instantiator(x); });
    }

    void instantiator(Flexus::Core::index_t aSystemWidth)
    {
        if (theScaleWithSystem && theMultiply) {
            theWidth = aSystemWidth * theWidth;
        } else if (theScaleWithSystem && !theMultiply) {
            theWidth = aSystemWidth / theWidth;
        }
        theComponent = new iface*[theWidth];
        for (Flexus::Core::index_t i = 0; i < theWidth; ++i) {
            theComponent[i] = ComponentInterface::instantiate(theConfiguration, theJumpTable, i, theWidth);
            ComponentManager::getComponentManager().registerComponent(theComponent[i]);
        }
    }

    iface* operator[](Flexus::Core::index_t anIndex)
    {
        DBG_Assert(anIndex < theWidth,
                   (<< "Component: " << theConfiguration.name() << " Index: " << anIndex << " Width: " << theWidth));
        return theComponent[anIndex];
    }
};

} // namespace Core
} // namespace Flexus

#endif // FLEXUS_COMPONENT_HPP_INCLUDED
