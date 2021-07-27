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
#include <iostream>
#include <components/Dummy/Dummy.hpp>

#define FLEXUS_BEGIN_COMPONENT Dummy
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDummy
{

class FLEXUS_COMPONENT(Dummy) 
{
	FLEXUS_COMPONENT_IMPL(Dummy);

	public:
	FLEXUS_COMPONENT_CONSTRUCTOR(Dummy) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {}

	bool isQuiesced() const { return true; }

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
  	FLEXUS_PORT_ALWAYS_AVAILABLE(pullStateRet);
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
	FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(pullStateRetDyn);  	
	int pull(interface::pullStateRetDyn const &, index_t anIndex)
	{
    		return anIndex * 1000;
  	}
	// Drive Interfaces
	void drive(interface::DummyDrive const &) 
	{
		curState++;
		std::cout << "Drive Called. Sending incremented state " << curState <<  " over Port:getState\n";
		if(FLEXUS_CHANNEL(getState).available())
			FLEXUS_CHANNEL(getState) << curState;
		if(FLEXUS_CHANNEL(pullStateIn).available())
			FLEXUS_CHANNEL(pullStateIn) >> curState;
		int out = 20;
		if(FLEXUS_CHANNEL_ARRAY(getStateDyn, 3).available())
			FLEXUS_CHANNEL_ARRAY(getStateDyn, 3) << out;
		if(FLEXUS_CHANNEL_ARRAY(pullStateInDyn, 3).available())
			FLEXUS_CHANNEL_ARRAY(pullStateInDyn, 3) >> curState;
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
