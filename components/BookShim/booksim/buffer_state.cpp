// $Id: buffer_state.cpp 119 2008-12-12 03:06:32Z razzfazz $

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

/*buffer_state.cpp
 *
 * This class is the buffere state of the next router down the channel
 * tracks the credit and how much of the buffer is in use 
 */

#include "booksim.hpp"
#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include "buffer_state.hpp"
#include "random_utils.hpp"

BufferState::BufferState( const Configuration& config ) :
  Module( )
{
  _Init( config );
}

BufferState::BufferState( const Configuration& config, 
			  Module *parent, const string& name ) : 
  Module( parent, name )
{
  _Init( config );
}

void BufferState::_Init( const Configuration& config )
{
  _buf_size     = config.GetInt( "vc_buf_size" );
  _vcs          = config.GetInt( "num_vcs" );
  
  _wait_for_tail_credit = config.GetInt( "wait_for_tail_credit" );

  _in_use       = new bool [_vcs];
  _tail_sent    = new bool [_vcs];
  _cur_occupied = new int  [_vcs];

  _last_avail   = 0;

  for ( int v = 0; v < _vcs; ++v ) {
    _in_use[v]       = false;
    _tail_sent[v]    = false;
    _cur_occupied[v] = 0;
  }

  /* each flit is given a type and these types can only exists in 
   * specific virtual channels
   */
  _vc_range_begin[Flit::READ_REQUEST] 
    = config.GetInt( "read_request_begin_vc" );
  _vc_range_end[Flit::READ_REQUEST] 
    = config.GetInt( "read_request_end_vc" );
  
  _vc_range_begin[Flit::WRITE_REQUEST] 
    = config.GetInt( "write_request_begin_vc" );
  _vc_range_end[Flit::WRITE_REQUEST] 
    = config.GetInt( "write_request_end_vc" );
  
  _vc_range_begin[Flit::READ_REPLY] 
    = config.GetInt( "read_reply_begin_vc" );
  _vc_range_end[Flit::READ_REPLY] 
    = config.GetInt( "read_reply_end_vc" );
  
  _vc_range_begin[Flit::WRITE_REPLY] 
    = config.GetInt( "write_reply_begin_vc" );
  _vc_range_end[Flit::WRITE_REPLY] 
    = config.GetInt( "write_reply_end_vc" );
  
  _vc_range_begin[Flit::ANY_TYPE] = 0 ;
  _vc_range_end[Flit::ANY_TYPE]   = _vcs - 1 ;
  
}

BufferState::~BufferState( )
{
  delete [] _in_use;
  delete [] _tail_sent;
  delete [] _cur_occupied;
}

void BufferState::ProcessCredit( Credit *c )
{
/*by mehdi:  c->vc_cnt is the number of credits in the current credit packet (may combine some credits inside a single packet) nad each credit is put into one of the entries of vc[x], so vc[x]=v where v is the vc number of the the x-th.credit carried by this packet. see for example the _Normal_nject() function at trafficmanager.cpp to understand how to synthesize such credit packet
*/
 
  assert( c );

  for ( int v = 0; v < c->vc_cnt; ++v ) {
    assert( ( c->vc[v] >= 0 ) && ( c->vc[v] < _vcs ) );

    if ( ( _wait_for_tail_credit ) && 
	 ( !_in_use[c->vc[v]] ) ) {
      Error( "Received credit for idle buffer" );
    }

    if ( _cur_occupied[c->vc[v]] > 0 ) {
      --_cur_occupied[c->vc[v]];

      if ( ( _cur_occupied[c->vc[v]] == 0 ) && 
	   ( _tail_sent[c->vc[v]] ) ) {
	_in_use[c->vc[v]] = false;
      }
    } else {
      cout << "VC = " << c->vc[v] << endl;
      Error( "Buffer occupancy fell below zero" );
    }
  }
}


void BufferState::SendingFlit( Flit *f )// mehdi: sends a flit to the in buffer of PE-port, actually only reduces the free buufers by one
{
  assert( f && ( f->vc >= 0 ) && ( f->vc < _vcs ) );

  if ( _cur_occupied[f->vc] < _buf_size ) {
    ++_cur_occupied[f->vc];

    if ( f->tail ) {
      _tail_sent[f->vc] = true;

      if ( !_wait_for_tail_credit ) {// mehdi: the functionality of this if-statement must be investigated
	_in_use[f->vc] = false;
      }
    }
  } else {
    Error( "Flit sent to full buffer" );
  }
}

void BufferState::TakeBuffer( int vc )// mehdi: set the vc state as busy or taken to reserve it for a packet
{
  assert( ( vc >= 0 ) && ( vc < _vcs ) );

  if ( _in_use[vc] ) {
    cout << "TakeBuffer( " << vc << " )" << endl;
    Display( );
    Error( "Buffer taken while in use" );
  }
  _in_use[vc]    = true;
  _tail_sent[vc] = false;
}

bool BufferState::IsFullFor( int vc  ) const
{
  assert( ( vc >= 0 ) && ( vc < _vcs ) );
  return ( _cur_occupied[vc] == _buf_size ) ? true : false;
}

bool BufferState::IsAvailableFor( int vc ) const// mehdi: returns 1 if the vc is free, otherwise 0
{
 
  assert( ( vc >= 0 ) && ( vc < _vcs ) );
  return !_in_use[vc];
}

int BufferState::FindAvailable( )// mehdi: returns a free vc in a round-roubin-based fashion
{
  int available_vc = -1;
  int vc;

  _last_avail = RandomInt( _vcs - 1 );

  for ( int v = 0; v < _vcs; ++v ) {
    vc = ( v + _last_avail + 1 ) % _vcs; // Round-robin

    if ( IsAvailableFor( vc ) ) {
      available_vc = vc;
      _last_avail  = vc;
      break;
    }
  }

  return available_vc;
}

int BufferState::FindAvailable( Flit::FlitType type )// mehdi: this function is never used in the simulator
{
  int available_vc = -1;
  int vc = -1;

  for (vc = _vc_range_begin[type]; vc <= _vc_range_end[type]; vc++) {
    if ( IsAvailableFor(vc) ){
      available_vc = vc;
      break;
    }
  }

  return available_vc;
}

int BufferState::Size(int vc) const{
  assert( ( vc >= 0 ) && ( vc < _vcs ) );

  return  _cur_occupied[vc];
}

void BufferState::Display( ) const
{
  cout << _fullname << " :" << endl;
  for ( int v = 0; v < _vcs; ++v ) {
    cout << "  buffer class " << v << endl;
    cout << "    in_use = " << _in_use[v] 
	 << " tail_sent = " << _tail_sent[v] << endl;
    cout << "    occupied = " << _cur_occupied[v] << endl;
  }

  for ( int f = 0; f < Flit::NUM_FLIT_TYPES; ++f) {
    cout << "vc_range[" << f << "] = [" << _vc_range_begin[f] 
	 << "," <<  _vc_range_end[f] << "]" << endl;
  }
}
