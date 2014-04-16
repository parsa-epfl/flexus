#ifndef MAIN_HPP
#define MAIN_HPP


#include <string>
#include <stdlib.h>
#include <fstream>
#include "booksim.hpp"
#include "routefunc.hpp"
#include "traffic.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "random_utils.hpp"
#include "network.hpp"
#include "injection.hpp"
#include "power_module.hpp"

///////////////////////////////////////////////////////////////////////////////
//include new network here//

#include "singlenet.hpp"
#include "kncube.hpp"
#include "fly.hpp"
#include "isolated_mesh.hpp"
#include "cmo.hpp"
#include "cmesh.hpp"
#include "cmeshx2.hpp"
#include "flatfly_onchip.hpp"
#include "qtree.hpp"
#include "tree4.hpp"
#include "fattree.hpp"
#include "mecs.hpp"
#include "anynet.hpp"
#include "dragonfly.hpp"
/**/
///////////////////////////////////////////////////////////////////////////////


//by mehdi
//#define FlexusDebug

class FlexNet
{
public:
	TrafficManager *  trafficManager;
	Network **net;
	BookSimConfig config;

        
	FlexNet(){;};	
	int booksim_main(std::string Topology, std::string RoutingFunc, int NetworkRadix, int NetworkDimension, int Concentration, int NumRoutersX, int NumRoutersY, int NumNodesPerRouterX, int NumNodesPerRouterY, int NumInPorts, int NumOutPorts, int Speculation, int VChannels, int VCBuffSize, int RoutingDelay, int VCAllocDelay, int SWAllocDelay, int STPrepDelay, int STFinalDelay, int ChannelWidth, int DataPackLength, int CtrlPackLength);
	bool AllocatorSim();

	bool available(int, int);
	bool FlexInject();
	bool FlexEject();
	
		 
};


int booksim_main( TrafficManager *  );



#endif

