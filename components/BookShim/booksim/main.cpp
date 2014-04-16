// $Id: main.cpp 138 2009-02-05 20:47:15Z njiang37 $

/*
Copyright (c) 2007, Trustees of Leland Stanford Junior University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this 
list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.
Neither the name of the Stanford University nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*main.cpp
 *
 *The starting point of the network simulator
 *-Include all network header files
 *-initilize the network
 *-initialize the traffic manager and set it to run
 *
 *
 */

#include "main.hpp"

//Global declarations
//////////////////////
/* printing activity factor*/
bool _print_activity = false;

int gK = 0;//radix
int gN = 0;//dimension
int gC = 0;//concentration

/*These extra variables are necessary for correct traffic pattern generation
 *The difference is due to concentration, radix 4 with concentration of 4 is
 *equivalent to radix 8 with no concentration. Though this only really applies
 *under NOCs since NOCS are inheriently 2 dimension
 */
int realgk;
int realgn;

int gNodes = 0;

/*These variables are used by NOCS to specify the node concentration per 
 *router. Technically the realdgk realgn can be calculated from these 
 *global variables, thus they maybe removed later
 */
int xrouter = 0;
int yrouter = 0;
int xcount  = 0;
int ycount  = 0;

//generate nocviewer trace
bool _trace =false; //= true; //false; changed by mehdi

/*false means all packet types are the same length "gConstantsize"
 *All packets uses all VCS
 *packet types are generated randomly, essentially making it only 1 type
 *of packet in the network
 *
 *True means only request packets are generated and replies are generated
 *as a response to the requests, packets are now difference length, correspond
 *to "read_request_size" etc. 
 */
bool _use_read_write = false;

//injection functions
map<string, tInjectionProcess> gInjectionProcessMap;

//burst rates
double gBurstAlpha;
double gBurstBeta;

/*number of flits per packet, when _use_read_write is false*/

int    gConstPacketSize;

//for on_off injections
int *gNodeStates = 0;

//flits to watch
string watch_file;

//latency type, noc or conventional network
bool _use_noc_latency;


/////////////////////////////////////////////////////////////////////////////

bool AllocatorSim( const Configuration& config , TrafficManager * trafficManager)
{
  Network **net;
  string topo;
  bool result;

  config.GetStr( "topology", topo );
  short networks = config.GetInt("physical_subnetworks");
  /*To include a new network, must register the network here
   *add an else if statement with the name of the network
   */
  net = new Network*[networks];
  for (int i = 0; i < networks; ++i) {
    if ( topo == "torus" ) {
      KNCube::RegisterRoutingFunctions() ;
      net[i] = new KNCube( config, true );
    } else if ( topo == "mesh" ) {
      KNCube::RegisterRoutingFunctions() ;
      net[i] = new KNCube( config, false );
    } else if ( topo == "cmesh" ) {
      CMesh::RegisterRoutingFunctions() ;
      net[i] = new CMesh( config );
    } else if ( topo == "cmeshx2" ) {
      CMeshX2::RegisterRoutingFunctions() ;
      net[i] = new CMeshX2( config );
    } else if ( topo == "fly" ) {
      KNFly::RegisterRoutingFunctions() ;
      net[i] = new KNFly( config );
    } else if ( topo == "single" ) {
      SingleNet::RegisterRoutingFunctions() ;
      net[i] = new SingleNet( config );
    } else if ( topo == "isolated_mesh" ) {
      IsolatedMesh::RegisterRoutingFunctions() ;
      net[i] = new IsolatedMesh( config );
    } else if ( topo == "qtree" ) {
      QTree::RegisterRoutingFunctions() ;
      net[i] = new QTree( config );
    } else if ( topo == "tree4" ) {
      Tree4::RegisterRoutingFunctions() ;
      net[i] = new Tree4( config );
    } else if ( topo == "fattree" ) {
      FatTree::RegisterRoutingFunctions() ;
      net[i] = new FatTree( config );
    } else if ( topo == "flatfly" ) {
      FlatFlyOnChip::RegisterRoutingFunctions() ;
      net[i] = new FlatFlyOnChip( config );
    } else if ( topo == "cmo"){
      CMO::RegisterRoutingFunctions() ;
      net[i] = new CMO(config);
    } else if ( topo == "MECS"){
      MECS::RegisterRoutingFunctions() ;
      net[i] = new MECS(config);
    } else if ( topo == "anynet"){
      AnyNet::RegisterRoutingFunctions() ;
      net[i] = new AnyNet(config);
    } else if ( topo == "dragonflynew"){
      DragonFlyNew::RegisterRoutingFunctions() ;
      net[i] = new DragonFlyNew(config);
    }else {
      cerr << "Unknown topology " << topo << endl;
      exit(-1);
    }

  /*legacy code that insert random faults in the networks
   *not sure how to use this
   */
  if ( config.GetInt( "link_failures" ) ) {
      net[i]->InsertRandomFaults( config );
    }
  }
//added by mehdi
cout<<"\nVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n";
net[0]->Display();
cout<<"\nVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n";

  string traffic ;
  config.GetStr( "traffic", traffic ) ;

  /*tcc and characterize are legacy
   *not sure how to use them 
   */
  //TrafficManager *trafficManager ;
  trafficManager = new TrafficManager( config, net ) ;

  /*Start the simulation run
   */

  result = trafficManager->Run() ;
   
/* must be added somewhere in Flexus Netshim component
 ///Power analysis
  if(config.GetInt("sim_power")==1){
    Power_Module * pnet = new Power_Module(net[0], trafficManager, config);
    pnet->run();
    delete pnet;
  }

  delete trafficManager ;
  for (int i=0; i<networks; ++i)
    delete net[i];
  delete [] net;
*/
  return result;
}


int booksim_main( TrafficManager * trf_mng/*int argc, char **argv*/ )
{

        cout<<"plz dont call me. call my updated version!";
	exit(0);
}



int FlexNet::booksim_main(std::string Topology, std::string RoutingFunc, int NetworkRadix, int NetworkDimension, int Concentration, int NumRoutersX, int NumRoutersY, int NumNodesPerRouterX, int NumNodesPerRouterY, int NumInPorts, int NumOutPorts, int Speculation, int VChannels, int VCBuffSize, int RoutingDelay, int VCAllocDelay, int SWAllocDelay, int STPrepDelay, int STFinalDelay, int ChannelWidth, int DataPackLength, int CtrlPackLength)
{
	//BookSimConfig config;
	
	
//	char* booksim_in_file_name = new char [50]; 
//	booksim_in_file_name= "torus-4x4.topology";//"/home/modarres/torus44";
	
	
//	config.Parse( booksim_in_file_name );
	//ifstream in(argv[1]);
//	ifstream in;
//	in.open(booksim_in_file_name);//"./examples/torus88");
//	char c;
//	cout << "BEGIN Configuration File" << endl;
//	cout << "Name: " << /*argv[1]*/ booksim_in_file_name<< endl;
//	while (!in.eof()) {
//	in.get(c);
//	cout << c ;
//	}
//	cout << "END Configuration File" << endl;

	// initialization of booksim parameters -- Plotfi
	config.Assign( "topology", Topology );
        config.Assign( "routing_function", RoutingFunc );
	config.Assign( "k", (unsigned int) NetworkRadix );
	config.Assign( "n", (unsigned int) NetworkDimension );
	config.Assign( "c", (unsigned int) Concentration );
	config.Assign( "x", (unsigned int) NumRoutersX );
	config.Assign( "y", (unsigned int) NumRoutersY );
	config.Assign( "xr", (unsigned int) NumNodesPerRouterX );
	config.Assign( "yr", (unsigned int) NumNodesPerRouterY );
	config.Assign( "in_ports", (unsigned int) NumInPorts );
	config.Assign( "out_ports", (unsigned int) NumOutPorts );
	config.Assign( "speculative", (unsigned int) Speculation );
	config.Assign( "num_vcs", (unsigned int) VChannels );
	config.Assign( "vc_buf_size", (unsigned int) VCBuffSize );
	config.Assign( "routing_delay", (unsigned int) RoutingDelay );
	config.Assign( "vc_alloc_delay", (unsigned int) VCAllocDelay );
	config.Assign( "sw_alloc_delay", (unsigned int) SWAllocDelay );
	config.Assign( "st_prepare_delay", (unsigned int) STPrepDelay );
	config.Assign( "st_final_delay", (unsigned int) STFinalDelay );
	config.Assign( "channel_width", (unsigned int) ChannelWidth );
	config.Assign( "packet_length_data", (unsigned int) DataPackLength );
	config.Assign( "packet_length_ctrl", (unsigned int) CtrlPackLength );
	// end of booksim parameter initialization -- Plotfi
	
	/*initialize routing, traffic, injection functions
	*/
	InitializeRoutingMap( );
	InitializeTrafficMap( );
	InitializeInjectionMap( );
	
	_print_activity = (config.GetInt("print_activity")==1);
	_trace = (config.GetInt("viewer trace")==1);
	
	_use_read_write = (config.GetInt("use_read_write")==1);
	
	config.GetStr( "watch_file", watch_file );
	
	_use_noc_latency = (config.GetInt("use_noc_latency")==1);
	
	/*configure and run the simulator
	*/
	
	bool result = AllocatorSim();
	return result ? -1 : 0;
		
}


bool FlexNet::AllocatorSim()
{

	string topo;
	bool result;
	
	config.GetStr( "topology", topo );
	short networks = config.GetInt("physical_subnetworks");
	/*To include a new network, must register the network here
	*add an else if statement with the name of the network
	*/
	net = new Network*[networks];
	for (int i = 0; i < networks; ++i) {
	if ( topo == "torus" ) {
	KNCube::RegisterRoutingFunctions() ;
	net[i] = new KNCube( config, true );
	} else if ( topo == "mesh" ) {
	KNCube::RegisterRoutingFunctions() ;
	net[i] = new KNCube( config, false );
	} else if ( topo == "cmesh" ) {
	CMesh::RegisterRoutingFunctions() ;
	net[i] = new CMesh( config );
	} else if ( topo == "cmeshx2" ) {
	CMeshX2::RegisterRoutingFunctions() ;
	net[i] = new CMeshX2( config );
	} else if ( topo == "fly" ) {
	KNFly::RegisterRoutingFunctions() ;
	net[i] = new KNFly( config );
	} else if ( topo == "single" ) {
	SingleNet::RegisterRoutingFunctions() ;
	net[i] = new SingleNet( config );
	} else if ( topo == "isolated_mesh" ) {
	IsolatedMesh::RegisterRoutingFunctions() ;
	net[i] = new IsolatedMesh( config );
	} else if ( topo == "qtree" ) {
	QTree::RegisterRoutingFunctions() ;
	net[i] = new QTree( config );
	} else if ( topo == "tree4" ) {
	Tree4::RegisterRoutingFunctions() ;
	net[i] = new Tree4( config );
	} else if ( topo == "fattree" ) {
	FatTree::RegisterRoutingFunctions() ;
	net[i] = new FatTree( config );
	} else if ( topo == "flatfly" ) {
	FlatFlyOnChip::RegisterRoutingFunctions() ;
	net[i] = new FlatFlyOnChip( config );
	} else if ( topo == "cmo"){
	CMO::RegisterRoutingFunctions() ;
	net[i] = new CMO(config);
	} else if ( topo == "MECS"){
	MECS::RegisterRoutingFunctions() ;
	net[i] = new MECS(config);
	} else if ( topo == "anynet"){
	AnyNet::RegisterRoutingFunctions() ;
	net[i] = new AnyNet(config);
	} else if ( topo == "dragonflynew"){
	DragonFlyNew::RegisterRoutingFunctions() ;
	net[i] = new DragonFlyNew(config);
	}else {
	cerr << "Unknown topology " << topo << endl;
	exit(-1);
	}
	
	/*legacy code that insert random faults in the networks
	*not sure how to use this
	*/
	if ( config.GetInt( "link_failures" ) ) {
	net[i]->InsertRandomFaults( config );
	}
	}
	

        //commented by mehdi
	//cout<<"\nVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n";
	//net[0]->Display();
	//cout<<"\nVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n";
	
	string traffic ;
	config.GetStr( "traffic", traffic ) ;
	
	/*tcc and characterize are legacy
	*not sure how to use them 
	*/
	//TrafficManager *trafficManager ;
	trafficManager = new TrafficManager( config, net ) ;
	
	/*Start the simulation run
	*/
	
	result = trafficManager->Run() ;
	
	/* must be added somewhere in Flexus Netshim component
	///Power analysis
	if(config.GetInt("sim_power")==1){
	Power_Module * pnet = new Power_Module(net[0], trafficManager, config);
	pnet->run();
	delete pnet;
	}
	
	delete trafficManager ;
	for (int i=0; i<networks; ++i)
	delete net[i];
	delete [] net;
	*/
	bool it_is_redundant=true;
	return it_is_redundant;
}


	bool FlexNet::FlexInject()
	{
		return false;
	};
	bool FlexNet::FlexEject()
	{
		return false;
	};


/*int main()
{
	booksim_main();
	return 0;
}
*/


bool FlexNet::available(int node, int vc) //check if it should return 0 when there are free slots for a new packet?
{
	//int value=1;
        bool value= trafficManager->available(node,vc,0);//_partial_packets[node][vc][0].max_size()==trafficManager->_partial_packets[node][vc][0].size() );
        //cout<<"\n ----  "<<;
        //cout<<" -----  " <<trafficManager->_partial_packets[node][vc][0].size();
        
	//int lsss= (trafficManager->_partial_packets[node][vc][0].empty( ));
        return value;
}
