// $Id: router.hpp 119 2008-12-12 03:06:32Z razzfazz $

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

#ifndef _ROUTER_HPP_
#define _ROUTER_HPP_

#include <string>
#include <vector>

#include "module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "flitchannel.hpp"
#include "creditchannel.hpp"
#include "config_utils.hpp"

#include "orion.hpp"

class Router : public Module {
public:
  int _id;

  
  int _inputs;
  int _outputs;
 
  int _input_speedup;
  int _output_speedup;

  int _routing_delay;
  int _vc_alloc_delay;
  int _sw_alloc_delay;
  int _st_prepare_delay;
  int _st_final_delay;

  int _credit_delay;


  vector<FlitChannel *>   *_input_channels;
  vector<CreditChannel *> *_input_credits;
  vector<FlitChannel *>   *_output_channels;
  vector<CreditChannel *> *_output_credits;
  vector<bool>            *_channel_faults;

  Credit *_NewCredit( int vcs = 1 );
  void    _RetireCredit( Credit *c );

public:
  Router( const Configuration& config,
	  Module *parent, string name, int id,
	  int inputs, int outputs );

  virtual ~Router( );

  static Router *NewRouter( const Configuration& config,
			    Module *parent, string name, int id,
			    int inputs, int outputs );

  void AddInputChannel( FlitChannel *channel, CreditChannel *backchannel );
  void AddOutputChannel( FlitChannel *channel, CreditChannel *backchannel );
 


  virtual void ReadInputs( ) = 0;
  virtual void InternalStep( ) = 0;
  virtual void WriteOutputs( ) = 0;

  void OutChannelFault( int c, bool fault = true );
  bool IsFaultyOutput( int c ) const;

  const string& Name() ;

  int GetID( ) const;


  virtual int GetCredit(int out, int vc_begin, int vc_end ) const = 0;
  virtual int GetBuffer(int i) const = 0;

  int NumOutputs(){return _outputs;}
  
  

  //Orion Power
  SIM_router_power_t router_power;
  SIM_router_info_t  router_info;
  double* link_power;

  int* LastCxbarIn;
  double LinkEnergy;
  double LinkStatic;
  double link_len; //link length in meter, per each port 
};

#endif
