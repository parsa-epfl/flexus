// $Id: trafficmanager.cpp 140 2009-03-11 17:49:56Z njiang37 $

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
      
#include <sstream>
#include <math.h>
#include <fstream>
#include "trafficmanager.hpp"
#include "random_utils.hpp" 
//#include "SIM_port.h"

#include "components/BookShim/netcommon.hpp"
#include "components/BookShim/BooksimStats.hpp"

void *global_theBooksimStats;
using namespace nNetwork;

#define MaxCyclePerFlexpoint 149999

//#define NiCBuffDepth 20
//#define FlexusDebug
//#define NetLog
//#define MaxSameSandD 20

//batched time-mode, know what you are doing
bool timed_mode = false;

	 //nNetShim::MessageState * msg;
	 //msg->srcNode=0;


TrafficManager::TrafficManager( const Configuration &config, Network **net )
  : Module( 0, "traffic_manager" )
{

  /*cout<<PARM_TECH_POINT;
  int abra; cin>>abra;*/
  
  ostringstream tmp_name;
  string sim_type, priority;
  
  _net    = net;
  _cur_id = 0;

  _sources = _net[0]->NumSources( );
  _dests   = _net[0]->NumDests( );
  
  //nodes higher than limit do not produce or receive packets
  //for default limit = sources

  _limit = config.GetInt( "limit" );
  if(_limit == 0){
    _limit = _sources;
  }
  assert(_limit<=_sources);
 
  duplicate_networks = config.GetInt("physical_subnetworks");

//Flexus-specific method--added by mehdi   
  TimeScale=1.2;	

//MEHDIX 


  int s;


  cntr=cntr2=0;


  #ifdef pointer_def
  Book2Flex       =  new BookPacket**[duplicate_networks];
  Book2Flex_IsFull=  new bool*[duplicate_networks];

  for ( s = 0; s < duplicate_networks; ++s ) 
  {
      Book2Flex[s]=new BookPacket*[_sources];
      
      for ( int a = 0; a < _sources; ++a )
      {
	Book2Flex[s][a]=new BookPacket;
	Book2Flex[s][a]->valid=false;
        
      }	
  }
  for ( s = 0; s < duplicate_networks; ++s ) 
  	 Book2Flex_IsFull[s]= new bool[_sources];

  for ( s = 0; s < duplicate_networks; ++s ) 
  {
	for ( int i = 0; i < _sources; ++i ) 
  	{
		Book2Flex_IsFull[s][i]=0;
	}	
  }
  #else
  

  for ( s = 0; s < 1; ++s ) 
  {
      for ( int a = 0; a < _sources; ++a )
      {
         
	 Book2Flex_IsFull[s][a]=false;
      }
  }

  for ( s = 0; s < _sources; ++s ) 
  {
	NiCBuff[0][s].init(MaxSameSandD);
  }


  #endif
  
   
 /* */
//MEHDIX 
  
  flexin.open("components/BookShim/TraceFile");
  if(!flexin)
  {
      	//cout<<"--[!] unable to open trace file\n";
	//exit(0);
  }
 TraceOut.open("traceOutB.txt");//components/BookShim/trace_Flexus.txt");//, ios::app);
 if(!TraceOut)
  {
      	cout<<"--[!] unable to open trace-out file\n";
	//exit(0);
  }
 StatOut.open("BookSimStatOut.txt");//components/BookShim/trace_Flexus.txt");//, ios::app);
 if(!StatOut)
  {
      	cout<<"--[!] unable to open stat-out file\n";
	//exit(0);
  }
	
  //flexin>>last_packet_time;
  /* mehdi: for debug *///cout<<"++"<<last_packet_time<<"\n";
  //unsigned long last_packet_time;
  //TraceLine TL[MaxPacketPerCycle];
  CurrentPackets=1;
// end of mehdi
 FlxPacketRcv=0;
 FlxFlitRcv=0;
 FlxPacketSnd=0;
 FlxFlitSnd=0;
  // ============ Message priorities ============ 

  config.GetStr( "priority", priority );

  _classes = 1;

  if ( priority == "class" ) {
    _classes  = 2;
    _pri_type = class_based;
  } else if ( priority == "age" ) {
    _pri_type = age_based;
  } else if ( priority == "none" ) {
    _pri_type = none;
  } else {
    Error( "Unknown priority " + priority );
  }

  // ============ Injection VC states  ============ 

  _buf_states = new BufferState ** [_sources];

  for ( s = 0; s < _sources; ++s ) 
  {
    tmp_name << "terminal_buf_state_" << s;
    _buf_states[s] = new BufferState * [duplicate_networks];
    for (int a = 0; a < duplicate_networks; ++a) {
      _buf_states[s][a] = new BufferState( config, this, tmp_name.str( ) );
    }
    tmp_name.seekp( 0, ios::beg );
  }


  // ============ Injection queues ============ 

  _time               = 0;
  #ifdef FlexusDebug
	_time=-1;
  #endif

  _warmup_time        = -1;
  _drain_time         = -1;
  _empty_network      = false;

  _measured_in_flight = 0;
  _total_in_flight    = 0;

  _qtime              = new int *  [_sources];
  _qdrained           = new bool * [_sources];
  _partial_packets    = new list<Flit *> ** [_sources];

  for ( s = 0; s < _sources; ++s ) {
    _qtime[s]           = new int [_classes];
    _qdrained[s]        = new bool [_classes];
    _partial_packets[s] = new list<Flit *> * [_classes];
    for (int a = 0; a < _classes; ++a)
      _partial_packets[s][a] = new list<Flit *> [duplicate_networks];
  }

  _voqing = config.GetInt( "voq" );
  if ( _voqing ) {
    _use_lagging = false;
  } else {
    _use_lagging = true;
  }

  _read_request_size = config.GetInt("read_request_size");
  _read_reply_size = config.GetInt("read_reply_size");
  _write_request_size = config.GetInt("write_request_size");
  _write_reply_size = config.GetInt("write_reply_size");
  if(_use_read_write) {
    _packet_size = (_read_request_size + _read_reply_size +
		    _write_request_size + _write_reply_size) / 2;
  }
  else
    _packet_size = config.GetInt( "const_flits_per_packet" );

  _packets_sent = new int [_sources];
  _batch_size = config.GetInt( "batch_size" );
  _repliesPending = new list<int> [_sources];
  _requestsOutstanding = new int [_sources];
  _maxOutstanding = config.GetInt ("max_outstanding_requests");  

  if ( _voqing ) {
    _voq         = new list<Flit *> * [_sources];
    _active_list = new list<int> [_sources];
    _active_vc   = new bool * [_sources];
  }

  for ( s = 0; s < _sources; ++s ) {
    if ( _voqing ) {
      _voq[s]       = new list<Flit *> [_dests];
      _active_vc[s] = new bool [_dests];
    }
  }

  // ============ Statistics ============ 

  _latency_stats   = new Stats * [_classes];
  _overall_latency = new Stats * [_classes];

  for ( int c = 0; c < _classes; ++c ) {
    tmp_name << "latency_stat_" << c;
    _latency_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
    tmp_name.seekp( 0, ios::beg );

    tmp_name << "overall_latency_stat_" << c;
    _overall_latency[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
    tmp_name.seekp( 0, ios::beg );  
  }

  _pair_latency = new Stats * [_dests];
  _hop_stats    = new Stats( this, "hop_stats", 1.0, 20 );;

  // added by PLotfi
  ((BooksimStats *)global_theBooksimStats)->PairLatency_SumSample = new Stat::StatCounter*[_dests];
  ((BooksimStats *)global_theBooksimStats)->PairLatency_NoSample  = new Stat::StatCounter*[_dests];

  ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_SumSample = new Stat::StatCounter*[_dests];
  ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_NoSample  = new Stat::StatCounter*[_dests]; 
  // end PLotfi

  _accepted_packets = new Stats * [_dests];

  _overall_accepted     = new Stats( this, "overall_acceptance" );
  _overall_accepted_min = new Stats( this, "overall_min_acceptance" );

  for ( int i = 0; i < _dests; ++i ) {
    tmp_name << "pair_stat_" << i;
    _pair_latency[i] = new Stats( this, tmp_name.str( ), 1.0, 250 );
    tmp_name.seekp( 0, ios::beg );

    tmp_name << "accepted_stat_" << i;
    _accepted_packets[i] = new Stats( this, tmp_name.str( ) );
    tmp_name.seekp( 0, ios::beg );   

    // added by PLotfi
    string Number = boost::lexical_cast<string>( i );
    ((BooksimStats *)global_theBooksimStats)->PairLatency_SumSample[i] = new Stat::StatCounter(((BooksimStats *)global_theBooksimStats)->theInitialName + "-Booksim:PairLatency:" + Number + ":SumSample");
    ((BooksimStats *)global_theBooksimStats)->PairLatency_NoSample[i]  = new Stat::StatCounter(((BooksimStats *)global_theBooksimStats)->theInitialName + "-Booksim:PairLatency:" + Number + ":NoSample");
    
    ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_SumSample[i] = new Stat::StatCounter(((BooksimStats *)global_theBooksimStats)->theInitialName + "-Booksim:AcceptedPackets:" + Number + ":SumSample");
    ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_NoSample[i]  = new Stat::StatCounter(((BooksimStats *)global_theBooksimStats)->theInitialName + "-AcceptedPackets:" + Number + ":NoSample");
    // end Plotfi
  }

  deadlock_counter = 1;

  // ============ Simulation parameters ============ 

  if(config.GetInt( "injection_rate_uses_flits" )) {
    _flit_rate = config.GetFloat( "injection_rate" ); 
    _load = _flit_rate / _packet_size;
  } else {
    _load = config.GetFloat( "injection_rate" ); 
    _flit_rate = _load * _packet_size;
  }

  _total_sims = config.GetInt( "sim_count" );

  _internal_speedup = config.GetFloat( "internal_speedup" );
  _partial_internal_cycles = new float[duplicate_networks];
  class_array = new short* [duplicate_networks];
  for (int i=0; i < duplicate_networks; ++i) {
    _partial_internal_cycles[i] = 0.0;
    class_array[i] = new short [_classes];
    memset(class_array[i], 0, sizeof(short)*_classes);
  }

  _traffic_function  = GetTrafficFunction( config );
  _routing_function  = GetRoutingFunction( config );
  _injection_process = GetInjectionProcess( config );

  config.GetStr( "sim_type", sim_type );

  if ( sim_type == "latency" ) {
    _sim_mode = latency;
  } else if ( sim_type == "throughput" ) {
    _sim_mode = throughput;
  }  else if ( sim_type == "batch" ) {
    _sim_mode = batch;
  }  else if (sim_type == "timed_batch"){
    _sim_mode = batch;
    timed_mode = true;
  }//added by mehdi
  else if ( sim_type == "flexus" ) {
    _sim_mode = Flexmode;
  }//end of mehdi 
  else{
    Error( "Unknown sim_type " + sim_type );
  }

  _sample_period = config.GetInt( "sample_period" );
  _max_samples    = config.GetInt( "max_samples" );
  _warmup_periods = config.GetInt( "warmup_periods" );
  _latency_thres = config.GetFloat( "latency_thres" );
  _include_queuing = config.GetInt( "include_queuing" );

  _print_csv_results = config.GetInt( "print_csv_results" );
  config.GetStr( "traffic", _traffic ) ;
  _drain_measured_only = config.GetInt( "drain_measured_only" );
  _LoadWatchList();

   //packet length
   Packet_length_data = config.GetInt( "packet_length_data" );
   Packet_length_ctrl = config.GetInt( "packet_length_ctrl" );

   //cout<<Dplen<<" "<< Cplen<<"\n";int fl;cin>>fl;


   cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% BookSim initialized "<<"\n\n";




   
}

TrafficManager::~TrafficManager( )
{
  for ( int s = 0; s < _sources; ++s ) {
    delete [] _qtime[s];
    delete [] _qdrained[s];
    for (int a = 0; a < duplicate_networks; ++a) {
      delete _buf_states[s][a];
    }
    if ( _voqing ) {
      delete [] _voq[s];
      delete [] _active_vc[s];
    }
    for (int a = 0; a < _classes; ++a) {
      delete [] _partial_packets[s][a];
    }
    delete [] _partial_packets[s];
    delete [] _buf_states[s];
  }
  if ( _voqing ) {
    delete [] _voq;
    delete [] _active_vc;
  }
  delete [] _buf_states;
  delete [] _qtime;
  delete [] _qdrained;
  delete [] _partial_packets;

  for ( int c = 0; c < _classes; ++c ) {
    delete _latency_stats[c];
    delete _overall_latency[c];
  }
  for (int i = 0; i < duplicate_networks; ++i) {
    delete [] class_array[i];
  }
  delete[] class_array;

  delete [] _latency_stats;
  delete [] _overall_latency;

  delete _hop_stats;
  delete _overall_accepted;
  delete _overall_accepted_min;

  for ( int i = 0; i < _dests; ++i ) {
    delete _accepted_packets[i];
    delete _pair_latency[i];
  }

  delete [] _accepted_packets;
  delete [] _pair_latency;
  delete [] _partial_internal_cycles;


  //DebugOut.close();
}


// Decides which subnetwork the flit should go to.
// Is now called once per packet.
// This should change according to number of duplicate networks
int TrafficManager::DivisionAlgorithm (int packet_type) {
//mehdi: directs packets to different networks based on their types 

  if(packet_type == Flit::ANY_TYPE) {
    static unsigned short counter = 0;
    return (counter++)%duplicate_networks; // Even distribution.
  } else {
    switch(duplicate_networks) {
    case 1:
      return 0;
    case 2:
      switch(packet_type) {
      case Flit::WRITE_REQUEST:
      case Flit::READ_REQUEST:
        return 1;
      case Flit::WRITE_REPLY:
      case Flit::READ_REPLY:
        return 0;
      default:
	assert(false);
	return -1;
      }
    case 4:
      switch(packet_type) {
      case Flit::WRITE_REQUEST:
        return 0;
      case Flit::READ_REQUEST:
        return 1;
      case Flit::WRITE_REPLY:
        return 2;
      case Flit::READ_REPLY:
        return 3;
      default:
	assert(false);
	return -1;
      }
    default:
      assert(false);
      return -1;
    }
  }
} 

Flit *TrafficManager::_NewFlit( )
{
  Flit *f;
  //the constructor should initialize everything
  f = new Flit();
  f->id    = _cur_id;
  _in_flight[_cur_id] = true;
  if(flits_to_watch.size()!=0   && 
     flits_to_watch.find(_cur_id)!=flits_to_watch.end()){
    f->watch =true;
    map<int, bool>::iterator match;
    match = flits_to_watch.find( f->id );
    flits_to_watch.erase(match);
  }
  ++_cur_id;
  return f;
}

void TrafficManager::DeliverData(long int data  )//by Monsieur Mehdi
{
	//DebugOut<<"-- "<<data<<"\n";
}



void TrafficManager::_RetireFlit( Flit *f, int dest )
{
  static int sample_num = 0;
  deadlock_counter = 1;

  map<int, bool>::iterator match;

  match = _in_flight.find( f->id );

  if ( match != _in_flight.end( ) ) {
    if ( f->watch ) {
      cout << "Matched flit ID = " << f->id << endl;
    }
    _in_flight.erase( match );
  } else {
    cout << "Unmatched flit! ID = " << f->id << endl;
    Error( "" );
  }
  
if ( f->watch ) 
 { 
    cout << "\n+++Ejecting flit " << f->id 
	 << ",  lat = " << _time - f->time
	 <<" from "<<_time<<" to " << f->time
	 << ", src = " << f->src 
	 << ", dest = " << f->dest << endl;
  }

  if ( f->head && ( f->dest != dest ) ) {
    cout << "At output " << dest << endl;
    cout << *f;
    Error( "Flit arrived at incorrect output" );
  }

  if ( f->tail ) {
    _total_in_flight--;
    
      

     //MEHDIX 
     //added by mehdi

     //Book2Flex[0][dest]=*(f->FlexData[0]);
     
     int destFlex = (f->FlexData[0]->destNode)%16;

     for(int mg=0; mg< f->PLlength ; mg++)
     {
	//cout<<"\nooooo "<<destFlex<<" "<<NiCBuff[0][destFlex].MaxSize<<" "<<MaxSameSandD<<"\n";//int g;cin>>g;

	if(!NiCBuff[0][destFlex].isfull())//if(!NiCBuff[0][f->FlexData[0]->destNode].isfull())//NiCBuff[1][NumberOfNodes]
	{

		NiCBuff[0][destFlex].push(f->FlexData[mg]);//NiCBuff[0][f->FlexData[0]->destNode].push(f->FlexData[mg]);
		if(f->head == true && (destFlex >= _sources || destFlex<0))
		{
			cout<<"!"<<destFlex;
			cout<<" Ejection NIC, invalid destination node";int g;cin>>g;
			exit(0);			
		}
	}
	else
	{
		//cout<<sizeof(int)<<" "<<sizeof(NiCBuff[1][1]);
		cout<<"\n--"<<destFlex<<" "<<NiCBuff[0][destFlex].size<<" "<<NiCBuff[0][destFlex].MaxSize<<"\n";
		cout<<" Ejection NIC, NIC overflow";int g;cin>>g;
		exit(0);
	}
	//cout<<" "<< <<" + ";
	
     }/**/

 /*    cout<<"\n**** "<<f->id<<" : ";
     for(int hr=0;hr<f->PLlength;hr++)
		cout<<" "<<f->FlexData[hr]->srcNode<<" "<<f->FlexData[hr]->destNode<<" + ";
     cout<<"\n*****";
*/
    
#ifdef echo
 cout<<"\n---booksim sent: at "<< Book2Flex[0][dest].srcNode<<" from "<< Book2Flex[0][dest].destNode<<" serial "<< Book2Flex[0][dest].serial;
     //end of mehdi
#endif    
#ifdef NetLog

	TraceOut<<"+     "<<Book2Flex[0][dest].serial<<" "<<Book2Flex[0][dest].srcNode<<" "<<Book2Flex[0][dest].destNode<<" "<<Book2Flex[0][dest].priority<<"\n";
	//TraceOut<<_time<<" "<<PacketFromFlex[input]->srcNode<<" "<<PacketFromFlex[input]->destNode<<" "<<PacketFromFlex[input]->transmitLatency<<"\n";
	//TraceOut<<_time<<" "<<PacketFromFlex[input]->srcNode<<" "<<PacketFromFlex[input]->destNode<<" "<<PacketFromFlex[input]->transmitLatency<<"\n";
#endif


    if ( _total_in_flight < 0 ) {
      Error( "Total in flight count dropped below zero!" );
    }
    
    //code the source of request, look carefully, its tricky ;)
    if (f->type == Flit::READ_REQUEST) {
      Packet_Reply* temp = new Packet_Reply;
      temp->source = f->src;
      temp->time = _time;
      temp->type = f->type;
      _repliesDetails[f->id] = temp;
      _repliesPending[dest].push_back(f->id);
    } else if (f->type == Flit::WRITE_REQUEST) {
      Packet_Reply* temp = new Packet_Reply;
      temp->source = f->src;
      temp->time = _time;
      temp->type = f->type;
      _repliesDetails[f->id] = temp;
      _repliesPending[dest].push_back(f->id);
    } else if(f->type == Flit::READ_REPLY || f->type == Flit::WRITE_REPLY  ){
      //received a reply
      _requestsOutstanding[dest]--;
    } else if(f->type == Flit::ANY_TYPE && _sim_mode == batch  ){
      //received a reply
      _requestsOutstanding[f->src]--;
    }


    // Only record statistics once per packet (at tail)
    // and based on the simulation state1
    if ( ( _sim_state == warming_up ) || f->record ) {
      
      _hop_stats->AddSample( f->hops );
      ((BooksimStats *)global_theBooksimStats)->HopStats_SumSample += f->hops;
      ((BooksimStats *)global_theBooksimStats)->HopStats_NoSample ++;

      switch( _pri_type ) {
      case class_based:
	_latency_stats[f->pri]->AddSample( _time - f->time );
	break;
      case age_based: // fall through
      case none:
	_latency_stats[0]->AddSample( _time - f->time);
        ((BooksimStats *)global_theBooksimStats)->LatencyStats_SumSample += (_time - f->time);
        ((BooksimStats *)global_theBooksimStats)->LatencyStats_NoSample ++;
	break;
      }
      ++sample_num;
   
      if ( f->src == 0 ) {
	_pair_latency[dest]->AddSample( _time - f->time);
        // added by Plotfi
        ((BooksimStats *)global_theBooksimStats)->PairLatency_SumSample[dest] += (_time - f->time);
        ((BooksimStats *)global_theBooksimStats)->PairLatency_NoSample[dest] ++;
        // end PLotfi
      }
      
      if ( f->record ) {
	_measured_in_flight--;
	if ( _measured_in_flight < 0 ){ 
	  Error( "Measured in flight count dropped below zero!" );
	}
      }
    }
    FlxPacketRcv++;
  }
 


  FlxFlitRcv++;
  delete f;
}

int TrafficManager::_IssuePacket( int source, int cl ) const
{

//   mehdi: in Flexux we only use the following settings:
//          _sime_mod = injection 
//          traffic mod= normal (not _use_read_write)
//

  int result;
  int pending_time = INT_MAX; //reset to maxtime+1
//mehdi ***************************************************************************** batch mode
  if(_sim_mode == batch){ //batch mode
    if(_use_read_write){ //read write packets
      //check queue for waiting replies.
      //check to make sure it is on time yet
      if (!_repliesPending[source].empty()) {
	result = _repliesPending[source].front();
	pending_time = (_repliesDetails.find(result)->second)->time;
      }
      if (pending_time<=_qtime[source][0]) {
	result = _repliesPending[source].front();
	_repliesPending[source].pop_front();
	
      } else if ((_packets_sent[source] >= _batch_size && !timed_mode) || 
		 (_requestsOutstanding[source] >= _maxOutstanding)) {
	result = 0;
      } else {
	
	//coin toss to determine request type.
	result = -1;
	
	if (RandomFloat() < 0.5) {
	  result = -2;
	}
	
	_packets_sent[source]++;
	_requestsOutstanding[source]++;
      } 
    } else { //normal
      if ((_packets_sent[source] >= _batch_size && !timed_mode) || 
		 (_requestsOutstanding[source] >= _maxOutstanding)) {
	result = 0;
      } else {
	result = gConstPacketSize;
	_packets_sent[source]++;
	//here is means, how many flits can be waiting in the queue
	_requestsOutstanding[source]++;
      } 
    } 
//mehdi ***************************************************************************** injection mode

  } else { //injection rate mode
    if(_use_read_write){ //use read and write
      //check queue for waiting replies.
      //check to make sure it is on time yet
      if (!_repliesPending[source].empty()) {
	result = _repliesPending[source].front();
	pending_time = (_repliesDetails.find(result)->second)->time;
      }
      if (pending_time<=_qtime[source][0]) {
	result = _repliesPending[source].front();
	_repliesPending[source].pop_front();
      } else {
	result = _injection_process( source, _load );
	//produce a packet
	if(result){
	  //coin toss to determine request type.
	  result = -1;
	
	  if (RandomFloat() < 0.5) {
	    result = -2;
	  }
	}
      } 
    } else { //normal mode
      return _injection_process( source, _load );
    } 
  }
  return result;
}

void TrafficManager::_GeneratePacket( int source, int stype, 
				      int cl, int time )
{
  cout<<"this function is no longer useful";
}

void TrafficManager::Flex_GeneratePacket( int source, int packet_destination, 
				      int cl, int time, int Psize )
{

 cout<<"\n----Error: Use the updatetd version of Flex_GeneratePacket()\n";
 exit(0);
}







void TrafficManager::_FirstStep( )
{  
  //cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% _FirstStep starts\n\n";
  // Ensure that all outputs are defined before starting simulation
  for (int i = 0; i < duplicate_networks; ++i) { 
    _net[i]->WriteOutputs( );

   //cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% _FirstStep at mid\n\n";

  
    for ( int output = 0; output < _net[i]->NumDests( ); ++output ) {
      _net[i]->WriteCredit( 0, output );
    }
  }
}

void TrafficManager::_BatchInject()
{
  cout<<"\nI am no longer useful in the flexus version of Booksim, but Mr. Modarressi didnt remove me completly in order to keep the TrafficManagere class unchanged\n";
  exit(0);
}







//************************************************************************************************************************************
//************************************************************************************************************************************
//Flexus-specific method--added by mehdi   
void TrafficManager::_FileInject(){

  
}


//mehdi:
//    _buf_states is the buffer state of the next router down the channel
//     tracks the credit and how much of the buffer is in use, for this case it tracks the injection port buffer status
//    _partial_packets is the data structre actually keeps the packets


void TrafficManager::_NormalInject(){

 
}



void TrafficManager::_Step()
{

}





bool TrafficManager::_PacketsOutstanding( ) const
{
  bool outstanding;

  if ( _measured_in_flight == 0 ) {
    outstanding = false;

    for ( int c = 0; c < _classes; ++c ) {
      for ( int s = 0; s < _sources; ++s ) {
	if ( !_qdrained[s][c] ) {
#ifdef DEBUG_DRAIN
	  cout << "waiting on queue " << s << " class " << c;
	  cout << ", time = " << _time << " qtime = " << _qtime[s][c] << endl;
#endif
	  outstanding = true;
	  break;
	}
      }
      if ( outstanding ) { break; }
    }
  } else {
#ifdef DEBUG_DRAIN
    cout << "in flight = " << _measured_in_flight << endl;
#endif
    outstanding = true;
  }

  return outstanding;
}

void TrafficManager::_ClearStats( )
{
  for ( int c = 0; c < _classes; ++c ) {
    _latency_stats[c]->Clear( );
  }
  
  for ( int i = 0; i < _dests; ++i ) {
    _accepted_packets[i]->Clear( );
    _pair_latency[i]->Clear( );
  }
}

int TrafficManager::_ComputeAccepted( double *avg, double *min ) const 
{
  int dmin=0;

  *min = 1.0;
  *avg = 0.0;

  for ( int d = 0; d < _dests; ++d ) {
    if ( _accepted_packets[d]->Average( ) < *min ) {
      *min = _accepted_packets[d]->Average( );
      dmin = d;
    }
    *avg += _accepted_packets[d]->Average( );
  }

  *avg /= (double)_dests;

  return dmin;
}

void TrafficManager::_DisplayRemaining( ) const 
{
  cout << "Remaining flits (" << _measured_in_flight << " measurement packets, "
       << _total_in_flight << " total) : ";

  map<int, bool>::const_iterator iter;
  int i;
  for ( iter = _in_flight.begin( ), i = 0;
	( iter != _in_flight.end( ) ) && ( i < 10 );
	iter++, i++ ) {
    cout << iter->first << " ";
  }

  i = _in_flight.size();
  if(i > 10)
    cout << "[...] (" << i << " flits total)";

  cout << endl;

}







bool TrafficManager::_SingleSim( )
{
  int  iter=0;
  int  total_phases=0;
  int  converged=0;
  int  empty_steps=0;
  
  double cur_latency=0.0;
  double prev_latency=0.0;

  double cur_accepted=0.0;
  double prev_accepted=0.0;

  double warmup_threshold=0.0;
  double stopping_threshold=0.0;
  double acc_stopping_threshold=0.0;

  double min=0.0;
  double avg=0.0;

  bool   clear_last=false;

  int mm_cntr1=0, mm_cntr2=0;// added by mehdi

  _time = 0;
  //remove any pending request from the previous simulations
  for (int i=0;i<_sources;i++) {
    _packets_sent[i] = 0;
    _requestsOutstanding[i] = 0;

    while (!_repliesPending[i].empty()) {
      _repliesPending[i].pop_front();
    }
  }

  //reset queuetime for all sources
  for ( int s = 0; s < _sources; ++s ) {
    for ( int c = 0; c < _classes; ++c  ) {
      _qtime[s][c]    = 0;
      _qdrained[s][c] = false;
    }
  }

  stopping_threshold     = 0.05;
  acc_stopping_threshold = 0.05;
  warmup_threshold       = 0.05;
  iter            = 0;
  converged       = 0;
  total_phases    = 0;

  // warm-up ...
  // reset stats, all packets after warmup_time marked
  // converge
  // draing, wait until all packets finish
  _sim_state    = running;//warming_up;
  total_phases  = 0;
  prev_latency  = 0;
  prev_accepted = 0;

  _ClearStats( );
  clear_last    = false;

  if (_sim_mode == batch && timed_mode){
    while(_time<_sample_period){
      _Step();
      if ( _time % 10000 == 0 ) {
	cout <<_sim_state<< "%=================================" << endl;
	int dmin;
	cur_latency = _latency_stats[0]->Average( );
	dmin = _ComputeAccepted( &avg, &min );
	cur_accepted = avg;
	
	cout << "% Average latency = " << cur_latency << endl;
	cout << "% Accepted packets = " << min << " at node " << dmin << " (avg = " << avg << ")" << endl;
	cout << "lat(" << total_phases + 1 << ") = " << cur_latency << ";" << endl;
	_latency_stats[0]->Display();
      } 
    }
    cout<<"Total inflight "<<_total_in_flight<<endl;
    converged = 1;

  } else if(_sim_mode == batch && !timed_mode){//batch mode   
    while(_packets_sent[0] < _batch_size){
      _Step();
      if ( _time % 1000 == 0 ) {
	cout <<_sim_state<< "%=================================" << endl;
	int dmin;
	cur_latency = _latency_stats[0]->Average( );
	dmin = _ComputeAccepted( &avg, &min );
	cur_accepted = avg;
	
	cout << "% Average latency = " << cur_latency << endl;
	cout << "% Accepted packets = " << min << " at node " << dmin << " (avg = " << avg << ")" << endl;
	cout << "lat(" << total_phases + 1 << ") = " << cur_latency << ";" << endl;
	_latency_stats[0]->Display();
      }
    }
    cout << "batch size of "<<_batch_size  <<  " sent. Time used is " << _time << " cycles" <<endl;
    cout<< "Draining the Network...................\n";
    empty_steps = 0;
    while( (_drain_measured_only ? _measured_in_flight : _total_in_flight) > 0 ) { 
      _Step( ); 
      ++empty_steps;
      
      if ( empty_steps % 1000 == 0 ) {
	_DisplayRemaining( ); 
	cout << ".";
      }
    }
    cout << endl;
    cout << "batch size of "<<_batch_size  <<  " received. Time used is " << _time << " cycles" <<endl;
    converged = 1;
  } 
  else if(_sim_mode == Flexmode)
  {

      _ClearStats( );

      cout<<"\n Entring Flexus Mode (overall "<<_sample_period<<" cycles), Round: "<< mm_cntr2++<<"\n";
      
/*

      for ( iter = 0; iter < _sample_period; ++iter ) 
      { 
	_Step( ); 
	mm_cntr1++;
      } 


     

	cout<<"\n <><><><><><><><><><><><><><><><> simulation cycles "<< mm_cntr1<<"\n";
	cout<<"\nSent Packets: "<<FlxPacketSnd<<" Sent Flits: "<<FlxFlitSnd<<" Received Packets: "<<FlxPacketRcv<<" Received Flits: "<<FlxFlitRcv<<endl;
	

      int dmin;
      cur_latency = _latency_stats[0]->Average( );
      dmin = _ComputeAccepted( &avg, &min );
      cur_accepted = avg;
      cout << "% Average latency = " << cur_latency << endl;
      cout << "% Accepted packets = " << min << " at node " << dmin << " (avg = " << avg << ")" << endl;
      cout << "lat(" << total_phases + 1 << ") = " << cur_latency << ";" << endl;
     */
     
	
  }//end of flexus mode
 

  //return ( converged > 0 );
    return 1;//by mehdi
}

bool TrafficManager::Run( )
{
  double min, avg;
  int total_packets = 0;
   //cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Run started\n\n";

  _FirstStep( );
    // cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Run at the mid\n\n";

  _SingleSim( );

   //cout<<"\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Run Finished\n\n";


  
 
  return true;
}

void TrafficManager::DisplayStats() {
  for ( int c = 0; c < _classes; ++c ) {

    if(_print_csv_results) {
      cout << "results:"
	   << c
	   << "," << _traffic
	   << "," << _packet_size
	   << "," << _load
	   << "," << _flit_rate
	   << "," << _overall_latency[c]->Average( )
	   << "," << _overall_accepted_min->Average( )
	   << "," << _overall_accepted_min->Average( )
	   << "," << _hop_stats->Average( )
	   << endl;
    }

    cout << "====== Traffic class " << c << " ======" << endl;
    
    cout << "Overall average latency = " << _overall_latency[c]->Average( )
	 << " (" << _overall_latency[c]->NumSamples( ) << " samples)" << endl;
    
    cout << "Overall average accepted rate = " << _overall_accepted->Average( )
	 << " (" << _overall_accepted->NumSamples( ) << " samples)" << endl;
    
    cout << "Overall min accepted rate = " << _overall_accepted_min->Average( )
	 << " (" << _overall_accepted_min->NumSamples( ) << " samples)" << endl;
    
  }

  cout << "Average hops = " << _hop_stats->Average( )
       << " (" << _hop_stats->NumSamples( ) << " samples)" << endl;
  
}

//read the watchlist
void TrafficManager::_LoadWatchList(){
  ifstream watch_list;
  string line;
  string delimiter = ",";
  watch_list.open(watch_file.c_str());

  if(watch_list.is_open()){
    while(!watch_list.eof()){
      getline(watch_list,line);
      if(line!=""){
	flits_to_watch[atoi(line.c_str())]= true;
      }
    }

  } else {
    //cout<<"Unable to open flit watch file, continuing with simulation\n";
  }
}

//pqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpqpq
//bdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbdbd

/*
int TrafficManager::Flxus_Available(int FlxNode, int FlxClass, int FlxSubnet)
{
	if ( _partial_packets[FlxNode][FlxClass][FlxSubnet].max_size() == _partial_packets[FlxNode][FlxClass][FlxSubnet].size() )
	{	
		return 1;
	}
	else
		return 0;
       //return (!_partial_packets[FlxNode][FlxClass][FlxSubnet].empty( ));
}
*/
int TrafficManager::Flxus_PushPacket(int FlxNode, int FlxClass, int FlxSubnet)
{
	return 0;
}
int TrafficManager::Flxus_DeliverPacket(int FlxNode, int FlxClass, int FlxSubnet)
{
	return 0;
}







void TrafficManager::Flex_GeneratePacket( int source, int packet_destination, int cl, int time, int Psize, int payload_size, BookPacket ** PacketFromFlex )
{

  //cout<<"\nsize: "<<Psize<<"\n";
  //refusing to generate packets for nodes greater than limit
  source=source%16;
  packet_destination=packet_destination%16;

  if(source >=_limit){
    return ;
  }
  if ((packet_destination <0) || (packet_destination >= _net[0]->NumDests())) {
    cout << "Wrong Packet destination " << packet_destination  << endl;
    Error("Incorrect destination");
  } 

  Flit *f;
  bool record;
  Flit::FlitType packet_type;
  //int size; //input size 

  //use uniform packet size
  packet_type = Flit::ANY_TYPE;
  //size =  Psize;//mehdi
  //packet_destination = _traffic_function( source, _limit ); //commented by mehdi

  if ( ( _sim_state == running ) ||
       ( ( _sim_state == draining ) && ( time < _drain_time ) ) ) {
    ++_measured_in_flight;
    
    record = true;
  } else {
    record = false;
  }
  ++_total_in_flight;

  sub_network = DivisionAlgorithm(packet_type);// determine the subnetwork

  FlxPacketSnd++;

	
  if (Psize>1)
	Psize=Packet_length_data;
  
  if (Psize==1)
	Psize=Packet_length_ctrl;
  //cout<<"\n----"<<Psize;

  for ( int i = 0; i < Psize; ++i ) 
  {//mehdi: generate a "Psize"-flit packet
  	f = _NewFlit( );
    	f->subnetwork = sub_network;
    	f->src    = source;
   	f->time   = time;
    	f->record = record;
	FlxFlitSnd++;
    
    	if(_trace || f->watch)
	{
      		cout<<"New Flit "<<f->src<<endl;
    	}
    	f->type = packet_type;

    	if ( i == 0 ) 
	{ // Head flit
      		f->head = true;
      		//packets are only generated to nodes smaller or equal to limit
      		f->dest = packet_destination;
    	} 
	else 
	{
      		f->head = false;
      		f->dest = -1;
    	}
    	switch( _pri_type ) 
	{
    		case class_based:
      			f->pri = cl; break;
    		case age_based://fall through
    		case none:
     	 		f->pri = 0; break;
    	}

    	if ( i == ( Psize - 1 ) )
	{ // Tail flit
      		f->tail = true;
		f->data=time;
                if(PacketFromFlex!=0)
                {
                        f->FlexData = new BookPacket*[payload_size];        
			for(int mg=0;mg<payload_size;mg++)
				f->FlexData[mg]=PacketFromFlex[mg];
		}
		else
		{
			cout<<"******** invalid data when generating packet";
			exit(0);
		}
		f-> PLlength = payload_size;

               //cout<<"\n ???? "<<(f->FlexData[0]->destNode)%16<<"  "<< packet_destination<<"\n";
		//f->destFlex=packet_destination;

		/*cout<<"\n----"<<f->id<<" : ";
		for(int mg=0;mg<payload_size;mg++)
			cout<<" "<<f->FlexData[mg]->srcNode<<" "<<f->FlexData[mg]->destNode<<" + ";
                cout<<"\n----";*/
		//cout<<"\n++++";
		
    	} 
	else 
	{
      		f->tail = false;
		//cout<<"\n----";
    	}	
    
   	f->vc  = -1;
#ifdef FlexusDebug
	//cout << "Generating flit at time " << time << ": ";
      	//cout << *f<<"\n";
#endif

    	if( f->watch ) 
	{ 
      		cout << "Generating flit at time " << time << endl;
      		cout << *f;
    	}

    	_partial_packets[source][cl][sub_network].push_back( f );
  }//end of for
}







void TrafficManager::_Step( BookPacket ** PacketFromFlex, int IndexFromFlex, int FlexTime)
{
#ifdef echo
cout<<"\n<><><><><><><><><><><><><><><><><> time: "<<_time;//<<"\n";
#endif	
          int ss,sss=0; 
        
        for (sss = 0; sss < duplicate_networks; ++sss) 
        {
             // Eject traffic and send credits
             for ( int ssss = 0; ssss < _net[0]->NumDests( ); ++ssss ) {
    
               	if(Book2Flex_IsFull[sss][ssss]==false)
                   	cntr2++;
		}
         }

        sss=0;
   if(deadlock_counter++ == 0)
   {
      cout << "WARNING: Low traffic: Low traffic generation rate or possible network deadlock.\n";
   }

  Flit   *f;
  Credit *cred;


//this part is modified by mehdi
  
    	if(_sim_mode == Flexmode)
    		_FlexInject( PacketFromFlex,  IndexFromFlex);
	else
		_FileInject(); 
 
//end of mehdi


 
  //advance networks
  for (int i = 0; i < duplicate_networks; ++i) {
    _net[i]->ReadInputs( );
    _partial_internal_cycles[i] += _internal_speedup;
    while( _partial_internal_cycles[i] >= 1.0 ) {
      _net[i]->InternalStep( );
      _partial_internal_cycles[i] -= 1.0;
    }
  }

  for (int a = 0; a < duplicate_networks; ++a) {
    _net[a]->WriteOutputs( );
  }
  
   //++_time;//by mehdi
   _time=FlexTime;
  //cout<<"TIME::: "<<FlexTime<<endl;//by mehdi for debug

  if(FlexTime==MaxCyclePerFlexpoint-1)//149999
  {
      cout<<"\n\n\n\n\n\n\n+++++++++++++++++++++++++++++++++++\n time:"<<FlexTime<<"\nSent Packets: "<<FlxPacketSnd<<" Sent Flits: "<<FlxFlitSnd<<" Received Packets: "<<FlxPacketRcv<<" Received Flits: "<<FlxFlitRcv<<endl;
      double cur_latency=0.0;
      //int dmin;
      cur_latency = _latency_stats[0]->Average( );
      cout << "% Average Packet latency = " << cur_latency<<"\n";// <<"   counter: "<<cntr<<"   counter2: "<<cntr2<< endl;    
      cout<<"\n\n\n\n\n\n\n";

        int ams=0;
	double total_xbar=0, total_buff=0, total_salloc=0, total_vcalloc=0, total_Eclock=0, total_link=0;
      	for( ams = 0; ams < _net[0]->NumDests( ); ++ams)//SIM_crossbar_report(SIM_crossbar_t *crsbar)
      	{
		double xbar    =SIM_crossbar_report(&(_net[0]->_routers[ams]->router_power.crossbar))* (PARM_Freq/FlexTime);
		total_xbar+=xbar;
		double buff    =SIM_array_power_report(&(_net[0]->_routers[ams]->router_info.in_buf_info) , &(_net[0]->_routers[ams]->router_power.in_buf))* (PARM_Freq/FlexTime);
		total_buff+=buff;
		double salloc  =SIM_arbiter_report(&(_net[0]->_routers[ams]->router_power.sw_out_arb))* (PARM_Freq/FlexTime);
		total_salloc+=salloc;
		double vcalloc =SIM_arbiter_report(&(_net[0]->_routers[ams]->router_power.vc_out_arb))* (PARM_Freq/FlexTime);
		total_vcalloc+=vcalloc;
		double Eclock  =SIM_total_clockEnergy(&(_net[0]->_routers[ams]->router_info), &(_net[0]->_routers[ams]->router_power) )* PARM_Freq;
		total_Eclock+=Eclock;
		
                cout<<"\n\n-- "<<ams<<" Xbar= "<<xbar<<" Buff:"<<buff<<" SW allocator: "<<salloc<<" VC allocator: "<<vcalloc<<"\nlink:";//<<"clock (per cycle): "<<Eclock
		for(int mgg=0;mgg<_net[0]->_routers[ams]->_outputs;mgg++)
		{
			double temp=_net[0]->_routers[ams]->link_power[mgg]* (PARM_Freq/FlexTime);
			cout<<temp<<"   +   ";
			total_link+=temp;
		}
	}
	
	double buf_Pstatic         = _net[0]->_routers[0]->router_power.I_buf_static         * Vdd * SCALE_S * _net[0]->NumDests( );
	double xbar_Pstatic        = _net[0]->_routers[0]->router_power.I_crossbar_static    * Vdd * SCALE_S * _net[0]->NumDests( );
	double sw_arbiter_Pstatic  = _net[0]->_routers[0]->router_power.I_sw_arbiter_static  * Vdd * SCALE_S * _net[0]->NumDests( );
	double vc_arbiter_Pstatic  = _net[0]->_routers[0]->router_power.I_vc_arbiter_static  * Vdd * SCALE_S * _net[0]->NumDests( );
	double clk_Pstatic	   = _net[0]->_routers[0]->router_power.I_clock_static       * Vdd * SCALE_S * _net[0]->NumDests( );
	double link_Pstatic        = _net[0]->_routers[0]->LinkStatic *  _net[0]->_routers[0]->router_info.flit_width * _net[0]->_channels;// LinkLeakagePowerPerMeter(_net[0]->_routers[0]->link_len, Vdd)



	double total_static_power=   buf_Pstatic+ xbar_Pstatic+ sw_arbiter_Pstatic+ vc_arbiter_Pstatic+ clk_Pstatic+ link_Pstatic;
	double total_dynamic_power=  total_xbar+ total_buff+ total_salloc+ total_vcalloc+ total_Eclock+ total_link;

        

	cout<<"\n\n Dynamic Power "<<"\nCrossbar Dynamic Power: "<<total_xbar<<"\nBuffer Dynamic Power: "<<total_buff<<"\nSW allocator Dynamic Power: "<<total_salloc<<"\nVC allocator Dynamic Power : "<<total_vcalloc<<"\nLink Dynamic Power: "<< total_link<<"\n";//"\nClock Dynamic Power: "<<total_Eclock<<"\n";

	
	cout<<"\n\n static Power"<<"\nCrossbar Static Power: "<<xbar_Pstatic<<"\nBuffer Static Power: "<<buf_Pstatic<<"\nSW allocator Static Power: "<<sw_arbiter_Pstatic<<"\nVC allocator Static Power: "<<vc_arbiter_Pstatic<<"\nLink Static Power: "<< link_Pstatic<<"\n";//"Clock Static Power: "<<clk_Pstatic<<"\n";

        
	/*cout<<"\n\n Total Power "<<"Crossbar Total Power: "<<total_xbar<<" \nBuffer Total Power: "<<total_buff<<"\nSW allocator Total Power: "<<total_salloc<<"\nVC allocator Total Power : "<<total_vcalloc<<"\nLink Total Power: "<< total_link<<"\nClock Total Power: "<<total_Eclock<<"\n";*/

	cout.flush();

	

	StatOut<<"\n% Average network latency = " << cur_latency <<"\n"; 
        StatOut<<"\n time:"<<FlexTime<<"\nSent Packets: "<<FlxPacketSnd<<" Sent Flits: "<<FlxFlitSnd<<" Received Packets: "<<FlxPacketRcv<<" Received Flits: "<<FlxFlitRcv<<endl;

	StatOut<<"\n\n Dynamic Power "<<"\nCrossbar Dynamic Power: "<<total_xbar<<"\nBuffer Dynamic Power: "<<total_buff<<"\nSW allocator Dynamic Power: "<<total_salloc<<"\nVC allocator Dynamic Power : "<<total_vcalloc<<"\nLink Dynamic Power: "<< total_link<<"\nClock Dynamic Power: "<<total_Eclock<<"\n";

	
	StatOut<<"\n\n static Power"<<"\nCrossbar Static Power: "<<xbar_Pstatic<<"\nBuffer Static Power: "<<buf_Pstatic<<"\nSW allocator Static Power: "<<sw_arbiter_Pstatic<<"\nVC allocator Static Power: "<<vc_arbiter_Pstatic<<"\nLink Static Power: "<< link_Pstatic<<"\nClock Static Power: "<<clk_Pstatic<<"\n";
     	StatOut.flush();

  }



  if(_trace)
  {
    cout<<"TIME::: "<<_time<<endl;
  }


  for (int i = 0; i < duplicate_networks; ++i) 
  {
    // Eject traffic and send credits
    for ( int output = 0; output < _net[0]->NumDests( ); ++output ) 
    {
    	//MEHDIX 
	//added by mehdi 
       if(Book2Flex_IsFull[i][output]==false)//1)//Book2Flex[i][output]->valid==false)//Book2Flex_IsFull[0][output]==0)//1)//
       {	
                cntr++;                               
       	 	f = _net[i]->ReadFlit( output );
		
		
        	if ( f ) 
        	{
				//cout<<"\n eject: "<<f->id;
				if ( f->watch ) 
				{
						cout << "ejected flit " << f->id << " at output " << output << endl;
						cout << "sending credit for " << f->vc << endl;
				}
			
				cred = new Credit( 1 );
				cred->vc[0] = f->vc;
				cred->vc_cnt = 1;
				cred->dest_router = f->from_router;
				_net[i]->WriteCredit( cred, output );
				_RetireFlit( f, output );
				_accepted_packets[output]->AddSample( 1 );
                                // added by Plotfi
                                ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_SumSample[output] += 1;
                                ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_NoSample[output] ++;
                                // end Plotfi

		} 
		else 
		{
				_net[i]->WriteCredit( 0, output );
				_accepted_packets[output]->AddSample( 0 );
                                // added by Plotfi
                                ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_SumSample[output] += 0;
                                ((BooksimStats *)global_theBooksimStats)->AcceptedPackets_NoSample[output] ++;
                                // end Plotfi
		}
	}
        if(!NiCBuff[i][output].isempty())//NiCBuff[1][NumberOfNodes]
	{
		//cout<<"\n -- @"<<output<<" size "<< NiCBuff[i][output].size <<"\n";
		Book2Flex_IsFull[i][output]=true;
		Book2Flex[i][output]= *(NiCBuff[i][output].pop());
		
		//BookPacket* temp= NiCBuff[i][output].pop();
		//cout<<"\n ++ @"<<output<<" size "<< NiCBuff[i][output].size <<"\n";
		
	}
    }
  }


}

bool TrafficManager::available(int source, int vc, int FlxSubnet) //check if it should return 0 when there are free slots for a new packet?
{
	source=source%16;
	bool value=0;
        for(int i=0;i<_classes;i++)
        	if(_partial_packets[source][i][FlxSubnet].max_size()>_partial_packets[source][i][FlxSubnet].size())
			value=1;
   

        if(value==0)
        {
 		cout<<"input queue at node number "<<source<<" is full, just FYI :)\n";
		//exit(0);
	}
        return value;
      

}

  

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrafficManager::_FlexInject(BookPacket ** PacketFromFlex, int IndexFromFlex){

  #ifdef FlexusDebug
	_time++;
  #endif

  int input;
 
  Flit   *f, *nf;
  Credit *cred;
  int    psize;


   /*BookPacket * p[4];
   p[0]=p[1]=p[2]=p[3]=0;*/
   
 
  for (  input = 0; input < _net[0]->NumSources( ); ++input )
  {
  //HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
	// mehdi: Receive credits and update the status of the downstram nodes
	for (int i = 0; i < duplicate_networks; ++i) 
	{
		cred = _net[i]->ReadCredit( input );// by mehdi:pick the head of the credit queue for the injection port
		if ( cred ) 
		{
		//by mehdi: now process it! _buf_state keeps the state of the buffer at the NI (injection), this function simply increases the credit of the VCs (a credit packet may contain credit for more than one VCs)
			_buf_states[input][i]->ProcessCredit( cred );
			delete cred;
		}	
	}
  }

int mg; 
/*cout<<"\n++++++++ before\n";
for( mg=0;mg<IndexFromFlex;mg++)
{
   cout<<PacketFromFlex[mg]->srcNode<<" "<<PacketFromFlex[mg]->destNode<<" "<<PacketFromFlex[mg]->priority<<"\n";
}
cout<<"++++++++-\n";
*/

for( mg=0;mg<IndexFromFlex;mg++)
{
	int minx=PacketFromFlex[mg]->srcNode;
        int miny=PacketFromFlex[mg]->destNode;
	int minp=PacketFromFlex[mg]->priority;
 
	int minind=-1;
	for(int mm=mg;mm<IndexFromFlex;mm++)
	{
		if(PacketFromFlex[mm]->srcNode < minx)
		{
			minx=PacketFromFlex[mm]->srcNode;
                        miny=PacketFromFlex[mm]->destNode;
			minp=PacketFromFlex[mm]->priority;
			minind=mm;
		}
		else
		{
			if(PacketFromFlex[mm]->srcNode == minx)
			{
				if( PacketFromFlex[mm]->destNode  <  miny )
				{
					//minx=PacketFromFlex[mm]->srcNode;
                                        miny=PacketFromFlex[mm]->destNode;
					minp=PacketFromFlex[mm]->priority;
					minind=mm;
				}
				else
				{
					if(PacketFromFlex[mm]->destNode  ==  miny)
					{
						if( PacketFromFlex[mm]->priority  >  minp )
						{
							minx=PacketFromFlex[mm]->srcNode;
							miny=PacketFromFlex[mm]->destNode;
							minp=PacketFromFlex[mm]->priority;
							minind=mm;
						}	
					}
				}
			}			
		}	
	}
        if(minind==-1)
		continue;

        if(mg>IndexFromFlex || minind>IndexFromFlex)
	{
		cout<<"IndexFromFlex: out of range index! ";exit(0);
	}

        
	BookPacket * temp = PacketFromFlex[mg];
	PacketFromFlex[mg]  = PacketFromFlex[minind];
	PacketFromFlex[minind]  = temp;
        //delete temp;
}

/*cout<<"\n-------- after:\n";
for( mg=0;mg<IndexFromFlex;mg++)
{
   cout<<PacketFromFlex[mg]->srcNode<<" "<<PacketFromFlex[mg]->destNode<<" "<<PacketFromFlex[mg]->priority<<"\n";
}
cout<<"--------\n";
cout.flush();*/

 for (  input = 0; input < IndexFromFlex; ++input ) 
  {
  	bool generated;
        
	BookPacket * SendQ[MaxSameSandD];
        
        SendQ[0] = PacketFromFlex[input];
        
        int mg=1;
	int mm=input+1;
        /*if(mm >= IndexFromFlex)
		continue;*/
        while(SendQ[0]->srcNode == PacketFromFlex[mm]->srcNode  &&   SendQ[0]->destNode == PacketFromFlex[mm]->destNode && mm < IndexFromFlex)
	{
		SendQ[mg++]=PacketFromFlex[mm++];
	}
        
        input+=mg-1;
	if(mm> IndexFromFlex || mg>MaxSameSandD)
	{	
		cout<<" max: "<<IndexFromFlex<<" mm:"<<mm<<"\n";
		cout<<" max: "<<MaxSameSandD<<" mg:"<<mg<<"\n";
		cout<<"queue full!  :(\n";
		exit(0);
	}

/*	cout<<"\n-------- before:\n";
	for( int jj=0;jj<IndexFromFlex;jj++)
	{
	cout<<PacketFromFlex[jj]->srcNode<<" "<<PacketFromFlex[jj]->destNode<<" "<<PacketFromFlex[jj]->priority<<"\n";
	}
	cout<<"--------\n";
	for(int kk=0;kk<mg;kk++)
	{
		cout<<"--"<<SendQ[kk]->srcNode<<" "<<SendQ[kk]->destNode<<" "<<SendQ[kk]->priority<<"\n";
	}
*/       

	
        
	Flex_GeneratePacket( PacketFromFlex[input]->srcNode, PacketFromFlex[input]->destNode, 0, _time, (PacketFromFlex[input]->transmitLatency/8),    mg    , SendQ);
                 //(              source                 destination                          cl  time                 Psize                     no of payloads    PacketFromFlex   )
	generated = true;
	
	if ( generated ) 
	{
	  	//highest_class = c;
		class_array[sub_network][0]++; //mehdi: if classes is used, record one more packet for this class.
	}
  }


#ifdef echo
cout<<"\n";
for (  input = 0; input < IndexFromFlex; ++input ) 
  {	
	cout<<"BookReport:  S: "<<PacketFromFlex[input]->srcNode<<" D: "<<PacketFromFlex[input]->destNode<<" P#: "<<PacketFromFlex[input]->transmitLatency<<" #: "<<PacketFromFlex[input]->serial<<" prio: "<<PacketFromFlex[input]->priority<<" vc#: "<<PacketFromFlex[input]->networkVC <<" \n ";
  }
#endif

#ifdef NetLog
for (  input = 0; input < IndexFromFlex; ++input ) 
  {	
	TraceOut<<"- "<<_time<<" "<<PacketFromFlex[input]->serial<<" "<<PacketFromFlex[input]->srcNode<<" "<<PacketFromFlex[input]->destNode<<" "<<PacketFromFlex[input]->priority<<"\n";
	//TraceOut<<_time<<" "<<PacketFromFlex[input]->srcNode<<" "<<PacketFromFlex[input]->destNode<<" "<<PacketFromFlex[input]->transmitLatency<<"\n";
	//TraceOut<<_time<<" "<<PacketFromFlex[input]->srcNode<<" "<<PacketFromFlex[input]->destNode<<" "<<PacketFromFlex[input]->transmitLatency<<"\n";
  }
#endif

sorry_to_use_goto:

  for (  input = 0; input < _net[0]->NumSources(); ++input ) 
  {
	bool write_flit    = false;
   	int  highest_class = 0;

	for (int i = 0; i < duplicate_networks; ++i) {
	write_flit = false;
	highest_class = 0;
	
        //cout<<"\n----  "<<_partial_packets[input][highest_class][i].max_size();
        //cout<<"   -----   "<<_partial_packets[input][highest_class][i].size();

        if ( _partial_packets[input][highest_class][i].max_size() == _partial_packets[input][highest_class][i].size() )
	{	
		cout<<"queue full!  :(\n";
		exit(0);
	}
	if ( !_partial_packets[input][highest_class][i].empty( ) ) 
	{
		f = _partial_packets[input][highest_class][i].front( );
		if ( f->head && f->vc == -1) { // Find first available VC
	
		if ( _voqing ) {//mehdi: it checks output queueing
		if ( _buf_states[input][i]->IsAvailableFor( f->dest ) ) {
		f->vc = f->dest;
		}
		} else {
		f->vc = _buf_states[input][i]->FindAvailable( ); // mehdi: ( _buf_states keeps the Injection VC states) find a free vc at the PE-port of the router
		}
		
		if ( f->vc != -1 ) {
		_buf_states[input][i]->TakeBuffer( f->vc );// mehdi: mark it as busy/taken
		}
		}
	
		if ( ( f->vc != -1 ) &&	( !_buf_states[input][i]->IsFullFor( f->vc ) ) ) 
		{// mehdi: if a vc is taken and corresponding buffer has empty slots
	
			_partial_packets[input][highest_class][i].pop_front( );
			_buf_states[input][i]->SendingFlit( f );//mehdi: decrease the free buffers by one
		
			write_flit = true;
		
			// Pass VC "back"
			if ( !_partial_packets[input][highest_class][i].empty( ) && !f->tail ) 
			{// mehdi: pass VC number back to body flits, but if current flit is tail no need do this!
				nf = _partial_packets[input][highest_class][i].front( );
				nf->vc = f->vc;
			}
		}
	} 
	
	_net[i]->WriteFlit( write_flit ? f : 0, input );
	if (write_flit && f->tail) // If a tail flit, reduce the number of packets of this class.
		class_array[i][highest_class]--;
	}
  }

  
 //HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

}

