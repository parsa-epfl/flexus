#include <components/Dummy/Dummy.hpp>
#include <iostream>

#define FLEXUS_BEGIN_COMPONENT Dummy
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDummy {

class FLEXUS_COMPONENT(Dummy)
{
    FLEXUS_COMPONENT_IMPL(Dummy);

  public:
    FLEXUS_COMPONENT_CONSTRUCTOR(Dummy)
      : base(FLEXUS_PASS_CONSTRUCTOR_ARGS)
    {
    }

    bool isQuiesced() const { return true; }

    // Initialization
    void initialize()
    {
        std::cout << "Called Init \n";
        curState = cfg.InitState;
    }

    void finalize() { std::cout << "Final state is:" << curState << std::endl; }

    int getStateNonWire() { return curState; }

    // setState PushInput Port
    //=========================
    bool available(interface::setState const&)
    {
        std::cout << "Port:setState is always available\n";
        return true;
    }

    void push(interface::setState const&, int& newState)
    {
        std::cout << "Received:" << newState << " on Port:setState\n";
        curState = newState;
    }

    // pullStateRet PullOutput Port
    // =============================
    FLEXUS_PORT_ALWAYS_AVAILABLE(pullStateRet);
    int pull(interface::pullStateRet const&) { return curState; }

    // setStateDyn Dynamic PushInput Port
    // ===================================
    bool available(interface::setStateDyn const&, index_t anIndex) { return anIndex % 2 == 0; }

    void push(interface::setStateDyn const&, index_t anIndex, int& payload) { curState = anIndex * payload; }

    // pullStateRetDyn Dynamic PullOutput Port
    // =======================================
    FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(pullStateRetDyn);
    int pull(interface::pullStateRetDyn const&, index_t anIndex) { return anIndex * 1000; }
    // Drive Interfaces
    void drive(interface::DummyDrive const&)
    {
        curState++;
        std::cout << "Drive Called. Sending incremented state " << curState << " over Port:getState\n";
        if (FLEXUS_CHANNEL(getState).available()) FLEXUS_CHANNEL(getState) << curState;
        if (FLEXUS_CHANNEL(pullStateIn).available()) FLEXUS_CHANNEL(pullStateIn) >> curState;
        int out = 20;
        if (FLEXUS_CHANNEL_ARRAY(getStateDyn, 3).available()) FLEXUS_CHANNEL_ARRAY(getStateDyn, 3) << out;
        if (FLEXUS_CHANNEL_ARRAY(pullStateInDyn, 3).available()) FLEXUS_CHANNEL_ARRAY(pullStateInDyn, 3) >> curState;
    }

  private:
    int curState = 100;
};

} // End namespace nDummy

FLEXUS_COMPONENT_INSTANTIATOR(Dummy, nDummy);

FLEXUS_PORT_ARRAY_WIDTH(Dummy, setStateDyn)
{
    return 4;
}

FLEXUS_PORT_ARRAY_WIDTH(Dummy, getStateDyn)
{
    return 4;
}

FLEXUS_PORT_ARRAY_WIDTH(Dummy, pullStateInDyn)
{
    return 4;
}

FLEXUS_PORT_ARRAY_WIDTH(Dummy, pullStateRetDyn)
{
    return 4;
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Dummy
