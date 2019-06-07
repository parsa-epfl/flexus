// DO-NOT-REMOVE begin-copyright-block 
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian 
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic, 
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason 
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex 
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block


#ifndef FLEXUS_MULTI_NIC_NUMPORTS
#error "FLEXUS_MULTI_NIC_NUMPORTS must be defined"
#endif

#include <core/simulator_layout.hpp>
#include <boost/preprocessor/repeat.hpp>

#define MultiNicX BOOST_PP_CAT(MultiNic, FLEXUS_MULTI_NIC_NUMPORTS)

#define FLEXUS_BEGIN_COMPONENT MultiNicX
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/CommonQEMU/Transports/MemoryTransport.hpp>

#define NodeOutputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, BOOST_PP_CAT(ToNode,N) )
#define NodeInputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, BOOST_PP_CAT(FromNode,N) )

COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, uint32_t, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, uint32_t, "Send Queue Capacity", "send-capacity", 1)
  // Msutherl: RMC port for fake multinode
  FLEXUS_PARAMETER( MachineCount, uint32_t, "Number of machines on the same \"node\"", "MachineCount", 1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryTransport, ToNetwork )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryTransport, FromNetwork )
  BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, NodeOutputPort, _ )
  BOOST_PP_REPEAT(FLEXUS_MULTI_NIC_NUMPORTS, NodeInputPort, _ )
  DRIVE( MultiNicDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MultiNicX
