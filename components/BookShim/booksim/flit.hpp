// $Id: flit.hpp 119 2008-12-12 03:06:32Z razzfazz $

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

#ifndef _FLIT_HPP_
#define _FLIT_HPP_

#include "booksim.hpp"
#include <iostream>
#define MaxSameSandD 20


struct BookPacket
{
	bool valid;
	int srcNode;
	int destNode;
	int priority;
	int networkVC;
	int nextHop;   // Output port for the next hop
	int nextVC;
	int transmitLatency;
	bool flexusInFastMode;
	int serial;
	
	// Statistics follow below...
	int hopCount;       /* Number of hops that the message has taken */
	
	long long bufferTime;
	
	long long atHeadTime;     /* Number of cycles that the message has been at the
				* head of a queue.  This can tell us about arbitration
				* contention inside a switch.
				*/
	long long acceptTime;     /* Number of cycles that the message has been at the
				* head of an output queue.  This can tell us if we have
				* channel contention/hot spot channels.
				*/
	long long startTS;  /* When did this message enter the network? */
    // CMU-ONLY-BLOCK-BEGIN
};



struct Flit {

  const static int NUM_FLIT_TYPES = 5;
  enum FlitType { READ_REQUEST  = 0, 
		  READ_REPLY    = 1,
		  WRITE_REQUEST = 2,
		  WRITE_REPLY   = 3,
                  ANY_TYPE      = 4 };
  FlitType type;

  int vc;

  bool head;
  bool tail;
  bool true_tail;
  
  int  time;

  int  sn;
  int  rob_time;

  int  id;
  bool record;

  int  src;
  int  dest;
  //int destFlex;//by mehdi

  int  pri;

  int  hops;
  bool watch;
  short subnetwork;

  //for credit tracking, last router visited
  mutable int from_router;

  // Fields for multi-phase algorithms
  mutable int intm;
  mutable int ph;

  //reservation
  mutable int delay;

  mutable int dr;
  mutable int minimal; // == 1 minimal routing, == 0, nonminimal routing

  // Which VC parition to use for deadlock avoidance in a ring
  mutable int ring_par;

  // Fileds for XY or YX randomized routing
  mutable int x_then_y;

  // Fields for arbitrary data
  //modified by mehdi
  //void* data ;
  unsigned long  data;
  int PLlength; //how many payloads are integrated into this packet.
  BookPacket ** FlexData;//added by mehdi
 

  // Constructor
  Flit() ;
};



class BookPacketQueue
{
public:
	BookPacket* Q[MaxSameSandD];
	BookPacketQueue();
	void init(int maxs);
	int MaxSize;
	int rind;
	int wind;
	int size;
	void push(BookPacket*);
	BookPacket* pop();
	int isfull();
	int isempty();
	
};


ostream& operator<<( ostream& os, const Flit& f );

#endif
