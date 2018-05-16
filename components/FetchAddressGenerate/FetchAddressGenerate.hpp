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


#include <core/simulator_layout.hpp>

#include <components/uFetch/uFetchTypes.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#if FLEXUS_TARGET_IS(ARM)
typedef Flexus::SharedTypes::VirtualMemoryAddress vaddr_pair;
#elif FLEXUS_TARGET_IS(v9)
typedef std::pair<Flexus::SharedTypes::VirtualMemoryAddress, Flexus::SharedTypes::VirtualMemoryAddress> vaddr_pair;
#endif
COMPONENT_PARAMETERS(
  PARAMETER( MaxFetchAddress, uint32_t, "Max fetch addresses generated per cycle", "faddrs", 10 )
  PARAMETER( MaxBPred, uint32_t, "Max branches predicted per cycle", "bpreds", 2 )
  PARAMETER( Threads, uint32_t, "Number of threads under control of this FAG", "threads", 1 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, vaddr_pair, RedirectIn )
  DYNAMIC_PORT_ARRAY( PushInput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackIn )
  DYNAMIC_PORT_ARRAY( PushOutput, boost::intrusive_ptr<FetchCommand>, FetchAddrOut )
  DYNAMIC_PORT_ARRAY( PullInput, int, AvailableFAQ )
  DYNAMIC_PORT_ARRAY( PullOutput, bool, Stalled)
  DRIVE( FAGDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

