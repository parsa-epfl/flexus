// $Id: trafficmanager.hpp 132 2009-01-22 01:29:04Z razzfazz $

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

#ifndef _TRAFFICMANAGER_HPP_
#define _TRAFFICMANAGER_HPP_

#include <list>
#include <map>
#include <fstream>
#include "booksim.hpp"

#include "module.hpp"
#include "config_utils.hpp"
#include "network.hpp"
#include "flit.hpp"
#include "buffer_state.hpp"
#include "stats.hpp"
#include "traffic.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"
#include "injection.hpp"
#include <assert.h>

#include "orion.hpp"


//Flexus-specific part--added by mehdi   
#define MaxPacketPerCycle 30
#define NumberOfNodes 16


class BookPacket; 
struct TraceLine
{
	
	 long int T;
	 int S;
	 int D;
	 int P;

};
//end of mehdi	

//register the requests to a node
struct Packet_Reply {
  int source;
  int time;
  Flit::FlitType type;
};

class TrafficManager : public Module {
//protected:
public:
  int _sources;
  int _dests;

  Network **_net;




  // ============ Message priorities ============ 

  enum ePriority { class_based, age_based, none };

  ePriority _pri_type;
  int       _classes;

  // ============ Injection VC states  ============ 

  BufferState ***_buf_states;

  // ============ Injection queues ============ 

  int          _voqing;
  int          **_qtime;
  bool         **_qdrained;
  list<Flit *> ***_partial_packets;

  int                 _measured_in_flight;
  int                 _total_in_flight;
  map<int,bool> _in_flight;
  bool                _empty_network;
  bool _use_lagging;

  // ============ sub-networks and deadlock ==========

  short duplicate_networks;
  unsigned char deadlock_counter;

  // ============ batch mode ==========================
  int *_packets_sent;
  int _batch_size;
  list<int>* _repliesPending;
  map<int,Packet_Reply*> _repliesDetails;
  int * _requestsOutstanding;
  int _maxOutstanding;

  // ============voq mode =============================
  list<Flit*> ** _voq;
  list<int>* _active_list;
  bool** _active_vc;
  
  // ============ Statistics ============

  Stats **_latency_stats;     
  Stats **_overall_latency;  

  Stats **_pair_latency;
  Stats *_hop_stats;

  Stats **_accepted_packets;
  Stats *_overall_accepted;
  Stats *_overall_accepted_min;

  // ============ Simulation parameters ============ 

  enum eSimState { warming_up, running, draining, done };
  eSimState _sim_state;

  //modified by mehdi
  enum eSimMode { latency, throughput, batch, Flexmode };
  //original
  //enum eSimMode { latency, throughput, batch };

  eSimMode _sim_mode;

  int   _limit; //any higher clients do not generate packets

  int   _warmup_time;
  int   _drain_time;

  float _load;
  float _flit_rate;

  int   _packet_size;
  int _read_request_size;
  int _read_reply_size;
  int _write_request_size;
  int _write_reply_size;

  int   _total_sims;
  int   _sample_period;
  int   _max_samples;
  int   _warmup_periods;
  short ** class_array;
  short sub_network;

  int   _include_queuing;

  double _latency_thres;

  float _internal_speedup;
  float *_partial_internal_cycles;

  int _cur_id;
  int _time;

  list<Flit *> _used_flits;
  list<Flit *> _free_flits;

  tTrafficFunction  _traffic_function;
  tRoutingFunction  _routing_function;
  tInjectionProcess _injection_process;

  map<int,bool> flits_to_watch;

  bool _print_csv_results;
  string _traffic;
  bool _drain_measured_only;


  //by mehdi : Flexus specific
  //-------------------------------------------------
  double 	TimeScale; 
  long    	last_packet_time;
  TraceLine 	TL[MaxPacketPerCycle];
  int 		CurrentPackets;
  long   	FlxPacketRcv, FlxFlitRcv, FlxPacketSnd, FlxFlitSnd;

  int Packet_length_ctrl, Packet_length_data;

  #ifdef pointer_def
  BookPacket *** 	Book2Flex;
  bool ** 		Book2Flex_IsFull;
  #else
  BookPacket   		Book2Flex[1][NumberOfNodes];
  BookPacketQueue   	NiCBuff[1][NumberOfNodes];
  bool         		Book2Flex_IsFull[1][NumberOfNodes];
  #endif

  ifstream 		flexin;//("InTrace");
  ofstream TraceOut;//("/home/modarres/trace_Flexus.txt");
  ofstream StatOut;//("/home/modarres/trace_Flexus.txt");

  void DeliverData(long int data  );

  bool available(int node, int vc, int FlxSubnet); //check if it should return 0 when there are free slots for a new packet?
  int Flxus_PushPacket(int FlxNode, int FlxClass, int FlxSubnet);
  int Flxus_DeliverPacket(int FlxNode, int FlxClass, int FlxSubnet);


  long cntr, cntr2;

  //int Flxus_IsNodeAvailable();
  //end of mehdi
  //-----------------------------------------------------

  // ============ Internal methods ============ 
//protected:
  virtual Flit *_NewFlit( );
  virtual void _RetireFlit( Flit *f, int dest );

  void _FirstStep( );
  void _NormalInject();
  void _BatchInject();
  //
  
  //void Flex_GeneratePacket( int source, int packet_destination, int cl, int time, int Psize, int NoPacket, BookPacket * PacketFromFlex, BookPacket * PacketFromFlex_p2, BookPacket * PacketFromFlex_p3);

  void Flex_GeneratePacket( int source, int packet_destination, int cl, int time, int Psize );
  void Flex_GeneratePacket( int source, int packet_destination, int cl, int time, int Psize, int payload_length, BookPacket ** PacketFromFlex );
//
  void _Step(BookPacket ** PacketFromFlex, int IndexFromFlex, int FlexTime );
  void _Step();

  bool _PacketsOutstanding( ) const;
  
  virtual int  _IssuePacket( int source, int cl ) const;
  virtual void _GeneratePacket( int source, int size, int cl, int time );

  void _ClearStats( );

  int  _ComputeAccepted( double *avg, double *min ) const;

  virtual bool _SingleSim( );

  int DivisionAlgorithm(int packet_type);

  void _DisplayRemaining( ) const;
  
  void _LoadWatchList();

public:
  TrafficManager( const Configuration &config, Network **net );
  ~TrafficManager( );

  void _FileInject();//Flexus-specific method--added by mehdi   
  void _FlexInject(BookPacket ** PacketFromFlex, int IndexFromFlex);
  bool Run( );

  void DisplayStats();

  const Stats * GetOverallLatency(int c) { return _overall_latency[c]; }
  const Stats * GetAccepted() { return _overall_accepted; }
  const Stats * GetAcceptedMin() { return _overall_accepted_min; }
  const Stats * GetHops() { return _hop_stats; }
  int just_for_test(int i)
  {
    return i; //by mehdi
  }

  int getTime() { return _time;}
};

//////////////////////////////




//////////////////////////////
#endif
