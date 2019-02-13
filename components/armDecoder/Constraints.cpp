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


#include <iostream>
#include <iomanip>
#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <functional>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "SemanticInstruction.hpp"
#include "Effects.hpp"
#include "Constraints.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using nuArchARM::SemanticAction;
using nuArchARM::eResourceStatus;
using nuArchARM::kReady;

using nuArchARM::kSC;
using nuArchARM::kTSO;

bool checkStoreQueueAvailable( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  if ( anInstruction->core()->sbFull()) {
    return false;
  }
  return true;
}

std::function<bool()> storeQueueAvailableConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkStoreQueueAvailable(anInstruction); };
}

bool checkMembarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStSt();
}

std::function<bool()> membarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkMembarStoreStoreConstraint(anInstruction); };
}

bool checkMembarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStLd();
}

std::function<bool()> membarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkMembarStoreLoadConstraint(anInstruction); };
}

bool checkMembarSyncConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARSync();
}

std::function<bool()> membarSyncConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkMembarSyncConstraint(anInstruction); };
}

bool checkMemoryConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  switch (anInstruction->core()->consistencyModel() ) {
    case kSC:
      if (! anInstruction->core()->speculativeConsistency()) {
        //Under nonspeculative SC, a load instruction may only retire when no stores are outstanding.
        if ( ! anInstruction->core()->sbEmpty()) {
          return false;
        }
      }
      break;
    case kTSO:
    case kRMO:
      //Under TSO and RMO, a load may always retire when it reaches the
      //head of the re-order buffer.
      break;
    default:
      DBG_Assert( false, ( << "Load Memory Instruction does not support consistency model " << anInstruction->core()->consistencyModel() ) );
  }
  return true;
}

std::function<bool()> loadMemoryConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkMemoryConstraint(anInstruction); };
}

bool checkStoreQueueEmpty( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->sbEmpty();
}

std::function<bool()> storeQueueEmptyConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkStoreQueueEmpty; };
}

bool checkSideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->checkStoreRetirement(boost::intrusive_ptr<nuArchARM::Instruction>(anInstruction));
}

std::function<bool()> sideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  return [anInstruction](){ return checkSideEffectStoreConstraint(anInstruction); };
}

bool checkpaddrResolutionConstraint(SemanticInstruction * anInstruction){
    return anInstruction->isResolved();
}
std::function<bool()> paddrResolutionConstraint( SemanticInstruction * anInstruction ) {
    return [anInstruction](){ return checkpaddrResolutionConstraint(anInstruction); };
}

} //armDecoder
