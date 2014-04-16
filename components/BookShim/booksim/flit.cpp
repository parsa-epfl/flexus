// $Id: flit.cpp 119 2008-12-12 03:06:32Z razzfazz $

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

/*flit.cpp
 *
 *flit struct is a flit, carries all the control signals that a flit needs
 *Add additional signals as necessary. Flits has no concept of length
 *it is a singluar object.
 *
 *When adding objects make sure to set a default value in this constructor
 */

#include "booksim.hpp"
#include "flit.hpp"

ostream& operator<<( ostream& os, const Flit& f )
{
  os << "  Flit ID: " << f.id << " (" << &f << ")" 
     << " Type: " << f.type 
     << " Head: " << f.head << " Tail: " << f.tail << endl;
  os << "  Source : " << f.src << "  Dest : " << f.dest << " Intm: "<<f.intm<<endl;
  os << "  Injection time : " << f.time << " Delay: "<<f.delay<<" Phase: "<<f.ph<< endl;
  os << "  From router "<<f.from_router<< endl;
  return os;
}

Flit::Flit() 
{  
  type      = ANY_TYPE ;
  vc        = -1 ;
  delay     = 0;
  head      = false ;
  tail      = false ;
  true_tail = false ;
  time      = -1 ;
  sn        = 0 ;
  rob_time  = 0 ;
  id        = -1 ;
  hops      = 0 ;
  watch     = false;//true;//false;//true;//false ;
  record    = false ;
  intm = 0;
  src = -1;
  dest = -1;
  pri = 0;
  intm =-1;
  ph = -1;
  dr = -1;
  minimal = 1;
  ring_par = -1;
  x_then_y = -1;
  data = 1;
  from_router = -1;
  FlexData=0;//new BookPacket;//added by mehdi
  //destFlex=-1;
   
}


/*class BookPacketQueue
{
private:
	BookPacket* Q[MaxSameSandD];
	BookPacketQueue();
	int rid, wind, size;
	int push(BookPacket*);
	int BookPacket* pop();
	int isfull();
	int isempty();
	
};*/

BookPacketQueue::BookPacketQueue()
{
	size=wind=rind=0;
}
void BookPacketQueue::init(int sz)
{
	size=wind=rind=0;
	MaxSize=MaxSameSandD;
	for (int mg=0;mg<MaxSize;mg++)
		Q[mg]=new BookPacket;

}

int BookPacketQueue::isfull()
{

	if(size==MaxSize)
		return 1;
	else 
		return 0;
}

int BookPacketQueue::isempty()
{
	if(size==0)
		return 1;
	else 
		return 0;
}

void BookPacketQueue::push(BookPacket* bp)
{
	Q[wind]=bp;
	wind++;
	if(wind==MaxSize)
		wind=0;	
	size++;
}

BookPacket* BookPacketQueue::pop() 
{
	BookPacket* bp= Q[rind];
	rind++;
	size--;
	if(rind==MaxSize)
		rind=0;	
	return bp;
}



