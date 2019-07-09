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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
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

// clang-format off
#define FLEXUS_BEGIN_COMPONENT armDecoder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/uFetch/uFetchTypes.hpp>
#include <components/CommonQEMU/Slices/AbstractInstruction.hpp>

typedef std::pair < int32_t /*# of instruction */ , bool /* core synchronized? */ > dispatch_status;

COMPONENT_PARAMETERS(
  PARAMETER( FIQSize, uint32_t, "Fetch instruction queue size", "fiq", 32 )
  PARAMETER( DispatchWidth, uint32_t, "Maximum dispatch per cycle", "dispatch", 8 )
  PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )
);

COMPONENT_INTERFACE(
  PORT( PushInput, pFetchBundle, FetchBundleIn)
  PORT( PullOutput, int32_t, AvailableFIQOut)
  PORT( PullInput, dispatch_status, AvailableDispatchIn)
  PORT( PushOutput, boost::intrusive_ptr< AbstractInstruction>, DispatchOut)
  PORT( PushOutput, eSquashCause, SquashOut)
  PORT( PushInput, eSquashCause, SquashIn)
  PORT( PullOutput, int32_t, ICount)
  PORT( PullOutput, bool, Stalled)

  PORT( PushOutput, int64_t, DispatchedInstructionOut) // Send instruction word to Power Tracker

  DRIVE( DecoderDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT armDecoder
// clang-format on
