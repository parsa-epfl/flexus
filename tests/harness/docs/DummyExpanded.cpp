#include <core/simulator_layout.hpp>

// clang-format off
#define FLEXUS_BEGIN_COMPONENT Dummy
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()


struct DummyConfiguration_struct
{
    std::string theConfigName_;
    DummyConfiguration_struct (std::string const &aConfigName) : theConfigName_(aConfigName) { }

    std::string const &name() const
    {
        return theConfigName_;
    }

    template <class Parameter> struct get;
    int InitState;
    struct InitState_param {};
};

template <> struct DummyConfiguration_struct::get< DummyConfiguration_struct::InitState_param>
{
    static std::string name()
    {
        return "InitState";
    }

    static std::string description()
    {
        return "Initial state";
    }

    static std::string cmd_line_switch()
    {
        return "init";
    }

    typedef int type;
    static type defaultValue()
    {
        return 0;
    }

    static type DummyConfiguration_struct::*member()
    {
        return &DummyConfiguration_struct::InitState;
    }
};

struct DummyConfiguration
{
    typedef DummyConfiguration_struct cfg_struct_;
    cfg_struct_ theCfg_;

    std::string name()
    {
        return theCfg_.theConfigName_;
    }

    DummyConfiguration_struct & cfg()
    {
        return theCfg_;
    }

    DummyConfiguration (const std::string &aConfigName) : theCfg_(aConfigName), InitState(theCfg_) { }

    Flexus::Core::aux_::DynamicParameter< DummyConfiguration_struct, DummyConfiguration_struct::InitState_param> InitState;
};;


struct DummyJumpTable;
struct DummyInterface : public Flexus::Core::ComponentInterface
{
    typedef DummyConfiguration_struct configuration;
    typedef DummyJumpTable jump_table;

    static DummyInterface * instantiate(configuration &, jump_table &, Flexus::Core::index_t, Flexus::Core::index_t);

    struct getState
    {
        typedef int payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = false;
    };

    struct setState
    {
        typedef int payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = false;
    };
    virtual void push(setState const &, setState::payload &) = 0;
    virtual bool available(setState const &) = 0;

    struct pullStateIn
    {
        typedef int payload;
        typedef Flexus::Core::aux_::pull port_type;
        static const bool is_array = false;
    };

    struct pullStateRet
    {
        typedef int payload;
        typedef Flexus::Core::aux_::pull port_type;
        static const bool is_array = false;
    };
    virtual pullStateRet::payload pull(pullStateRet const &) = 0;
    virtual bool available(pullStateRet const &) = 0;

    struct setStateDyn
    {
        typedef int payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = true;
    };
    virtual void push(setStateDyn const &, Flexus::Core::index_t, setStateDyn::payload &) = 0;
    virtual bool available(setStateDyn const &, Flexus::Core::index_t) = 0;
    static Flexus::Core::index_t width(configuration &cfg, setStateDyn const &);

    struct getStateDyn
    {
        typedef int payload;
        typedef Flexus::Core::aux_::push port_type;
        static const bool is_array = true;
    };
    static Flexus::Core::index_t width(configuration &cfg, getStateDyn const &);

    struct pullStateInDyn
    {
        typedef int payload;
        typedef Flexus::Core::aux_::pull port_type;
        static const bool is_array = true;
    };
    static Flexus::Core::index_t width(configuration &cfg, pullStateInDyn const &);

    struct pullStateRetDyn
    {
        typedef int payload;
        typedef Flexus::Core::aux_::pull port_type;
        static const bool is_array = true;
    };
    virtual pullStateRetDyn::payload pull(pullStateRetDyn const &, Flexus::Core::index_t) = 0;
    virtual bool available(pullStateRetDyn const &, Flexus::Core::index_t) = 0;
    static Flexus::Core::index_t width(configuration &cfg, pullStateRetDyn const &);

    struct DummyDrive
    {
        static std::string name()
        {
            return "DummyInterface" "::" "DummyDrive";
        }
    };
    virtual void drive(DummyDrive const &) = 0;
};

struct DummyJumpTable
{
    typedef DummyInterface iface;

    bool (*wire_available_getState)(Flexus::Core::index_t anIndex) = 0;
    void (*wire_manip_getState)(Flexus::Core::index_t anIndex, iface::getState::payload & aPayload) = 0;

    bool (*wire_available_pullStateIn)(Flexus::Core::index_t anIndex) = 0;
    void (*wire_manip_pullStateIn)(Flexus::Core::index_t anIndex, iface::pullStateIn::payload & aPayload) = 0;

    bool (*wire_available_getStateDyn)(Flexus::Core::index_t anIndex) = 0;
    void (*wire_manip_getStateDyn)(Flexus::Core::index_t anIndex, iface::getStateDyn::payload & aPayload) = 0;

    bool (*wire_available_pullStateInDyn)(Flexus::Core::index_t anIndex) = 0;
    void (*wire_manip_pullStateInDyn)(Flexus::Core::index_t anIndex, iface::pullStateInDyn::payload & aPayload) = 0;

    DummyJumpTable()
    {
        wire_available_getState = 0;
        wire_available_pullStateIn = 0;
        wire_available_getStateDyn = 0;
        wire_available_pullStateInDyn = 0;
    }

    void check(std::string const &anInstance)
    {
        if (wire_available_getState == 0)
        {
            do
            {
                if (false)
                {
                    // Some debug stuff
                }
            }
            while (0);
        }
        if (wire_available_pullStateIn == 0)
        {
            do
            {
                if (false)
                {
                    // Some debug stuff
                }
            }
            while (0);
        }
        if (wire_available_getStateDyn == 0)
        {
            do
            {
                if (false)
                {
                    // Some debug stuff
                }
            }
            while (0);
        }
        if (wire_available_pullStateInDyn == 0)
        {
            do
            {
                if (false)
                {
                    // Some debug stuff
                }
            }
            while (0);
        }
    }
};


namespace nDummy
{
class DummyComponent : public Flexus::Core::FlexusComponentBase<DummyComponent, DummyConfiguration, DummyInterface>
{
    typedef Flexus::Core::FlexusComponentBase<DummyComponent, DummyConfiguration, DummyInterface> base;
    typedef base::cfg_t cfg_t;

    static std::string componentType()
    {
        return "Dummy";
    }

public:
    using base::flexusIndex;
    using base::flexusWidth;
    using base::name;
    using base::statName;
    using base::cfg;
    using base::interface;
    using interface::get_channel;
    using interface::get_channel_array;

private:
    typedef base::self self;

public:
    DummyComponent (cfg_t & aCfg, DummyInterface::jump_table & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth) : base(aCfg, aJumpTable, anIndex, aWidth) {}
    bool isQuiesced() const
    {
        return true;
    }
    
    // Initialization
    void initialize()
    {
        std::cout << "Called Init \n";
        curState = cfg.InitState;
    }
    
    void finalize()
    {
        std::cout << "Final state is:" << curState << std::endl;
    }
    
    int getStateNonWire()
    {
        return curState;
    }
    
    // setState PushInput Port
    //=========================
    bool available(interface::setState const &)
    {
        std::cout << "Port:setState is always available\n";
        return true;
    }
    void push(interface::setState const &, int &newState)
    {
        std::cout << "Received:" << newState << " on Port:setState\n";
        curState = newState;
    }
    
    // pullStateRet PullOutput Port
    // =============================
    bool available(interface::pullStateRet const &)
    {
        return true;
    } struct eat_semicolon__;
    int pull(interface::pullStateRet const &)
    {
        return curState;
    }
    
    // setStateDyn Dynamic PushInput Port
    // ===================================
    bool available(interface::setStateDyn const &, index_t anIndex)
    {
        return anIndex % 2 == 0;
    }
    void push(interface::setStateDyn const &, index_t anIndex, int &payload)
    {
        curState = anIndex * payload;
    }

    // pullStateRetDyn Dynamic PullOutput Port
    // =======================================
    bool available(interface::pullStateRetDyn const &pn, Flexus::Core::index_t aWidth)
    {
        // Some debug stuff
        return true;
    } struct eat_semicolon__;
    int pull(interface::pullStateRetDyn const &, index_t anIndex)
    {
        return anIndex * 1000;
    }

    // Drive Interfaces
    void drive(interface::DummyDrive const &)
    {
        curState++;
        std::cout << "Drive Called. Sending incremented state " << curState << " over Port:getState\n";
        if(get_channel(interface::getState(), jump_table_.wire_available_getState, jump_table_.wire_manip_getState, flexusIndex()).available())
            get_channel(interface::getState(), jump_table_.wire_available_getState, jump_table_.wire_manip_getState, flexusIndex()) << curState;
        if(get_channel(interface::pullStateIn(), jump_table_.wire_available_pullStateIn, jump_table_.wire_manip_pullStateIn, flexusIndex()).available())
            get_channel(interface::pullStateIn(), jump_table_.wire_available_pullStateIn, jump_table_.wire_manip_pullStateIn, flexusIndex()) >> curState;
        int out = 20;
        if(get_channel_array(interface::getStateDyn(), jump_table_.wire_available_getStateDyn, jump_table_.wire_manip_getStateDyn, flexusIndex(), 3, interface::width(cfg, interface::getStateDyn())).available())
            get_channel_array(interface::getStateDyn(), jump_table_.wire_available_getStateDyn, jump_table_.wire_manip_getStateDyn, flexusIndex(), 3, interface::width(cfg, interface::getStateDyn())) << out;
        if(get_channel_array(interface::pullStateInDyn(), jump_table_.wire_available_pullStateInDyn, jump_table_.wire_manip_pullStateInDyn, flexusIndex(), 3, interface::width(cfg, interface::pullStateInDyn())).available())
            get_channel_array(interface::pullStateInDyn(), jump_table_.wire_available_pullStateInDyn, jump_table_.wire_manip_pullStateInDyn, flexusIndex(), 3, interface::width(cfg, interface::pullStateInDyn())) >> curState;
    }
private:
    int curState = 100;
};
}
// End namespace nDummy
DummyInterface * DummyInterface::instantiate( DummyConfiguration::cfg_struct_ &aCfg, DummyInterface::jump_table &aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth)
{
    return new nDummy::DummyComponent(aCfg, aJumpTable, anIndex, aWidth);
}
struct eat_semicolon_;

Flexus::Core::index_t DummyInterface::width( DummyInterface::configuration &cfg, DummyInterface::setStateDyn const &)
{
    return 4;
}
Flexus::Core::index_t DummyInterface::width( DummyInterface::configuration &cfg, DummyInterface::getStateDyn const &)
{
    return 4;
}
Flexus::Core::index_t DummyInterface::width( DummyInterface::configuration &cfg, DummyInterface::pullStateInDyn const &)
{
    return 4;
}
Flexus::Core::index_t DummyInterface::width( DummyInterface::configuration &cfg, DummyInterface::pullStateRetDyn const &)
{
    return 4;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Dummy
